#pragma once

#include <cstdint>
#include <linux/can.h>

// 对 struct can_frame 的 C++ 封装
// id 存储真实 ID，不含标志位
struct CanFrame {
    uint32_t id = 0;
    bool is_extended = false;
    bool is_rtr = false;
    uint8_t dlc = 0;
    uint8_t data[8] = {0};

    // 从内核 can_frame 转换
    static CanFrame fromRaw(const struct can_frame& raw);

    // 转换为内核 can_frame
    struct can_frame toRaw() const;
};