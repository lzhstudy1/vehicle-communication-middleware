#pragma once

#include<mutex>
#include<string>
#include<chrono>
#include<fstream>
#include<iostream>
#include<sstream>
#include<iomanip>
#include<ctime>

namespace vcm{
    enum class LogLevel{
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3
    };

    class logger{
        public:
        static logger& instance();

        logger(const logger& other) = delete;
        logger& operator = (const logger& other) = delete;

        void setLevel(LogLevel level);//
        LogLevel level() const;//
        void log(LogLevel level, const std::string &msg);//

        bool openFile(const std::string &filename);//
        void close();//

        void setConsoleOutput(bool enable);
        bool consoleOutput() const;

        private:
        logger() = default;
        ~logger();//
        static const char* levelToString(LogLevel level);//
        static std::string getCurrentTime();//

        mutable std::mutex mutex_;
        LogLevel level_  = LogLevel::INFO;
        bool console_enable = true;
        std::ofstream  file_;
    };
}

#define LOG_DEBUG(msg) ::vcm::logger::instance().log(::vcm::LogLevel::DEBUG, (msg))
#define LOG_INFO(msg) ::vcm::logger::instance().log(::vcm::LogLevel::INFO, (msg))
#define LOG_WARNING(msg) ::vcm::logger::instance().log(::vcm::LogLevel::WARNING, (msg))
#define LOG_ERROR(msg) ::vcm::logger::instance().log(::vcm::LogLevel::ERROR, (msg))