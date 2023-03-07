
#include "server/webserver.h"
#include<iostream>
int main() {
    /* 守护进程 后台运行 */
    //daemon(1, 0); 
    // std::cout<<"hello world";
    SSL_ctx ctx("server.pem","server.key");
    WebServer server(5505, 60000, false,3306, "root", "root", "webserver", 12, 6, true, 1, 1024,&ctx);             /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */
    server.Start();
    return 0;
} 
  