#include"Thread.h"
#include<semaphore.h>
#include"CurrentThread.h"
std::atomic_int Thread::numCreated_(0);
Thread::Thread(ThreadFunction f, std::string name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(f))
    , name_(name)
{

}
Thread::~Thread()
{
    sem_t sem;
    sem_init(&sem,0,0);
    if(started_&&!joined_)
    {   
        tid_ = CurrentThread::tid();
        thread_ -> detach();
        sem_post(&sem);
    }
    sem_wait(&sem);
}
void Thread::setDefaultName()
{
    int n = numCreated_++;
    if(name_.empty())
    {
        char buf[32];
        snprintf(buf,sizeof(buf),"Thread %d",n);
        name_ = buf;
    }
}

void Thread::start()  
{
    started_ = true;
    sem_t sem; // to get the correct tid
    sem_init(&sem, false, 0);

    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = CurrentThread::tid();
        sem_post(&sem);
        func_(); 
    }));

    sem_wait(&sem);
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}