#include "webserver.h"

WebServer::WebServer(
    int _port, int _timeoutMS, bool _OptLinger, int _threadNum,
    bool _openLog, int _logLevel, int _logQueSize, std::string _server_pem, std::string _server_key) : port(_port), openLinger(_OptLinger), timeoutMS(_timeoutMS), isClose(false),
                                                                                                       threadpool(new ThreadPool(_threadNum)), epoller(new Epoller()), ctx(_server_pem, _server_key), timer(new HeapTimer())
{

    srcDir = getcwd(nullptr, 256);

    strncat(srcDir, "/resources/", 16);

    HttpConn::userCount = 0;
    HttpConn::srcDir = srcDir;

    listenEvent = EPOLLRDHUP;
    connEvent = EPOLLONESHOT | EPOLLRDHUP;
    // listenEvent |= EPOLLET;
    // connEvent |= EPOLLET;
    HttpConn::isET = (connEvent & EPOLLET);
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
        }
    }
}

WebServer::~WebServer()
{

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
        if (!timeoutMS > 0)
        {
            timeMS = timer->GetNextTick();
        }
        int eventCnt = epoller->Wait(timeMS);

        for (int i = 0; i < eventCnt; i++)
        {
            /* 处理事件 */
            int fd = epoller->GetEventFd(i);
            uint32_t events = epoller->GetEvents(i);
            if (fd == listenFd.getFd())
            {
                DealListen_();
            }
            else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {

                CloseConn_(&users[fd]);
            }
            else if (events & EPOLLIN)
            {

                DealRead_(&users[fd]);
            }
            else if (events & EPOLLOUT)
            {

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
    int temp = client->GetFd();
    client->Close(ssl_hash[client->GetFd()]);
    ssl_hash.erase(temp);
}

void WebServer::AddClient_(int fd, sockaddr_in addr)
{
    assert(fd > 0);

    ssl_hash[fd] = ctx.SSL_new();
    users[fd].init(fd, addr);

    SSL_set_fd(ssl_hash[fd], fd);
    SSL_accept(ssl_hash[fd]);
    if (timeoutMS > 0)
    {
        timer->add(fd, timeoutMS, std::bind(&WebServer::CloseConn_, this, &users[fd]));
    }

    epoller->AddFd(fd, EPOLLIN | connEvent);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    LOG_INFO("Client[%d] in!", users[fd].GetFd());
    return;
}

void WebServer::DealListen_()
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    int fd = accept(listenFd.getFd(), (struct sockaddr *)&addr, &len);

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
    ExtentTime_(client);
    // OnRead_(client);
    threadpool->submit(std::bind(&WebServer::OnRead_, this, client));
}

void WebServer::DealWrite_(HttpConn *client)
{
    assert(client);
    ExtentTime_(client);

    // OnWrite_(client);
    threadpool->submit(std::bind(&WebServer::OnWrite_, this, client));
}

void WebServer::ExtentTime_(HttpConn *client)
{
    assert(client);
    if (timeoutMS > 0)
    {
        timer->adjust(client->GetFd(), timeoutMS);
    }
}

void WebServer::OnRead_(HttpConn *client)
{
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->ssl_read(ssl_hash[client->GetFd()], &readErrno);

    OnProcess(client);
}

void WebServer::OnProcess(HttpConn *client)
{
    // std::future<bool> flag = threadpool->submit(std::bind(&HttpConn::process, client));
    if (client->process())
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

        if (client->IsKeepAlive())
        {
            OnProcess(client);
            return;
        }
    }
    else if (ret < 0)
    {
        if (SSL_get_error(ssl_hash[client->GetFd()], writeErrno) == SSL_ERROR_WANT_WRITE)
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

    if (port > 65535 || port < 1024)
    {
        LOG_ERROR("Port:%d error!", port);

        return false;
    }

    struct linger optLinger = {0};
    if (openLinger)
    {
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
    }

    if (listenFd.getFd() < 0)
    {
        LOG_ERROR("Create socket error!", port);
        std::cout << "1";
        return false;
    }

    ret = listenFd.setsockopt(SOL_SOCKET, SO_LINGER, &optLinger, static_cast<socklen_t>(sizeof(optLinger)));
    if (ret < 0)
    {
        listenFd.close();
        LOG_ERROR("Init linger error!", port);
        std::cout << "2";
        return false;
    }

    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = listenFd.setsockopt(SOL_SOCKET, SO_REUSEADDR, (void *)&optval, static_cast<socklen_t>(sizeof(int)));
    if (ret == -1)
    {
        LOG_ERROR("set socket setsockopt error !");
        listenFd.close();

        return false;
    }
    IPAddress addr(port);
    ret = listenFd.bind(&addr);

    if (ret < 0)
    {
        LOG_ERROR("Bind Port:%d error!", port);
        listenFd.close();

        return false;
    }

    ret = listenFd.listen();
    if (ret < 0)
    {
        LOG_ERROR("Listen port:%d error!", port);
        listenFd.close();

        return false;
    }

    ret = epoller->AddFd(listenFd.getFd(), listenEvent | EPOLLIN);

    if (ret == 0)
    {
        LOG_ERROR("Add listen error!");
        listenFd.close();
        std::cout << "6";
        return false;
    }

    listenFd.setnonblocking();
    LOG_INFO("Server port:%d", port);
    return true;
}
