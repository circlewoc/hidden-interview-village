#include"EventLoopThreadPool.h"
#include"EventLoopThread.h"
#include <memory>
EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseloop, const std::string &name)
    :baseloop_(baseloop)
    ,name_(name)
    , started_(false)
    , numThreads_(0)
    , next_(0)
{}
EventLoopThreadPool::~EventLoopThreadPool()
{}
void EventLoopThreadPool::Start(const ThreadInitCallback& cb)
{
    started_ = true;
    for(int i=0; i<numThreads_; i++)
    {
        char buf[name_.size()+32];
        snprintf(buf,sizeof(buf),"%s%d",name_.c_str(),i);
        EventLoopThread *t = new EventLoopThread(cb,buf);
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        loops_.push_back(t->startloop());//bug

    }
    if(numThreads_==0)
    {
        cb(baseloop_);
    }
}
EventLoop* EventLoopThreadPool::GetNextLoop()
{
    EventLoop* loop = baseloop_;
    if(!loops_.empty())
    {
        loop = loops_[next_];
        next_++;
        if(next_>loops_.size())
        {
            next_=0;
        }
    }

    return loop;
}
std::vector<EventLoop*> EventLoopThreadPool::GetAllLoops()
{
    if(loops_.empty())
    {
        return std::vector<EventLoop*>(1,baseloop_);
    }
    else
    {
        return loops_;
    }
}