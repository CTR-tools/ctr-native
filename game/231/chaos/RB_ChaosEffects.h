#ifndef CTR_NATIVE_RB_CHAOS_EFFECTS_H
#define CTR_NATIVE_RB_CHAOS_EFFECTS_H

#include <stdbool.h>

struct ChaosRestart;
struct Driver;

void Chaos_ResetSteeringPose(struct Driver *driver);
void Chaos_EndRestartHold(struct ChaosRestart *restart, struct Driver *driver);
bool Chaos_IsBeingRescued(const struct Driver *driver);
void Chaos_FollowBody(int player, struct Driver *driver);

#endif
