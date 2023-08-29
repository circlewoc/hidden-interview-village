#pragma once
#include<queue>
#include<string>
#include<memory>
#include<condition_variable>
#include<string>
class MyChannel
{
public:
    void pushData(const std::string& s);
    std::string popData();
private:
    std::queue<std::string> que;
    std::mutex mut_;
    std::condition_variable cv;
};