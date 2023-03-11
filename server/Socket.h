#ifndef _SOCKET_
#define _SOCKET_

#include "IPAddress.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "../log/log.h"

class Socket
{
private:
    int fd;
public:
    Socket();
    ~Socket();

    int bind(IPAddress *addr);
    int listen();
    void setnonblocking();
    void close();
    int setsockopt(int _level,int _optname, void *  _optval,socklen_t _optlen);
    int getFd();
};

#endif