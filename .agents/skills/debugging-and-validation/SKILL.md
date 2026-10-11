---
name: debugging-and-validation
description: Validate focused CTR Native changes with builds, replays, quick states, and internal performance tools. For adding or changing tests, see the `testing` skill.
---

# Debugging and validation

Use this skill when reproducing a bug or choosing checks after a change.

1. Choose a 32-bit build appropriate to the host. On Windows, prefer `build.bat` (MinGW) or `build-msvc.bat` (MSVC); on Linux, use `build.sh`. These scripts are the normal build entry points and avoid an unnecessary first-time CMake preset configure, which can take a while when it configures vendored SDL. Use direct CMake/CTest commands when a build tree is already configured or a task specifically needs a preset/test configuration. The build scripts compile the game and the `tests/` suites but run no test; run them afterwards with `ctest --test-dir build --output-on-failure`. See the `testing` skill for writing or changing them.
2. For reproducible runtime bugs in internal builds, see `docs/REPLAYS.md`: F5/F8 quick state, `--record` (optionally `--toggle` and `--detailed`), and `--replay <path>`. Recording and playback use separate memory-card copies; do not modify a user's real saves when collecting a repro.
3. Inspect `platform/native_replay_scheduler.c`, `native_checkpoint.c`, `native_savestate.c`, and `native_state.c` when debugging state capture or frame scheduling. `platform/native_perf.c` and the scope markers in `game/MAIN/MainFrame_RenderFrame.c` can help isolate rendering performance.
4. Validate the concrete transition or screen where the bug occurs and one nearby mode/viewport; tests of helpers alone cannot prove renderer, HUD, or game-mode integration.
5. When a bug is about what covers what and the screen is hard to reach (needs a specific save, co-op setup, or long input sequence), model the emission order off-line first: mirror `ClearOTagR`, `AddPrim`/`AddPrimitive`, the `DecalHUD`/`DecalFont` prepends, and the `DrawOTag` walk from `RenderSubmit` over a small stand-in table, then print the resulting order. That reproduces the layering without launching the game, and it is usually enough to show which slot or call order is wrong.

Keep generated build output, local `config.ini`, replay reports, and user worktree changes intact unless asked otherwise.
