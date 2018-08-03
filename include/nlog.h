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
    * _NLOG_ERR("Hello, %s", "nlog") << " Now Time:" << nlog::time; //c,c++风格混搭格式化输出
    * ...
    * _NLOG_SHUTDOWN();                                             //最后执行清理
*/

#include <map>
#include <sstream>

#define  WIN32_LEAN_AND_MEAN 
#include <windows.h>

/* export */
#ifdef  NLOG_STATIC_LIB
# define _EXPORT_ 
#else

#pragma warning( push )
#pragma warning( disable : 4251 ) 

#ifdef  NLOG_SHARE_LIB
# define _EXPORT_ __declspec(dllexport)
#else
# define _EXPORT_ __declspec(dllimport)
#endif // NLOG_SHARE_LIB
#endif // NLOG_STATIC_LIB

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
    std::wstring logDir;        //日志存储目录    default: "/log/"
    std::wstring fileName;      //文件名格式      default: "log-%m%d-%H%M.log"
    std::wstring dateFormat;    //日期格式        default: "%m-%d %H:%M:%S"
    std::wstring prefixion;     //前缀格式        default: "[{time}][{level}][{id}]: "
};

class _EXPORT_ CLog
{
    CLog();
    ~CLog();

    CLog(const CLog&);
    CLog operator=(const CLog&);

    friend class CLogHelper;
    friend _EXPORT_ CLogHelper& time(CLogHelper& slef);

    static std::map<std::string, CLog*>  __sMapInstance;
    static std::auto_ptr<SimpleLock>     __mapLock;
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
        unsigned int line;
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

    unsigned int __count;
    LARGE_INTEGER __liNextOffset;
};

//////////////////////////////////////////////////////////////////////////
class _EXPORT_ CLogHelper
{
public:
    CLogHelper(LogLevel level, const char* file, const unsigned int line, const std::string& guid = "");
    ~CLogHelper();

    CLogHelper& Format();
    CLogHelper& Format(const wchar_t * _Format, ...);
    CLogHelper& Format(const char    * _Format, ...);

    template<class T> 
    CLogHelper& operator<<(T info);
    CLogHelper& operator<<(const std::string& info);
    CLogHelper& operator<<(CLogHelper&(__cdecl* pfn)(CLogHelper &));

    friend _EXPORT_ CLogHelper& time(CLogHelper& slef);
    friend _EXPORT_ CLogHelper& id  (CLogHelper& slef);

private:
    std::string __sessionId;
    std::wstringstream  __strbuf;
    CLog::LogInfomation __logInfo;
};

template<class T> 
CLogHelper& CLogHelper::operator<<(T info){
    __strbuf << info;
    return *this;
}

_EXPORT_ CLogHelper& time(CLogHelper& slef);
_EXPORT_ CLogHelper& id  (CLogHelper& slef);

}// namespace nlog

#ifndef  NLOG_STATIC_LIB
#pragma warning( pop )
#endif

/*
*	使用默认Log实例, 格式化输出一条信息
*   example:
*   _NLOG_ERR("hello") << "nlog";
*/
#define _NLOG_ERR  nlog::CLogHelper(nlog::LV_ERR, __FILE__, __LINE__).Format
#define _NLOG_WAR  nlog::CLogHelper(nlog::LV_WAR, __FILE__, __LINE__).Format
#define _NLOG_APP  nlog::CLogHelper(nlog::LV_APP, __FILE__, __LINE__).Format
#define _NLOG_PRO  nlog::CLogHelper(nlog::LV_PRO, __FILE__, __LINE__).Format

/*
*	使用指定的Log实例, 格式化输出一条信息
*   example:
*   #define LOG_UID    "device_support"
*   #define LOG_ERR    _NLOG_ERR_WITH_ID(LOG_UID)   
*   ...
*   _NLOG_ERR("hello") << "nlog";     
*/
#define _NLOG_ERR_WITH_ID(id) nlog::CLogHelper(nlog::LV_ERR, __FILE__, __LINE__, id).Format
#define _NLOG_WAR_WITH_ID(id) nlog::CLogHelper(nlog::LV_WAR, __FILE__, __LINE__, id).Format
#define _NLOG_APP_WITH_ID(id) nlog::CLogHelper(nlog::LV_APP, __FILE__, __LINE__, id).Format
#define _NLOG_PRO_WITH_ID(id) nlog::CLogHelper(nlog::LV_PRO, __FILE__, __LINE__, id).Format

/*
*   设置初始配置
*   example:
*      
*   _NLOG_CFG cfg = {
*       L"",
*       L"ios device support %m%d %H%M.log",
*       L"",
*       L"[{time}][{level}][{id}][{file}:{line}]: "
*   };
*
*   _NLOG_SET_CONFIG(cfg);
*/

#define _NLOG_CFG                            nlog::Config
#define _NLOG_SET_CONFIG(cfg)                nlog::CLog::Instance().SetConfig(cfg)
#define _NLOG_SET_CONFIG_WITH_ID(id, cfg)    nlog::CLog::Instance(id).SetConfig(cfg)

/*
*	执行清理工作, 销毁所有存在的nlog实例
*   example: - 初始配置与自动销毁
*
*   struct _NLogMgr 
*   {
*        _NLogMgr() 
*        {
*            _NLOG_CFG cfg = {
*                L"",
*                L"nlog-%m%d%H%M.log",
*                L"",
*                L"[{time}][{level}][{id}][{file}:{line}]: "
*            };
*    
*            _NLOG_SET_CONFIG(cfg);
*        }
*    
*        ~_NLogMgr() {
*            _NLOG_SHUTDOWN();
*        }
*    };
*
*    static _NLogMgr _NLog;
*/
#define _NLOG_SHUTDOWN  nlog::CLog::ReleaseAll

#endif // nlog_h__