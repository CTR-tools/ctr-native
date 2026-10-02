#ifndef CTR_MATCHING_HOWL_GARAGE_RETAIL_SYMBOLS_H
#define CTR_MATCHING_HOWL_GARAGE_RETAIL_SYMBOLS_H

#define HOWL_GARAGE_SOUND_IDS ctr_garageSoundIDs
#define HOWL_GARAGE_RNG_READ  ctr_garageRngRead
#include "../../retail_bindings.h"

// NOTE(aalhendi): Symbolic reads retain separate absolute address materialization.
extern u8 ctr_garageSoundIDs[8] asm("sdata_static+276");
extern u32 ctr_garageRngRead asm("sdata_static+236");

// NOTE(aalhendi): The canonical RNG offset permits the retail store expansion through at.
#define HOWL_GARAGE_RNG_WRITE (*(u32 *)(0x8008cf6cu + OFFSETOF(struct sData, audioRNG)))

#endif
