# pragma once
#include<netinet/ip.h>
#include<arpa/inet.h>
#include<string>
class InetAddress
{
public:
    InetAddress(){}
    explicit InetAddress(uint16_t port, std::string ip="127.0.0.1");
    explicit InetAddress(sockaddr_in& addr):addr_(addr){};
    uint16_t ToPort();
    std::string ToIp();
    std::string ToIpPort() const;
    const sockaddr_in* getSockAddr() const {return &addr_;}
    void setSockAddr(const sockaddr_in addr){addr_=addr;}
private:
    sockaddr_in addr_;
};