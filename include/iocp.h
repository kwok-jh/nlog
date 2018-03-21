#pragma once

#include <windows.h>
#include <assert.h>

/*    
HANDLE WINAPI CreateIoCompletionPort(
    _In_     HANDLE    FileHandle,
    _In_opt_ HANDLE    ExistingCompletionPort,        
    _In_     ULONG_PTR CompletionKey,                
    _In_     DWORD     NumberOfConcurrentThreads    
    );
*/

/*
    完成端口的简单封装 
    2016-6-16 By GuoJH
*/
class CIOCP 
{
public:
    //参数：线程最大并发数量
    CIOCP(int nMaxConcurrency = -1)
        : m_hIOCP(NULL)
    { 
        if (nMaxConcurrency != -1)
        {
            Create(nMaxConcurrency);
        }
    }

    ~CIOCP() 
    { 
        if (m_hIOCP != NULL) 
        {
            CloseHandle(m_hIOCP);
        }
    }

    void Close() 
    {
        CloseHandle(m_hIOCP);
        m_hIOCP = NULL;
    }

    //创建完成端口，设置线程并发数量
    bool Create(int nMaxConcurrency = 0) 
    {
        m_hIOCP = ::CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, nMaxConcurrency);
        return m_hIOCP != NULL;
    }

    //将设备、完成键（自定义的指针）关联完成端口
    bool AssociateDevice(HANDLE hDevice, ULONG_PTR CompKey) 
    {
        return ::CreateIoCompletionPort(hDevice, m_hIOCP, CompKey, 0) == m_hIOCP;
    }
    
    //将socket关联完成端口
    bool AssociateSocket(SOCKET hSocket, ULONG_PTR CompKey) 
    {
        return AssociateDevice((HANDLE) hSocket, CompKey);
    }

    //向完成端口投放一个状态，GetStatus可以获取该状态
    bool PostStatus(ULONG_PTR CompKey, DWORD dwNumBytes = 0,  OVERLAPPED* po = NULL) 
    {
        //dwNumBytes 操作完成转移的字节数
        //po重叠结构的指针
        return !!::PostQueuedCompletionStatus(m_hIOCP, dwNumBytes, CompKey, po);
    }

    //监听完成端口的状态
    bool GetStatus(LPDWORD pdwNumBytes, ULONG_PTR* pCompKey, LPOVERLAPPED* ppo, DWORD dwMilliseconds = INFINITE) 
    {
        //_in    pCompKey        与设备关联时设置的指针
        //_in    dwMilliseconds  操作等待时间 INFINITE：无限等待
        //_out    pdwNumBytes    操作完成的字节数
        //_out    ppo            重叠I/O结构指针,投放I/O操作时设置的指针 
        //                       若传入的是一个包含OVERLAPPED的对象 
        //                       则可以使用CONTAINING_RECORD获得该结构的指针。
        return !!::GetQueuedCompletionStatus(m_hIOCP, pdwNumBytes, pCompKey, ppo, dwMilliseconds);
    }
private:
    HANDLE m_hIOCP;
};


///////////////////////////////// End of File /////////////////////////////////