#pragma once
#include"noncopyable.h"
#include<vector>
#include<atomic>
#include"Timestamp.h"
#include<functional>
#include<memory>
#include<mutex>
#include"CurrentThread.h"
class Channel;
class Poller;
class EventLoop
{
public:
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();
    
    void loop();
    void quit();
    
    Timestamp PollReturnTime() const { return pollReturnTime_;};
    
    void RunInLoop(Functor cb);
    void QueueInLoop(Functor cb);
    void Wakeup();
    void UpdateChannel(Channel* channel);
    void RemoveChannel(Channel* channel);
    bool HasChannel(Channel* channel);
    bool IsInLoopThread() const { return threadId_==CurrentThread::tid();}
private:
    void HandleRead();
    void DoPendingFunctors();
    using ChannelList = std::vector<Channel*>;
    std::atomic_bool looping_;
    std::atomic_bool quit_;
    const pid_t threadId_;
    Timestamp pollReturnTime_;
    std::unique_ptr<Poller> poller_;
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;
    ChannelList activeChannels_;
    ChannelList* currentWakeupChannel_;
    std::atomic_bool callingPendingFunctors_;
    std::vector<Functor> pendingFunctors_;
    std::mutex mutex_;
};