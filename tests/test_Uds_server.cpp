#include <gtest/gtest.h>
#include "vcm/uds_server.hpp"
#include "vcm/isotp_sender.hpp"
#include "vcm/IsoTpReceiver.hpp"
#include "vcm/can_socket.hpp"
#include <vector>
#include <thread>
#include <chrono>

class UdsServerTest : public ::testing::Test {
protected:
    CanSocket ecu_socket;
    CanSocket tester_socket;

    void SetUp() override {
        ASSERT_TRUE(ecu_socket.open("vcan0"));
        ASSERT_TRUE(tester_socket.open("vcan0"));
    }
};


std::vector<uint8_t> sendRequestAndGetResponse(
    CanSocket& tester_can, 
    const std::vector<uint8_t>& request, 
    uint32_t req_id = 0x7E0, 
    uint32_t resp_id = 0x7E8) {
        IsoTpSender tester_tx(tester_can, req_id, resp_id);
        tester_tx.send(request, 1000);
        IsoTpReceiver tester_rx(tester_can, resp_id, req_id);
        std::vector<uint8_t> response;
        tester_rx.receive(response, 1000);

        return response;
}

uint32_t VectorToInt32(std::vector<uint8_t>& req){
    uint32_t endKey = (req[2] << 24) | (req[3] << 16)
    | (req[4] << 8) | req[5];
    return endKey;
}

void Int32_ToVector(uint32_t num,std::vector<uint8_t>& req){
    req.push_back(static_cast<uint8_t> (num >> 24));
    req.push_back(static_cast<uint8_t> (num >> 16));
    req.push_back(static_cast<uint8_t> (num >> 8));
    req.push_back(static_cast<uint8_t> (num));   
}
// -----1.0x10 会话控制（正常） ---
TEST_F(UdsServerTest, Handle_0x10_ExtendedSession) {
    
    IsoTpSender ecu_tx(ecu_socket, 0x7E8, 0x7E0);
    IsoTpReceiver ecu_rx(ecu_socket, 0x7E0, 0x7E8);
    UdsServer server(ecu_tx, ecu_rx);

    std::thread ecu_thread([&]() {
        std::vector<uint8_t> req;
        if (ecu_rx.receive(req, 1000)) {//接收SRAGR里面发送的数据
            server.handleRequest(req); //发送回复帧给SRAGR
        }
    });
    std::vector<uint8_t> request = {0x10, 0x03};
    std::vector<uint8_t> resp = sendRequestAndGetResponse(tester_socket, request);
    
    if (ecu_thread.joinable()) ecu_thread.join();
    ASSERT_GE(resp.size(), 2);
    EXPECT_EQ(resp[0], 0x50);
    EXPECT_EQ(resp[1], 0x03);
    EXPECT_EQ(server.getSession(), UdsServer::Session::EXTENDED);
}

// ----2.0x10 会话控制（长度错误） ---
TEST_F(UdsServerTest, Handle_0x10_LengthError) {
    IsoTpSender ecu_tx(ecu_socket, 0x7E8, 0x7E0);
    IsoTpReceiver ecu_rx(ecu_socket, 0x7E0, 0x7E8);
    UdsServer server(ecu_tx, ecu_rx);

    std::thread ecu_thread([&]() {
        std::vector<uint8_t> req;
        if (ecu_rx.receive(req, 1000)) server.handleRequest(req);
    });
    std::vector<uint8_t> request = {0x10};
    std::vector<uint8_t> resp = sendRequestAndGetResponse(tester_socket, request);

    if (ecu_thread.joinable()) ecu_thread.join();
    ASSERT_EQ(resp.size(), 3);
    EXPECT_EQ(resp[0], 0x7F);
    EXPECT_EQ(resp[1], 0x10);
    EXPECT_EQ(resp[2], 0x13);
}

