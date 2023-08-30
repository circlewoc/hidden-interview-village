#pragma once
#include<functional>
#include<mutex>
#include<condition_variable>
#include<string>
#include "noncopyable.h"
#include"Thread.h"

class EventLoop;
class EventLoopThread : Noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
        const std::string name = std::string());
    ~EventLoopThread();
    EventLoop* startloop();
private:
    void ThreadFunc();
    EventLoop* loop_;
    bool existing_;
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;
};