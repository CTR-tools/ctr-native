#ifndef CTR_NATIVE_RB_CHAOS_RUNTIME_H
#define CTR_NATIVE_RB_CHAOS_RUNTIME_H

#include <stdbool.h>

// Race-pool queries shared by the Chaos implementation modules.
int Chaos_PlayerPoolCount(void);
// Unlike gameplay activation queries, this retains immunity while paused.
bool Chaos_PlayerHasTimeStop(int player);

#endif
