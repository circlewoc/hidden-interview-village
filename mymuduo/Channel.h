#pragma once
#include<functional>
#include"Timestamp.h"
#include<memory>
#include"EventLoop.h"

class Timestamp;

// this class encapsulate: 1. a file descriptor 2. its intersting event 3. four callbacks
class Channel
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp timestamp)>;
    Channel(EventLoop* loop,int fd);
    ~Channel();

    void HandleEvent(Timestamp receiveTime);
    void SetReadEventCallback(ReadEventCallback cb)
    {readEventCallback_ = std::move(cb);}
    void SetWriteEventCallback(EventCallback cb)
    {writeEventCallback_ = std::move(cb);}
    void SetCloseEventCallback(EventCallback cb)
    {closeEventCallback_ = std::move(cb);}
    void SetErrorEventCallback(EventCallback cb)
    {errorEventCallback_ = std::move(cb);}

    void tie(const std::shared_ptr<void>&);

    int fd() const {return fd_;}
    int events() const {return event_;}
    void set_revents(int e) { revent_=e;}
    bool IsNoneEvent() const {return event_==kNoneEvent;}
    bool IsReading() const {return event_& kReadEvent;}
    bool IsWriting() const {return event_& kWriteEvent;}
    int index() const {return index_;}
    void set_index(int index) {index_ = index;}

    void EnableRead() {event_ |= kReadEvent;update();}
    void DisableRead() {event_&=~kReadEvent;update();}
    void EnableWrite() {event_|=kWriteEvent;update();}
    void DisableWrite() {event_&=~kWriteEvent;update();}
    void DisableAll() {event_=kNoneEvent;update();}
    EventLoop* ownerloop() {return loop_;}
    void remove();
private:
    void update();
    void HandleEventWithGuard(Timestamp receiveTime);
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop* loop_;
    const int fd_;
    int event_;
    int revent_;
    int index_;
    
    std::weak_ptr<void> tie_;
    bool tied_;
    ReadEventCallback readEventCallback_;
    EventCallback writeEventCallback_;
    EventCallback closeEventCallback_;
    EventCallback errorEventCallback_;

    
};