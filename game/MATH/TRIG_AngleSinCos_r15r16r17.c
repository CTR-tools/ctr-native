#ifndef CTR_NATIVE
#include <ctr_types.h>

// NOTE(aalhendi): Confetti passes t7 and receives signed s0/s1; only v1 also changes.
register u32 trigAngle CTR_PSX_REGISTER("$15");
register s32 trigSine CTR_PSX_REGISTER("$16");
register s32 trigCosine CTR_PSX_REGISTER("$17");

#include <common.h>

void TRIG_AngleSinCos_r15r16r17(void)
{
#include "TRIG_AngleSinCos.inc"
}
#endif
