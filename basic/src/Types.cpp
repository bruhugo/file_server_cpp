#include "Types.hpp"
#include <string>

std::string statusMessage(ResponseStatus status){
    switch (status)
    {
    case ResponseStatus::SUCCESS:
        return "Success";
    case ResponseStatus::UNAUTHORIZED:
        return "Password is incorrect or user does not exist.";
    case ResponseStatus::FILE_NOT_FOUND:
        return "File not found.";
    case ResponseStatus::SERVER_SIDE_ERROR:
        return "An error happened at the server side, please try again later.";
    case ResponseStatus::BAD_REQUEST:
        return "Bad request provided";
    default:
        return "Uknown status code.";
    }
}

const char* SSFTServerError::what(){
    return msg;
}