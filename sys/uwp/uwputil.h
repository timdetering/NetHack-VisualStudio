#pragma once

#include <assert.h>
#include <string>

namespace brh
{
    inline std::string to_string(const std::wstring & inString)
    {
        std::string outString;
        outString.assign(inString.begin(), inString.end());
        return outString;
    }

    inline void to_wstring(const std::string & inString, std::wstring & outString)
    {
        outString.assign(inString.begin(), inString.end());
    }

    inline std::wstring to_wstring(const std::string & inString)
    {
        std::wstring outString;
        outString.assign(inString.begin(), inString.end());
        return outString;
    }

    inline void to_string(const std::wstring & inString, std::string & outString)
    {
        outString.assign(inString.begin(), inString.end());
    }

}

namespace Nethack
{

    inline Platform::String ^ to_string(const std::string & inString)
    {
        std::wstring string;
        string.assign(inString.begin(), inString.end());
        return ref new Platform::String(string.c_str());
    }

    inline Platform::String ^ to_string(const std::wstring & inString)
    {
        return ref new Platform::String(inString.c_str());
    }

    inline std::string to_string(Platform::String ^ inString)
    {
        std::wstring string(inString->Data());
        std::string outString;
        outString.assign(string.begin(), string.end());
        return outString;
    }

    inline std::wstring to_wstring(Platform::String ^ inString)
    {
        return std::wstring(inString->Data());
    }

}

