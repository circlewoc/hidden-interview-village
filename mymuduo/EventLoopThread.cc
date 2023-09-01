#include "EventLoopThread.h"
#include"EventLoop.h"
EventLoopThread::EventLoopThread(const ThreadInitCallback & cb, std::string name)
    : loop_(nullptr)
    , existing_(false)
    , thread_(std::bind(&EventLoopThread::ThreadFunc,this),name)
    , mutex_()
    , cond_()
    , callback_(cb)
{

}

EventLoopThread::~EventLoopThread()
{
    existing_ = true;
    if(loop_!=nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startloop()
{
    thread_.start();//bug
    EventLoop* loop;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while(loop_==nullptr)
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

void EventLoopThread::ThreadFunc()
{
    EventLoop loop;
    if(callback_)
    {
        callback_(&loop);
    }
    {
        std::unique_lock<std::mutex> lock;
        loop_ = &loop;
        cond_.notify_one();
    }
    loop.loop();
    std::unique_lock<std::mutex> lock;
    loop_ = nullptr;

}