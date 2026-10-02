#ifndef CTR_MAIN_FRAME_RETAIL_SYMBOLS_H
#define CTR_MAIN_FRAME_RETAIL_SYMBOLS_H

#include <common.h>
#include "video_symbols.h"
#include "reset_symbols.h"

// NOTE(aalhendi): The frame code imports the existing four-byte resident slots.
extern struct GameTracker *ctr_mainFrameGameTracker;
extern struct GamepadSystem *ctr_mainFrameGamepads;
#undef GAME_TRACKER
#define GAME_TRACKER ctr_mainFrameGameTracker
#undef GAMEPADS
#define GAMEPADS ctr_mainFrameGamepads

// NOTE(aalhendi): These three accesses use absolute resident pages in retail.
extern struct RectMenu *ctr_mainFrameActiveMenu asm("sdata_static+2460");
extern s32 ctr_mainFrameAnyTap asm("sdata_static+2532");
extern b16 ctr_mainFrameSaveCup asm("sdata_static+2476");
#define MAIN_FRAME_ACTIVE_MENU ctr_mainFrameActiveMenu
#define MAIN_FRAME_ANY_TAP     ctr_mainFrameAnyTap
#define MAIN_FRAME_SAVE_CUP    ctr_mainFrameSaveCup

// NOTE(aalhendi): These resident calls carry the current tracker in a0.
extern void ctr_mainFrameBotsUpdate(struct GameTracker *) asm("BOTS_UpdateGlobals");
extern void ctr_mainFrameParticleUpdate(struct GameTracker *) asm("Particle_UpdateAllParticles");
#define MAIN_FRAME_BOTS_UPDATE(tracker)     ctr_mainFrameBotsUpdate(tracker)
#define MAIN_FRAME_PARTICLE_UPDATE(tracker) ctr_mainFrameParticleUpdate(tracker)

#endif
