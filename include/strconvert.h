#pragma once

#include <windows.h>
#include <assert.h>
#include <string>

class Conver
{
public:
    static const uintptr_t Ansi2Unicode(const char* pscr, wchar_t* pdst)
    {
        assert(pscr);
        int len = MultiByteToWideChar(CP_ACP, 0, pscr, -1, NULL, 0);

        if( !pdst )
            return (uintptr_t)len;

        MultiByteToWideChar(CP_ACP, 0, pscr, -1, (LPWSTR)pdst, len);
        return (uintptr_t)pdst;
    }

    static const uintptr_t Unicode2Ansi(const wchar_t* pscr, char* pdst)
    {
        assert(pscr);
        int len = WideCharToMultiByte(CP_ACP, 0, pscr, -1, NULL, 
            0, NULL, NULL);

        if( !pdst )
            return (uintptr_t)len;
        WideCharToMultiByte(CP_ACP, 0, pscr, -1, pdst, len, NULL, NULL);
        return (uintptr_t)pdst;
    }

    static const uintptr_t Utf82Unicode(const char* pscr, wchar_t* pdst)
    {
        assert(pscr);
        int len = MultiByteToWideChar(CP_UTF8, 0, pscr, -1, NULL, 0);

        if( !pdst )
            return (uintptr_t)len;

        MultiByteToWideChar(CP_UTF8, 0, pscr, -1, (LPWSTR)pdst, len);
        return (uintptr_t)pdst;
    }

    static const uintptr_t Unicode2Utf8(const wchar_t* pscr, char* pdst)
    {
        assert(pscr);
        int len = WideCharToMultiByte(CP_UTF8, 0, pscr, -1, NULL, 
            0, NULL, NULL);

        if( !pdst )
            return (uintptr_t)len;
        WideCharToMultiByte(CP_UTF8, 0, pscr, -1, pdst, len, NULL, NULL);
        return (uintptr_t)pdst;
    }

    static std::wstring Str2WStr(const std::string& str)
    {
        size_t len = Ansi2Unicode(str.c_str(), 0);

        std::wstring buff(len, 0);
        Ansi2Unicode(str.c_str(), const_cast<wchar_t*>(buff.data()));

        if(buff[buff.size()-1] == '\0')
            buff.erase( buff.begin() + (buff.size()-1) );

        return buff;
    }

    static std::string WStr2Str(const std::wstring& str)
    {
        size_t len = Unicode2Ansi(str.c_str(), 0);

        std::string buff(len, 0);
        Unicode2Ansi(str.c_str(), const_cast<char*>(buff.data()));

        if(buff[buff.size()-1] == '\0')
            buff.erase( buff.begin() + (buff.size()-1) );

        return buff;
    }

    static std::wstring Utf82WStr(const std::string& str)
    {
        size_t len = Utf82Unicode(str.c_str(), 0);

        std::wstring buff(len, 0);
        Utf82Unicode(str.c_str(), const_cast<wchar_t*>(buff.data()));

        if(buff[buff.size()-1] == '\0')
            buff.erase( buff.begin() + (buff.size()-1) );

        return buff;
    }

    static std::string WStr2Utf8(const std::wstring& str)
    {
        size_t len = Unicode2Utf8(str.c_str(), 0);

        std::string buff(len, 0);
        Unicode2Utf8(str.c_str(), const_cast<char*>(buff.data()));

        if(buff[buff.size()-1] == '\0')
            buff.erase( buff.begin() + (buff.size()-1) );

        return buff;
    }

    static std::string Utf82Str(const std::string& str)
    {
        return WStr2Str( Utf82WStr(str) );
    }
};