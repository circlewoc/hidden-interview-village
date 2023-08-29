#pragma once
#include<memory>
#include"SRaftRpc.h"

class SkvServerPeer
{
public:
    std::string name;
    std::shared_ptr<KvserverClient> client;
};

class Skvclient
{
public:
    using Guard = std::lock_guard<std::mutex>;
    void AddServers(std::string);
    std::string Get(std::string key);
    void Put(std::string key, std::string value);
    void Append();
    std::string OnGetResponse(const GetResponse& response);
    void OnPutAppendResponse(const PutAppendResponse& response);

private:
    std::mutex mut_;
    std::unordered_map<std::string, SkvServerPeer*> servers_;
    std::string leaderName;
};