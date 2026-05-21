#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80019e7c-0x80019f58
void DECOMP_CAM_LookAtPosition(int scratchpad, int *positions, s16 *desiredPos, s16 *desiredRot)
{
	int dirX = desiredPos[0] - (positions[0] >> 8);
	int dirY = desiredPos[1] - ((positions[1] >> 8) + data.Spin360_heightOffset_driverPos[sdata->gGT->numPlyrCurrGame]);
	int dirZ = desiredPos[2] - (positions[2] >> 8);

	// Store dirX, dirY, dirZ in scratchpad
	*(int *)(scratchpad + 0x24c) = dirX;
	*(int *)(scratchpad + 0x250) = dirY;
	*(int *)(scratchpad + 0x254) = dirZ;

	int distance = SquareRoot0_stub(CAM_MulLo(dirX, dirX) + CAM_MulLo(dirZ, dirZ));

	// rotations
	desiredRot[0] = 0x800 - (s16)ratan2(dirY, distance);
	desiredRot[1] = (s16)ratan2(dirX, dirZ);
	desiredRot[2] = 0;

	return;
}
