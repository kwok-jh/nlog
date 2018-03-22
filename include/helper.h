#pragma once

#include <windows.h>
#include <stdint.h>
#include <tchar.h>
#include <stdarg.h>
#include <assert.h>

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
        return path.substr(path.rfind(_T("\\")) + 1, tstring::npos);
    }

    static tstring GetDateTime(const tstring& format)
    {
        return (LPCTSTR)CTime::GetCurrentTime().Format(format.c_str());
    }

    static tstring GetModulePath(HMODULE hModule = 0)
    {
        tstring modulePath(MAX_PATH, 0);
        ::GetModuleFileName(hModule, (TCHAR*)modulePath.data(), modulePath.size());

        return GetPath(modulePath);
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

    static bool FilePathIsExist(const tstring& fileName, bool is_directory)
    {
        const DWORD file_attr = ::GetFileAttributes(fileName.c_str());
        if (file_attr != INVALID_FILE_ATTRIBUTES)
        {
            if (is_directory)
                return (file_attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
            else
                return true;
        }
        return false;
    }

    static bool IsLittleEndian()
    {
        uint16_t buf16 = 0x00ff;
        uint8_t* buf8 = (uint8_t*)&buf16;
        return *buf8 == 0xff;
    }
};