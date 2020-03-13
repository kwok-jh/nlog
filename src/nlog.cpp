#include "nlog.h"
#include "iocp.hpp"
#include "simple_lock.hpp"

#include <atltime.h>
#include <vector>
#include <algorithm>
#include <functional>

namespace nlog {
namespace detail {

/*
*   If return true that the file or directory is exists.
*   Otherwise return false.
*/
inline bool FileExists(const std::wstring& file)
{
    return ::GetFileAttributesW(file.c_str()) != INVALID_FILE_ATTRIBUTES;
}

/*
*   Return the parent directory of the specified path.
*/
inline std::wstring FindParentDirectory(const std::wstring& path)
{
    size_t pos  = path.npos;
    size_t pos1 = path.rfind('\\');
    size_t pos2 = path.rfind('/');

    if(pos1 == path.npos)
        pos = pos2;
    else if(pos2 == path.npos)
        pos = pos1;
    else
        pos = std::max(pos1, pos2);

    return path.substr(0, pos);
}

/*
*   Create directories recursively
*/
inline bool CreateDirectories(const std::wstring& path)
{
    if(::GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES)
        return true;

    std::wstring pdir = path;
    if(!FileExists(pdir = FindParentDirectory(pdir)))
        CreateDirectories(pdir);

    if(::CreateDirectoryW(path.c_str(), NULL) == 0)
    {
        switch(::GetLastError())
        {
        case ERROR_ALREADY_EXISTS:
            return true;

        default:
            return false;
        }
    }

    return true;
}

/*
*   Return a copy of the the filename.
*   etc. log.txt -> log(1).txt
*                   log(1).txt -> log(2).txt
*/
inline std::wstring FileNameIncrement(const std::wstring& file)
{
    size_t dotPos = file.rfind(L".");
    size_t leftPos = file.rfind(L"(");
    size_t rightPos = file.rfind(L")");

    std::wstring copy = file;

    if(leftPos == file.npos || rightPos == file.npos ||
        leftPos > rightPos)
    {
        if(dotPos == file.npos)
            return copy + L"(1)";

        return copy.insert(dotPos, L"(1)");
    }

    ++leftPos;
    int number = 1;

    std::wstring index = file.substr(leftPos, rightPos - leftPos);
    if(!index.empty())
    {
        wchar_t * end;
        number = wcstol(index.c_str(), &end, 10);
        ++number;
    }

    return copy.replace(leftPos, rightPos - leftPos, ToWString(number));
}

/*
*   1. Format string
*   2. Replace string
*   3. Converts a multi-byte string to a wide-byte string.
*/
inline std::wstring StringFormat(const wchar_t * format, ...) 
{  
    va_list marker = NULL;  
    va_start(marker, format);

    std::wstring text(_vsctprintf(format, marker) + 1, 0);
    _vstprintf_s(&text[0], text.capacity(), format, marker);
    va_end(marker);

    return text.data();
}

inline std::wstring& StringReplace(
          std::wstring& target,
    const std::wstring& before, 
    const std::wstring& after)
{  
    std::wstring::size_type beforeLen = before.length();  
    std::wstring::size_type afterLen  = after.length();  
    std::wstring::size_type pos       = target.find(before, 0);

    while( pos != std::wstring::npos ) 
    {  
        target.replace(pos, beforeLen, after);  
        pos = target.find(before, (pos + afterLen));  
    }

    return target;
}

inline std::wstring StringToWString(const std::string& str)
{
    int len = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);

    std::wstring buff(len, 0);
    ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, (LPWSTR)buff.data(), len);

    if(buff[buff.size()-1] == '\0')
        buff.erase( buff.begin() + (buff.size()-1) );

    return buff;
}

} // detail

/*
*   CLogIO used for overlapping IO
*/
class CLogIO : public OVERLAPPED
{
    std::wstring  __strBuf;
public:
    CLogIO(const std::wstring& strBuf, const LARGE_INTEGER& offset)
    {
        // 初始化父OVERLAPPED
        memset(this, 0, sizeof(OVERLAPPED));

        __strBuf = strBuf;
        this->Offset = offset.LowPart;
        this->OffsetHigh = offset.HighPart;
    }

