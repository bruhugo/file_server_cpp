#include "Logger.hpp"
#include <cstdio>
#include <chrono>
#include <ctime>

namespace cr = chrono;

LogStream::LogStream(
    const char *filename,
    int line,
    LOG_LEVEL level
): filename_{filename}, line_{line}, level_{level}{};

LogStream::~LogStream(){
    if (Logger::level < level_) return;
    auto now = cr::system_clock::now();
    time_t unixt = cr::system_clock::to_time_t(now);
    tm* timestamp = localtime(&unixt);

    char buff[32];
    strftime(buff, sizeof(buff), "%d-%m-%Y %H:%M:%S", timestamp);
    char *buff = reinterpret_cast<char*>(buff);

    string finalMsg = "[" + string(buff, sizeof(buff)) + "]" + " ";
}