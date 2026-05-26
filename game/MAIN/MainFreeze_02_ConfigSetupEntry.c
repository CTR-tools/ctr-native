#include <common.h>

static inline void MainFreeze_ConfigDrawWire(s16 x1, s16 y1, s16 x2, s16 y2, u8 r, u8 g, u8 b, void *ot)
{
	CTR_Box_DrawWirePrims(MakePoint(x1, y1), MakePoint(x2, y2), MakeColor(r, g, b), ot);
}

static inline void MainFreeze_ConfigDrawRaceWheel(int value, struct GameTracker *gGT)
{
	s16 triangle[8];
	RECT rect;

	for (int i = 0; i < 3; i++)
	{
		int sin = MATH_Sin(value);
		void *ot = gGT->pushBuffer_UI.ptrOT;

		if ((i != 1) && (value == 0x600))
		{
			ot = (void *)((intptr_t)ot + 0xc);
		}

		s16 y = sdata->unk_drawingRaceWheelRects[0] + ((sin * (i - 1) * 0x20) >> 0xc) + 0x20;
		MainFreeze_ConfigDrawWire(0xe2, y, 0x11e, y, 0, 0xff, 0, ot);
	}

	for (int tri = 0; tri < 2; tri++)
	{
		u32 wave = ((u32)sdata->frameCounter << 6) + (tri * 0x800);
		int sin = MATH_Sin(wave);
		int angle = (value * sin) >> 0xc;
		int angleSin = MATH_Sin(angle);

		for (int point = 0; point < 3; point++)
		{
			int base = tri * 6 + point * 2;
			triangle[point * 2] = data.raceConfig_unk80084290[base + 2] + ((tri == 0) ? 0x114 : 0xec);
			triangle[(point * 2) + 1] = sdata->unk_drawingRaceWheelRects[0] + ((angleSin << 5) >> 0xc) + 0x20 + data.raceConfig_unk80084290[base + 3];
		}

		RECTMENU_DrawRwdTriangle(triangle, (char *)data.raceConfig_colors_arrows, gGT->pushBuffer_UI.ptrOT, &gGT->backBuffer->primMem);
	}

	rect.x = 0xec;
	rect.y = sdata->unk_drawingRaceWheelRects[0];
	rect.w = 0x28;
	rect.h = 0x41;
	RECTMENU_DrawRwdBlueRect(&rect, (char *)data.raceConfig_colors_blueRect, gGT->pushBuffer_UI.ptrOT, &gGT->backBuffer->primMem);

	rect.x = -0x14;
	rect.y = sdata->unk_drawingRaceWheelRects[0] - 0x14;
	rect.w = 0x228;
	rect.h = 0x91;
	RECTMENU_DrawInnerRect(&rect, 4, gGT->pushBuffer_UI.ptrOT);
}

