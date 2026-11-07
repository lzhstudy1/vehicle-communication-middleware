#pragma once
#include<vector>
#include<cstdint>
#include<unordered_map>
#include<vcm/isotp_sender.hpp>
#include<vcm/IsoTpReceiver.hpp>
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

        void handleRequest(const std::vector<uint8_t>& req);

        void registerReadDID(uint16_t did, std::function<std::vector<uint8_t>()> readFnc);

    private:
        IsoTpSender& sender;
        IsoTpReceiver& receiver;
        Session session;
        std::unordered_map<uint16_t, std::function<std::vector<uint8_t>() > > readDID;
        
        void handle_0x10(const std::vector<uint8_t>& req);

        void handle_0x3E(const std::vector<uint8_t>& req);

        void handle_0x22(const std::vector<uint8_t>& req);

        void sendNRC(uint8_t sid, uint8_t nrc);
};