#include "Server.hpp"
#include <iostream>
#include <chrono>
#include <mutex>
#include <Logger.hpp>

#include <stdlib.h>

int main(int argc, char *argv[]){
    if (argc == 2){
        
    }

    SSFTServer server;
    server.startServer(8080);
}