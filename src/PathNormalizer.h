#pragma once

#include <string>
#include <string_view>

// Maps an executable path to a version-independent identity, so that
// "...\SpotifyMusic_1.272.438.0_x64__...\Spotify.exe" and
// "...\SpotifyMusic_1.300.277.0_x64__...\Spotify.exe" compare equal.
std::wstring NormalizeExecutablePath(std::wstring_view path);
