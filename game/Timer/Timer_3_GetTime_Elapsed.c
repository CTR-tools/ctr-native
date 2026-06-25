#include <common.h>

// Usage: elapsed(frameStart, &frameStart)
// will overwrite new frameStart, and return
// elapsed time since previous frameStart
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004b41c-0x8004b470.
int Timer_GetTime_Elapsed(int oldVal, int *retVal)
{
	int newVal = Timer_GetTime_Total();

	if (retVal != 0)
	{
		*retVal = newVal;
	}

	// impossible?
	if (newVal < oldVal)
	{
		newVal += 0xc7e18;
	}

	return newVal - oldVal;
}
