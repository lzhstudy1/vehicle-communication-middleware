#pragma once
#include<atomic>
#include<vector>
#include<cstdint>
#include<unordered_map>
#include<vcm/isotp_sender.hpp>
#include<vcm/IsoTpReceiver.hpp>
#include<chrono>
#include<functional>

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

        void stop(){is_running = false;}
        void handleRequest(const std::vector<uint8_t>& req);
        void registerReadDID(uint16_t did, std::function<std::vector<uint8_t>()> readFnc);
        void registerKeyAlgorithm(SecurityLevel level, std::function<uint32_t(uint32_t)> algo);
        int getLockMessage();

    private:
        IsoTpSender& sender;
        IsoTpReceiver& receiver;
        std::atomic<bool> is_running = {true};
        Session session = Session::DEFAULT;
        SecurityLevel current_level = SecurityLevel::LEVEL_1;
        bool unlock_level1 = false;
        bool unlock_level3 = false;
        uint32_t current_seed = 0; //发给诊断仪的随即种子
        int failed_attempts = 0;//解锁失败次数
        std::chrono::steady_clock::time_point lockout_time;//锁定状态开始时间点
        std::chrono::steady_clock::time_point last_active_time; //最后一次活跃时间点

        std::unordered_map<uint16_t, std::function<std::vector<uint8_t>() > > readDID;
        
        std::unordered_map<SecurityLevel, std::function<uint32_t(uint32_t)>> safeFnc;

        bool isLockout();
        uint32_t getRandNum();
        void checkS3Timeout();

        void handle_0x27(const std::vector<uint8_t>& req);
        void handle_0x10(const std::vector<uint8_t>& req);
        void handle_0x3E(const std::vector<uint8_t>& req);
        void handle_0x22(const std::vector<uint8_t>& req);
        void sendNRC(uint8_t sid, uint8_t nrc);
};