// ----------3.0x3E 保持连接 ----
TEST_F(UdsServerTest, Handle_0x3E_TesterPresent) {
    IsoTpSender ecu_tx(ecu_socket, 0x7E8, 0x7E0);
    IsoTpReceiver ecu_rx(ecu_socket, 0x7E0, 0x7E8);
    UdsServer server(ecu_tx, ecu_rx);

    std::thread ecu_thread([&]() {
        std::vector<uint8_t> req;
        if (ecu_rx.receive(req, 1000)) server.handleRequest(req);
    });

    std::vector<uint8_t> request = {0x3E, 0x00};
    std::vector<uint8_t> resp = sendRequestAndGetResponse(tester_socket, request);

    if (ecu_thread.joinable()) ecu_thread.join();

    ASSERT_EQ(resp.size(), 2);
    EXPECT_EQ(resp[0], 0x7E);
    EXPECT_EQ(resp[1], 0x00);
}

// -------4.0x22 读取存在的数据 -----
TEST_F(UdsServerTest, Handle_0x22_ReadValidDID) {
    IsoTpSender ecu_tx(ecu_socket, 0x7E8, 0x7E0);
    IsoTpReceiver ecu_rx(ecu_socket, 0x7E0, 0x7E8);
    UdsServer server(ecu_tx, ecu_rx);
    std::string fake_vin = "LSVAM4188E2100001"; 
    server.registerReadDID(0xF190, [fake_vin]() {
        return std::vector<uint8_t>(fake_vin.begin(), fake_vin.end());
    });

    std::thread ecu_thread([&]() {
        std::vector<uint8_t> req;
        if (ecu_rx.receive(req, 1000)) server.handleRequest(req);
    });
    std::vector<uint8_t> request = {0x22, 0xF1, 0x90};
    std::vector<uint8_t> resp = sendRequestAndGetResponse(tester_socket, request);

    if (ecu_thread.joinable()) ecu_thread.join();

    ASSERT_EQ(resp.size(), 20);
    EXPECT_EQ(resp[0], 0x62);
    EXPECT_EQ(resp[1], 0xF1);
    EXPECT_EQ(resp[2], 0x90);
    
    std::string rx_vin(resp.begin() + 3, resp.end());
    EXPECT_EQ(rx_vin, fake_vin);
}

// ------5.0x22 读取不存在的 DID ---
TEST_F(UdsServerTest, Handle_0x22_InvalidDID) {
    IsoTpSender ecu_tx(ecu_socket, 0x7E8, 0x7E0);
    IsoTpReceiver ecu_rx(ecu_socket, 0x7E0, 0x7E8);
    UdsServer server(ecu_tx, ecu_rx);

    std::thread ecu_thread([&]() {
        std::vector<uint8_t> req;
        if (ecu_rx.receive(req, 1000)) server.handleRequest(req);
    });

    std::vector<uint8_t> request = {0x22, 0x12, 0x34};
    std::vector<uint8_t> resp = sendRequestAndGetResponse(tester_socket, request);

    if (ecu_thread.joinable()) ecu_thread.join();
    ASSERT_EQ(resp.size(), 3);
    EXPECT_EQ(resp[0], 0x7F);
    EXPECT_EQ(resp[1], 0x22);
    EXPECT_EQ(resp[2], 0x31);
}

// --------6.不支持的服务 -------
TEST_F(UdsServerTest, Handle_UnknownService) {
    IsoTpSender ecu_tx(ecu_socket, 0x7E8, 0x7E0);
    IsoTpReceiver ecu_rx(ecu_socket, 0x7E0, 0x7E8);
    UdsServer server(ecu_tx, ecu_rx);

    std::thread ecu_thread([&]() {
        std::vector<uint8_t> req;
        if (ecu_rx.receive(req, 1000)) server.handleRequest(req);
    });

    std::vector<uint8_t> request = {0x99};
    std::vector<uint8_t> resp = sendRequestAndGetResponse(tester_socket, request);

    if (ecu_thread.joinable()) ecu_thread.join();
    ASSERT_EQ(resp.size(), 3);
    EXPECT_EQ(resp[0], 0x7F);
    EXPECT_EQ(resp[1], 0x99);
    EXPECT_EQ(resp[2], 0x11);
}

