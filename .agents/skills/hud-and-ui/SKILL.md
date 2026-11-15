---
name: hud-and-ui
description: Add or change HUD elements, menu screens, primitives, race overlays, and viewport-specific UI behavior.
---

# HUD and UI

Use this skill for heads-up display, 2D drawing, or menus.

1. Start with `game/MAIN/MainFrame_RenderFrame.c` (`RenderAllHUD`, `RenderDispEnv_UI`) and `game/UI/UI_RenderFrame.c` for racing, adventure hub, crystal challenge, and multiplayer HUD paths. Menu code is chiefly in `game/230/`; the configurable Options menu is `game/230/config/MM_ConfigMenu.c`.
2. Put drawing routines in the relevant `game/UI/` file and declare public game functions in `include/functions.h`. HUD positions come from the per-player `data.hudStructPtr` layouts; use `UI_HUD_SLOT_*` and `UI_HUD_SLOT_COUNT` from `include/namespace_UI.h` rather than numeric offsets.
3. Emit graphics through the existing primitive allocation/ordering-table path (`GetPrimMem`, `AddPrimitive`, `gGT->pushBuffer_UI.ptrOT` or the appropriate per-player push buffer). Follow neighboring routines for clipping, scale, and primitive lifetime; do not bypass the rendering pipeline with direct GL drawing.
4. Layering comes from which ordering-table slot you submit to, not from call order. `ClearOTagR` links every slot to the slot below it and `RenderSubmit` starts `DrawOTag` at `&pushBuffer[0].ptrOT[0x3ff]`, so the chain is emitted from the highest slot down to slot 0, and every primitive is prepended into its slot. A higher slot index is therefore a lower layer.
5. Inside one slot the last primitive submitted is emitted first, so submission order is reversed within a layer. This is why `UI_DrawSpeedBG` runs after the speedometer needle, and why a draw call made late in the frame can land underneath an earlier one. When two primitives overlap, change the slot or the submission order rather than nudging coordinates.
6. The UI table (`gGT->pushBuffer_UI.ptrOT`, the same pointer as `backBuffer->otMem.uiOT`) is five slots at `otSwapchainDB + 4`: slot 0 is the default layer for HUD, prompt, and menu primitives, slot 3 carries the boxes and bars drawn under their own contents, and slot 4 is the lowest layer holding the UI draw environment and full-screen backgrounds such as `ElimBG`. `UI_HUD_ORDERING_SLOT_LOWEST` in `include/namespace_UI.h` names that last slot. The per-viewport tables that follow at `+0x18` are separate depth-bucket tables, not layers.
7. To submit to a layer other than the default, pass `&gGT->pushBuffer_UI.ptrOT[n]` as the `ot` argument, or use `DecalFont_DrawLineOT`, which swaps the UI table pointer for the duration of the call.
8. Explicitly decide visibility in 1P and 2P–4P, battle and crystal modes, start/end-of-race, adventure hub, pause, and menu states. Use `gGT->hudFlags` and mode bits from `include/namespace_Main.h`. `UI_RenderFrame_Racing` and `UI_RenderFrame_CrystChall` have distinct call sites.
9. For an adjustable HUD feature, follow the runtime-configuration skill. The existing reserves meter is drawn from `game/UI/UI_Meter.c` and gated by `g_config.showReservesMeter` in the HUD paths.

Visually verify placement, layering, and visibility in each affected viewport/mode; check configuration-enabled and default-disabled behavior where applicable.
