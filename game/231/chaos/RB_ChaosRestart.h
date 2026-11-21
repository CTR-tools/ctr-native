#ifndef CTR_NATIVE_RB_CHAOS_RESTART_H
#define CTR_NATIVE_RB_CHAOS_RESTART_H

#include <stdbool.h>

struct ChaosRestart;
struct Driver;

void Chaos_ResetSteeringPose(struct Driver *driver);
void Chaos_EndRestartHold(struct ChaosRestart *restart, struct Driver *driver);
bool Chaos_IsBeingRescued(const struct Driver *driver);
void Chaos_RestartDriver(struct Driver *driver, struct ChaosRestart *restart);
void Chaos_UpdateRestarts(int elapsedMS);

#endif
