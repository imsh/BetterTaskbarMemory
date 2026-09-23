# BetterTaskbarMemory

A tray app that keeps your "show this icon in the system tray" choices across app updates.

Windows 11 stores tray visibility per executable path in `HKCU\Control Panel\NotifyIconSettings\<id>`
(`ExecutablePath`, `IsPromoted`). Apps whose install path contains a version (Store apps like Spotify,
Squirrel-style `app-1.2.3` folders, …) get a fresh entry after every update and the icon falls back to hidden.

BetterTaskbarMemory watches that key. When an entry appears **without** `IsPromoted` and an older entry for the
same app (same path once version numbers are ignored, same icon `UID`/`IconGuid`) has an explicit value, it
copies that value. Entries with an explicit `IsPromoted` are never changed.

## Build

Requires Visual Studio 2026 (v145 toolset) with the Windows SDK; nothing else.

```
msbuild BetterTaskbarMemory.slnx -p:Configuration=Release -p:Platform=x64
bin\x64\Release\Tests.exe
```

Output: `bin\x64\Release\BetterTaskbarMemory.exe` (static CRT, no redistributables needed).

## Use

- `BetterTaskbarMemory.exe --dry-run` prints what it would change and exits without writing.
- `BetterTaskbarMemory.exe` runs in the tray. Menu: Fix now, Recent fixes, Pause watching,
  Show notifications, Start with Windows, Open log, Exit.
- Log: `%LOCALAPPDATA%\BetterTaskbarMemory\log.txt`. Settings: `HKCU\Software\BetterTaskbarMemory`.

Back up before the first real run: `reg export "HKCU\Control Panel\NotifyIconSettings" backup.reg`.

## Layout

| File | Purpose |
| --- | --- |
| `src/PathNormalizer.*` | version-independent path identity (pure) |
| `src/Fixer.*` | grouping and fix planning (pure, unit-tested) |
| `src/NotifyIconStore.*` | registry read/write |
| `src/FixPass.*` | read → plan → write → log |
| `src/Watcher.*` | `RegNotifyChangeKeyValue` worker thread with debounce |
| `src/TrayIcon.*`, `src/main.cpp` | tray icon, menu (`App::BuildMenuItems` is where new features plug in) |
| `res/make-icon.ps1` | regenerates `res/app.ico` |
