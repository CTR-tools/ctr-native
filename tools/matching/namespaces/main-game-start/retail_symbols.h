#ifndef CTR_MATCHING_MAIN_GAME_START_RETAIL_SYMBOLS_H
#define CTR_MATCHING_MAIN_GAME_START_RETAIL_SYMBOLS_H

struct GameTracker;
extern struct GameTracker *ctr_mainGameStartGameTracker;

// NOTE(aalhendi): Bind the resident pointer slot at gp+0x340 independently of its value.
#define GAME_TRACKER ctr_mainGameStartGameTracker

#endif
