#pragma once
#include <unistd.h>
#include <sys/syscall.h>
namespace CurrentThread
{
    extern __thread int t_cachedtid;
    void CacheTid();

    inline int tid()
    {
        if(__builtin_expect(t_cachedtid==0,0))
        {
            CacheTid();
        }
        return t_cachedtid;
    } 

}