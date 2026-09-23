#include "PathNormalizer.h"

#include <cwctype>
#include <regex>

std::wstring NormalizeExecutablePath(std::wstring_view path)
{
    std::wstring lower;
    lower.reserve(path.size());
    for (wchar_t ch : path)
        lower.push_back(static_cast<wchar_t>(std::towlower(ch)));

    // Dotted version numbers ("1.272.438.0", "app-2.1.4", "gpu-z.2.70.0.exe") become "*".
    static const std::wregex versionToken(LR"(\d+(\.\d+)+)");
    return std::regex_replace(lower, versionToken, L"*");
}
