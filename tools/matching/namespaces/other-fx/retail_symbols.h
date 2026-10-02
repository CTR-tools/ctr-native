#ifndef CTR_MATCHING_OTHER_FX_RETAIL_SYMBOLS_H
#define CTR_MATCHING_OTHER_FX_RETAIL_SYMBOLS_H

// Resident OtherFX uses absolute addresses for these sdata fields.
#define HOWL_OTHER_FX_METADATA     ctr_otherFxMetadata
#define HOWL_OTHER_FX_VOLUME       ctr_otherFxVolume
#define HOWL_OTHER_FX_VOICE_VOLUME ctr_otherFxVoiceVolume

#include "../../retail_bindings.h"

extern struct OtherFX *ctr_otherFxMetadata asm("sdata_static+2156");
extern u8 ctr_otherFxVolume asm("sdata_static+2112");
extern u8 ctr_otherFxVoiceVolume asm("sdata_static+2128");

#endif
