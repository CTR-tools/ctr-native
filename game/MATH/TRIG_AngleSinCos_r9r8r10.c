#ifndef CTR_NATIVE
#include <ctr_types.h>

// NOTE(aalhendi): Tires pass t1 and receive signed t0/t2; only v1 also changes.
register u32 trigAngle CTR_PSX_REGISTER("$9");
register s32 trigSine CTR_PSX_REGISTER("$8");
register s32 trigCosine CTR_PSX_REGISTER("$10");

#include <common.h>

void TRIG_AngleSinCos_r9r8r10(void)
{
#include "TRIG_AngleSinCos.inc"
}
#endif
