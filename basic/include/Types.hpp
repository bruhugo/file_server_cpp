#pragma once
#include <cstdint>
#include <filesystem>

#define BUFFER_SIZE 2048
#define PORT_STR "7890"
#define PORT 7890

namespace fs = std::filesystem;
using namespace std;

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
    BAD_REQUEST,
    FILE_NOT_FOUND
};

string statusMessage(ResponseStatus status);

struct UsernamePassword {
    uint8_t username[32];
    uint8_t password[32];
};

#pragma pack(push, 1)
struct RequestHeader {
    RequestType type;
    uint8_t username[32];
    uint8_t password[32];
    uint8_t filename[64];
    uint32_t filesize;
};

struct ResponseHeader {
    ResponseStatus responseStatus;
    uint32_t filesize;
};
#pragma pack(pop)

struct Request {
    RequestHeader header;
    int fd;
};

struct SSFTServerError : exception {
public:
    SSFTServerError(ResponseStatus sc, char* m): statusCode{sc}, msg{m} {}

    const char* what();

    ResponseStatus statusCode;
private:
    char* msg;
};