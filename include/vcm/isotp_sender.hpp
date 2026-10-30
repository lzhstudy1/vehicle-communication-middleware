#pragma once 
#include <chrono>
#include"vcm/can_socket.hpp"
#include<stdint.h>
#include <vector>


class IsoTpSender{
   public:
   IsoTpSender(uint32_t tx_id,uint32_t rx_id,CanSocket &can_socket);
   IsoTpSender(const IsoTpSender &) = delete;
   IsoTpSender& operator=(const IsoTpSender &) = delete;
   bool send(const std::vector<uint8_t> &payload,int timeout_ms = 1000);

   enum class State{ 
   Idle,
   WaitFC,
   SendCF,
   Done,
   Error
};
  State getState() const{ return state_;}

   private:
   uint32_t tx_id;
   uint32_t rx_id;
   CanSocket &can_socket_;

   State state_ = State::Idle;
   
   uint8_t sequence_number = 1;//CF帧的序号
   uint8_t block_size = 0; //流控帧的分块大小
   uint8_t st_min = 0; //流控帧的最小间隔
   int block_count = 0; //当前块内发送帧数
   int total_length = 0; //总长度
   int sent_length = 0; //已发送长度

   void resetState();
   bool sendSingleFrame(const std::vector<uint8_t> &payload);
   bool sendFirstFrame(const std::vector<uint8_t> &payload);
   bool sendConsecutiveFrames(const std::vector<uint8_t> &payload,int timeout_ms);
   bool waitFlowControl(int timeout_ms);
};