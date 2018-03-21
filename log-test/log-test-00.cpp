#include <iostream>
#include "../nlog/nlog.h"

int main()
{
    LOG_ERR() << _T("[中文测试]");
    LOG_WAR(_T("my name is %s, I am "), _T("nlog")) << int(6666) << _T(" years old!");
    LOG_APP(_T("time is ")) << nlog::time << _T(" and thread id: ") << nlog::id;
  
	/*一个新的日志会话，将打印在另一个文件*/
    LOG_ERR(0x01, _T("你好, %s"), _T("nlog")) << _T(" 现在时间:") << nlog::time;

	/*释放所有资源*/
    nlog::CLog::ReleaseAll();
    return 0;
}