#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80066e3c-0x80066e8c.
void DECOMP_VehStuckProc_MaskGrab_PhysLinear(struct Thread *t, struct Driver *d)
{
	DECOMP_VehPhysProc_Driving_PhysLinear(t, d);

	d->baseSpeed = 0;
	d->fireSpeed = 0;
	d->jump_TenBuffer = 0;

	// reset turning state
	d->simpTurnState = 0;

	d->actionsFlagSet &= ~(0x20024);
	d->actionsFlagSet |= 8;
}
