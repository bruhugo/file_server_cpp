#pragma once
#include <cstdint>
#include <filesystem>

#define BUFFER_SIZE 2048

namespace fs = std::filesystem;
using namespace std;

const fs::path APP_STORAGE_DIRECTORY = "/var/lib/ssftserver";
const fs::path APP_DATABASE_FILE = APP_STORAGE_DIRECTORY / "users.db";

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

    const char* what(){
        return msg;
    }

private:
    char* msg;
    ResponseStatus statusCode;
};