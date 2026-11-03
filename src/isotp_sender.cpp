#include"vcm/isotp_sender.hpp"
#include"vcm/can_frame.hpp"
#include<thread> //sleep_for()
#include<iostream>

IsoTpSender::IsoTpSender(CanSocket &can_socket,uint32_t tx_id,uint32_t rx_id)
     : tx_id(tx_id), rx_id(rx_id), can_socket_(can_socket) {
    resetState();
}

bool IsoTpSender::send(const std::vector<uint8_t> &payload,int timeout_ms){
      if(payload.empty()) return false;
      resetState();
      total_length = payload.size();
      
      if(total_length <= 7){//直接发送首帧
        state_ = State::Done;
        return sendSingleFrame(payload);
      }else{//首帧发送完成后等待回执与发送流控帧
        state_ = State::WaitFC;
        if(!sendFirstFrame(payload)){//发送首帧
            state_  = State::Error;
            return false;
        }
        if(!waitFlowControl(timeout_ms)){//等待回执
            state_ = State::Error;
            return false;
        }
        state_ = State::SendCF;
        if(!sendConsecutiveFrames(payload,timeout_ms)){//发送流控帧
            state_ = State::Error;
            return false;
        }
        state_ = State::Done;
        return true;
      }
}

void IsoTpSender::resetState(){
    state_ = State::Idle;
    sequence_number = 1;
    block_size = 0;
    st_min = 0; 
    block_count = 0; 
    total_length = 0; 
    sent_length = 0; 
}

bool IsoTpSender::sendSingleFrame(const std::vector<uint8_t> &payload){
    CanFrame frame_;
    frame_.id = tx_id;
    frame_.dlc = 8;
    frame_.is_extended = false;
    frame_.is_rtr = false;

    frame_.data[0] = static_cast<uint8_t>(payload.size() & 0x0F);

    for(uint64_t i = 0; i < payload.size(); i++){
        frame_.data[i + 1] = payload[i];
    }

    for(uint64_t i = payload.size() + 1; i < 8; i++){
        frame_.data[i] = 0x00;
    }

    bool success = can_socket_.send(frame_);

    if(success){
        state_ = State::Done;
        return true;
    }else{
        state_ = State::Error;
        return false;
    }
}

bool IsoTpSender::sendFirstFrame(const std::vector<uint8_t>& payload){
    if(total_length > 4095){
        state_ = State::Error;
        return false;
    }
    CanFrame frame_;
    frame_.id = tx_id;
    frame_.dlc = 8;
    frame_.is_extended = false;
    frame_.is_rtr = false;
    //byte0高4位为1,低四位与byte1表示数据个数12个二进制位
    frame_.data[0] = 0x10 | ((total_length >> 8) & 0x0F);  //byte0低四位
    frame_.data[1] = total_length & 0xFF;

    for(int i = 2; i < 8; i++){
        frame_.data[i] = payload[i - 2];
    }
    sent_length = 6;

    bool success = can_socket_.send(frame_);
    if(success){
        state_ = State::WaitFC;
        return true;
    }else{
        state_ = State::Error;
        return false;
    }
}

bool IsoTpSender::waitFlowControl(int timeout_ms){
    CanFrame frame_;
   auto start = std::chrono::steady_clock::now();
   while(1){
    auto cost = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - start).count();

      if(cost > timeout_ms) return false;

      if(!can_socket_.receive(frame_,100)){
        continue;
      }else{
        if(frame_.id != rx_id) continue;
        if((frame_.data[0] & 0xF0) != 0x30) continue;
        uint8_t fs = frame_.data[0] & 0x0F;
        if(fs == 0){
            block_size = frame_.data[1];
            st_min = frame_.data[2];
            block_count = 0;
            return true;
        }else if(fs == 1){
            continue;
        }else{
            std::cerr<<"溢出"<<std::endl;
            state_ = State::Error;
            return false;
        }
      }
   }
}

bool IsoTpSender::sendConsecutiveFrames(const std::vector<uint8_t>& payload,
    int timeout_ms){
    CanFrame frame_;
    frame_.id = tx_id;
    frame_.dlc = 8;
    frame_.is_extended = false;
    frame_.is_rtr = false;

     while(sent_length < total_length){
        frame_.data[0] = 0x20 | sequence_number;//找CF帧与发送序号
        int send_size = std::min(7,total_length - sent_length);
        for(int i = 0; i < send_size; i++){
            frame_.data[i + 1] = payload[sent_length + i];
        }
        if(send_size < 7){
            for(int i = send_size + 1; i < 8; i++){
                frame_.data[i] = 0x00;
            }
        }
        bool success = can_socket_.send(frame_);
        if(!success) {
            state_ = State::Error;
            return false;
        }
        
        sent_length += send_size;
        sequence_number++;
        block_count++;
        if(sequence_number > 15){
            sequence_number = 0;
        }
        //STmin等待时间
        if(st_min > 0){
            std::this_thread::sleep_for(std::chrono::milliseconds(st_min));
        }
        if(sent_length < total_length){
           if(block_size > 0 && block_count >= block_size){
              if(!waitFlowControl(timeout_ms)) return false;
            }
        }
     }
     return true;
}