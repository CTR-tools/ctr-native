#ifndef CTR_NATIVE_RB_CHAOS_EFFECTS_H
#define CTR_NATIVE_RB_CHAOS_EFFECTS_H

struct Driver;

void Chaos_SetDrivingClass(struct Driver *driver, int next);
void Chaos_ResetKartScale(struct Driver *driver);
void Chaos_RefreshBodySwapModifiers(void);

#endif
