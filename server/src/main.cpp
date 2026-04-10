#include "Server.hpp"
#include "Types.hpp"
#include <iostream>
#include <chrono>
#include <mutex>
#include <Logger.hpp>

#include <stdlib.h>

int main(int argc, char *argv[]){
    if (argc == 2)
        Logger::setLogLevel(string(argv[1]));       

    SSFTServer server;
    server.startServer(PORT);
}