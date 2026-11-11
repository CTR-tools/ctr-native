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

// NOTE(aalhendi): RenderFrame reads the language pointer from its existing GP slot.
extern char **ctr_mainFrameLanguageStrings;
#undef GAME_LANGUAGE_STRINGS
#define GAME_LANGUAGE_STRINGS ctr_mainFrameLanguageStrings

// NOTE(aalhendi): These typed views name existing storage, not new data.
// ErrorY stays incomplete so GCC does not mistake its eight bytes for GP data.
extern s16 mainFrameErrorPosY[] asm("data+14512");
extern s16 mainFrameUnpluggedIndices[6] asm("data+14500");
extern u16 mainFrameFinalLapTimers[4] asm("sdata_static+820");
extern char mainFrameLOD[8] asm("sdata_static+868");
extern s16 ctr_mainFramePulseCounter asm("sdata_static+2564");
extern Color ctr_mainFramePulseNormal asm("sdata_static+2528");
extern Color ctr_mainFramePulseGreen asm("sdata_static+2524");
extern u32 *ctr_mainFrameBorderColors[NUM_COLORS] asm("data+5072");
extern RECT mainFrameVideoSourceRect asm("sdata_static+2324");
extern s32 mainFrameDebugDisplayEnabled asm("sdata_static+132");
extern DISPENV mainFrameBlankDisplayEnv asm("sdata_static+5992");
#define MAIN_FRAME_ERROR_POS_Y           mainFrameErrorPosY
#define MAIN_FRAME_UNPLUGGED_INDICES     mainFrameUnpluggedIndices
#define MAIN_FRAME_FINAL_LAP_TIMERS      mainFrameFinalLapTimers
#define MAIN_FRAME_LOD                   mainFrameLOD
#define MAIN_FRAME_PULSE_COUNTER         ctr_mainFramePulseCounter
#define MAIN_FRAME_PULSE_NORMAL          ctr_mainFramePulseNormal
#define MAIN_FRAME_PULSE_GREEN           ctr_mainFramePulseGreen
#define MAIN_FRAME_BORDER_COLORS         ctr_mainFrameBorderColors
#define MAIN_FRAME_VIDEO_SOURCE_RECT     mainFrameVideoSourceRect
#define MAIN_FRAME_DEBUG_DISPLAY_ENABLED mainFrameDebugDisplayEnabled
#define MAIN_FRAME_BLANK_DISPLAY_ENV     mainFrameBlankDisplayEnv

#endif
