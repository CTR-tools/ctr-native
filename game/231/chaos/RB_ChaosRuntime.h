#ifndef CTR_NATIVE_RB_CHAOS_RUNTIME_H
#define CTR_NATIVE_RB_CHAOS_RUNTIME_H

#include <stdbool.h>
#include <chaos/chaos_effect.h>

struct Driver;

// Race-pool queries shared by the Chaos implementation modules.
int Chaos_PlayerPoolCount(void);
bool Chaos_HasEnabledPlayer(void);
// Unlike gameplay activation queries, this retains immunity while paused.
bool Chaos_PlayerHasTimeStop(int player);
// Includes effects retained while gameplay is paused.
bool Chaos_HasEffectForDriver(ChaosEffectId effect, const struct Driver *driver);
bool Chaos_ActivationTargetsDriver(ChaosEffectId effect, int player, const struct Driver *driver);

#endif
