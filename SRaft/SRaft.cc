#include<thread>
#include<iostream>
#include<chrono>
#include<algorithm>
#include<fstream>
#include "SRaft.h"
#include"utils.h"
#include"Logger.h"
#include"MyChannel.h"

SRaft::SRaft(std::string addrStr, std::shared_ptr<MyChannel> channel)
    :addrStr_(addrStr)
    , channel_(channel)
{
    SRaftServer_ = std::make_unique<SRaftServer>(this);
    LOG_DEBUG("Term=%lu NewNode: \t %s is created",currentTerm_, addrStr_.c_str());
    timer_thread = std::thread([&](){
        while(1)
        {
            if(toDestroy_)
            {
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            OnTimer();
        }
    });
}

SRaft::~SRaft()
{
    toDestroy_=true;
    timer_thread.join();
    LOG_DEBUG("SRaft %s \t shutting down",addrStr_.c_str());
}

void SRaft::Run()
{
    LOG_DEBUG("Term=%lu Node: \t %s is running",currentTerm_, addrStr_.c_str());
    Guard guard(mut_);
    Load(guard);
    running_ = true;
    dueElection_ = get_current_ms()+get_ranged_random(interval_election_low,interval_election_high);
}
void SRaft::Dump(Guard& guard)
{
    std::fstream fo((addrStr_+std::string(".persist")).c_str(),std::ios::binary|std::ios::out);
    PersistRecord record;
    record.set_name(addrStr_);
    record.set_vote_for(votedFor_);
    record.set_term(currentTerm_);
    record.set_last_seq(0);
    for(int i=0; i<logs.size();i++)
    {
        LogEntry& entry = *(record.add_entries());
        entry = logs[i];
    }
    record.SerializeToOstream(&fo);
    fo.close();
    LOG_DEBUG("Term=%lu %s: \t %s persist",currentTerm_, stateStr().c_str(),addrStr_.c_str());

}
void SRaft::Load(Guard& guard)
{
    std::fstream fo((addrStr_+std::string(".persist")).c_str(),std::ios::binary|std::ios::in);
    if(!fo.is_open())
    {
        return;
        LOG_DEBUG("Term=%lu %s: \t %s persist file doesn't exist",currentTerm_, stateStr().c_str(),addrStr_.c_str());

    }
    PersistRecord record;
    record.ParseFromIstream(&fo);
    currentTerm_ = record.term();
    votedFor_ = record.vote_for();
    logs.clear();
    for(int i=0; i<record.entries_size();i++)
    {
        logs.push_back(record.entries(i));
    }
    fo.close();
    LOG_DEBUG("Term=%lu %s: \t %s load, currentTerm=%lu, votedFor=%s, logs.size=%lu"
    ,currentTerm_, stateStr().c_str(),addrStr_.c_str(),currentTerm_,votedFor_.c_str(), logs.size());

}
void SRaft::AddPeer(std::string addrStr)
{
    Guard guard(mut_);
    SRaftPeer* peer = new SRaftPeer();
    peer->name = addrStr;
    peer->peerSRaftClient = std::make_shared<SRaftClient>(addrStr,this);
    peers_[addrStr].reset(peer);
}

void SRaft::OnTimer()
{
    Guard guard(mut_);
    if(!running_) return;
    uint64_t currentMs = get_current_ms();
    if(state_==LEADER && currentMs>dueHeartBeat_ && !peers_.empty())
    {
        dueHeartBeat_ = currentMs+interval_heartBeat;
        SendHeartbeat(guard);
    }
    if(state_==FOLLOWER && currentMs>dueElection_)
    {
        DoElection(guard);
    }
    if(state_==CANDIDATE && currentMs>dueElectionFail_)
    {
        LOG_DEBUG("Term=%lu CANDIDATE:\t%s election fail due to timeout",currentTerm_, addrStr_.c_str());
        state_ = FOLLOWER;

        dueElection_ = get_current_ms()+get_ranged_random(interval_election_low,interval_election_high);
    }
}

void SRaft::SendHeartbeat(Guard& guard)
{
    LOG_DEBUG("Term=%lu LEADER: \t %s SendHeartbeat to %ld followers",currentTerm_, addrStr_.c_str(), peers_.size());
    DoAppendEntries(guard,true);
}

void SRaft::DoElection(Guard& guard)
{
    LOG_DEBUG("Term=%lu FOLLOWER: \t %s DoElection",currentTerm_, addrStr_.c_str());
    state_ = CANDIDATE;
    currentTerm_++;
    votesRecieved_=1;
    votedFor_ = addrStr_;// vote for itself
    Dump(guard);
    uint64_t currentMs = get_current_ms();
    dueElectionFail_ = currentMs+interval_election_fail;
    if(peers_.empty())
    {
        state_ = LEADER;
        LOG_DEBUG("Term=%lu %s: \t %s has no peers, becoms leader directly",currentTerm_, stateStr().c_str(), addrStr_.c_str());
    }
    for(auto & pp:peers_)
    {
        SRaftPeer& peer = *pp.second;
        RequestVoteRequest request;
        request.set_initial(true);
        uint64_t lastLogTerm = logs.size()>0?logs.back().term():-1;
        request.set_last_log_term(lastLogTerm);
        request.set_last_log_index(logs.size()-1);
        request.set_name(addrStr_);
        request.set_seq(-1);
        request.set_term(currentTerm_);
        request.set_time(currentMs);
        peer.peerSRaftClient->RequestVote(request);
    }
}

int SRaft::DoLog(std::string cmd)
{
    Guard guard(mut_);
    if(!running_)
    {
        LOG_INFO("Term=%lu Node:\t %s is not running",currentTerm_, addrStr_.c_str());
        return -1;
    }
    if(state_!=LEADER)
    {
        LOG_INFO("Term=%lu Node:\t %s is not the LEADER",currentTerm_, addrStr_.c_str());
        return -2;
    }
    LogEntry entry;
    entry.set_data(cmd);
    entry.set_command(0);
    entry.set_term(currentTerm_);
    entry.set_index(LastLogIndex()+1);
    logs.push_back(entry);
    LOG_DEBUG("Term=%lu LEADER: \t %s added a new log",currentTerm_, addrStr_.c_str());
    Dump(guard);
    DoAppendEntries(guard,false);
    return 1;
}

void SRaft::DoApply(Guard& guard,bool fromSnapshot)
{
    while(lastApplied_<commitIndex_)
    {
        lastApplied_++;
        std::string cmd = GetLogByIndex(lastApplied_).data();
        if(!cmd.empty())
        {
            if(channel_)
            {
                channel_->pushData(cmd);
                LOG_DEBUG("Term=%lu %s: \t %s DoApply log: %s",currentTerm_, stateStr().c_str(), addrStr_.c_str(),cmd.c_str());
            }
            else
            {
                LOG_ERROR("Term=%lu %s: \t %s channel is null",currentTerm_, stateStr().c_str(), addrStr_.c_str());
            }

        }
    }
}

void SRaft::DoAppendEntries(Guard& guard, bool isHeartbeat)
{
    if(peers_.empty()&&!isHeartbeat)
    {
        LOG_DEBUG("Term=%lu %s: \t %s has no peers, apply log directly",currentTerm_, stateStr().c_str(), addrStr_.c_str());
        commitIndex_ = LastLogIndex();
        DoApply(guard,false);
    }
    for(auto & pp:peers_)
    {
        SRaftPeer & peer = *pp.second;
        AppendEntriesRequest request;
        request.set_name(addrStr_);
        request.set_term(currentTerm_);
        request.set_leader_commit(commitIndex_);
        request.set_time(get_current_ms());
        request.set_prev_log_index(-1);
        request.set_prev_log_term(-1); 
        request.set_seq(0);
        request.set_initial(true);
        if(!isHeartbeat)
        {
            int64_t prevLogIndex = peer.nextIndex_-1;
            int64_t prevLogTerm = prevLogIndex>-1?GetLogByIndex(prevLogIndex).term():0;
            request.set_prev_log_index(prevLogIndex);
            request.set_prev_log_term(prevLogTerm);
            for(uint64_t i = std::max((int64_t)0,peer.nextIndex_); i<=LastLogIndex();i++)
            {
                LogEntry& entry = *(request.add_entries());
                entry = GetLogByIndex(i);
            }
        }
        peer.peerSRaftClient->AppendEntries(request,isHeartbeat);
    }
}

int SRaft::OnAppendEntriesRequest(const AppendEntriesRequest* request, AppendEntriesResponse* response)
{
    // LOG_DEBUG("Term=%lu %s: \t %s doing AppendEntriesRequest from %s",currentTerm_, stateStr().c_str(),addrStr_.c_str(), request->name().c_str());
    Guard guard(mut_);
    response->set_name(addrStr_);
    response->set_success(false);
    response->set_seq(request->seq());
    response->set_term(currentTerm_);
    response->set_last_log_index(LastLogIndex());
    response->set_last_log_term(LastLogTerm());
    response->set_time(get_current_ms());
    if(!running_)
    {
        LOG_DEBUG("Term=%lu %s: \t %s AppendEntriesRequest fail (not running)",currentTerm_, stateStr().c_str(), addrStr_.c_str());
        return -1;
    }
    if(request->term()<currentTerm_)
    {
        LOG_DEBUG("Term=%lu %s: \t %s AppendEntriesRequest fail (leader small term)",currentTerm_, stateStr().c_str(), addrStr_.c_str());
        return -1;
    }
    // heartbeat
    if(request->entries().size()==0)
    {
        if(state_==LEADER)
        {
            LOG_DEBUG("Term=%lu %s: \t %s will be replaced",currentTerm_, stateStr().c_str(),addrStr_.c_str());
        }
        dueElection_ = get_current_ms()+get_ranged_random(interval_election_low,interval_election_high);
        state_=FOLLOWER;
        currentTerm_ = request->term();
        LOG_DEBUG("Term=%lu %s: \t %s recieve heartbeat",currentTerm_, stateStr().c_str(), addrStr_.c_str());
        return 1;
    }
    // appendEntries
    if(request->prev_log_index() >= BaseLogIndex())
    {
        response->set_success(true);
        uint64_t logStartIndex = FindLog(request->prev_log_term(),request->prev_log_index());
        logs.erase(logs.begin()+logStartIndex+1,logs.end());
        logs.insert(logs.end(),request->entries().begin(),request->entries().end());
        response->set_last_log_index(LastLogIndex());
        response->set_last_log_term(LastLogTerm());
        LOG_DEBUG("Term=%lu %s: \t %s AppendEntries append logs (size=%lu)",currentTerm_, stateStr().c_str(), addrStr_.c_str(),logs.size());
    }
    else
    {
        LOG_DEBUG("Term=%lu %s: \t %s append failed prevLogIdx=%ld, baseLogIdx=%ld",currentTerm_, stateStr().c_str()
            ,addrStr_.c_str(), request->prev_log_index(), BaseLogIndex());
    }
    Dump(guard);
    return 1;
}

void SRaft::OnAppendEntriesReponse(const AppendEntriesResponse& response, bool isHeartbeat)
{
    Guard guard(mut_);
    SRaftPeer& peer = *(peers_[response.name()]);
    if(state_!=LEADER)
    {
        LOG_DEBUG("Term=%lu %s: \t %s AppendEntriesResponse failed (Not Leader)",currentTerm_, stateStr().c_str(), addrStr_.c_str());
        return;
    }
    if(response.term()>currentTerm_)
    {
        LOG_DEBUG("Term=%lu %s: \t %s AppendEntriesResponse failed (Small term), becoming FOLLOWER now",currentTerm_,stateStr().c_str(), addrStr_.c_str());
        state_=FOLLOWER;
        Dump(guard);
    }
    if(isHeartbeat)
    {
        return;
    }
    if(!response.success())
    {
        LOG_DEBUG("Term=%lu %s: \t %s AppendEntriesResponse failed (FOLLOWER failed)",currentTerm_, stateStr().c_str(),addrStr_.c_str());
        peer.nextIndex_ = response.last_log_index()+1;
        return;
    }
    int64_t newCommit = response.last_log_index();
    if(newCommit<=commitIndex_)
    {
        LOG_DEBUG("Term=%lu %s: \t %s append failed (log commited) newComit=%ld vs commitIndex_=%ld"
        ,currentTerm_, stateStr().c_str(), addrStr_.c_str(),newCommit,commitIndex_);
        return;
    }

    // success
    peer.nextIndex_ = response.last_log_index()+1;
    peer.matchIndex_ = response.last_log_index();
    LOG_DEBUG("Term=%lu peer: \t %s nextindex=%ld",currentTerm_, peer.name.c_str(),peer.nextIndex_);
    size_t commitVote = 1;
    for(auto& pp:peers_)
    {
        SRaftPeer& peer2 = *pp.second;
        if(peer2.matchIndex_>=newCommit)
        {
            commitVote++;
        }
    }
    LOG_DEBUG("Term=%lu LEADER: \t %s AppendEntriesResponse, logid=%li, commitvote=%lu",currentTerm_, addrStr_.c_str(),newCommit,commitVote);
    if(commitVote>=(peers_.size())/2)
    {
        commitIndex_=newCommit;
        LOG_DEBUG("Term=%lu LEADER: \t %s AppendEntriesResponse Log no.%lu committed",currentTerm_, addrStr_.c_str(),newCommit);
        DoApply(guard,false);
    }

}

int SRaft::OnRequestVoteRequest(const RequestVoteRequest* request, RequestVoteResponse* response)
{
    Guard guard(mut_);
    if(!running_)
    {
        LOG_DEBUG("Term=%lu %s: \t %s not doing RequestVote from %s",currentTerm_, stateStr().c_str(),addrStr_.c_str(), request->name().c_str());
        return -1;
    }
    response->set_name(addrStr_);
    response->set_seq(request->seq());
    response->set_time(get_current_ms());
    response->set_term(currentTerm_);
    bool candidateLogTermOld = request->last_log_term()<lastIncludeTerm_||
                logs.size()>0 && request->last_log_term()<logs.back().term();
    bool candidateLogIndexOld = request->last_log_index()<lastIncludeIndex_||
                logs.size()>0 && request->last_log_term()==logs.back().term() && request->last_log_index()<logs.size()+lastIncludeIndex_;
    // vote fail
    if(request->term()<=currentTerm_)
    {
        LOG_DEBUG("Term=%lu %s \t %s didn't recieve votes because",currentTerm_, stateStr().c_str(),request->name().c_str());
        LOG_DEBUG("Term=%lu %s \t %s term %lu vs %lu",currentTerm_, stateStr().c_str(),addrStr_.c_str(),currentTerm_,request->term());
        response->set_vote_granted(false);
        return -1;
    }
    else
    {
        currentTerm_ = request->term();
    }
    if(candidateLogIndexOld)
    {
        LOG_DEBUG("Term=%lu %s \t %s didn't recieve votes because",currentTerm_, stateStr().c_str(),request->name().c_str());
        response->set_vote_granted(false);
        LOG_DEBUG("Term=%lu %s \t %s Log Index too Old",currentTerm_,stateStr().c_str(), request->name().c_str());
        return -1;
    }
    if(candidateLogTermOld)
    {
        LOG_DEBUG("Term=%lu %s \t %s didn't recieve votes because",currentTerm_, stateStr().c_str(),request->name().c_str());
        response->set_vote_granted(false);            
        LOG_DEBUG("Term=%lu %s \t %s Log Term too Old",currentTerm_,stateStr().c_str(), request->name().c_str());
        return -1;
    }
    // vote success
    if(state_==LEADER) state_=FOLLOWER;
    LOG_DEBUG("Term=%lu %s:\t %s votes for %s",currentTerm_,stateStr().c_str(), addrStr_.c_str(), request->name().c_str());
    response->set_vote_granted(true);
    Dump(guard);
    return 1;
}

void SRaft::OnRequestVoteResponse(const RequestVoteResponse& response)
{
    Guard guard(mut_);
    LOG_DEBUG("Term=%lu %s:\t %s OnRequestVoteReponse from %s",currentTerm_, stateStr().c_str(),addrStr_.c_str(),response.name().c_str());
    if(response.term()>currentTerm_)
    {
        state_ = FOLLOWER;
        Dump(guard);
        LOG_DEBUG("Term=%lu CANDIDATE:\t %s becomes a FOLLOWER due to a small term %lu vs %lu",currentTerm_, addrStr_.c_str(),response.term(),currentTerm_);
        currentTerm_ = response.term();
    }
    if(state_!=CANDIDATE)
    {
        LOG_DEBUG("Term=%lu %s:\t %s not counting votes, it is not a CANDIDATE it is a %s",currentTerm_, stateStr().c_str(),addrStr_.c_str(),stateStr().c_str());
        return;
    }
    if(response.vote_granted())
    {
        votesRecieved_++;
        if(votesRecieved_>peers_.size()/2)
        {
            state_ = LEADER;
            LOG_DEBUG("Term=%lu CANDIDATE:\t %s becomes the LEADER",currentTerm_, addrStr_.c_str());
        }
    }
}

LogEntry SRaft::GetLogByIndex(uint64_t index)
{
    uint64_t baseIndex = BaseLogIndex();
    if(index-baseIndex>=logs.size())
    {
        LOG_DEBUG("Term=%lu %s: \t %s access log out of bound %lu vs %lu "
            ,currentTerm_, stateStr().c_str(), addrStr_.c_str(),index-baseIndex,logs.size());  
        return {};
    }
    if(index-baseIndex>=0)
    {
        return logs[index-baseIndex];
    }
    else
    {
        return {};
    }
}