//------7.0x27正常解锁流程---
TEST_F(UdsServerTest,Handle_0x27_normalUnlock){
    IsoTpReceiver ecu_rx(ecu_socket,0x7E0,0x7E8);
    IsoTpSender ecu_tx(ecu_socket,0x7E8,0x7E0);
    UdsServer server(ecu_tx,ecu_rx);

    server.registerKeyAlgorithm(UdsServer::SecurityLevel::LEVEL_1,
    [](uint32_t seed){ return seed;}); // 设置一个比较简单的解码算法

    std::thread ecu_thread([&]{
        server.processLoop();
    });
    //resp获得第一阶段的密钥
    std::vector<uint8_t> resp = sendRequestAndGetResponse(tester_socket,{0x27,0x01});
    ASSERT_EQ(resp[0],0x67);

    uint32_t seed = VectorToInt32(resp);
    std::vector<uint8_t> resp1;
    resp1.push_back(0x27);
    resp1.push_back(0x02);
    Int32_ToVector(seed,resp1);
    std::vector<uint8_t> resp2 = sendRequestAndGetResponse(tester_socket,resp1);
    server.stop();
    ecu_thread.join();
    EXPECT_EQ(resp2[0],0x67);
    int lock_state = server.getLockMessage();
    EXPECT_EQ(lock_state, 1);
}

//----8.0x27 防爆破锁定测试 ---
TEST_F(UdsServerTest, Handle_0x27_Lockout) {
    IsoTpReceiver ecu_rx(ecu_socket, 0x7E0, 0x7E8);
    IsoTpSender ecu_tx(ecu_socket, 0x7E8, 0x7E0);
    UdsServer server(ecu_tx, ecu_rx);
    
    server.registerKeyAlgorithm(UdsServer::SecurityLevel::LEVEL_1, [](uint32_t seed){ return seed ^ 0x1234; });
    
    std::thread ecu_thread([&]{ server.processLoop(); });

    for (int i = 0; i < 3; i++) {
        auto resp_seed = sendRequestAndGetResponse(tester_socket, {0x27, 0x01});
        uint32_t seed = VectorToInt32(resp_seed);
        
        std::vector<uint8_t> req_key = {0x27, 0x02};
        Int32_ToVector(seed + 1, req_key); //给错误的key
        sendRequestAndGetResponse(tester_socket, req_key);
    }
    auto resp_lock = sendRequestAndGetResponse(tester_socket, {0x27, 0x01});
    EXPECT_EQ(resp_lock[0], 0x7F);
    EXPECT_EQ(resp_lock[1], 0x27);
    EXPECT_EQ(resp_lock[2], 0x36); 
    server.stop();
    if (ecu_thread.joinable()) ecu_thread.join();
}

//----9.S3 定时器超时降级测试 ---
TEST_F(UdsServerTest, Handle_S3Timeout) {
    IsoTpReceiver ecu_rx(ecu_socket, 0x7E0, 0x7E8);
    IsoTpSender ecu_tx(ecu_socket, 0x7E8, 0x7E0);
    UdsServer server(ecu_tx, ecu_rx);

    std::thread ecu_thread([&]{ server.processLoop(); });

    sendRequestAndGetResponse(tester_socket, {0x10, 0x03});
    EXPECT_EQ(server.getSession(), UdsServer::Session::EXTENDED);
    std::cout << "等待 6 秒，测试 S3 超时..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(6));
    EXPECT_EQ(server.getSession(), UdsServer::Session::DEFAULT);
    server.stop();
    if (ecu_thread.joinable()) ecu_thread.join();
}

//----10.0x2E 正常写入---
TEST_F(UdsServerTest, Handle_0x2E_WriteValidDID) {
    IsoTpReceiver ecu_rx(ecu_socket, 0x7E0, 0x7E8);
    IsoTpSender ecu_tx(ecu_socket, 0x7E8, 0x7E0);
    UdsServer server(ecu_tx, ecu_rx);

    uint8_t current_speed = 0;
    std::string saved_vin = "00000000000000000";
    server.registerDIDWrite(0xF190, [&current_speed, &saved_vin](const std::vector<uint8_t>& data) -> bool {
        if (current_speed != 0) {
            return false; 
        }
        if (data.size() != 17) {
            return false;
        }
        saved_vin = std::string(data.begin(), data.end());
        return true; 
    });

    std::thread ecu_thread([&]{ server.processLoop(); });

    sendRequestAndGetResponse(tester_socket, {0x10, 0x03});
    auto seed_resp = sendRequestAndGetResponse(tester_socket, {0x27, 0x01});
    server.registerKeyAlgorithm(UdsServer::SecurityLevel::LEVEL_1, [](uint32_t seed){ return seed; });
    uint32_t seed = VectorToInt32(seed_resp);
    std::vector<uint8_t> req_key = {0x27, 0x02};
    Int32_ToVector(seed, req_key);
    sendRequestAndGetResponse(tester_socket, req_key);

    std::vector<uint8_t> write_req = {0x2E, 0xF1, 0x90};
    std::string new_vin = "LSVAM4188E2100001";
    write_req.insert(write_req.end(), new_vin.begin(), new_vin.end());

    auto resp = sendRequestAndGetResponse(tester_socket, write_req);
    EXPECT_EQ(resp[0], 0x6E);
    EXPECT_EQ(saved_vin, new_vin); 

    current_speed = 10;
    auto resp_fail = sendRequestAndGetResponse(tester_socket, write_req);
    EXPECT_EQ(resp_fail[0], 0x7F);
    EXPECT_EQ(resp_fail[2], 0x22);
    
    server.stop();
    ecu_thread.join();
}

