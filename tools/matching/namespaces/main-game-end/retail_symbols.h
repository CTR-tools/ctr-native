#ifndef CTR_MAIN_GAME_END_RETAIL_SYMBOLS_H
#define CTR_MAIN_GAME_END_RETAIL_SYMBOLS_H
#include "tools/matching/retail_bindings.h"
#undef GAME_TRACKER
extern struct GameTracker *ctr_mainGameEndTracker;
#define GAME_TRACKER ctr_mainGameEndTracker
extern struct HighScoreEntry *ctr_mainGameEndHighScores asm("sdata_static+1996");
extern u16 ctr_mainGameEndTropyBit asm("data+15260");
extern u16 ctr_mainGameEndOxideBit asm("data+15264");
extern u32 ctr_mainGameEndTropyFlash asm("data+15268");
extern u32 ctr_mainGameEndOxideFlash asm("data+15272");
#define MAIN_HIGH_SCORES                              ctr_mainGameEndHighScores
#define MAIN_LOAD_PROGRESS_PAGE_AFTER(page, mode)     CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(page, "sdata_static+6012", mode)
#define MAIN_PROGRESS_FROM_PAGE(result, page)         CTR_PSX_ADD_SYMBOL_LOW(result, page, "sdata_static+6012", &GAME_PROGRESS)
#define MAIN_LOAD_TROPY_PAGE(page)                    CTR_PSX_LOAD_SYMBOL_PAGE(page, "data+15260")
#define MAIN_LOAD_TROPY_LOW(result, page)             CTR_PSX_LOAD_UNSIGNED_HALF(result, page, 17724, ctr_mainGameEndTropyBit)
#define MAIN_READ_OXIDE_METADATA(metadata)            CTR_PSX_WITH_RETAIL_PAGE(metadata, 0x80080000, 17728, &ctr_mainGameEndOxideBit, *metadata)
#define MAIN_LOAD_TROPY_FLASH_PAGE_AFTER(page, flags) CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(page, "data+15268", flags)
#define MAIN_TROPY_FLASH_FROM_PAGE(page)              CTR_PSX_PAGE_LVALUE(u32, page, 17732, ctr_mainGameEndTropyFlash)
#define MAIN_LOAD_OXIDE_FLASH_PAGE_AFTER(page, flags) CTR_PSX_LOAD_SYMBOL_PAGE_AFTER(page, "data+15272", flags)
#define MAIN_OXIDE_FLASH_FROM_PAGE(page)              CTR_PSX_PAGE_LVALUE(u32, page, 17736, ctr_mainGameEndOxideFlash)

extern s8 ctr_mainGameEndKartSpawnOrder[8] asm("sdata_static+1840");
#define MAIN_KART_SPAWN_ORDER ctr_mainGameEndKartSpawnOrder
extern s16 ctr_mainGameEndReplayHumanGhost asm("sdata_static+2540");
#define MAIN_REPLAY_HUMAN_GHOST ctr_mainGameEndReplayHumanGhost
extern s16 ctr_mainGameEndGhostTooBig asm("sdata_static+2008");
#define MAIN_GHOST_TOO_BIG ctr_mainGameEndGhostTooBig
extern struct GhostHeader *ctr_mainGameEndPlayingGhost asm("sdata_static+2024");
#define MAIN_PLAYING_GHOST ctr_mainGameEndPlayingGhost

// NOTE(aalhendi): Read-only view of the canonical eight-byte SDATA seed.
// Native uses the source-local constant instead of mutable wheel state.
extern const struct MainGameEndTieSlots ctr_mainGameEndTieSeed asm("sdata_static+904");
#define MAIN_TIE_SEED ctr_mainGameEndTieSeed

#endif
