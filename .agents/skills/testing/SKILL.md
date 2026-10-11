---
name: testing
description: Add or change focused tests under tests/, and run them.
---

# Testing

Use this skill when adding a suite, changing an existing one, or deciding what a
test should cover.

Suites live in `tests/`, one executable each, and every one is registered with
CTest. `build.bat`, `build-msvc.bat` and `build.sh` compile the game and the
suites but run nothing, so tests are a separate step:

```
ctest --test-dir build --output-on-failure             # everything
ctest --test-dir build --output-on-failure -R "test_"  # suites only
cmake --build build --target tests                     # rebuild suites, not the game
```

`ctr_native_expanded_version` runs the game executable, so it needs an ordinary build and
is skipped by `-R "test_"`.

## Writing a suite

1. Add the suite to `tests/CMakeLists.txt` with `ctr_add_test_suite(<name> SOURCES <file>)`, passing `LIBRARIES SDL3::SDL3` if it uses SDL. Keep the fixture, `WORKING_DIRECTORY` and output directory that the function sets; do not re-add them per suite.
2. Use the shared harness rather than a private one: `CTR_TEST_CHECK(condition, "what it should show")` counts a check whether or not it held, `CTR_TEST_SECTION("group")` heads a block, and `return CtrTest_Summary()` from `main`. `tests/shared/ctr_test_scratch.h` provides a disposable directory under the SDL pref path.
3. Write the assertion message as the claim, not the syntax: `"a disabled player has no device to pick"`, not `"count == 0"`. When a check is a proof against a tempting one-line mutation, say which mutation in the comment; that is the part that survives the next edit.

## The unity constraint

`global_variable` is `static` (`include/macros.h`), so a platform module's state
is file-static and no other translation unit can name it. `main.c` gets all the
sources into one unit; a suite cannot link against `ctr_native_expanded.exe` at all.

A suite that needs that state includes the module's own sources into its own
translation unit, in the same order `main.c` uses:

```c
#include <platform/native_config.c>
#include <platform/native_controls.c>
#include <platform/native_input.c>
```

Order matters where one module reads another. Then double whatever the module
calls but does not define — `NativeAssets_GetBaseDir`, `Platform_Log`,
`Platform_LogWarn`, `g_padCommEnable` and similar — with the real signature, or
the link fails. `tests/controls/test_native_controls.c` is the worked example.

## Rules

- Never let a suite read or write the working directory. The build tree holds `config.ini` and `controls.ini`; a suite that writes one will overwrite a real file. Point `NativeAssets_GetBaseDir` at `CtrTestScratch_Open` instead.
- A suite may reach into module internals, but only as a last resort. Reaching for `s_playerPref` or `s_capturePhase` is acceptable when there is no public accessor; note why in the comment, because that coupling is what forces the include chain.
- Keep the suite on the same warning set as the game. `ctr_native_apply_common_options` exists for exactly this, and it has already earned its place: `-Wformat-truncation` caught test storage over-allocated against a narrower field, where a too-long key would have been silently truncated inside the code under test.
- A suite must not depend on the game executable, the disc image, game assets or a window. Only `ctr_native_expanded_version` may run the game.
- Assert on the module's behaviour, not on the game's integration. Helpers can be pinned off-line; renderer, HUD and mode integration need `docs/REPLAYS.md` or a visual check (see the `debugging-and-validation` skill).
- Keep tests atomic and readable.

## After changing a suite

Run it before claiming it passes: `ctest --test-dir build --output-on-failure -R <name>`. Check the check count, not just the exit code — a suite that silently stopped checking is the failure mode that survives a refactor.