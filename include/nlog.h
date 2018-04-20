#pragma once

/*
    * nlog
    * 创建:  2016-6-16
    * 修改:  2018-3-15
    * Email：<kwok-jh@qq.com>
    * Git:   https://gitee.com/skygarth/nlog
	
	* 异步
    * 多线程安全
    * LOG_ERR("Hello, %s", "nlog") << " 现在时间:" << nlog::time;
*/

#include "helper.hpp"
#include "simplelock.hpp"
#include "strconvert.hpp"

#ifndef _WINDOWS_
#error "仅支持Windows平台"
#endif

#include <map>


//iocp.h
class CIOCP;

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
    tstring logDir;
    tstring fileName;
    tstring dateFormat;
    tstring prefixion;
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
    static CSimpleLock                  __mapCsec;
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
        tstring  file;
        uint32_t line;
    };
    tstring Format(const tstring& text, const LogInfomation& info = LogInfomation());
    CLog&   FormatWriteLog(const tstring& strBuf, const LogInfomation& info = LogInfomation());
    CLog&   WriteLog(const tstring& strBuf);
private:
    CIOCP*   __pIocp; 
    HANDLE   __hFile;
    Config   __config;
    bool     __bAlreadyInit;
    LogLevel __filterLevel;

    volatile uint32_t __count;
    LARGE_INTEGER __liNextOffset;
};

//////////////////////////////////////////////////////////////////////////


class CLogHelper
{
public:
    CLogHelper(LogLevel level, const char* file, const uint32_t line)
        : __sessionId("")
    {
        __logInfo.level = level;
#ifdef UNICODE
        __logInfo.file  = helper::GetName(Conver::Str2WStr(file));
#else
        __logInfo.file  = helper::GetName(file);
#endif
        __logInfo.line  = line;
    }

    ~CLogHelper()
    {
        CLog::Instance(__sessionId).FormatWriteLog(__strbuf.str(), __logInfo);
    }

    CLogHelper& Format()
    {
        return *this;
    }

    CLogHelper& Format(const TCHAR * _Format, ...) 
    {
        va_list marker = NULL;  
        va_start(marker, _Format);  
        __strbuf << helper::StrFormatVar(_Format, marker);
        va_end(marker);

        return *this;
    }

    CLogHelper& Format(std::string guid, const TCHAR * _Format, ...)
    {
        __sessionId = guid;

        va_list marker = NULL;  
        va_start(marker, _Format);  
        __strbuf << helper::StrFormatVar(_Format, marker);
        va_end(marker);

        return *this;
    }

    template<class T> 
    CLogHelper& operator<<(T info)
    {
        __strbuf << info;
        return *this;
    }

    CLogHelper& operator<<( CLogHelper&(__cdecl* pfn)(CLogHelper &) )
    {
        return ((*pfn)(*this));
    }

    friend CLogHelper& time(CLogHelper& slef);
    friend CLogHelper& id  (CLogHelper& slef);

protected:
    tsstream __strbuf;
    std::string __sessionId;
    CLog::LogInfomation __logInfo;
};

}// namespace nlog

#define LOG_ERR  nlog::CLogHelper(nlog::LV_ERR, __FILE__, __LINE__).Format
#define LOG_WAR  nlog::CLogHelper(nlog::LV_WAR, __FILE__, __LINE__).Format
#define LOG_APP  nlog::CLogHelper(nlog::LV_APP, __FILE__, __LINE__).Format
#define LOG_PRO  nlog::CLogHelper(nlog::LV_PRO, __FILE__, __LINE__).Format
