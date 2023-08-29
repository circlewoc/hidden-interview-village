#include "SRaft.h"
#include "utils_test.h"
#include"Logger.h"
#include<thread>
#include<chrono>
void TestDoLog(int numNode)
{
    MakeNodes(numNode);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    int logNum = 10;
    for(int i=0;i<5;i++)
    {
        SRaft* leader = PickLeader();
        if(leader!=nullptr)
        {
            std::string logstr = std::string("log ") + std::to_string(i);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));       
            LOG_INFO("user do log i=%d",i);

            leader->DoLog(logstr);
        }
    }
}

void TestMakeServer()
{
    MakeServers(5);
    RunServerRaft();
}

void TestClientGet(int serverNum)
{
    MakeServers(serverNum);
    RunServerRaft();
    InitClient();
    for(int i=0;i<5;i++)
    {
        client.Get("hello");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));       
    }
}

void TestClientPut(int serverNum)
{
    MakeServers(serverNum);
    RunServerRaft();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));   
    InitClient();
    std::vector<std::string> keys{"k1","k2","k3"};
    std::vector<std::string> values{"v1","v2","v3"};
    for(int i=0;i<keys.size();i++)
    {
        client.Put(keys[i],values[i]);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));   
    }
    for(int i=0;i<keys.size();i++)
    {
        std::string value = client.Get(keys[i]);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));   
        LOG_INFO("client Get: key=%s value=%s",keys[i].c_str(),value.c_str());
    }
    LOG_INFO("finished");
}

int main()
{
    TestClientPut(10);

}

