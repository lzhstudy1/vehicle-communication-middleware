#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <optional>
#include <cstdint>
#include <array>

enum class ByteOrder {
    Intel,
    Motorola
};

struct SignalInfo {
    std::string name;          // 建议用 name，和 .cpp 里的 sig.name 对应
    int start = 0;             // 起始位
    int length = 0;            // 长度
    ByteOrder byte_order;      // 字节序
    bool is_signed = false;    // 是否有符号
    double factor = 1.0;       // 比例
    double offset = 0.0;       // 偏移
    double min_num = 0.0;      // 最小值
    double max_num = 0.0;      // 最大值
    std::string unit;          // 单位
};

struct MessageInfo {
    int id = 0;
    std::string name;
    int length = 8;
    std::vector<std::string> signal_names;
};

class SignalDatabase {
public:
    // 参数加上 const 引用，避免字符串拷贝
    bool load(const std::string& json_path);
    
    std::optional<double> unpack(const std::string& signal_name, const std::array<uint8_t, 8>& data) const;

    bool pack(const std::string& signal_name, double physical_value, std::array<uint8_t, 8>& data) const;

    std::optional<SignalInfo> getSignalInfo(const std::string& signal_name) const {
        auto it = signals_.find(signal_name);
        if (it == signals_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::vector<std::string> getMessageNames(int message_id) const {
        auto it = messages_.find(message_id);
        if (it == messages_.end()) {
            return {};
        }
        return it->second.signal_names;
    }

private:
    std::unordered_map<int, MessageInfo> messages_;
    std::unordered_map<std::string, SignalInfo> signals_; // 修复了逗号和命名
};