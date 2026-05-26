#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006402c-0x8006406c.
void VehPhysProc_SpinLast_Update(struct Thread *t, struct Driver *d)
{
	int driftAngle = d->turnAngleCurr;

	// if almost facing forward
	if ((driftAngle < 16) && (driftAngle > -16))
	{
		// stop spin
		VehPhysProc_SpinStop_Init(t, d);
	}
}
