# CTR Native

A native PC port of Crash Team Racing (PS1, 1999), built on top of the [CTR-ModSDK](https://github.com/CTR-tools/CTR-ModSDK) decompilation project.

## Philosophy

- **No byte budget.** Game source lives in `game/` as our own copies. Edit freely.
- **No PSX toolchain.** Targets Windows and Linux with SDL3 plus a small remaining PsyCross compatibility tail. No MIPS compiler needed.
- **Clean platform layer.** Host details stay in `ctr_native.c` and `platform/native_*`.
- **No build system nonsense.** Just `build.bat` / `build.sh`.
- **Fully static build.** Single executable, zero dependencies. SDL3 and the remaining PsyCross compatibility code are compiled from vendored source and linked statically.

## Directory Layout

```
ctr_native/
  ctr_native.c        Platform layer: SDL3 init, main(), frame loop
  ctr_native.h        Platform state structs
  platform.h          Platform API the game calls through
  platform/           Native-owned audio, input, memcard, CD, and PSX facade glue
  game_includes.h     Ordered include chain for all game source files
  build.bat           Windows build (MinGW32)
  build.sh            Linux build
  README.md           This file
  game/               Our copies of all decompiled game source (943 files)
  include/            Project headers (structs, globals, declarations)
  externals/
    PsyCross/         Remaining PS1 GTE/libetc compatibility tail
    SDL/              SDL3 source (static build)
```

## Prerequisites

### Windows

1. Install [MSYS2](https://www.msys2.org/)
2. In an MSYS2 terminal:
   ```
   pacman -S mingw-w64-i686-gcc mingw-w64-i686-tools-git
   ```
3. Add `C:\msys64\mingw32\bin` to your system PATH

That's it. SDL3 is compiled from vendored source -- no separate install needed.

### Linux (Debian/Ubuntu)

```
sudo apt install gcc-multilib
sudo apt install libx11-dev libxext-dev libgl1-mesa-dev libasound2-dev libudev-dev libdbus-1-dev
```

## Building

```
build.bat            # Windows
chmod +x build.sh
./build.sh           # Linux
```

First build compiles SDL3 and the remaining PsyCross compatibility code from source. These are cached as static libraries in `build/` -- subsequent builds only recompile touched native sources.

Output: `build/ctr_native.exe` (Windows) or `build/ctr_native` (Linux)

### Clean build

```
rm -rf build/        # Delete cached libraries
build.bat            # Rebuild everything
```

## Running

1. Create an `assets/` directory next to the source files
2. Extract the following from a CTR NTSC-U retail disc image:
   - `BIGFILE.BIG`
   - `SOUNDS/KART.HWL`
   - `XA/ENG.XNF`
   - `XA/ENG/EXTRA/S00.XA` through `S05.XA`
   - `XA/ENG/GAME/S00.XA` through `S05.XA`
   - `XA/MUSIC/S00.XA` through `S05.XA`
3. Run `build/ctr_native.exe`

Directory structure:
```
ctr_native/
  assets/
    BIGFILE.BIG
    SOUNDS/KART.HWL
    XA/
      ENG.XNF
      ENG/EXTRA/S00.XA ... S05.XA
      ENG/GAME/S00.XA ... S05.XA
      MUSIC/S00.XA ... S05.XA
```

## Bug Replays

Internal builds can record a small bug report folder. See `docs/REPLAYS.md`.

## Architecture

```
ctr_native.c (platform layer)
  |
  +-- PsyCross (remaining PS1 GTE/libetc compatibility tail)
  |
  +-- platform/native_* (audio, input, memcard, CD, renderer, PSX facade glue)
  |
  +-- game_includes.h
        |
        +-- game/ (all decompiled game source)
              |
              +-- include/ (headers: structs, globals, declarations)
```

- `CTR_NATIVE` is defined for native host/platform-specific code
- The build currently uses 32-bit mode because some native paths still pack host pointers into retail-shaped 24-bit GPU primitive links. See `docs/MEMORY_MODEL.md` for the roadmap to replace that with an explicit native GPU link bridge.

## Roadmap

- Clean up `game/` copies strip byte budget hacks and route platform-specific code through `CTR_NATIVE`
- Replace the remaining PsyCross GTE/libetc compatibility tail, replace low-address primitive-link assumptions with an explicit native GPU link bridge, remove the 32-bit constraint, and own the full stack.

## Credits

- [CTR-ModSDK](https://github.com/CTR-tools/CTR-ModSDK) — the decompilation project this is built on
- [PsyCross](https://github.com/OpenDriver2/PsyCross) — vendored PS1 compatibility code that CTR Native is progressively replacing with owned platform code
- [SDL3](https://github.com/libsdl-org/SDL) — cross-platform multimedia
- Crash Team Racing is a trademark of Sony Computer Entertainment / Naughty Dog
