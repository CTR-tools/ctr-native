#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80063cf4-0x80063d44.
void VehPhysProc_SpinFirst_Update(struct Thread *t, struct Driver *d)
{
	int speedApprox;

	// cooldown after the spinning started
	if (d->NoInputTimer != 0)
	{
		// absolute value
		speedApprox = d->speedApprox;
		if (speedApprox < 0)
			speedApprox = -speedApprox;

		// quit if moving quickly
		if (speedApprox > 0x2ff)
			return;
	}

	// if speed has slown to near-halt,
	// or if NoInputTimer ran out
	VehPhysProc_SpinLast_Init(t, d);
}
