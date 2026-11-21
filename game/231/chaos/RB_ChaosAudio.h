#ifndef CTR_NATIVE_RB_CHAOS_AUDIO_H
#define CTR_NATIVE_RB_CHAOS_AUDIO_H

#include <common.h>

void Chaos_ChangeEngineAudio(const struct Driver *driver, int previous, int next);
void Chaos_PlayTimeEffectSound(int sound);
void Chaos_TickTimeEffectSound(const struct ChaosActivation *entry, s32 elapsedMS);

#endif
