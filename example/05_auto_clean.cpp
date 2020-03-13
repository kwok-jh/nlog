#include <iostream>

//1.连接静态库需要在包含头文件前定义 NLOG_STATIC_LIB, 否则会出现连接错误
#define NLOG_STATIC_LIB

//2.包含头文件
#include "../include/nlog.h"

//3.连接静态库的lib文件
#ifdef _DEBUG
#pragma comment(lib, "nloglibd.lib")
#else
#pragma comment(lib, "nloglib.lib")
#endif

int main()
{
    _NLOG_CFG cfg = {                                //创建日志配置结构       
        L"{module_dir}\\log",                        //日志存储目录    默认是: "{module_dir}\\log\\"
		/* 
		*   文件名格式为: auto_clean_分秒, 
		*   这里精确到秒钟只是为了迫使每一次初始化输出都指向不同的文件;
		*/
        L"auto_clean_%M%S.log",                      //文件名格式      默认是: "log-%m%d-%H%M.log"
        L"%Y-%m-%d",                                 //日期格式        默认是: "%m-%d %H:%M:%S"
        L"[{time}][{level}][{id}][{file}:{line}]: ", //前缀格式        默认是: "[{time}][{level}][{id}]: "
        L"auto_clean_",                              //前缀匹配        默认是: ""
        2,                                           //最大文件数      默认是: -1
        4096,                                        //最大文件大小    默认是: -1
    };

    _NLOG_SET_CONFIG(cfg);      //设置配置
    _NLOG_APP("这是消息一");

    _NLOG_SHUTDOWN();           //输出后释放日志实例, 迫使nlog结束输出并关闭文件
    _NLOG_SET_CONFIG(cfg);

    _NLOG_APP("这是消息二");

    _NLOG_SHUTDOWN();
    _NLOG_SET_CONFIG(cfg);

    printf("即将删除第一个日志文件\n");
    system("pause");
    _NLOG_APP("这是消息三");

    _NLOG_SHUTDOWN();
    _NLOG_SET_CONFIG(cfg); 

    printf("即将删除第二个日志文件\n");
    system("pause");
    _NLOG_APP("这是消息四");

	_NLOG_SHUTDOWN();
	
    cfg.fileName = L"auto_split_%M%S.log";
    cfg.prefixMatch = L"auto_split_";
    _NLOG_SET_CONFIG(cfg);

    printf("采用1MB分割文件\n");
    system("pause");
    for (int i = 0; i < 100; ++i)
    {
        _NLOG_APP() << std::wstring(1024*128, L'H');
    }

    _NLOG_SHUTDOWN(); 

    return 0;
}