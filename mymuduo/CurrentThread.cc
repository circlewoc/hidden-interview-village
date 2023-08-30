#include "CurrentThread.h"

namespace CurrentThread
{
    __thread int t_cachedtid = 0;
    void CacheTid()
    {
        if(t_cachedtid==0)
        {
            t_cachedtid = static_cast<int>(syscall(SYS_gettid));
        }
    }
}