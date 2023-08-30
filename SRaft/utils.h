#pragma once
#include<chrono>
#include<random>
#include<queue>
#include<string>
#include<mutex>
#include<condition_variable>
uint64_t get_current_ms()
{
    auto currentTime = std::chrono::system_clock::now();
    auto currentTimeInMillis = std::chrono::time_point_cast<std::chrono::milliseconds>(currentTime);
    return currentTimeInMillis.time_since_epoch().count();
}

uint64_t get_ranged_random(uint64_t fr, uint64_t to)
{
    std::random_device rd;
    std::mt19937_64 engine(rd());
    std::uniform_int_distribution<uint64_t> dist(fr,to);
    return dist(engine);
}

