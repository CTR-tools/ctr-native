#ifndef CTR_MATCHING_HOWL_ENGINE_RETAIL_SYMBOLS_H
#define CTR_MATCHING_HOWL_ENGINE_RETAIL_SYMBOLS_H

#define GAME_ENGINE_FX_METADATA       ctr_engineFxMetadata
#define GAME_ENGINE_VOLUME_FX         ctr_engineVolumeFx
#define GAME_ENGINE_DISTORTIONS       ctr_engineDistortions
#define GAME_ENGINE_AUDIO_DEFAULTS    ctr_engineAudioDefaults
#define GAME_ENGINE_DISTANCE_DEFAULTS ctr_engineDistanceDefaults
#define GAME_ENGINE_PAN_DEFAULTS      ctr_enginePanDefaults

#include "../../retail_bindings.h"

extern struct EngineFX *ctr_engineFxMetadata asm("sdata_static+2148");
extern u8 ctr_engineVolumeFx asm("sdata_static+2112");
extern s32 ctr_engineDistortions[] asm("data+7180");
// Retail loads these three defaults regions through separate bases.
extern s32 ctr_engineAudioDefaults[9] asm("sdata_static+2220");
extern s32 ctr_engineDistanceDefaults[2] asm("sdata_static+2228");
extern s32 ctr_enginePanDefaults[2] asm("sdata_static+2236");

#endif
