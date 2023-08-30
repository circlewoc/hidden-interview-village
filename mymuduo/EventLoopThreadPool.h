#pragma once
#include"noncopyable.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : Noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    EventLoopThreadPool(EventLoop* baseloop, const std::string &name);
    ~EventLoopThreadPool();
    void SetThreadNum(int numThreads){numThreads_=numThreads;}
    void Start(const ThreadInitCallback &cb = ThreadInitCallback());
    EventLoop* GetNextLoop();
    std::vector<EventLoop*> GetAllLoops();
    bool Started() const {return started_;}
    const std::string Name() const { return name_;}
private:
    EventLoop* baseloop_;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};


