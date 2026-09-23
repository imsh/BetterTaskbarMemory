#pragma once

#include <windows.h>

#include <atomic>
#include <functional>
#include <thread>
#include <vector>

#include "Fixer.h"

// Watches HKCU\Control Panel\NotifyIconSettings on a worker thread and runs a fix pass
// after each (debounced) change, once at startup, and on request.
class Watcher
{
public:
    // Called on the worker thread when fixes were written, or after every manual scan.
    using FixesCallback = std::function<void(std::vector<PlannedFix> fixes, bool manual)>;

    explicit Watcher(FixesCallback callback);
    ~Watcher();
    Watcher(const Watcher&) = delete;
    Watcher& operator=(const Watcher&) = delete;

    void Start();
    void Stop();
    void RequestScan();              // runs even while paused
    void SetPaused(bool paused);     // paused: registry changes are ignored

private:
    void Run();
    void Scan(bool manual) const;

    FixesCallback callback_;
    HANDLE stopEvent_ = nullptr;
    HANDLE scanEvent_ = nullptr;
    std::thread thread_;
    std::atomic<bool> paused_{ false };
};
