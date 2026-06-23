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
  build_macos.sh      macOS build (Apple Silicon)
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
sudo apt install libx11-dev libxext-dev libgl1-mesa-dev libasound2-dev libudev-dev libdbus-1-dev
```

### macOS (Apple Silicon)

1. Install Xcode Command Line Tools: `xcode-select --install`
2. Install CMake: `brew install cmake`
3. Run `./build_macos.sh`

Unlike Windows and Linux, the macOS build is 64-bit (arm64) -- macOS has not
supported 32-bit executables since Catalina (2019).

**Status: compiles, links, and launches as an arm64 binary; not yet known to
run a game.** The retail-offset asserts that previously blocked the 64-bit
build are now routed through `CTR_STATIC_ASSERT_LAYOUT` (active on the 32-bit
Win/Linux builds, compiled out on 64-bit hosts where embedded host pointers
legitimately shift struct offsets). What remains before the game actually runs
is the runtime pointer-truncation gap: retail code stores and relocates
pointers in 32-bit slots (e.g. `LOAD_RunPtrMap`), which is only lossless when
those addresses fit in 32 bits. On macOS that cannot be arranged by address
placement -- the kernel SIGKILLs any arm64 binary with a non-default
`__PAGEZERO`, so all process memory lives above 4 GiB -- so the truncation must
be fixed in code. See "Known Gap" sections in `docs/MEMORY_MODEL.md`, and
`docs/MACOS_PORT.md` for the milestone roadmap (status, build/debug recipe, and
the remaining asset-relocation work).

## Building

```
build.bat            # Windows
chmod +x build.sh
./build.sh           # Linux
chmod +x build_macos.sh
./build_macos.sh     # macOS
```

First build compiles SDL3 from source. This is cached as a static library in `build/` -- subsequent builds only recompile touched native sources.

Output: `build/ctr_native.exe` (Windows) or `build/ctr_native` (Linux, macOS)

### Clean build

```
rmdir /s /q build    # Windows: delete cached libraries
build.bat            # Windows: rebuild everything

rm -rf build/        # Linux/macOS: delete cached libraries
./build.sh           # Linux: rebuild everything
./build_macos.sh     # macOS: rebuild everything
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
- Windows and Linux build in 32-bit mode while remaining PSX address-shaped data and host-pointer contracts are audited. macOS has no 32-bit support at all, so that build is 64-bit. GPU primitive links are bridged through 24-bit native tokens; see `docs/MEMORY_MODEL.md`.
- Struct field widths match PSX hardware layouts. Enums that back struct fields use GCC/Clang fixed-underlying syntax (`typedef enum Name : s16`, `: u8`, etc.) so `sizeof` matches the retail field width without relying on default enum size.

## Roadmap

- Clean up `game/` copies strip byte budget hacks and route platform-specific code through `CTR_NATIVE`
- Keep reducing 32-bit host-pointer assumptions in PSX-shaped data, and keep pruning inherited compatibility code now owned in `include/` and `platform/`.

## Credits

- [CTR-ModSDK](https://github.com/CTR-tools/CTR-ModSDK) — the decompilation project this is built on
- [PsyCross](https://github.com/OpenDriver2/PsyCross) — original PS1 compatibility code from which parts of CTR Native's owned platform layer and PsyQ facade headers are derived
- [SDL3](https://github.com/libsdl-org/SDL) — cross-platform multimedia
- Crash Team Racing is a trademark of Sony Computer Entertainment / Naughty Dog
