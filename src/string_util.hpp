#pragma once

inline std::wstring StrFormat(const wchar_t * format, ...) 
{  
    va_list marker = NULL;  
    va_start(marker, format);

    std::wstring text(_vsctprintf(format, marker) + 1, 0);
    _vstprintf_s(&text[0], text.capacity(), format, marker);
    va_end(marker);

    return text.data();
}

inline std::wstring& StrReplace(std::wstring& target, const std::wstring& before, 
    const std::wstring& after)
{  
    std::wstring::size_type beforeLen = before.length();  
    std::wstring::size_type afterLen  = after.length();  
    std::wstring::size_type pos       = target.find(before, pos);

    while( pos != std::wstring::npos )  
    {  
        target.replace(pos, beforeLen, after);  
        pos = target.find(before, (pos + afterLen));  
    }

    return target;
}