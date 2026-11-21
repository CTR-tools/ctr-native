#ifndef CTR_NATIVE_RB_CHAOS_REPOSITION_H
#define CTR_NATIVE_RB_CHAOS_REPOSITION_H

struct Driver;

enum { CHAOS_TIME_SKIP_PALETTE_MS = 500 };

void Chaos_DefectiveTeleportDriver(struct Driver *driver);
void Chaos_TeleportDriver(struct Driver *driver);
void Chaos_SkipBotTime(void);

#endif
