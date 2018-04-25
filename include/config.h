#pragma once

#include <stdint.h>
#include <tchar.h>

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

typedef std::string         bytedata;


//////////////////////////////////////////////////////////////////////////
#include "strconvert.hpp"

inline tstring totstr(const std::string& str)
{
#ifdef UNICODE
    const tstring& tstr = Conver::Str2WStr(str);
#else
    const tstring& tstr = str;
#endif

#if _MSC_VER >= 1600
    return std::forward<const tstring&>(tstr);
#else
    return tstr;
#endif
}

inline tstring totstr(const std::wstring& str)
{
#ifdef UNICODE
    const tstring& tstr = str;
#else
    const tstring& tstr = Conver::WStr2Str(str);
#endif

#if _MSC_VER >= 1600
    return std::forward<const tstring&>(tstr);
#else
    return tstr;
#endif
}