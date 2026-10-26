#include <gtest/gtest.h>
#include "vcm/signal_database.hpp"
#include <array>
#include <string>

// 测试基类，用于复用加载逻辑
class SignalDatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        // TEST_DATA_DIR 是由 CMake 的 target_compile_definitions 传进来的宏
        std::string json_path = std::string(TEST_DATA_DIR) + "/vehicle.json";
        ASSERT_TRUE(db.load(json_path)) << "Failed to load " << json_path;
    }
    SignalDatabase db;
};

// 1. 测试加载失败的情况（路径错误）
TEST_F(SignalDatabaseTest, LoadInvalidPath) {
    SignalDatabase bad_db;
    EXPECT_FALSE(bad_db.load("/invalid/path/to.json"));
}

// 2. 测试 Intel 跨字节解析 (Speed: 起始位 0, 长度 16, factor 0.01)
// 数据: 0x34, 0x12 -> 小端解析为 0x1234 (4660) -> 4660 * 0.01 = 46.60
TEST_F(SignalDatabaseTest, UnpackIntelCrossByte) {
    std::array<uint8_t, 8> data = {0x34, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    auto result = db.unpack("Speed", data);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result.value(), 46.60);
}

// 3. 测试带负偏移量的信号 (Temp: 起始位 0, 长度 8, factor 1, offset -40)
// 数据: 0x64 (100) -> 100 * 1 + (-40) = 60
TEST_F(SignalDatabaseTest, UnpackWithOffset) {
    std::array<uint8_t, 8> data = {0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    auto result = db.unpack("Temp", data);
    
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result.value(), 60.0);
}

// 4. 测试打包 (Pack) 并验证字节写入
TEST_F(SignalDatabaseTest, PackIntelCrossByte) {
    std::array<uint8_t, 8> data = {0}; // 初始全是 0
    // 打包 46.60 -> raw = 46.60 / 0.01 = 4660 = 0x1234
    // 小端: Byte0 放低位 0x34, Byte1 放高位 0x12
    ASSERT_TRUE(db.pack("Speed", 46.60, data));
    
    EXPECT_EQ(data[0], 0x34);
    EXPECT_EQ(data[1], 0x12);
}

// 5. 测试往返 (Round-Trip)
TEST_F(SignalDatabaseTest, RoundTrip) {
    std::array<uint8_t, 8> data = {0};
    double target_value = 123.45;
    
    // Pack 进去
    ASSERT_TRUE(db.pack("Speed", target_value, data));
    
    // Unpack 出来
    auto result = db.unpack("Speed", data);
    ASSERT_TRUE(result.has_value());
    
    // 验证误差在合理范围内（因为 0.01 精度问题，允许微小误差）
    EXPECT_NEAR(result.value(), target_value, 0.001);
}

// 6. 测试超出范围的打包
TEST_F(SignalDatabaseTest, PackOutOfRange) {
    std::array<uint8_t, 8> data = {0};
    // 假设 DBC 里定义了 Max 是 655.35，这里传 1000.0
    EXPECT_FALSE(db.pack("Speed", 1000.0, data));
}