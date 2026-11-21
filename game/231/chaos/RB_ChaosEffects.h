#ifndef CTR_NATIVE_RB_CHAOS_EFFECTS_H
#define CTR_NATIVE_RB_CHAOS_EFFECTS_H

struct Driver;

void Chaos_SetDrivingClass(struct Driver *driver, int next);
void Chaos_FollowBody(int player, struct Driver *driver);

#endif
