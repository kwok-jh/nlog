# nlog - 简单易用的C++日志，线程安全、异步、高性能

* 异步
* 多线程安全
* 据不严格的测试,多线程并发写40w条记录到一个文件的时间是1.343秒
* 同时支持宽字节,多字节字符, 这增加了使用的灵活度但作为代价牺牲的是内部转码的性能损耗.
*       如果你追求性能, 可以尽量采用宽字节字符串

Example:
#include "nlog.h"                                             //包含头文件, 并连接对应的lib
...
LOG_ERR("Hello, %s", "nlog") << " Not Time:" << nlog::time;   //c,c++风格混搭格式化输出
...
LOG_CLOSE();                                                  //最后执行清理