#include"Timestamp.h"
#include<chrono>
#include<ctime>
#include<cstdio>

Timestamp::Timestamp():microSecondSinceEpoch_(0){}

Timestamp::Timestamp(int64_t t):microSecondSinceEpoch_(t){}

Timestamp Timestamp::now()
{
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
    return Timestamp(currentTime);
}

std::string Timestamp::toString() const
{
    std::tm* timeinfo = std::localtime(&microSecondSinceEpoch_);
    char buf[128];
    snprintf(buf,128,"%04d/%02d/%02d %02d:%02d:%02d",
    timeinfo->tm_year+1900,
    timeinfo->tm_mon+1,
    timeinfo->tm_mday,
    timeinfo->tm_hour,
    timeinfo->tm_min,
    timeinfo->tm_sec);
    return buf;
}

// #include<iostream>
// int main()
// {
//     std::cout<<Timestamp::now().toString()<<std::endl;
// }