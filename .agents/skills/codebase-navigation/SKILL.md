---
name: codebase-navigation
description: Navigate CTR Native Expanded's unity build, platform boundary, headers, overlays, and build tools when locating or adding code.
---

# Navigate the codebase

Use this skill when finding the owner of a change or adding a new game or platform source file.

1. Start at `main.c`: it includes `game/game_unity.h`, resident `game/zGlobal_*.c`, and the `platform/native_*.c` implementations in a specific order. This is a unity build; a new `.c` file must be included in the appropriate chain, and file-local identifiers can collide across included files.
2. Put game behavior in `game/` (e.g. `game/MAIN/`, `game/UI/`, `game/230/` through `game/233/`); put SDL, OpenGL, host filesystem, audio, input, and compatibility adapters in `platform/`. Shared declarations and layouts belong in `include/`; game function declarations go in `include/functions.h`.
3. Consult `docs/OVERLAYS.md` and `game/LOAD/LOAD_Overlays.c` for logical overlay ownership: 221–225 end-of-race UI, 226–229 per-player-count level renderers, 230 main menu, 231 race/battle, 232 adventure hub, 233 cutscenes/podiums. Native links these functions into the executable but still tracks overlay indices and reinitializes region-3 data on transitions.
4. Build with `build-msvc.bat` (Windows MSVC x86), `build.bat` (Windows MinGW i686), or `build.sh` (Linux); see `CMakeLists.txt` and `CMakePresets.json`. The application target is 32-bit C17 and CMake defines `CTR_INTERNAL`. There is a single build target, so do not add conditional-compilation forks for application changes.
5. Focused tests live in `tests/`, one suite per executable under `tests/CMakeLists.txt`, and every one is registered with CTest. A suite that needs the unity build's file-static state includes the module's own sources into its own translation unit; see `tests/shared/ctr_test.h` for the shared harness. An ordinary build compiles the game and the suites but runs no test; `cmake --build <dir> --target tests` rebuilds only the suites. Avoid changes to vendored `externals/SDL/` for application features.

Before editing, check the worktree for user changes; do not overwrite or discard them.
