#ifndef iocp_h__
#define iocp_h__

#define  WIN32_LEAN_AND_MEAN 
#include <windows.h>

/*
*    完成端口的简单封装 
*    2016-6 By qiling
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

    bool Create(int nMaxConcurrency = 0) 
    {
        m_hIOCP = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, nMaxConcurrency);
        return m_hIOCP != NULL;
    }

    bool AssociateDevice(HANDLE hDevice, ULONG_PTR CompKey) 
    {
        return ::CreateIoCompletionPort(hDevice, m_hIOCP, CompKey, 0) == m_hIOCP;
    }
    
    bool AssociateSocket(SOCKET hSocket, ULONG_PTR CompKey) 
    {
        return AssociateDevice((HANDLE) hSocket, CompKey);
    }

    bool PostStatus(ULONG_PTR CompKey, DWORD dwNumBytes = 0,  OVERLAPPED* po = NULL) 
    {
        return !!::PostQueuedCompletionStatus(m_hIOCP, dwNumBytes, CompKey, po);
    }

    bool GetStatus(LPDWORD pdwNumBytes, ULONG_PTR* pCompKey, LPOVERLAPPED* ppo, DWORD dwMilliseconds = INFINITE) 
    {
        //_in    pCompKey        与设备关联时设置的指针
        //_in    dwMilliseconds  操作等待时间 INFINITE：无限等待
        //_out   pdwNumBytes     操作完成的字节数
        //_out   ppo             重叠I/O结构指针,投放I/O操作时设置的指针 
        //                       若传入的是一个包含OVERLAPPED的对象 
        //                       则可以使用CONTAINING_RECORD获得该结构的指针。
        return !!::GetQueuedCompletionStatus(m_hIOCP, pdwNumBytes, pCompKey, ppo, dwMilliseconds);
    }

private:
    HANDLE m_hIOCP;
};

#endif // iocp_h__