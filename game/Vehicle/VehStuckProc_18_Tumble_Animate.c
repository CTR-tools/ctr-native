#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80068244-0x800682a4.
void VehStuckProc_Tumble_Animate(struct Thread *thread, struct Driver *driver)
{
	int matrixIndex;
	int arrLength;

	driver->matrixArray = 6;
	arrLength = data.bakedGteMath[6].numEntries;

	// divide by 32ms to get frame index
	matrixIndex = driver->NoInputTimer >> 5;

	// modulus to wrap repeat animation
	matrixIndex %= arrLength;

	if (driver->KartStates.Blasted.boolPlayBackwards != 0)
	{
		matrixIndex = arrLength - (matrixIndex + 1);
	}

	driver->matrixIndex = matrixIndex;
}
