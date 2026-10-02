#ifndef CTR_MATCHING_MAIN_DRAW_CB_RETAIL_SYMBOLS_H
#define CTR_MATCHING_MAIN_DRAW_CB_RETAIL_SYMBOLS_H

#include "../../retail_bindings.h"

// NOTE(aalhendi): The tracker load is gp-relative before the frame setup;
// the root-counter total uses an absolute page in this retail callback.
#undef GAME_TRACKER
#define GAME_TRACKER ctr_mainDrawCbGameTracker
#undef GAME_RCNT_TOTAL_UNITS
#define GAME_RCNT_TOTAL_UNITS ctr_mainDrawCbRcntTotalUnits

extern struct GameTracker *ctr_mainDrawCbGameTracker;
extern s32 ctr_mainDrawCbRcntTotalUnits asm("sdata_static+2588");

#endif
