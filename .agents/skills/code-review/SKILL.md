---
name: code-review
description: Review Crash Team Racing Expanded changes for actionable bugs, regressions, and missing validation, using the project's architecture and subsystem risks.
---

# Code review

Use this skill when asked to review changes, a branch, a commit, or a pull request. Review the requested scope without changing code unless the user also asks for fixes.

1. Establish the scope and baseline from the request. Inspect the worktree status and the relevant diff, including staged, unstaged, and untracked files when reviewing local changes. For a branch or PR, compare against the specified base; if none is clear, identify the likely base and state the assumption. Do not alter or discard user changes.
2. Read the surrounding implementation, callers, and affected tests, not just the diff. Trace changed behavior through initialization, transitions, cleanup, and error paths. Load the relevant subsystem skills for cross-cutting changes; use `codebase-navigation` for ownership and `game-domain-knowledge` when judging gameplay fidelity.
3. Check applicable project-specific risks:
   - Unity build: include order, file-static name collisions, declarations, moved sources, and test/build paths. Respect the `game/` and `platform/` boundary and overlay ownership.
   - Memory and state: 32-bit layouts, MEMPACK lifetimes, scratchpad overlap, packed GPU links, pointer repair, and checkpoint/save-state restoration. Trace consumers before flagging a layout change.
   - Gameplay and presentation: mode and player-count gates, overlay transitions, per-viewport rendering and HUD, ordering tables, VRAM ownership, and behavior in adjacent screens or modes.
   - Configuration and I/O: existing and vanilla defaults, INI/menu consistency, compatibility with existing data, asset loading, input, and protection of real memory-card saves.
4. Review general organization and maintainability: place functions and files with their thematic and architectural owners; check whether long functions or crowded files obscure responsibilities and should be split. Look for existing functionality that should be reused instead of duplicated or reinvented, and for shared abstractions where they meaningfully remove duplication without coupling unrelated code or adding unnecessary indirection. Apply these checks proportionately to the change, not as arbitrary length limits or style-only findings.
5. Verify that the newly introduced code is readable and meaningful: report code that could be simplified or that doesn't serve any clear purpose.
6. Verify suspected issues against actual control flow and supported inputs. Run focused, non-destructive checks when useful and feasible; consult `testing` and `debugging-and-validation`. Builds compile tests but do not run them. Do not claim a check passed if it was not run, and distinguish unverified risks from confirmed defects.
7. Report actionable findings first, ordered by severity. For each, give a concise title, severity, precise file and line range, triggering conditions, and concrete impact. Prefer issues introduced by the change; avoid speculative problems, style-only feedback, and pre-existing defects unless directly relevant. Mention material test gaps and checks not run separately. If there are no findings, say so explicitly and summarize residual risks or validation gaps.
