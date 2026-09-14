# Silent Dialogue Universal

Silent Dialogue Universal is an independent SKSE plugin that supplies silent
voice audio to otherwise unvoiced dialogue, allowing the dialogue to remain on
screen long enough to read. It supports Skyrim Special Edition 1.5.97, Skyrim
Anniversary Edition runtimes supported by Address Library (including
1.7.104.0), and supported GOG runtimes. Skyrim VR is not supported.

The implementation was written independently and does not include code,
binaries, or voice assets from Fuz Ro D'oh.

## Requirements

- A matching version of SKSE
- Address Library for SKSE Plugins matching the game runtime
- The generated silent FUZ files from the release package

## Configuration

`Data/SKSE/Plugins/SilentDialogueUniversal.ini` controls reading speed,
minimum and maximum silent-line duration, forced subtitles, and verbose
logging.

## Building

The tested build uses CMake 3.24 or newer, Ninja, MSVC, and a vcpkg toolchain.
CMake fetches the pinned CommonLibSSE-NG 8.0.0 and MinHook 1.3.4 revisions
automatically. Existing local checkouts can be supplied for offline builds.

Configure and build a standalone clone:

```powershell
cmake --preset release `
  -DCMAKE_TOOLCHAIN_FILE="C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static-md
cmake --build --preset release
```

For an offline build, additionally pass `-DCOMMONLIBSSE_ROOT` and
`-DMINHOOK_ROOT` with paths to the pinned dependency checkouts.

The silent assets can be regenerated using Bethesda's Creation Kit tools and
ffmpeg:

```powershell
./tools/generate_silence_assets.ps1 -SkyrimRoot "C:/path/to/Skyrim Special Edition"
```

## License

Copyright (C) 2026 DeadOnKeyboard

Silent Dialogue Universal is licensed under the GNU General Public License
version 3 or, at your option, any later version. See [LICENSE](LICENSE) and
[NOTICE.md](NOTICE.md). MinHook's separate license is included under
[`licenses`](licenses/MinHook-LICENSE.txt).
