#include"TcpServer.h"
#include"Logger.h"
#include<strings.h>
#include<functional>

static EventLoop* CheckloopNotNull(EventLoop* loop)
{
    if(loop==nullptr)
    {
        LOG_FATAL("%s:%s:%d mainloop is null\n",__FILE__,__FUNCTION__,__LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg, Option option)
    :loop_(CheckloopNotNull(loop))
    , ipPort_(listenAddr.ToIpPort())
    , name_(nameArg)
    , acceptor_(new Acceptor(loop,listenAddr, option==kNoReusePort))
    , threadPool_(new EventLoopThreadPool(loop,name_))
    , connectionCallback_()
    , messageCallback_()
    , nextConnId_(1)
    , started_(0)
{
    acceptor_->SetNewConnectionCB(std::bind(&TcpServer::newConnection,this,
        std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for(auto &item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn ->getLoop()->RunInLoop(
            std::bind(&TcpConnection::connectDestroyed,conn)
        );
        
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->SetThreadNum(numThreads);
}

// start main loop
// start sub loop
void TcpServer::start()
{
    if(started_++ == 0)
    {
        threadPool_->Start(threadInitCallback_);
        loop_->RunInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    // choose a subloop
    EventLoop *ioloop = threadPool_->GetNextLoop();
    char buf[64] = {0};
    snprintf(buf,sizeof(buf), "-%s%d",ipPort_.c_str(),nextConnId_);
    ++nextConnId_;
    std::string connName = name_+buf;
    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
    name_.c_str(), connName.c_str(), peerAddr.ToIpPort().c_str());

    // local ip and port
    sockaddr_in local;
    ::bzero(&local,sizeof local);
    socklen_t addrlen = sizeof(local);
    if(::getsockname(sockfd,(sockaddr*)&local, &addrlen)<0)
    {
        LOG_ERROR("can't get local addr");
    }
    InetAddress localAddr(local);

    // create new connection
    TcpConnectionPtr conn(new TcpConnection(
        ioloop,
        connName,
        sockfd,
        localAddr,
        peerAddr));
    connections_[connName] = conn;
    conn ->setConnectionCallback(connectionCallback_);
    conn -> setMessageCallback(messageCallback_);
    conn -> setWriteCompleteCallback(writeCompleteCallback_);
    conn -> setCloseCallback(
        std::bind(&TcpServer::removeConnection,this,std::placeholders::_1)
    );
    ioloop -> RunInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->RunInLoop(
        std::bind(&TcpServer::removeConnectionInLoop,this,conn)
    );
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n", 
        name_.c_str(), conn->name().c_str());
    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop -> QueueInLoop(
        std::bind(&TcpConnection::connectDestroyed,conn)
    );
}
