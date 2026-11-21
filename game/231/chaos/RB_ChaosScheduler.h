#ifndef CTR_NATIVE_RB_CHAOS_SCHEDULER_H
#define CTR_NATIVE_RB_CHAOS_SCHEDULER_H

#include <stdbool.h>

int Chaos_NextTriggerGap(void);
bool Chaos_TriggerForPlayer(int player);
void Chaos_UpdateSchedule(int elapsedMS);

#endif
