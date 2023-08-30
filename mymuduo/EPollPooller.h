#pragma once
#include"Poller.h"
#include<vector>

#include<sys/epoll.h>
// ? class Channel;
class EPollPoller : public Poller
{
public: 
    EPollPoller(EventLoop* loop);
    ~EPollPoller() override;
    Timestamp Poll(int timeoutMs, ChannelList* activeChannelList) override;// use epoll_wait
    void UpdateChannel(Channel* channel) override; // write epoll_ctrl for a fd: MOD and DEL
    void RemoveChannel(Channel* channel) override;// write epoll_ctrl for a fd: DEL
private:
    static const int kInitEventListSize = 16;
    void fillActiveChannels(int numEvent, ChannelList* channels);
    void Update(int operation, Channel* channel);

    using EventList = std::vector<epoll_event> ;
    EventList eventList_;
    int epollfd_;
    
};

/**
 *  Eventloop
 *  --ChannelList  Poller
 *                 --ChannelMap<fd, Channel*>
 *                 --fd
*/