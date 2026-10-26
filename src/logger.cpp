#include"vcm/logger.hpp"
#include<iostream>
#include<sstream>
#include<iomanip>
#include<ctime>

namespace vcm{
    logger& logger::instance(){
        static logger inst;
        return inst;
    }

    logger::~logger(){
        if(file_.is_open()){
            file_.close();
        }
    }

    const char* logger::levelToString(LogLevel level){
        switch(level){
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::ERROR: return "ERROR";
        }
        return "UNKNOWN";
    }

    void logger::setLevel(LogLevel level){
        std::lock_guard<std::mutex> lock(mutex_);
        level_ = level;
    }

    std::string logger::getCurrentTime(){
        auto now = std::chrono::system_clock::now();
        const std::time_t tm = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf{};

        #ifdef _WIN32
        localtime_s(&tm_buf, &tm);
        #else 
        localtime_r(&tm, &tm_buf);
        #endif

        std::stringstream ss;
        ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%H:%S");
        return ss.str();
    }

    LogLevel logger::level() const{
     std::lock_guard<std::mutex> lock(mutex_);
     return level_;
    }

    bool logger::openFile(const std::string& filename){
        std::lock_guard<std::mutex> lock(mutex_);
        if(file_.is_open()){
            file_.close();
        }
        file_.clear();//清理流的状态
        file_.open(filename,std::ios::app);
        return file_.is_open();
    }

    void logger::close(){
        std::lock_guard<std::mutex> lock(mutex_);
        if(file_.is_open()){
            file_.close();
        }
    }

    void logger::setConsoleOutput(bool enable){
       std::lock_guard<std::mutex> lock(mutex_);
       console_enable = enable;
    }

    bool logger::consoleOutput() const {
     std::lock_guard<std::mutex> lock(mutex_);
     return console_enable;
    }

    void logger::log(vcm::LogLevel level, const std::string& msg){
       std::lock_guard<std::mutex> lock(mutex_);
       if(level < level_) return ;

       std::ostringstream oss;
       oss<<'[' << getCurrentTime() <<']'
       <<'[' << levelToString(level) <<']'
       <<msg<<"\n";
       const std::string line = oss.str();
       if(console_enable){
         std::cout << line;
         std::cout.flush();//工作台输出
       }
       if(file_.is_open()){
        file_<<line;
        file_.flush();//文件流输出
       }
    }
}