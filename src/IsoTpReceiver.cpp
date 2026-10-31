#include"vcm/IsoTpReceiver.hpp"
#include"vcm/can_frame.hpp"

IsoTpReceive::IsoTpReceive(CanSocket& socket,uint32_t rx_id, uint32_t tx_id):
    can_socket_(socket),rx_id_(rx_id),tx_id_(tx_id){
        state_ = State::Idle;
}

void IsoTpReceive::sendFlowControl(uint8_t fs,uint8_t stmin){
   CanFrame rx_frame;
}