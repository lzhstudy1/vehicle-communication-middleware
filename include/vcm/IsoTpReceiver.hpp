#pragma once
#include<stdint.h>
#include"vcm/can_socket.hpp"
#include<vector>

class IsoTpReceiver{//接收方
    public:
        IsoTpReceiver(CanSocket& socket,uint32_t rx_id, uint32_t tx_id);
        bool receive(std::vector<uint8_t>& out_payload, int timeout_ms = 1000);

        enum class State{
            Idle,
            WaitCF,
            Done,
            Error
        };
        State getState() const { return state_;}

    private:
            static constexpr int MemoryPoolSize = 512;
            CanSocket& can_socket_;
            uint32_t rx_id_;
            uint32_t tx_id_;
            //rx_id我要接听什么标签的帧
            //tx_id 我发送的数据需要什么id
            
            uint8_t buffer[MemoryPoolSize];//存放拼装的数据
            int total_length = 0;//总长度
            int current_length = 0;//已经发送的长度
            uint8_t expected_sn = 1;//期待的下一个连续帧的序号

            void sendFlowControl(uint8_t fs,uint8_t bs,uint8_t stmin);
            State state_;
};