static inline void MainFreeze_ConfigDrawNamco(int value, struct GameTracker *gGT)
{
	int mirrorValue = -value;
	RECT rect;

	for (int i = 0; i < 2; i++)
	{
		int currValue = (i == 0) ? mirrorValue : value;
		u32 angle = (currValue - 0x400) & 0xfff;
		int sin = MATH_Sin(angle);
		int cos = MATH_Cos(angle);

		MainFreeze_ConfigDrawWire(0x100 + ((cos * 400) / 0x5000), sdata->unk_drawingRaceWheelRects[1] + ((sin * 0x32) >> 0xc), 0x100 + ((cos * 0x118) / 0x5000),
		                          sdata->unk_drawingRaceWheelRects[1] + ((sin * 0x23) >> 0xc), 0, 0xff, 0, gGT->pushBuffer_UI.ptrOT);
	}

	u32 frameAngle = (u32)sdata->frameCounter << 6;
	u32 baseAngle = (((MATH_Sin(frameAngle) * value) >> 0xc) - 0x400) & 0xfff;
	int baseSin = MATH_Sin(baseAngle);
	int baseCos = MATH_Cos(baseAngle);

	for (int point = 0; point < 3; point++)
	{
		int offset = point * 2;
		MainFreeze_ConfigDrawNPC105(data.unkNamcoGamepad_800842DC[offset] + ((baseCos * 200) / 0x5000) + 0x100,
		                            data.unkNamcoGamepad_800842DC[offset + 1] + sdata->unk_drawingRaceWheelRects[1] + ((baseSin * 0x19) >> 0xc), 10, 0x80,
		                            baseAngle, (char *)&data.unkNamcoGamepadRwdTriangleColors[offset], gGT->pushBuffer_UI.ptrOT, &gGT->backBuffer->primMem);
	}

	for (int row = 0; row < 0x400; row += 0xaa)
	{
		for (int angle = 0; angle < 0x1000; angle += 0x400)
		{
			u8 color = (row != 0) ? 0x50 : 0x32;
			u32 currAngle = (baseAngle + angle + row) & 0xfff;
			int sin = MATH_Sin(currAngle);
			int cos = MATH_Cos(currAngle);

			MainFreeze_ConfigDrawWire(0x100 + ((cos < 0 ? cos + 0x3f : cos) >> 6), sdata->unk_drawingRaceWheelRects[1] + ((sin * 0x28) >> 0xc),
			                          0x100 + ((cos * 0x120) / 0x5000), sdata->unk_drawingRaceWheelRects[1] + ((sin * 0x24) >> 0xc), color, color, color,
			                          gGT->pushBuffer_UI.ptrOT);
		}
	}

	for (u16 row = 0; row < 3; row++)
	{
		int rowOffset = row * 2;
		for (int point = 0; point < 3; point++)
		{
			int pointOffset = point * 2;
			s16 scale = data.unkNamcoGamepad_800842DC[rowOffset + 7];
			MainFreeze_ConfigDrawNPC105(data.unkNamcoGamepad_800842DC[pointOffset + 18] * scale + 0x100,
			                            sdata->unk_drawingRaceWheelRects[1] + (data.unkNamcoGamepad_800842DC[pointOffset + 19] * scale),
			                            data.unkNamcoGamepad_800842DC[rowOffset + 6], 0x80, baseAngle, (char *)&data.unkNamcoGamepad_800842DC[pointOffset + 12],
			                            gGT->pushBuffer_UI.ptrOT, &gGT->backBuffer->primMem);
		}
	}

	rect.x = -0x14;
	rect.y = sdata->unk_drawingRaceWheelRects[1] - 0x3c;
	rect.w = 0x228;
	rect.h = 0xa0;
	RECTMENU_DrawInnerRect(&rect, 4, gGT->pushBuffer_UI.ptrOT);
}

