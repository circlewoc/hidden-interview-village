#include"MyChannel.h"
void MyChannel::pushData(const std::string& s)
{
    std::unique_lock<std::mutex>lock(mut_);
    que.push(s);
    cv.notify_one();
}

std::string MyChannel::popData()
{
    std::unique_lock<std::mutex>lock(mut_);
    cv.wait(lock,[&]{return !que.empty();});
    std::string ans = que.front();
    que.pop();
    return ans;
}