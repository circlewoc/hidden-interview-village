#pragma once
#include<thread>
#include<unordered_map>
#include<mutex>
#include<vector>
#include<functional>
#include"SRaftRpc.h"

class MyChannel;

static uint64_t interval_heartBeat = 30;
static uint64_t interval_election_low = 450;
static uint64_t interval_election_high = 600;
static uint64_t interval_election_fail = 500;
enum State
{
    FOLLOWER,
    CANDIDATE,
    LEADER
};
class SRaftPeer
{
public:
    std::string name;
    std::shared_ptr<SRaftClient> peerSRaftClient;
    int64_t nextIndex_=0;
    int64_t matchIndex_=-1;
};

class SRaft
{
public:
    using Guard = std::lock_guard<std::mutex>;
    using CommitCallback = std::function<int(std::string cmd)>;
    SRaft(std::string addrStr, std::shared_ptr<MyChannel> channel=nullptr);
    ~SRaft();
    
    //rpc
    void AddPeer(std::string peerAddrStr);
    void Run();
    int DoLog(std::string cmd);
    void DoApply(Guard& guard,bool fromSnapshot);
    int OnRequestVoteRequest(const RequestVoteRequest* request, RequestVoteResponse* reponse);
    void OnRequestVoteResponse(const RequestVoteResponse& reponse);
    int OnAppendEntriesRequest(const AppendEntriesRequest* request, AppendEntriesResponse* response);
    void OnAppendEntriesReponse(const AppendEntriesResponse& reponse, bool isHeartbeat);

    // get and set
    std::string AddrStr(){return addrStr_;}
    State GetState(){return state_;};
    void SetCommitCallback(CommitCallback cb){commitCB_ = cb;}
private:
    void OnTimer();
    void SendHeartbeat(Guard& guard);
    void DoAppendEntries(Guard& guard, bool isHeartbeat);
    void DoElection(Guard& guard);
    void Dump(Guard& guard);
    void Load(Guard& guard);
    int64_t BaseLogIndex()
    {
        if(logs.size()==0)
        {
            return -1;
        }
        else
        {
            return logs[0].index();
        }
    }
    int64_t LastLogIndex()
    {
        if(logs.size()==0)
        {
            return -1;
        }
        else
        {
            return logs.back().index();
        }
    }
    uint64_t LastLogTerm()
    {
        if(logs.size()==0)
        {
            return 0;
        }
        else
        {
            return logs.back().term();
        }
    }    
    LogEntry GetLogByIndex(uint64_t index);
    uint64_t FindLog(uint64_t term, uint64_t index)
    {
        for(int i=logs.size()-1;i>=0;i--)
        {
            if(logs[i].term()==term && logs[i].index()==index)
            {
                return i;
            }
        }
        return -1;
    }
        
    //raft states
    State state_=FOLLOWER;
    std::vector<LogEntry> logs;
    std::string votedFor_;
    uint64_t votesRecieved_=0;
    int64_t commitIndex_= -1;
    int64_t lastApplied_= -1;
    uint64_t currentTerm_= 0;
    //install snapshot states
    uint64_t lastIncludeTerm_=0;
    int64_t lastIncludeIndex_=-1;

    //timer
    std::thread timer_thread;
    uint64_t dueHeartBeat_=0;
    uint64_t dueElection_=0;
    uint64_t dueElectionFail_=0;
    bool toDestroy_ = false;

    // rpc
    std::unordered_map<std::string,std::unique_ptr<SRaftPeer>> peers_;
    std::unique_ptr<SRaftServer> SRaftServer_;
    std::string addrStr_;

    // sync
    std::mutex mut_;
    bool running_=false;

    // callback
    CommitCallback commitCB_;
    // others
    std::shared_ptr<MyChannel> channel_;

    std::string stateStr()
    {
        switch (state_)
        {
        case FOLLOWER:
            return "FOLLOWER";
        case CANDIDATE:
            return "CANDIDATE";
        case LEADER:
            return "LEADER";
        default:
            return "UNKNOWN";
        }
        
    }
};