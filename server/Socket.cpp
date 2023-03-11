#include "Socket.h"
#include <sys/socket.h>
Socket::Socket(){
    fd=socket(AF_INET,SOCK_STREAM,0);

}


Socket::~Socket(){
    std::cout<<"close";
    if(fd != -1){
        close();
        fd = -1;
    }
}

int Socket::bind( IPAddress * addr){
   return  ::bind(fd,reinterpret_cast<sockaddr *>(&addr->addr),addr->addr_len);

}

int Socket::listen(){
    return ::listen(fd,SOMAXCONN);
}

void Socket::setnonblocking(){
    fcntl(fd,F_SETFL,fcntl(fd,F_GETFL)| O_NONBLOCK);
}

int Socket::setsockopt(int _level,int _optname, void *  _optval,socklen_t _optlen){
    
   return  ::setsockopt(fd,_level,_optname,_optval,_optlen);
}

void Socket::close(){
   
    ::close(fd);
}


int Socket::getFd(){
    return fd;
}


