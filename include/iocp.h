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
    ��ɶ˿ڵļ򵥷�װ 
    2016-6-16 By GuoJH
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

    //������ɶ˿ڣ������̲߳�������
    bool Create(int nMaxConcurrency = 0) 
    {
        m_hIOCP = ::CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, nMaxConcurrency);
        return m_hIOCP != NULL;
    }

    //���豸����ɼ����Զ����ָ�룩������ɶ˿�
    bool AssociateDevice(HANDLE hDevice, ULONG_PTR CompKey) 
    {
        return ::CreateIoCompletionPort(hDevice, m_hIOCP, CompKey, 0) == m_hIOCP;
    }
    
    //��socket������ɶ˿�
    bool AssociateSocket(SOCKET hSocket, ULONG_PTR CompKey) 
    {
        return AssociateDevice((HANDLE) hSocket, CompKey);
    }

    //����ɶ˿�Ͷ��һ��״̬��GetStatus���Ի�ȡ��״̬
    bool PostStatus(ULONG_PTR CompKey, DWORD dwNumBytes = 0,  OVERLAPPED* po = NULL) 
    {
        //dwNumBytes �������ת�Ƶ��ֽ���
        //po�ص��ṹ��ָ��
        return !!::PostQueuedCompletionStatus(m_hIOCP, dwNumBytes, CompKey, po);
    }

    //������ɶ˿ڵ�״̬
    bool GetStatus(LPDWORD pdwNumBytes, ULONG_PTR* pCompKey, LPOVERLAPPED* ppo, DWORD dwMilliseconds = INFINITE) 
    {
        //_in    pCompKey        ���豸����ʱ���õ�ָ��
        //_in    dwMilliseconds  �����ȴ�ʱ�� INFINITE�����޵ȴ�
        //_out    pdwNumBytes    ������ɵ��ֽ���
        //_out    ppo            �ص�I/O�ṹָ��,Ͷ��I/O����ʱ���õ�ָ�� 
        //                       ���������һ������OVERLAPPED�Ķ��� 
        //                       �����ʹ��CONTAINING_RECORD��øýṹ��ָ�롣
        return !!::GetQueuedCompletionStatus(m_hIOCP, pdwNumBytes, pCompKey, ppo, dwMilliseconds);
    }
private:
    HANDLE m_hIOCP;
};


///////////////////////////////// End of File /////////////////////////////////