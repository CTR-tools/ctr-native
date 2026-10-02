#ifndef CTR_MAIN_LOAD_VLC_RETAIL_SYMBOLS_H
#define CTR_MAIN_LOAD_VLC_RETAIL_SYMBOLS_H

#include <common.h>

// NOTE(aalhendi): This unit loads the canonical BIGFILE pointer directly from
// its resident address instead of retaining the entire sData base in a register.
extern struct BigHeader *ctr_mainLoadVlcBigfile asm("sdata_static+304");
#define MAIN_LOAD_VLC_BIGFILE ctr_mainLoadVlcBigfile

#endif
