#include <iostream>
#include "../nlog/nlog.h"

int main()
{
    LOG_ERR() << _T("[���Ĳ���]");
    LOG_WAR(_T("my name is %s, I am "), _T("nlog")) << int(6666) << _T(" years old!");
    LOG_APP(_T("time is ")) << nlog::time << _T(" and thread id: ") << nlog::id;
  
	/*һ���µ���־�Ự������ӡ����һ���ļ�*/
    LOG_ERR(0x01, _T("���, %s"), _T("nlog")) << _T(" ����ʱ��:") << nlog::time;

	/*�ͷ�������Դ*/
    nlog::CLog::ReleaseAll();
    return 0;
}