#include "Logger.h"
#include<iostream>
#include"Timestamp.h"
Logger& Logger::instance()
{
    static Logger logger;
    return logger;
}
void Logger::setLogLevel(int Level)
{
    std::lock_guard<std::mutex> lock(mut_);
    logLevel_ = Level;
}
void Logger::log(std::string msg)
{
    std::lock_guard<std::mutex> lock(mut_);
    switch (logLevel_)
    {
    case INFO:
        std::cout<<"[INFO]";
        break;
    case ERROR:
        std::cout<<"[ERROR]";
        break;
    case FATAL:
        std::cout<<"[FATAL]";
        break;
    case DEBUG:
        std::cout<<"[DEBUG]";
        break;
    default:
        break;
    }

    std::cout<<Timestamp::now().toString()<<" : "<<msg<<std::endl;
}