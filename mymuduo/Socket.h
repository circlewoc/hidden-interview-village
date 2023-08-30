#pragma onve
#include"noncopyable.h"

class InetAddress;

class Socket : Noncopyable
{
public:
    explicit Socket(int sockfd):sockfd_(sockfd){}
    ~Socket();
    int fd() const{return sockfd_;}
    void BindAddress(const InetAddress& localaddress);
    void Listen();
    int Accept(InetAddress* peeraddress);
    void ShutdownWrite();
    void SetTcpNoDelay(bool on);
    void SetReuseAddr(bool on);
    void SetReusePort(bool on);
    void SetKeepAlive(bool on);
private:
    const int sockfd_;
};