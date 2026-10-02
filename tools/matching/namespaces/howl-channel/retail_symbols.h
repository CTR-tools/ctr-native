#ifndef CTR_MATCHING_HOWL_CHANNEL_RETAIL_SYMBOLS_H
#define CTR_MATCHING_HOWL_CHANNEL_RETAIL_SYMBOLS_H

// Named entry scalars allow their loads before the resident prologues.
#define HOWL_CHANNEL_CRITICAL_INITIAL ctr_channelCriticalInitial
#define HOWL_CHANNEL_AUDIO_INITIAL    ctr_channelAudioInitial
#define HOWL_CHANNEL_ATTR_CUR         ctr_channelAttrCur
#define HOWL_CHANNEL_OPCODE_FUNCS     ctr_channelOpcodeFuncs
#define HOWL_CHANNEL_OPCODE_OFFSETS   ctr_channelOpcodeOffsets

#include "../../retail_bindings.h"

extern s32 ctr_channelCriticalInitial;
extern u8 ctr_channelAudioInitial;
extern struct ChannelAttr ctr_channelAttrCur[NUM_SFX_CHANNELS] asm("sdata_static+12000");
extern void (*ctr_channelOpcodeFuncs[11])(struct SongSeq *) asm("data+9828");
extern s32 ctr_channelOpcodeOffsets[11] asm("data+9872");

#endif
