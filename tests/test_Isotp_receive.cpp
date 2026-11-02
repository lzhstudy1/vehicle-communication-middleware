#include<gtest/gtest.h>
#include<vcm/IsoTpReceiver.hpp>
#include<thread>
#include<chrono>
#include<vcm/can_frame.hpp>
#include<vcm/isotp_sender.hpp>

class TestIsoTpReceive : public :: testing ::Test{
    protected:
    CanSocket tx_socket;
    CanSocket rx_socket;

    void SetUp(){
        ASSERT_TRUE(tx_socket.open("vcan0"));
        ASSERT_TRUE(rx_socket.open("vcan0"));
    }
};

//------1.单帧接收----
TEST_F(TestIsoTpReceive, SingleFrameReceive){
    IsoTpReceiver receiver(rx_socket,0x7F0,0x7F8);//rx_id我要接听什么标签的帧
    //tx_id 我发送的数据需要什么id
    std::vector<uint8_t> payload(10,0);
    std::thread sender([this]{
        CanFrame frame{0};
        frame.id = 0x7F0;
        frame.dlc = 8;
        frame.data[0] = 0x03;
        frame.data[1] = 0x01;
        frame.data[2] = 0x02;
        frame.data[3] = 0x03;
        tx_socket.send(frame);
    });
     
    ASSERT_TRUE(receiver.receive(payload,1000));
    ASSERT_EQ(payload.size(),3);
    
    if(receiver.getState() == IsoTpReceiver::State::Done){
        ASSERT_EQ(payload[0],1);
        ASSERT_EQ(payload[1],2);
        ASSERT_EQ(payload[2],3);
    }
    sender.join();
}

//------2.多帧接收-----
TEST_F(TestIsoTpReceive,ReceiveMutiFrame){
    IsoTpReceiver receiver(rx_socket,0x7E0,0x7E8);
    std::vector<uint8_t> payload(12,0);
    std::thread sender([this] { 
        CanFrame frame{};
        frame.id = 0x7E0;
        frame.dlc = 8;
        frame.data[0] = 0x10;
        frame.data[1] = 0x0C;
        for(int i = 2;i < 8; i++){
           frame.data[i] = ((uint8_t)(i - 1));
        }
        tx_socket.send(frame);

        CanFrame fc{};
        if(tx_socket.receive(fc,500)){
            EXPECT_EQ(fc.data[0],0x30);
        }
        frame.data[0] = 0x21; 
        for(int i=1; i<7; i++) frame.data[i] = 0x22; 
        tx_socket.send(frame);
    });
     
     ASSERT_TRUE(receiver.receive(payload,1000));
     
     ASSERT_EQ(payload.size(),12);
     ASSERT_EQ(receiver.getState(),IsoTpReceiver::State::Done);
     sender.join();
}

//------3.SN错误检测----
TEST_F(TestIsoTpReceive,SNWrongTest){
    IsoTpReceiver receiver(rx_socket,7E0,7E8);
    std::vector<uint8_t> payload(20,0);

    std::thread sender([this]{
        CanFrame frame{};
        frame.data[0] = 0x10;
        frame.data[1] = 0x14;
        for(int i = 2; i < 8;i++){
            frame.data[i] = ((uint8_t)(i - 1));
        }
        tx_socket.send(frame);

        CanFrame fc{};
        if(tx_socket.receive(fc,500)){
            EXPECT_EQ(fc.data[0],0x30);
        }
        frame.data[0] = 0x22;
        for(int i = 1; i <= 7;i++) frame.data[i] = 0x22;
        tx_socket.send(frame);
    });
    bool success = receiver.receive(payload,1000);
    
    EXPECT_FALSE(success);
    ASSERT_EQ(receiver.getState(),IsoTpReceiver::State::Error);
    sender.join();
}

//------4.溢出保护----
TEST_F(TestIsoTpReceive,OverFlowProduce){
    IsoTpReceiver receiver(rx_socket,7E0,7E8);
    std::thread sender([this]{
        CanFrame frame{};
        frame.data[0] = 0x12;
        frame.data[1] = 0x58;
        tx_socket.send(frame);
        
        CanFrame fc{};
        if(tx_socket.receive(fc,500)){
            EXPECT_EQ(fc.data[0],0x32);
        }
    });
    std::vector<uint8_t> out_payload;
    bool success = receiver.receive(out_payload,1000);
    
    EXPECT_FALSE(success);
    ASSERT_EQ(receiver.getState() , IsoTpReceiver::State::Error);
    sender.join();
}

//----5.接收超时-----
TEST_F(TestIsoTpReceive,ReceiverTimeout){
    IsoTpReceiver receiver(rx_socket,7E0,7E8);
    std::thread sender([this] { 
        CanFrame frame{};
        frame.id = 0x7E0;
        frame.dlc = 8;
        frame.data[0] = 0x10;
        frame.data[1] = 0x0C;
        for(int i = 2;i < 8; i++){
           frame.data[i] = ((uint8_t)(i - 1));
        }
        tx_socket.send(frame);

        CanFrame fc{};
        tx_socket.receive(fc,200);
    });
     std::vector<uint8_t> out_payload;
     bool success = receiver.receive(out_payload,200);
     EXPECT_FALSE(success);
     ASSERT_EQ(receiver.getState() , IsoTpReceiver::State::Error);
     sender.join();
}

//-----6端到端测试
TEST_F(TestIsoTpReceive,EndToEnd){
    IsoTpReceiver receiver(rx_socket,0x7E0,0x7E8);
    IsoTpSender sender(0x7E0,0x7E8,tx_socket);

    std::vector<uint8_t> payload(20,0xAA);
    std::thread sender_thread([&]{
        EXPECT_TRUE(sender.send(payload,1000));
    });
    std::vector<uint8_t> out_payload;
    ASSERT_TRUE(receiver.receive(out_payload,2000));
    EXPECT_EQ(out_payload.size(),20);
    EXPECT_EQ(payload,out_payload);
    sender_thread.join();
}