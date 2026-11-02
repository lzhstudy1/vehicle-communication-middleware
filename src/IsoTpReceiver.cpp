#include"vcm/IsoTpReceiver.hpp"
#include"vcm/can_frame.hpp"
#include<chrono>
#include<iostream>
#include<cstring>

IsoTpReceiver::IsoTpReceiver(CanSocket& socket,uint32_t rx_id, uint32_t tx_id):
    can_socket_(socket),rx_id_(rx_id),tx_id_(tx_id){
        state_ = State::Idle;
}

void IsoTpReceiver::sendFlowControl(uint8_t fs,uint8_t bs,uint8_t stmin){
   CanFrame rx_frame;
   rx_frame.id = tx_id_;
   rx_frame.dlc = 8;
   rx_frame.data[0] = 0x30 | fs;
   rx_frame.data[1] = bs;
   rx_frame.data[2] = stmin;
   for(int i = 3; i < 8; i++){
    rx_frame.data[i] = 0x00;
   }
   can_socket_.send(rx_frame);
}

bool IsoTpReceiver::receive(std::vector<uint8_t>& out_payload,int timeout_ms){
    out_payload.clear();
    total_length = 0;
    current_length = 0;
    state_ = State::Idle;
    auto start = std::chrono::steady_clock::now();
    CanFrame frame;

    while(1){
        auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

            if(cost > timeout_ms){
                state_ = State::Error;
                return false;
            }

            if(!can_socket_.receive(frame,100)) continue;
            if(frame.id != rx_id_) continue;
            //接收单帧
            uint8_t mes_type = frame.data[0] & 0xF0;
            if(mes_type == 0x00 && state_ == State::Idle){
                int len = frame.data[0] & 0x0F;
                if(len > 7) {
                    state_ = State::Error;
                    return false;
                }
                out_payload.clear();
                for(int i = 0; i < len; i++){
                    out_payload.push_back(frame.data[i+1]);
                }
                state_ = State::Done;
                return true;
            }else if(mes_type ==0x10 && state_ == State::Idle){
                total_length = ((frame.data[0] & 0x0F) << 8) | frame.data[1];
                if(total_length > MemoryPoolSize){
                    state_ = State::Error;
                    sendFlowControl(2,0,0);
                    return false;
                }
               std::memcpy(buffer,&frame.data[2],6);
               current_length = 6;
               sendFlowControl(0,0,0);
               state_ = State::WaitCF;
               expected_sn = 1;
            }else if(mes_type == 0x20 && state_ == State::WaitCF){
                uint8_t actual_sn = frame.data[0] & 0x0F;
                if(actual_sn != expected_sn){
                    state_ = State::Error;
                    return false;
                }
                int remain = total_length - current_length;
                int copy_size = std::min(7,remain);
                std::memcpy(buffer + current_length,&frame.data[1],copy_size);
                current_length += copy_size;
                expected_sn++;
                if(expected_sn > 15) expected_sn = 0;
                if(current_length >= total_length){
                   out_payload.assign(buffer, buffer + total_length);
                    state_ = State::Done;
                    return true;
                }
            }  
    }
}