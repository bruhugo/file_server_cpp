#include "Server.hpp"
#include <thread>
#include <algorithm>
#include <stdexcept> 
#include <filesystem>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <Logger.hpp>

#include <sys/socket.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <errno.h>

fs::path APP_STORAGE_DIRECTORY;
fs::path APP_DATABASE_FILE;

SSFTServer::SSFTServer(): threadPool(10){};

void SSFTServer::startServer(int port){
    char *homedir = getenv("HOME");
    if (homedir == nullptr) {
        LOG_FATAL << "Home env variable is not set.";
        return;
    }

    fs::path p(homedir);
    APP_STORAGE_DIRECTORY = p / ".ssftserver";
    APP_DATABASE_FILE = APP_STORAGE_DIRECTORY / "users";

    if (!fs::exists(APP_STORAGE_DIRECTORY)){
        LOG_DEBUG << "Create directory " << APP_STORAGE_DIRECTORY;
        fs::create_directories(APP_STORAGE_DIRECTORY);
    }

    sockaddr_in addr;
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    LOG_INFO << "Server is starting...";

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == -1){
        perror("opening socket");
        return;
    }

    int optval = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)));

    int err = bind(serverSocket, (sockaddr*)&addr, sizeof(addr));
    if (err == -1){
        perror("binding socket");
        close(serverSocket);
        return;
    }

    err = listen(serverSocket, 20);
    if (err == -1){
        perror("listening socket");
        close(serverSocket);
        return;
    }

    LOG_INFO << "Server started, listening for connections.";

    efd = eventfd(0, EFD_NONBLOCK);
    pollfd epollfd;
    epollfd.fd = efd;
    epollfd.events = POLLIN;
    epollfd.revents = 0;
    pfds.push_back(epollfd);

    thread acceptThread([this]{acceptWorker();});
    acceptThread.detach();

    for (;;){
        vector<pollfd> pfdsbuff;
        {
            lock_guard<mutex> lk(mu);
            pfdsbuff = pfds;
        }
        
        int ready = poll(pfdsbuff.data(), pfdsbuff.size(), -1);
        if (ready == -1){
            perror("poll");
            continue;
        }

        for (size_t i = 0; i < pfdsbuff.size(); ++i){
            if (pfdsbuff[i].fd == efd){
                uint64_t i;
                read(efd, &i, sizeof(i));
                continue;
            };

            if (pfdsbuff[i].revents & POLLIN){
                int fd = pfdsbuff[i].fd;

                {
                    lock_guard lk(mu);
                    for (int i = 0; i < pfds.size(); i++)
                        if (pfds[i].fd == fd) pfds.erase(pfds.begin() + i);
                }

                threadPool.submit([this, fd](){
                    requestHandler(fd);
                });
            }
        }
    } 
}

