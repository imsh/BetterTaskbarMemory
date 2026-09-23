# BetterTaskbarMemory

A tray app that keeps your "show this icon in the system tray" choices across app updates.

Windows stores tray visibility per executable path in `HKCU\Control Panel\NotifyIconSettings\<id>`
(`ExecutablePath`, `IsPromoted`). Apps whose install path contains a version (Store apps like Spotify,
Squirrel-style `app-1.2.3` folders, …) get a fresh entry after every update and the icon falls back to hidden.

BetterTaskbarMemory watches that key. When an entry appears **without** `IsPromoted` and an older entry for the
same app (same path once version numbers are ignored, same icon `UID`/`IconGuid`) has an explicit value, it
copies that value. Entries with an explicit `IsPromoted` are never changed.

## Build

Requires Visual Studio 2026 (v145 toolset) with the Windows SDK. NuGet packages (Nerdbank.GitVersioning,
WiX Toolset) are restored automatically by VS/Rider; on the command line pass `-restore`:

```
msbuild BetterTaskbarMemory.slnx -restore -p:RestorePackagesConfig=true -p:Configuration=Release -p:Platform=x64
bin\x64\Release\Tests.exe
```

Output in `bin\x64\Release\`:
- `BetterTaskbarMemory.exe` (static CRT, no redistributables needed)
- `BetterTaskbarMemory.msi` (installer)

## Versioning

Versions come from [Nerdbank.GitVersioning](https://github.com/dotnet/Nerdbank.GitVersioning): `version.json`
holds major.minor, and the git height (commits since `version.json` last changed) becomes the build number.
The exe's version resource is generated at build time; `nbgv get-version` shows the current version.
Bump major/minor by editing `version.json` (or `nbgv set-version 1.1`).

## Installer

`installer/` is a [WiX Toolset](https://wixtoolset.org) v7 project producing a per-user MSI (no UAC prompt):

- installs to `%LOCALAPPDATA%\Programs\BetterTaskbarMemory` with a Start Menu shortcut;
- enables "Start with Windows" and offers to launch the app when setup finishes;
- closes a running instance on upgrade/uninstall; uninstall also removes the autostart entry and
  `HKCU\Software\BetterTaskbarMemory` (the log folder is kept).

The MSI version is taken from the exe, so every commit produces an upgradable installer.
If you turn off "Start with Windows" in the app, repairing the MSI turns it back on.

## CI and releases

Both workflows build on the `windows-2025-vs2026` runner.

- **`.github/workflows/build.yml`** runs on every push to `main` and every pull request (or manually). It builds,
  runs the tests, and uploads `BetterTaskbarMemory-<version>-x64.msi` and a portable zip (exe + license) as
  workflow artifacts.
- **`.github/workflows/release.yml`** is triggered manually: Actions → Release → Run workflow on `main`. It runs
  `build.yml`, then publishes a GitHub release tagged `v<major.minor.height>` with those files and notes
  generated from the commits since the previous release. Re-running on the same commit fails because the tag
  already exists.

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
| `installer/` | WiX v7 MSI project |

## License

[MIT](LICENSE)
