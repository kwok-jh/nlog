 ## nlog - 简单易用的C++日志，线程安全、异步、高性能
 
 1. 异步
 1. 多线程安全
 1. 据不严格的测试,多线程并发写40w条记录到一个文件的时间是1.343秒
 1. 同时支持宽字节,多字节字符, 这增加了使用的灵活度但作为代价牺牲的是内部转码的性能损耗.
 1. 如果你追求性能, 可以尽量采用宽字节字符串
 
 ## 快速上手
 ```
 #include "nlog.h"                                             //包含头文件, 并连接对应的lib
 ...
 _NLOG_ERR("Hello, %s", "nlog") << " Now Time:" << nlog::time; //c,c++风格混搭格式化输出
 ...
 _NLOG_SHUTDOWN();                                             //最后执行清理
 ```
 
 ## 组织结构
 
 | 文件                                 | 说明                              |
 | --------                             | -----:                            |
 | ./example                            | 实例代码                          |
 | ./include                            | 外部引用所需的头文件              |
 | ./src                                | 源文件                            |
 | ./msvc08                             | vs2005                            |
 | ./msvc08/nlog.vcproj                 | 动态库项目文件                    |
 | ./msvc08/nloglib.vcproj              | 静态库项目文件                    |
 | ./msvc10                             | vs2010                            |
 | ./msvc10nlog.sln                     | vs2010解决方案                    |
 | ./msvc10/nlog.vcproj                 | 动态库项目文件                    |
 | ./msvc10/nloglib.vcproj              | 静态库项目文件                    |
 | ./msvc10/simple.vcxproj              | 最简单的使用例子(使用动态库)      |
 | ./msvc10/custom_style.vcxproj        | 自定义打印风格的例子(使用静态库)  |
 
 ## 更新
 1. 提供example
 1. 提供动态库, 静态库vs2010与vs2005的项目文件