#include"Socket.h"
#include"Logger.h"
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include"InetAddress.h"
#include<strings.h>
#include<netinet/tcp.h>
Socket::~Socket()
{
    close(sockfd_);
}

void Socket::BindAddress(const InetAddress& localaddress)
{
    if(0!=::bind(sockfd_,(sockaddr*)localaddress.getSockAddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL("bind sockfd%d failed\n",sockfd_);
    }
}

void Socket::Listen()
{
    if(0!=::listen(sockfd_,1024))
    {
        LOG_FATAL("listen sockfd%d failed\n",sockfd_);
    }
}

int Socket::Accept(InetAddress* peeraddress)
{
    sockaddr_in addr;
    socklen_t len;
    bzero(&addr,sizeof(addr));
    int confd = ::accept(sockfd_,(sockaddr*)&addr,&len);
    if(confd>=0)
    {
        peeraddress->setSockAddr(addr);
    }
    return confd;

}

void Socket::ShutdownWrite()
{
    if(::shutdown(sockfd_,SHUT_WR)<0)
    {
        LOG_ERROR("shutdown error\n");
    }
}

void Socket::SetTcpNoDelay(bool on)
{
    int optval = on ? 1:0;
    ::setsockopt(sockfd_,IPPROTO_TCP,TCP_NODELAY, &optval, sizeof(sockfd_));
}
void Socket::SetReuseAddr(bool on)
{
    int optval = on ? 1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEADDR, &optval, sizeof(sockfd_));
}
void Socket::SetReusePort(bool on)
{
    int optval = on ? 1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_REUSEPORT, &optval, sizeof(sockfd_));
}
void Socket::SetKeepAlive(bool on)
{
    int optval = on ? 1:0;
    ::setsockopt(sockfd_,SOL_SOCKET,SO_KEEPALIVE, &optval, sizeof(sockfd_));
}