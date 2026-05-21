#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80019f58-0x8001a054
void DECOMP_CAM_FollowDriver_Spin360(struct CameraDC *cDC, int param_2, struct Driver *d, s16 *desiredPos, s16 *desiredRot)
{
	int ratio;

	// === Union Missing ===
	// Not really "transitionTo" but the variables
	// are shared with other camera modes, therefore
	// need a union with proper names for each mode

	// rotate other way for odd number
	if ((d->driverID & 1) != 0)
	{
		cDC->unk90 -= cDC->transitionTo.pos[0];
	}
	else
	{
		// rotate one way
		cDC->unk90 += cDC->transitionTo.pos[0];
	}

	int angle = cDC->unk90;
	ratio = DECOMP_MATH_Sin(angle);
	desiredPos[0] = (s16)(d->posCurr.x >> 8) + (s16)(CAM_MulLo(ratio, cDC->transitionTo.pos[2]) >> 0xc);

	ratio = DECOMP_MATH_Cos(angle);
	desiredPos[2] = (s16)(d->posCurr.z >> 8) + (s16)(CAM_MulLo(ratio, cDC->transitionTo.pos[2]) >> 0xc);

	desiredPos[1] = (s16)(d->posCurr.y >> 8) + cDC->transitionTo.pos[1];

	DECOMP_CAM_LookAtPosition(param_2, (int *)&d->posCurr.x, desiredPos, desiredRot);
	return;
}
