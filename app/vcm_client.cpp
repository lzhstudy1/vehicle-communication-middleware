#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <thread>
#include <atomic>
#include <chrono>
#include <iomanip>
#include "vcm/can_socket.hpp"
#include "vcm/isotp_sender.hpp"
#include "vcm/IsoTpReceiver.hpp"

/*
    客户端：模拟一个类似于candump的工具
    在终端1发送字符串报文，然后终端2接收并且处理回发响应报文
    终端1：显示数据。
    客户端需要有让用户在没有主动推出之前一直运行
    期间有发送处理回发的逻辑所以需要保证原子性->atomic
    为了保证真实我必须模拟真实情况，我需要一个保活线程 —> keep_alive 每3秒回发一次0x3E
    终端发送字符,所以需要一个string -> uint8_t[] 的转换线程 -> parseHexString
    还需要一个NRC字典，解析回发的错误码的中文解释
    std::hex 与 map.at(key)
*/

const std::map<uint8_t , std::string> NRC_map = {
        {0x11, "服务不支持"},
        {0x12, "子功能不支持"},
        {0x13, "消息长度错误"},
        {0x22 , "条件不满足"},
        {0x24, "请求顺序错误"},
        {0x31, "请求超出范围"},
        {0x33,"无权限"},
        {0x35,"无效密钥"},
        {0x36,"尝试次数超限"},
        {0x78,"相应挂起"}
};

std::atomic<bool> keep_alive_running{true};
std::atomic<bool> need_keep_alive{false};

void keep_alive(IsoTpSender& tx){
        while(keep_alive_running){
                std::this_thread::sleep_for(std::chrono::seconds(3));
                if(need_keep_alive){
                        std::vector<uint8_t> msg = {0x3E,0x00};
                        tx.send(msg,100);
                }
        }
}

std::vector<uint8_t> parseHexString(std::string& str){
        std::vector<uint8_t> res;
        std::stringstream ss(str);
        std::string token;
        
        while(ss >> token){
                res.push_back(std::stoul(token,nullptr,16));
        }
        return res;
}

constexpr uint32_t TESTER_TX = 0x7E0;
constexpr uint32_t TESTER_RX = 0x7E8;

int main(){
        CanSocket socket;
        if(!socket.open("vcan0")){
                std::cerr<<"can总线打开失败\n";
                return -1;
        }
        IsoTpSender Tester_tx(socket,TESTER_TX,TESTER_RX);
        IsoTpReceiver Tester_rx(socket,TESTER_RX,TESTER_TX);

        std::thread keep_alive_thread(keep_alive, std::ref(Tester_tx));
        std::cout<<"----VCM 客户端启动----\n";
        std::cout<<"输入字符报文如(10 01) (22 F1 90) , 输入quit退出"<< std::endl;
        std::cout<<"输入keep off/keep on 退出或者保持保活"<<std::endl;

        std::string input;
        while(1){
                //发送报文
                std::getline(std::cin,input);
                if(input == "quit") break;
                if(input == "keep off") {
                        need_keep_alive = false;
                        continue;
                }
                if(input == "keep on") {
                        need_keep_alive = true;
                        continue;
                }
                std::vector<uint8_t> msg;
                try{
                        msg = parseHexString(input);
                }catch(...){
                        std::cout << "输入格式并非十六进制字符串\n";
                        continue;
                }
                if(Tester_tx.send(msg,1000)){
                        std::vector<uint8_t> resp;
                        //接收报文
                        if(Tester_rx.receive(resp,1000)){
                                if(resp.size() > 3 && resp[0] == 0x7F){//收到报错nrc
                                        uint8_t nrc = resp[2];
                                        std::cout<<"[NRC] :" << std::hex << (int)nrc << std::dec;
                                        auto it = NRC_map.find(nrc);
                                        if(it != NRC_map.end()){
                                                std::cout<<"( " << it->second << " )"<<std::endl;
                                        }
                                }else{//受到正常报文
                                        for(auto& b : resp){
                                                std::cout<<std::setw(2)<<std::setfill('0')<<(int)b<<" ";
                                        }
                                        std::cout<<std::dec<<std::endl;
                                        if(resp[0] == 0x62 && resp.size() > 3){//接收到需要读取的数据
                                                std::string ASCIIData(resp.begin()+3, resp.end());
                                                std::cout<<"[ASCII] : " << ASCIIData<<std::endl;
                                        }
                                }    
                        }else{
                                std::cout<<"未接收到报文\n";
                        }
                }
        }
        keep_alive_running = false;
        keep_alive_thread.join();
        return 0;
}