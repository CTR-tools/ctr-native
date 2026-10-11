# CTR Native Expanded Agent Guide

## Project Purpose

CTR Native Expanded is a customizable native PC port of Crash Team Racing. It
builds on `CTR-tools/ctr-native`; the local fork adds display, gameplay, and
quality-of-life options.

## Architecture and skills

`main.c` includes `game/game_unity.h` and the platform implementation in a
unity build. Game code lives in `game/`, shared declarations and layouts in
`include/`, and SDL3/OpenGL and host integrations in `platform/`.
`externals/SDL/` is vendored; do not edit it for application changes. This is a
32-bit C17 native target. CMake defines `CTR_INTERNAL`.

Task-focused, provider-agnostic skills live under `.agents/skills/`. Each
`SKILL.md` has a short description and instructions. Read the relevant skill
when the task needs that subsystem; combine skills for cross-cutting work:

| Task | Skill |
|------|-------|
| Explore feature ideas and prior art | `.agents/skills/reference-sources/SKILL.md` |
| Retail game rules, modes, characters, items, known quirks | `.agents/skills/game-domain-knowledge/SKILL.md` |
| Find code, add source, build architecture and overlays | `.agents/skills/codebase-navigation/SKILL.md` |
| Render targets, VRAM, scaling, presentation, level geometry | `.agents/skills/rendering/SKILL.md` |
| Race/battle/adventure modes, transitions, player counts | `.agents/skills/game-modes/SKILL.md` |
| Resident layouts, MEMPACK, scratchpad, primitive links | `.agents/skills/memory-model/SKILL.md` |
| INI options and in-game config menu | `.agents/skills/runtime-configuration/SKILL.md` |
| HUD and menus | `.agents/skills/hud-and-ui/SKILL.md` |
| Disc/extracted assets, CD, audio, input, memory cards | `.agents/skills/assets-and-io/SKILL.md` |
| Builds, quick states, bug replays, performance | `.agents/skills/debugging-and-validation/SKILL.md` |
| Add or change focused tests, run them | `.agents/skills/testing/SKILL.md` |

Treat the skills as task-specific maps into the source, not as substitutes for
reading the implementation. The current PS1-shaped rendering and memory paths
describe existing behavior; future work can move beyond PS1 constraints when
all affected consumers are updated coherently. For new options, default to
vanilla behavior unless the user requests otherwise; keep existing defaults.

## Git And Collaboration

- `origin` is the personal fork; `upstream` is `CTR-tools/ctr-native`. Never
  push to upstream.
- `master` is protected. Make a topic branch and merge through a pull request.
- `config.ini` and `errors.txt` are local only. Don't add them to commits.
- Preserve user changes and untracked files. Do not reset, clean, or discard
  worktree changes without explicit approval.
- Do not commit changes, stage them and let the user commit.

## Source Attribution

When porting a feature from another project or fork, retain concise code
attribution where appropriate and add README credit when the user requests it.
Avoid importing broad bundled commits when only one independent feature is
needed; isolate the relevant implementation and preserve its provenance in the
commit message.

## Coding Style

- Prioritize readability; split functions that become too long.
- Always use braces for `if` statements, including single-statement bodies.
- Use named constants and enums instead of magic numbers.
- Fix underlying issues rather than reverting a requested feature when an initial approach fails.
- Keep host-specific code behind the `platform/` boundary; there is one build
  target, so do not reintroduce conditional compilation forks for application
  code.
- Avoid redundant parentheses.
- Don't overexplain and don't repeat explanations over and over in comments.