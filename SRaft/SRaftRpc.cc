#include "SRaftRpc.h"
#include"SRaft.h"
#include"Logger.h"
#include"Skvserver.h"
#include"Skvclient.h"
using grpc::Channel;
using grpc::ClientContext;

//SRaft client;
void SRaftClient::RequestVote(const RequestVoteRequest& request)
{

    std::shared_ptr<SRaftClient> sharedThis = shared_from_this();
    std::thread t = std::thread([sharedThis,request](){
        RequestVoteResponse response;
        ClientContext contex;
        Status status = sharedThis->stub_->RequestVote(&contex,request,&response);
        if(!status.ok())
        {
            LOG_ERROR("SRaft error at RequestVote() %d: %s",status.error_code(),status.error_message().c_str());
        }
        sharedThis->node_->OnRequestVoteResponse(response);
    });
    t.detach();
}

void SRaftClient::AppendEntries(const AppendEntriesRequest& request, bool isHeartbeat)
{

    std::shared_ptr<SRaftClient> sharedThis = shared_from_this();
    std::thread t = std::thread([sharedThis,request,isHeartbeat](){
        AppendEntriesResponse response;
        ClientContext contex;
        Status status = sharedThis->stub_->AppendEntries(&contex,request,&response);
        if(!status.ok())
        {
            LOG_ERROR("SRaft error at RequestVote() %d: %s",status.error_code(),status.error_message().c_str());
        }
        sharedThis->node_->OnAppendEntriesReponse(response, isHeartbeat);
    });
    t.detach();
}

//SRaft service
Status RaftMessagesServiceImpl::AppendEntries(ServerContext* context, const AppendEntriesRequest* request, AppendEntriesResponse* response)
{
    // LOG_DEBUG("service \t %s OnAppendEntriesRequest", node_->AddrStr().c_str());
    int result = node_->OnAppendEntriesRequest(request,response);
    return Status::OK;
}
Status RaftMessagesServiceImpl::RequestVote(ServerContext* context, const RequestVoteRequest* request, RequestVoteResponse* response)
{
    // LOG_DEBUG("service \t %s OnRequestVoteRequest", node_->AddrStr().c_str());
    int result = node_->OnRequestVoteRequest(request,response);
    return Status::OK;
}
Status RaftMessagesServiceImpl::InstallSnapshot(ServerContext* context, const InstallSnapshotRequest* request, InstallSnapshotResponse* response)
{
    return Status::OK;

}

//SRaft server
SRaftServer::SRaftServer(SRaft* node)
    :service_(std::make_unique<RaftMessagesServiceImpl>(node))
    , builder_(std::make_unique<ServerBuilder>())
    , name_(node->AddrStr())
{
    builder_->AddListeningPort(node->AddrStr(),grpc::InsecureServerCredentials());
    builder_->RegisterService(service_.get());
    server_ = std::unique_ptr<Server>(builder_->BuildAndStart());
}
SRaftServer::~SRaftServer()
{
    LOG_INFO("SRaftServer %s \t wait shutdown",name_.c_str());
    server_->Shutdown();
    LOG_INFO("SRaftServer %s wait join",name_.c_str());
    std::thread waitThread = std::thread([&]()
    {
        server_->Wait();
    });
    waitThread.join();
    LOG_INFO("SRaftServer %s \t joined",name_.c_str());
}
//kvserver client
GetResponse KvserverClient::Get(const GetRequest& request)
{
    std::shared_ptr<KvserverClient> sharedThis = shared_from_this();
    GetResponse response;
    ClientContext contex;
    Status status = sharedThis->stub_->Get(&contex,request,&response);
    if(!status.ok())
    {
        LOG_ERROR("grpc error at Get() %d: %s",status.error_code(),status.error_message().c_str());
    }
    return response;
}
void KvserverClient::PutAppend(const PutAppendRequest& request)
{
    std::shared_ptr<KvserverClient> sharedThis = shared_from_this();
    std::thread t = std::thread([sharedThis,request](){
        PutAppendResponse response;
        ClientContext contex;
        Status status = sharedThis->stub_->PutAppend(&contex,request,&response);
        if(!status.ok())
        {
            LOG_ERROR("grpc error at PutAppend() %d: %s",status.error_code(),status.error_message().c_str());
        }
        sharedThis->skvclient_->OnPutAppendResponse(response);
    });
    t.detach();
}

// kvserver service
Status KvserverServiceImpl::Get(ServerContext* context, const GetRequest* request, GetResponse* response)
{
    skvserver_->get(request,response);
    return Status::OK;
}

Status KvserverServiceImpl::PutAppend(ServerContext* context, const PutAppendRequest* request, PutAppendResponse* response)
{
    skvserver_->putAppend(request,response);
    return Status::OK;
}

// kvserver server
KvserverServer::KvserverServer(Skvserver* outServer)
    :name_(outServer->addrstr())
{
    service_ = std::make_unique<KvserverServiceImpl>(outServer);
    builder_ =std::make_unique<ServerBuilder>();
    builder_->AddListeningPort(outServer->addrstr(),grpc::InsecureServerCredentials());
    builder_->RegisterService(service_.get());
    server_ = std::unique_ptr<Server>(builder_->BuildAndStart());
}

KvserverServer::~KvserverServer()
{
    LOG_INFO("kvserver %s \t wait shutdown",name_.c_str());
    server_->Shutdown();
    LOG_INFO("kvserver %s wait join",name_.c_str());
    std::thread waitThread = std::thread([&]()
    {
        server_->Wait();
    });
    waitThread.join();
    LOG_INFO("kvserver %s \t joined",name_.c_str());

}