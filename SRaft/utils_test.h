# pragma once
#include<vector>
#include<string>
#include<stdexcept>
#include<memory>
#include"SRaft.h"
#include"Logger.h"
#include"Skvserver.h"
#include"Skvclient.h"

std::vector<std::unique_ptr<SRaft>> nodes;
std::vector<std::unique_ptr<Skvserver>> servers;
Skvclient client;
uint64_t basePort = 9000;
uint64_t baseRaftPort = 8000;


void MakeNodes(int n)
{
    nodes.resize(n);
    for(int i=0;i<n;i++)
    {
        std::string addr = std::string("127.0.0.1:")+std::to_string(baseRaftPort+i);
        nodes[i].reset(new SRaft(addr));
    }
    for(int i=0; i<n; i++)
    {
        for(int j=0; j<n; j++)
        {
            if(nodes[i]->AddrStr()!=nodes[j]->AddrStr())
            {
                nodes[i]->AddPeer(nodes[j]->AddrStr());
            }
        }
    }
    for(auto& nd:nodes)
    {
        nd->Run();
    }
}

void MakeServers(int n)
{
    servers.resize(n);
    for(int i=0;i<n;i++)
    {
        std::string serverAddr = std::string("127.0.0.1:")+std::to_string(basePort+i);
        std::string raftAddr = std::string("127.0.0.1:")+std::to_string(baseRaftPort+i);
        servers[i] = std::make_unique<Skvserver>(serverAddr,raftAddr);
    }
    for(int i=0; i<n; i++)
    {
        for(int j=0; j<n; j++)
        {
            if(servers[i]->addrstr()!=servers[j]->addrstr())
            {
                servers[i]->addPeer(servers[j]->raftAddrstr());
            }
        }
    }

}

void RunServerRaft()
{
    for(auto& kv:servers)
    {
        kv->run();
    }
}

void InitClient()
{
    for(auto& s:servers)
    {
        client.AddServers(s->addrstr());
    }
}

SRaft* PickLeader()
{
    for(auto& nd:nodes)
    {
        if(nd->GetState()==LEADER)
        {
            return nd.get();
        }
    }
    LOG_ERROR("no leader yet");
    return nullptr;
}

