#pragma once

#include <windows.h>
#include <stdint.h>
#include <tchar.h>
#include <stdarg.h>
#include <assert.h>

#include <list>
#include <vector>
#include <string>
#include <sstream> 
#include <fstream>

#ifdef UNICODE
typedef std::wstring        tstring; 
typedef std::wstringstream  tsstream;
#else
typedef std::string         tstring;
typedef std::stringstream   tsstream;
#endif

#include <shlobj.h>     // SHCreateDirectory
#include <shellapi.h>   // SHFileOperation
#include <atltime.h>    // CTime

class helper
{
public:
    static tstring StrFormatVar(const TCHAR * _Format, va_list marker)
    {
        tstring result(_vsctprintf(_Format, marker) + 1, 0);
        _vstprintf_s((TCHAR *)result.data(), result.capacity(), _Format, marker); 
        return result.data();
    }

    static tstring StrFormat(const TCHAR * _Format, ...) 
    {  
        va_list marker = NULL;  
        va_start(marker, _Format);
        return StrFormatVar(_Format, marker);
    }

    static tstring& StrReplace(tstring& target, const tstring& before, const tstring& after)  
    {  
        tstring::size_type pos = 0;  
        tstring::size_type beforeLen = before.length();  
        tstring::size_type afterLen  = after.length();  

        pos = target.find(before, pos);   
        while( pos != tstring::npos )  
        {  
            target.replace(pos, beforeLen, after);  
            pos = target.find(before, (pos + afterLen));  
        }

        return target;
    }  

    static tstring GetPath(const tstring& fileName)
    {
        return fileName.substr(0, fileName.rfind(_T("\\")));
    }

    static tstring GetName(const tstring& path)
    {
        size_t pos = path.length() - 1;
        while( pos >= 0 && path.at(pos) == _T('\\') )
            --pos;

        size_t findPos = path.rfind(_T("\\"), pos);
        return path.substr(findPos + 1, pos - findPos);
    }

    static tstring GetDateTime(const tstring& format)
    {
        /*
            格式: %Y(%y) %m %d %H %M %S 分别是 年 月 日 时 分 秒
            see: https://msdn.microsoft.com/zh-tw/library/fe06s4ak.aspx
        */
       return (LPCTSTR)CTime::GetCurrentTime().Format(format.c_str());
    }

    static tstring GetDateTime(const tstring& format, FILETIME ft)
    {
        return (LPCTSTR)CTime::CTime(ft).Format(format.c_str());
    }

    static tstring GetModuleName(HMODULE hModule = 0)
    {
        TCHAR buffer[MAX_PATH];
        ::GetModuleFileName(hModule, buffer, sizeof buffer);

        return buffer;
    }

    static tstring GetModulePath(HMODULE hModule = 0)
    {
        return GetPath(GetModuleName());
    }

    static tstring GetSysDir()
    {
        TCHAR buffer[MAX_PATH];
        ::GetSystemDirectory(buffer, sizeof buffer);

        return buffer;
    }

    static bool CreateDirRecursively(const tstring& dir)
    {
        HRESULT result = ::SHCreateDirectoryEx(NULL, dir.c_str(), 0);
        return (result == ERROR_SUCCESS || result == ERROR_FILE_EXISTS 
            || result == ERROR_ALREADY_EXISTS);
    }

    static bool DeleteDirRecursively(const tstring& dir)
    {
        DWORD attributes = ::GetFileAttributes(dir.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) // not exists
            return true;

        if (!(attributes & FILE_ATTRIBUTE_DIRECTORY)) // not a file
            return false;

        SHFILEOPSTRUCT file_op = {
            NULL,
            FO_DELETE,
            dir.c_str(),
            _T(""),
            FOF_NOCONFIRMATION|FOF_NOERRORUI|FOF_SILENT,
            FALSE,
            0,
            _T("")};

            if (::SHFileOperation(&file_op) && !file_op.fAnyOperationsAborted)
                return true;

            return false;
    }

    static bool DeleteFileOrDir(const tstring& path)
    {
        DWORD attributes = ::GetFileAttributes(path.c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES) // not exists
            return true;

        SHFILEOPSTRUCT file_op = {
            NULL,
            FO_DELETE,
            path.c_str(),
            _T(""),
            FOF_NOCONFIRMATION|FOF_NOERRORUI|FOF_SILENT,
            FALSE,
            0,
            _T("")};

            if (::SHFileOperation(&file_op) && !file_op.fAnyOperationsAborted)
                return true;

            return false;
    }

