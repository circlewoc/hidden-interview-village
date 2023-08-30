#include"Skvclient.h"
#include"Logger.h"
void Skvclient::AddServers(std::string name)
{
    Guard guard(mut_);
    SkvServerPeer* peer = new SkvServerPeer();
    peer->name = name;
    peer->client = std::make_shared<KvserverClient>(name,this);
    servers_[name] = peer;
}
std::string Skvclient::Get(std::string key)
{
    GetRequest request;
    request.set_key(key);
    request.set_clerkid(1);
    request.set_cmdindex(1);
    GetResponse response;
    std::string value;
    if(servers_.count(leaderName))
    {
        SkvServerPeer& peer = *(servers_[leaderName]);
        response = peer.client->Get(request);
        if(!response.wrongleader())
        {
            return OnGetResponse(response);
        }

    }
    for(auto& s:servers_)
    {
        SkvServerPeer& peer = *(s.second);
        response = peer.client->Get(request);
        if(!response.wrongleader())
        {
            leaderName = peer.name;
            return OnGetResponse(response);
        }
    }
    return value;
}

std::string Skvclient::OnGetResponse(const GetResponse& response)
{
    if(response.err()!="none")
    {
        LOG_INFO("client Get error: %s",response.err().c_str());
    }
    return response.value();
}

void Skvclient::Put(std::string key, std::string value)
{
    PutAppendRequest request;
    request.set_key(key);
    request.set_value(value);
    request.set_op("put");
    request.set_clerkid(1);
    request.set_cmdindex(1);
    for(auto& s:servers_)
    {
        SkvServerPeer& peer = *(s.second);
        peer.client->PutAppend(request);
    }
}
void Skvclient::Append()
{

}

void Skvclient::OnPutAppendResponse(const PutAppendResponse& response)
{

}