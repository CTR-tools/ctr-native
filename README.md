# CTR Native

A native PC port of Crash Team Racing (PS1, 1999), built on top of the [CTR-ModSDK](https://github.com/CTR-tools/CTR-ModSDK) decompilation project.

## Philosophy

- **No byte budget.** Game source lives in `game/` as our own copies. Edit freely.
- **No PSX toolchain.** Targets Windows and Linux with SDL3. No MIPS compiler needed.
- **Clean platform layer.** `main.c` owns process startup; host details stay in `platform/native_*`.
- **No build system nonsense.** Just `build.bat` / `build.sh`.
- **Fully static build.** Single executable, zero dependencies. SDL3 is compiled from vendored source and linked statically.

## Directory Layout

```
ctr_native/
  main.c              Entrypoint and native platform boundary
  platform/           Native-owned audio, input, memcard, CD, and PSX facade glue
  build-msvc.bat      Windows build (MSVC x86)
  build.bat           Windows build (MinGW i686)
  build.sh            Linux build
  CMakePresets.json   Shared CLion/command-line CMake configurations
  README.md           This file
  game/               Our copies of all decompiled game source (943 files)
    game_unity.h      Ordered unity include chain for all game source files
  include/            Project headers (structs, globals, declarations, platform facade)
  externals/
    SDL/              SDL3 source (static build)
```

## Prerequisites

### Windows

#### The recommended native Windows toolchain is MSVC x86:

1. Ensure you have an up to date "MSVC Build Tools", "Windows SDK" and "CMake" installed.  
   ```shell
   # Install OR Update VisualStudio Components
   winget install --id Microsoft.VisualStudio.BuildTools --force --custom "--focusedUi --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.Windows11SDK.28000"

   # Install CMake into PATH
   winget install --id Kitware.CMake --installer-type zip
   ```
4. Run `build-msvc.bat`, or select the `windows-msvc-x86` CMake preset in CLion.

#### The existing MinGW i686 build remains supported:

1. Install [MSYS2](https://www.msys2.org/).
2. In an MSYS2 terminal:
   ```
   pacman -Syu
   pacman -S --needed git mingw-w64-i686-gcc mingw-w64-i686-cmake mingw-w64-i686-make
   ```
   If the update asks you to close the terminal, reopen MSYS2 and run the install command.
3. Add `C:\msys64\mingw32\bin` to your system PATH
4. Open a new Command Prompt or PowerShell and run `build.bat`.

That's it. SDL3 is compiled from vendored source -- no separate install needed.

### Linux (Debian/Ubuntu)

```
sudo apt install gcc-multilib
sudo apt install libx11-dev libxext-dev libgl1-mesa-dev libasound2-dev libudev-dev libdbus-1-dev
```

## Building

```
build-msvc.bat       # Windows, MSVC x86 (recommended)
build.bat            # Windows, MinGW i686
chmod +x build.sh
./build.sh           # Linux
```

The shared CMake presets can also be used directly or selected as CLion CMake profiles:

```
cmake --preset windows-msvc-x86
cmake --build --preset windows-msvc-x86-debug
ctest --preset windows-msvc-x86-debug
```

First build compiles SDL3 from source. This is cached as a static library in the selected build directory.

Output:

- MSVC: `build-msvc-x86/Release/ctr_native.exe`
- MinGW: `build/ctr_native.exe`
- Linux: `build/ctr_native`

### Clean build

```
rmdir /s /q build    # Windows: delete cached libraries
build.bat            # Windows: rebuild everything

rmdir /s /q build-msvc-x86
build-msvc.bat       # Windows MSVC: rebuild everything

rm -rf build/        # Linux: delete cached libraries
./build.sh           # Linux: rebuild everything
```

## Running

### Normal Setup

If you downloaded a release build, you only need two things for normal play:

1. The game executable:
   - `ctr_native.exe` on Windows
   - `ctr_native` on Linux
2. Your own NTSC-U retail CTR disc image, named (put in directory called `assets`):
   - `assets/ctr-u.bin`

Example:

```
CTR-Native/
  ctr_native.exe
  assets/
    ctr-u.bin
```

Then run `ctr_native.exe`.

The disc image must be the common single-track raw PSX BIN layout: MODE2/2352 sectors, with the data track starting at byte 0. A cooked 2048-byte `.iso` does not preserve the XA/STR sector data needed for audio and video playback.

For development builds run from `build/`, put the same `assets/ctr-u.bin` next to the source tree:

```
ctr-native/
  build/
    ctr_native.exe
  assets/
    ctr-u.bin
```

### Extracted Asset Override

You do not need extracted assets for normal play.

Extracted files are still supported for development, modding, and debugging. If present, they override files from `ctr-u.bin`.

Extracted-asset override structure:

```
CTR-Native/
  ctr_native.exe
  assets/
    BIGFILE.BIG
    SOUNDS/KART.HWL
    TEST.STR
    XA/
      ENG.XNF
      ENG/EXTRA/S00.XA ... S05.XA
      ENG/GAME/S00.XA ... S20.XA
      MUSIC/S00.XA ... S01.XA
```

The full extracted asset list is:

- `BIGFILE.BIG`
- `SOUNDS/KART.HWL`
- `TEST.STR`
- `XA/ENG.XNF`
- `XA/ENG/EXTRA/S00.XA` through `S05.XA`
- `XA/ENG/GAME/S00.XA` through `S20.XA`
- `XA/MUSIC/S00.XA` through `S01.XA`

`BIGFILE.BIG` and `SOUNDS/KART.HWL` are required. Missing XA/XNF files or
`TEST.STR` produce a startup warning: the game can continue without the affected streamed audio or Scrapbook video. Files available in `ctr-u.bin` still provide the fallback for missing extracted files. A present but invalid XA manifest remains a startup error.

## Bug Replays

Internal builds can record a small bug report folder. See `docs/REPLAYS.md`.

## Architecture

```
main.c (entrypoint)
  |
  +-- platform/native_* (platform shell, audio, input, memcard, CD, renderer, PSX facade glue)
  |
  +-- game/game_unity.h
        |
        +-- game/ (all decompiled game source)
              |
              +-- include/ (headers: structs, globals, declarations)
```

- `CTR_NATIVE` is defined for native host/platform-specific code
- First-party native code targets portable C17 with compiler extensions disabled
- The default build uses 32-bit mode while remaining PSX address-shaped data and host-pointer contracts are audited. GPU primitive links are bridged through 24-bit native tokens; see `docs/MEMORY_MODEL.md`.

## Roadmap

- Clean up `game/` copies strip byte budget hacks and route platform-specific code through `CTR_NATIVE`
- Keep reducing 32-bit host-pointer assumptions in PSX-shaped data, and keep pruning inherited compatibility code now owned in `include/` and `platform/`.

## Credits

- [CTR-ModSDK](https://github.com/CTR-tools/CTR-ModSDK) — the decompilation project this is built on
- [PsyCross](https://github.com/OpenDriver2/PsyCross) — original PS1 compatibility code from which parts of CTR Native's owned platform layer and PsyQ facade headers are derived
- [SDL3](https://github.com/libsdl-org/SDL) — cross-platform multimedia
- Crash Team Racing is a trademark of Sony Computer Entertainment / Naughty Dog