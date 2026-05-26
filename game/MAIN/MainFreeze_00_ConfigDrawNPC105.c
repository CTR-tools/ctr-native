#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800379f4-0x80037bc0.
void MainFreeze_ConfigDrawNPC105(s16 startX, s16 startY, s16 radius, int angleStep, s16 angle, char *color, u_long *otMem, struct PrimMem *primMem)
{
	s16 pos[6];
	char colors[0xc];

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			colors[(i * 4) + j] = color[j];
		}
	}

	pos[0] = startX;
	pos[1] = startY;

	int scaledRadiusX = (radius << 3) / 5;
	int currAngleStep = 0;

	while (true)
	{
		u32 currAngle = (u16)(currAngleStep + angle);

		pos[4] = startX + (s16)((scaledRadiusX * MATH_Cos(currAngle)) >> 0xc);
		pos[5] = startY + (s16)((radius * MATH_Sin(currAngle)) >> 0xc);

		if ((s16)currAngleStep != 0)
		{
			RECTMENU_DrawRwdTriangle(pos, colors, otMem, primMem);
		}

		currAngleStep = (s16)(currAngleStep + angleStep);

		pos[2] = pos[4];
		pos[3] = pos[5];

		if ((s16)currAngleStep > 0x1000)
		{
			return;
		}
	}
}
