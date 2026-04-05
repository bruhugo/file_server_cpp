#pragma once

#include "ThreadPool.hpp"
#include "Types.hpp"
#include <vector>
#include <mutex>
#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <cstdint>

#include <poll.h>

using Handler = function<void(const Request& request)>;

class SSFTServer {
public:
    SSFTServer();
    void startServer(int port);

private:

    void handlerAuthenticate(const Request& request);   
    void handlerUpload(const Request& request);
    void handlerDownload(const Request& request);
    void handlerListFiles(const Request& request);

    bool isAllowed(const Request&);

    void acceptWorker();
    void requestHandler(int fd);
    void closeConnection(int fd);
    void sendResponseHeader(ResponseStatus status, uint32_t filesize, int fd);

    ThreadPool threadPool;
    mutex mu;

    int serverSocket;
    vector<pollfd> pfds;
    unordered_map<RequestType, Handler> handlers = {
        {RequestType::AUTHENTICATE, [this](const Request& r){handlerAuthenticate(r); }},
        {RequestType::DOWNLOAD,     [this](const Request& r){handlerUpload(r);       }},
        {RequestType::UPLOAD,       [this](const Request& r){handlerDownload(r);     }},
        {RequestType::LIST,         [this](const Request& r){handlerAuthenticate(r); }},
    };
};
