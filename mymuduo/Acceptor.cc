#include"Acceptor.h"
#include"Logger.h"
#include"InetAddress.h"
#include<sys/types.h>
#include<sys/socket.h>
#include<errno.h>
#include<unistd.h>
static int createNonBlock()
{
    int sockfd = socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);
    if(sockfd<0)
    {
        LOG_FATAL("create socket failed erro no=%d",errno);
    }
    return sockfd;
}
Acceptor::Acceptor(EventLoop* loop, const InetAddress& addr, bool reuseport)
    :loop_(loop)
    , acceptSocket_(createNonBlock())
    , acceptChannel_(loop,acceptSocket_.fd())
    , listening_(false)
{
    acceptSocket_.SetReuseAddr(true);
    acceptSocket_.SetReusePort(true);
    acceptSocket_.BindAddress(addr);
    acceptChannel_.SetReadEventCallback(std::bind(&Acceptor::handleRead,this));
}

Acceptor::~Acceptor()
{
    acceptChannel_.DisableAll();
    acceptChannel_.remove();
}

void Acceptor::listen()
{   
    listening_=true;
    acceptSocket_.Listen();
    acceptChannel_.EnableRead();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            

}

void Acceptor::handleRead()
{
    InetAddress peer;
    int connfd = acceptSocket_.Accept(&peer);
    if(connfd>0)
    {
        if(newConnectionCallback_)
        {
            newConnectionCallback_(connfd,peer);
        }
        else
        {
            close(connfd);
        }
    }
    else
    {
        LOG_ERROR("accept socket failed erro no=%d",errno);
        if(errno==EMFILE)
        {
            LOG_ERROR("connection full");
        }

    }
}