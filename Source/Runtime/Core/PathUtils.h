#pragma once

#include "Core/Encoding.h"
#include <filesystem>

namespace PathUtils
{
    // Windows 파일명 금지 문자와 제어 문자를 '_'로 치환한다.
    inline FString MakeSafeFileName(std::string_view Name)
    {
        FString Result(Name);
        for (char& Character : Result)
        {
            if (static_cast<unsigned char>(Character) < 32 || std::string_view("<>:\"/\\|?*").find(Character) != std::string_view::npos)
            {
                Character = '_';
            }
        }
        return Result;
    }

    // 엔진 UTF-8 경로의 Windows native 경로 변환
    inline std::filesystem::path FromUtf8(std::string_view Path)
    {
        return std::filesystem::path(Encoding::Utf8ToWide(Path));
    }

    // native 경로의 UTF-8 및 '/' 구분자 변환
    inline FString ToUtf8(const std::filesystem::path& Path)
    {
        return Encoding::WideToUtf8(Path.generic_wstring());
    }
}   