void SSFTServer::acceptWorker(){
    for(;;){
        int acceptSocket = accept(serverSocket, nullptr, 0);
        if (acceptSocket == -1){
            perror("accepting socket");
            continue;
        }

        LOG_INFO << "Connection accepted on socket: " << acceptSocket;

        timeval optval = {5, 0};
        if (setsockopt(acceptSocket, SOL_SOCKET, SO_RCVTIMEO, &optval, sizeof(optval)) == -1){
            perror("seting socket option");
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

        uint64_t val = 1;
        write(efd, &val, sizeof(val));
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
    }catch(SSFTServerError& err){
        LOG_ERROR << err.what();
        sendResponseHeader(err.statusCode, 0, fd);
    }catch(const exception& err){
        LOG_ERROR << err.what();
        sendResponseHeader(ResponseStatus::SERVER_SIDE_ERROR, 0, fd);
    }
    closeConnection(fd);
}

void SSFTServer::closeConnection(int fd){
    {
        lock_guard<mutex> lk(mu);
        pfds.erase(remove_if(pfds.begin(), pfds.end(), [fd](pollfd p){
            return p.fd == fd;
        }), pfds.end());
    }
    close(fd);
    LOG_DEBUG << "Connection closed at socket " << fd;
    
}

void SSFTServer::handlerAuthenticate(const Request& request){  
    LOG_DEBUG << "Authenticate request received at socket " << request.fd;
    UsernamePassword buff;

    fstream userfile(APP_DATABASE_FILE, ios::binary | ios::in | ios::out);
    if (!userfile.is_open()) {
        LOG_DEBUG << "Creating users file at path " << APP_DATABASE_FILE;
        fs::create_directories(APP_STORAGE_DIRECTORY);
        {
            ofstream create(APP_DATABASE_FILE);
        }
        userfile.open(APP_DATABASE_FILE, ios::binary | ios::in | ios::out);
    }

    while (userfile.read((char*)&buff, sizeof(buff))){
        if (memcmp(buff.username, request.header.username, sizeof(buff.username)) == 0) 
            throw SSFTServerError(ResponseStatus::BAD_REQUEST, "User conflict");
    }

    UsernamePassword write;
    memcpy(&write.password, &request.header.password,sizeof(write.password));
    memcpy(&write.username, &request.header.username, sizeof(write.username));

    userfile.clear();
    userfile.seekp(0, ios::end);
    if(!userfile.write((char*)&write, sizeof(write)))
        throw runtime_error(strerror(errno));
    
    sendResponseHeader(ResponseStatus::SUCCESS, 0, request.fd);
}

void SSFTServer::handlerUpload(const Request& request){
    LOG_DEBUG << "Upload request received at socket " << request.fd;
    if (!isAllowed(request))
        throw SSFTServerError(ResponseStatus::UNAUTHORIZED, "Unautorized.");

    string username(reinterpret_cast<const char*>(request.header.username));
    string filename(reinterpret_cast<const char*>(request.header.filename));
    
    fs::path filepath = APP_STORAGE_DIRECTORY / username / filename; 

    fs::create_directories(filepath.parent_path());
    ofstream file(filepath, ios::binary | ios::out);

    uint32_t read = request.header.filesize;
    char buff[BUFFER_SIZE];

    LOG_DEBUG << "Writing to file " << filepath;
    while (read > 0){
        int32_t r = recv(request.fd, buff, sizeof(buff), 0);
        if (r == -1){
            fs::remove(filepath);
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                throw SSFTServerError(ResponseStatus::FILE_SIZE, "File size is smaller than file size provided.");
            throw SSFTServerError(ResponseStatus::SERVER_SIDE_ERROR, strerror(errno));
        }else if (r == 0){
            fs::remove(filepath);
            throw SSFTServerError(ResponseStatus::SERVER_SIDE_ERROR, "Socket closed while reading the file.");
        }
        read -= r;
        if (!file.write(buff, r)){
            LOG_ERROR << "Error writing to file " << filepath << " at socket " << request.fd << ": " << strerror(errno);
            throw SSFTServerError(ResponseStatus::SERVER_SIDE_ERROR, "Could not write to file.");
        }
    }

    LOG_DEBUG << "File received on socket " << request.fd;

    sendResponseHeader(ResponseStatus::SUCCESS, 0, request.fd);
}

void SSFTServer::handlerDownload(const Request& request){
    LOG_DEBUG << "Download request received at socket " << request.fd;
    if (!isAllowed(request))
        throw SSFTServerError(ResponseStatus::UNAUTHORIZED, "Unauthorized.");

    string username(reinterpret_cast<const char*>(request.header.username));
    string filename(reinterpret_cast<const char*>(request.header.filename));

    fs::path filepath = APP_STORAGE_DIRECTORY;
    filepath.append(username);
    filepath.append(filename);

    if (!fs::exists(filepath))
        throw SSFTServerError(ResponseStatus::FILE_NOT_FOUND, "File could not be found.");
    
    ifstream file(filepath, ios::in | ios::binary);
    if (!file.is_open())
        throw SSFTServerError(ResponseStatus::FILE_NOT_FOUND, "File could not be found.");
    
    uint32_t maxsent = 0;
    uint32_t filesize = fs::file_size(filepath);
    char buff[BUFFER_SIZE];
    sendResponseHeader(ResponseStatus::SUCCESS, filesize, request.fd);
    while (maxsent < filesize){
        file.read(buff, sizeof(buff));
        int sent = file.gcount();
        if (sent == 0) break;

        maxsent += sent;
        send(request.fd, buff, sent, 0);
    }
}

void SSFTServer::handlerListFiles(const Request& request){
    LOG_DEBUG << "List files request received at socket " << request.fd;
    if (!isAllowed(request))
        throw SSFTServerError(ResponseStatus::UNAUTHORIZED, "Unauthorized.");

    string username(reinterpret_cast<const char*>(request.header.username));
    fs::path userPath = APP_STORAGE_DIRECTORY / username;
    if (!fs::exists(userPath)){
        fs::create_directory(userPath);
    }else if(!fs::is_directory(userPath)) {
        fs::remove(userPath);
        fs::create_directory(userPath);
    }
    
    LOG_DEBUG << "Listing files in directory " << userPath;
    vector<string> names;
    for (const auto& dirEntry : fs::directory_iterator(userPath))
        names.push_back(dirEntry.path().filename());
    
    size_t size = names.size();

    sendResponseHeader(ResponseStatus::SUCCESS, size, request.fd);
    char nameBuffer[sizeof(RequestHeader::filename)];
    for (auto& name : names){
        memset(nameBuffer, 0, sizeof(nameBuffer));
        memcpy(nameBuffer, name.data(), name.size());
        
        if (send(request.fd, name.data(), sizeof(RequestHeader::filename), 0) == -1)
            throw SSFTServerError(ResponseStatus::SERVER_SIDE_ERROR, "Error sending filenames.");
    }
}

bool SSFTServer::isAllowed(const Request& request){
    UsernamePassword buff;
    fstream userfile(APP_DATABASE_FILE, ios::binary | ios::in | ios::out);
    if (!userfile.is_open()){
        userfile.open(APP_DATABASE_FILE, ios::out);
    }

    while (userfile.read(reinterpret_cast<char*>(&buff), sizeof(buff))){
        if (memcmp(buff.username, request.header.username, sizeof(buff.username)) == 0 &&
            memcmp(buff.password, request.header.password, sizeof(buff.password)) == 0){
            return true;
        }       
    }

    LOG_DEBUG << "Unauthorized request at socket " << request.fd;

    return false;
}

void SSFTServer::sendResponseHeader(ResponseStatus status, uint32_t filesize, int fd){
    ResponseHeader responseHeader{status, filesize};
    uint32_t sent = send(fd, reinterpret_cast<char*>(&responseHeader), sizeof(responseHeader), 0);
    if (sent < sizeof(responseHeader)){
        throw SSFTServerError(ResponseStatus::SERVER_SIDE_ERROR, "Error sending response.");
    }
}
