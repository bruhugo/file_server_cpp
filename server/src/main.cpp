#include "Server.hpp"
#include <iostream>
#include <chrono>
#include <mutex>

#include <stdlib.h>

int main(int argc, char *argv[]){
    SSFTServer server;
    server.startServer(8080);
}