    int Size() const 
    {
        return static_cast<int>(
            __strBuf.length() * sizeof(std::wstring::value_type));
    }

    const wchar_t* szBuf() const
    {
        return __strBuf.c_str();
    }
};

/*
*   设置静态成员对象的初始化顺序
*/
#ifdef _MSC_VER
#pragma warning ( push )
#pragma warning ( disable : 4073 ) 
#pragma init_seg( lib )
#pragma warning ( pop )
#endif

/*
*   CLog static member
*/
std::map<std::string, CLog*>   CLog::__Instances;
std::auto_ptr<CSimpleLock>     CLog::__pLock(new CSimpleLock);

CLog& 
CLog::Instance( const std::string& guid /*= ""*/ )
{
    CAutoLock lock(*__pLock);

    std::map<std::string, CLog*>::iterator it = __Instances.find(guid);
    if(it != __Instances.end())
        return *it->second;
    else
        return *(__Instances[guid] = new CLog());
}

bool 
CLog::Release( const std::string& guid /*= ""*/ )
{
    CLog* _this = 0;
    {
        CAutoLock lock(*__pLock);

        std::map<std::string, CLog*>::iterator i = __Instances.find(guid);
        if(i == __Instances.end())
            return false;

        _this = i->second;
        __Instances.erase(i);
    }

    if(_this != NULL)
        _this->UinitLog();

    return true;
}

bool 
CLog::ReleaseAll()
{
    CAutoLock lock(*__pLock);

    std::map<std::string, CLog*>::iterator i = __Instances.begin();
    while(i != __Instances.end())
    {
        i->second->UinitLog();
        i = __Instances.erase(i);
    }

    return true;
}

/*
*   CLog member function
*/
CLog::CLog()
    : __pIocp(new CIOCP())
    , __count(0)
    , __hFile(INVALID_HANDLE_VALUE)
    , __initFlag(E_UNINIT)
    , __filterLevel(LV_PRO)
{
    __liNextOffset.QuadPart = 0;

    Config cfg = {};
    SetConfig(cfg);
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
        __config.logDir = L"{module_dir}\\log\\";

    if(setting.fileName.empty())
        __config.fileName = L"log-%Y%m%d.log";

    if(setting.dateFormat.empty())
        __config.dateFormat = L"%m-%d %H:%M:%S";

    if(setting.prefixion.empty())
    {
        /*[{time}] [{id}] [{level}] [{file}:{line}]*/
        __config.prefixion = L"[{time}][{level}][{id}]: ";
    }

    if(setting.maxFileNumber == 0)
        __config.maxFileNumber = -1;

    if(setting.maxFileSize == 0)
        __config.maxFileSize = -1;

    if(0 < __config.maxFileSize && __config.maxFileSize < 0x100000)
        __config.maxFileSize = 0x100000;

    return true;
}

nlog::Config 
CLog::GetConfig() const
{
    return __config;
}

void
CLog::SetLevel( LogLevel level )
{
    __filterLevel = level;
}

LogLevel 
CLog::GetLevel() const
{
    return __filterLevel;
}

