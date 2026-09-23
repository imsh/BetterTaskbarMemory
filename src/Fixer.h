#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// One subkey of HKCU\Control Panel\NotifyIconSettings.
struct IconEntry
{
    std::wstring id;                        // subkey name
    std::wstring executablePath;            // ExecutablePath
    std::optional<std::wstring> iconGuid;   // IconGuid (icons registered by GUID)
    std::optional<uint32_t> uid;            // UID (icons registered by window + id)
    std::optional<uint32_t> isPromoted;     // IsPromoted; absent = user never decided
    uint64_t lastWriteTime = 0;             // key last-write time (FILETIME ticks)
};

// Copy IsPromoted from sourceId to targetId.
struct PlannedFix
{
    std::wstring targetId;
    std::wstring targetPath;
    std::wstring sourceId;
    std::wstring sourcePath;
    uint32_t isPromoted = 0;
};

// Identity of a tray icon across app versions: normalized path + icon id within the exe.
std::wstring IconIdentity(const IconEntry& entry);

// For every entry without IsPromoted, finds a sibling with the same identity that has an
// explicit value (newest last-write time wins) and plans copying that value.
// Entries that already have IsPromoted are never touched.
std::vector<PlannedFix> PlanFixes(const std::vector<IconEntry>& entries);
