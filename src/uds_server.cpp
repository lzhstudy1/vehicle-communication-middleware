#include"vcm/uds_server.hpp"
#include<fstream>//从底层硬件获得熵随机数
#include<iostream>

UdsServer::UdsServer(IsoTpSender& sender, IsoTpReceiver& receiver):
    sender(sender), receiver(receiver){};

void UdsServer::handleRequest(const std::vector<uint8_t>& req){// 请求SID 子功能(可选) 数据
    last_active_time = std::chrono::steady_clock::now();
    if(req.size() == 0) return;
    uint8_t sid = req[0];
    switch(sid){
        case 0x10 :
            handle_0x10(req);
            break;
        case 0x3E :
            handle_0x3E(req);
            break;
        case 0x22 :
            handle_0x22(req);
            break;
        case 0x27 :
            handle_0x27(req);
            break;
        case 0x2E :
            handle_0x2E(req);
            break;
        case 0x31:
            handle_0x31(req);
            break;
        default :
            sendNRC(sid, 0x11);
    }
    return;
}

void UdsServer::registerReadDID(uint16_t did, std::function<std::vector<uint8_t>()> read_Fnc){
    readFnc[did] = read_Fnc;
}

void UdsServer::handle_0x10(const std::vector<uint8_t>& req){
    if(req.size() < 2) {
        sendNRC(req[0],0x13);
        return;
    }
    uint8_t sub_func = req[1] & 0x7F;
    switch(sub_func){
        case 0x01 :
        session = Session::DEFAULT;
        break;

        case 0x02 :
        session = Session::PROGRAMMING;
        break;

        case 0x03 :
        session = Session::EXTENDED;
        break;

        default:
        sendNRC(req[0], 0x12); 
        return;
    }
    std::vector<uint8_t> resp = {0x50, sub_func, 0x00,0x42,0x01,0xF4};
    sender.send(resp);
}

void UdsServer::handle_0x3E(const std::vector<uint8_t>& req){
    if(req.size() != 2){
         sendNRC(req[0],0x13);
         return;
    }
    uint8_t sub_func = req[1] & 0x7F;

    std::vector<uint8_t> resp = {0x7E,sub_func};
    sender.send(resp);
}

void UdsServer::handle_0x22(const std::vector<uint8_t>& req){

    if(req.size() != 3){
        sendNRC(req[0],0x13);
        return;
    }
    uint16_t Did = (req[1] << 8) | req[2];
    auto it = readFnc.find(Did);

    if(it == readFnc.end()){
        sendNRC(req[0],0x31);
        return;
    }
        std::vector<uint8_t> data = it->second();
        std::vector<uint8_t> resp;
        resp.push_back(0x62);
        resp.push_back(req[1]);
        resp.push_back(req[2]);
        resp.insert(resp.end(),data.begin(),data.end());
        sender.send(resp);
}

void UdsServer::sendNRC(uint8_t sid, uint8_t nrc){
    std::vector<uint8_t> resp = {0x7F,sid,nrc};
    sender.send(resp);
}

uint32_t UdsServer::getRandNum(){
    uint32_t seed = 0;
    std::ifstream urandom("/dev/urandom",std::ios::in|std::ios::binary);
    if(urandom){
        urandom.read(reinterpret_cast<char*>(&seed),sizeof(seed));
    }
    return seed;
}

bool UdsServer::isLockout(){
    if(failed_attempts < 3){
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lockout_time).count();

    if(duration < 10){
        return true;
    }else{
        failed_attempts = 0;
        return false;
    }
}

