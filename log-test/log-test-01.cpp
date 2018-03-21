#include <iostream>
#include "../nlog/nlog.h"

int main()
{
    nlog::Config logCfg;

	/*日志前缀 如：[log-test-01.cpp: 32 ][BD4     ]: */
    logCfg.prefixion  = _T("[{file}:{line}][{id}]: ");

	/*日期格式*/
	logCfg.dateFormat = _T("%m%d");

	/*日志目录 默认:模块当前目录/log*/
	logCfg.logDir;

	/*文件名格式 {time} {id}将被转换为时间与线程id*/
    logCfg.fileName   = _T("日志{time}.log");
    
	/*设置配置*/
    nlog::CLog::Instance().SetConfig(logCfg);

	/*设置日志等级，大于当前等级的日志将不被打印*/
    nlog::CLog::Instance().SetLevel(nlog::LV_WAR);

	LOG_ERR() << _T("[中文测试]");
	LOG_WAR(_T("my name is %s, I am "), _T("nlog")) << int(6666) << _T(" years old!");
	LOG_APP(_T("time is ")) << nlog::time << _T(" and thread id: ") << nlog::id;

	/*session = 0x01的日志将使用默认设置*/
	LOG_ERR(0x01, _T("你好, %s"), _T("nlog")) << _T(" 现在时间:") << nlog::time;

	/*释放所有资源*/
    nlog::CLog::ReleaseAll();
    return 0;
}