#pragma once
#include<queue>
#include<string>
#include<memory>
#include<condition_variable>
#include<unordered_map>
#include<string>
#include<thread>
#include"SRaftRpc.h"
#include"MyChannel.h"

class SRaft;

class Skvserver
{
public:
    Skvserver(std::string serverAddrStr, std::string raftAddrStr);
    ~Skvserver();  
    std::string addrstr() const {return addrStr_;}
    std::string raftAddrstr() const {return raftAddrStr_;}

    void addPeer(std::string raftAddrStr);
    void run();

    void get(const GetRequest* request, GetResponse* response);
    void putAppend(const PutAppendRequest* request, PutAppendResponse* response);
private:    
    std::unique_ptr<SRaft> node_;
    std::shared_ptr<MyChannel> channel_;
    std::unique_ptr<KvserverServer> kvserver_;
    std::string addrStr_;
    std::string raftAddrStr_;
    std::thread recieveMsgThread;
    std::unordered_map<std::string,std::string> kvDB_;
    std::mutex mut_;
    bool tobeDestroy_=false;
};

