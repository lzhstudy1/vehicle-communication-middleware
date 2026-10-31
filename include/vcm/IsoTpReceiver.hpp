#pragma once
#include<stdint.h>
#include"vcm/can_socket.hpp"
#include<vector>

class IsoTpReceive{
    public:
        IsoTpReceive(CanSocket& socket,uint32_t rx_id, uint32_t tx_id);
        bool receive(std::vector<uint8_t>& out_payload, int timeout_ms = 1000);

        enum class State{
            Idle,
            WaitCF,
            Done,
            Error
        };
        State getState() const;

        private:
            CanSocket& can_socket_;
            uint32_t rx_id_;
            uint32_t tx_id_;
            
            //存放拼装的数据
            int total_length = 0;//总长度
            uint8_t expected_sn = 1;//期待的下一个连续帧的序号

            void sendFlowControl(uint8_t fs,uint8_t stmin);
            State state_;
};