#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include <sys/epoll.h>
const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), event_(0), revent_(0), index_(-1), tied_(false) {}

Channel::~Channel(){}

void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_=true;
}

void Channel::update()
{
    // addcode
    loop_->UpdateChannel(this);
}

void Channel::remove()
{
    // addcode
    loop_ -> RemoveChannel(this);
}

void Channel::HandleEvent(Timestamp timestamp)
{
    std::shared_ptr<void> guard;
    if(tied_)
    {
        guard = tie_.lock();
        if(guard)
        {
            HandleEventWithGuard(timestamp);
        }
    }
    else
    {
        HandleEventWithGuard(timestamp);
    }
}

void Channel::HandleEventWithGuard(Timestamp receivetime)
{
    LOG_INFO("revent_ = %d\n",revent_);
    if((revent_ & EPOLLHUP) && !(revent_ & EPOLLIN))
    {
        if(closeEventCallback_)
        {
            closeEventCallback_();
        }
    }
    if(revent_ & EPOLLERR)
    {
        if(errorEventCallback_)
        {
            errorEventCallback_();
        }
    }
    if(revent_ & (EPOLLIN | EPOLLPRI))
    {
        if(readEventCallback_)
        {
            readEventCallback_(receivetime);
        }
    }
    if(revent_ & EPOLLOUT)
    {
        if(writeEventCallback_)
        {
            writeEventCallback_();
        }
    }
}