#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800680d0-0x80068150.
void VehStuckProc_Tumble_PhysLinear(struct Thread *thread, struct Driver *driver)
{
	driver->NoInputTimer -= sdata->gGT->elapsedTimeMS;

	if (driver->NoInputTimer < 0)
		driver->NoInputTimer = 0;

	VehPhysProc_Driving_PhysLinear(thread, driver);

	driver->jump_ForcedMS = 0x60;
	driver->baseSpeed = 0;
	driver->fireSpeed = 0;
	driver->actionsFlagSet |= 0x5808;
	driver->jump_InitialVelY = driver->NoInputTimer * 2 + 6000;
}
