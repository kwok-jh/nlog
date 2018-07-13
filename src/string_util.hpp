#ifndef string_util_h__
#define string_util_h__


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
    std::wstring::size_type pos       = target.find(before, 0);

    while( pos != std::wstring::npos )  
    {  
        target.replace(pos, beforeLen, after);  
        pos = target.find(before, (pos + afterLen));  
    }

    return target;
}

inline std::pair<std::wstring, std::wstring> 
    StrRightCarveWhit(const std::wstring& target, const std::wstring& substr)
{
    std::wstring::size_type index;
    if( (index = target.rfind(substr)) != std::wstring::npos )
    {
        return std::make_pair(target.substr(0, index), target.substr(index + 1));
    }

    return std::pair<std::wstring, std::wstring>();
}

#endif // string_util_h__