bool 
CLog::InitLog()
{
    if(__initFlag != E_UNINIT)
        return false;
 
    std::wstring dirPath = Format(__config.logDir);
    std::wstring fileName = Format(__config.fileName);

    /*
    *   检查目录是否存在
    *   若不则创建之
    *   否则尝试执行目录清理流程
    */
    if(!detail::FileExists(dirPath))
    {
        if(!detail::CreateDirectories(dirPath))
        {
            __initFlag = E_FAILED_INIT;
            return false; //文件夹创建失败
        }
    }
    else
        CleanStoreDir();

    int  nTryCount = 50;
    bool bFileExist = false;
    do 
    {
        bFileExist = detail::FileExists(dirPath + L"\\" + fileName);

        __hFile = ::CreateFileW( 
            (dirPath + L"\\" + fileName).c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ,     //共享读取
            NULL,
            OPEN_ALWAYS,         //打开文件若不存在则创建
            FILE_FLAG_OVERLAPPED,//使用重叠IO
            0);

        if(__hFile != INVALID_HANDLE_VALUE)
        {
            ::GetFileSizeEx(__hFile, &__liNextOffset);
            if(__config.maxFileSize != -1 && 
               __config.maxFileSize <= __liNextOffset.QuadPart)
            {
                /*
                *   若文件大小超出配置指定的值, 那么跳过此文件尝试创建新文件.
                */
                ::CloseHandle(__hFile);
                              __hFile = INVALID_HANDLE_VALUE;
            }
            else
                break;
        }

        if(__hFile == INVALID_HANDLE_VALUE)
        {
            if(--nTryCount > 0)
            {
                fileName = detail::FileNameIncrement(fileName);
            }
            else
            {
                __initFlag = E_FAILED_INIT;
                return false;
            }
        }
    }
    while(__hFile == INVALID_HANDLE_VALUE);

    //创建并关联iocp
    __pIocp->Create(1);
    __pIocp->AssociateDevice(__hFile, 0 );
    __count = 0;

    if(!bFileExist) 
    {
#ifdef UNICODE
        /*如果是Unicode则写入BOM, 文件头LE:0xfffe BE:0xfeff*/
        WriteLog(std::wstring(1, 0xfeff));
#endif
    }

    __initFlag = E_ALREADY_INIT;
    return true;
}

void 
CLog::UinitLog()
{
    //首先尝试释放已经完成的资源, 给予200ms等待时间
    for(int i = 0; !CompleteHandle() && i < 10; ++i)
    {
        ::Sleep(20);
    }

    //关闭完成端口, 文件, 迫使回收未完成的资源 
    __pIocp->Close();

    if(__hFile != INVALID_HANDLE_VALUE) 
    {
        ::CloseHandle(__hFile);
                      __hFile = INVALID_HANDLE_VALUE;
    }

    CompleteHandle(true);

    __initFlag = E_UNINIT;
}

