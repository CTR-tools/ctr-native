#ifndef CTR_MATCHING_HOWL_VOICELINE_RETAIL_SYMBOLS_H
#define CTR_MATCHING_HOWL_VOICELINE_RETAIL_SYMBOLS_H

// The named enable scalar permits the retail prologue and epilogue scheduling.
#define HOWL_VOICE_ENABLED  ctr_voiceEnabled
#define HOWL_VOICE_STATS    ctr_voiceStats
#define HOWL_VOICE_ATTR_CUR ctr_voiceAttrCur
#define HOWL_VOICE_TIMES1   ctr_voicePlayed
#define HOWL_VOICE_TIMES2   ctr_voiceTimes
#define HOWL_VOICE_FREE     ctr_voiceFree
#define HOWL_VOICE_TAKEN    ctr_voiceTaken
#define HOWL_VOICE_POOL     ctr_voicePool
#define HOWL_VOICE_BACKUP   ctr_voiceBackup
#define HOWL_VOICE_RNG      ctr_voiceRng
#define HOWL_VOICE_TYPES    ctr_voiceTypes
#define HOWL_VOICE_SETS     ctr_voiceSets

#include "../../retail_bindings.h"

extern struct ChannelStats ctr_voiceStats[NUM_SFX_CHANNELS] asm("sdata_static+36648");
extern struct ChannelAttr ctr_voiceAttrCur[NUM_SFX_CHANNELS] asm("sdata_static+12000");
extern s32 ctr_voicePlayed[16] asm("sdata_static+37592");
extern s32 ctr_voiceTimes[16] asm("sdata_static+37656");
extern struct LinkedList ctr_voiceFree asm("sdata_static+37432");
extern struct LinkedList ctr_voiceTaken asm("sdata_static+37448");
extern struct VoicelineItem ctr_voicePool[8] asm("sdata_static+37464");
extern u8 ctr_voiceEnabled;
extern s32 ctr_voiceBackup[4] asm("sdata_static+37416");
extern u32 ctr_voiceRng asm("sdata_static+236");
extern u8 ctr_voiceTypes[24] asm("data+12092");
extern s32 ctr_voiceSets[16] asm("data+12028");

#endif
