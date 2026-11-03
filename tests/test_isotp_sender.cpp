#include"vcm/isotp_sender.hpp"
#include<gtest/gtest.h>
#include"vcm/can_socket.hpp"
#include<thread>
#include<chrono>

class IsotpSenderTest : public ::testing::Test {
    protected:
    CanSocket tx_socket;
    CanSocket rx_socket;

    void SetUp(){
        ASSERT_TRUE(tx_socket.open("vcan0"));
        ASSERT_TRUE(rx_socket.open("vcan0"));
    }
};

//------1.单帧发送测试------
TEST_F(IsotpSenderTest, SingleFrameSend){
    
    IsoTpSender sender(tx_socket,0x7E0,0x7E8);
    std::vector<uint8_t> payload = {0x10,0x20,0x31};

    ASSERT_TRUE(sender.send(payload));
    ASSERT_EQ(sender.getState(),IsoTpSender::State::Done);

    CanFrame rx_frame;
    ASSERT_TRUE(rx_socket.receive(rx_frame,100));
    ASSERT_EQ(rx_frame.id,0x7E0);
    ASSERT_EQ(rx_frame.data[0] ,0x03);
    ASSERT_EQ(rx_frame.data[1] , 0x10);
    ASSERT_EQ(rx_frame.data[2] , 0x20);
    ASSERT_EQ(rx_frame.data[3] , 0x31);
}

//------2.多帧发送测试------
TEST_F(IsotpSenderTest, MultiFrameSend){
  
    IsoTpSender sender(tx_socket,0x7E0,0x7E8);
    std::vector<uint8_t> payload(12,0xBB);

    std::thread receiver_thread([this](){
        CanFrame rx_frame;
        if(rx_socket.receive(rx_frame,500)){
            EXPECT_EQ(rx_frame.data[0],0x10);
            EXPECT_EQ(rx_frame.data[1],0x0C);

            CanFrame fc_frame;
            fc_frame.id = 0x7E8;
            fc_frame.dlc = 8;
            fc_frame.data[0] = 0x30;
            fc_frame.data[1] = 0x01;
            fc_frame.data[2] = 0x0A;
            rx_socket.send(fc_frame);
        }
        if(rx_socket.receive(rx_frame,500)){
            EXPECT_EQ(rx_frame.data[0],0x21);
        }
    });
    ASSERT_TRUE(sender.send(payload,1000));
    ASSERT_EQ(sender.getState(),IsoTpSender::State::Done);
    receiver_thread.join();
}

//------3.BS分块流控限制测试-----
TEST_F(IsotpSenderTest,BSDivertSend){

    IsoTpSender sender(tx_socket,0x7E0,0x7E8);
    std::vector<uint8_t> payload(24,0xAB);

    std::thread receive_thread([this](){
        CanFrame rx_frame{};
        if(rx_socket.receive(rx_frame,500)){
            EXPECT_EQ(rx_frame.data[0],0x10);
            EXPECT_EQ(rx_frame.data[1],24);

            CanFrame fc_frame1{};
            fc_frame1.id = 0x7E8;
            fc_frame1.dlc = 8;
            fc_frame1.data[0] = 0x30;
            fc_frame1.data[1] = 0x02;
            fc_frame1.data[2] = 0x00;
            rx_socket.send(fc_frame1);

            if(rx_socket.receive(rx_frame,500)){
                EXPECT_EQ(rx_frame.data[0],0x21);
            }
            if(rx_socket.receive(rx_frame,500)){
                EXPECT_EQ(rx_frame.data[0],0x22);
            }
            CanFrame unexpection_frame {};
            bool got_more = rx_socket.receive(unexpection_frame,100);
            EXPECT_FALSE(got_more);
            CanFrame fc_frame2{};
            fc_frame2.id = 0x7E8;
            fc_frame2.dlc = 8;
            fc_frame2.data[0] = 0x30;
            fc_frame2.data[1] = 0x02;
            fc_frame2.data[2] = 0x00;
            rx_socket.send(fc_frame2);
            if(rx_socket.receive(rx_frame,500)){
                EXPECT_EQ(rx_frame.data[0],0x23);
            }
        }
    });
    ASSERT_TRUE(sender.send(payload,1000));
    ASSERT_EQ(sender.getState(),IsoTpSender::State::Done);
    receive_thread.join();
}

//------4.超时报错---
TEST_F(IsotpSenderTest, TimeoutError){
    IsoTpSender sender(tx_socket,0x7E0,0x7E8);
    std::vector<uint8_t>payload(12,0xBB);

    std::thread receive_thread([this]{
        CanFrame rx_frame;
        rx_socket.receive(rx_frame,500);
    });
    ASSERT_FALSE(sender.send(payload,200));
    ASSERT_EQ(sender.getState(),IsoTpSender::State::Error);
    
    receive_thread.join();
}

//-----5.溢出报错----
TEST_F(IsotpSenderTest, OverflowError){
    IsoTpSender sender(tx_socket,0x7E0,0x7E8);
    std::vector<uint8_t>payload(12,0xBB);

    std::thread receive_thread([this]{
        CanFrame rx_frame;
        if(rx_socket.receive(rx_frame,500)){
            CanFrame fc_frame;
            fc_frame.id = 0x7E8;
            fc_frame.dlc = 8;
            fc_frame.data[0] = 0x32;
            rx_socket.send(fc_frame);
        }
    });
    
    ASSERT_FALSE(sender.send(payload,500));
    ASSERT_EQ(sender.getState(),IsoTpSender::State::Error);
    receive_thread.join();
}