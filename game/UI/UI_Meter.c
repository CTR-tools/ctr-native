#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80051c64-0x80051e24.

void UI_JumpMeter_Update(struct Driver *d)
{
	if ((d->actionsFlagSet & ACTION_AIRBORNE) == 0)
	{
		if ((d->actionsFlagSetPrevFrame & ACTION_AIRBORNE) == 0)
		{
			// if Jump meter Timer is done
			if (d->jumpMeterTimer == 0)
			{
				// reset Jump meter
				d->jumpMeter = 0;
			}
			// if Jump meter Timer is not done
			else
			{
				// reduce Jump meter Timer by ~32ms
				d->jumpMeterTimer -= sdata->gGT->elapsedTimeMS;
				// if Jump meter Timer goes negative
				if (d->jumpMeterTimer < 0)
				{
					// prevent Jump meter Timer from going negative
					d->jumpMeterTimer = 0;
				}
			}
		}
		// if, in previous frame? player was in the air
		else
		{
			// if jump is high enough to be significant
			if (0x150 < d->jumpMeter)
			{
				// keep track of all jumps
				d->timeSpentJumping += d->jumpMeter;
			}

			// if highest jump is less than current jump
			if (d->highestJump < d->jumpMeter)
			{
				// save highest jump
				d->highestJump = d->jumpMeter;
			}

			if (d->jumpMeter >= 640)
			{
				int param = 0;
				if (d->jumpMeter >= 960)
				{
					param = 0x80;
				}
				if (d->jumpMeter >= 1440)
				{
					param = 0x100;
				}

				// add one second reserves
				VehFire_Increment(d, 960, POWER_SLIDE_HANG_TIME, param);
			}
		}
	}

	// if player is in the air
	else
	{
		if ((0x480 < d->jump_LandingBoost) && (d->jumpMeter < 0x481))
		{
			// Make driver talk
			Voiceline_RequestPlay(7, data.characterIDs[d->driverID], 0x10);
		}

		// Jump meter = 0x3FC
		d->jumpMeter = d->jump_LandingBoost;

		// if Jump meter > 0x960
		if (0x960 < d->jump_LandingBoost)
		{
			// prevent Jump meter from going over 0x960
			d->jumpMeter = 0x960;
		}
		// keep Jump meter Timer at 0x5A0.
		d->jumpMeterTimer = 0x5a0;
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80051e24-0x80052250.
void UI_JumpMeter_Draw(s16 posX, s16 posY, struct Driver *driver)
{
	struct GameTracker *gGT;
	u32 colorAndCode;
	s16 jumpMeter;
	struct DB *backDB;
	int iVar5;
	uint32_t *primmemCurr;
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
	iVar10 = (((whateverThisIs / 6 + (whateverThisIs >> 0x1f)) >> 4) - (whateverThisIs >> 0x1f)) * 0x10000 >> 0x10;
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
	primmemCurr = backDB->primMem.cursor;
	p = 0;

	// if there is room left for more
	if (primmemCurr <= (uint32_t *)backDB->primMem.guardEnd)
	{
		// add primitives
		backDB->primMem.cursor = &primmemCurr[6];
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

		*(int *)p = CtrGpu_PackOTTag(*primmemCurr, 0x5000000);
		*primmemCurr = CtrGpu_PrimToOTLink24(p);

		box2.y = posY - barHeight;
		box2.w = 0xc;
		box2.h = barHeight;
		box2.x = posX;

		CTR_Box_DrawWireBox(&box2, &color, gGT->pushBuffer_UI.ptrOT, &backDB->primMem);

		backDB = gGT->backBuffer;
		primmemCurr = backDB->primMem.cursor;
		p = 0;

		if (primmemCurr <= (uint32_t *)backDB->primMem.guardEnd)
		{
			backDB->primMem.cursor = &primmemCurr[6];
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

			*(int *)p = CtrGpu_PackOTTag(*primmemCurr, 0x5000000);
			*primmemCurr = CtrGpu_PrimToOTLink24(p);

			backDB = gGT->backBuffer;
			primmemCurr = backDB->primMem.cursor;
			p = 0;

			// If there is room to add more
			if (primmemCurr <= (uint32_t *)backDB->primMem.guardEnd)
			{
				// Add more primitives
				backDB->primMem.cursor = &primmemCurr[6];
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

				*(int *)p = CtrGpu_PackOTTag(*primmemCurr, 0x5000000);
				*primmemCurr = CtrGpu_PrimToOTLink24(p);
			}
		}
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80052250-0x800524c4.
void UI_DrawSlideMeter(s16 posX, s16 posY, struct Driver *driver)
{
	const struct GameTracker *gGT = sdata->gGT;
	const int barWidth = 49;
	int barHeight = gGT->numPlyrCurrGame > 2 ? 3 : 7;

	int meterLength = 0;
	if (driver->turbo_MeterRoomLeft != 0)
	{
		int currentRoomRemaining = driver->turbo_MeterRoomLeft * barWidth;
		int maxRoom = driver->const_turboMaxRoom * ELAPSED_MS;
		meterLength = barWidth - (currentRoomRemaining / maxRoom);
	}

	RECT box;
	s16 topX = posX - barWidth;
	s16 topY = posY - barHeight;
	box.x = topX;
	box.y = topY;
	box.w = barWidth;
	box.h = barHeight;
	Color black = MakeColor(0, 0, 0);
	CTR_Box_DrawWireBox(&box, &black, gGT->pushBuffer_UI.ptrOT, &gGT->backBuffer->primMem);

	const PrimCode primCode = {.poly = {.quad = 1, .renderCode = RenderCode_Polygon}};
	ColorCode colorCode = MakeColorCode(0xFF, 0, 0, primCode); // red color, ready to boost

	if (driver->const_turboLowRoomWarning * ELAPSED_MS < driver->turbo_MeterRoomLeft)
	{
		colorCode = MakeColorCode(0, 0xFF, 0, primCode); // green color, no boost yet
	}

	for (int i = 0; i < 2; i++)
	{
		PolyF4 *p;
		GetPrimMem(p);
		if (p == nullptr)
		{
			return;
		}

		p->colorCode = colorCode;

		p->v[0].pos.y = topY;
		p->v[1].pos.y = topY;
		p->v[2].pos.y = posY;
		p->v[3].pos.y = posY;

		p->v[0].pos.x = posX - meterLength;
		p->v[1].pos.x = posX;
		p->v[2].pos.x = posX - meterLength;
		p->v[3].pos.x = posX;

		AddPrimitive(p, gGT->pushBuffer_UI.ptrOT);
		colorCode = MakeColorCode(0x80, 0x80, 0x80, primCode); // Gray color for background bar
		meterLength = barWidth;
	}
}
