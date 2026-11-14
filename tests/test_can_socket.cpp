#include <gtest/gtest.h>
#include "vcm/can_socket.hpp"
#include <fstream>
#include <cstdio>   // for std::remove
#include <thread>
#include <chrono>

class CanSocketTest : public ::testing::Test {
protected:
    void SetUp() override {  
    }
};


TEST_F(CanSocketTest, OperationsWithoutOpen) {
    CanSocket sock; 
    CanFrame frame;
    EXPECT_FALSE(sock.send(frame));
    EXPECT_FALSE(sock.receive(frame, 100));
    EXPECT_FALSE(sock.setFilter(0x100, 0x700));
    EXPECT_FALSE(sock.setFilters({{0x100, 0x700}}));
    EXPECT_FALSE(sock.clearFilter());
    EXPECT_FALSE(sock.loadFiltersFromFile("dummy.txt"));
}

TEST_F(CanSocketTest, OpenNonexistentInterface) {
    CanSocket sock;
    
    EXPECT_FALSE(sock.open("non_exist_can_iface"));
}

TEST_F(CanSocketTest, SendReceiveAndTimeout) {
    CanSocket sender;
    CanSocket receiver;
    
    ASSERT_TRUE(sender.open("vcan0"));
    ASSERT_TRUE(receiver.open("vcan0"));

    CanFrame rx_frame;
    EXPECT_FALSE(receiver.receive(rx_frame, 50));

    CanFrame tx_frame;
    tx_frame.id = 0x123;
    tx_frame.dlc = 2;
    tx_frame.data[0] = 0xAA;
    tx_frame.data[1] = 0xBB;
    
    EXPECT_TRUE(sender.send(tx_frame));
    
    EXPECT_TRUE(receiver.receive(rx_frame, 100));
    EXPECT_EQ(rx_frame.id, 0x123);
    EXPECT_EQ(rx_frame.dlc, 2);
    EXPECT_EQ(rx_frame.data[0], 0xAA);
}

TEST_F(CanSocketTest, FilterFunctionality) {
    CanSocket sender;
    CanSocket receiver;
    
    ASSERT_TRUE(sender.open("vcan0"));
    ASSERT_TRUE(receiver.open("vcan0"));

    CanFrame tx1{}; tx1.id = 0x200; tx1.dlc = 1;
    CanFrame tx2{}; tx2.id = 0x300; tx2.dlc = 1;
    CanFrame tx3{}; tx3.id = 0x400; tx3.dlc = 1;
    CanFrame rx{};

    ASSERT_TRUE(receiver.setFilter(0x200, 0x7FF)); 

    ASSERT_TRUE(sender.send(tx1)); 
    EXPECT_TRUE(receiver.receive(rx, 100)); 
    
    ASSERT_TRUE(sender.send(tx2)); 
    EXPECT_FALSE(receiver.receive(rx, 50));

    std::vector<struct can_filter> filters = { {0x300, 0x7FF}, {0x400, 0x7FF} };
    ASSERT_TRUE(receiver.setFilters(filters));

    ASSERT_TRUE(sender.send(tx3)); 
    EXPECT_TRUE(receiver.receive(rx, 100)); 

    ASSERT_TRUE(receiver.setFilters({}));
    ASSERT_TRUE(receiver.clearFilter());
    
    ASSERT_TRUE(sender.send(tx1)); 
    EXPECT_TRUE(receiver.receive(rx, 100)); 
}

TEST_F(CanSocketTest, LoadFiltersFromFile) {
    CanSocket sock;
    ASSERT_TRUE(sock.open("vcan0"));

    EXPECT_FALSE(sock.loadFiltersFromFile("not_exist_file.txt"));
    std::string filepath = "test_dirty_filters.txt";
    std::ofstream ofs(filepath);
    ofs << "# 这是一个注释行\n"
        << "\n"                             
        << "0x100\n"                        
        << "ABC INVALID_HEX\n"              
        << "0x500 0x7FF\n";                 
    ofs.close();

    EXPECT_TRUE(sock.loadFiltersFromFile(filepath));

    std::remove(filepath.c_str());
}