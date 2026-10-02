#ifndef CTR_MAIN_RACE_TRACK_RETAIL_SYMBOLS_H
#define CTR_MAIN_RACE_TRACK_RETAIL_SYMBOLS_H

#include <common.h>

// NOTE(aalhendi): Ordinary sized symbols retain the resident GP-relative
// accesses and keep the requested-level slot distinct from the stack.
// These declarations name existing storage and allocate no game data.
#undef GAME_TRACKER
extern struct GameTracker *ctr_mainRaceTrackGameTracker;
#define GAME_TRACKER ctr_mainRaceTrackGameTracker

extern u32 ctr_mainRaceTrackRequestedLevel;
#define MAIN_REQUESTED_LEVEL ctr_mainRaceTrackRequestedLevel

#endif
