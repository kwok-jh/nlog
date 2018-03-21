#include <iostream>
#include <time.h>

#include "../nlog/nlog.h"

/*���̲߳���д40������־*/

#define SAME_FILE		//�Ƿ�Ϊͬһ���ļ�
#define THREAD_NUM 4	//�����߳���

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
    for (int i = 1; i <= 100000; ++ i)
    {
#ifdef  SAME_FILE
		LOG_APP(_T("log infomation number: %-d  "), i) << _T("endl");
#else
		LOG_APP(GetCurrentThreadId(), _T("log infomation number: %-d  "), i) << _T("endl");
#endif
    }
    return 0L;
}

int main()
{
	std::cout << "���̲߳���д40������־" << std::endl;
	time_t start = clock();

    HANDLE handle[THREAD_NUM] = {0};
    for (int i = 0; i < THREAD_NUM; ++i)
    {
        handle[i] = CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL);
    }

    WaitForMultipleObjects(THREAD_NUM, handle, true, INFINITE);

	for (int i = 0; i < THREAD_NUM; ++i)
	{
		CloseHandle(handle[i]);
	}
	
    nlog::CLog::ReleaseAll();

	time_t end = clock();
	std::cout << "����ʱ�䣺" << (double(end -start)/CLOCKS_PER_SEC) << std::endl;
	system("pause");
    return 0;
}