#ifndef CTR_MATCHING_HOWL_LOAD_RETAIL_SYMBOLS_H
#define CTR_MATCHING_HOWL_LOAD_RETAIL_SYMBOLS_H

// NOTE(aalhendi): Named scalars permit the resident entry loads before the prologue.
// Their linker addresses alias the canonical sData fields; the buffer uses an absolute address.
#define HOWL_LOAD_AUDIO_ENABLED_INITIAL ctr_howlLoadAudioEnabledInitial
#define HOWL_LOAD_STAGE_INITIAL         ctr_howlLoadStageInitial

#define HOWL_LOAD_SONG_BUFFER           ctr_howlLoadSongBuffer

#include "../../retail_bindings.h"

extern u8 ctr_howlLoadAudioEnabledInitial;
extern u8 ctr_howlLoadStageInitial;
extern char ctr_howlLoadSongBuffer[0x800];

#endif
