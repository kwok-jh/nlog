#include "../include/nlog.h"
#include "../include/iocp.hpp"

namespace nlog{

//继承重叠机构的IO结构，用于投递异步写入重叠操作
class CLogIO : public OVERLAPPED
{
    tstring  __strBuf;

public:
    CLogIO()
    { 
        Init();
    }

    CLogIO(const tstring& strBuf, const LARGE_INTEGER& offset)
    {
        Init();

        __strBuf = strBuf;
        this->Offset = offset.LowPart;
        this->OffsetHigh = offset.HighPart;
    }

    size_t Size() const
    {
        return __strBuf.length() * sizeof(tstring::value_type);
    }

    const TCHAR* szBuf()
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
std::map<uint32_t, CLog*> CLog::__sMapInstance;
CSimpleLock               CLog::__mapCsec;

CLog& 
CLog::Instance( uint32_t id /*= -1*/ )
{
    CAutolock lock(&__mapCsec);

    std::map<uint32_t, CLog*>::iterator i = __sMapInstance.find(id);
    if(i == __sMapInstance.end())
    {
        __sMapInstance[id] = new CLog();
    }

    return *__sMapInstance[id];
}

bool 
CLog::Release( uint32_t id /*= -1*/ )
{
    CLog* _this = 0;
    {
        CAutolock lock(&__mapCsec);

        std::map<uint32_t, CLog*>::iterator i = __sMapInstance.find(id);
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
    CAutolock lock(&__mapCsec);

    std::map<uint32_t, CLog*>::iterator i = __sMapInstance.begin();
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
        __config.logDir = helper::GetModulePath() + _T("\\log\\");

    if(setting.fileName.empty())
        __config.fileName = helper::GetDateTime(_T("log-%m%d-%H%M.log"));

    if(setting.dateFormat.empty())
        __config.dateFormat = _T("%m-%d %H:%M:%S");

    if(setting.prefixion.empty())
        /*[{time}] [{id}] [{level}] [{file}:{line}]*/
        __config.prefixion = _T("[{time}][{level}][{id}]: ");

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
    CAutolock lock(&__mapCsec);

    if(__bAlreadyInit)
    {
        return false;
    }
    
    //////////////////////////////////////////////////////////////////////////
    tstring fileName   = __config.logDir + _T("\\") + Format(__config.fileName);
    bool bFileExist = false;
    do
    {
        bFileExist = helper::FilePathIsExist(fileName, false);

        helper::CreateDirRecursively(__config.logDir);
        __hFile = CreateFile( 
            fileName.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ,     //共享读取
            NULL,
            OPEN_ALWAYS,         //打开文件若不存在则创建
            FILE_FLAG_OVERLAPPED,//使用重叠IO
            0);

        if(__hFile == INVALID_HANDLE_VALUE)
        {
            fileName.insert(fileName.rfind(_T(".")), _T("_1"));
        }

    }while( __hFile == INVALID_HANDLE_VALUE );

    ::GetFileSizeEx(__hFile, &__liNextOffset);
    __pIocp->AssociateDevice(__hFile, 0 );

    if(!bFileExist)
    {
#ifdef UNICODE
        /*如果是Unicode则写入BOM, 文件头LE:0xfffe BE:0xfeff*/
        WriteLog(tstring(1, 0xfeff));
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
            ::InterlockedDecrement(&__count);

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

tstring 
CLog::Format( const tstring& text, const LogInfomation& info /*= Loginfomation()*/ )
{
    tstring result  = text;
    tstring strTime = helper::GetDateTime(__config.dateFormat);
    tstring strId   = helper::StrFormat(_T("%- 8X"), ::GetCurrentThreadId());
    tstring strLine = helper::StrFormat(_T("%- 4d"), info.line);
    tstring strLevel;

    switch(info.level)
    {
    case LV_ERR: strLevel = _T("ERR"); break;
    case LV_WAR: strLevel = _T("WAR"); break;
    case LV_APP: strLevel = _T("APP"); break;
    case LV_PRO: strLevel = _T("PRO"); break;
    }
    
    result = helper::StrReplace(result, _T("{level}"),strLevel);
    result = helper::StrReplace(result, _T("{time}"), strTime);
    result = helper::StrReplace(result, _T("{id}"),   strId);
    result = helper::StrReplace(result, _T("{file}"), info.file);
    result = helper::StrReplace(result, _T("{line}"), strLine);
    
    return result;
}

CLog& 
CLog::FormatWriteLog( const tstring& strBuf, const LogInfomation& info /*= Loginfomation()*/ )
{
    if(__filterLevel >= info.level)
    {
        if(!__bAlreadyInit)
        {
            InitLog();
        }

        return WriteLog(Format(__config.prefixion, info) + strBuf + _T("\r\n"));
    }
    else
        return *this;
}

CLog& 
CLog::WriteLog( const tstring& strBuf )
{
    CompleteHandle();

    CLogIO* pIo = NULL;
    {
        CAutolock lock(&__mapCsec);
        pIo = new CLogIO(strBuf, __liNextOffset);
        ::InterlockedExchangeAdd64(&__liNextOffset.QuadPart, pIo->Size());
    }

    //投递重叠IO
    ::WriteFile( __hFile, pIo->szBuf(), pIo->Size(), NULL, pIo );
    ::InterlockedIncrement(&__count);

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
    slef.__strbuf << helper::GetDateTime(
        CLog::Instance(slef.__sessionId).__config.dateFormat);
    return slef;
}

CLogHelper& 
id(CLogHelper& slef)
{
    slef.__strbuf << helper::StrFormat(_T("%- 8X"), ::GetCurrentThreadId());
    return slef;
}

}// namespace nlog