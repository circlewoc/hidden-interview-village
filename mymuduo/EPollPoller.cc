#include"EPollPooller.h"
#include"Logger.h"
#include"Channel.h"
#include<errno.h>
#include<cstring>
#include<unistd.h>
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop* loop):
    Poller(loop)
    , eventList_(kInitEventListSize)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
{
    if(epollfd_<0)
    {
        LOG_FATAL("epoll create error:%d \n",errno);
    }
}

EPollPoller::~EPollPoller()
{
    close(epollfd_);
}

void EPollPoller::UpdateChannel(Channel* channel)
{
    int index = channel->index();
    LOG_INFO("func = %s, fd = %d, event = %d, index = %d\n",__FUNCTION__,channel->fd(),channel->events(), channel->index());
    if (index == kNew || index == kDeleted)
    {
        if (index == kNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(kAdded);
        Update(EPOLL_CTL_ADD, channel);
    }
    else  // channel已经在poller上注册过了
    {
        int fd = channel->fd();
        if (channel->IsNoneEvent())
        {
            Update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            Update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EPollPoller::Update(int operation, Channel* channel)
{
    epoll_event event;
    memset(&event, 0, sizeof(event));
    event.events = channel->events();
    event.data.fd = channel->fd();
    event.data.ptr = channel;
    if(::epoll_ctl(epollfd_,operation,channel->fd(),&event)<0)
    {
        if(operation==EPOLL_CTL_DEL)
        {
            LOG_ERROR("EPOLL_CTL_DEL error error number = %d\n",errno);
        }
        else
        {
            LOG_FATAL("EPOLL ADD/MOD error. error number = %d\n",errno);
        }
    }

}

void EPollPoller::RemoveChannel(Channel* channel)
{
    LOG_INFO("func = %s, fd = %d\n",__FUNCTION__,channel->fd());
    int fd = channel->fd();
    channels_.erase(fd);
    int index = channel->index();
    if(index==kAdded)
    {
        Update(EPOLL_CTL_DEL,channel);
    }
    channel->set_index(kNew);
}

Timestamp EPollPoller::Poll(int timeoutMs, ChannelList* activeChannelList)
{
    LOG_INFO("function = %s, fd total count=%lu\n",__FUNCTION__, channels_.size());
    Timestamp now = Timestamp::now();
    // the fd to be epolled is specified by epoll_ctl, so only specify the events now
    int num_events = epoll_wait(epollfd_,&*eventList_.begin(),static_cast<int>(eventList_.size()),timeoutMs);
    int save_errno = errno;
    if(num_events>0)
    {
        LOG_INFO("%d events happened\n",num_events);
        fillActiveChannels(num_events,activeChannelList);
        if(num_events = eventList_.size())
        {
            eventList_.reserve(2*num_events);
        }
    }
    else if(num_events==0)
    {
        LOG_DEBUG("%s timeout\n",__FUNCTION__);
    }
    else
    {
        if(save_errno != EINTR)
        {
            LOG_ERROR("EPollPoller::Poll() error\n");
        }
    }
    return now;
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannelList)
{
    for(int i=0; i<numEvents; i++)
    {
        Channel* channel = static_cast<Channel*>(eventList_[i].data.ptr);
        channel->set_revents(eventList_[i].events);
        activeChannelList->push_back(channel);
    }
}