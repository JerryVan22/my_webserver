#include "Epoll.h"
#include<iostream>
Epoller::Epoller(int maxEvent){
    epollFd=epoll_create(maxEvent);
    events=move(std::vector<epoll_event>(maxEvent));
}

Epoller::~Epoller(){
    close(epollFd);
}

bool Epoller::AddFd(int fd,uint32_t events){
    
    epoll_event event={0};
    event.events=events;
    event.data.fd=fd;
    return 0==epoll_ctl(epollFd,EPOLL_CTL_ADD,fd,&event);
}

bool Epoller::DelFd(int fd){
     epoll_event event={0};
    return 0==epoll_ctl(epollFd,EPOLL_CTL_DEL,fd,&event);
}

bool Epoller::ModFd(int fd,uint32_t events){
    epoll_event event={0};
    event.events=events;
    event.data.fd=fd;
    return 0==epoll_ctl(epollFd,EPOLL_CTL_MOD,fd,&event);
    
}

int Epoller::Wait(int timeoutMs){
    return epoll_wait(epollFd,events.data(),static_cast<int>(events.size()),timeoutMs);
}

int Epoller::GetEventFd(size_t i)const{
    return events[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i)const{
    return events[i].events;
}