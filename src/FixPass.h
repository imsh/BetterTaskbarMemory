#pragma once

#include <vector>

#include "Fixer.h"

struct FixPassResult
{
    bool ok = false;                  // false if the registry could not be read
    std::vector<PlannedFix> fixes;    // fixes written (or, in a dry run, that would be written)
};

// Reads NotifyIconSettings, plans fixes and (unless dryRun) writes them. Logs every change.
FixPassResult RunFixPass(bool dryRun);

std::wstring DescribeFix(const PlannedFix& fix);
