#include "Fixer.h"

#include <cwctype>
#include <map>
#include <ranges>

#include "PathNormalizer.h"

std::wstring IconIdentity(const IconEntry& entry)
{
    std::wstring identity = NormalizeExecutablePath(entry.executablePath);
    if (entry.iconGuid)
    {
        identity += L"|guid:";
        for (wchar_t ch : *entry.iconGuid)
            identity.push_back(static_cast<wchar_t>(std::towlower(ch)));
    }
    else if (entry.uid)
    {
        identity += L"|uid:" + std::to_wstring(*entry.uid);
    }
    return identity;
}

std::vector<PlannedFix> PlanFixes(const std::vector<IconEntry>& entries)
{
    std::map<std::wstring, std::vector<const IconEntry*>> groups;
    for (const IconEntry& entry : entries)
    {
        if (!entry.executablePath.empty())
            groups[IconIdentity(entry)].push_back(&entry);
    }

    std::vector<PlannedFix> fixes;
    for (const auto& members : groups | std::views::values)
    {
        const IconEntry* source = nullptr;
        for (const IconEntry* member : members)
        {
            if (!member->isPromoted)
                continue;
            if (!source || member->lastWriteTime > source->lastWriteTime ||
                (member->lastWriteTime == source->lastWriteTime && member->id > source->id))
            {
                source = member;
            }
        }
        if (!source)
            continue;

        for (const IconEntry* member : members)
        {
            if (!member->isPromoted)
            {
                fixes.push_back({ .targetId = member->id,
                                  .targetPath = member->executablePath,
                                  .sourceId = source->id,
                                  .sourcePath = source->executablePath,
                                  .isPromoted = *source->isPromoted });
            }
        }
    }
    return fixes;
}
