# Crash Team Racing Expanded Agent Guide

## Project Purpose

Crash Team Racing Expanded is a customizable native PC port of Crash Team Racing. It
builds on `CTR-tools/ctr-native`; the local fork adds display, gameplay, and
quality-of-life options.

## Architecture and skills

`main.c` includes the resident core/rendering chain in `game/game_unity.h` and
most platform implementations. Audio, HUD, vehicles and overlays use separate
subsystem unity sources listed in `CMakeLists.txt`; configuration and the OpenGL
loader also compile separately. Game code lives in `game/`, shared declarations
and layouts in
`include/`, and SDL3/OpenGL and host integrations in `platform/`.
`externals/SDL/` is vendored; do not edit it for application changes. This is a
32-bit C17 native target. CMake defines `CTR_INTERNAL`.

Module-owned shared constant groups live in small headers beside their game
implementations, with explicit consumer includes rather than global layout
namespace exposure. Keep private policy behind owner operations where useful.

Keep directories navigable as the project grows: reuse existing thematic
subdirectories and create new ones when a coherent set of related files would
otherwise crowd a directory. Avoid unnecessary nesting or grouping unrelated
code solely to reduce file counts. Preserve the game/platform boundary and
overlay ownership; when moving files, update unity includes, tests, build
paths, and documentation without changing include order.

Task-focused, provider-agnostic skills live under `.agents/skills/`. Each
`SKILL.md` has a short description and instructions. Read the relevant skill
when the task needs that subsystem; combine skills for cross-cutting work:

| Task | Skill |
|------|-------|
| Review changes for bugs and regressions | `.agents/skills/code-review/SKILL.md` |
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
- `config.ini`, `profile.ini`, and `errors.txt` are local only. Don't add them to commits.
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
- Don't reinvent the wheel, don't duplicate code. Try to reuse pre-existing functionality where it makes sense. 
- Extracting common functionality into a shared helper is allowed when it makes sense.
- Put new code where it belongs, not in random files because it seems conveniente. If necessary, you can create new thematic files with clear responsibilites.
