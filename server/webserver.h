
#include<openssl/ssl.h>
#include<openssl/err.h>



#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "Epoll.h"
#include "IPAddress.h"
#include "Socket.h"

#include "../log/log.h"

#include "../pool/threadpool.h"
#include "../http/httpconn.h"
#include "../ssl/SSL_ctx.h"
#include "../timer/heaptimer.h"
class WebServer {
public:
    WebServer(
        int _port,  int _timeoutMS, bool _OptLinger, int _threadNum,
        bool _openLog, int _logLevel, int _logQueSize,std::string _server_pem, std::string _server_key);

    ~WebServer();
    void Start();

private:
    bool InitSocket_(); 
    void AddClient_(int fd, sockaddr_in addr);
  
    void DealListen_();
    void DealWrite_(HttpConn* client);
    void DealRead_(HttpConn* client);

    void SendError_(int fd, const char*info);
    void ExtentTime_(HttpConn* client);
    void CloseConn_(HttpConn* client);

    void OnRead_(HttpConn* client);
    void OnWrite_(HttpConn* client);
    void OnProcess(HttpConn* client);

    static const int MAX_FD = 65536;


    int port;
   
    bool openLinger;
    int timeoutMS;  /* 毫秒MS */
    bool isClose;
    Socket listenFd;
    char* srcDir;
    
    uint32_t listenEvent;
    uint32_t connEvent;
   
   
   std::unique_ptr<ThreadPool> threadpool;
    std::unique_ptr<Epoller> epoller;
    std::unordered_map<int, HttpConn> users;
    std::unordered_map<int,SSL *> ssl_hash;
     std::unique_ptr<HeapTimer> timer;
    SSL_ctx ctx;
};
