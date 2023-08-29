#include"Skvserver.h"
#include"Logger.h"
#include"SRaft.h"
#include<sstream>

Skvserver::Skvserver(std::string serverAddrStr, std::string raftAddrStr)
    :addrStr_(serverAddrStr)
    , raftAddrStr_(raftAddrStr)
{
    channel_ = std::make_shared<MyChannel>();
    node_ = std::make_unique<SRaft>(raftAddrStr,channel_);
    kvserver_ = std::make_unique<KvserverServer>(this);
    kvDB_["hello"] = "world";
    recieveMsgThread = std::thread([&]()
    {
        while(!tobeDestroy_)
        {
            std::string cmd = channel_->popData();
            std::istringstream ss(cmd);
            std::string op;
            std::string key;
            std::string value;
            if(ss>>op && ss>>key && ss>>value)
            {
                if(op=="put")
                {
                    kvDB_[key] = value;
                }
                else if(op=="append")
                {
                    kvDB_[key] += value;
                }
                else
                {
                    LOG_ERROR("Skvserver: %s \t wrong operation [%s]",addrStr_.c_str(),op.c_str());
                }
            }
            else
            {
                LOG_ERROR("Skvserver: %s \t wrong command [%s]",addrStr_.c_str(),cmd.c_str());
            }
        }
        LOG_DEBUG("recieveMsgThread break");
    });
}

Skvserver::~Skvserver()
{
    tobeDestroy_=true;
    LOG_INFO("Skvserver: %s \t shutting down",addrStr_.c_str());
    recieveMsgThread.join();
}

void Skvserver::addPeer(std::string raftAddrStr)
{
    node_->AddPeer(raftAddrStr);
}

void Skvserver::run()
{
    node_->Run();
}

void Skvserver::get(const GetRequest* request, GetResponse* response)
{
    std::unique_lock<std::mutex>lock(mut_);
    if(node_->GetState()!=LEADER)
    {
        response->set_err("wrong leader");
        response->set_value("Error");
        response->set_wrongleader(true);
        return;
    }
    if(!kvDB_.count(request->key()))
    {
        response->set_err("wrong key");
        response->set_value("Error");
        response->set_wrongleader(false);
    }
    else
    {
        response->set_err("none");
        response->set_value(kvDB_[request->key()]);
        response->set_wrongleader(false);
    }
    return;
}

void Skvserver::putAppend(const PutAppendRequest* request, PutAppendResponse* response)
{
    std::unique_lock<std::mutex>lock(mut_);
    if(node_->GetState()!=LEADER)
    {
        response->set_err("wrong leader");
        response->set_wrongleader(true);
        return;
    }    
    response->set_wrongleader(false);
    response->set_err("none");
    std::string cmd = request->op() + " " + request->key() + " "+ request->value();
    LOG_INFO("Skvserver: %s \t DoLog %s",addrStr_.c_str(),cmd.c_str());
    node_->DoLog(cmd);
    return;
}