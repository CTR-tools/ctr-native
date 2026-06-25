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
  main.c              Entrypoint and unity include manifest
  platform.h          Platform API the game calls through
  platform/           Native-owned audio, input, memcard, CD, and PSX facade glue
  game_includes.h     Ordered include chain for all game source files
  build.bat           Windows build (MinGW32)
  build.sh            Linux build
  README.md           This file
	  game/               Our copies of all decompiled game source (943 files)
	  include/            Project headers (structs, globals, declarations)
	  externals/
	    SDL/              SDL3 source (static build)
```

## Prerequisites

### Windows

1. Install [MSYS2](https://www.msys2.org/)
2. In an MSYS2 terminal:
   ```
   pacman -Syu
   pacman -S --needed git mingw-w64-i686-gcc mingw-w64-i686-cmake mingw-w64-i686-make
   ```
   If the update asks you to close the terminal, reopen MSYS2 and run the install command.
3. Add `C:\msys64\mingw32\bin` to your system PATH
4. Open a new Command Prompt or PowerShell and run `build.bat`

That's it. SDL3 is compiled from vendored source -- no separate install needed.

### Linux (Debian/Ubuntu)

```
sudo apt install gcc-multilib
sudo apt install libx11-dev libxext-dev libgl1-mesa-dev libasound2-dev libudev-dev libdbus-1-dev libxtst-dev
```

## Building

```
build.bat            # Windows
chmod +x build.sh
./build.sh           # Linux
```

First build compiles SDL3 from source. This is cached as a static library in `build/` -- subsequent builds only recompile touched native sources.

Output: `build/ctr_native.exe` (Windows) or `build/ctr_native` (Linux)

### Clean build

```
rmdir /s /q build    # Windows: delete cached libraries
build.bat            # Windows: rebuild everything

rm -rf build/        # Linux: delete cached libraries
./build.sh           # Linux: rebuild everything
```

## Running

1. Create an `assets/` directory next to the executable for packaged builds, or
   next to the source files for development builds run from `build/`
2. Extract the following from a CTR NTSC-U retail disc image:
   - `BIGFILE.BIG`
   - `SOUNDS/KART.HWL`
   - `TEST.STR`
   - `XA/ENG.XNF`
   - `XA/ENG/EXTRA/S00.XA` through `S05.XA`
   - `XA/ENG/GAME/S00.XA` through `S20.XA`
   - `XA/MUSIC/S00.XA` through `S01.XA`
3. Run `build/ctr_native.exe`

Packaged directory structure:
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

Development directory structure:
```
ctr_native/
  build/
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

## Bug Replays

Internal builds can record a small bug report folder. See `docs/REPLAYS.md`.

## Architecture

```
main.c (entrypoint)
  |
  +-- platform/native_* (platform shell, audio, input, memcard, CD, renderer, PSX facade glue)
  |
  +-- game_includes.h
        |
        +-- game/ (all decompiled game source)
              |
              +-- include/ (headers: structs, globals, declarations)
```

- `CTR_NATIVE` is defined for native host/platform-specific code
- The default build uses 32-bit mode while remaining PSX address-shaped data and host-pointer contracts are audited. GPU primitive links are bridged through 24-bit native tokens; see `docs/MEMORY_MODEL.md`.

## Roadmap

- Clean up `game/` copies strip byte budget hacks and route platform-specific code through `CTR_NATIVE`
- Keep reducing 32-bit host-pointer assumptions in PSX-shaped data, and keep pruning inherited compatibility code now owned in `include/` and `platform/`.

## Credits

- [CTR-ModSDK](https://github.com/CTR-tools/CTR-ModSDK) — the decompilation project this is built on
- [PsyCross](https://github.com/OpenDriver2/PsyCross) — original PS1 compatibility code from which parts of CTR Native's owned platform layer and PsyQ facade headers are derived
- [SDL3](https://github.com/libsdl-org/SDL) — cross-platform multimedia
- Crash Team Racing is a trademark of Sony Computer Entertainment / Naughty Dog
