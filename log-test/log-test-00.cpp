#include <iostream>
#include "../include/nlog.h"
#pragma comment(lib, "nlogd.lib")

#define _T(x) x

int main()
{
    LOG_ERR() << "[中文测试]";
    LOG_WAR("my name is %s, I am ", "nlog") << 6666 <<" years old!";
    LOG_APP(L"time is ") << nlog::time << " and thread id: " << nlog::id;

    /*一个新的日志会话，将打印在另一个文件*/
    LOG_ERR("1", _T("你好, %s"), _T("nlog")) << _T(" 现在时间:") << nlog::time;

    LOG_ERR() << std::string("");

	/*释放所有资源*/
    nlog::CLog::ReleaseAll();
    return 0;
}