#pragma once

#include "vcm/can_frame.hpp"
#include <linux/can.h>          // struct can_filter
#include <string>
#include <vector>

// RAII 封装 SocketCAN
class CanSocket {
public:
    CanSocket() = default;
    ~CanSocket();

    // 禁止拷贝
    CanSocket(const CanSocket&) = delete;
    CanSocket& operator=(const CanSocket&) = delete;

    // 打开接口，例如 "vcan0"
    bool open(const std::string& ifname);

    // 关闭接口
    void close();

    // 是否已打开
    bool isOpen() const { return sock_ >= 0; }

    // 发送一帧
    bool send(const CanFrame& frame);

    // 接收一帧，带超时（毫秒）
    // 返回 true：收到帧
    // 返回 false：超时或错误
    bool receive(CanFrame& frame, int timeout_ms);

    // 设置单条过滤器
    bool setFilter(uint32_t filter_id, uint32_t mask);

    // 设置多条过滤器：任意一条匹配即接收
    bool setFilters(const std::vector<struct can_filter>& filters);

    // 清空过滤器：不接收任何帧
    bool clearFilter();

    // 从文件批量加载过滤器并设置
    // 文件格式：每行 `<can_id> <can_mask>`，支持 # 注释和空行
    // 数字支持 0x 十六进制和十进制
    bool loadFiltersFromFile(const std::string& path);

private:
    int sock_ = -1;
};