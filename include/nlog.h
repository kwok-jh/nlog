#ifndef nlog_h__
#define nlog_h__

/*
    * nlog
    * 创建:  2016-6-16
    * 修改:  2018-5-8
    * Email：<kwok-jh@qq.com>
    * Git:   https://gitee.com/skygarth/nlog
	
	* 异步
    * 多线程安全
    
    * Example:
    * #include "nlog.h"                                             //包含头文件, 并连接对应的lib
    * ...
    * LOG_ERR("Hello, %s", "nlog") << " Now Time:" << nlog::time;   //c,c++风格混搭格式化输出
    * ...
    * LOG_CLOSE();                                                  //最后执行清理
*/

#include <map>
#include <sstream>
#include <stdint.h>

#define  WIN32_LEAN_AND_MEAN 
#include <windows.h>

class CIOCP;
class SimpleLock;

namespace nlog{

enum LogLevel
{
    LV_ERR = 0,
    LV_WAR = 1,    
    LV_APP = 2,    
    LV_PRO = 3
};

struct Config 
{
    std::wstring logDir;        //日志存储目录
    std::wstring fileName;      //文件名格式
    std::wstring dateFormat;    //日期格式
    std::wstring prefixion;     //前缀格式
};

class CLog
{
    CLog();
    ~CLog();

    CLog(const CLog&);
    CLog operator=(const CLog&);

    friend class CLogHelper;
    friend CLogHelper& time(CLogHelper& slef);

    static std::map<std::string, CLog*> __sMapInstance;
    static std::auto_ptr<SimpleLock>    __mapLock;
public:
    /*
    *   获得一个Log的实例, 允许存在多个Log实例, guid代表实例的唯一Id
    *   每一个实例独占一个日志文件, 若它们之间具有相同的文件名称格式
    *   那么后一个被实例化的Log将指向一个具有"_1"的名称
    */
    static CLog& Instance(std::string guid = "");
    static bool  Release (std::string guid = "");
    static bool  ReleaseAll();

    /* 
    *   Log配置, 输出文件名称格式, 打印格式等...
    *   要注意的是, 设置必须在打印第一条日志之前完成否则可能不起任何作用 
    */
    bool     SetConfig(const Config& setting);
    Config   GetConfig() const;

    /* 在任何时候都可以指定日志打印的等级 */
    LogLevel SetLevel(LogLevel level); 
protected:
    bool    InitLog();
    bool    CompleteHandle(bool bClose = false);

    struct  LogInfomation
    {
        LogLevel level;
        uint32_t line;
        std::wstring  file;
    };
    std::wstring Format (const std::wstring& text, const LogInfomation& info = LogInfomation());
    CLog& FormatWriteLog(const std::wstring& strBuf, const LogInfomation& info = LogInfomation());
    CLog& WriteLog      (const std::wstring& strBuf);
private:
    CIOCP*   __pIocp; 
    HANDLE   __hFile;
    Config   __config;
    bool     __bAlreadyInit;
    LogLevel __filterLevel;

    uint32_t __count;
    LARGE_INTEGER __liNextOffset;
};

//////////////////////////////////////////////////////////////////////////
class CLogHelper
{
public:
    CLogHelper(LogLevel level, const char* file, const uint32_t line, const std::string& guid = "");
    ~CLogHelper();

    CLogHelper& Format();
    CLogHelper& Format(const wchar_t * _Format, ...);
    CLogHelper& Format(const char    * _Format, ...);

    template<class T> 
    CLogHelper& operator<<(T info);
    CLogHelper& operator<<(const std::string& info);
    CLogHelper& operator<<(CLogHelper&(__cdecl* pfn)(CLogHelper &));

    friend CLogHelper& time(CLogHelper& slef);
    friend CLogHelper& id  (CLogHelper& slef);

protected:
    std::string __sessionId;
    std::wstringstream  __strbuf;
    CLog::LogInfomation __logInfo;
};

template<class T> 
CLogHelper& CLogHelper::operator<<(T info){
    __strbuf << info;
    return *this;
}

}// namespace nlog

//使用默认Log实例格式化输出一条信息
#define LOG_ERR  nlog::CLogHelper(nlog::LV_ERR, __FILE__, __LINE__).Format
#define LOG_WAR  nlog::CLogHelper(nlog::LV_WAR, __FILE__, __LINE__).Format
#define LOG_APP  nlog::CLogHelper(nlog::LV_APP, __FILE__, __LINE__).Format
#define LOG_PRO  nlog::CLogHelper(nlog::LV_PRO, __FILE__, __LINE__).Format

//使用指定的Log实例格式化输出一条信息
#define LOG_ERR_WITH_ID(id) nlog::CLogHelper(nlog::LV_ERR, __FILE__, __LINE__, id).Format
#define LOG_WAR_WITH_ID(id) nlog::CLogHelper(nlog::LV_WAR, __FILE__, __LINE__, id).Format
#define LOG_APP_WITH_ID(id) nlog::CLogHelper(nlog::LV_APP, __FILE__, __LINE__, id).Format
#define LOG_PRO_WITH_ID(id) nlog::CLogHelper(nlog::LV_PRO, __FILE__, __LINE__, id).Format

//执行清理工作
#define LOG_CLOSE nlog::CLog::ReleaseAll

#endif // nlog_h__