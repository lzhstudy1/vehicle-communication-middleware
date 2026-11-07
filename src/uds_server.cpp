#include"vcm/uds_server.hpp"

UdsServer::UdsServer(IsoTpSender& sender, IsoTpReceiver& receiver):
    sender(sender), receiver(receiver){};

void UdsServer::handleRequest(const std::vector<uint8_t>& req){// 请求SID 子功能(可选) 数据
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
        default :
            sendNRC(sid, 0x11);
    }
}

void UdsServer::registerReadDID(uint16_t did, std::function<std::vector<uint8_t>()> readFnc){
    readDID[did] = readFnc;
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
    auto it = readDID.find(Did);

    if(it == readDID.end()){
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