//----11.0x2E非解锁写入---
TEST_F(UdsServerTest, Handle_0x2E_WriteWithoutUnlock) {
    IsoTpReceiver ecu_rx(ecu_socket, 0x7E0, 0x7E8);
    IsoTpSender ecu_tx(ecu_socket, 0x7E8, 0x7E0);
    UdsServer server(ecu_tx, ecu_rx);

    uint8_t current_speed = 0;
    std::string saved_vin = "00000000000000000";
    server.registerDIDWrite(0xF190, [&current_speed,&saved_vin](const std::vector<uint8_t>& data) -> bool {
        if(current_speed != 0){
            return false;
        }
        if(data.size() != 17){
            return false;
        }
        saved_vin = std::string(data.begin(), data.end());
        return true; 
     });

     std::thread ecu_thread([&]{server.processLoop();});
     std::vector<uint8_t> req = {0x2E,0xF1,0x90};
     std::string new_vin = "LSVAM4188E2100001";
     req.insert(req.end(), new_vin.begin(), new_vin.end());

     auto resp = sendRequestAndGetResponse(tester_socket, req);
     EXPECT_EQ(resp[0],0x7F);
     EXPECT_EQ(resp[2],0x33);
     server.stop();
     ecu_thread.join();
}

//----12.0x31 启动与查结果
TEST_F(UdsServerTest, Handle_0x31_FanRoutine) {
    IsoTpReceiver ecu_rx(ecu_socket, 0x7E0, 0x7E8);
    IsoTpSender ecu_tx(ecu_socket, 0x7E8, 0x7E0);
    UdsServer server(ecu_tx, ecu_rx);

    server.registerRouting(0x0203, [](uint8_t sub_func, const std::vector<uint8_t>& req_data)
     -> std::optional<std::vector<uint8_t>> {
        if (sub_func == 0x01) {
            if(!req_data.empty()){
                std::cout<<"参数为"<<req_data[0]<<std::endl;
            }
            return std::vector<uint8_t>{};
        }
        if (sub_func == 0x02) return std::vector<uint8_t>{};
        if (sub_func == 0x03) return std::vector<uint8_t>{0x00}; 
        return std::nullopt;
    });

    std::thread ecu_thread([&]{ server.processLoop(); });

    auto resp1 = sendRequestAndGetResponse(tester_socket, {0x31, 0x01, 0x02, 0x03});
    EXPECT_EQ(resp1[0], 0x71);
    EXPECT_EQ(resp1[1], 0x01);
    EXPECT_EQ(resp1.size(), 4); 

    auto resp2 = sendRequestAndGetResponse(tester_socket, {0x31, 0x03, 0x02, 0x03});
    EXPECT_EQ(resp2[0], 0x71);
    EXPECT_EQ(resp2[1], 0x03);
    EXPECT_EQ(resp2.size(), 5); 
    EXPECT_EQ(resp2[4], 0x00);  

    auto resp3 = sendRequestAndGetResponse(tester_socket, {0x31, 0x02, 0x02, 0x03});
    EXPECT_EQ(resp3[0], 0x71);
    EXPECT_EQ(resp3[1], 0x02);

    server.stop();
    if (ecu_thread.joinable()) ecu_thread.join();
}