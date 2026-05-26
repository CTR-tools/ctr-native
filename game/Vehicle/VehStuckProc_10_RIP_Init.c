#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80067930-0x80067960.
void VehStuckProc_RIP_Init(struct Thread *t, struct Driver *d)
{
	VehStuckProc_PlantEaten_Init(t, d);
	d->invisibleTimer = 0;
	d->funcPtrs[1] = NULL;
	d->funcPtrs[11] = NULL;
}
