#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "Fixer.h"

// Registry access to HKCU\Control Panel\NotifyIconSettings.

inline constexpr wchar_t kNotifyIconSettingsKey[] = L"Control Panel\\NotifyIconSettings";

// Returns nullopt if the NotifyIconSettings key cannot be opened.
std::optional<std::vector<IconEntry>> ReadIconEntries();

enum class WriteResult
{
    Written,
    AlreadySet,   // the user (or Explorer) set a value in the meantime; left alone
    Failed,
};

// Writes IsPromoted only if the entry still has no IsPromoted value.
WriteResult SetIsPromotedIfUnset(const std::wstring& id, uint32_t value);