void UdsServer::checkS3Timeout(){
    if(session == Session::DEFAULT){
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>
    (now - last_active_time).count();

    if(duration > 5000){
        session = Session::DEFAULT;
        current_level = SecurityLevel::LEVEL_1;
        unlock_level1 = false;
        unlock_level3 = false;
        std::cout<<"[UDS] Session 3 timeout"<<std::endl;
    }
}

void UdsServer::registerKeyAlgorithm(SecurityLevel level, std::function<uint32_t(uint32_t)> algo){
    safeFnc[level] = algo;
}

void UdsServer::handle_0x27(const std::vector<uint8_t>& req){
    if(req.size() < 2){
        sendNRC(req[0],0x13);
        return;
    }

    uint8_t sub_func = req[1];
    bool is_req_seed = (sub_func % 2 != 0);//判断为奇数
    SecurityLevel level;
    if(sub_func == 0x01 || sub_func == 0x02){
        level = SecurityLevel::LEVEL_1;
    }else if(sub_func == 0x05 || sub_func == 0x06){
        level = SecurityLevel::LEVEL_3;
    }else{
        sendNRC(req[0],0x12);
        return;
    }

    if(isLockout()){
        sendNRC(req[0],0x36);
        return;
    }
    //请求种子
    if(is_req_seed){
        current_seed = getRandNum();
        std::vector<uint8_t> resp;
        resp.push_back(0x67);
        resp.push_back(sub_func);
        resp.push_back((current_seed >> 24) & 0xFF);
        resp.push_back((current_seed >> 16) & 0xFF);
        resp.push_back((current_seed >> 8) & 0xFF);
        resp.push_back(current_seed & 0xFF);
        sender.send(resp);
        return;
    }
    //发送密钥
    if(req.size() < 6){
        sendNRC(req[0],0x13);
        return;
    }

    if(current_seed == 0){
        sendNRC(req[0],0x24);
        return;
    }
    //提取发送的key
    uint32_t received_key = 0;
    for(size_t i = 2; i < req.size();i++){
        received_key = (received_key << 8);
        received_key |= req[i];
    }
    //找算法
    auto it = safeFnc.find(level);
    if(it == safeFnc.end()){
        sendNRC(req[0],0x11);
        return;
    }
    //通过算法计算到真实key
    uint32_t expect_key = it->second(current_seed);

    if(expect_key == received_key){
        if(level == SecurityLevel::LEVEL_1) unlock_level1 = true;
        if(level == SecurityLevel::LEVEL_3) unlock_level3 = true;
        failed_attempts = 0;
        current_seed = 0;
        std::vector<uint8_t> resp = {0x67,sub_func};
        sender.send(resp);
    }else{
        failed_attempts++;
        if(failed_attempts < 3){
            sendNRC(0x27,0x35);
        }else if(failed_attempts == 3){
            lockout_time = std::chrono::steady_clock::now();
            sendNRC(0x27,0x36);
            return;
        }
        current_seed = 0;
    }
}

int UdsServer::getLockMessage(){
    if(unlock_level1) return 1;
    if(unlock_level3) return 3;
    return 0;
}
void UdsServer::processLoop(){
    while(is_running){
        std::vector<uint8_t> req;
        if(receiver.receive(req,500)){
            handleRequest(req);
        }
        checkS3Timeout();
    }
}

void UdsServer::handle_0x2E(const std::vector<uint8_t>& req){
    if(session == Session::DEFAULT){
        sendNRC(req[0],0x33);
        return;
    }
    if(!unlock_level1 && !unlock_level3){
    sendNRC(req[0],0x33);
    return;
    }
    if(req.size() < 4){
        sendNRC(req[0],0x13);
        return;
    }
    uint16_t did = (req[1] << 8) | req[2];
    auto it = writeFnc.find(did);
    if(it == writeFnc.end()){
        sendNRC(req[0],0x31);
        return;
    }
    bool success = it->second(std::vector<uint8_t>(req.begin() + 3,req.end()));
   if(!success){
    sendNRC(req[0],0x22);
    return;
   }
   std::vector<uint8_t> resp = {0x6E,req[1],req[2]};
   sender.send(resp);
}

void UdsServer::handle_0x31(const std::vector<uint8_t>& req){
    if(req.size() < 4){
        sendNRC(req[0],0x13);
        return;
    }
    uint8_t sub_func = req[1];
    uint16_t rid = (req[2] << 8) | req[3];
    if(sub_func != 0x01 && sub_func != 0x02 && sub_func != 0x03){
        sendNRC(req[0],0x12);
        return;
    }

    auto it = routingFnc.find(rid);
    if(it == routingFnc.end()){
        sendNRC(req[0],0x31);
        return;
    }

    if(sub_func == 0x01 && routing_running){
        sendNRC(req[0],0x24);
        return;
    }

    std::vector<uint8_t> params = {req.begin() + 4,req.end()};

    auto res = it->second(sub_func,params);
   
    if(res.has_value()){
        if(sub_func == 0x01) routing_running = true;
        if(sub_func == 0x02) routing_running = false;

        std::vector<uint8_t> resp = {0x71,req[1],req[2],req[3]};
        std::vector<uint8_t> data = res.value();
        resp.insert(resp.end(),data.begin(),data.end());
        sender.send(resp);
    }else{
        sendNRC(req[0],0x22);
        return;
    }
}

void UdsServer::registerRouting(uint16_t rid, std::function<std::optional<std::vector<uint8_t>>
    (uint8_t sub_fnc, const std::vector<uint8_t>& req_data)> routing_func){
        routingFnc[rid] = routing_func;
}

void UdsServer::registerDIDWrite(uint16_t did, std::function<bool(const std::vector<uint8_t>&)> write_func){
    writeFnc[did] = write_func;
}
