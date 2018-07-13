#include <iostream>
#include "../include/nlog.h"
#pragma comment(lib, "nlogd.lib")

#define _T(x) x

int main()
{
    LOG_ERR() << "[���Ĳ���]";
    LOG_WAR("my name is %s, I am ", "nlog") << 6666 <<" years old!";
    LOG_APP(L"time is ") << nlog::time << " and thread id: " << nlog::id;

    /*һ���µ���־�Ự������ӡ����һ���ļ�*/
    LOG_ERR("1", _T("���, %s"), _T("nlog")) << _T(" ����ʱ��:") << nlog::time;

    LOG_ERR() << std::string("");

	/*�ͷ�������Դ*/
    nlog::CLog::ReleaseAll();
    return 0;
}