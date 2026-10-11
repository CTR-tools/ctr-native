---
name: reference-sources
description: Find and compare useful repository, retail/decompilation, upstream, and community references when exploring ideas for a feature or design.
---

# Consult reference sources for ideas

Use this skill when looking for inspiration, prior art, expected behavior, or implementation approaches. Treat references as leads to investigate, not instructions to copy.

## Recommended external references

Use these as starting points; check activity, branch, release, and version before relying on details.

| Source | What it is useful for | Important context |
|--------|-----------------------|------------------|
| [CTR-ModSDK](https://github.com/CTR-tools/CTR-ModSDK) — especially [`mods/`](https://github.com/CTR-tools/CTR-ModSDK/tree/main/mods) | CTR-specific C mods, gameplay/UI ideas, decompilation history, and examples such as [OxideFix](https://github.com/CTR-tools/CTR-ModSDK/tree/main/mods/Modules/OxideFix) | Mods target the original game/runtime and may depend on PS1 addresses, hooks, or toolchain details. Port the behavior and relevant logic, not those assumptions blindly. |
| [CTR Archipelago native fork](https://github.com/dowlle/ctr-native-ap) and its [player/reference guide](https://ap-pie.com/ctr) | A substantial native CTR feature set: randomized Adventure progression, checks, kart upgrades, traps, connection/UI, and how a community-facing feature is explained | It is a separate fork with its own architecture and release versions. Compare its source and history with this checkout before borrowing implementation. The guide describes current product behavior; it is not a specification for this project. |
| [CTR Native upstream](https://github.com/CTR-tools/ctr-native) | Current decompilation fixes, native implementation patterns, renderer/gameplay changes, and divergence checks | This project is a fork; compare relevant files/commits rather than assuming upstream has the same options or platform behavior. |
| [CTR-tools](https://github.com/CTR-tools/CTR-tools) | CTR-specific file formats and tools to inspect and extract assets from the game, including models and track data | Use it when an asset is needed for inspection, comparison, or modding. Verify extracted formats and interpretations against this project's loaders and asset override behavior. |
| [CTR-LevEdit](https://github.com/CTR-tools/CTR-LevEdit) and [io_ctr_tools](https://github.com/CTR-tools/io_ctr_tools) | Track/level authoring workflows, geometry import, and editing ideas | Level-editor data workflows may not map directly to runtime rendering or native asset loading. |
| [CrashTeamEditor](https://github.com/mateusfavarin/CrashTeamEditor) | Track editing, level-data inspection, and its Python bindings/API; useful for track-tooling ideas and understanding editable CTR track structures | An editor/tooling project, not a game runtime implementation. Confirm format and API assumptions against CTR-tools and this repository's actual loaders. |
| [SDL3 documentation](https://wiki.libsdl.org/SDL3/) | SDL windowing, input, controller, audio, and platform API behavior used by this native application | Prefer the SDL version vendored in `externals/SDL/` when API/version details differ. |
| [OpenGL reference pages](https://registry.khronos.org/OpenGL-Refpages/) | GL state, framebuffer, texture, and shader API semantics for native renderer ideas | Check the project's supported GL context and existing wrappers before choosing an API. |

## How to consult them

1. **Current project behavior first:** search `game/`, `platform/`, `include/`, `docs/`, `README.md`, and `tests/`. Similar features, adjacent modes, native adapters, and focused tests are usually the most directly applicable references. Follow call sites and consumers, not just symbol names.
2. **Repository history and metadata:** inspect `git log` and blame for provenance and implementation context. The retail matching pipeline, its metadata, and its vendored Linux toolchains are not part of this fork; `upstream` (`CTR-tools/ctr-native`) still carries them if that evidence is needed. Treat inferred names as evidence to verify.
3. **Choose the external reference by question:** use the table above for prior-art candidates; prefer source, relevant issue/PR discussions, and release notes for implementation facts. Record an exact URL and revision or release when a reference informs a design.
4. **Compare assumptions:** distinguish observed behavior, documented intent, and a design idea. Check game mode, player count, data/layout version, target platform, memory ownership, coordinate space, configuration defaults, and whether the source assumes PS1 hardware or another renderer.
5. **Apply narrowly:** validate promising ideas against this checkout's call graph and tests. Keep host-specific behavior in `platform/` and game rules in `game/`. Preserve provenance with concise code attribution where appropriate; prefer adapting the smallest relevant idea over copying a broad fork.
6. **Report useful references:** give links or local paths, what each contributed, and unresolved differences. Do not present an unverified lead as confirmed behavior.

## Compare and apply references

- Separate **observed behavior**, **documented intent**, and **your design idea**. A retail implementation detail, a mod's choice, and this fork's intended behavior are not automatically the same requirement.
- Compare assumptions: game mode, player count, data/layout version, target platform, memory ownership, coordinate space, and configuration defaults. Check whether the reference assumes PS1 hardware or a different native renderer.
- Validate promising ideas against this checkout's call graph and tests before adapting them. Keep host-specific behavior in `platform/` and game rules in `game/`.
- Preserve provenance for borrowed implementation details with concise code attribution where appropriate. Add README credit when requested; prefer independently applying an idea over copying large unrelated patches.
- Summarize useful references in the response with links or local paths, what each contributed, and any unresolved differences. Do not present an unverified lead as confirmed behavior.
