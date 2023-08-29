#pragma once
#define SRAFTDEBUG
#include "noncopyable.h"
#include <string>
#include<cstdio>
#include<mutex>

#define LOG_INFO(logFormat,...)\
    do\
    {\
        Logger& logger = Logger::instance();\
        logger.setLogLevel(INFO);\
        char buf[1024] = {0};\
        snprintf(buf,1024,logFormat, ##__VA_ARGS__);\
        logger.log(buf);\
    } while (0)

#define LOG_ERROR(logFormat,...)\
    do\
    {\
        Logger& logger = Logger::instance();\
        logger.setLogLevel(ERROR);\
        char buf[1024] = {0};\
        snprintf(buf,1024,logFormat, ##__VA_ARGS__);\
        logger.log(buf);\
    } while (0)

#define LOG_FATAL(logFormat,...)\
    do\
    {\
        Logger& logger = Logger::instance();\
        logger.setLogLevel(FATAL);\
        char buf[1024] = {0};\
        snprintf(buf,1024,logFormat, ##__VA_ARGS__);\
        logger.log(buf);\
        exit(-1);\
    } while (0)

#ifdef SRAFTDEBUG
    #define LOG_DEBUG(logFormat,...)\
        do\
        {\
            Logger& logger = Logger::instance();\
            logger.setLogLevel(DEBUG);\
            char buf[1024] = {0};\
            snprintf(buf,1024,logFormat, ##__VA_ARGS__);\
            logger.log(buf);\
        } while (0)
#else
    #define LOG_DEBUG(logFormat,...)
#endif

//log level
enum logLvel
{
    INFO,
    ERROR,
    FATAL,
    DEBUG,
};

//log class
class Logger:Noncopyable
{
public:
    static Logger& instance();
    void setLogLevel(int Level);
    void log(std::string msg);
private:
    int logLevel_;
    std::mutex mut_;
    Logger(){}
};