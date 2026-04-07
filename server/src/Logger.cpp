#include "Logger.hpp"
#include <cstdio>
#include <string.h>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <iostream>
#include <stdexcept>

namespace cr = chrono;

static string getLevelString(LOG_LEVEL level){
    switch (level)
    {
    case LOG_LEVEL::FATAL:
        return "FATAL";
    case LOG_LEVEL::ERROR:
        return "ERROR";
    case LOG_LEVEL::WARNING:
        return "WARNING";
    case LOG_LEVEL::INFO:
        return "INFO";
    case LOG_LEVEL::DEBUG:
        return "DEBUG";
    case LOG_LEVEL::TRACE:
        return "TRACE";
    }
}

void Logger::write(
    const char* filename,
    int line,
    LOG_LEVEL msglevel,
    string msg
){
    if (level < msglevel) return;
    auto now = cr::system_clock::now();
    time_t unixt = cr::system_clock::to_time_t(now);
    tm* timestamp = localtime(&unixt);

    char buff[32];
    strftime(buff, sizeof(buff), "%d-%m-%Y %H:%M:%S", timestamp);
    int output = 1;
    if ((int)msglevel <= 1) output = 2;

    dprintf(output, "[%s] %s %s:%d %s\n", 
        buff, 
        getLevelString(msglevel).c_str(), 
        filename, 
        line, 
        msg.c_str()
    );
};

void Logger::setLogLevel(const string& debugLevelStr){
    if (debugLevelStr == "FATAL")           level = LOG_LEVEL::FATAL;
    else if (debugLevelStr == "ERROR")      level = LOG_LEVEL::ERROR;
    else if (debugLevelStr == "WARNING")    level = LOG_LEVEL::WARNING;
    else if (debugLevelStr == "INFO")       level = LOG_LEVEL::INFO;
    else if (debugLevelStr == "DEBUG")      level = LOG_LEVEL::DEBUG;
    else if (debugLevelStr == "TRACE")      level = LOG_LEVEL::TRACE;
    else 
        throw runtime_error("Log level option invalid.");
}

LOG_LEVEL Logger::level = LOG_LEVEL::INFO;

LogStream::LogStream(
    const char *filename,
    int line,
    LOG_LEVEL level
): filename_{filename}, line_{line}, level_{level}{};

LogStream::~LogStream(){
    Logger::write(filename_, line_, level_, stream_.str());
}