#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80051e24-0x80052250.
void UI_JumpMeter_Draw(s16 posX, s16 posY, struct Driver *driver)
{
	struct GameTracker *gGT;
	u32 colorAndCode;
	s16 jumpMeter;
	struct DB *backDB;
	int iVar5;
	u_long *primmemCurr;
	POLY_F4 *p;
	int iVar8;
	s16 sVar9;
	int iVar10;
	int iVar11;
	RECT box;
	RECT box2;
	int jumpMeterHeight;
	int whateverThisIs;

	int numbersYOffset = -45;
	int numbersYHeight = 10;
	int barHeight = 38;

	gGT = sdata->gGT;

	iVar5 = ((int)driver->jumpMeter / 0x3c0) * 0x10000 >> 0x10;
	whateverThisIs = (int)driver->jumpMeter + iVar5 * -0x3c0;
	iVar10 = ((whateverThisIs / 6 + (whateverThisIs >> 0x1f) >> 4) - (whateverThisIs >> 0x1f)) * 0x10000 >> 0x10;
	iVar11 = (int)posX;
	iVar8 = (int)posY + numbersYOffset + 2;

	DebugFont_DrawNumbers(iVar5, iVar11 - 0x10, iVar8);
	DebugFont_DrawNumbers(iVar10, iVar11 + -4, iVar8);
	DebugFont_DrawNumbers((((whateverThisIs + iVar10 * -0x60) * 100) / 0x3c0) * 0x10000 >> 0x10, iVar11 + 4, iVar8);

	sVar9 = posX + -0x14;
	box.w = 0x22; // dont use 3/4 ratio
	box.h = numbersYHeight;
	box.x = sVar9;
	box.y = posY + numbersYOffset;

	Color color;
	color.self = data.colors[21][0];
	CTR_Box_DrawWireBox(&box, &color, gGT->pushBuffer_UI.ptrOT, &gGT->backBuffer->primMem);

	backDB = gGT->backBuffer;
	primmemCurr = backDB->primMem.curr;
	p = 0;

	// if there is room left for more
	if (primmemCurr <= (u_long *)backDB->primMem.endMin100)
	{
		// add primitives
		backDB->primMem.curr = &primmemCurr[6];
		p = (POLY_F4 *)primmemCurr;
	}

	if (p != 0)
	{
		*(u32 *)&p->r0 = 0x28ffffff;
		p->x1 = posX + 13;
		p->x3 = posX + 13;
		p->x0 = box.x;
		p->y0 = box.y;
		p->y1 = box.y;
		p->x2 = box.x;
		p->y2 = box.y + numbersYHeight;
		p->y3 = box.y + numbersYHeight;

		// pointer to OT memory
		primmemCurr = gGT->pushBuffer_UI.ptrOT;

		*(int *)p = *primmemCurr | 0x5000000;
		*primmemCurr = (u32)p & 0xffffff;

		box2.y = posY - barHeight;
		box2.w = 0xc;
		box2.h = barHeight;
		box2.x = posX;

		CTR_Box_DrawWireBox(&box2, &color, gGT->pushBuffer_UI.ptrOT, &backDB->primMem);

		backDB = gGT->backBuffer;
		primmemCurr = backDB->primMem.curr;
		p = 0;

		if (primmemCurr <= (u_long *)backDB->primMem.endMin100)
		{
			backDB->primMem.curr = &primmemCurr[6];
			p = (POLY_F4 *)primmemCurr;
		}

		if (p != 0)
		{
			jumpMeter = driver->jumpMeter;
			sVar9 = driver->jumpMeter;
			colorAndCode = 0x28ff0000;
			if (0x27f < jumpMeter)
			{
				if (jumpMeter < 0x3c0)
				{
					colorAndCode = 0x2800ff00;
				}
				else
				{
					if (jumpMeter < 0x5a0)
					{
						colorAndCode = 0x2800ffff;
					}
					else
					{
						colorAndCode = 0x280000ff;
					}
				}
			}
			*(u32 *)&p->r0 = colorAndCode;
			jumpMeterHeight = (int)sVar9 * barHeight;
			sVar9 = posX + 0xc;
			p->x0 = posX;
			p->x1 = sVar9;
			p->x2 = posX;
			p->y2 = posY;
			p->x3 = sVar9;
			p->y3 = posY;
			jumpMeter = posY - ((s16)(jumpMeterHeight / 0x960));
			p->y0 = jumpMeter;
			p->y1 = jumpMeter;

			primmemCurr = gGT->pushBuffer_UI.ptrOT;

			*(int *)p = *primmemCurr | 0x5000000;
			*primmemCurr = (u32)p & 0xffffff;

			backDB = gGT->backBuffer;
			primmemCurr = backDB->primMem.curr;
			p = 0;

			// If there is room to add more
			if (primmemCurr <= (u_long *)backDB->primMem.endMin100)
			{
				// Add more primitives
				backDB->primMem.curr = &primmemCurr[6];
				p = (POLY_F4 *)primmemCurr;
			}

			if (p != 0)
			{
				*(u32 *)&p->r0 = 0x28808080;
				p->x0 = posX;
				p->y0 = posY - barHeight;
				p->x1 = sVar9;
				p->y1 = posY - barHeight;
				p->x2 = posX;
				p->y2 = posY;
				p->x3 = sVar9;
				p->y3 = posY;

				// pointer to OT memory
				primmemCurr = gGT->pushBuffer_UI.ptrOT;

				*(int *)p = *primmemCurr | 0x5000000;
				*primmemCurr = (u32)p & 0xffffff;
			}
		}
	}
	return;
}
