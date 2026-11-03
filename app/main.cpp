#include"vcm/can_frame.hpp"
#include"vcm/can_socket.hpp"
#include"vcm/IsoTpReceiver.hpp"
#include"vcm/isotp_sender.hpp"
#include"vcm/IsoTpReceiver.hpp"
#include"vcm/signal_database.hpp"
#include"vcm/logger.hpp"
#include<iostream>
#include<chrono>
#include<thread>
#include<vector>
#include<iomanip>

constexpr uint32_t TX_ID = 0x7E0;
constexpr uint32_t RX_ID = 0x7E8;

void printData(std::vector<uint8_t>& data){
    for(auto b : data){
        std::cout<<std::setw(2)<<std::setfill('0')<<(int)b<< " ";
    }
    std::cout<<std::dec;
}

int main(){
    vcm::logger::instance().setLevel(vcm::LogLevel::INFO);
    vcm::logger::instance().openFile("vcm_system.log");
    LOG_INFO("  车载通信中间件系统启动中  ");

    std::string dbc_path = "../dbc/vehicle.json";
    SignalDatabase db;
    if(!db.load(dbc_path)){
        LOG_ERROR("加载信号数据库失败,路径" + dbc_path);
        return -1;
    }
    LOG_INFO("加载数据库成功");
    CanSocket tester_socket;//模拟诊断仪
    CanSocket ecu_socket;//模拟ecu
    if(!tester_socket.open("vcan0") || !ecu_socket.open("vcan0")){
        LOG_ERROR("can总线打开失败");
        return -1;
    }
    LOG_INFO("can总线打开成功");

    IsoTpSender sender(tester_socket,TX_ID,RX_ID);
    IsoTpReceiver receiver(ecu_socket,TX_ID,RX_ID);
    std::vector<uint8_t> diagnostic_request = {
        0x10, 0x14, 0x2E, 0xF1, 0x90, 0x31, 0x32, 
        0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x41, 0x42, 0x43, 0x44, 0x45, 0x46 
    };// 总共 20 字节
    LOG_INFO("[Tester]准备发送 " + std::to_string(diagnostic_request.size()) + " 字节诊断请求...");

    std::thread tester_sender([&](){
        if(!sender.send(diagnostic_request,500)){
            LOG_ERROR("诊断请求发送异常");
            
        }else{
            LOG_INFO("诊断请求发送成功");
        }
    });

    std::vector<uint8_t> out_payload{};
    LOG_INFO("[ECU] 正在监听Tester发送的数据");
    bool rx_success = receiver.receive(out_payload,500);
    if(rx_success){
        LOG_INFO("接收到了" +  std::to_string(out_payload.size()) + "个数据");
        std::string hex_dump = "[ECU] 数据内容: ";
        for (auto b : out_payload) {
            char buf[4];
            snprintf(buf, sizeof(buf), "%02X ", b);
            hex_dump += buf;
        }
        LOG_INFO(hex_dump);
                if (out_payload.size() >= 2) {
            std::array<uint8_t, 8> can_data = {0};
            can_data[0] = out_payload[0];
            can_data[1] = out_payload[1];
            
            auto speed = db.unpack("Speed", can_data);
            if (speed.has_value()) {
                LOG_INFO("[App] DBC 解析车速信号: " + std::to_string(speed.value()) + " km/h");
            }
    }else{
        LOG_ERROR("接收失败");
    }
    LOG_INFO("模拟结束安全退出");
    return 0;
  }
}