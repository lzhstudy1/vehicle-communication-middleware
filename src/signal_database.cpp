#include "vcm/signal_database.hpp"
#include "nlohmann/json.hpp"
#include <iostream>
#include <fstream>
#include <cmath> // 必须包含，用于 std::llround

using json = nlohmann::json;

bool SignalDatabase::load(const std::string& json_path) {
    std::ifstream ifs(json_path);
    if (!ifs.is_open()) {
        std::cerr << "文件打不开: " << json_path << "\n";
        return false;
    }
    
    json j;
    try {
        ifs >> j;
    } catch (const std::exception& e) {
        std::cerr << "JSON 解析失败: " << e.what() << "\n";
        return false;
    }

    messages_.clear();
    signals_.clear();

    // 遍历所有消息
    for (auto& msg : j["messages"]) {
        MessageInfo m;
        m.id = msg["id"];
        m.name = msg["name"];
        m.length = msg["length"];
        
        // 遍历该消息下的所有信号
        // 注意：如果你的 Python 脚本生成的键名是 "signal"，请把这里的 "signals" 改掉
        for (auto& sig_json : msg["signals"]) { 
            SignalInfo sig;
            sig.name = sig_json["name"];         // hpp 中是 name
            sig.start = sig_json["start_bit"];   // hpp 中是 start
            sig.length = sig_json["length"];
            
            // 字符串转枚举
            std::string bo = sig_json["byte_order"];
            sig.byte_order = (bo == "intel") ? ByteOrder::Intel : ByteOrder::Motorola;
            
            sig.is_signed = sig_json["is_signed"];
            sig.factor = sig_json["factor"];
            sig.offset = sig_json["offset"];
            sig.min_num = sig_json["min"];       // hpp 中是 min_num
            sig.max_num = sig_json["max"];       // hpp 中是 max_num
            sig.unit = sig_json["unit"];

            signals_[sig.name] = sig;            // hpp 中是 signals_
            m.signal_names.push_back(sig.name);
        }
        messages_[m.id] = m;                     // hpp 中是 messages_
    }
    return true;
}

std::optional<double> SignalDatabase::unpack(const std::string& signal_name, const std::array<uint8_t, 8>& data) const {
    auto it = signals_.find(signal_name);
    if (it == signals_.end()) return std::nullopt;
    
    const SignalInfo& signal = it->second;
    uint64_t raw = 0;

    if (signal.byte_order == ByteOrder::Intel) {
        for (int i = 0; i < signal.length; ++i) {
            int bit_pos = signal.start + i;      // hpp 中是 start
            int byte_idx = bit_pos / 8;
            int bit_in_byte = bit_pos % 8;
            
            // 注意括号优先级
            if (((data[byte_idx] >> bit_in_byte) & 1) != 0) {
                raw |= (1ULL << i);
            }
        }
    } else {
        int current_byte = signal.start / 8;
        int current_bit = signal.start % 8;
        for (int i = 0; i < signal.length; ++i) {
            if (((data[current_byte] >> current_bit) & 1) != 0) {
                raw |= (1ULL << (signal.length - 1 - i));
            }
            if (current_bit == 0) {
                current_bit = 7;
                current_byte++;
            } else {
                current_bit--;
            }
        }
    }

    int64_t signed_raw = static_cast<int64_t>(raw);
    if (signal.is_signed) {
        // 检查最高位是否为 1
        if (((raw >> (signal.length - 1)) & 1) != 0) {
            signed_raw |= (~0ULL << signal.length); // 符号扩展，高位补 1
        }
    }

    return static_cast<double>(signed_raw) * signal.factor + signal.offset;
}

bool SignalDatabase::pack(const std::string& signal_name, double physical_value, std::array<uint8_t, 8>& data) const {
    auto it = signals_.find(signal_name);
    if (it == signals_.end()) return false;
    
    const SignalInfo& signal = it->second;

    // 范围校验
    if (physical_value < signal.min_num || physical_value > signal.max_num) {
        return false;
    }

    // 物理值反推 raw
    double raw_double = (physical_value - signal.offset) / signal.factor;
    int64_t signed_raw = std::llround(raw_double);
    uint64_t raw = static_cast<uint64_t>(signed_raw);

    if (signal.byte_order == ByteOrder::Intel) {
        for (int i = 0; i < signal.length; ++i) {
            // 动态计算跨字节的位置，不能写死
            int bit_pos = signal.start + i;
            int byte_idx = bit_pos / 8;
            int bit_in_byte = bit_pos % 8;
            
            data[byte_idx] &= ~(1 << bit_in_byte); // 先清零
            if (((raw >> i) & 1) != 0) {
                data[byte_idx] |= (1 << bit_in_byte); // 再写入
            }
        }
    } else {
        int current_byte = signal.start / 8;
        int current_bit = signal.start % 8;
        for (int i = 0; i < signal.length; ++i) {
            data[current_byte] &= ~(1 << current_bit); // 先清零
            if (((raw >> (signal.length - 1 - i)) & 1) != 0) {
                data[current_byte] |= (1 << current_bit); // 再写入
            }
            
            // 游标移动
            if (current_bit == 0) {
                current_bit = 7;
                current_byte++;
            } else {
                current_bit--;
            }
        }
    }
    return true;
}