void 
CLog::CleanStoreDir()
{
    if(__config.maxFileNumber == -1 && __config.prefixMatch.empty())
        return;

    std::wstring parentDir = Format(__config.logDir);

    std::multimap<long long, std::wstring> logFiles;
    {
        WIN32_FIND_DATAW file_data;
        HANDLE fd = ::FindFirstFileW((parentDir + L"\\*").c_str(), &file_data);

        while(fd != INVALID_HANDLE_VALUE)
        {
            if(file_data.cFileName[0] != L'.')
            {
                if(!(file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    std::wstring targetFile = file_data.cFileName;
                    if(targetFile.find(__config.prefixMatch) !=  targetFile.npos)
                    {
                        logFiles.insert(std::make_pair(
                            long long(file_data.ftCreationTime.dwHighDateTime) << 32 | 
                            long long(file_data.ftCreationTime.dwLowDateTime),
                            targetFile));
                    }
                }
            }

            if(::FindNextFileW(fd, &file_data) == FALSE)
                break;
        }
    }

    // 因稍后即将创建一个文件, 故这里的最大文件数量应该减一
    size_t maxCount = __config.maxFileNumber - 1;
    if(logFiles.size() > maxCount)
    {
        for (size_t i = 0; i < maxCount; ++i)
        {
            logFiles.erase((++logFiles.rbegin()).base());
        }
        
        for(std::multimap<long long, std::wstring>::const_iterator it = logFiles.begin();
            it != logFiles.end(); ++it)
        {
            ::DeleteFileW((parentDir + L"\\" + it->second).c_str());
        }
    }
}

bool 
CLog::CompleteHandle( bool bClose /*= false*/ )
{
    DWORD       dwNumBytes = 0;
    ULONG_PTR   compKey    = 0;
    CLogIO*     pIo        = 0;

    /*
    *   在某个特殊的时间终止程序, 投递的事件不能全部返回????
    *   这里临时设置一个重试次数, 大约2秒钟左右
    */
    int tryCount = 200;
    do
    {
        /* get已完成的状态，如果pio不为null那么就将其释放 */
        __pIocp->GetStatus( &dwNumBytes, &compKey, (OVERLAPPED**)&pIo, 0 );

        if( NULL != pIo ) 
        { 
            ::InterlockedDecrement((LONG*)&__count);

            delete pIo; 
                   pIo = NULL; 
        }
        else if( !bClose )
            break;

        if( bClose )
        {
            if( 0 == __count )
                break;
            else
                --tryCount;

            ::Sleep(10);
        }
    }
    while(tryCount > 0);

    return 0 == __count;
}

std::wstring 
CLog::Format( const std::wstring& strBuf, const LogContext& context /*= LogContext()*/ )
{
    /*
    *   这里还存在大量的性能浪费, 后面可以专门做优化
    */
    std::wstring result  = (LPCTSTR)CTime::GetCurrentTime().Format(strBuf.c_str());
    std::wstring strTime = (LPCTSTR)CTime::GetCurrentTime().Format(__config.dateFormat.c_str());
    std::wstring strId   = detail::StringFormat(L"%- 8X", ::GetCurrentThreadId());
    std::wstring strLine = detail::StringFormat(L"%- 4d", context.line);
    
    std::wstring strLevel;
    switch(context.level) 
    {
    case LV_ERR: strLevel = L"ERR"; break;
    case LV_WAR: strLevel = L"WAR"; break;
    case LV_APP: strLevel = L"APP"; break;
    case LV_PRO: strLevel = L"PRO"; break;
    }

    std::wstring strModule;
    {
        wchar_t buffer[MAX_PATH] = {0};
        ::GetModuleFileNameW(0, buffer, sizeof buffer);

        if(wchar_t *fname = wcsrchr(buffer, L'\\'))
            *fname = L'\0';

        strModule = buffer;
    }

    result = detail::StringReplace(result, L"{module_dir}",strModule);
    result = detail::StringReplace(result, L"{level}", strLevel);
    result = detail::StringReplace(result, L"{time}", strTime);
    result = detail::StringReplace(result, L"{id}", strId);
    result = detail::StringReplace(result, L"{file}", context.file);
    result = detail::StringReplace(result, L"{func}", context.func);
    result = detail::StringReplace(result, L"{line}", strLine);
    
    return result;
}

CLog& 
CLog::FormatWriteLog( const std::wstring& strBuf, const LogContext& context /*= LogContext()*/ )
{
    if(__filterLevel >= context.level) 
    {
        /*
        *   1. 校验文件大小是否越界
        *   2. 校验文件是否初始化
        */
        {
            CAutoLock lock(*__pLock);

            if(__config.maxFileSize != -1 && __config.maxFileSize <= __liNextOffset.QuadPart)
                UinitLog();

            switch(__initFlag)
            {
            case E_UNINIT:
                if(!InitLog())
                    return *this;   // 如果初始化失败了, 那么什么也不做, 至少应该保证程序运行
                break;

            case E_FAILED_INIT:
                return *this;
            }
        }

        return WriteLog(Format(__config.prefixion, context) + strBuf + L"\r\n");
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
        CAutoLock lock(*__pLock);
                                   pIo = new CLogIO(strBuf, __liNextOffset);
        __liNextOffset.QuadPart += pIo->Size();
    }

    /* 投递重叠IO */
    ::WriteFile( __hFile, pIo->szBuf(), pIo->Size(), NULL, pIo );
    ::InterlockedIncrement((LONG*)&__count);

    if(GetLastError() == ERROR_IO_PENDING) 
    {
        /*
        *   MSDN:The error code WSA_IO_PENDING indicates that the overlapped operation 
        *   has been successfully initiated and that completion will be indicated at a 
        *   later time.
        */
        SetLastError(S_OK);
    }
    return *this;
}

/*
*   CLogHelper firend function
*/
CLogHelper& 
time(CLogHelper& slef, bool append)
{
    if(append)
    {
        return slef << (LPCTSTR)CTime::GetCurrentTime().Format(
            slef.__log.__config.dateFormat.c_str());
    }
    else
    {
        return slef % (LPCTSTR)CTime::GetCurrentTime().Format(
            slef.__log.__config.dateFormat.c_str());
    }
}

CLogHelper& 
id(CLogHelper& slef, bool append)
{
    if(append)
        return slef << detail::StringFormat(L"%- 8X", ::GetCurrentThreadId());
    else
        return slef % detail::StringFormat(L"%- 8X", ::GetCurrentThreadId());
}

CLogHelper& 
d_out(CLogHelper& slef, bool append)
{
    return slef.DebugOutput();
}

CLogHelper::CLogHelper(LogLevel level, const char* file, const unsigned int line, 
                                       const char* func, const std::string& guid /*= ""*/) 
    : __log(CLog::Instance(guid))
    , __argIndex(0)
{
    char* dfunc = (char *)strrchr(func, ':');
    char* dfile = (char *)file;

    while (*dfile++);
    while (--dfile != file && *dfile != char('\\') && *dfile != char('/'));

    __logInfo.file  = detail::StringToWString(dfile ? dfile + 1 : "");
    __logInfo.func  = detail::StringToWString(dfunc ? dfunc + 1 : "");
    __logInfo.line  = line;
    __logInfo.level = level;
}

CLogHelper::~CLogHelper()
{
    __log.FormatWriteLog(__strbuf, __logInfo);
}

CLogHelper& 
CLogHelper::Format()
{
    return *this;
}

CLogHelper& 
CLogHelper::Format(const wchar_t * format, ...)
{
    va_list ap = 0;  
    va_start(ap, format);
    Format_(format, ap);
    va_end(ap);

    return (*this);
}

CLogHelper& 
CLogHelper::Format(const char * format, ...)
{
    va_list ap = 0;  
    va_start(ap, format);
    Format_(format, ap);
    va_end(ap);

    return (*this);
}

CLogHelper& 
CLogHelper::Format_(const wchar_t * format, va_list ap)
{
    std::wstring text;
                 text.resize(_vscwprintf(format, ap) + 1);
    vswprintf_s(&text[0], text.capacity(), format, ap);
    return (*this) << text.data();
}

CLogHelper& 
CLogHelper::Format_(const char * format, va_list ap)
{
    std::string text;
                text.resize(_vscprintf(format, ap) + 1);
    vsprintf_s(&text[0], text.capacity(), format, ap);
    return (*this) << detail::StringToWString(text.data());
}

CLogHelper& 
CLogHelper::DebugOutput()
{
    ::OutputDebugStringW(__strbuf.c_str());
    ::OutputDebugStringW(L"\r\n");

    return *this;
}

CLogHelper& 
CLogHelper::operator<<(CLogHelper&(__cdecl* pfn)(CLogHelper &, bool))
{
    return ((*pfn)(*this, true));
}

CLogHelper& 
CLogHelper::operator<<(const std::string& info)
{
    return (*this) << detail::StringToWString(info);
}

CLogHelper& 
CLogHelper::operator<<(const std::wstring& info)
{
    __strbuf += info;
    return *this;
}

CLogHelper& 
CLogHelper::operator%(CLogHelper&(__cdecl* pfn)(CLogHelper &, bool))
{
    return ((*pfn)(*this, false));
}

CLogHelper& 
CLogHelper::operator%(const std::string& arg)
{
    return (*this) % detail::StringToWString(arg);
}

CLogHelper& 
CLogHelper::operator%(const std::wstring& arg)
{
    std::wstring specifier(5, 0);
    swprintf_s(&specifier[0], specifier.capacity(), L"{%d}", ++__argIndex);
    
    detail::StringReplace(__strbuf, specifier.c_str(), arg);
    return *this;
}

}// namespace nlog