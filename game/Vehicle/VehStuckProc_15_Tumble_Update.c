#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006809c-0x800680d0.
void VehStuckProc_Tumble_Update(struct Thread *thread, struct Driver *driver)
{
	if (driver->NoInputTimer != 0)
		return;

	driver->matrixArray = 0;
	driver->matrixIndex = 0;
	VehPhysProc_Driving_Init(thread, driver);
}
