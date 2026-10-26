#include <iostream>
#include <iomanip>
#include <array>
#include "vcm/signal_database.hpp"

void printData(const std::array<uint8_t, 8>& data) {
    for (uint8_t byte : data) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    }
    std::cout << std::dec << "\n";
}

int main() {
    SignalDatabase db;
    
    // 注意：这要求在 app 目录下运行，或者在 vcm 根目录下运行
    // 如果你在 build 目录下运行，路径可能是 "../dbc/vehicle.json"
    std::string json_path = "dbc/vehicle.json"; 
    
    if (!db.load(json_path)) {
        std::cerr << "Failed to load " << json_path << "\n";
        std::cerr << "Make sure you are running from the vcm root directory.\n";
        return 1;
    }
    
    std::cout << "SignalDatabase loaded successfully.\n\n";

    // 场景 1: 模拟从 CAN 总线收到一帧数据
    // 假设收到了 0x34, 0x12, 0x64, 0, 0, 0, 0, 0
    std::array<uint8_t, 8> rx_data = {0x34, 0x12, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    std::cout << "Received CAN Frame: ";
    printData(rx_data);
    std::cout << "\n";

    // 解析车速
    auto speed = db.unpack("Speed", rx_data);
    if (speed) {
        std::cout << "Decoded Speed: " << speed.value() << " km/h\n";
    }

    // 解析温度
    auto temp = db.unpack("Temp", rx_data);
    if (temp) {
        std::cout << "Decoded Temp : " << temp.value() << " degC\n\n";
    }

    // 场景 2: 模拟要发送一帧数据
    std::array<uint8_t, 8> tx_data = {0}; // 清空发送缓冲区
    
    std::cout << "Packing new values...\n";
    // 设置车速为 88.88 km/h
    db.pack("Speed", 88.88, tx_data);
    // 设置温度为 25 degC
    db.pack("Temp", 25.0, tx_data);

    std::cout << "To Be Sent Frame:   ";
    printData(tx_data);

    return 0;
}