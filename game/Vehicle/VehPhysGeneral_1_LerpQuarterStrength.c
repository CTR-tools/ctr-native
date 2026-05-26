#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80060458-0x80060488.
int VehPhysGeneral_LerpQuarterStrength(int current, int desired)
{
	if (desired != 0)
	{
		desired = desired >> 2;

		if (desired == 0)
		{
			desired = 1;
		}
	}

	if (desired <= current)
	{
		current = desired;
	}

	return current;
}
