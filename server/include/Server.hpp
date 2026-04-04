#include "ThreadPool.hpp"
#include <vector>
#include <mutex>
#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <cstdint>

#include <poll.h>

#define BUFFER_SIZE 2048

// REQUEST TYPES
enum class RequestType : uint8_t {
    AUTHENTICATE,    
    UPLOAD,          
    DOWNLOAD,        
    LIST            
};

// RESPONSE STATUS
enum class ResponseStatus : uint8_t {
    SUCCESS,
    UNAUTHORIZED,
    FILE_SIZE,
    SERVER_SIDE_ERROR,
    BAD_REQUEST
};

using namespace std;

#pragma pack(push, 1)
struct Header {
    RequestType type;
    uint8_t username[32];
    uint8_t password[32];
    uint8_t filename[64];
    uint32_t filesize;
};
#pragma pack(pop)

struct Request {
    Header header;
    int fd;
};

struct SSFTServerError : exception {
public:
    SSFTServerError(ResponseStatus sc, char* m): statusCode{sc}, msg{m} {}

    const char* what(){
        return msg;
    }

private:
    char* msg;
    ResponseStatus statusCode;
};

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

    void acceptWorker();
    void requestHandler(int fd);
    void closeConnection(int fd);

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
