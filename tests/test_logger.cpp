#include <gtest/gtest.h>
#include "vcm/logger.hpp"

#include <thread>
#include <vector>

// ---------- 1. 单例验证 ----------
TEST(LoggerTest, Singleton) {
    vcm::logger& a = vcm::logger::instance();
    vcm::logger& b = vcm::logger::instance();
    EXPECT_EQ(&a, &b);
}

// ---------- 2. 设置并读取日志级别 ----------
TEST(LoggerTest, SetAndGetLevel) {
    vcm::logger::instance().setLevel(vcm::LogLevel::WARNING);
    EXPECT_EQ(vcm::logger::instance().level(), vcm::LogLevel::WARNING);

    vcm::logger::instance().setLevel(vcm::LogLevel::DEBUG);
    EXPECT_EQ(vcm::logger::instance().level(), vcm::LogLevel::DEBUG);
}

// ---------- 3. 级别过滤 ----------
TEST(LoggerTest, LevelFilter) {
    vcm::logger::instance().setLevel(vcm::LogLevel::WARNING);

    LOG_DEBUG("this should be filtered");
    LOG_INFO("this should be filtered");
    LOG_WARNING("this should be printed");
    LOG_ERROR("this should be printed");

    vcm::logger::instance().setLevel(vcm::LogLevel::INFO);
    SUCCEED();
}

// ---------- 4. 控制台输出开关 ----------
TEST(LoggerTest, ConsoleOutputToggle) {
    vcm::logger::instance().setConsoleOutput(false);
    EXPECT_FALSE(vcm::logger::instance().consoleOutput());

    vcm::logger::instance().setConsoleOutput(true);
    EXPECT_TRUE(vcm::logger::instance().consoleOutput());
}

// ---------- 5. 文件输出 ----------
TEST(LoggerTest, FileOutput) {
    bool ok = vcm::logger::instance().openFile("../vcm.log");
    EXPECT_TRUE(ok);

    LOG_INFO("test file output");

    vcm::logger::instance().close();
    SUCCEED();
}

// ---------- 6. 多线程写日志 ----------
TEST(LoggerTest, MultiThreadLog) {
    vcm::logger::instance().setLevel(vcm::LogLevel::INFO);

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([] {
            for (int j = 0; j < 100; ++j) {
                LOG_INFO("thread message");
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
    SUCCEED();
}