#include <iostream>

//1.���Ӿ�̬����Ҫ�ڰ���ͷ�ļ�ǰ���� NLOG_STATIC_LIB, �����������Ӵ���
#define NLOG_STATIC_LIB

//2.����ͷ�ļ�
#include "../include/nlog.h"

//3.���Ӿ�̬���lib�ļ�
#ifdef _DEBUG
#pragma comment(lib, "nloglibd.lib")
#else
#pragma comment(lib, "nloglib.lib")
#endif

int main()
{
    _NLOG_CFG cfg = {                                //������־���ýṹ       
        L"{module_dir}\\log",                        //��־�洢Ŀ¼    Ĭ����: "{module_dir}\\log\\"
		/* 
		*   �ļ�����ʽΪ: auto_clean_����, 
		*   ���ﾫȷ������ֻ��Ϊ����ʹÿһ�γ�ʼ�������ָ��ͬ���ļ�;
		*/
        L"auto_clean_%M%S.log",                      //�ļ�����ʽ      Ĭ����: "log-%m%d-%H%M.log"
        L"%Y-%m-%d",                                 //���ڸ�ʽ        Ĭ����: "%m-%d %H:%M:%S"
        L"[{time}][{level}][{id}][{file}:{line}]: ", //ǰ׺��ʽ        Ĭ����: "[{time}][{level}][{id}]: "
        L"auto_clean_",                              //ǰ׺ƥ��        Ĭ����: ""
        2,                                           //����ļ���      Ĭ����: -1
        4096,                                        //����ļ���С    Ĭ����: -1
    };

    _NLOG_SET_CONFIG(cfg);      //��������
    _NLOG_APP("������Ϣһ");

    _NLOG_SHUTDOWN();           //������ͷ���־ʵ��, ��ʹnlog����������ر��ļ�
    _NLOG_SET_CONFIG(cfg);

    _NLOG_APP("������Ϣ��");

    _NLOG_SHUTDOWN();
    _NLOG_SET_CONFIG(cfg);

    printf("����ɾ����һ����־�ļ�\n");
    system("pause");
    _NLOG_APP("������Ϣ��");

    _NLOG_SHUTDOWN();
    _NLOG_SET_CONFIG(cfg); 

    printf("����ɾ���ڶ�����־�ļ�\n");
    system("pause");
    _NLOG_APP("������Ϣ��");

	_NLOG_SHUTDOWN();
	
    cfg.fileName = L"auto_split_%M%S.log";
    cfg.prefixMatch = L"auto_split_";
    _NLOG_SET_CONFIG(cfg);

    printf("����1MB�ָ��ļ�\n");
    system("pause");
    for (int i = 0; i < 100; ++i)
    {
        _NLOG_APP() << std::wstring(1024*128, L'H');
    }

    _NLOG_SHUTDOWN(); 

    return 0;
}