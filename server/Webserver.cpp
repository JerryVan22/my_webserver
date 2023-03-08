#include "webserver.h"

WebServer::WebServer(
    int _port, int _timeoutMS, bool _OptLinger,
    int _sqlPort, const char *_sqlUser, const char *_sqlPwd,
    const char *_dbName, int _connPoolNum, int _threadNum,
    bool _openLog, int _logLevel, int _logQueSize, SSL_ctx *_ctx) : port(_port), openLinger(_OptLinger), timeoutMS(_timeoutMS), isClose(false),
                                                                    threadpool(new ThreadPool(_threadNum)), epoller(new Epoller())
{

    ctx = _ctx;

    srcDir = getcwd(nullptr, 256);

    strncat(srcDir, "/resources/", 16);

    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir;

    // SqlConnPool::Instance()->Init("localhost", _sqlPort, _sqlUser, _sqlPwd, _dbName, _connPoolNum);

    listenEvent |= EPOLLET;
    connEvent |= EPOLLET;
    if (!InitSocket_())
    {
        isClose = true;
    }

    if (_openLog)
    {
        Log::Instance()->init(_logLevel, "./record", ".log", _logQueSize);
        if (isClose)
        {
            LOG_ERROR("========== Server init error!==========");
        }
        else
        {
            LOG_INFO("========== Server init ==========");
            LOG_INFO("Port:%d, OpenLinger: %s", port, _OptLinger ? "true" : "false");
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                     (listenEvent & EPOLLET ? "ET" : "LT"),
                     (connEvent & EPOLLET ? "ET" : "LT"));
            LOG_INFO("LogSys level: %d", _logLevel);
            LOG_INFO("srcDir: %s", HttpConn::srcDir);
            LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", _connPoolNum, _threadNum);
        }
    }
}

WebServer::~WebServer()
{
    close(listenFd);
    isClose = true;
    free(srcDir);
    // SqlConnPool::Instance()->ClosePool();
}

void WebServer::Start()
{
    int timeMS = -1; /* epoll wait timeout == -1 无事件将阻塞 */

    if (!isClose)
    {
        LOG_INFO("========== Server start ==========");
    }

    while (!isClose)
    {

        int eventCnt = epoller->Wait(timeMS);

        for (int i = 0; i < eventCnt; i++)
        {
            /* 处理事件 */
            int fd = epoller->GetEventFd(i);
            uint32_t events = epoller->GetEvents(i);
            if (fd == listenFd)
            {
                DealListen_();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                // assert(users_.count(fd) > 0);
                CloseConn_(&users[fd]);
            }
            else if (events & EPOLLIN)
            {
                // assert(users_.count(fd) > 0);
                DealRead_(&users[fd]);
            }
            else if (events & EPOLLOUT)
            {
                // assert(users_.count(fd) > 0);

                DealWrite_(&users[fd]);
            }
            else
            {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void WebServer::SendError_(int fd, const char *info)
{
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if (ret < 0)
    {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::CloseConn_(HttpConn *client)
{
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    epoller->DelFd(client->GetFd());
    client->Close(ssl_hash[client->GetFd()]);
}

void WebServer::AddClient_(int fd, sockaddr_in addr)
{
    assert(fd > 0);

    ssl_hash[fd] = ctx->SSL_new();
    users[fd].init(fd, addr);

    SSL_set_fd(ssl_hash[fd], fd);
    SSL_accept(ssl_hash[fd]);
    // std::cout<<"stauts"<<status<<std::endl;

    epoller->AddFd(fd, EPOLLIN);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    LOG_INFO("Client[%d] in!", users[fd].GetFd());
    return;
}

void WebServer::DealListen_()
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    // SSL_accept()
    int fd = accept(listenFd, (struct sockaddr *)&addr, &len);

    if (HttpConn::userCount >= MAX_FD)
    {
        SendError_(fd, "Server busy!");
        LOG_WARN("Clients is full!");
    }
    return AddClient_(fd, addr);
}

void WebServer::DealRead_(HttpConn *client)
{
    assert(client);
    // ExtentTime_(client);
    OnRead_(client);
    // threadpool->AddTask(std::bind(&WebServer::OnRead_, this, client));
}

void WebServer::DealWrite_(HttpConn *client)
{
    assert(client);
    // ExtentTime_(client);
    // std::cout<<"write"<<std::endl;
    OnWrite_(client);
    //   threadpool->AddTask(std::bind(&WebServer::OnWrite_, this, client));
}

void WebServer::ExtentTime_(HttpConn *client)
{
    assert(client);
}

void WebServer::OnRead_(HttpConn *client)
{
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->ssl_read(ssl_hash[client->GetFd()], &readErrno);
    // std::cout<<"ret:"<<ret<<std::endl;
    // if(ret <= 0 ) {
    //     CloseConn_(client);
    //     return;
    // }
    // std::future<bool> flag;
    //  std::future<bool> flag = threadpool->submit(std::bind(&HttpConn::process, client));
    OnProcess(client);

}

void WebServer::OnProcess(HttpConn *client)
{
    std::future<bool> flag = threadpool->submit(std::bind(&HttpConn::process, client));
    if (flag.get())
    {

        epoller->ModFd(client->GetFd(), connEvent | EPOLLOUT);
    }
    else
    {
        epoller->ModFd(client->GetFd(), connEvent | EPOLLIN);
    }
}

void WebServer::OnWrite_(HttpConn *client)
{

    assert(client);
    int ret = 0;
    int writeErrno = 0;
    client->ssl_write(ssl_hash[client->GetFd()], &writeErrno);
    if (client->ToWriteBytes() == 0)
    {
        /* 传输完成 */
        std::cout << client->GetFd() << "传输完成" << std::endl;
        if (client->IsKeepAlive())
        {
            OnProcess(client);
            return;
        }
    }
    else if (ret < 0)
    {
        if (writeErrno == EAGAIN)
        {
            /* 继续传输 */
            epoller->ModFd(client->GetFd(), connEvent | EPOLLOUT);
            return;
        }
    }

    CloseConn_(client);
}

/* Create listenFd */
bool WebServer::InitSocket_()
{
    int ret;
    struct sockaddr_in addr;
    if (port > 65535 || port < 1024)
    {
        LOG_ERROR("Port:%d error!", port);
        return false;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    struct linger optLinger = {0};
    if (openLinger)
    {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd < 0)
    {
        LOG_ERROR("Create socket error!", port);
        return false;
    }

    ret = setsockopt(listenFd, SOL_SOCKET, SO_LINGER, &optLinger, sizeof(optLinger));
    if (ret < 0)
    {
        close(listenFd);
        LOG_ERROR("Init linger error!", port);
        return false;
    }

    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));
    if (ret == -1)
    {
        LOG_ERROR("set socket setsockopt error !");
        close(listenFd);
        return false;
    }

    ret = bind(listenFd, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0)
    {
        LOG_ERROR("Bind Port:%d error!", port);
        close(listenFd);
        return false;
    }

    ret = listen(listenFd, 6);
    if (ret < 0)
    {
        LOG_ERROR("Listen port:%d error!", port);
        close(listenFd);
        return false;
    }

    ret = epoller->AddFd(listenFd, listenEvent | EPOLLIN);

    if (ret == 0)
    {
        LOG_ERROR("Add listen error!");
        close(listenFd);
        return false;
    }

    fcntl(listenFd, F_SETFL, fcntl(listenFd, F_GETFL) | O_NONBLOCK); // 更改文件标识符
    LOG_INFO("Server port:%d", port);
    return true;
}
