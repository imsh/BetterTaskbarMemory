#include "Watcher.h"

#include <string>

#include "FixPass.h"
#include "Log.h"
#include "NotifyIconStore.h"

namespace
{
// Explorer writes several values per icon in bursts; wait for it to settle.
constexpr DWORD kDebounceMs = 1500;
}

Watcher::Watcher(FixesCallback callback)
    : callback_(std::move(callback))
    , stopEvent_(CreateEventW(nullptr, TRUE, FALSE, nullptr))
    , scanEvent_(CreateEventW(nullptr, FALSE, FALSE, nullptr))
{
}

Watcher::~Watcher()
{
    Stop();
    CloseHandle(stopEvent_);
    CloseHandle(scanEvent_);
}

void Watcher::Start()
{
    if (!thread_.joinable())
    {
        ResetEvent(stopEvent_);
        thread_ = std::thread(&Watcher::Run, this);
    }
}

void Watcher::Stop()
{
    if (thread_.joinable())
    {
        SetEvent(stopEvent_);
        thread_.join();
    }
}

void Watcher::RequestScan()
{
    SetEvent(scanEvent_);
}

void Watcher::SetPaused(bool paused)
{
    paused_ = paused;
}

void Watcher::Scan(bool manual) const
{
    if (FixPassResult result = RunFixPass(false); !result.fixes.empty() || manual)
        callback_(std::move(result.fixes), manual);
}

void Watcher::Run()
{
    HKEY key = nullptr;
    if (LSTATUS status = RegOpenKeyExW(HKEY_CURRENT_USER, kNotifyIconSettingsKey, 0, KEY_NOTIFY, &key); status != ERROR_SUCCESS)
    {
        Log(L"Cannot watch HKCU\\" + std::wstring(kNotifyIconSettingsKey) + L" (error " + std::to_wstring(status) + L")");
        key = nullptr;
    }

    const HANDLE changeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    bool armed = false;
    bool manual = false;   // the first pass (at startup) is automatic

    // Every iteration scans once: at startup, after a debounced change, or on request.
    for (;;)
    {
        // Re-arm before scanning so that no change made during the scan is missed.
        if (key && !armed)
        {
            const LSTATUS status = RegNotifyChangeKeyValue(key, TRUE, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, changeEvent, TRUE);
            armed = status == ERROR_SUCCESS;
            if (!armed)
            {
                Log(L"RegNotifyChangeKeyValue failed (error " + std::to_wstring(status) + L"); watching stopped");
                RegCloseKey(key);
                key = nullptr;
            }
        }

        if (manual || !paused_)
            Scan(manual);

        const HANDLE handles[] = { stopEvent_, scanEvent_, changeEvent };
        const DWORD wait = WaitForMultipleObjects(armed ? 3 : 2, handles, FALSE, INFINITE);
        if (wait == WAIT_OBJECT_0)
            break;
        if (wait == WAIT_OBJECT_0 + 1)
        {
            manual = true;
        }
        else if (wait == WAIT_OBJECT_0 + 2)
        {
            manual = false;
            armed = false;
            if (WaitForSingleObject(stopEvent_, kDebounceMs) == WAIT_OBJECT_0)
                break;
        }
        else
        {
            Log(L"Watcher wait failed (error " + std::to_wstring(GetLastError()) + L")");
            break;
        }
    }

    if (key)
        RegCloseKey(key);
    CloseHandle(changeEvent);
}
