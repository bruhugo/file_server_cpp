#include "Connection.hpp"
#include "Types.hpp"

#include <netdb.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <stdexcept>
#include <assert.h>
#include <errno.h>

Connection::Connection(string destination): 
    destination_{destination}, state_{CONN_STATE::CLOSED}
    {
        addrinfo hints, *r, *p;
        sockaddr_storage addr;
        int err, clientSocket;

        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_family = AF_UNSPEC;
        hints.ai_flags = AI_PASSIVE;
        hints.ai_socktype = SOCK_STREAM;

        if ((err = getaddrinfo(destination_.c_str(), PORT_STR, &hints, &r)) != 0)
            throw runtime_error(gai_strerror(err));

        for(p = r; p != nullptr; p = p->ai_next){
            if ((clientSocket = socket(
                p->ai_family, 
                p->ai_socktype, 
                p->ai_protocol)
            ) == -1)
                throw runtime_error(strerror(errno));


            if (connect(clientSocket, p->ai_addr, p->ai_addrlen) != 0){
                close(clientSocket);
                throw runtime_error(strerror(errno));
            }

            socket_ = clientSocket;
            state_ = CONN_STATE::OPEN;
            freeaddrinfo(r);
            return;
        }

        freeaddrinfo(r);
        throw runtime_error("Could not find IP address for the given host.");
    }

Connection::~Connection(){
    closeConnection();
}

void Connection::closeConnection(){
    state_ =  CONN_STATE::CLOSED;
    close(socket_);
}

void Connection::sendData(void* buff, uint32_t buffsize){
    assert(state_ == CONN_STATE::OPEN);
    uint32_t total = 0, sent = 0;
    while (total < buffsize){
        if ((sent = send(socket_, buff, buffsize, 0)) == -1){
            throw runtime_error(strerror(errno));
        }else if (sent == 0){
            close(socket_);
            throw runtime_error("Socket closed while transfering.");
        }

        total += sent;
    }
}


uint32_t Connection::recvData(void* buff, uint32_t buffsize){
    assert(state_ == CONN_STATE::OPEN);

    uint32_t received = recv(socket_, buff, buffsize, 0);
    if (received == -1)
        throw runtime_error(strerror(errno));

    return received;
}
