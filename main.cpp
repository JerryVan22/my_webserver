
#include "server/webserver.h"
#include<string>



int main() {
    /* 守护进程 后台运行 */
    //daemon(1, 0); 
    

    WebServer server(6800, 60000, false, 12, true, 1, 1024,"server.pem","server.key"); 
    server.Start();
    return 0;
} 
  