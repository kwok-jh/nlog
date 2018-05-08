#pragma once

/*
    * nlog
    * 创建:  2016-6-16
    * 修改:  2018-5-8
    * Email：<kwok-jh@qq.com>
    * Git:   https://gitee.com/skygarth/nlog
	
	* 异步
    * 多线程安全
    * LOG_ERR("Hello, %s", "nlog") << " 现在时间:" << nlog::time;
*/

#include <map>
#include <sstream>
#include <stdint.h>
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
    std::wstring logDir;
    std::wstring fileName;
    std::wstring dateFormat;
    std::wstring prefixion;
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
    static CLog& Instance(std::string guid = "");
    static bool  Release (std::string guid = "");
    static bool  ReleaseAll();

    bool     SetConfig(const Config& setting);
    Config   GetConfig() const;
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

#define LOG_ERR  nlog::CLogHelper(nlog::LV_ERR, __FILE__, __LINE__).Format
#define LOG_WAR  nlog::CLogHelper(nlog::LV_WAR, __FILE__, __LINE__).Format
#define LOG_APP  nlog::CLogHelper(nlog::LV_APP, __FILE__, __LINE__).Format
#define LOG_PRO  nlog::CLogHelper(nlog::LV_PRO, __FILE__, __LINE__).Format

#define LOG_ERR_WITH_ID(id) nlog::CLogHelper(nlog::LV_ERR, __FILE__, __LINE__, id).Format
#define LOG_WAR_WITH_ID(id) nlog::CLogHelper(nlog::LV_WAR, __FILE__, __LINE__, id).Format
#define LOG_APP_WITH_ID(id) nlog::CLogHelper(nlog::LV_APP, __FILE__, __LINE__, id).Format
#define LOG_PRO_WITH_ID(id) nlog::CLogHelper(nlog::LV_PRO, __FILE__, __LINE__, id).Format