#include <common.h>

static s32 CAM_MulLo(s32 a, s32 b)
{
	return (s32)(u32)((s64)a * (s64)b);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80018fec-0x80019128
void DECOMP_CAM_ProcessTransition(s16 *currPos, s16 *currRot, s16 *startPos, s16 *startRot, s16 *endPos, s16 *endRot, int frame)
{
	int deltaRot;

	currPos[0] = startPos[0] + (s16)(CAM_MulLo((int)endPos[0] - (int)startPos[0], frame) >> 0xc);
	currPos[1] = startPos[1] + (s16)(CAM_MulLo((int)endPos[1] - (int)startPos[1], frame) >> 0xc);
	currPos[2] = startPos[2] + (s16)(CAM_MulLo((int)endPos[2] - (int)startPos[2], frame) >> 0xc);

	deltaRot = (int)endRot[0] - (int)startRot[0] & 0xfff;
	if (0x7ff < deltaRot)
	{
		deltaRot -= 0x1000;
	}

	currRot[0] = startRot[0] + (s16)(CAM_MulLo(deltaRot, frame) >> 0xc) & 0xfff;
	deltaRot = (int)endRot[1] - (int)startRot[1] & 0xfff;
	if (0x7ff < deltaRot)
	{
		deltaRot -= 0x1000;
	}
	currRot[1] = startRot[1] + (s16)(CAM_MulLo(deltaRot, frame) >> 0xc) & 0xfff;
	deltaRot = (int)endRot[2] - (int)startRot[2] & 0xfff;
	if (0x7ff < deltaRot)
	{
		deltaRot -= 0x1000;
	}
	currRot[2] = startRot[2] + (s16)(CAM_MulLo(deltaRot, frame) >> 0xc) & 0xfff;
	return;
}
