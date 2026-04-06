#pragma once

#include <cstdint>
#include <string>
#include "Buffer.hpp"

using namespace std;

enum class LOG_LEVEL {
    FATAL, 
    ERROR,
    WARNING,
    INFO, 
    DEBUG,
    TRACE
};

class Logger {
public:
    static LOG_LEVEL level;
    static int output;
};

class LogStream {
public:

    LogStream(
        const char *filename,
        int line,
        LOG_LEVEL level
    );

    ~LogStream();

    template<typename T>
    LogStream& operator<<(const T& log){
        stream_ << log;
        return *this;
    }
    
private:
    LOG_LEVEL level_;
    Buffer stream_;
    const char *filename_;
    uint32_t line_;
};


#define LOG_FATAL       LogStream(__FILE__, __LINE__, LOG_LEVEL::FATAL)
#define LOG_ERROR       LogStream(__FILE__, __LINE__, LOG_LEVEL::ERROR)
#define LOG_WARNING     LogStream(__FILE__, __LINE__, LOG_LEVEL::WARNING)
#define LOG_INFO        LogStream(__FILE__, __LINE__, LOG_LEVEL::INFO)
#define LOG_DEBUG       LogStream(__FILE__, __LINE__, LOG_LEVEL::DEBUG)
#define LOG_TRACE       LogStream(__FILE__, __LINE__, LOG_LEVEL::TRACE)