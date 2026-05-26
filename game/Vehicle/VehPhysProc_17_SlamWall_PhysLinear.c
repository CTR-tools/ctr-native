#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063b00-0x80063b2c.
void VehPhysProc_SlamWall_PhysLinear(struct Thread *t, struct Driver *d)
{
	VehPhysProc_Driving_PhysLinear(t, d);

	// baseSpeed and fireSpeed
	// set both "shorts" in one "int"
	*(int *)&d->baseSpeed = 0;
}
