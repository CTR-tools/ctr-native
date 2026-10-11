---
name: rendering
description: Work on CTR Native's game render submission, OpenGL renderer, render scaling, VRAM feedback, presentation, and split-screen geometry.
---

# Rendering pipeline

Use this skill for graphics, resolution, clipping, feedback, texture, or display changes.

1. Trace the game side in `game/MAIN/MainFrame_RenderFrame.c` (`RenderAllHUD`, render buckets, level geometry, `RenderDispEnv_UI`, `RenderSubmit`), `game/Display.c`, `game/PushBuffer.c`, `game/RenderBucket/`, and `game/226/`–`game/229/`. The backend lives in `platform/native_gpu.c`, `platform/native_libgpu.c`, and `platform/native_renderer.c`.
2. Identify the coordinate space: logical display/draw rectangles for game vertices and UI; scaled RGBA main target; PS1-sized offscreen feedback target; persistent 1024×512 packed-pixel VRAM texture with a CPU mirror; letterboxed host viewport. `include/platform/native_render_scale.h` documents and calculates the mappings.
3. For main-target sizing, follow `NativeRenderer_PrepareMainRenderTarget`, projection, scissor/clear scaling, `NativeRenderer_StoreFrameBuffer`, VRAM load/update, and presentation together. `Original` uses VRAM presentation; `2X`/`3X`/`4X`/`Native` present the main target directly while still updating VRAM for feedback and reads. Native sizing follows the presentation viewport. Inspect `NativeRenderer_SetOffscreenState` for heat/warp/clock effects.
4. For VRAM correctness, respect CPU-dirty rect uploads and GPU-newer regions: `NativeRenderer_UpdateVRAM`, `NativeRenderer_ResolveVRAMRead`, `NativeRenderer_ReadVRAM`, and framebuffer packing. Save-state capture resolves GPU-authored VRAM first. GPU primitive ordering-table links use the token bridge in `platform/native_gpu_links.c`, not host pointers truncated to 24 bits. `ParsePrimitivesLinkedList` emits the chain in link order from the entry slot `RenderSubmit` passes, so what reaches that entry first ends up behind; the `hud-and-ui` skill covers the slot layout this produces.
5. For detail/visibility or split-screen work, inspect `gGT->renderFlags` in `include/namespace_Main.h`, `MainFrame_RenderFrame.c`, the 1P–4P renderers, and `LOAD_OvrLOD` before changing the OpenGL backend.

Run `tests/test-render-scale.c` for mapping changes (`ctest -R render_scale`). Visually check 1P and split-screen, scaled modes, UI, feedback effects, and VRAM-copy scenes for renderer changes. The PS1-sized stages describe the current implementation, not a requirement for future features: if removing a constraint, migrate all consumers coherently.
