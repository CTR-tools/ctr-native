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

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80037bc0-0x80037da0.
void MainFreeze_ConfigDrawArrows(s16 offsetX, s16 offsetY, char *str)
{
	int lineWidth;
	int color;
	u8 **colorPtr;
	struct GameTracker *gGT = sdata->gGT;

	// orange color
	color = 0;

	if ((sdata->frameCounter & 4) == 0)
	{
		// red color
		color = 3;
	}

	lineWidth = DecalFont_GetLineWidth(str, 1) >> 1;

	// get color data
	colorPtr = (u8 **)&data.ptrColor[color];

	struct Icon **iconPtrArray = ICONGROUP_GETICONS(gGT->iconGroup[4]);

	// Draw left arrow
	DecalHUD_Arrow2D(
	    // largeFont
	    iconPtrArray[0x38],

	    (offsetX - lineWidth) - 0x14, (int)offsetY + 7,

	    // pointer to PrimMem struct
	    &gGT->backBuffer->primMem,

	    // pointer to OT memory
	    gGT->pushBuffer_UI.ptrOT,

	    // color data
	    *colorPtr[0], *colorPtr[1], *colorPtr[2], *colorPtr[3],

	    0, FP(1.0), 0x800);

	// Draw right arrow
	DecalHUD_Arrow2D(
	    // largeFont
	    iconPtrArray[0x38],

	    (offsetX + lineWidth) + 0x12, (int)offsetY + 7,

	    // pointer to PrimMem struct
	    &gGT->backBuffer->primMem,

	    // pointer to OT memory
	    gGT->pushBuffer_UI.ptrOT,

	    // color data
	    *colorPtr[0], *colorPtr[1], *colorPtr[2], *colorPtr[3],

	    0, FP(1.0), 0);

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80037da0-0x80038b5c.
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

		s16 y = sdata->analogConfigY[0] + ((sin * (i - 1) * 0x20) >> 0xc) + 0x20;
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
			triangle[(point * 2) + 1] = sdata->analogConfigY[0] + ((angleSin << 5) >> 0xc) + 0x20 + data.raceConfig_unk80084290[base + 3];
		}

		RECTMENU_DrawRwdTriangle(triangle, (char *)data.raceConfig_colors_arrows, gGT->pushBuffer_UI.ptrOT, &gGT->backBuffer->primMem);
	}

	rect.x = 0xec;
	rect.y = sdata->analogConfigY[0];
	rect.w = 0x28;
	rect.h = 0x41;
	RECTMENU_DrawRwdBlueRect(&rect, (char *)data.raceConfig_colors_blueRect, gGT->pushBuffer_UI.ptrOT, &gGT->backBuffer->primMem);

	rect.x = -0x14;
	rect.y = sdata->analogConfigY[0] - 0x14;
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

		MainFreeze_ConfigDrawWire(0x100 + ((cos * 400) / 0x5000), sdata->analogConfigY[1] + ((sin * 0x32) >> 0xc), 0x100 + ((cos * 0x118) / 0x5000),
		                          sdata->analogConfigY[1] + ((sin * 0x23) >> 0xc), 0, 0xff, 0, gGT->pushBuffer_UI.ptrOT);
	}

	u32 frameAngle = (u32)sdata->frameCounter << 6;
	u32 baseAngle = (((MATH_Sin(frameAngle) * value) >> 0xc) - 0x400) & 0xfff;
	int baseSin = MATH_Sin(baseAngle);
	int baseCos = MATH_Cos(baseAngle);

	for (int point = 0; point < 3; point++)
	{
		int offset = point * 2;
		int colorOffset = point * 4;
		MainFreeze_ConfigDrawNPC105(data.unkNamcoGamepad_800842DC[offset] + ((baseCos * 200) / 0x5000) + 0x100,
		                            data.unkNamcoGamepad_800842DC[offset + 1] + sdata->analogConfigY[1] + ((baseSin * 0x19) >> 0xc), 10, 0x80, baseAngle,
		                            (char *)&data.unkNamcoGamepadRwdTriangleColors[colorOffset], gGT->pushBuffer_UI.ptrOT, &gGT->backBuffer->primMem);
	}

	for (int row = 0; row < 0x400; row += 0xaa)
	{
		for (int angle = 0; angle < 0x1000; angle += 0x400)
		{
			u8 color = (row != 0) ? 0x50 : 0x32;
			u32 currAngle = (baseAngle + angle + row) & 0xfff;
			int sin = MATH_Sin(currAngle);
			int cos = MATH_Cos(currAngle);

			MainFreeze_ConfigDrawWire(0x100 + ((cos < 0 ? cos + 0x3f : cos) >> 6), sdata->analogConfigY[1] + ((sin * 0x28) >> 0xc),
			                          0x100 + ((cos * 0x120) / 0x5000), sdata->analogConfigY[1] + ((sin * 0x24) >> 0xc), color, color, color,
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
			                            sdata->analogConfigY[1] + (data.unkNamcoGamepad_800842DC[pointOffset + 19] * scale),
			                            data.unkNamcoGamepad_800842DC[rowOffset + 6], 0x80, baseAngle, (char *)&data.unkNamcoGamepad_800842DC[pointOffset + 12],
			                            gGT->pushBuffer_UI.ptrOT, &gGT->backBuffer->primMem);
		}
	}

	rect.x = -0x14;
	rect.y = sdata->analogConfigY[1] - 0x3c;
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

		DecalFont_DrawMultiLine(sdata->lngStrings[LNG_SELECT_DEAD_ZONE_AND_PRESS_BUTTON], 0x100, sdata->posY_MultiLine[posIndex], 0x1cc, FONT_BIG,
		                        JUSTIFY_CENTER);
		DecalFont_DrawLine(sdata->lngStrings[data.raceConfig_DeadZone[(s16)sdata->WheelConfigOption].lngIndex], 0x100, sdata->posY_Arrows[posIndex], FONT_BIG,
		                   JUSTIFY_CENTER);
		MainFreeze_ConfigDrawArrows(0x100, sdata->posY_Arrows[posIndex], sdata->lngStrings[data.raceConfig_DeadZone[(s16)sdata->WheelConfigOption].lngIndex]);
		sdata->unk_RaceWheelConfig[0] = data.raceConfig_DeadZone[(s16)sdata->WheelConfigOption].lo16;
	}
	else if (sdata->raceWheelConfigPageIndex < 2)
	{
		if (sdata->raceWheelConfigPageIndex == 0)
		{
			DecalFont_DrawMultiLine(sdata->lngStrings[LNG_CENTER_THE_CONTROLLER_AND_PRESS_BUTTON], 0x100, sdata->posY_MultiLine[posIndex], 0x1cc, FONT_BIG,
			                        JUSTIFY_CENTER);

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
		DecalFont_DrawMultiLine(sdata->lngStrings[LNG_SELECT_RANGE_AND_PRESS_BUTTON], 0x100, sdata->posY_MultiLine[posIndex], 0x1cc, FONT_BIG, JUSTIFY_CENTER);
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

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80038b5c-0x80039908.

typedef struct
{
	int numGamepads;
	int numAnalogs;
	int gamepadId[4];
	int analogId[4];
	int isGamepadAnalog[4];
	int menuRowsToRemove;
} GAMEPAD_MainFreeze_MenuPtrOptions;

force_inline void IDENTIFYGAMEPADS_MainFreeze_MenuPtrOptions(struct RectMenu *menu, GAMEPAD_MainFreeze_MenuPtrOptions *gamepad)
{
	struct GameTracker *gGT = sdata->gGT;

	// get number of ordinary gamepads and/or "analog controllers" connected, and which players are using which

	for (int i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		struct ControllerPacket *ptrControllerPacket = sdata->gGamepads->gamepad[i].ptrControllerPacket;

		// if gamepad is not an "analog controller", as CTR uses to refer to jogcons and negcons
		if (
		    // this function needs to display menu graphics for digital controllers at all times,
		    // even if they're unplugged; this is simply an additional check for if
		    // an "analog controller" is connected or not
		    ((ptrControllerPacket == 0) || (ptrControllerPacket->plugged != PLUGGED)) ||

		    ((ptrControllerPacket->controllerData != ((PAD_ID_JOGCON << 4) | 3)) && (ptrControllerPacket->controllerData != ((PAD_ID_NEGCON << 4) | 3))))
		{
			gamepad->gamepadId[gamepad->numGamepads] = i;
			gamepad->isGamepadAnalog[i] = false;
			gamepad->numGamepads++;
		}
		else
		{
			gamepad->analogId[gamepad->numAnalogs] = i;
			gamepad->isGamepadAnalog[i] = true;
			gamepad->numAnalogs++;
		}
	}

	// the menu buttons for configuring dualshocks and analog controllers are accompanied by labels each
	// these labels can appear at once
	int areBothControllerLabelsNecessary = false;
	if (gamepad->numGamepads != 0)
		areBothControllerLabelsNecessary = (gamepad->numAnalogs != 0);

	// set amount of menu rows to hide/remove
	// used for the dualshock and/or "analog" rows which are variable

	// in singleplayer, regardless of gamepad, 2 of these rows are always visible
	// (dualshock label + 1 gamepad)

	// with 4 regular gamepads connected, in multiplayer, there's 5 rows visible
	// (dualshock label + 4 gamepads)

	// maximum amount of rows is 6, which happens if there's 4 controllers and one of them is analog
	// (dualshock label + analog label + 4 gamepads)

	// in the last scenario, menuRowsToRemove will equal -1
	gamepad->menuRowsToRemove = (4 - areBothControllerLabelsNecessary) - gGT->numPlyrCurrGame;
}

force_inline int PROCESSINPUTS_MainFreeze_MenuPtrOptions(struct RectMenu *menu, GAMEPAD_MainFreeze_MenuPtrOptions *gamepad)
{
	struct GameTracker *gGT = sdata->gGT;
	int exitMenu = false;

	if (sdata->AnyPlayerTap & (BTN_UP | BTN_DOWN))
	{
		// play sound for when you're moving around in the menu
		OtherFX_Play(0, 1);

		// there are only 9 rows total
		if (sdata->AnyPlayerTap & BTN_UP)
		{
			menu->rowSelected = (menu->rowSelected + (9 - 1)) % 9;
			if (menu->rowSelected == 7)
				menu->rowSelected = gGT->numPlyrCurrGame + 3;
		}
		else if (sdata->AnyPlayerTap & BTN_DOWN)
		{
			menu->rowSelected = (menu->rowSelected + 1) % 9;
			if (menu->rowSelected > (gGT->numPlyrCurrGame + 3))
				menu->rowSelected = 8;
		}
	}
	else
	{
		switch (menu->rowSelected)
		{
		// 0: FX slider
		// 1: Music slider
		// 2: Voice slider
		case 0:
		case 1:
		case 2:
			OptionsMenu_TestSound(menu->rowSelected, 1);
			if (sdata->AnyPlayerHold & (BTN_LEFT | BTN_RIGHT))
			{
				int volume = howl_VolumeGet(menu->rowSelected);

				if (sdata->AnyPlayerHold & BTN_LEFT)
				{
					volume -= 4;
				}
				else if (sdata->AnyPlayerHold & BTN_RIGHT)
				{
					volume += 4;
				}

				if (volume < 0)
					volume = 0;
				if (volume > 0xff)
					volume = 0xff;

				howl_VolumeSet(menu->rowSelected, volume);
			}
			break;

		// Mode	(Stereo/Mono)
		case 3:
			// clear test sound
			OptionsMenu_TestSound(0, 0);

			if (sdata->AnyPlayerTap & (BTN_CIRCLE | BTN_CROSS_one))
			{
				OtherFX_Play(1, 1);
				int mode = howl_ModeGet();
				howl_ModeSet(mode == 0);
			}
			break;

		// DualShock/"Analog controller" settings
		case 4:
		case 5:
		case 6:
		case 7:
			// clear test sound
			OptionsMenu_TestSound(0, 0);

			if (sdata->AnyPlayerTap & (BTN_CIRCLE | BTN_CROSS_one))
			{
				OtherFX_Play(1, 1);

				int gamepadRow = menu->rowSelected - 4;

				if (gamepadRow < gamepad->numGamepads)
				{
					// selecting dualshock row
					// toggle gamepad vibration
					gGT->gameMode1 ^= data.gGT_gameMode1_VibPerPlayer[gamepad->gamepadId[gamepadRow]];
				}
				else
				{
					// selecting "analog controller" row
					// this will open the analog controller config menu
					sdata->gamepadID_OwnerRaceWheelConfig = gamepad->analogId[gamepadRow - gamepad->numGamepads];
					sdata->boolOpenWheelConfig = true;
					sdata->raceWheelConfigPageIndex = 0;
				}
			}
			break;

		// Exit
		case 8:
			// clear test sound
			OptionsMenu_TestSound(0, 0);

			if (sdata->AnyPlayerTap & (BTN_CIRCLE | BTN_CROSS_one))
			{
				OtherFX_Play(1, 1);
				exitMenu = true;
			}
			break;
		}
	}

	return exitMenu;
}

// stuff is drawn last to first
force_inline void DISPLAYRECTMENU_MainFreeze_MenuPtrOptions(struct RectMenu *menu, GAMEPAD_MainFreeze_MenuPtrOptions *gamepad)
{
	struct GameTracker *gGT = sdata->gGT;

	// note: multitap only works if it's connected to the P1 slot
	int multitapStringOffset = (sdata->gGamepads->slotBuffer[0].controllerData == (PAD_ID_MULTITAP << 4)) ? 2 : 0;

	// a menu row is 10 pixels
	int menuRowsNegativePadding = gamepad->menuRowsToRemove * 10;

	int analogRowPosY = 0;
	if (gamepad->numGamepads != 0)
		analogRowPosY = (gamepad->numGamepads + 1) * 10;

	// cursor location for exit button, which will change depending on how many dualshock rows there need to be
	data.Options_HighlightBar[8].posY = gamepad->menuRowsToRemove * -10 + 119;

	// cursor location for dualshock rows
	if (gamepad->numGamepads > 0)
	{
		for (int i = 0; i < gamepad->numGamepads; i++)
		{
			data.Options_HighlightBar[i + 4].posY = (i * 10) + 79;
		}
	}

	// cursor location for "analog" rows
	if (gamepad->numAnalogs > 0)
	{
		for (int i = 0; i < gamepad->numAnalogs; i++)
		{
			int areBothControllerLabelsNecessary = false;
			if (gamepad->numGamepads != 0)
				areBothControllerLabelsNecessary = (gamepad->numAnalogs != 0);

			data.Options_HighlightBar[gamepad->numGamepads + i + 4].posY = ((gamepad->numGamepads + i + areBothControllerLabelsNecessary) * 10) + 79;
		}
	}

	// drawStyle needs research...
	menu->drawStyle &= 0xfeff;
	if (gGT->numPlyrCurrGame > 2)
		menu->drawStyle |= 0x100;

	int volumeSliderTriangleLeftMargin = 0;

	for (int i = 0; i < 3; i++)
	{
		//"FX:", "MUSIC:", "VOICE:"
		int lineWidth = DecalFont_GetLineWidth(sdata->lngStrings[data.Options_StringIDs_Audio[i]], FONT_SMALL);
		if (volumeSliderTriangleLeftMargin < lineWidth)
			volumeSliderTriangleLeftMargin = lineWidth;
	}

	DecalFont_DrawLine(sdata->lngStrings[LNG_OPTIONS_TITLE], 256, 26 + (menuRowsNegativePadding / 2), FONT_BIG, (JUSTIFY_CENTER | ORANGE));

	int volumeSliderWidth = 380 - (30 + volumeSliderTriangleLeftMargin);

	u_long *ot = gGT->backBuffer->otMem.uiOT;
	struct PrimMem *primMem = &gGT->backBuffer->primMem;
	Color color;

	// draw volume sliders
	for (int i = 0; i < 3; i++)
	{
		int volume = howl_VolumeGet(i) & 0xff;
		int volumeSliderValue = volume * (volumeSliderWidth - 5);

		s16 volumeSliderPosY = (menuRowsNegativePadding / 2) + (i * 10);

		if (volumeSliderValue < 0)
			volumeSliderValue += 0xff;

		int volumeSliderTriangleLeftPosX = 30 + volumeSliderTriangleLeftMargin;
		int volumeSliderBarPosX = 0x38 + volumeSliderTriangleLeftPosX + (s16)((u32)volumeSliderValue >> 8);

		s16 volumeSliderTriangle[8] = {// 0, 1
		                               volumeSliderTriangleLeftPosX + 56, volumeSliderPosY + 58,

		                               // 2, 3
		                               volumeSliderTriangleLeftPosX + volumeSliderWidth + 56, volumeSliderPosY + 48,

		                               // 4, 5
		                               0, 0};

		volumeSliderTriangle[4] = volumeSliderTriangle[2];
		volumeSliderTriangle[5] = volumeSliderTriangle[1];

		RECT volumeSliderBar = {.x = volumeSliderBarPosX + 1, .y = volumeSliderPosY + 48, .w = 3, .h = 10};
		color = *(Color *)(data.Options_VolumeSlider_Colors + 0xc);
		CTR_Box_DrawSolidBox(&volumeSliderBar, color, ot);

		RECT volumeSliderBarOutline = {.x = volumeSliderBarPosX, .y = volumeSliderPosY + 47, .w = 5, .h = 12};

		color = *(Color *)(data.Options_VolumeSlider_Colors + 0x10);
		CTR_Box_DrawSolidBox(&volumeSliderBarOutline, color, ot);

		RECTMENU_DrawRwdTriangle(volumeSliderTriangle, data.Options_VolumeSlider_Colors, ot, primMem);

		// "FX:" "MUSIC:" "VOICE:"
		DecalFont_DrawLine(sdata->lngStrings[data.Options_StringIDs_Audio[i]], 76, 50 + (menuRowsNegativePadding / 2) + (i * 10), FONT_SMALL, ORANGE);
	}

	DecalFont_DrawLine(sdata->lngStrings[LNG_MODE], 76, 80 + (menuRowsNegativePadding / 2), FONT_SMALL, ORANGE);

	// 333: MONO
	// 334: STEREO
	int mode = howl_ModeGet();

	// "MONO", "STEREO"
	DecalFont_DrawLine(sdata->lngStrings[333 + mode], 436, 80 + (menuRowsNegativePadding / 2), FONT_SMALL, (JUSTIFY_RIGHT | WHITE));

	if (gamepad->numGamepads != 0)
	{
		DecalFont_DrawLine(sdata->lngStrings[LNG_DUAL_SHOCK], 76, 90 + (menuRowsNegativePadding / 2), FONT_SMALL, ORANGE);

		int lineWidth_controller1A = DecalFont_GetLineWidth(sdata->lngStrings[data.Options_StringIDs_Gamepads[2]], FONT_SMALL);

		// width can change depending on language
		int lineWidth_vibrateOff = DecalFont_GetLineWidth(sdata->lngStrings[LNG_VIBRATE_OFF], FONT_SMALL);
		int lineWidth_vibrateOn = DecalFont_GetLineWidth(sdata->lngStrings[LNG_VIBRATE_ON], FONT_SMALL);
		if (lineWidth_vibrateOn < lineWidth_vibrateOff)
			lineWidth_vibrateOn = lineWidth_vibrateOff;

		lineWidth_vibrateOn = (lineWidth_controller1A + lineWidth_vibrateOn + 10);
		lineWidth_vibrateOn = 256 - (lineWidth_vibrateOn >> 1);

		for (int i = 0; i < gamepad->numGamepads; i++)
		{
			int dualShockRowColor = ORANGE;
			int currPad = gamepad->gamepadId[i];

			struct ControllerPacket *ptrControllerPacket = sdata->gGamepads->gamepad[currPad].ptrControllerPacket;

			if (ptrControllerPacket == 0 || ptrControllerPacket->plugged != PLUGGED)
			{
				dualShockRowColor = GRAY;
			}

			int rowY = 100 + (menuRowsNegativePadding / 2) + (i * 10);

			// "CONTROLLER 1", "CONTROLLER 2",
			// "CONTROLLER 1A", "CONTROLLER 1B",
			// "CONTROLLER 1C", "CONTROLLER 1D"
			DecalFont_DrawLine(sdata->lngStrings[data.Options_StringIDs_Gamepads[currPad + multitapStringOffset]], lineWidth_vibrateOn, rowY, FONT_SMALL,
			                   dualShockRowColor);

			int boolDisabled = (gGT->gameMode1 & data.gGT_gameMode1_VibPerPlayer[currPad]) != 0;

			if (dualShockRowColor != GRAY)
			{
				dualShockRowColor = boolDisabled ? RED : WHITE;
			}

			// 325: "VIBRATE ON"
			// 326: "VIBRATE OFF"

			DecalFont_DrawLine(sdata->lngStrings[325 + boolDisabled], lineWidth_vibrateOn + lineWidth_controller1A + 10, rowY, FONT_SMALL, dualShockRowColor);
		}
	}

	if (gamepad->numAnalogs != 0)
	{
		DecalFont_DrawLine(sdata->lngStrings[LNG_CONFIGURE_ANALOG], 76, 90 + (menuRowsNegativePadding / 2) + analogRowPosY, FONT_SMALL, ORANGE);

		for (int i = 0; i < gamepad->numAnalogs; i++)
		{
			DecalFont_DrawLine(sdata->lngStrings[data.Options_StringIDs_Gamepads[gamepad->analogId[i] + multitapStringOffset]], 256,
			                   100 + (menuRowsNegativePadding / 2) + analogRowPosY + (i * 10), FONT_SMALL, (JUSTIFY_CENTER | ORANGE));
		}
	}

	DecalFont_DrawLine(sdata->lngStrings[LNG_OPTIONS_EXIT], 76, 140 - (menuRowsNegativePadding / 2), FONT_SMALL, ORANGE);

	RECT cursor = {.x = 74,
	               .y = data.Options_HighlightBar[menu->rowSelected].posY + (menuRowsNegativePadding / 2) + 20,
	               .w = 364,
	               .h = data.Options_HighlightBar[menu->rowSelected].sizeY};

	CTR_Box_DrawClearBox(&cursor, &sdata->menuRowHighlight_Normal, TRANS_50_DECAL, ot);

	RECT titleSeparatorLine = {.x = 66, .y = (menuRowsNegativePadding / 2) + 43, .w = 380, .h = 2};

	color.self = sdata->battleSetup_Color_UI_1;
	RECTMENU_DrawOuterRect_Edge(&titleSeparatorLine, color, 0x20, ot);

	RECT menuBG = {.x = 56, .y = (menuRowsNegativePadding / 2) + 20, .w = 400, .h = 135 - menuRowsNegativePadding};

	RECTMENU_DrawInnerRect(&menuBG, 4, ot);
}

void MainFreeze_MenuPtrOptions(struct RectMenu *menu)
{
	MainFreeze_SafeAdvDestroy();

	// open racing wheel config menu instead
	if (sdata->boolOpenWheelConfig != 0)
	{
		MainFreeze_ConfigSetupEntry();
		return;
	}

	// the options menu has gamepad-related settings
	// specificially for DualShock controllers
	// as well as what CTR refers to as "analog controllers", which are jogcons and negcons
	// this struct will be filled out in IDENTIFYGAMEPADS
	GAMEPAD_MainFreeze_MenuPtrOptions gamepad = {
	    .numGamepads = 0,
	    .numAnalogs = 0,
	    .gamepadId = {-1, -1, -1, -1},
	    .analogId = {-1, -1, -1, -1},
	    .isGamepadAnalog = {false, false, false, false},
	    .menuRowsToRemove = 0,
	};

	IDENTIFYGAMEPADS_MainFreeze_MenuPtrOptions(menu, &gamepad);
	int exitMenu = PROCESSINPUTS_MainFreeze_MenuPtrOptions(menu, &gamepad);
	DISPLAYRECTMENU_MainFreeze_MenuPtrOptions(menu, &gamepad);

	if (exitMenu || (sdata->AnyPlayerTap & (BTN_TRIANGLE | BTN_START | BTN_SQUARE_one)))
	{
		OtherFX_Play(1, 1);
		OptionsMenu_TestSound(0, 0);
		RECTMENU_ClearInput();
		sdata->ptrDesiredMenu = MainFreeze_GetMenuPtr();
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80039908-0x800399fc.
void MainFreeze_MenuPtrQuit(struct RectMenu *menu)
{
	s16 row;
	struct GameTracker *gGT = sdata->gGT;

	if (menu->unk1e == 0)
	{
		row = menu->rowSelected;
		if (row == 0)
		{
			// Erase ghost of previous race from RAM
			GhostTape_Destroy();

			// Add bit for "in menu" when loading is done
			sdata->Loading.OnBegin.AddBitsConfig0 |= MAIN_MENU;

			// Go to main menu
			sdata->mainMenuState = MAIN_MENU_TITLE;

			// Remove bit for "In Adventure Arena" when loading is done
			sdata->Loading.OnBegin.RemBitsConfig0 |= ADVENTURE_ARENA;

			// Unpause game
			gGT->gameMode1 &= ~1;

			// Level ID for main menu (39)
			MainRaceTrack_RequestLoad(0x27);
			return;
		}

		if ((row == 1) || (row == -1))
		{
			sdata->ptrActiveMenu = MainFreeze_GetMenuPtr();
		}
	}
	else
	{
		menu->drawStyle &= ~0x100;

		// if more than 2 screens
		if (gGT->numPlyrCurrGame > 2)
		{
			menu->drawStyle |= 0x100;
		}

		MainFreeze_SafeAdvDestroy();
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800399fc-0x80039a44.
void MainFreeze_SafeAdvDestroy(void)
{
	// If you're in Adventure Arena
	if ((sdata->gGT->gameMode1 & ADVENTURE_ARENA) == 0)
		return;

	// check if Adv Hub is loaded
	if (LOAD_IsOpen_AdvHub() == 0)
		return;

	AH_Pause_Destroy();
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80039a44-0x80039dcc.
void MainFreeze_MenuPtrDefault(struct RectMenu *menu)
{
	int levID = 0; // dingo canyon
	u16 stringID;
	u32 gameMode;

	struct GameTracker *gGT = sdata->gGT;
	gameMode = gGT->gameMode1;

	u8 ADV_CUP = 100;

	// if you have not waited 5 frames since the game was paused then quit
	if (gGT->cooldownfromPauseUntilUnpause != 0)
		return;

	// assume 5 frames have passed since paused

	if (menu->unk1e != 0)
	{
		menu->drawStyle &= 0xfeff;

		// if more than 2 screens
		if (2 < gGT->numPlyrCurrGame)
		{
			menu->drawStyle |= 0x100;
		}

		if (((gameMode & ADVENTURE_ARENA) == 0) || (menu->state & NEEDS_TO_CLOSE))
		{
			return;
		}

		// quit adv hub if it's not loaded
		if (LOAD_IsOpen_AdvHub() == 0)
			return;

		AH_Pause_Update();
		return;
	}

	if (menu->rowSelected < 0)
		return;

	// get stringID from selected row
	stringID = menu->rows[menu->rowSelected].stringIndex;

	// stringID 14: "OPTIONS"
	if (stringID == 14)
	{
		// Set Menu to Options
		sdata->ptrDesiredMenu = &data.menuRacingWheelConfig;

		data.menuRacingWheelConfig.rowSelected = 8;
		return;
	}

	// stringID 11: "AKU AKU HINTS"
	// stringID 12: "UKA UKA HINTS"
	if (stringID == 11 || stringID == 12)
	{
		// Set Menu to Hints
		sdata->ptrDesiredMenu = &D232.menuHintMenu; // in 232
		return;
	}

	// stringID 3: "QUIT"
	if (stringID == 3)
	{
		// Set Menu to Quit
		sdata->ptrDesiredMenu = &data.menuQuit;
		data.menuQuit.rowSelected = 1;
		return;
	}

	// must wait 5 frames until next pause
	gGT->cooldownFromUnpauseUntilPause = 5;

	// hide Menu
	RECTMENU_Hide(menu);

	MainFreeze_SafeAdvDestroy();

	// careful, it's stringID MINUS one
	switch (stringID)
	{
	// stringID 1: "RESTART"
	case 1:

	// stringID 4: "RETRY"
	case 4:

		// get rid of pause flag
		gGT->gameMode1 &= ~PAUSE_1;

		if (RaceFlag_IsFullyOffScreen() == 1)
		{
			// checkered flag, begin transition on-screen
			RaceFlag_BeginTransition(1);
		}

		// restart race
		sdata->Loading.stage = LOAD_RESTART;

		// if you are not showing a ghost during a race
		if (sdata->boolReplayHumanGhost == 0)
			return;

		// If the ghost playing buffer is nullptr
		if (sdata->ptrGhostTapePlaying == 0)
			return;

		// Make P2 the character that is saved in the header of the
		// ghost that you will see in the race
		data.characterIDs[1] = sdata->ptrGhostTapePlaying->characterID;
		return;

	// stringID 2: "RESUME"
	case 2:

		// unpause game
		ElimBG_Deactivate(gGT);

		// get rid of pause flag
		gGT->gameMode1 &= ~PAUSE_1;

		// unpause audio
		MainFrame_TogglePauseAudio(0);

		// play pause/unpause sound
		OtherFX_Play(1, 1);
		return;

	// stringID 5: "CHANGE CHARACTER"
	case 5:

		// erase ghost of previous race from RAM
		GhostTape_Destroy();

		// set level ID to main menu
		levID = MAIN_MENU_LEVEL;

		// return to character selection
		sdata->mainMenuState = MAIN_MENU_CHARACTERS;

		// when loading is done, add bit for "in mb"
		sdata->Loading.OnBegin.AddBitsConfig0 |= 0x2000;

		// get rid of pause flag
		gGT->gameMode1 &= ~PAUSE_1;
		break;

	// stringID 6: "CHANGE LEVEL"
	case 6:

		// erase ghost of previous race from RAM
		GhostTape_Destroy();

		// level ID of main mb
		levID = MAIN_MENU_LEVEL;

		// return to track selection
		sdata->mainMenuState = MAIN_MENU_TRACK_SELECT;

		// when loading is done
		// add bit for "in mb"
		sdata->Loading.OnBegin.AddBitsConfig0 |= 0x2000;

		// get rid of pause flag
		gGT->gameMode1 &= ~PAUSE_1;
		break;

	// stringID 10: "CHANGE SETUP"
	case 10:

		// set level ID to main menu
		levID = MAIN_MENU_LEVEL;

		// return to battle setup
		sdata->mainMenuState = MAIN_MENU_BATTLE_SETUP;

		// when loading is done
		// add bit for "in mb"
		sdata->Loading.OnBegin.AddBitsConfig0 |= 0x2000;

		// get rid of pause flag
		gGT->gameMode1 &= ~PAUSE_1;
		break;

	// stringID 13: "EXIT TO MAP"
	case 13:

		// when loading is done
		// add this bit for In Adventure Arena
		sdata->Loading.OnBegin.AddBitsConfig0 |= 0x100000;

		// when loading is done
		// remove bits for Relic Race or Crystal Challenge
		sdata->Loading.OnBegin.RemBitsConfig0 |= 0xc000000;

		// when loading is done
		// remove bit for CTR Token Challenge
		sdata->Loading.OnBegin.RemBitsConfig8 |= 8;

		// get rid of pause flag
		gGT->gameMode1 &= ~PAUSE_1;

		// If you are not in Adventure cup
		if ((gameMode & ADVENTURE_CUP) == 0)
		{
			// 0x80000000
			// If you're in Boss Mode
			if ((int)gameMode < 0)
			{
				// when loading is done remove bit for Boss Race, relic, and crystal challenge
				sdata->Loading.OnBegin.RemBitsConfig0 |= 0x8c000000;

				// When loading is done add bit to spawn driver near boss door
				sdata->Loading.OnBegin.AddBitsConfig8 |= SPAWN_AT_BOSS;
			}

			// set levID to level you were in previously
			levID = gGT->prevLEV;
		}

		// If you're in Adventure Cup
		else
		{
			levID = GEM_STONE_VALLEY;

			// when loading is done remove bits for Adventure Cup, relic, and crystal challenge
			sdata->Loading.OnBegin.RemBitsConfig0 |= 0x1c000000;

			// Level ID
			gGT->levelID = gGT->cup.cupID + ADV_CUP;
		}
		break;
	default:
		return;
	}

	MainRaceTrack_RequestLoad(levID);
	return;
}

extern int mainFreezeFlags[5];
extern struct RectMenu *mainFreezeMenuArr[5];

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80039dcc-0x80039e98.
struct RectMenu *MainFreeze_GetMenuPtr(void)
{
	struct GameTracker *gGT = sdata->gGT;

	// Set string to "Uka Uka Hints",
	// or if boolGoodGuy, then set "Aku Aku Hints"
	int var1 = 0xc;
	if (VehPickupItem_MaskBoolGoodGuy(gGT->drivers[0]) != 0)
		var1 = 0xb;

	data.rowsAdvHub[1].stringIndex = var1;

	if ((gGT->gameMode2 & CUP_ANY_KIND) != 0)
	{
		return &data.menuArcadeCup;
	}

	u32 gameMode = gGT->gameMode1;
	int *flagPtr = &mainFreezeFlags[0];
	struct RectMenu **menuPtrToPtr = &mainFreezeMenuArr[0];

	for (
	    /**/; *flagPtr != 0;

	    flagPtr++, menuPtrToPtr++)
	{
		if ((gameMode & (*flagPtr)) != 0)
			break;
	}

	// if nothing else,
	// then menuArcadeRace
	return (*menuPtrToPtr);
}

int mainFreezeFlags[5] = {
    BATTLE_MODE, ADVENTURE_ARENA, ADVENTURE_CUP, ADVENTURE_MODE,
    0 // null terminator
};

struct RectMenu *mainFreezeMenuArr[5] = {&data.menuBattle, &data.menuAdvHub, &data.menuAdvCup, &data.menuAdvRace, &data.menuArcadeRace};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80039e98-0x80039fa8.
void MainFreeze_IfPressStart(void)
{
	struct GameTracker *gGT = sdata->gGT;
	u32 gameMode1;
	struct RectMenu *menu;

	if (RaceFlag_IsFullyOnScreen() != 0)
		return;

	if ((gGT->renderFlags & 0x1000) != 0)
		return;

	if (sdata->AkuAkuHintState != 0)
		return;

	if (sdata->ptrActiveMenu != NULL)
		return;

	gameMode1 = gGT->gameMode1;

	if ((gameMode1 & (END_OF_RACE | PAUSE_ALL)) != 0)
		return;

	if (gGT->levelID == MAIN_MENU_LEVEL)
		return;

	if ((gameMode1 & GAME_CUTSCENE) != 0)
		return;

	if (gGT->boolDemoMode != 0)
		return;

	if ((u32)(gGT->levelID - OXIDE_ENDING) < 2)
		return;

	if (sdata->load_inProgress != 0)
		return;

	if ((gGT->gameMode2 & VEH_FREEZE_PODIUM) != 0)
		return;

	gGT->gameMode1 = gameMode1 | PAUSE_1;

	menu = MainFreeze_GetMenuPtr();
	menu->rowSelected = 0;

	RECTMENU_Show(menu);
	MainFrame_TogglePauseAudio(1);
	OtherFX_Play(1, 1);
	ElimBG_Activate(gGT);
}
