#ifndef CTR_NATIVE_RB_CHAOS_BODY_SWAP_H
#define CTR_NATIVE_RB_CHAOS_BODY_SWAP_H

struct Driver;

void Chaos_FollowBody(int player, struct Driver *driver);
int Chaos_CollectBodySwapPartners(int player, int *candidates);
void Chaos_BeginBodySwapPair(int player, int partner);
void Chaos_BeginBodySwaps(void);
void Chaos_ReturnBodySwap(int player);
void Chaos_ReturnBodySwaps(void);

#endif
