#ifndef nlog_h__
#define nlog_h__

/*
    * nlog
    * ����:  2016-6-16
    * �޸�:  2018-5-8
    * Email��<kwok-jh@qq.com>
    * Git:   https://gitee.com/skygarth/nlog
	
	* �첽
    * ���̰߳�ȫ
    
    * Example:
    * #include "nlog.h"                                             //����ͷ�ļ�, �����Ӷ�Ӧ��lib
    * ...
    * LOG_ERR("Hello, %s", "nlog") << " Now Time:" << nlog::time;   //c,c++������ʽ�����
    * ...
    * LOG_CLOSE();                                                  //���ִ������
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
    std::wstring logDir;        //��־�洢Ŀ¼
    std::wstring fileName;      //�ļ�����ʽ
    std::wstring dateFormat;    //���ڸ�ʽ
    std::wstring prefixion;     //ǰ׺��ʽ
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
    *   ���һ��Log��ʵ��, ������ڶ��Logʵ��, guid����ʵ����ΨһId
    *   ÿһ��ʵ����ռһ����־�ļ�, ������֮�������ͬ���ļ����Ƹ�ʽ
    *   ��ô��һ����ʵ������Log��ָ��һ������"_1"������
    */
    static CLog& Instance(std::string guid = "");
    static bool  Release (std::string guid = "");
    static bool  ReleaseAll();

    /* 
    *   Log����, ����ļ����Ƹ�ʽ, ��ӡ��ʽ��...
    *   Ҫע�����, ���ñ����ڴ�ӡ��һ����־֮ǰ��ɷ�����ܲ����κ����� 
    */
    bool     SetConfig(const Config& setting);
    Config   GetConfig() const;

    /* ���κ�ʱ�򶼿���ָ����־��ӡ�ĵȼ� */
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

//ʹ��Ĭ��Logʵ����ʽ�����һ����Ϣ
#define LOG_ERR  nlog::CLogHelper(nlog::LV_ERR, __FILE__, __LINE__).Format
#define LOG_WAR  nlog::CLogHelper(nlog::LV_WAR, __FILE__, __LINE__).Format
#define LOG_APP  nlog::CLogHelper(nlog::LV_APP, __FILE__, __LINE__).Format
#define LOG_PRO  nlog::CLogHelper(nlog::LV_PRO, __FILE__, __LINE__).Format

//ʹ��ָ����Logʵ����ʽ�����һ����Ϣ
#define LOG_ERR_WITH_ID(id) nlog::CLogHelper(nlog::LV_ERR, __FILE__, __LINE__, id).Format
#define LOG_WAR_WITH_ID(id) nlog::CLogHelper(nlog::LV_WAR, __FILE__, __LINE__, id).Format
#define LOG_APP_WITH_ID(id) nlog::CLogHelper(nlog::LV_APP, __FILE__, __LINE__, id).Format
#define LOG_PRO_WITH_ID(id) nlog::CLogHelper(nlog::LV_PRO, __FILE__, __LINE__, id).Format

//ִ��������
#define LOG_CLOSE nlog::CLog::ReleaseAll

#endif // nlog_h__