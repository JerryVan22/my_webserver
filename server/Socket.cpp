#include "Socket.h"

Socket::Socket(){
    fd=socket(AF_INET,SOCK_STREAM,0);
}


Socket::~Socket(){
    if(fd != -1){
        close(fd);
        fd = -1;
    }
}

void Socket::bind(IPAddress * addr){
    ::bind(fd,reinterpret_cast<sockaddr *>(&addr->addr),addr->addr_len);

}

void Socket::listen(){
    ::listen(fd,SOMAXCONN);
}

void Socket::setnonblocking(){
    fcntl(fd,F_SETFL,fcntl(fd,F_GETFL)| O_NONBLOCK);
}



int Socket::getFd(){
    return fd;
}


