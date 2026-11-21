#ifndef CTR_NATIVE_RB_CHAOS_ITEMS_H
#define CTR_NATIVE_RB_CHAOS_ITEMS_H

struct Driver;
struct Instance;

void Chaos_ClearItems(struct Driver *driver);
void Chaos_GiveRandomItem(struct Driver *driver);
void Chaos_SendWarpball(void);
void Chaos_GiveWumpas(struct Driver *driver);
void Chaos_GiveMask(struct Driver *driver);
void Chaos_GiveShield(struct Driver *driver);
void Chaos_GiveReserves(struct Driver *driver);
void Chaos_UpgradeTurbo(struct Driver *driver);
void Chaos_ApplyClockCurse(struct Driver *driver, int durationMS);
void Chaos_ExplodeCrate(struct Driver *driver, struct Instance *crateInst);
void Chaos_TickWumpaDrain(int elapsedMS, struct Driver *driver);

#endif
