#include <gtest/gtest.h>
#include "vcm/ring_buffer.hpp"
#include <thread>
#include <vector>

// 1. 单线程基础测试：验证 FIFO 顺序和边界
TEST(RingBufferTest, BasicPushPop) {
    // 容量设为 4（必须是 2 的幂）。因为牺牲一个格子，实际只能存 3 个。
    RingBuffer<int, 4> rb;

    EXPECT_TRUE(rb.empty());
    EXPECT_FALSE(rb.full());

    // 压入 3 个数据
    EXPECT_TRUE(rb.push(10));
    EXPECT_TRUE(rb.push(20));
    EXPECT_TRUE(rb.push(30));

    // 第 4 个必定失败（满了）
    EXPECT_FALSE(rb.push(40));
    EXPECT_TRUE(rb.full());
    EXPECT_EQ(rb.size(), 3);

    // 弹出并验证 FIFO 顺序
    int val;
    EXPECT_TRUE(rb.pop(val));
    EXPECT_EQ(val, 10);
    
    EXPECT_TRUE(rb.pop(val));
    EXPECT_EQ(val, 20);
    
    EXPECT_TRUE(rb.pop(val));
    EXPECT_EQ(val, 30);

    // 弹空后，不能再弹了
    EXPECT_FALSE(rb.pop(val));
    EXPECT_TRUE(rb.empty());
}

// 2. 多线程极限压测：单生产者 + 单消费者 (SPSC)
TEST(RingBufferTest, MultiThreadStress) {
    // 容量设为 1024，因为数据流转很快
    RingBuffer<int, 1024> rb;
    const int total_items = 100000; // 准备压 10 万个数据

    // 生产者线程
    std::thread producer([&rb]() {
        for (int i = 0; i < total_items; ++i) {
            // 如果队列满了，就死循环等待（自旋）
            // 这里也可以用 std::this_thread::yield() 让出 CPU 片
            while (!rb.push(i)) {
                std::this_thread::yield();
            }
        }
    });

    // 消费者线程
    std::thread consumer([&rb]() {
        int expected_val = 0;
        int val = 0;
        while (expected_val < total_items) {
            if (rb.pop(val)) {
                // 核心验证：取出来的数据，必须和先进去的顺序一模一样（无丢包，无乱序）
                ASSERT_EQ(val, expected_val);
                expected_val++;
            } else {
                std::this_thread::yield();
            }
        }
    });

    // 等待两个线程结束
    producer.join();
    consumer.join();

    // 最终队列应该是空的
    EXPECT_TRUE(rb.empty());
}

// 3. 多轮循环测试：验证游标无限回绕
TEST(RingBufferTest, WrapAround) {
    RingBuffer<int, 4> rb; // 实际容量 3
    
    // 我们把队列压满，再取空，重复 1000 次。
    // 这会让 write_idx 和 read_idx 不断越过 4 的边界，测试位运算取模是否正确。
    for (int round = 0; round < 50; ++round) {
        EXPECT_TRUE(rb.push(round));
        EXPECT_TRUE(rb.push(round + 1));
        EXPECT_TRUE(rb.push(round + 2));
        
        int val;
        EXPECT_TRUE(rb.pop(val));
        EXPECT_EQ(val, round);
        EXPECT_TRUE(rb.pop(val));
        EXPECT_EQ(val, round + 1);
        EXPECT_TRUE(rb.pop(val));
        EXPECT_EQ(val, round + 2);
    }
}