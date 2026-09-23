#include "FixPass.h"

#include "Log.h"
#include "NotifyIconStore.h"

std::wstring DescribeFix(const PlannedFix& fix)
{
    return L"IsPromoted=" + std::to_wstring(fix.isPromoted) + L" on " + fix.targetId + L" (" + fix.targetPath +
           L"), copied from " + fix.sourceId + L" (" + fix.sourcePath + L")";
}

FixPassResult RunFixPass(bool dryRun)
{
    FixPassResult result;
    auto entries = ReadIconEntries();
    if (!entries)
    {
        Log(L"Cannot read HKCU\\" + std::wstring(kNotifyIconSettingsKey));
        return result;
    }
    result.ok = true;

    for (const PlannedFix& fix : PlanFixes(*entries))
    {
        if (dryRun)
        {
            Log(L"[dry-run] Would set " + DescribeFix(fix));
            result.fixes.push_back(fix);
            continue;
        }

        switch (SetIsPromotedIfUnset(fix.targetId, fix.isPromoted))
        {
        case WriteResult::Written:
            Log(L"Set " + DescribeFix(fix));
            result.fixes.push_back(fix);
            break;
        case WriteResult::AlreadySet:
            break;
        case WriteResult::Failed:
            Log(L"Failed to set " + DescribeFix(fix));
            break;
        }
    }
    return result;
}