void MainFreeze_ConfigSetupEntry(void)
{
	struct GameTracker *gGT = sdata->gGT;

	if ((sdata->AnyPlayerTap & (BTN_TRIANGLE | BTN_SQUARE_one)) != 0)
	{
		sdata->boolOpenWheelConfig = false;
		return;
	}

	int gamepadID = sdata->gamepadID_OwnerRaceWheelConfig;
	struct GamepadBuffer *gamepad = &sdata->gGamepads->gamepad[gamepadID];
	struct ControllerPacket *controller = gamepad->ptrControllerPacket;

	if ((controller == NULL) || (controller->plugged != PLUGGED))
	{
		sdata->boolOpenWheelConfig = false;
		return;
	}

	int isNamco = controller->controllerData == ((PAD_ID_JOGCON << 4) | 3);
	int posIndex = isNamco * 2;

	if (sdata->raceWheelConfigPageIndex == 1)
	{
		u32 tap = sdata->buttonTapPerPlayer[gamepadID];

		if ((tap & (BTN_UP | BTN_LEFT)) != 0)
		{
			sdata->WheelConfigOption--;
			if ((s16)sdata->WheelConfigOption < 0)
			{
				sdata->WheelConfigOption = 3;
			}
		}
		else if ((tap & (BTN_DOWN | BTN_RIGHT)) != 0)
		{
			sdata->WheelConfigOption++;
			if ((s16)sdata->WheelConfigOption > 3)
			{
				sdata->WheelConfigOption = 0;
			}
		}
		else if ((tap & (BTN_CIRCLE | BTN_CROSS_one)) != 0)
		{
			sdata->raceWheelConfigPageIndex = 2;
			data.rwd[gamepadID].deadZone = data.raceConfig_DeadZone[(s16)sdata->WheelConfigOption].hi1;
		}

		DecalFont_DrawMultiLine(sdata->lngStrings[0x223], 0x100, sdata->posY_MultiLine[posIndex], 0x1cc, FONT_BIG, JUSTIFY_CENTER);
		DecalFont_DrawLine(sdata->lngStrings[data.raceConfig_DeadZone[(s16)sdata->WheelConfigOption].lngIndex], 0x100, sdata->posY_Arrows[posIndex], FONT_BIG,
		                   JUSTIFY_CENTER);
		MainFreeze_ConfigDrawArrows(0x100, sdata->posY_Arrows[posIndex], sdata->lngStrings[data.raceConfig_DeadZone[(s16)sdata->WheelConfigOption].lngIndex]);
		sdata->unk_RaceWheelConfig[0] = data.raceConfig_DeadZone[(s16)sdata->WheelConfigOption].lo16;
	}
	else if (sdata->raceWheelConfigPageIndex < 2)
	{
		if (sdata->raceWheelConfigPageIndex == 0)
		{
			DecalFont_DrawMultiLine(sdata->lngStrings[0x222], 0x100, sdata->posY_MultiLine[posIndex], 0x1cc, FONT_BIG, JUSTIFY_CENTER);

			if ((sdata->buttonTapPerPlayer[gamepadID] & (BTN_CIRCLE | BTN_CROSS_one)) != 0)
			{
				sdata->raceWheelConfigPageIndex++;
				if (!isNamco)
				{
					data.rwd[gamepadID].gamepadCenter = controller->analog.rightX;
				}
				else
				{
					gamepad->unk44 = 4;
					data.rwd[sdata->gamepadID_OwnerRaceWheelConfig].gamepadCenter = 0x80;
				}
				RECTMENU_ClearInput();
			}

			sdata->unk_RaceWheelConfig[0] = 0;
		}
	}
	else if (sdata->raceWheelConfigPageIndex == 2)
	{
		u32 tap = sdata->buttonTapPerPlayer[gamepadID];

		if ((tap & (BTN_UP | BTN_LEFT)) != 0)
		{
			sdata->raceWheelConfigOptionIndex--;
			if ((s16)sdata->raceWheelConfigOptionIndex < 0)
			{
				sdata->raceWheelConfigOptionIndex = data.raceConfig_unk80084290[isNamco];
			}
		}
		else if ((tap & (BTN_DOWN | BTN_RIGHT)) != 0)
		{
			sdata->raceWheelConfigOptionIndex++;
			if ((s16)data.raceConfig_unk80084290[isNamco] < (s16)sdata->raceWheelConfigOptionIndex)
			{
				sdata->raceWheelConfigOptionIndex = 0;
			}
		}
		else if ((tap & (BTN_CIRCLE | BTN_CROSS_one)) != 0)
		{
			sdata->boolOpenWheelConfig = false;
			data.rwd[gamepadID].range = data.raceConfig_Range[(s16)sdata->raceWheelConfigOptionIndex].hi1;
			RECTMENU_ClearInput();
		}

		sdata->unk_RaceWheelConfig[0] = data.raceConfig_Range[(s16)sdata->raceWheelConfigOptionIndex].lo16;
		DecalFont_DrawMultiLine(sdata->lngStrings[0x228], 0x100, sdata->posY_MultiLine[posIndex], 0x1cc, FONT_BIG, JUSTIFY_CENTER);
		DecalFont_DrawLine(sdata->lngStrings[data.raceConfig_Range[(s16)sdata->raceWheelConfigOptionIndex].lngIndex], 0x100, sdata->posY_Arrows[posIndex],
		                   FONT_BIG, JUSTIFY_CENTER);
		MainFreeze_ConfigDrawArrows(0x100, sdata->posY_Arrows[posIndex],
		                            sdata->lngStrings[data.raceConfig_Range[(s16)sdata->raceWheelConfigOptionIndex].lngIndex]);
	}

	if (!isNamco)
	{
		MainFreeze_ConfigDrawRaceWheel(sdata->unk_RaceWheelConfig[0], gGT);
	}
	else
	{
		MainFreeze_ConfigDrawNamco(sdata->unk_RaceWheelConfig[0], gGT);
	}
}
