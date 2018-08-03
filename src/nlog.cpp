
#include <atltime.h>
#include "../include/nlog.h"
#include "../include/iocp.hpp"

#include "string_util.hpp"
#include "simple_lock.hpp"
#include "string_conversions.hpp"

namespace nlog{

//继承重叠机构的IO结构，用于投递异步写入重叠操作
class CLogIO : public OVERLAPPED
{
    std::wstring  __strBuf;

public:
    CLogIO()
    { 
        Init();
    }

    CLogIO(const std::wstring& strBuf, const LARGE_INTEGER& offset)
    {
        Init();

        __strBuf = strBuf;
        this->Offset = offset.LowPart;
        this->OffsetHigh = offset.HighPart;
    }

    size_t Size() const
    {
        return __strBuf.length() * sizeof(std::wstring::value_type);
    }

    const wchar_t* szBuf()
    {
        return __strBuf.c_str();
    }
private:
    void Init()
    {
        //初始化OVERLAPPED结构
        memset( this, 0, sizeof(OVERLAPPED) );
    }
};

//////////////////////////////////////////////////////////////////////////
//CLog static function
std::map<std::string, CLog*> CLog::__sMapInstance;
std::auto_ptr<SimpleLock>    CLog::__mapLock = std::auto_ptr<SimpleLock>(new SimpleLock);

CLog& 
CLog::Instance( std::string guid /*= ""*/ )
{
    AutoLock lock(*__mapLock);

    std::map<std::string, CLog*>::iterator i = __sMapInstance.find(guid);
    if(i == __sMapInstance.end())
    {
        __sMapInstance[guid] = new CLog();
    }

    return *__sMapInstance[guid];
}

bool 
CLog::Release( std::string guid /*= ""*/ )
{
    CLog* _this = 0;
    {
        AutoLock lock(*__mapLock);

        std::map<std::string, CLog*>::iterator i = __sMapInstance.find(guid);
        if(i == __sMapInstance.end())
        {
            return false;
        }

        _this = i->second;
        __sMapInstance.erase(i);
    }

    //////////////////////////////////////////////////////////////////////////
    /*关闭完成端口, 并释放完成的资源*/

    if(_this->__hFile != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_this->__hFile);
        _this->__hFile = INVALID_HANDLE_VALUE;
    }

    _this->CompleteHandle(true);
    _this->__pIocp->Close();
    
    return true;
}

bool 
CLog::ReleaseAll()
{
    AutoLock lock(*__mapLock);

    std::map<std::string, CLog*>::iterator i = __sMapInstance.begin();
    while(i != __sMapInstance.end())
    {
        Release(i->first);
        i = __sMapInstance.begin();
    }

    return true;
}
//CLog member function
//////////////////////////////////////////////////////////////////////////

CLog::CLog()
    : __pIocp(new CIOCP(1))
    , __count(0)
    , __hFile(INVALID_HANDLE_VALUE)
    , __bAlreadyInit(false)
    , __filterLevel(LV_PRO)
{
    __liNextOffset.QuadPart = 0;

    SetConfig(Config());
}

CLog::~CLog()
{
    if(__pIocp)
    {
        delete __pIocp;
        __pIocp = 0;
    }
}

bool 
CLog::SetConfig( const Config& setting )
{
    __config = setting;
    if(setting.logDir.empty())
    {
        wchar_t moduleName[MAX_PATH];
        ::GetModuleFileNameW(0, moduleName, sizeof moduleName);

        __config.logDir = StrRightCarveWhit(moduleName, L"\\").first +  L"\\log\\";
    }

    if(setting.fileName.empty())
        __config.fileName = (LPCTSTR)CTime::GetCurrentTime().Format(L"log-%m%d-%H%M.log");

    if(setting.dateFormat.empty())
        __config.dateFormat = L"%m-%d %H:%M:%S";

    if(setting.prefixion.empty())
        /*[{time}] [{id}] [{level}] [{file}:{line}]*/
        __config.prefixion = L"[{time}][{level}][{id}]: ";

    return true;
}

nlog::Config 
CLog::GetConfig() const
{
    return __config;
}

LogLevel 
CLog::SetLevel( LogLevel level )
{
    LogLevel old = __filterLevel;
    __filterLevel = level;

    return old;
}

bool 
CLog::InitLog()
{
    AutoLock lock(*__mapLock);

    if(__bAlreadyInit)
    {
        return false;
    }
    
    //////////////////////////////////////////////////////////////////////////
    std::wstring fileName   = __config.logDir + _T("\\") + Format(__config.fileName);
    bool bFileExist = false;
    do
    {
        bFileExist = (::GetFileAttributesW(fileName.c_str()) != INVALID_FILE_ATTRIBUTES);

        if(!bFileExist)
            ::CreateDirectoryW(__config.logDir.c_str(), 0);

        __hFile = ::CreateFileW( 
            fileName.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ,     //共享读取
            NULL,
            OPEN_ALWAYS,         //打开文件若不存在则创建
            FILE_FLAG_OVERLAPPED,//使用重叠IO
            0);

        if(__hFile == INVALID_HANDLE_VALUE)
        {
            fileName.insert(fileName.rfind(L"."), L"_1");
        }

    }while( __hFile == INVALID_HANDLE_VALUE );

    ::GetFileSizeEx(__hFile, &__liNextOffset);
    __pIocp->AssociateDevice(__hFile, 0 );

    if(!bFileExist)
    {
#ifdef UNICODE
        /*如果是Unicode则写入BOM, 文件头LE:0xfffe BE:0xfeff*/
        WriteLog(std::wstring(1, 0xfeff));
#endif
    }

    __bAlreadyInit = true;
    return true;
}

