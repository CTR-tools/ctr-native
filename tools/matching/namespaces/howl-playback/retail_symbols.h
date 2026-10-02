#ifndef CTR_MATCHING_HOWL_PLAYBACK_RETAIL_SYMBOLS_H
#define CTR_MATCHING_HOWL_PLAYBACK_RETAIL_SYMBOLS_H

#include "../../retail_bindings.h"

#define GAME_PLAYBACK_VOLUME_FX          ctr_playbackVolumeFX
#define GAME_PLAYBACK_VOLUME_VOICE       ctr_playbackVolumeVoice
#define GAME_PLAYBACK_ENGINE_DISTORTIONS ctr_playbackEngineDistortions
#define GAME_PLAYBACK_ENGINE_FX_METADATA ctr_playbackEngineMetadata
#define GAME_PLAYBACK_OTHER_FX_METADATA  ctr_playbackOtherMetadata
#define GAME_PLAYBACK_CHANNEL_BACKUPS    ctr_playbackBackups
#define GAME_PLAYBACK_BACKUP_COUNT       ctr_playbackBackupCount

// NOTE(aalhendi): Playback accesses these scalars and pointers through $gp.
// Named small-data symbols receive their canonical addresses from link_symbols.
extern u8 ctr_playbackVolumeFX;
extern u8 ctr_playbackVolumeVoice;
extern struct SpuAddrEntry *ctr_playbackSpuAddrs;
extern struct EngineFX *ctr_playbackEngineMetadata asm("2148($28)");
extern struct OtherFX *ctr_playbackOtherMetadata asm("2156($28)");
extern s32 ctr_playbackBackupCount asm("2084($28)");
extern u8 ctr_playbackAudioEnabled asm("240($28)");

// Canonical aggregate views retain the retail absolute bases.
extern s32 ctr_playbackEngineDistortions[0x100] asm("data+7180");
extern struct ChannelStats ctr_playbackBackups[NUM_SFX_CHANNELS] asm("sdata_static+12384");

#undef GAME_AUDIO_ENABLED
#define GAME_AUDIO_ENABLED ctr_playbackAudioEnabled
#undef GAME_HOWL_SPU_ADDRS
#define GAME_HOWL_SPU_ADDRS ctr_playbackSpuAddrs

#endif
