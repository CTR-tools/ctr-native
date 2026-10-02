#ifndef CTR_MATCHING_HOWL_AUDIO_STATE_RETAIL_SYMBOLS_H
#define CTR_MATCHING_HOWL_AUDIO_STATE_RETAIL_SYMBOLS_H

// Private linkage aliases retain the canonical field widths and retail addresses.
#define GAME_AUDIO_STATE        ctr_audioState
#define GAME_ADV_HUB_SONG_SET   (&ctr_audioAdvHubSongSet)
#define GAME_REVERB_BOSS        ctr_reverbBoss
#define GAME_REVERB_LEVEL       ctr_reverbLevel
#define GAME_SOUND_FADE_0       ctr_soundFade0
#define GAME_SOUND_FADE_1       ctr_soundFade1
#define GAME_AUDIO_DEFAULT_0    ctr_audioDefault0
#define GAME_AUDIO_DEFAULT_1    ctr_audioDefault1
#define GAME_AUDIO_DEFAULT_2    ctr_audioDefault2
#define GAME_AUDIO_DEFAULT_3    ctr_audioDefault3
#define GAME_AUDIO_DEFAULT_4    ctr_audioDefault4
#define GAME_AUDIO_DEFAULT_5    ctr_audioDefault5
#define GAME_AUDIO_DEFAULT_6    ctr_audioDefault6
#define GAME_AUDIO_DEFAULT_7    ctr_audioDefault7
#define GAME_AUDIO_DEFAULT_8    ctr_audioDefault8
#define GAME_XA_CATEGORY        ctr_audioXACategory
#define GAME_XA_PAUSE_FRAME     ctr_audioXAPauseFrame
#define GAME_XA_OFFSET          ctr_audioXAOffset
#define GAME_NTROPY_XA          ctr_audioNTropyXA
#define GAME_NTROPY_VOICE_COUNT ctr_audioNTropyVoiceCount

#include "../../retail_bindings.h"

extern AudioState ctr_audioState;
extern struct SongSet ctr_audioAdvHubSongSet asm("sdata_static+252");
extern char ctr_reverbBoss[] asm("sdata_static+260");
extern char ctr_reverbLevel[] asm("data+12148");
extern struct SoundFadeInput ctr_soundFade0 asm("sdata_static+37720");
extern struct SoundFadeInput ctr_soundFade1 asm("sdata_static+37736");
extern s32 ctr_audioDefault0;
extern s32 ctr_audioDefault1;
extern s32 ctr_audioDefault2;
extern s32 ctr_audioDefault3;
extern s32 ctr_audioDefault4;
extern s32 ctr_audioDefault5;
extern s32 ctr_audioDefault6;
extern u8 ctr_audioDefault7;
extern s32 ctr_audioDefault8;
extern s32 ctr_audioXACategory asm("sdata_static+1912");
extern s32 ctr_audioXAPauseFrame asm("sdata_static+1920");
extern s32 ctr_audioXAOffset asm("sdata_static+1932");
extern s16 ctr_audioNTropyXA[] asm("data+12136");
extern s16 ctr_audioNTropyVoiceCount;

#endif
