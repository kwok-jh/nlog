#include <iostream>
#include "../nlog/nlog.h"

int main()
{
    nlog::Config logCfg;

	/*��־ǰ׺ �磺[log-test-01.cpp: 32 ][BD4     ]: */
    logCfg.prefixion  = _T("[{file}:{line}][{id}]: ");

	/*���ڸ�ʽ*/
	logCfg.dateFormat = _T("%m%d");

	/*��־Ŀ¼ Ĭ��:ģ�鵱ǰĿ¼/log*/
	logCfg.logDir;

	/*�ļ�����ʽ {time} {id}����ת��Ϊʱ�����߳�id*/
    logCfg.fileName   = _T("��־{time}.log");
    
	/*��������*/
    nlog::CLog::Instance().SetConfig(logCfg);

	/*������־�ȼ������ڵ�ǰ�ȼ�����־��������ӡ*/
    nlog::CLog::Instance().SetLevel(nlog::LV_WAR);

	LOG_ERR() << _T("[���Ĳ���]");
	LOG_WAR(_T("my name is %s, I am "), _T("nlog")) << int(6666) << _T(" years old!");
	LOG_APP(_T("time is ")) << nlog::time << _T(" and thread id: ") << nlog::id;

	/*session = 0x01����־��ʹ��Ĭ������*/
	LOG_ERR(0x01, _T("���, %s"), _T("nlog")) << _T(" ����ʱ��:") << nlog::time;

	/*�ͷ�������Դ*/
    nlog::CLog::ReleaseAll();
    return 0;
}