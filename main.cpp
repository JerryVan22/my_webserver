
#include "server/webserver.h"
#include<iostream>

class A{
public:
void multiply(const int a, const int b) {
//   simulate_hard_computation();
  const int res = a * b;
  std::cout << a << " * " << b << " = " << res << std::endl;
}
};


int main() {
    /* 守护进程 后台运行 */
    //daemon(1, 0); 
    // std::cout<<"hello world";
    //  ThreadPool pool(4);
    // A a;
    // for(int i=0;i<8;i++)
    // {
    //     pool.submit(std::bind(&A::multiply,&a,i,i+1));
    // }


    SSL_ctx ctx("server.pem","server.key");
    WebServer server(6855, 60000, false,3306, "root", "root", "webserver", 12, 6, true, 1, 1024,&ctx);             /* 连接池数量 线程池数量 日志开关 日志等级 日志异步队列容量 */
    server.Start();
    return 0;
} 
  