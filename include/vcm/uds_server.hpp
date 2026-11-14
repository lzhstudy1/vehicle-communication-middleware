#pragma once
#include<atomic>
#include<vector>
#include<cstdint>
#include<unordered_map>
#include"vcm/isotp_sender.hpp"
#include"vcm/IsoTpReceiver.hpp"
#include"vcm/DTC_Manager.hpp"
#include<chrono>
#include<functional>
#include<optional>

class UdsServer {
    public:
        UdsServer(IsoTpSender& sender, IsoTpReceiver& receiver);
        void processLoop();

        enum class Session{
            DEFAULT,//只可读
            PROGRAMMING,//可以刷写ECU
            EXTENDED//诊断测试配置
        };
        Session getSession() const { return session;}

        enum class SecurityLevel{
            LEVEL_1 = 1,
            LEVEL_3 = 3
        };
        SecurityLevel getSecurityLevel() const { return current_level;}

        DtcManager& getDtc(){return dtc_manager;};

        void stop(){is_running = false;}
        void handleRequest(const std::vector<uint8_t>& req);
        void registerReadDID(uint16_t did, std::function<std::vector<uint8_t>()> readFnc);//读函数
        void registerKeyAlgorithm(SecurityLevel level, std::function<uint32_t(uint32_t)> algo);
        int getLockMessage();
        void registerRouting(uint16_t rid, std::function<std::optional<std::vector<uint8_t>>
            (uint8_t sub_fnc, const std::vector<uint8_t>& req_data)> routing_func);
        void registerDIDWrite(uint16_t did, std::function<bool(const std::vector<uint8_t>&)> write_func);//写函数

    private:
        IsoTpSender& sender;
        IsoTpReceiver& receiver;
        std::atomic<bool> is_running = {true};
        std::atomic<bool> routing_running = {false};
        DtcManager dtc_manager;
        Session session = Session::DEFAULT;
        SecurityLevel current_level = SecurityLevel::LEVEL_1;
        bool unlock_level1 = false;
        bool unlock_level3 = false;
        uint32_t current_seed = 0; //发给诊断仪的随即种子
        int failed_attempts = 0;//解锁失败次数
        std::chrono::steady_clock::time_point lockout_time;//锁定状态开始时间点
        std::chrono::steady_clock::time_point last_active_time; //最后一次活跃时间点

        std::unordered_map<uint16_t, std::function<std::vector<uint8_t>() >> readFnc;//DID读取函数映射表
        std::unordered_map<SecurityLevel, std::function<uint32_t(uint32_t)>> safeFnc;
        std::unordered_map<uint16_t, std::function<std::optional<std::vector<uint8_t>>
        (uint8_t sub_fnc, const std::vector<uint8_t>& req_data)>> routingFnc;
        std::unordered_map<uint16_t, std::function<bool(const std::vector<uint8_t>&)>> writeFnc;//DID写函数映射表

        bool isLockout();
        uint32_t getRandNum();
        void checkS3Timeout();

        void handle_0x27(const std::vector<uint8_t>& req);//发送安全请求
        void handle_0x10(const std::vector<uint8_t>& req);//会话控制
        void handle_0x3E(const std::vector<uint8_t>& req);//保持连接
        void handle_0x22(const std::vector<uint8_t>& req);//读取操作
        void handle_0x2E(const std::vector<uint8_t>& req);//写操作
        void handle_0x31(const std::vector<uint8_t>& req);//例程控制
        void handle_0x14(const std::vector<uint8_t>& req);//清除DTC
        void handle_0x19(const std::vector<uint8_t>& req);//读取DTC信息
        void sendNRC(uint8_t sid, uint8_t nrc);
};