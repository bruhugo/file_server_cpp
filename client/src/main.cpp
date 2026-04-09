#include <iostream>
#include <stdexcept>
#include <functional>
#include <filesystem>
#include <fstream>
#include <string>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "Connection.hpp"

using namespace std;
namespace fs = filesystem;

struct RequestInfo {
    RequestHeader header;
    string hostname;
    string filepath;
};

RequestInfo parseArgs(int argc, char *argv[]);
RequestType getRequestTypeByName(const string& name);
void setFilenameAndFilesize(char *filepath, const RequestInfo& requestInfo);

ResponseHeader downloadFileHandler(   RequestInfo& req, Connection& conn);
ResponseHeader uploadFileHandler(     RequestInfo& req, Connection& conn);
ResponseHeader listFileHandler(       RequestInfo& req, Connection& conn);
ResponseHeader authenticateHandler(   RequestInfo& req, Connection& conn);


int main(int argc, char *argv[]){
    try{
        RequestInfo requestInfo = parseArgs(argc, argv);
        Connection conn(requestInfo.hostname);
        ResponseHeader h;
        
        switch (requestInfo.header.type){
        case RequestType::AUTHENTICATE:
            h = authenticateHandler(requestInfo, conn);
            break;
        case RequestType::DOWNLOAD:
            h = downloadFileHandler(requestInfo, conn);
            break;
        case RequestType::LIST:
            h = listFileHandler(requestInfo, conn);
            break;
        case RequestType::UPLOAD:
            h = uploadFileHandler(requestInfo, conn);
            break;
        default:
            throw runtime_error("Uknown request type provided.");
        }

        cout << statusMessage(h.responseStatus) << endl;
        
    }catch(const exception& err){
        cout << err.what() << endl;
    }
}

RequestInfo parseArgs(int argc, char *argv[]) {
    if (argc < 4)
        throw runtime_error("Must provide hostname, username and command.");

    RequestInfo requestInfo;
    memset(&requestInfo.header, 0, sizeof(requestInfo.header));

    // HOST
    requestInfo.hostname = string(argv[1]);

    // TYPE
    requestInfo.header.type = getRequestTypeByName(string(argv[3]));

    // USERNAME
    size_t usernameLen = strlen(argv[2]);
    if (usernameLen > sizeof(RequestHeader::username))
        usernameLen = sizeof(RequestHeader::username);
    memcpy(requestInfo.header.username, argv[2], usernameLen);

    // PASSWORD
    termios oldconfig, newconfig;
    tcgetattr(STDIN_FILENO, &oldconfig);
    newconfig = oldconfig;
    newconfig.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &newconfig);

    string password;
    cout << "Please, enter the password: ";
    getline(cin, password);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldconfig);
    cout << endl;

    size_t pwdLen = password.size();
    if (pwdLen > sizeof(RequestHeader::password))
        pwdLen = sizeof(RequestHeader::password);
    memcpy(requestInfo.header.password, password.c_str(), pwdLen);

    if (argc == 5) {
        uint32_t min = strlen(argv[4]);
        if (sizeof(RequestHeader::filename) < min) min = sizeof(RequestHeader::filename);
        
        if (requestInfo.header.type == RequestType::DOWNLOAD)
            memcpy(requestInfo.header.filename, argv[4], min);
        else
            requestInfo.filepath = string(argv[4]);
    }

    return requestInfo;
}

RequestType getRequestTypeByName(const string& name){
    if (name == "download") 
        return RequestType::DOWNLOAD;
    else if (name == "upload") 
        return RequestType::UPLOAD;
    else if (name == "list") 
        return RequestType::LIST;
    else if (name == "authenticate") 
        return RequestType::AUTHENTICATE;
    else
        throw runtime_error("Uknown request type provided");
}

void setFilenameAndFilesize(char *filepathstr, RequestInfo& requestInfo){
    fs::path filepath(filepathstr);
    if (!fs::exists(filepath))
        throw runtime_error("File given does not exist."); 

    requestInfo.header.filesize = static_cast<uint32_t>(fs::file_size(filepath));

    string filename = filepath.filename();
    memcpy(requestInfo.header.filename, filename.c_str(), filename.size());
}

ResponseHeader downloadFileHandler(RequestInfo& req, Connection& conn){
    ResponseHeader header = conn.recvHeader();
    string filename(reinterpret_cast<char*>(req.header.filename));

    conn.recvFile(filename, header.filesize);
}

ResponseHeader uploadFileHandler(RequestInfo& req, Connection& conn){
    fs::path filepath(req.filepath);
    if (!fs::exists(filepath))
        throw runtime_error("File does not exist.");
    else if (fs::is_directory(filepath)){
        throw runtime_error("Must be a file, not a directory.");
    }

    string filename = filepath.filename();
    memcpy(req.header.filename, filename.c_str(), filename.size());

    ifstream file(filepath, ios::binary | ios::in);
    if(file.is_open()){
        file.seekg(0, ios::end);
        req.header.filesize = file.tellg();
        file.seekg(0, ios::beg);
    }else {
        throw runtime_error("Failed to open file.");
    }

    conn.sendData(&req.header, sizeof(req.header));

    char buff[BUFFER_SIZE];
    while (file.read(buff, BUFFER_SIZE))
        conn.sendData(buff, BUFFER_SIZE);

    if (file.gcount() > 0)
        conn.sendData(buff, file.gcount());

    return conn.recvHeader();
}

ResponseHeader listFileHandler(RequestInfo& req, Connection& conn){
    conn.sendData(&req.header, sizeof(req.header));
    ResponseHeader header = conn.recvHeader();
    
    // List file request type receives the name of the files 
    // in the data field as names of 64 bytes each. The number of 
    // names is given in filesize field.
    vector<string> names;
    for (int i = 0; i < header.filesize; i++){
        char name[64];
        uint32_t received, total = 0;
        while (total < sizeof(name)){
            received = conn.recvData(name + total, sizeof(name) - total);
            if (received == -1)
                throw runtime_error(strerror(errno));
            else if (received == 0)
                throw runtime_error("Connection terminated while reading.");
        }
        names.emplace_back(name);
    }

    for (const string& name : names)
        cout << name << endl;
}

ResponseHeader authenticateHandler(RequestInfo& req, Connection& conn){
    conn.sendData(&req.header, sizeof(req.header));
    return conn.recvHeader();
}