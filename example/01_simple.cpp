#include <iostream>

//1.包含头文件
#include "../include/nlog.h"

//2.连接动态库的lib文件, 注意在项目设置中已将附加库目录设置为"$(SolutionDir)bin\msvc10\lib\"
#ifdef _DEBUG
#pragma comment(lib, "nlogd.lib")
#else
#pragma comment(lib, "nlog.lib")
#endif

int main()
{
    _NLOG_APP("Hello %s!", "World");                                       //打印C风格    多字节

    _NLOG_WAR() << nlog::time  << L" 此后起我们的征程便是星辰大海...";     //打印C++风格  宽字节

    _NLOG_ERR("{1} This is the {2} of %s {3}.", "Cpp")                     //打印C++, Qt风格 
        % nlog::time % "output" % L"style";                                //多字节混合, 宽字节

    _NLOG_ERR("Oh, %s!", "No") << L" 有人动了你的代码并在里面下了毒...";   //混搭式打印

    _NLOG_SET_LEVE(nlog::LV_WAR);                                          //设置只打印警告及以上的
    _NLOG_APP(L"这是一条应用消息...");                                      
    _NLOG_WAR(L"这是一条警告消息...");

    _NLOG_SHUTDOWN();                                                      //关闭nlog, 清理资源
    return 0;
}