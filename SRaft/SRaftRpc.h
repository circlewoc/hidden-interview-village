#pragma once
#include <grpcpp/grpcpp.h>
#include<memory>
#include<string>
#include"raft_messages.grpc.pb.h"
#include"raft_messages.pb.h"
using namespace raft_messages;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::Channel;

class SRaft;
class Skvserver;
class Skvclient;

// SRaft client class
class SRaftClient : public std::enable_shared_from_this<SRaftClient>
{
public:
    SRaftClient(std::string addr_str, SRaft* node)
        :addrStr_(addr_str)
    {
        node_ = node;
        std::shared_ptr<Channel> channel = grpc::CreateChannel(addr_str, grpc::InsecureChannelCredentials());
        stub_ = RaftMessages::NewStub(channel);
        
    }
    ~SRaftClient()
    {
        node_=nullptr;
    }
    void RequestVote(const RequestVoteRequest& request);
    void AppendEntries(const AppendEntriesRequest& request, bool isHeartbeat);
    void InstallSnapshot(const InstallSnapshotRequest& request);
    std::string addrStr_;
private:    
    std::unique_ptr<RaftMessages::Stub> stub_;
    SRaft* node_;
};

// SRaft service class
class RaftMessagesServiceImpl: public RaftMessages::Service
{
public:
    RaftMessagesServiceImpl(SRaft* node):node_(node){}
    Status RequestVote(ServerContext* context, const RequestVoteRequest* request, RequestVoteResponse* response);
    Status AppendEntries(ServerContext* context, const AppendEntriesRequest* request, AppendEntriesResponse* response);
    Status InstallSnapshot(ServerContext* context, const InstallSnapshotRequest* request, InstallSnapshotResponse* response);
private:
    SRaft* node_ = nullptr;

};

// SRaft server class
class SRaftServer
{
public:
    SRaftServer(SRaft* node);
    ~SRaftServer();

    std::unique_ptr<RaftMessagesServiceImpl> service_;
    std::unique_ptr<ServerBuilder> builder_;
    std::shared_ptr<Server> server_;
private:
    std::string name_;    
};

//kvserver client
class KvserverClient : public std::enable_shared_from_this<KvserverClient>
{
public:    
    KvserverClient(std::string addr_str, Skvclient* node)
        :addrStr_(addr_str)
        , skvclient_(node)
    {
        std::shared_ptr<Channel> channel = grpc::CreateChannel(addr_str, grpc::InsecureChannelCredentials());
        stub_ = kvserver::NewStub(channel);
    }
    GetResponse Get(const GetRequest& request);
    void PutAppend(const PutAppendRequest& request);
    std::string addrStr_;

private:    
    std::unique_ptr<kvserver::Stub> stub_;    
    Skvclient* skvclient_;
};

// kvserver service class
class KvserverServiceImpl: public kvserver::Service
{
public:
    KvserverServiceImpl(Skvserver* skvserver):skvserver_(skvserver){}
    Status Get(ServerContext* context, const GetRequest* request, GetResponse* response);
    Status PutAppend(ServerContext* context, const PutAppendRequest* request, PutAppendResponse* response);
private:
    Skvserver* skvserver_ = nullptr;    
};

// kvserver server class
class KvserverServer
{
public:
    KvserverServer(Skvserver*);
    ~KvserverServer();
    std::unique_ptr<KvserverServiceImpl> service_;
    std::unique_ptr<ServerBuilder> builder_;
    std::shared_ptr<Server> server_;
private:
    std::string name_;
};