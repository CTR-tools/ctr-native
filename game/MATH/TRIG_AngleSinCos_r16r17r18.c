#ifndef CTR_NATIVE
#include <ctr_types.h>

// NOTE(aalhendi): Weather passes s0 and receives signed s1/s2; only v1 also changes.
register u32 trigAngle CTR_PSX_REGISTER("$16");
register s32 trigSine CTR_PSX_REGISTER("$17");
register s32 trigCosine CTR_PSX_REGISTER("$18");

#include <common.h>

void TRIG_AngleSinCos_r16r17r18(void)
{
#include "TRIG_AngleSinCos.inc"
}
#endif
