#include <common.h>

void DECOMP_VehStuckProc_RIP_Init(struct Thread *t, struct Driver *d)
{
#ifdef CTR_NATIVE
	// NOTE(aalhendi): PC build shim until the full PlantEaten stuck-proc
	// cluster is wired into native; retail calls VehStuckProc_PlantEaten_Init.
	DECOMP_VehPhysProc_FreezeEndEvent_Init(t, d);
#else
	VehStuckProc_PlantEaten_Init(t, d);
#endif
	d->invisibleTimer = 0;
	d->funcPtrs[1] = NULL;
	d->funcPtrs[11] = NULL;
}
