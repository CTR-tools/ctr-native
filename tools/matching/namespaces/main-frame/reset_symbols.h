#ifndef CTR_MAIN_FRAME_RESET_SYMBOLS_H
#define CTR_MAIN_FRAME_RESET_SYMBOLS_H

#include <common.h>

// NOTE(aalhendi): ResetDB loads the resident bigfile pointer as a GP word.
extern struct BigHeader *ctr_mainFrameBigfile1;
#define MAIN_FRAME_BIGFILE1 ctr_mainFrameBigfile1

#endif