    static bool FilePathIsExist(const tstring& fileName, bool is_directory = true)
    {
        DWORD file_attr = ::GetFileAttributes(fileName.c_str());
        if (file_attr != INVALID_FILE_ATTRIBUTES)
        {
            if (is_directory)
                return (file_attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
            else
                return true;
        }
        return false;
    }

    static std::list<tstring> EnumDir(const tstring& dir, int type = -1, const tstring& filter = _T("*.*"),
        bool recur = true)
    {
        std::list<tstring> resultList;

        WIN32_FIND_DATA file_data;
        tstring path = dir + _T("\\") + filter;

        HANDLE hFile = ::FindFirstFile(path.c_str(), &file_data);
        while( hFile != INVALID_HANDLE_VALUE )
        {
            if( file_data.cFileName[0] != _T('.') )
            {
                switch(type)
                {
                    /*所有文件*/
                case -1:
                    break;

                    /*仅文件*/
                case 0:
                    if( file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
                    {
                        continue;
                    }
                    break;

                    /*仅文件夹*/
                case 1:
                    if( file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
                    {
                        break;
                    }
                    continue;
                }

                tstring targetName = dir + _T("\\") + file_data.cFileName;
                resultList.push_back(targetName);

                /*递归目录*/
                if( recur && file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
                {
                    resultList.merge( EnumDir(targetName, type, filter, recur) );
                }
            }

            if( ::FindNextFile(hFile, &file_data) == FALSE )
            {
                break;
            }
        }

        ::FindClose(hFile);
        return resultList;
    }

    static uint64_t GetFileSize(const tstring& fileName)
    {
        HANDLE hFile = CreateFile( 
            fileName.c_str(),           // lpFileName
            FILE_READ_ATTRIBUTES,       // dwDesiredAccess
            FILE_SHARE_READ,            // dwShareMode
            NULL,                       // lpSecurityAttributes
            OPEN_EXISTING,              // dwCreationDisposition
            FILE_FLAG_BACKUP_SEMANTICS, // dwFlagsAndAttributes
            NULL );                     // hTemplateFile

        if(hFile != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER large;
            if(::GetFileSizeEx(hFile, &large) != 0)
            {
                CloseHandle (hFile);
                return large.QuadPart;
            }
        }

        //failed GetLastError
        CloseHandle (hFile);
        return 0;
    }

    static std::vector<FILETIME> GetFileTime(const tstring& fileName)
    {
        HANDLE hFile = CreateFile( 
            fileName.c_str(),           // lpFileName
            GENERIC_READ ,              // dwDesiredAccess
            FILE_SHARE_READ,            // dwShareMode
            NULL,                       // lpSecurityAttributes
            OPEN_EXISTING,              // dwCreationDisposition
            FILE_FLAG_BACKUP_SEMANTICS, // dwFlagsAndAttributes
            NULL );                     // hTemplateFile

        std::vector<FILETIME> timeVec(3);

        if(hFile != INVALID_HANDLE_VALUE)
        {
            if( ::GetFileTime(hFile, 
                &timeVec[0],           //CreationTime 
                &timeVec[1],           //LastAccessTime
                &timeVec[2] )          //LastWriteTime
                != 0 )
            {
                CloseHandle (hFile);
                return timeVec;
            }
        }

        //failed
        ::CloseHandle (hFile);
        return timeVec;
    }

    static bool IsLittleEndian()
    {
        uint16_t buf16 = 0x00ff;
        uint8_t* buf8 = (uint8_t*)&buf16;

        return *buf8 == 0xff;
    }

    static bool IsWow64()
    {
        static bool bIsRead  = false;
        static BOOL bIsWow64 = false;

        if(!bIsRead)
        {
            //32位进程运行在64位系统下返回true
            typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
            LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
                GetModuleHandle(TEXT("kernel32")),"IsWow64Process");

            if(NULL != fnIsWow64Process)
            {
                if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
                {
                    //handle error
                    assert(0);
                }
            }

            bIsRead = true;
        }

        return !!bIsWow64;
    }

    static bool Is64BitWindows()
    {
#if defined(_WIN64)

        return true; // 64-bit programs run only on Win64

#else // _WIN32

        // 32-bit programs run on both 32-bit and 64-bit Windows
        // so must sniff
        return IsWow64();
#endif
    }

};