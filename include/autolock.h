#pragma once

#include <windows.h>
#include <assert.h>

class CSimpleLock  
{  
    CRITICAL_SECTION __cs;
public:  
    CSimpleLock()  
    {  
        InitializeCriticalSection(&__cs);  
    } 

    ~CSimpleLock()  
    {  
        DeleteCriticalSection(&__cs);  
    }  

    void lock()  
    {  
        EnterCriticalSection(&__cs);  
    }  

    void unlock()  
    {  
        LeaveCriticalSection(&__cs);  
    }  
};

class CAutolock  
{  
    CSimpleLock* __pLock;
public:  
    CAutolock(CSimpleLock* pLock)
        : __pLock(pLock)
    {   
        assert(__pLock);
        __pLock->lock();
    }  

    ~CAutolock()  
    {  
        __pLock->unlock();  
    }  
};