#include "Server.hpp"
#include <thread>
#include <algorithm>
#include <stdexcept> 
#include <filesystem>
#include <iostream>
#include <fstream>
#include <cstdint>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>

namespace fs = std::filesystem;
using namespace std;

const fs::path APP_STORAGE_DIRECTORY = "/var/lib/ssftserver";
const fs::path APP_DATABASE_FILE = APP_STORAGE_DIRECTORY / "users.db";

SSFTServer::SSFTServer(): threadPool(10){};

void SSFTServer::startServer(int port){
    if (!fs::exists(APP_STORAGE_DIRECTORY))
        fs::create_directory(APP_STORAGE_DIRECTORY);

    sockaddr_in addr;
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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

    thread acceptThread([&]{acceptWorker();});

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

void SSFTServer::requestHandler(int fd){
    RequestHeader header;

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
    UsernamePassword buff;
    fstream userfile(APP_DATABASE_FILE, ios::binary | ios::in | ios::out);

    while (userfile.read((char*)&buff, sizeof(buff))){
        if (memcmp(buff.username, request.header.username, sizeof(buff.username)) != 0) 
            throw SSFTServerError(ResponseStatus::BAD_REQUEST, "User conflict");
    }

    UsernamePassword write;
    memcpy(&write.password, &request.header.password, sizeof(write.password));
    memcpy(&write.username, &request.header.username, sizeof(write.username));

    userfile.clear();
    userfile.seekp(0, ios::end);
    userfile.write((char*)&write, sizeof(write));
}

void SSFTServer::handlerUpload(const Request& request){
    if (!isAllowed(request))
        throw SSFTServerError(ResponseStatus::UNAUTHORIZED, "User is not authenticated.");

    string username(
        reinterpret_cast<const char*>(request.header.username), 
        sizeof(request.header.username)
    );
    
    string filename(
        reinterpret_cast<const char*>(request.header.filename), 
        sizeof(request.header.filename)
    );
    
    fs::path filepath = APP_STORAGE_DIRECTORY; 
    filepath.append(username);
    filepath.append(filename);

    fs::path tempPath = "/var/tmp";
    tempPath.append(username);
    tempPath.append(filename);

    fs::create_directories(tempPath.parent_path());

    ofstream temp(tempPath, ios::out | ios::binary);
    if(!temp.is_open())
        temp.open(tempPath, ios::out | ios::binary);
    
    uint32_t read = request.header.filesize;
    char buff[BUFFER_SIZE];

    while (read > 0){
        int32_t r = recv(request.fd, buff, sizeof(buff), MSG_DONTWAIT);
        if (r == -1){
            fs::remove(tempPath);
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                throw SSFTServerError(ResponseStatus::FILE_SIZE, "File size is smaller than file size provided.");
            throw SSFTServerError(ResponseStatus::SERVER_SIDE_ERROR, strerror(errno));
        }
        read -= r;
        temp.write(buff, r);
    }

    fs::copy_file(tempPath, filepath);
    fs::remove(tempPath);

    sendResponseHeader(ResponseStatus::SUCCESS, 0, request.fd);
}

void SSFTServer::handlerDownload(const Request& request){
    if (!isAllowed(request))
        throw SSFTServerError(ResponseStatus::UNAUTHORIZED, "File size is smaller than file size provided.");

    string username(
        reinterpret_cast<const char*>(request.header.username), 
        sizeof(request.header.username)
    );
    
    string filename(
        reinterpret_cast<const char*>(request.header.filename), 
        sizeof(request.header.filename)
    );
    
    fs::path filepath = APP_STORAGE_DIRECTORY;
    filepath.append(username);
    filepath.append(filename);

    if (!fs::exists(filepath))
        throw SSFTServerError(ResponseStatus::FILE_NOT_FOUND, "File could not be found.");
    
    ifstream file(filepath, ios::in | ios::binary);
    if (!file.is_open())
        file.open(filepath, ios::in | ios::binary);
    
    uint32_t maxsent = 0;
    uint32_t filesize = fs::file_size(filepath);
    char buff[BUFFER_SIZE];
    sendResponseHeader(ResponseStatus::SUCCESS, filesize, request.fd);
    while (maxsent < filesize){
        if (!file.read(buff, sizeof(buff)))
            return;

        int sent = file.gcount();
        maxsent += sent;
        send(request.fd, buff, sent, 0);
    }
}

void SSFTServer::handlerListFiles(const Request& request){
    
}

bool SSFTServer::isAllowed(const Request& request){
    UsernamePassword buff;
    fstream userfile(APP_DATABASE_FILE, ios::binary | ios::in | ios::out);
    if (!userfile.is_open()){
        userfile.open(APP_DATABASE_FILE, ios::out);
    }

    while (userfile.read(reinterpret_cast<char*>(&buff), sizeof(buff))){
        if (memcmp(buff.username, request.header.username, sizeof(buff.username)) != 0 &&
            memcmp(buff.password, request.header.password, sizeof(buff.password)) != 0){
            return true;
        }       
    }

    return false;
}

void SSFTServer::sendResponseHeader(ResponseStatus status, uint32_t filesize, int fd){
    ResponseHeader responseHeader{status, filesize};
    uint32_t sent = send(fd, reinterpret_cast<char*>(&responseHeader), sizeof(responseHeader), 0);
    if (sent < sizeof(responseHeader)){
        throw SSFTServerError(ResponseStatus::SERVER_SIDE_ERROR, "Error sending response.");
    }
}
