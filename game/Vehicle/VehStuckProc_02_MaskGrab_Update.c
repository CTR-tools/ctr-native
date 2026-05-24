#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80066d4c-0x80066e3c.
void DECOMP_VehStuckProc_MaskGrab_Update(struct Thread *t, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;

	d->NoInputTimer -= gGT->elapsedTimeMS;

	if (d->NoInputTimer < 0)
		d->NoInputTimer = 0;

	if (d->NoInputTimer != 0)
		return;

	// when input is allowed,
	// which is when driver is spawned back over track

	struct MaskHeadWeapon *mask = d->KartStates.MaskGrab.maskObj;

	if (mask != NULL)
	{
		// mask rotZ
		mask->rot[2] &= ~(1);

		// scale = 100%
		mask->scale = 0x1000;
	}


	// CameraDC flag
	gGT->cameraDC[d->driverID].flags |= 8;


	DECOMP_VehStuckProc_MaskGrab_FindDestPos(d, d->lastValid);

	DECOMP_VehBirth_TeleportSelf(d, 0, 0x80);

	DECOMP_VehStuckProc_RevEngine_Init(t, d);
}
