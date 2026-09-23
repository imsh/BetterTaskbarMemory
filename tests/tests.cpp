#include <cstdio>
#include <string>
#include <vector>

#include "../src/Fixer.h"
#include "../src/PathNormalizer.h"

namespace
{
int g_failures = 0;
int g_checks = 0;

#define CHECK(condition)                                                        \
    do                                                                          \
    {                                                                           \
        ++g_checks;                                                             \
        if (!(condition))                                                       \
        {                                                                       \
            ++g_failures;                                                       \
            std::printf("FAILED %s:%d: %s\n", __FILE__, __LINE__, #condition);  \
        }                                                                       \
    } while (false)

bool SameApp(const wchar_t* a, const wchar_t* b)
{
    return NormalizeExecutablePath(a) == NormalizeExecutablePath(b);
}

IconEntry Entry(std::wstring id, std::wstring path, std::optional<uint32_t> isPromoted, uint64_t lastWrite = 0,
                std::optional<uint32_t> uid = 0)
{
    IconEntry entry;
    entry.id = std::move(id);
    entry.executablePath = std::move(path);
    entry.isPromoted = isPromoted;
    entry.lastWriteTime = lastWrite;
    entry.uid = uid;
    return entry;
}

constexpr wchar_t kSpotifyOld[] = L"{6D809377-6AF0-444B-8957-A3773F02200E}\\WindowsApps\\SpotifyAB.SpotifyMusic_1.272.438.0_x64__zpdnekdrzrea0\\Spotify.exe";
constexpr wchar_t kSpotifyNew[] = L"{6D809377-6AF0-444B-8957-A3773F02200E}\\WindowsApps\\SpotifyAB.SpotifyMusic_1.300.277.0_x64__zpdnekdrzrea0\\Spotify.exe";

void TestNormalizer()
{
    CHECK(SameApp(kSpotifyOld, kSpotifyNew));
    CHECK(SameApp(L"C:\\Users\\Example\\AppData\\Local\\FlowLauncher\\app-2.1.3\\Flow.Launcher.exe",
                  L"C:\\Users\\Example\\AppData\\Local\\FlowLauncher\\app-2.1.4\\Flow.Launcher.exe"));
    CHECK(SameApp(L"C:\\Program Files\\WindowsApps\\10186emoacht.Monitorian_4.13.0.0_neutral__0q7myvhtpbc7w\\MonitorianPlus\\MonitorianPlus.exe",
                  L"C:\\Program Files\\WindowsApps\\10186emoacht.Monitorian_4.15.2.0_neutral__0q7myvhtpbc7w\\MonitorianPlus\\MonitorianPlus.exe"));
    CHECK(SameApp(L"{6D809377-6AF0-444B-8957-A3773F02200E}\\Google\\Drive File Stream\\131.0.2.0\\GoogleDriveFS.exe",
                  L"{6D809377-6AF0-444B-8957-A3773F02200E}\\Google\\Drive File Stream\\132.1.0.0\\GoogleDriveFS.exe"));
    CHECK(SameApp(L"C:\\Program Files\\GPU-Z\\GPU-Z.2.70.0.exe", L"C:\\Program Files\\GPU-Z\\GPU-Z.2.71.0.exe"));
    CHECK(SameApp(L"{6D809377-6AF0-444B-8957-A3773F02200E}\\WindowsApps\\MSTeams_26225.1806.5074.1452_x64__8wekyb3d8bbwe\\ms-teams.exe",
                  L"{6D809377-6AF0-444B-8957-A3773F02200E}\\WindowsApps\\MSTeams_26300.100.1.2_x64__8wekyb3d8bbwe\\ms-teams.exe"));
    CHECK(SameApp(L"C:\\Program Files\\App\\APP.EXE", L"c:\\program files\\app\\app.exe"));

    CHECK(!SameApp(L"C:\\Users\\Example\\AppData\\Local\\Programs\\Rider\\bin\\rider64.exe",
                   L"C:\\Users\\Example\\AppData\\Local\\Programs\\Rider 2\\bin\\rider64.exe"));
    CHECK(!SameApp(L"C:\\Program Files\\App\\app.exe", L"C:\\Program Files\\Other\\app.exe"));
    CHECK(!SameApp(kSpotifyOld, L"{6D809377-6AF0-444B-8957-A3773F02200E}\\WindowsApps\\SpotifyAB.SpotifyMusic_1.300.277.0_arm64__zpdnekdrzrea0\\Spotify.exe"));
}

void TestNewVersionInheritsSetting()
{
    auto fixes = PlanFixes({ Entry(L"old", kSpotifyOld, 1u), Entry(L"new", kSpotifyNew, std::nullopt) });
    CHECK(fixes.size() == 1);
    if (fixes.size() == 1)
    {
        CHECK(fixes[0].targetId == L"new");
        CHECK(fixes[0].sourceId == L"old");
        CHECK(fixes[0].isPromoted == 1);
    }
}

void TestHiddenIsInheritedToo()
{
    auto fixes = PlanFixes({ Entry(L"old", kSpotifyOld, 0u), Entry(L"new", kSpotifyNew, std::nullopt) });
    CHECK(fixes.size() == 1 && fixes[0].isPromoted == 0);
}

void TestExplicitValueIsNeverOverwritten()
{
    CHECK(PlanFixes({ Entry(L"old", kSpotifyOld, 1u), Entry(L"new", kSpotifyNew, 0u) }).empty());
}

void TestNoSourceNoFix()
{
    CHECK(PlanFixes({ Entry(L"a", kSpotifyOld, std::nullopt), Entry(L"b", kSpotifyNew, std::nullopt) }).empty());
}

void TestNewestSourceWins()
{
    auto fixes = PlanFixes({
        Entry(L"older", L"C:\\App\\1.0\\app.exe", 0u, 100),
        Entry(L"newer", L"C:\\App\\2.0\\app.exe", 1u, 200),
        Entry(L"target", L"C:\\App\\3.0\\app.exe", std::nullopt, 300),
    });
    CHECK(fixes.size() == 1 && fixes[0].sourceId == L"newer" && fixes[0].isPromoted == 1);
}

void TestDifferentIconsOfSameExeAreSeparate()
{
    constexpr wchar_t outlook[] = L"{6D809377-6AF0-444B-8957-A3773F02200E}\\Microsoft Office\\root\\Office16\\OUTLOOK.EXE";
    CHECK(PlanFixes({ Entry(L"a", outlook, 1u, 0, 0u), Entry(L"b", outlook, std::nullopt, 0, 12345u) }).empty());

    IconEntry shown = Entry(L"c", L"{F38BF404-1D43-42F2-9305-67DE0B28FC23}\\explorer.exe", 1u, 0, std::nullopt);
    shown.iconGuid = L"{7820AE83-23E3-4229-82C1-E41CB67D5B9C}";
    IconEntry other = Entry(L"d", L"{F38BF404-1D43-42F2-9305-67DE0B28FC23}\\explorer.exe", std::nullopt, 0, std::nullopt);
    other.iconGuid = L"{7820AE78-23E3-4229-82C1-E41CB67D5B9C}";
    CHECK(PlanFixes({ shown, other }).empty());

    IconEntry sameGuid = other;
    sameGuid.iconGuid = L"{7820ae83-23e3-4229-82c1-e41cb67d5b9c}";
    CHECK(PlanFixes({ shown, sameGuid }).size() == 1);
}

void TestEntriesWithoutPathAreIgnored()
{
    CHECK(PlanFixes({ Entry(L"a", L"", 1u), Entry(L"b", L"", std::nullopt) }).empty());
}
} // namespace

int main()
{
    TestNormalizer();
    TestNewVersionInheritsSetting();
    TestHiddenIsInheritedToo();
    TestExplicitValueIsNeverOverwritten();
    TestNoSourceNoFix();
    TestNewestSourceWins();
    TestDifferentIconsOfSameExeAreSeparate();
    TestEntriesWithoutPathAreIgnored();

    std::printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
