#ifndef _SOCKET_
#define _SOCKET_

#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "IPAddresss.h"
class Socket
{
private:
    int fd;
public:
    Socket();
    ~Socket();

    void bind(IPAddress*);
    void listen();
    void setnonblocking();

    int getFd();
};

#endif