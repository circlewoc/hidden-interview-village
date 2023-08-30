#pragma once
#include"noncopyable.h"
#include"Channel.h"
#include"Socket.h"
#include<functional>
class InetAddress;
class EventLoop;
class Acceptor
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, InetAddress&)>;
    Acceptor(EventLoop* loop, const InetAddress& addr, bool reuseport);
    ~Acceptor();
    void SetNewConnectionCB(const NewConnectionCallback &cb){newConnectionCallback_=cb;};
    bool listening(){return listening_;}
    void listen();
private:
    void handleRead();
    EventLoop* loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listening_;
};