#include "EventLoop.h"
#include"Logger.h"
#include "Poller.h"
#include "Channel.h"
#include<errno.h>
#include<sys/eventfd.h>
#include<memory>

__thread EventLoop* t_loopInThisThread = nullptr;
const int kPollTimeMs = 10000;
int CreateEventFd()
{
    int evtfd = eventfd(0,EFD_CLOEXEC|EFD_NONBLOCK);
    if(evtfd<0)
    {
        LOG_FATAL("event fd error:%d\n",errno);
    }
    return evtfd;
}

EventLoop::EventLoop():
    threadId_(CurrentThread::tid())
    , looping_(false)
    , quit_(false)
    , poller_(Poller::NewDefaultPoller(this))
    , wakeupFd_(CreateEventFd())
    , wakeupChannel_(new Channel(this,wakeupFd_))
    , callingPendingFunctors_(false)
    , currentWakeupChannel_(nullptr)
    
{
    LOG_DEBUG("Eventloop created %p threadid= %d\n",this,threadId_);
    wakeupChannel_->SetReadEventCallback(std::bind(&EventLoop::HandleRead,this));
    wakeupChannel_->EnableRead();
}
EventLoop::~EventLoop()
{
    wakeupChannel_->DisableAll();
    wakeupChannel_->remove();
    close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;
    LOG_INFO("Eventloop %p start looping\n",this);
    while(!quit_)
    {
        activeChannels_.clear();
        Timestamp t = poller_->Poll(kPollTimeMs,&activeChannels_);
        for(Channel* channel:activeChannels_)
        {
            channel->HandleEvent(t);
        }
        DoPendingFunctors();
    }
    LOG_INFO("Eventloop %p stop looping\n",this);
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    if(!IsInLoopThread())
    {
        Wakeup();
    }
}

void EventLoop::RunInLoop(Functor cb)
{
    if(IsInLoopThread())
    {
        cb();
    }
    else
    {
        QueueInLoop(cb);
    }
}

void EventLoop::QueueInLoop(Functor cb)
{

    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    if(!IsInLoopThread() || callingPendingFunctors_)
    {
        Wakeup();
    }
}

void EventLoop::HandleRead()
{
    uint64_t one;
    ssize_t n = read(wakeupFd_,&one, sizeof(one));
    if(n!=sizeof(one))
    {
        LOG_ERROR("EventLoop::HandleRead() reads ? bytes instead of 8\n");
    }
}

void EventLoop::Wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_,&one, sizeof(one));
    if(n!=sizeof(one))
    {
        LOG_ERROR("EventLoop::Wakeup() write %lu byte instead of 8\n",n);
    }
}

void EventLoop::UpdateChannel(Channel* channel)
{
    poller_->UpdateChannel(channel);
}

void EventLoop::RemoveChannel(Channel* channel)
{
    poller_->RemoveChannel(channel);
}

bool EventLoop::HasChannel(Channel* channel)
{
    return poller_->HasChannel(channel);
}

void EventLoop::DoPendingFunctors()
{
    std::vector<Functor> vec;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        vec.swap(pendingFunctors_);
    }
    callingPendingFunctors_ = true;
    for(const Functor& cb:vec)
    {
        cb();
    }
    callingPendingFunctors_ = false;


}

