#pragma once

#include <windows.h>

class SimpleLock  
{  
    CRITICAL_SECTION __cs;
public:  
    SimpleLock()  
    {  
        // The second parameter is the spin count, for short-held locks it avoid the
        // contending thread from going to sleep which helps performance greatly.
        ::InitializeCriticalSectionAndSpinCount(&__cs, 2000);
    } 

    ~SimpleLock()  
    {  
        ::DeleteCriticalSection(&__cs);  
    }  

    // If the lock is not held, take it and return true.  If the lock is already
    // held by something else, immediately return false.
    bool Try()
    {
        if (::TryEnterCriticalSection(&__cs) != FALSE) {
            return true;
        }
        return false;
    }

    // Take the lock, blocking until it is available if necessary.
    void Lock()  
    {  
        ::EnterCriticalSection(&__cs);  
    }  

    // Release the lock.  This must only be called by the lock's holder: after
    // a successful call to Try, or a call to Lock.
    void Unlock()  
    {  
        ::LeaveCriticalSection(&__cs);  
    }  
};

// A helper class that acquires the given Lock while the AutoLock is in scope.
class AutoLock  
{  
    AutoLock(const AutoLock&);
    AutoLock& operator=(const AutoLock&);

    SimpleLock& __pLock;
public:  
    explicit AutoLock(SimpleLock& pLock)
        : __pLock(pLock)
    {   
        __pLock.Lock();
    }  

    ~AutoLock()  
    {  
        __pLock.Unlock();  
    }  
};