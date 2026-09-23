#include "NotifyIconStore.h"

#include <windows.h>

#include <cwchar>

namespace
{
constexpr wchar_t kIsPromoted[] = L"IsPromoted";

class UniqueHKey
{
public:
    UniqueHKey() = default;
    ~UniqueHKey() { reset(); }
    UniqueHKey(const UniqueHKey&) = delete;
    UniqueHKey& operator=(const UniqueHKey&) = delete;

    [[nodiscard]] HKEY get() const { return key_; }
    HKEY* put() { reset(); return &key_; }
    void reset()
    {
        if (key_)
            RegCloseKey(key_);
        key_ = nullptr;
    }

private:
    HKEY key_ = nullptr;
};

std::optional<std::wstring> ReadString(HKEY key, const wchar_t* name)
{
    constexpr DWORD flags = RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ | RRF_NOEXPAND;
    DWORD size = 0;
    if (RegGetValueW(key, nullptr, name, flags, nullptr, nullptr, &size) != ERROR_SUCCESS)
        return std::nullopt;

    std::wstring value(size / sizeof(wchar_t) + 1, L'\0');
    size = static_cast<DWORD>(value.size() * sizeof(wchar_t));
    if (RegGetValueW(key, nullptr, name, flags, nullptr, value.data(), &size) != ERROR_SUCCESS)
        return std::nullopt;

    value.resize(wcsnlen(value.c_str(), value.size()));
    return value;
}

std::optional<uint32_t> ReadDword(HKEY key, const wchar_t* name)
{
    DWORD value = 0;
    DWORD size = sizeof(value);
    if (RegGetValueW(key, nullptr, name, RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS)
        return std::nullopt;
    return value;
}
} // namespace

std::optional<std::vector<IconEntry>> ReadIconEntries()
{
    UniqueHKey root;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kNotifyIconSettingsKey, 0, KEY_READ, root.put()) != ERROR_SUCCESS)
        return std::nullopt;

    std::vector<IconEntry> entries;
    for (DWORD index = 0;; ++index)
    {
        wchar_t name[256];
        DWORD nameLength = ARRAYSIZE(name);
        FILETIME lastWrite{};
        LSTATUS status = RegEnumKeyExW(root.get(), index, name, &nameLength, nullptr, nullptr, nullptr, &lastWrite);
        if (status == ERROR_NO_MORE_ITEMS)
            break;
        if (status != ERROR_SUCCESS)
            continue;

        UniqueHKey sub;
        if (RegOpenKeyExW(root.get(), name, 0, KEY_QUERY_VALUE, sub.put()) != ERROR_SUCCESS)
            continue;

        IconEntry entry;
        entry.id = name;
        entry.executablePath = ReadString(sub.get(), L"ExecutablePath").value_or(L"");
        entry.iconGuid = ReadString(sub.get(), L"IconGuid");
        entry.uid = ReadDword(sub.get(), L"UID");
        entry.isPromoted = ReadDword(sub.get(), kIsPromoted);
        entry.lastWriteTime = (static_cast<uint64_t>(lastWrite.dwHighDateTime) << 32) | lastWrite.dwLowDateTime;
        entries.push_back(std::move(entry));
    }
    return entries;
}

WriteResult SetIsPromotedIfUnset(const std::wstring& id, uint32_t value)
{
    const std::wstring path = std::wstring(kNotifyIconSettingsKey) + L"\\" + id;

    // Open (never create): the entry may have been removed since it was read.
    UniqueHKey key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, path.c_str(), 0, KEY_QUERY_VALUE | KEY_SET_VALUE, key.put()) != ERROR_SUCCESS)
        return WriteResult::Failed;

    if (ReadDword(key.get(), kIsPromoted))
        return WriteResult::AlreadySet;

    const DWORD data = value;
    if (RegSetValueExW(key.get(), kIsPromoted, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&data), sizeof(data)) != ERROR_SUCCESS)
        return WriteResult::Failed;
    return WriteResult::Written;
}
