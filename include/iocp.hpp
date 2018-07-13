#ifndef iocp_h__
#define iocp_h__

#define  WIN32_LEAN_AND_MEAN 
#include <windows.h>

/*
*    ��ɶ˿ڵļ򵥷�װ 
*    2016-6 By qiling
*/
class CIOCP 
{
public:
    //�������߳���󲢷�����
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
        //_in    pCompKey        ���豸����ʱ���õ�ָ��
        //_in    dwMilliseconds  �����ȴ�ʱ�� INFINITE�����޵ȴ�
        //_out   pdwNumBytes     ������ɵ��ֽ���
        //_out   ppo             �ص�I/O�ṹָ��,Ͷ��I/O����ʱ���õ�ָ�� 
        //                       ���������һ������OVERLAPPED�Ķ��� 
        //                       �����ʹ��CONTAINING_RECORD��øýṹ��ָ�롣
        return !!::GetQueuedCompletionStatus(m_hIOCP, pdwNumBytes, pCompKey, ppo, dwMilliseconds);
    }

private:
    HANDLE m_hIOCP;
};

#endif // iocp_h__