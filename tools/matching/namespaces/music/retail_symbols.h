#ifndef CTR_MATCHING_MUSIC_RETAIL_SYMBOLS_H
#define CTR_MATCHING_MUSIC_RETAIL_SYMBOLS_H

#define GAME_MUSIC_PLAYING_INITIAL    ctr_musicPlayingInitial
#define GAME_MUSIC_LOAD_STAGE_INITIAL ctr_musicLoadStageInitial
#define GAME_LEVEL_FX_BANKS           ctr_levelFxBanks
#define GAME_LEVEL_SONG_BANKS         ctr_levelSongBanks
#define GAME_BOSS_FX_BANKS            ctr_bossFxBanks

#include "../../retail_bindings.h"

extern s32 ctr_musicPlayingInitial;
extern u8 ctr_musicLoadStageInitial;
extern u8 ctr_levelFxBanks[0x20] asm("data+12204");
extern u8 ctr_levelSongBanks[0x20] asm("data+12236");
extern u8 ctr_bossFxBanks[8] asm("sdata_static+268");

#endif