bool 
CLog::CompleteHandle( bool bClose /*= false*/ )
{
    DWORD       dwNumBytes = 0;
    ULONG_PTR   compKey    = 0;
    CLogIO*     pIo        = 0;

    while(1)
    {
        //获得完成队列，如果pio不为null那么就将其释放
        __pIocp->GetStatus( &dwNumBytes, &compKey, 
            (OVERLAPPED**)&pIo, 0 );

        if( pIo != NULL ) 
        { 
            ::InterlockedDecrement((LONG*)&__count);

            delete pIo; 
            pIo = NULL; 
        }
        else if( !bClose )
        {
            break;
        }

        if( bClose && 0 == __count )
        { 
            break;
        }
    }
    return true;
}

std::wstring 
CLog::Format( const std::wstring& text, const LogInfomation& info )
{
    std::wstring result  = (LPCTSTR)CTime::GetCurrentTime().Format(text.c_str());
    std::wstring strTime = (LPCTSTR)CTime::GetCurrentTime().Format(__config.dateFormat.c_str());
    std::wstring strId   = StrFormat(_T("%- 8X"), ::GetCurrentThreadId());
    std::wstring strLine = StrFormat(_T("%- 4d"), info.line);
    std::wstring strLevel;

    switch(info.level)
    {
    case LV_ERR: strLevel = _T("ERR"); break;
    case LV_WAR: strLevel = _T("WAR"); break;
    case LV_APP: strLevel = _T("APP"); break;
    case LV_PRO: strLevel = _T("PRO"); break;
    }
    
    result = StrReplace(result, _T("{level}"),strLevel);
    result = StrReplace(result, _T("{time}"), strTime);
    result = StrReplace(result, _T("{id}"),   strId);
    result = StrReplace(result, _T("{file}"), info.file);
    result = StrReplace(result, _T("{line}"), strLine);
    
    return result;
}

CLog& 
CLog::FormatWriteLog( const std::wstring& strBuf, const LogInfomation& info /*= Loginfomation()*/ )
{
    if(__filterLevel >= info.level)
    {
        if(!__bAlreadyInit)
        {
            InitLog();
        }

        return WriteLog(Format(__config.prefixion, info) + strBuf + L"\r\n");
    }
    else
        return *this;
}

CLog& 
CLog::WriteLog( const std::wstring& strBuf )
{
    CompleteHandle();

    CLogIO* pIo = NULL;
    {
        AutoLock lock(*__mapLock);
        pIo = new CLogIO(strBuf, __liNextOffset);
        ::InterlockedExchangeAdd64(&__liNextOffset.QuadPart, pIo->Size());
    }

    //投递重叠IO
    ::WriteFile( __hFile, pIo->szBuf(), pIo->Size(), NULL, pIo );
    ::InterlockedIncrement((LONG*)&__count);

    if(GetLastError() == ERROR_IO_PENDING)
    {
        /*
            MSDN:The error code WSA_IO_PENDING indicates that the overlapped operation 
            has been successfully initiated and that completion will be indicated at a 
            later time.
        */
        SetLastError(S_OK);
    }
    return *this;
}

//////////////////////////////////////////////////////////////////////////
//CLogHelper firend function
CLogHelper& 
time(CLogHelper& slef)
{
    slef.__strbuf << (LPCTSTR)CTime::GetCurrentTime().Format(
        CLog::Instance(slef.__sessionId).__config.dateFormat.c_str());
    return slef;
}

CLogHelper& 
id(CLogHelper& slef)
{
    slef.__strbuf << StrFormat(_T("%- 8X"), ::GetCurrentThreadId());
    return slef;
}

CLogHelper::CLogHelper(LogLevel level, const char* file, const unsigned int line, const std::string& guid /*= ""*/) 
    : __sessionId(guid)
{
    __logInfo.file    = StrRightCarveWhit(Conver::Str2WStr(file), L"\\").first;
    __logInfo.level   = level;
    __logInfo.line    = line;
}

CLogHelper::~CLogHelper()
{
    CLog::Instance(__sessionId).FormatWriteLog(__strbuf.str(), __logInfo);
}

CLogHelper& CLogHelper::Format()
{
    return *this;
}

CLogHelper& CLogHelper::Format(const wchar_t * _Format, ...)
{
    va_list  marker = NULL;  
    va_start(marker, _Format);

    std::wstring text(_vscwprintf(_Format, marker) + 1, 0);
    vswprintf_s(&text[0], text.capacity(), _Format, marker);
    va_end(marker);

    __strbuf << text.data();
    return *this;
}

CLogHelper& CLogHelper::Format(const char * _Format, ...)
{
    va_list  marker = NULL;  
    va_start(marker, _Format);

    std::string text(_vscprintf(_Format, marker) + 1, 0);
    vsprintf_s(&text[0], text.capacity(), _Format, marker);
    va_end(marker);

    __strbuf << Conver::Str2WStr(text.data());
    return *this;
}

CLogHelper& CLogHelper::operator<<(CLogHelper&(__cdecl* pfn)(CLogHelper &))
{
    return ((*pfn)(*this));
}

CLogHelper& CLogHelper::operator<<(const std::string& info)
{
    __strbuf << Conver::Str2WStr(info);
    return *this;
}

}// namespace nlog