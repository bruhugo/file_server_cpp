#pragma once

#include <cstdint>
#include <string>

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
};

class LogStream {
public:

    LogStream(
        const char *filename,
        int line,
        LOG_LEVEL level
    );

    ~LogStream();

    LogStream& operator<<(const string& str);
    LogStream& operator<<(const int& number);
    LogStream& operator<<(const float& decimal);
    
private:
    LOG_LEVEL level_;
    // I know I could use a stringstream, 
    // but I want to handle it myself
    string msg_;
    string filename_;
    uint32_t line_;
};



#define LOG_FATAL       LogStream(__FILE__, __LINE__, LOG_LEVEL::FATAL)
#define LOG_ERROR       LogStream(__FILE__, __LINE__, LOG_LEVEL::ERROR)
#define LOG_WARNING     LogStream(__FILE__, __LINE__, LOG_LEVEL::WARNING)
#define LOG_INFO        LogStream(__FILE__, __LINE__, LOG_LEVEL::INFO)
#define LOG_DEBUG       LogStream(__FILE__, __LINE__, LOG_LEVEL::DEBUG)
#define LOG_TRACE       LogStream(__FILE__, __LINE__, LOG_LEVEL::TRACE)