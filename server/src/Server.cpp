#include "Server.hpp"
#include <thread>
#include <algorithm>
#include <stdexcept> 

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>


using namespace std;

SSFTServer::SSFTServer(): threadPool(10){};

void SSFTServer::startServer(int port){
    

    sockaddr_in addr;
    addr.sin_port = htons(port);
    addr.sin_family = AF_UNSPEC;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    serverSocket = socket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == -1){
        perror("opening socket");
        return;
    }

    int err = bind(serverSocket, (sockaddr*)&addr, sizeof(addr));
    if (err == -1){
        perror("binding socket");
        close(serverSocket);
        return;
    }

    err = listen(serverSocket, 100);
    if (err == -1){
        perror("listening socket");
        close(serverSocket);
        return;
    }

    thread(acceptWorker);

    for (;;){
        pollfd* pfdsbuff;
        nfds_t fdn;
        {
            lock_guard<mutex> lk(mu);
            memcpy(pfdsbuff, pfds.data(), pfds.size());
            fdn = pfds.size();
        }
        
        int ready = poll(pfdsbuff, fdn, -1);
        if (ready == -1){
            perror("poll");
            delete pfdsbuff;
            continue;
        }

        for (int i = 0; i < fdn; ++i){
            if (pfdsbuff[i].revents & POLLIN == POLLIN){
                int fd = pfdsbuff[i].fd;
                threadPool.submit([this, fd](){
                    requestHandler(fd);
                });
            }
        }

        delete pfdsbuff;
    } 
}

void SSFTServer::acceptWorker(){
    for(;;){
        int acceptSocket = accept(serverSocket, nullptr, 0);
        if (acceptSocket == -1){
            perror("accepting socket");
            continue;
        }

        pollfd pfd;
        pfd.fd = acceptSocket;
        pfd.events = POLLIN;
        pfd.revents = 0;

        {
            lock_guard<mutex> lk(mu);
            pfds.push_back(pfd);
        }
    }
}

// Redirects the request by it's type 
void SSFTServer::requestHandler(int fd){
    Header header;

    int r = recv(fd, &header, sizeof(header), 0);
    if (r == -1){
        closeConnection(fd);
        return;
    }

    Request request = {header, fd};

    try{
        const auto& it = handlers.find(header.type);
        if (it == handlers.end())
            throw SSFTServerError(ResponseStatus::BAD_REQUEST, "Unknowon request type.");
        
        handlers.at(header.type)(request);
    }catch(const SSFTServerError& err){
        closeConnection(fd);
    }catch(const exception& err){
        closeConnection(fd);
    }
}

void SSFTServer::closeConnection(int fd){
    {
        lock_guard<mutex> lk(mu);
        pfds.erase(remove_if(pfds.begin(), pfds.end(), [fd](pollfd p){
            return p.fd == fd;
        }), pfds.end());
    }
    close(fd);
}

void SSFTServer::handlerAuthenticate(const Request& request){
    
}

void SSFTServer::handlerUpload(const Request& request){

}

void SSFTServer::handlerDownload(const Request& request){

}

void SSFTServer::handlerListFiles(const Request& request){

}
