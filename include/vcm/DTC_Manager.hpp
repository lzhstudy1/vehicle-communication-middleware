#pragma once

#include<vector>
#include<cstdint>
#include<mutex>
#include<algorithm>

enum class DtcStatus : uint8_t{
    TEST_FAILED = 0x01,   //当前测试失败
    PENDING = 0x04,//未决故障
    CONFIRMED = 0x08,//已确认故障
    WARNING_LIGHT_REQUESTED = 0x80//请求点亮警告灯
};

struct DtcRecord{
    uint32_t code; //DTC故障码(id只有3个字节但是4字节对齐处理更快)低24位有效
    uint8_t status;//状态字节
    uint8_t aging_counter;//老化计数器
};

class DtcManager {
    public:
    DtcManager() = default;

    void processAgingCycle();//老化计数器每圈加1，老化计数器溢出则清除故障状态

    //上报故障存在则叠加状态否则创建
    void reportDtc(uint32_t code, uint8_t mask);

    //当ECU复位时清除状态位
    void clearDtcStatus(uint32_t code,uint8_t mask);

    //检测是否存在某个故障
    bool hasDtc(uint32_t code)const;

    //根据mask获取dtc例表
    std::vector<DtcRecord> getDtcList(uint8_t mask) const;

    //清除所有故障
    void clearAllDtc();

    //获取DTC总数
    size_t getDtcCount(uint8_t mask)const;
    private:
    std::vector<DtcRecord> dtc_list_;
    mutable std::mutex mutex_;
};