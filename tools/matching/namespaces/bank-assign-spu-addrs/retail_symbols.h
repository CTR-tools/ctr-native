#ifndef CTR_MATCHING_BANK_ASSIGN_SPU_ADDRS_RETAIL_SYMBOLS_H
#define CTR_MATCHING_BANK_ASSIGN_SPU_ADDRS_RETAIL_SYMBOLS_H

// NOTE(aalhendi): This is the same byte as sdata->bankLoadStage. Treating the
// entry read as a small external lets GCC schedule it before the stack frame.
#define GAME_BANK_LOAD_STAGE_INITIAL ctr_bankLoadStageInitial

#include "../../retail_bindings.h"

extern u8 ctr_bankLoadStageInitial;

#endif
