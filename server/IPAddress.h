#ifndef IP_ADDRESS_H
#define IP_ADDRESS_H



#include <arpa/inet.h>
#include <cstring>
class IPAddress
{
public:
    sockaddr_in addr;
    socklen_t addr_len;
    IPAddress()=delete;
    IPAddress(uint16_t port){
        bzero(&addr,sizeof(addr));
        addr.sin_family=AF_INET;
        addr.sin_addr.s_addr=htonl(INADDR_ANY);
        addr.sin_port=htons(port);
        addr_len=sizeof(addr);
    }
    
    IPAddress(const char* ip, uint16_t port){
        bzero(&addr,sizeof(addr));
        addr.sin_family=AF_INET;
        addr.sin_addr.s_addr=inet_addr(ip);
        addr.sin_port=htons(port);
        addr_len=sizeof(addr);
    }
    ~IPAddress()=default;
};

#endif