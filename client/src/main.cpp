#include <iostream>
#include <Logger.hpp>

int main(int argc, char *argv[]){

    if (argc == 2)
        Logger::setLogLevel(string(argv[1]));

    LOG_DEBUG << "This is debug.";
    LOG_INFO << "This is info.";

    return 0;
}