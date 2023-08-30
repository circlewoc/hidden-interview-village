#pragma once
#include<vector>
#include<unordered_map>
#include"noncopyable.h"
#include "Timestamp.h"
class EventLoop;
class Channel;
class Poller : Noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;
    Poller(EventLoop* loop);
    virtual ~Poller() = default;
    virtual Timestamp Poll(int timeoutMs, ChannelList* activeChannelList) = 0;
    virtual void UpdateChannel(Channel* channel) = 0;
    virtual void RemoveChannel(Channel* channel) = 0;
    bool HasChannel(Channel* channel) const;

    static Poller* NewDefaultPoller(EventLoop* loop);
protected:
    using ChannelMap = std::unordered_map<int,Channel*>;
    ChannelMap channels_;
private:
    EventLoop* ownerloop_;
};