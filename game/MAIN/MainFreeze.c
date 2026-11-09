#include <common.h>
#ifndef MAIN_FREEZE_WHEEL_CENTER_Y
#define MAIN_FREEZE_WHEEL_CENTER_Y sdata->analogConfigY[0]
#endif
#ifndef MAIN_FREEZE_DISPLAY_VALUE
#define MAIN_FREEZE_DISPLAY_VALUE sdata->unk_RaceWheelConfig[0]
#endif
#ifndef MAIN_FREEZE_LANGUAGE_STRINGS
#define MAIN_FREEZE_LANGUAGE_STRINGS sdata->lngStrings
#endif
#ifndef MAIN_FREEZE_NAMCO_CENTER_Y
#define MAIN_FREEZE_NAMCO_CENTER_Y ((u16)sdata->analogConfigY[1])
#endif
#ifndef MAIN_FREEZE_OPTIONS_HIGHLIGHTS
#define MAIN_FREEZE_OPTIONS_HIGHLIGHTS data.Options_HighlightBar
#endif
#ifndef MAIN_FREEZE_OPTIONS_PAD_LABELS
#define MAIN_FREEZE_OPTIONS_PAD_LABELS data.Options_StringIDs_Gamepads
#endif
#ifndef MAIN_FREEZE_OPTIONS_VIBRATION_MASKS
#define MAIN_FREEZE_OPTIONS_VIBRATION_MASKS data.gGT_gameMode1_VibPerPlayer
#endif
#ifndef MAIN_FREEZE_OPTIONS_SLIDER_COLORS
#define MAIN_FREEZE_OPTIONS_SLIDER_COLORS data.Options_VolumeSlider_Colors
#endif
#ifndef MAIN_FREEZE_OPTIONS_HOLD
#define MAIN_FREEZE_OPTIONS_HOLD sdata->AnyPlayerHold
#endif
#ifndef MAIN_FREEZE_OPTIONS_HIGHLIGHT_COLOR
#define MAIN_FREEZE_OPTIONS_HIGHLIGHT_COLOR sdata->menuRowHighlight_Normal
#endif
#ifndef MAIN_FREEZE_OPTIONS_EDGE_COLOR
#define MAIN_FREEZE_OPTIONS_EDGE_COLOR sdata->battleSetup_Color_UI_1
#endif

// NOTE(aalhendi): The resident build binds this table to its linked symbol;
// native uses the shared runtime table directly.
#ifndef MAIN_FREEZE_LOAD_TRIG_BASE
#define MAIN_FREEZE_LOAD_TRIG_BASE(page, base, offset) \
	do                                                 \
	{                                                  \
		(void)sizeof(page);                            \
		(void)sizeof(offset);                          \
		(base) = (void *)data.trigApprox;              \
	} while (0)
#endif

#ifndef MAIN_FREEZE_LOAD_TRIG_PAGE
#define MAIN_FREEZE_LOAD_TRIG_PAGE(page)     ((page) = 0)
#define MAIN_FREEZE_ADD_TRIG_LOW(base, page) ((base) = MAIN_FREEZE_TRIG_TABLE)
#endif

#ifndef MAIN_FREEZE_LOAD_TRIANGLE_OPTIONS
#define MAIN_FREEZE_LOAD_TRIANGLE_OPTIONS(page, base, dependency) \
	do                                                            \
	{                                                             \
		(void)sizeof(page);                                       \
		(void)sizeof(dependency);                                 \
		(base) = (const s16 *)&MAIN_FREEZE_OPTION_COUNTS[2];      \
	} while (0)
#endif


void MainFreeze_ConfigDrawNPC105(s16 startX, s16 startY, s16 radius, int angleStepInput, s16 angle, char *color, u32 *otMem, struct PrimMem *primMem)
{
	register int angleStep CTR_PSX_REGISTER("$20") = angleStepInput;
	Color colors[3];
	s16 pos[6];
	int scaledRadiusX;
	int scaledNumerator;
	int currAngleStep;
	u32 currAngle;
	register u32 trigPacked CTR_PSX_REGISTER("$3");
	register u32 trigPage CTR_PSX_REGISTER("$9");
	u32 trigOffset;
	register u8 *trigBase CTR_PSX_REGISTER("$9");
	u8 *trigAddress;
	int sine;
	int cosine;
	// NOTE(aalhendi): The products and shared shift have the retail v0/v1 lifetimes.
	register int xProduct CTR_PSX_REGISTER("$2");
	register int shifted CTR_PSX_REGISTER("$2");
	register int yProduct CTR_PSX_REGISTER("$3");
	int nextStep;

	pos[0] = startX;
	pos[1] = startY;

	scaledNumerator = radius * 8;
	// NOTE(aalhendi): GCC 2.8.1 then places the divide-by-five constant in a3.
	CTR_PSX_BIND_VALUE_CLOBBER(scaledNumerator, "$2");
	scaledRadiusX = scaledNumerator / 5;
	currAngleStep = 0;

	// NOTE(aalhendi): Retail repeats one packed color across all three vertices.
	colors[2] = ColorCode_Load(color);
	colors[1] = colors[2];
	colors[0] = colors[1];

	while (true)
	{
		currAngle = (s16)currAngleStep + (s16)angle;
		// NOTE(aalhendi): Retail reads one table word for both sine and cosine.
		trigOffset = ANG_MODULO_HALF_PI(currAngle) * sizeof(struct TrigTable);
		MAIN_FREEZE_LOAD_TRIG_BASE(trigPage, trigBase, trigOffset);
		CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(trigAddress, trigBase, trigOffset);
		trigPacked = CTR_ReadU32LE(trigAddress);
		if ((currAngle & ANG_QUADRANT_BIT) != 0)
		{
			cosine = (s16)trigPacked;
			sine = (s16)(trigPacked >> 16);
			if ((currAngle & ANG_SIGN_BIT) != 0)
			{
				sine = -sine;
			}
			else
			{
				cosine = -cosine;
			}
		}
		else
		{
			cosine = (s16)(trigPacked >> 16);
			sine = (s16)trigPacked;
			if ((currAngle & ANG_SIGN_BIT) != 0)
			{
				cosine = -cosine;
				sine = -sine;
			}
		}

		xProduct = scaledRadiusX * cosine;
		shifted = xProduct >> 0xc;
		// NOTE(aalhendi): The result is stored as s16; a low-word add preserves it.
		CTR_PSX_ADD_U32(shifted, startX, shifted);
		pos[4] = shifted;
		yProduct = radius * sine;
		shifted = yProduct >> 0xc;
		pos[5] = startY + (s16)shifted;

		if ((s16)currAngleStep != 0)
		{
			RECTMENU_DrawRwdTriangle(pos, (char *)colors, otMem, primMem);
		}

		nextStep = currAngleStep + angleStep;
		CTR_PSX_COPY_VALUE(currAngleStep, nextStep);
		CTR_PSX_KEEP_VALUE_RELAXED(currAngleStep);

		pos[2] = pos[4];
		pos[3] = pos[5];

		if ((s16)nextStep > 0x1000)
		{
			return;
		}
	}
}

void MainFreeze_ConfigDrawArrows(s16 offsetX, s16 offsetY, char *str)
{
	register int lineWidth CTR_PSX_REGISTER("$18");
	int rawWidth;
	int arrowX;
	int color;
	u32 **colorSlot;

	// orange color
	color = 0;

	if ((GAME_FRAME_COUNTER_LOW & 4) == 0)
	{
		// red color
		color = 3;
	}

	// NOTE(aalhendi): Retail narrows the measured width before halving it;
	// materializing x between those steps preserves the MIPS call-result order.
	rawWidth = DecalFont_GetLineWidth(str, 1);
	arrowX = offsetX;
	lineWidth = (s16)rawWidth / 2;

	// NOTE(aalhendi): Keep the palette slot so both draws reload its pointer.
	colorSlot = &data.ptrColor[color];

	// Draw left arrow
	DecalHUD_Arrow2D(
	    // largeFont
	    (ICONGROUP_GETICONS(GAME_TRACKER->iconGroup[4]))[0x38],

	    (arrowX - lineWidth) - 0x14, (int)offsetY + 7,

	    // pointer to PrimMem struct
	    &GAME_TRACKER->backBuffer->primMem,

	    // pointer to OT memory
	    GAME_TRACKER->pushBuffer_UI.ptrOT,

	    // color data
	    ColorCode_Load(&(*colorSlot)[0]), ColorCode_Load(&(*colorSlot)[1]), ColorCode_Load(&(*colorSlot)[2]), ColorCode_Load(&(*colorSlot)[3]),

	    0, FP(1.0), 0x800);

	// Draw right arrow
	DecalHUD_Arrow2D(
	    // largeFont
	    (ICONGROUP_GETICONS(GAME_TRACKER->iconGroup[4]))[0x38],

	    (arrowX + lineWidth) + 0x12, (int)offsetY + 7,

	    // pointer to PrimMem struct
	    &GAME_TRACKER->backBuffer->primMem,

	    // pointer to OT memory
	    GAME_TRACKER->pushBuffer_UI.ptrOT,

	    // color data
	    ColorCode_Load(&(*colorSlot)[0]), ColorCode_Load(&(*colorSlot)[1]), ColorCode_Load(&(*colorSlot)[2]), ColorCode_Load(&(*colorSlot)[3]),

	    0, FP(1.0), 0);

	return;
}


#ifndef MAIN_FREEZE_TRIG_TABLE
#define MAIN_FREEZE_TRIG_TABLE data.trigApprox
#endif
#ifndef MAIN_FREEZE_OPTION_COUNTS
#define MAIN_FREEZE_OPTION_COUNTS data.raceConfig_unk80084290
#endif
#ifndef MAIN_FREEZE_ARROW_COLORS
#define MAIN_FREEZE_ARROW_COLORS data.raceConfig_colors_arrows
#endif
#ifndef MAIN_FREEZE_BLUE_RECT_COLORS
#define MAIN_FREEZE_BLUE_RECT_COLORS data.raceConfig_colors_blueRect
#endif
#ifndef MAIN_FREEZE_NAMCO_TRIANGLE_COLORS
#define MAIN_FREEZE_NAMCO_TRIANGLE_COLORS data.jogConTriangleColors
#endif
#ifndef MAIN_FREEZE_NAMCO_GEOMETRY
#define MAIN_FREEZE_NAMCO_GEOMETRY data.unkNamcoGamepad_800842DC
#endif

#ifndef MAIN_FREEZE_NAMCO_ICON_OFFSETS
#define MAIN_FREEZE_NAMCO_ICON_OFFSETS (&MAIN_FREEZE_NAMCO_GEOMETRY[18])
#endif
#ifndef MAIN_FREEZE_NAMCO_ROWS
#define MAIN_FREEZE_NAMCO_ROWS (&MAIN_FREEZE_NAMCO_GEOMETRY[6])
#endif
#ifndef MAIN_FREEZE_NAMCO_ICON_COLORS
#define MAIN_FREEZE_NAMCO_ICON_COLORS (&MAIN_FREEZE_NAMCO_GEOMETRY[12])
#endif

#define MAIN_FREEZE_CONFIG_SIN_FROM_TABLE(angleValue, tableBase, result)      \
	do                                                                        \
	{                                                                         \
		u32 sinAngle = (angleValue);                                          \
		(result) = CTR_ReadU32LE(&(tableBase)[ANG_MODULO_HALF_PI(sinAngle)]); \
		if ((sinAngle & ANG_QUADRANT_BIT) == 0)                               \
			(result) = (u32)(result) << 16;                                   \
		(result) = (s32)(result) >> 16;                                       \
		if ((sinAngle & ANG_SIGN_BIT) != 0)                                   \
			(result) = -(result);                                             \
	} while (0)

static inline int MainFreeze_ConfigSin(u32 angle)
{
	const struct TrigTable *trigTable = MAIN_FREEZE_TRIG_TABLE;
	int sine;
	MAIN_FREEZE_CONFIG_SIN_FROM_TABLE(angle, trigTable, sine);
	return sine;
}

// NOTE(aalhendi): Both results must stay as local values; taking their
// addresses makes GCC spill the pair that retail keeps in registers.
#define MAIN_FREEZE_CONFIG_SINCOS(angle, tableBase, sine, cosine)                 \
	do                                                                            \
	{                                                                             \
		u32 trigAngle = (angle);                                                  \
		(sine) = (s32)CTR_ReadU32LE(&(tableBase)[ANG_MODULO_HALF_PI(trigAngle)]); \
		if ((trigAngle & ANG_QUADRANT_BIT) != 0)                                  \
		{                                                                         \
			register u32 trigLow CTR_PSX_REGISTER("$2") = (u32)(sine) << 16;      \
			CTR_PSX_KEEP_VALUE_RELAXED(trigLow);                                  \
			(cosine) = (s32)trigLow >> 16;                                        \
			(sine) = (s32)(sine) >> 16;                                           \
			if ((trigAngle & ANG_SIGN_BIT) != 0)                                  \
				(sine) = -(sine);                                                 \
			else                                                                  \
				(cosine) = -(cosine);                                             \
		}                                                                         \
		else                                                                      \
		{                                                                         \
			(cosine) = (s32)(sine) >> 16;                                         \
			(sine) = (s16)(sine);                                                 \
			if ((trigAngle & ANG_SIGN_BIT) != 0)                                  \
			{                                                                     \
				(cosine) = -(cosine);                                             \
				(sine) = -(sine);                                                 \
			}                                                                     \
		}                                                                         \
	} while (0)

#ifndef MAIN_FREEZE_WHEEL_DATA
#define MAIN_FREEZE_WHEEL_DATA data.rwd
#endif
#ifndef MAIN_FREEZE_DEAD_ZONE_OPTIONS
#define MAIN_FREEZE_DEAD_ZONE_OPTIONS data.raceConfig_DeadZone
#endif
#ifndef MAIN_FREEZE_RANGE_OPTIONS
#define MAIN_FREEZE_RANGE_OPTIONS data.raceConfig_Range
#endif
#ifndef MAIN_FREEZE_BUTTON_TAPS
#define MAIN_FREEZE_BUTTON_TAPS sdata->buttonTapPerPlayer
#endif
#ifndef MAIN_FREEZE_MULTILINE_Y
#define MAIN_FREEZE_MULTILINE_Y sdata->posY_MultiLine
#endif
#ifndef MAIN_FREEZE_ARROW_Y
#define MAIN_FREEZE_ARROW_Y sdata->posY_Arrows
#endif

void MainFreeze_ConfigSetupEntry(void)
{
	// NOTE(aalhendi): Preserve retail's point stores and expose a real s16 array
	// to the primitive drawer without stepping across struct subobjects.
	union
	{
		SVec2 points[4];
		s16 coords[8];
	} triangle;
	RECT configRect;
	RECT innerRect;
	struct GamepadBuffer *gamepad;
	struct ControllerPacket *controller;
	int gamepadID;
	register int isNamcoArg CTR_PSX_REGISTER("$4");
	register int isNamco CTR_PSX_REGISTER("$19");
	register int drawMode CTR_PSX_REGISTER("$2");
	register s16 namcoStart CTR_PSX_REGISTER("$19");
	u32 isNamcoXor;
	s16 pageIndex;
	u16 pageRaw;

	if ((GAME_ANY_PLAYER_TAP & (BTN_TRIANGLE | BTN_SQUARE_one)) != 0)
	{
		*(volatile b16 *)&sdata->boolOpenWheelConfig = false;
		return;
	}

	gamepadID = sdata->gamepadID_OwnerRaceWheelConfig;
	gamepad = &sdata->gGamepads->gamepad[gamepadID];
	controller = gamepad->ptrControllerPacket;

	if ((controller == NULL) || (controller->plugged != PLUGGED))
	{
		*(volatile b16 *)&sdata->boolOpenWheelConfig = false;
		return;
	}

	pageIndex = sdata->raceWheelConfigPageIndex;
	pageRaw = CTR_ReadU16LE(&sdata->raceWheelConfigPageIndex);
	isNamcoXor = controller->controllerData ^ ((PAD_ID_JOGCON << 4) | 3);
	isNamcoArg = (isNamcoXor == 0);
	// NOTE(aalhendi): Keep the input live through the result to avoid register coalescing.
	CTR_PSX_DEPEND_VALUE(isNamcoArg, isNamcoXor);
	isNamco = isNamcoArg;

	switch (pageIndex)
	{
	case 0:
	{
		register int namcoIndex CTR_PSX_REGISTER("$16");
		s16 ownerIndex;
		register int textX CTR_PSX_REGISTER("$5") = 0x100;
		register int textY CTR_PSX_REGISTER("$6");
		register char **strings CTR_PSX_REGISTER("$3");
		CTR_PSX_DEPEND_VALUE(isNamcoArg, textX);
		CTR_PSX_COPY_VALUE(namcoIndex, isNamcoArg);
		textY = MAIN_FREEZE_MULTILINE_Y[namcoIndex * 2];
		CTR_PSX_LOAD_WORD_AFTER(strings, sdata->lngStrings, textY);

		DecalFont_DrawMultiLine(strings[LNG_CENTER_THE_CONTROLLER_AND_PRESS_BUTTON], textX, textY, 0x1cc, FONT_BIG, JUSTIFY_CENTER);

		ownerIndex = sdata->gamepadID_OwnerRaceWheelConfig;
		if ((MAIN_FREEZE_BUTTON_TAPS[ownerIndex] & (BTN_CIRCLE | BTN_CROSS_one)) != 0)
		{
			(*(volatile s16 *)&sdata->raceWheelConfigPageIndex)++;
			if (namcoIndex)
			{
				sdata->gGamepads->gamepad[ownerIndex].jogCenteringFrames = 4;
				MAIN_FREEZE_WHEEL_DATA[sdata->gamepadID_OwnerRaceWheelConfig].gamepadCenter = 0x80;
			}
			else
			{
				MAIN_FREEZE_WHEEL_DATA[ownerIndex].gamepadCenter = sdata->gGamepads->gamepad[ownerIndex].ptrControllerPacket->payload.analog.rightX;
			}
			RECTMENU_ClearInput();
		}

		*(volatile int *)&sdata->unk_RaceWheelConfig[0] = 0;
	}
	break;
	case 1:
	{
		u32 tap = MAIN_FREEZE_BUTTON_TAPS[gamepadID];
		s16 option;

		if ((tap & (BTN_UP | BTN_LEFT)) != 0)
		{
			option = (s16)(sdata->WheelConfigOption - 1);
			sdata->WheelConfigOption = option;
			if (option < 0)
			{
				sdata->WheelConfigOption = 3;
				CTR_PSX_RELOAD(sdata->WheelConfigOption);
			}
		}
		else if ((tap & (BTN_DOWN | BTN_RIGHT)) != 0)
		{
			option = (s16)(sdata->WheelConfigOption + 1);
			sdata->WheelConfigOption = option;
			if (option > 3)
			{
				sdata->WheelConfigOption = 0;
				CTR_PSX_RELOAD(sdata->WheelConfigOption);
			}
		}
		else if ((tap & (BTN_CIRCLE | BTN_CROSS_one)) != 0)
		{
			sdata->raceWheelConfigPageIndex = pageRaw + 1;
			MAIN_FREEZE_WHEEL_DATA[gamepadID].deadZone = MAIN_FREEZE_DEAD_ZONE_OPTIONS[(s16)sdata->WheelConfigOption].inputValue;
		}

		DecalFont_DrawMultiLine(MAIN_FREEZE_LANGUAGE_STRINGS[LNG_SELECT_DEAD_ZONE_AND_PRESS_BUTTON], 0x100, MAIN_FREEZE_MULTILINE_Y[isNamco * 2], 0x1cc,
		                        FONT_BIG, JUSTIFY_CENTER);
		{
			int lineIndex = MAIN_FREEZE_DEAD_ZONE_OPTIONS[(s16)sdata->WheelConfigOption].lngIndex;
			int lineY = MAIN_FREEZE_ARROW_Y[isNamco * 2];
			CTR_PSX_DEPEND_MEMORY(&sdata->lngStrings, lineY);
			DecalFont_DrawLine(sdata->lngStrings[lineIndex], 0x100, lineY, FONT_BIG, JUSTIFY_CENTER);
		}
		MainFreeze_ConfigDrawArrows(0x100, MAIN_FREEZE_ARROW_Y[isNamco * 2],
		                            sdata->lngStrings[MAIN_FREEZE_DEAD_ZONE_OPTIONS[(s16)sdata->WheelConfigOption].lngIndex]);
		*(volatile int *)&sdata->unk_RaceWheelConfig[0] = MAIN_FREEZE_DEAD_ZONE_OPTIONS[(s16)sdata->WheelConfigOption].previewAngle;
	}
	break;
	case 2:
	{
		u32 tap = MAIN_FREEZE_BUTTON_TAPS[gamepadID];
		s16 option;
		u16 optionRaw;
		int maxOption;
		s16 rangeValue;
		register const u16 *optionCountBase CTR_PSX_REGISTER("$2");
		register int optionCountOffset CTR_PSX_REGISTER("$3");
		register const u16 *optionCountPtr CTR_PSX_REGISTER("$3");
		register const s16 *upperCountBase CTR_PSX_REGISTER("$3");
		register int upperCountOffset CTR_PSX_REGISTER("$4");
		register const s16 *upperCountPtr CTR_PSX_REGISTER("$4");
		register int rangeOffset CTR_PSX_REGISTER("$3");
		register struct RacingWheelData *rangeBase CTR_PSX_REGISTER("$6");
		register struct RacingWheelData *rangeDest CTR_PSX_REGISTER("$3");

		if ((tap & (BTN_UP | BTN_LEFT)) != 0)
		{
			option = (s16)(sdata->raceWheelConfigOptionIndex - 1);
			sdata->raceWheelConfigOptionIndex = option;
			if (option < 0)
			{
				optionCountBase = MAIN_FREEZE_OPTION_COUNTS;
				CTR_PSX_KEEP_VALUE_RELAXED(optionCountBase);
				optionCountOffset = isNamco * sizeof(*optionCountBase);
				CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(optionCountPtr, optionCountBase, optionCountOffset);
				sdata->raceWheelConfigOptionIndex = *optionCountPtr;
				CTR_PSX_RELOAD(sdata->raceWheelConfigOptionIndex);
			}
		}
		else if ((tap & (BTN_DOWN | BTN_RIGHT)) != 0)
		{
			upperCountOffset = isNamco * sizeof(*upperCountPtr);
			upperCountBase = (const s16 *)MAIN_FREEZE_OPTION_COUNTS;
			CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(upperCountPtr, upperCountBase, upperCountOffset);
			optionRaw = (u16)sdata->raceWheelConfigOptionIndex;
			CTR_PSX_LOAD_SIGNED_HALF_AFTER(maxOption, upperCountPtr, 0, *upperCountPtr, optionRaw);
			option = (s16)(optionRaw + 1);
			sdata->raceWheelConfigOptionIndex = option;
			if (maxOption < option)
			{
				// NOTE(aalhendi): Keep the wrap path's X argument separate from the common
				// literal so GCC fills its branch delay without moving the branch target.
				register int wrapTextX CTR_PSX_REGISTER("$5") = 0x100;
				sdata->raceWheelConfigOptionIndex = 0;
				CTR_PSX_DEPEND_MEMORY(&sdata->raceWheelConfigOptionIndex, wrapTextX);
			}
		}
		else if ((tap & (BTN_CIRCLE | BTN_CROSS_one)) != 0)
		{
			rangeBase = MAIN_FREEZE_WHEEL_DATA;
			CTR_PSX_KEEP_VALUE_RELAXED(rangeBase);
			rangeOffset = gamepadID * sizeof(*rangeBase);
			CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(rangeDest, rangeBase, rangeOffset);
			rangeValue = MAIN_FREEZE_RANGE_OPTIONS[(s16)sdata->raceWheelConfigOptionIndex].inputValue;
			*(volatile b16 *)&sdata->boolOpenWheelConfig = false;
			rangeDest->range = rangeValue;
			RECTMENU_ClearInput();
		}

		MAIN_FREEZE_DISPLAY_VALUE = MAIN_FREEZE_RANGE_OPTIONS[(s16)sdata->raceWheelConfigOptionIndex].previewAngle;
		DecalFont_DrawMultiLine(MAIN_FREEZE_LANGUAGE_STRINGS[LNG_SELECT_RANGE_AND_PRESS_BUTTON], 0x100, MAIN_FREEZE_MULTILINE_Y[isNamco * 2], 0x1cc, FONT_BIG,
		                        JUSTIFY_CENTER);
		{
			int lineIndex = MAIN_FREEZE_RANGE_OPTIONS[(s16)sdata->raceWheelConfigOptionIndex].lngIndex;
			int lineY = MAIN_FREEZE_ARROW_Y[isNamco * 2];
			CTR_PSX_DEPEND_MEMORY(&sdata->lngStrings, lineY);
			DecalFont_DrawLine(sdata->lngStrings[lineIndex], 0x100, lineY, FONT_BIG, JUSTIFY_CENTER);
		}
		MainFreeze_ConfigDrawArrows(0x100, MAIN_FREEZE_ARROW_Y[isNamco * 2],
		                            sdata->lngStrings[MAIN_FREEZE_RANGE_OPTIONS[(s16)sdata->raceWheelConfigOptionIndex].lngIndex]);
	}
	break;
	}

	drawMode = isNamco;
	CTR_PSX_KEEP_VALUE_RELAXED(drawMode);
	namcoStart = 0;
	if (!drawMode)
	{
		int value = sdata->unk_RaceWheelConfig[0];
		s16 i;
		register s16 tri CTR_PSX_REGISTER("$16");
		register s16 point CTR_PSX_REGISTER("$5");
		register int sin CTR_PSX_REGISTER("$4");
		int baseY;
		register int yOffset CTR_PSX_REGISTER("$3");
		const struct TrigTable *triTrigBase;
		register u32 trianglePage CTR_PSX_REGISTER("$2");
		const s16 *triangleOptions;
		register u32 iSpacing CTR_PSX_REGISTER("$2");
		register void *ot CTR_PSX_REGISTER("$8");
		register int y CTR_PSX_REGISTER("$2");
		u32 wave;
		register int angle CTR_PSX_REGISTER("$3");
		register int angleSin CTR_PSX_REGISTER("$4");
		register int triangleY CTR_PSX_REGISTER("$8");
		register int triangleOffset CTR_PSX_REGISTER("$7");
		register int triangleIndex CTR_PSX_REGISTER("$6");

		CTR_PSX_ZERO_VALUE(i);
		do
		{
			int wheelProduct;
			struct GameTracker *tracker;
			register int sin CTR_PSX_REGISTER("$3");

			sin = MainFreeze_ConfigSin(value);
			CTR_PSX_KEEP_VALUE_RELAXED(sin);
			iSpacing = (u32)((s16)i - 1);
			CTR_PSX_SHIFT_LEFT_IN_PLACE(iSpacing, 5);
			// NOTE(aalhendi): Retail reads the full word before narrowing the drawn Y coordinate.
			wheelProduct = sin * (s32)iSpacing;
			baseY = MAIN_FREEZE_WHEEL_CENTER_Y;
			yOffset = wheelProduct >> 12;
			yOffset += 0x20;
			y = baseY + yOffset;

			CTR_Box_DrawWirePrims(
			    0xe2, y, 0x11e, y, 0, 0xff, 0,
			    (tracker = GAME_TRACKER, ot = tracker->pushBuffer_UI.ptrOT, ((i != 1) && (value == 0x600)) ? (ot = (void *)((u8 *)ot + 0xc)) : ot),
			    &GAME_TRACKER->backBuffer->primMem);
			i++;
		} while (i < 3);

		// NOTE(aalhendi): Both triangles are drawn; entering directly avoids a non-retail loop precheck.
		tri = 0;
		triTrigBase = MAIN_FREEZE_TRIG_TABLE;
		do
		{
			int wheelProduct;
			// NOTE(aalhendi): Retail reads the signed low half of the frame counter for this angle.
			wave = ((u32)(s16)GAME_FRAME_COUNTER_LOW << 6) + (tri * 0x800);
			MAIN_FREEZE_CONFIG_SIN_FROM_TABLE(wave & ~0x3f, triTrigBase, sin);
			CTR_PSX_KEEP_VALUE_RELAXED(sin);
			wheelProduct = value * sin;
			angle = wheelProduct >> 12;
			MAIN_FREEZE_CONFIG_SIN_FROM_TABLE(angle, triTrigBase, angleSin);
			point = 0;
			MAIN_FREEZE_LOAD_TRIANGLE_OPTIONS(trianglePage, triangleOptions, point);

			CTR_PSX_DEPEND_VALUE(tri, triangleOptions);
			{
				register int indexWord CTR_PSX_REGISTER("$2") = (u32)tri << 16;
				register int yDelta CTR_PSX_REGISTER("$2");
				register u32 yBase CTR_PSX_REGISTER("$3");
				CTR_PSX_KEEP_VALUE_RELAXED(indexWord);
				triangleIndex = indexWord >> 16;
				indexWord = triangleIndex * 3;
				CTR_PSX_OBSERVE_VALUE(indexWord);
				triangleOffset = indexWord * 4;
				yDelta = (s32)((u32)angleSin << 5) >> 0xc;
				CTR_PSX_LOAD_UNSIGNED_HALF_AFTER(yBase, sdata, offsetof(struct sData, analogConfigY), sdata->analogConfigY[0], yDelta);
				yDelta += 0x20;
				CTR_PSX_KEEP_VALUE_RELAXED(yDelta);
				triangleY = yBase + yDelta;
			}
			do
			{
				register s16 nextPoint CTR_PSX_REGISTER("$4");
				register int pointWord CTR_PSX_REGISTER("$3");
				triangle.points[point].x = ((const s16 *)((u32)(point * 4 + triangleOffset) + (u32)triangleOptions))[0] + ((triangleIndex != 0) ? 0xec : 0x114);
				CTR_PSX_KEEP_VALUE_RELAXED(point);
				pointWord = (u32)point << 16;
				CTR_PSX_DEPEND_VALUE(point, pointWord);
				nextPoint = point + 1;
				CTR_PSX_COPY_VALUE(point, nextPoint);
				CTR_PSX_KEEP_VALUE_RELAXED(nextPoint);
				((SVec2 *)((u8 *)triangle.points + (pointWord >> 14)))->y =
				    triangleY + ((const s16 *)((u32)((pointWord >> 14) + triangleOffset) + (u32)triangleOptions))[1];
				if (nextPoint >= 3)
					break;
			} while (1);

			RECTMENU_DrawRwdTriangle(triangle.coords, (char *)MAIN_FREEZE_ARROW_COLORS, GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);
			{
				register s16 nextTriangle CTR_PSX_REGISTER("$2") = tri + 1;
				CTR_PSX_COPY_VALUE(tri, nextTriangle);
				CTR_PSX_KEEP_VALUE_RELAXED(nextTriangle);
				if (nextTriangle >= 2)
					break;
			}
		} while (1);

		configRect.y = sdata->analogConfigY[0];
		configRect.x = 0xec;
		configRect.w = 0x28;
		configRect.h = 0x41;
		RECTMENU_DrawRwdBlueRect(&configRect, (char *)MAIN_FREEZE_BLUE_RECT_COLORS, GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);

		innerRect.x = -0x14;
		innerRect.w = 0x228;
		innerRect.h = 0x91;
		innerRect.y = sdata->analogConfigY[0] - 0x14;
		RECTMENU_DrawInnerRect(&innerRect, 4, GAME_TRACKER->pushBuffer_UI.ptrOT);
	}
	else
	{
		int value;
		u32 mirrorAngle;
		register s16 i CTR_PSX_REGISTER("$19");
		u32 angle;
		register int sin CTR_PSX_REGISTER("$16");
		register int cos CTR_PSX_REGISTER("$17");
		u32 frameAngle;
		register u32 baseAngle CTR_PSX_REGISTER("$21");
		register int point CTR_PSX_REGISTER("$18");
		register int wireAngle CTR_PSX_REGISTER("$19");
		register int color CTR_PSX_REGISTER("$5");
		u32 currAngle;
		u16 iconRow;
		int rowOffset;

		{
			register const struct TrigTable *trigTable CTR_PSX_REGISTER("$22");
			register u32 tablePage CTR_PSX_REGISTER("$2");
			i = namcoStart;
			MAIN_FREEZE_LOAD_TRIG_PAGE(tablePage);
			MAIN_FREEZE_ADD_TRIG_LOW(trigTable, tablePage);
			value = MAIN_FREEZE_DISPLAY_VALUE;
			mirrorAngle = (u32)(-value - 0x400) & 0xfff;
			do
			{
				if ((s16)i == 0)
				{
					angle = mirrorAngle;
				}
				else
				{
					angle = ((u32)(value - 0x400) & 0xfff);
				}
				MAIN_FREEZE_CONFIG_SINCOS(angle, trigTable, sin, cos);

				{
					u16 centerY = sdata->analogConfigY[1];
					CTR_Box_DrawWirePrims(0x100 + ((cos * 400) / 0x5000), centerY + ((sin * 0x32) >> 0xc), 0x100 + ((cos * 0x118) / 0x5000),
					                      centerY + ((sin * 0x23) >> 0xc), 0, 0xff, 0, GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);
				}
				{
					register s16 nextIndex CTR_PSX_REGISTER("$2") = i + 1;
					CTR_PSX_COPY_VALUE(i, nextIndex);
					CTR_PSX_KEEP_VALUE_RELAXED(nextIndex);
					if ((s16)nextIndex >= 2)
						break;
				}
			} while (1);
		}

		{
			register const struct TrigTable *frameTable CTR_PSX_REGISTER("$3");
			register u32 framePage CTR_PSX_REGISTER("$3");
			register int wheelAngle CTR_PSX_REGISTER("$4");
			register int frameCounter CTR_PSX_REGISTER("$2");
			int product;
			register int shifted CTR_PSX_REGISTER("$2");
			MAIN_FREEZE_LOAD_TRIG_PAGE(framePage);
			frameCounter = (s16)GAME_FRAME_COUNTER_LOW;
			CTR_PSX_DEPEND_VALUE(framePage, frameCounter);
			MAIN_FREEZE_ADD_TRIG_LOW(frameTable, framePage);
			frameAngle = (u32)frameCounter << 6;
			MAIN_FREEZE_CONFIG_SIN_FROM_TABLE(frameAngle & ~0x3f, frameTable, sin);
			product = sin * value;
			frameTable = MAIN_FREEZE_TRIG_TABLE;
			CTR_PSX_KEEP_VALUE_RELAXED(frameTable);
			shifted = product >> 12;
			wheelAngle = shifted - 0x400;
			CTR_PSX_KEEP_VALUE_RELAXED(wheelAngle);
			baseAngle = wheelAngle & 0xfff;
			MAIN_FREEZE_CONFIG_SINCOS(wheelAngle, frameTable, sin, cos);
		}

		{
			const s16 *geometry = MAIN_FREEZE_NAMCO_GEOMETRY;
			register int cosDouble CTR_PSX_REGISTER("$2");
			register int cosTriple CTR_PSX_REGISTER("$23");
			register int nextPoint CTR_PSX_REGISTER("$3");

			cosDouble = cos * 2;
			CTR_PSX_KEEP_VALUE_RELAXED(cosDouble);
			cosTriple = cosDouble + cos;
			point = 0;
			do
			{
				register int cosPartial CTR_PSX_REGISTER("$2");
				int cosRadius;
				register int cosSign CTR_PSX_REGISTER("$20");
				// NOTE(aalhendi): Retail rebuilds the radius across the outer loop boundary.
				CTR_PSX_DEPEND_VALUE(cosTriple, point);
				cosPartial = cosTriple * 8 + cos;
				CTR_PSX_OBSERVE_VALUE(cosPartial);
				cosRadius = cosPartial * 8;
				cosSign = cosRadius >> 31;
				do
				{
					int colorOffset;
					register const s16 *position CTR_PSX_REGISTER("$2");
					register u8 *colorBase CTR_PSX_REGISTER("$2");
					register u32 x CTR_PSX_REGISTER("$4");
					register u32 y CTR_PSX_REGISTER("$5");
					register int radius CTR_PSX_REGISTER("$6");
					int centerX;
					register struct GameTracker *tracker CTR_PSX_REGISTER("$8");
					u32 centerY;
					CTR_PSX_KEEP_VALUE_RELAXED(cosRadius);
					// NOTE(aalhendi): Start with the sign term so GCC reuses the real result
					// instead of reserving a stack slot for a canceled division temporary.
					centerX = cosRadius >> 31;
					centerX += cosRadius / 0x5000;
					centerX -= cosSign;
					centerX += 0x100;

					colorOffset = (s16)point * 4;
					position = (const s16 *)((u32)colorOffset + (u32)geometry);
					CTR_PSX_KEEP_VALUE_RELAXED(position);
					radius = 10;
					CTR_PSX_DEPEND_VALUE(position, radius);
					x = (u16)position[0];
					y = (u16)position[1];
					colorBase = MAIN_FREEZE_NAMCO_TRIANGLE_COLORS;
					CTR_PSX_DEPEND_VALUE(colorBase, y);
					colorOffset += (u32)colorBase;
					tracker = GAME_TRACKER;
					centerY = MAIN_FREEZE_NAMCO_CENTER_Y;

					MainFreeze_ConfigDrawNPC105(x + (s16)centerX, y + centerY + ((sin * 0x19) >> 0xc), radius, 0x80, baseAngle, (char *)colorOffset,
					                            tracker->pushBuffer_UI.ptrOT, &tracker->backBuffer->primMem);
					{
						register int nextWord CTR_PSX_REGISTER("$2");
						nextPoint = point + 1;
						CTR_PSX_KEEP_VALUE_RELAXED(nextPoint);
						nextWord = (u32)nextPoint << 16;
						CTR_PSX_KEEP_VALUE_RELAXED(nextWord);
						point = nextPoint;
						if ((nextWord >> 16) >= 3)
							break;
					}
				} while (1);
				{
					register int next CTR_PSX_REGISTER("$2");
					CTR_PSX_KEEP_VALUE_RELAXED(nextPoint);
					next = (s16)(nextPoint + 1);
					CTR_PSX_KEEP_VALUE_RELAXED(next);
					point = 0;
					if (next >= 3)
						break;
				}
			} while (1);
		}

		{
			const struct TrigTable *trigTable = MAIN_FREEZE_TRIG_TABLE;
			do
			{
				register int rowWord CTR_PSX_REGISTER("$2");
				register int narrowRow CTR_PSX_REGISTER("$20");
				wireAngle = 0;
				CTR_PSX_DEPEND_VALUE(point, wireAngle);
				rowWord = (u32)point << 16;
				CTR_PSX_KEEP_VALUE_RELAXED(rowWord);
				narrowRow = rowWord >> 16;
				do
				{
					color = (narrowRow != 0) ? 0x50 : 0x32;
					currAngle = (baseAngle + (wireAngle + point)) & 0xfff;
					MAIN_FREEZE_CONFIG_SINCOS(currAngle, trigTable, sin, cos);

					{
						register int x CTR_PSX_REGISTER("$4");
						register int colorValue CTR_PSX_REGISTER("$2");
						register u32 centerY CTR_PSX_REGISTER("$7");
						register int yDelta CTR_PSX_REGISTER("$5");
						int quotient;
						if (cos < 0)
							x = cos + 0x3f;
						else
							x = cos;
						CTR_PSX_KEEP_VALUE_RELAXED(x);
						quotient = (cos * 0x120) / 0x5000;
						x = (s16)(0x100 + (x >> 6));
						CTR_PSX_COPY_VALUE(colorValue, color);
						CTR_PSX_DEPEND_VALUE(colorValue, x);
						yDelta = (sin * 0x28) >> 0xc;
						centerY = (u16)sdata->analogConfigY[1];
						CTR_Box_DrawWirePrims(x, centerY + yDelta, 0x100 + quotient, centerY + ((sin * 0x24) >> 0xc), colorValue, colorValue, colorValue,
						                      GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);
					}
					{
						register int next CTR_PSX_REGISTER("$2") = wireAngle + 0x400;
						CTR_PSX_COPY_VALUE(wireAngle, next);
						CTR_PSX_KEEP_VALUE_RELAXED(next);
						if ((s16)next >= 0x1000)
							break;
					}
				} while (1);
				{
					register int next CTR_PSX_REGISTER("$2") = point + 0xaa;
					CTR_PSX_COPY_VALUE(point, next);
					CTR_PSX_KEEP_VALUE_RELAXED(next);
					if ((s16)next >= 0x400)
						break;
				}
			} while (1);
		}

		for (iconRow = 0; iconRow < 3; iconRow++)
		{
			rowOffset = (s16)iconRow * 2;
			point = 0;
			do
			{
				register int byteOffset CTR_PSX_REGISTER("$3") = (s16)point * 4;
				const s16 *position;
				const s16 *shape;
				register u32 x CTR_PSX_REGISTER("$4");
				register u32 scaleValue CTR_PSX_REGISTER("$5");
				register u32 y CTR_PSX_REGISTER("$4");
				register int productX CTR_PSX_REGISTER("$8");
				int productY;

				CTR_PSX_KEEP_VALUE_RELAXED(byteOffset);
				position = (const s16 *)((u32)byteOffset + (u32)MAIN_FREEZE_NAMCO_ICON_OFFSETS);
				shape = MAIN_FREEZE_NAMCO_ROWS + rowOffset;
				x = (u16)position[0];
				scaleValue = (u16)shape[1];
				productX = x * scaleValue;
				y = (u16)position[1];

				MainFreeze_ConfigDrawNPC105(productX + 0x100, MAIN_FREEZE_NAMCO_CENTER_Y + (productY = y * scaleValue), shape[0], 0x80, baseAngle,
				                            (char *)((u32)byteOffset + (u32)MAIN_FREEZE_NAMCO_ICON_COLORS), GAME_TRACKER->pushBuffer_UI.ptrOT,
				                            &GAME_TRACKER->backBuffer->primMem);
				{
					register int next CTR_PSX_REGISTER("$2") = point + 1;
					CTR_PSX_COPY_VALUE(point, next);
					CTR_PSX_KEEP_VALUE_RELAXED(next);
					if ((s16)next >= 3)
						break;
				}
			} while (1);
		}

		configRect.x = -0x14;
		configRect.w = 0x228;
		configRect.h = 0xa0;
		configRect.y = sdata->analogConfigY[1] - 0x3c;
		RECTMENU_DrawInnerRect(&configRect, 4, GAME_TRACKER->pushBuffer_UI.ptrOT);
	}
}


void MainFreeze_MenuPtrOptions(struct RectMenu *inputMenu)
{
	s16 analogIDs[4];
	s16 gamepadIDs[4];
	b16 isAnalog[4];
	RECT sliderRect;
	s16 sliderTriangle[8];
	RECT rect;
	struct RectMenu *menu = inputMenu;
	b16 exitMenu = false;
	s16 rowPadding;
	s16 analogRowOffset;
	s16 numGamepads;
	s16 numAnalogs;
	s16 multitapOffset;
	s16 innerY;
	// NOTE(aalhendi): This spill home preserves retail's unused eight-byte frame gap.
	s16 sliderWidth CTR_PSX_SPILL_HOME(120);
	s16 maxLabelWidth = 0;
	s16 i;
	s16 selected;
	s16 controller;
	s16 labelX;
	s32 controllerWidth;
	s32 enabledWidth;
	s32 disabledWidth;
	s16 width;
	s16 sliderLeft;
	s16 sliderX;
	s16 rowY;
	b32 bothLabels;
	b16 disconnected;
	s32 volume;
	s16 rowOffset;
	s16 halfPadding;
	s32 scaledVolume;
	struct ControllerPacket *packet;
	char **setting;
	u8 slotControllerData;

	// NOTE(aalhendi): Retail writes this table, but drawing uses the compact lists.
	(void)isAnalog;

	slotControllerData = GAMEPADS->slotBuffer[0].controller.controllerData;
	numGamepads = 0;
	multitapOffset = (slotControllerData == (PAD_ID_MULTITAP << 4)) ? 2 : 0;
	MainFreeze_SafeAdvDestroy();
	numAnalogs = 0;
	if (sdata->boolOpenWheelConfig != 0)
	{
		MainFreeze_ConfigSetupEntry();
		return;
	}

	// Unplugged controllers remain visible; JogCon and NegCon get a separate section.
	for (i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
	{
		packet = GAMEPADS->gamepad[i].ptrControllerPacket;
		if (packet != 0 && packet->plugged == PLUGGED &&
		    (packet->controllerData == ((PAD_ID_JOGCON << 4) | 3) || packet->controllerData == ((PAD_ID_NEGCON << 4) | 3)))
		{
			analogIDs[numAnalogs++] = i;
			isAnalog[i] = true;
		}
		else
		{
			gamepadIDs[numGamepads++] = i;
			isAnalog[i] = false;
		}
	}

	// Each controller section needs its own heading when both types are present.
	bothLabels = false;
	if (numGamepads != 0)
		bothLabels = numAnalogs != 0;
	rowPadding = 4 - bothLabels;
	rowPadding = (rowPadding - GAME_TRACKER->numPlyrCurrGame) * 10;
	if (numGamepads != 0)
		analogRowOffset = (numGamepads + 1) * 10;
	else
		analogRowOffset = 0;
	halfPadding = rowPadding >> 1;
	MAIN_FREEZE_OPTIONS_HIGHLIGHTS[8].posY = 119 - rowPadding;
	for (i = 0; i < numGamepads; i++)
		MAIN_FREEZE_OPTIONS_HIGHLIGHTS[i + 4].posY = i * 10 + 79;
	for (i = 0; i < numAnalogs; i++)
	{
		__typeof__(MAIN_FREEZE_OPTIONS_HIGHLIGHTS[0]) *rows = MAIN_FREEZE_OPTIONS_HIGHLIGHTS;
		s32 highlightRow = i + 4;
		u16 *position;
		s32 playerRow;
		b32 analogHeadingGap;
		position = &rows[numGamepads + highlightRow].posY;
		playerRow = i + numGamepads;
		analogHeadingGap = false;
		if (numGamepads != 0)
			analogHeadingGap = numAnalogs != 0;
		*position = (playerRow + analogHeadingGap) * 10 + 79;
	}
	{
		struct RectMenu *activeMenu = menu;
		u16 style = activeMenu->drawStyle & ~RECTMENU_DRAW_STYLE_3P4P_LAYOUT;
		activeMenu->drawStyle = style;
		if (GAME_TRACKER->numPlyrCurrGame > 2)
			activeMenu->drawStyle = style | RECTMENU_DRAW_STYLE_3P4P_LAYOUT;
	}
	if (GAME_ANY_PLAYER_TAP & BTN_UP)
	{
		s16 row;
		OtherFX_Play(0, 1);
		row = menu->rowSelected - 1;
		menu->rowSelected = row;
		if (row < 0)
			menu->rowSelected = 8;
		else if (row == 7)
			menu->rowSelected = GAME_TRACKER->numPlyrCurrGame + 3;
	}
	else if (GAME_ANY_PLAYER_TAP & BTN_DOWN)
	{
		s16 row;
		OtherFX_Play(0, 1);
		row = menu->rowSelected + 1;
		menu->rowSelected = row;
		if (row >= 9)
			menu->rowSelected = 0;
		else if (row >= GAME_TRACKER->numPlyrCurrGame + 4)
			menu->rowSelected = 8;
	}
	else
	{
		switch (menu->rowSelected)
		{
		case 0:
		case 1:
		case 2:
			selected = menu->rowSelected;
			OptionsMenu_TestSound(selected, 1);
			if (MAIN_FREEZE_OPTIONS_HOLD & BTN_LEFT)
			{
				s32 branchVolume;
				branchVolume = (howl_VolumeGet(selected) & 0xff) - 4;
				if (branchVolume < 0)
					branchVolume = 0;
				howl_VolumeSet(selected, branchVolume);
			}
			else if (MAIN_FREEZE_OPTIONS_HOLD & BTN_RIGHT)
			{
				s32 branchVolume;
				branchVolume = howl_VolumeGet(selected);
				branchVolume &= 0xff;
				branchVolume += 4;
				if (branchVolume > 255)
					branchVolume = 255;
				howl_VolumeSet(selected, branchVolume);
			}
			break;
		case 3:
			OptionsMenu_TestSound(0, 0);
			if (GAME_ANY_PLAYER_TAP & (BTN_CIRCLE | BTN_CROSS_one))
			{
				OtherFX_Play(1, 1);
				howl_ModeSet((u8)howl_ModeGet() == 0);
			}
			break;
		case 4:
		case 5:
		case 6:
		case 7:
			OptionsMenu_TestSound(0, 0);
			if (GAME_ANY_PLAYER_TAP & (BTN_CIRCLE | BTN_CROSS_one))
			{
				selected = menu->rowSelected - 4;
				OtherFX_Play(1, 1);
				if (selected >= numGamepads)
				{
					s16 wheelOwner = analogIDs[selected - numGamepads];
					sdata->boolOpenWheelConfig = true;
					sdata->raceWheelConfigPageIndex = 0;
					sdata->gamepadID_OwnerRaceWheelConfig = wheelOwner;
					// NOTE(aalhendi): Keep the handoff's own reset before joining the draw pass.
					i = 0;
					goto MeasureAudioLabels;
				}
				else
					GAME_TRACKER->gameMode1 ^= MAIN_FREEZE_OPTIONS_VIBRATION_MASKS[gamepadIDs[selected]];
			}
			break;
		case 8:
			OptionsMenu_TestSound(0, 0);
			if (GAME_ANY_PLAYER_TAP & (BTN_CIRCLE | BTN_CROSS_one))
			{
				OtherFX_Play(1, 1);
				exitMenu = true;
			}
			break;
		}
	}

	i = 0;
MeasureAudioLabels:
	do
	{
		s16 *audioLabels = data.Options_StringIDs_Audio;
		width = DecalFont_GetLineWidth(MAIN_FREEZE_LANGUAGE_STRINGS[audioLabels[i]], FONT_SMALL);
		if (width > maxLabelWidth)
			maxLabelWidth = width;
		i++;
	} while ((u16)i < 3);
	DecalFont_DrawLine(MAIN_FREEZE_LANGUAGE_STRINGS[LNG_OPTIONS_TITLE], 256, halfPadding + 26, FONT_BIG, JUSTIFY_CENTER | ORANGE);
	sliderLeft = maxLabelWidth + 30;
	sliderWidth = 380 - sliderLeft;
	for (i = 0; i < 3; i++)
	{
		RECT *sliderPtr = &sliderRect;
		rowOffset = i * 10;
		volume = howl_VolumeGet(i) & 0xff;
		scaledVolume = volume * (sliderWidth - 5);
		rowY = halfPadding + rowOffset;
		sliderX = sliderLeft + 56 + scaledVolume / 256;
		sliderRect.x = sliderX + 1;
		sliderRect.y = rowY + 48;
		sliderRect.w = 3;
		sliderRect.h = 10;
		CTR_Box_DrawSolidBox(sliderPtr, (Color *)(MAIN_FREEZE_OPTIONS_SLIDER_COLORS + 12), GAME_TRACKER->backBuffer->otMem.uiOT,
		                     &GAME_TRACKER->backBuffer->primMem);
		sliderRect.x = sliderX;
		sliderRect.y = rowY + 47;
		sliderRect.w = 5;
		sliderRect.h = 12;
		CTR_Box_DrawSolidBox(sliderPtr, (Color *)(MAIN_FREEZE_OPTIONS_SLIDER_COLORS + 16), GAME_TRACKER->backBuffer->otMem.uiOT,
		                     &GAME_TRACKER->backBuffer->primMem);
		sliderTriangle[0] = sliderLeft + 56;
		sliderTriangle[1] = rowY + 58;
		sliderTriangle[2] = sliderLeft + (s16)(sliderWidth + 56);
		sliderTriangle[3] = rowY + 48;
		sliderTriangle[4] = sliderTriangle[2];
		sliderTriangle[5] = sliderTriangle[1];
		RECTMENU_DrawRwdTriangle(sliderTriangle, MAIN_FREEZE_OPTIONS_SLIDER_COLORS, GAME_TRACKER->backBuffer->otMem.uiOT, &GAME_TRACKER->backBuffer->primMem);
		{
			s16 *audioLabels = data.Options_StringIDs_Audio;
			DecalFont_DrawLine(MAIN_FREEZE_LANGUAGE_STRINGS[audioLabels[i]], 76, rowOffset + (s16)(halfPadding + 50), FONT_SMALL, ORANGE);
		}
	}
	DecalFont_DrawLine(MAIN_FREEZE_LANGUAGE_STRINGS[LNG_MODE], 76, halfPadding + 80, FONT_SMALL, ORANGE);
	setting = &MAIN_FREEZE_LANGUAGE_STRINGS[(u8)howl_ModeGet() == 0 ? LNG_MONO : LNG_STEREO];
	DecalFont_DrawLine(*setting, 436, halfPadding + 80, FONT_SMALL, JUSTIFY_RIGHT | WHITE);
	if (numGamepads != 0)
	{
		DecalFont_DrawLine(MAIN_FREEZE_LANGUAGE_STRINGS[LNG_DUAL_SHOCK], 76, halfPadding + 90, FONT_SMALL, ORANGE);
		controllerWidth = DecalFont_GetLineWidth(MAIN_FREEZE_LANGUAGE_STRINGS[MAIN_FREEZE_OPTIONS_PAD_LABELS[2]], FONT_SMALL);
		disabledWidth = DecalFont_GetLineWidth(MAIN_FREEZE_LANGUAGE_STRINGS[LNG_VIBRATE_OFF], FONT_SMALL);
		enabledWidth = DecalFont_GetLineWidth(MAIN_FREEZE_LANGUAGE_STRINGS[LNG_VIBRATE_ON], FONT_SMALL);
		if ((s16)enabledWidth < (s16)disabledWidth)
			enabledWidth = disabledWidth;
		// NOTE(aalhendi): Retail narrows the combined label span before centering.
		labelX = 256 - (s16)(controllerWidth + enabledWidth + 10) / 2;
		for (i = 0; i < numGamepads; i++)
		{
			controller = gamepadIDs[i];
			selected = controller + multitapOffset;
			{
				struct ControllerPacket *rowPacket = GAMEPADS->gamepad[controller].ptrControllerPacket;
				disconnected = false;
				if (rowPacket == 0 || rowPacket->plugged != PLUGGED)
					disconnected = true;
			}
			// NOTE(aalhendi): Form the offset first on both supported flat-address targets.
			setting = (char **)((size_t)MAIN_FREEZE_OPTIONS_PAD_LABELS[selected] * sizeof(*setting) + (size_t)MAIN_FREEZE_LANGUAGE_STRINGS);
			rowY = i * 10 + (s16)(halfPadding + 100);
			DecalFont_DrawLine(*setting, labelX, rowY, FONT_SMALL, disconnected ? GRAY : ORANGE);
			setting =
			    &MAIN_FREEZE_LANGUAGE_STRINGS[(GAME_TRACKER->gameMode1 & MAIN_FREEZE_OPTIONS_VIBRATION_MASKS[controller]) ? LNG_VIBRATE_OFF : LNG_VIBRATE_ON];
			DecalFont_DrawLine(*setting, labelX + controllerWidth + 10, i * 10 + (s16)(halfPadding + 100), FONT_SMALL,
			                   disconnected                                                                  ? GRAY
			                   : (GAME_TRACKER->gameMode1 & MAIN_FREEZE_OPTIONS_VIBRATION_MASKS[controller]) ? RED
			                                                                                                 : WHITE);
		}
	}
	if (numAnalogs != 0)
	{
		DecalFont_DrawLine(MAIN_FREEZE_LANGUAGE_STRINGS[LNG_CONFIGURE_ANALOG], 76, analogRowOffset + (s16)(halfPadding + 90), FONT_SMALL, ORANGE);
		for (i = 0; i < numAnalogs; i++)
		{
			selected = analogIDs[i] + multitapOffset;
			rowOffset = i * 10 + 100;
			rowY = analogRowOffset + rowOffset;
			DecalFont_DrawLine(MAIN_FREEZE_LANGUAGE_STRINGS[MAIN_FREEZE_OPTIONS_PAD_LABELS[selected]], 256, halfPadding + rowY, FONT_SMALL,
			                   JUSTIFY_CENTER | ORANGE);
		}
	}
	DecalFont_DrawLine(MAIN_FREEZE_LANGUAGE_STRINGS[LNG_OPTIONS_EXIT], 76, (s16)(halfPadding + 140) - rowPadding, FONT_SMALL, ORANGE);
	innerY = halfPadding + 20;
	rect.x = 74;
	rect.w = 364;
	rect.y = MAIN_FREEZE_OPTIONS_HIGHLIGHTS[menu->rowSelected].posY + innerY;
	rect.h = MAIN_FREEZE_OPTIONS_HIGHLIGHTS[menu->rowSelected].sizeY;
	CTR_Box_DrawClearBox(&rect, &MAIN_FREEZE_OPTIONS_HIGHLIGHT_COLOR, TRANS_50_DECAL, GAME_TRACKER->backBuffer->otMem.uiOT, &GAME_TRACKER->backBuffer->primMem);
	rect.x = 66;
	rect.y = halfPadding + 43;
	rect.w = 380;
	rect.h = 2;
	{
#ifdef CTR_NATIVE
		// NOTE(aalhendi): Stage packed storage as a real Color for native aliasing.
		Color edgeColor = ColorCode_Load(&MAIN_FREEZE_OPTIONS_EDGE_COLOR);
		const Color *edgeColorPtr = &edgeColor;
#else
		const Color *edgeColorPtr = (const Color *)&MAIN_FREEZE_OPTIONS_EDGE_COLOR;
#endif
		RECTMENU_DrawOuterRect_Edge(&rect, edgeColorPtr, 0x20, GAME_TRACKER->backBuffer->otMem.uiOT);
	}
	rect.x = 56;
	rect.y = innerY;
	rect.w = 400;
	rect.h = 135 - rowPadding;
	RECTMENU_DrawInnerRect(&rect, 4, GAME_TRACKER->backBuffer->otMem.uiOT);
	if (exitMenu || (GAME_ANY_PLAYER_TAP & (BTN_TRIANGLE | BTN_START | BTN_SQUARE_one)))
	{
		OtherFX_Play(1, 1);
		OptionsMenu_TestSound(0, 0);
		RECTMENU_ClearInput();
		GAME_DESIRED_MENU = MainFreeze_GetMenuPtr();
	}
}

void MainFreeze_MenuPtrQuit(struct RectMenu *menu)
{
	s16 row;
	// NOTE(aalhendi): Keep the menu pointer in v1 across the opening branch;
	// the retail row dispatch then reuses that register for the selected row.
	register struct RectMenu *menuCopy CTR_PSX_REGISTER("$3") = menu;

	if (menu->funcState != RECTMENU_FUNC_STATE_INPUT)
	{
		menu->drawStyle &= ~RECTMENU_DRAW_STYLE_3P4P_LAYOUT;

		// if more than 2 screens
		if (GAME_TRACKER->numPlyrCurrGame > 2)
		{
			menu->drawStyle |= RECTMENU_DRAW_STYLE_3P4P_LAYOUT;
		}

		MainFreeze_SafeAdvDestroy();
		return;
	}

	row = menuCopy->rowSelected;
	switch (row)
	{
	case 1:
	case -1:
	{
		GAME_DESIRED_MENU = MainFreeze_GetMenuPtr();
		break;
	}
	case 0:
	{
		// Erase ghost of previous race from RAM
		GhostTape_Destroy();

		// Add bit for "in menu" when loading is done
		sdata->Loading.OnBegin.AddBitsConfig0 |= MAIN_MENU;

		// Go to main menu
		GAME_MAIN_MENU_STATE = MAIN_MENU_TITLE;

		// Remove bit for "In Adventure Arena" when loading is done
		sdata->Loading.OnBegin.RemBitsConfig0 |= ADVENTURE_ARENA;

		// Unpause game
		GAME_TRACKER->gameMode1 &= ~PAUSE_1;

		// Level ID for main menu (39)
		MainRaceTrack_RequestLoad(0x27);
		break;
	}
	default:
	{
		break;
	}
	}
	return;
}

void MainFreeze_SafeAdvDestroy(void)
{
	// If you're in Adventure Arena
	if ((GAME_TRACKER->gameMode1 & ADVENTURE_ARENA) == 0)
	{
		return;
	}

	// check if Adv Hub is loaded
	if (!LOAD_IsOpen_AdvHub())
	{
		return;
	}

	AH_Pause_Destroy();
	return;
}

void MainFreeze_MenuPtrDefault(struct RectMenu *menu)
{
	register struct GameTracker *tracker CTR_PSX_REGISTER("$6") = GAME_TRACKER;
	register s32 signedStringID CTR_PSX_REGISTER("$5");
	register u32 stringID CTR_PSX_REGISTER("$16");
	register s32 optionsID CTR_PSX_REGISTER("$3");
	register struct RectMenu *activeMenu CTR_PSX_REGISTER("$7") = menu;
	s16 levID = 0; // dingo canyon
	s16 selectedOption;
	s32 replayHuman;
	register u32 addBits0 CTR_PSX_REGISTER("$2");
	register u32 remSource CTR_PSX_REGISTER("$3");
	register u32 remMask CTR_PSX_REGISTER("$2");
	register u32 remBits0 CTR_PSX_REGISTER("$6");
	register u32 tokenBits CTR_PSX_REGISTER("$2");
	register struct GameTracker *mapTracker CTR_PSX_REGISTER("$5");
	register u32 gameMode CTR_PSX_REGISTER("$2");
	register u32 pauseMask CTR_PSX_REGISTER("$3");
	register u32 modeForBranch CTR_PSX_REGISTER("$4");
	register u32 cupRemBits CTR_PSX_REGISTER("$3");
	register u32 bossMask CTR_PSX_REGISTER("$3");
	register u32 bossRemBits CTR_PSX_REGISTER("$3");
	register u32 bossAddBits8 CTR_PSX_REGISTER("$2");
	register s32 cupLevel CTR_PSX_REGISTER("$4");
	register s32 cupID CTR_PSX_REGISTER("$2");
	register struct MenuRow *row CTR_PSX_REGISTER("$2");
	register struct MenuRow *rows CTR_PSX_REGISTER("$3");
	u32 rowOffset;
	struct RectMenu *quitMenu;

	// if you have not waited 5 frames since the game was paused then quit
	if (tracker->cooldownfromPauseUntilUnpause != 0)
	{
		return;
	}
	CTR_PSX_KEEP_VALUE_RELAXED(activeMenu);
	// assume 5 frames have passed since paused

	if (menu->funcState != RECTMENU_FUNC_STATE_INPUT)
	{
		menu->drawStyle &= ~RECTMENU_DRAW_STYLE_3P4P_LAYOUT;

		// if more than 2 screens
		if (2 < tracker->numPlyrCurrGame)
		{
			menu->drawStyle |= RECTMENU_DRAW_STYLE_3P4P_LAYOUT;
		}

		if (((GAME_TRACKER->gameMode1 & ADVENTURE_ARENA) == 0) || (menu->state & NEEDS_TO_CLOSE))
		{
			return;
		}

		// quit adv hub if it's not loaded
		if (!LOAD_IsOpen_AdvHub())
		{
			return;
		}

		AH_Pause_Update();
		return;
	}

	if (menu->rowSelected < 0)
	{
		return;
	}

	// NOTE(aalhendi): Keep retail's six-byte row stride and two halfword reads;
	// the signed read drives the early menu cases, the unsigned read the dispatch.
	rowOffset = menu->rowSelected * 3;
	CTR_PSX_KEEP_VALUE_RELAXED(rowOffset);
	CTR_PSX_LOAD_WORD(rows, menu->rows);
	CTR_PSX_DEPEND_VALUE(rowOffset, rows);
	rowOffset <<= 1;
	CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(row, rows, rowOffset);
	optionsID = 14;
	CTR_PSX_KEEP_VALUE_RELAXED(optionsID);
	CTR_PSX_LOAD_SIGNED_HALF(signedStringID, row, 0, row->stringIndex);
	CTR_PSX_LOAD_UNSIGNED_HALF(stringID, row, 0, row->stringIndex);

	// stringID 14: "OPTIONS"
	if (signedStringID == optionsID)
	{
		// Set Menu to Options
		GAME_DESIRED_MENU = &data.menuRacingWheelConfig;

		data.menuRacingWheelConfig.rowSelected = 8;
		return;
	}

	// stringID 11: "AKU AKU HINTS"
	// stringID 12: "UKA UKA HINTS"
	if ((u16)(stringID - 11) < 2)
	{
		// Set Menu to Hints
		GAME_DESIRED_MENU = &D232.menuHintMenu; // in 232
		return;
	}

	// stringID 3: "QUIT"
	if (signedStringID == 3)
	{
		// Set Menu to Quit
		quitMenu = &data.menuQuit;
		quitMenu->rowSelected = 1;
		GAME_DESIRED_MENU = quitMenu;
		return;
	}

	// must wait 5 frames until next pause
	GAME_TRACKER->cooldownFromUnpauseUntilPause = 5;
	CTR_PSX_MEMORY_BARRIER();

	// hide Menu
	RECTMENU_Hide(activeMenu);

	MainFreeze_SafeAdvDestroy();
	CTR_PSX_MEMORY_BARRIER();

	// The retail dispatch indexes the menu rows from zero.
	selectedOption = stringID - 1;
	CTR_PSX_KEEP_VALUE(stringID);
	switch (selectedOption)
	{
	// stringID 1: "RESTART"
	case 0:

	// stringID 4: "RETRY"
	case 3:
	{
		// get rid of pause flag
		GAME_TRACKER->gameMode1 &= ~PAUSE_1;

		if (RaceFlag_IsFullyOffScreen() == 1)
		{
			// checkered flag, begin transition on-screen
			RaceFlag_BeginTransition(1);
		}

		replayHuman = GHOST_REPLAY_HUMAN;
		CTR_PSX_KEEP_VALUE_RELAXED(replayHuman);
		// restart race
		sdata->Loading.stage = LOAD_RESTART;
		CTR_PSX_MEMORY_BARRIER();

		// if you are not showing a ghost during a race
		if (replayHuman == 0)
		{
			return;
		}

		// If the ghost playing buffer is nullptr
		if (GHOST_PLAYING == 0)
		{
			return;
		}

		// Make P2 the character that is saved in the header of the
		// ghost that you will see in the race
		data.characterIDs[1] = GHOST_PLAYING->characterID;
		return;
	}

	// stringID 2: "RESUME"
	case 1:
	{
		// unpause game
		ElimBG_Deactivate(GAME_TRACKER);

		// get rid of pause flag
		GAME_TRACKER->gameMode1 &= ~PAUSE_1;

		// unpause audio
		MainFrame_TogglePauseAudio(0);

		// play pause/unpause sound
		OtherFX_Play(1, 1);
		return;
	}

	// stringID 5: "CHANGE CHARACTER"
	case 4:
	{
		// erase ghost of previous race from RAM
		GhostTape_Destroy();

		// set level ID to main menu
		levID = MAIN_MENU_LEVEL;

		// return to character selection
		GAME_MAIN_MENU_STATE = MAIN_MENU_CHARACTERS;

		// when loading is done, add bit for "in mb"
		sdata->Loading.OnBegin.AddBitsConfig0 |= MAIN_MENU;

		// get rid of pause flag
		GAME_TRACKER->gameMode1 &= ~PAUSE_1;
		MainRaceTrack_RequestLoad(levID);
		return;
	}

	// stringID 6: "CHANGE LEVEL"
	case 5:
	{
		// erase ghost of previous race from RAM
		GhostTape_Destroy();

		// level ID of main mb
		levID = MAIN_MENU_LEVEL;

		// return to track selection
		GAME_MAIN_MENU_STATE = MAIN_MENU_TRACK_SELECT;

		// when loading is done
		// add bit for "in mb"
		sdata->Loading.OnBegin.AddBitsConfig0 |= MAIN_MENU;

		// get rid of pause flag
		GAME_TRACKER->gameMode1 &= ~PAUSE_1;
		MainRaceTrack_RequestLoad(levID);
		return;
	}

	// stringID 10: "CHANGE SETUP"
	case 9:
	{
		// set level ID to main menu
		levID = MAIN_MENU_LEVEL;

		// return to battle setup
		GAME_MAIN_MENU_STATE = MAIN_MENU_BATTLE_SETUP;

		// when loading is done
		// add bit for "in mb"
		sdata->Loading.OnBegin.AddBitsConfig0 |= MAIN_MENU;

		// get rid of pause flag
		GAME_TRACKER->gameMode1 &= ~PAUSE_1;
		MainRaceTrack_RequestLoad(levID);
		return;
	}

	// stringID 13: "EXIT TO MAP"
	case 12:
	{
		// when loading is done
		// add this bit for In Adventure Arena
		addBits0 = sdata->Loading.OnBegin.AddBitsConfig0;
		CTR_PSX_KEEP_VALUE_RELAXED(addBits0);
		addBits0 |= ADVENTURE_ARENA;
		CTR_PSX_MEMORY_BARRIER();

		// when loading is done
		// remove bits for Relic Race or Crystal Challenge
		remSource = sdata->Loading.OnBegin.RemBitsConfig0;
		CTR_PSX_KEEP_VALUE_RELAXED(remSource);
		sdata->Loading.OnBegin.AddBitsConfig0 = addBits0;
		CTR_PSX_MEMORY_BARRIER();
		remMask = RELIC_RACE | CRYSTAL_CHALLENGE;
		CTR_PSX_KEEP_VALUE_RELAXED(remMask);
		remBits0 = remSource | remMask;

		// when loading is done
		// remove bit for CTR Token Challenge
		tokenBits = sdata->Loading.OnBegin.RemBitsConfig8;
		CTR_PSX_KEEP_VALUE_RELAXED(tokenBits);

		mapTracker = GAME_TRACKER;
		CTR_PSX_KEEP_VALUE_RELAXED(mapTracker);
		tokenBits |= TOKEN_RACE;
		sdata->Loading.OnBegin.RemBitsConfig8 = tokenBits;
		gameMode = mapTracker->gameMode1;
		CTR_PSX_KEEP_VALUE_RELAXED(gameMode);
		pauseMask = ~PAUSE_1;
		CTR_PSX_KEEP_VALUE_RELAXED(pauseMask);
		sdata->Loading.OnBegin.RemBitsConfig0 = remBits0;
		CTR_PSX_MEMORY_BARRIER();
		// get rid of pause flag
		gameMode &= pauseMask;
		mapTracker->gameMode1 = gameMode;
		CTR_PSX_MEMORY_BARRIER();
		CTR_PSX_COPY_VALUE(modeForBranch, gameMode);

		// Adventure Cup returns to Gem Stone Valley.
		if ((modeForBranch & ADVENTURE_CUP) != 0)
		{
			// when loading is done remove bits for Adventure Cup, relic, and crystal challenge
			cupRemBits = remBits0 | ADVENTURE_CUP;
			CTR_PSX_KEEP_VALUE_RELAXED(cupRemBits);
			cupLevel = GEM_STONE_VALLEY;
			CTR_PSX_LOAD_WORD_AFTER(cupID, mapTracker->cup.cupID, cupLevel);
			sdata->Loading.OnBegin.RemBitsConfig0 = cupRemBits;
			CTR_PSX_MEMORY_BARRIER();
			CTR_PSX_KEEP_VALUE_RELAXED(cupID);

			// Level ID
			mapTracker->levelID = cupID + ADVENTURE_CUP_SYNTHETIC_LEVEL_ID_BASE;
			MainRaceTrack_RequestLoad(cupLevel);
			return;
		}

		// A boss race also marks the return spawn point.
		if ((s32)modeForBranch < 0)
		{
			// NOTE(aalhendi): The spawn-flag load sits between the boss mask
			// and its OR; the clear-mask store precedes the spawn-bit OR.
			// when loading is done remove bit for Boss Race, relic, and crystal challenge
			bossMask = ADVENTURE_BOSS;
			CTR_PSX_KEEP_VALUE_RELAXED(bossMask);
			CTR_PSX_LOAD_WORD_AFTER(bossAddBits8, sdata->Loading.OnBegin.AddBitsConfig8, bossMask);
			CTR_PSX_DEPEND_VALUE(bossMask, bossAddBits8);
			bossRemBits = remBits0 | bossMask;
			CTR_PSX_KEEP_VALUE_RELAXED(bossRemBits);
			sdata->Loading.OnBegin.RemBitsConfig0 = bossRemBits;
			CTR_PSX_MEMORY_BARRIER();

			// When loading is done add bit to spawn driver near boss door
			bossAddBits8 |= SPAWN_AT_BOSS;
			sdata->Loading.OnBegin.AddBitsConfig8 = bossAddBits8;
		}

		// Return to the level you were in previously.
		MainRaceTrack_RequestLoad(CTR_ReadS16LE(&mapTracker->prevLEV));
		return;
	}
	default:
	{
		return;
	}
	}
}

struct RectMenu *MainFreeze_GetMenuPtr(void)
{
	struct GameTracker *gGT = GAME_TRACKER;
	u32 gameMode = gGT->gameMode1;

	if ((gameMode & ADVENTURE_ARENA) != 0)
	{
		struct MenuRow *rows = data.rowsAdvHub;
		struct RectMenu *menuResult;
		u32 menuPage;
		s16 hintString;
		// NOTE(aalhendi): Retail branches on the low half of the mask result.
		s16 boolGoodGuy = (s16)VehPickupItem_MaskBoolGoodGuy(gGT->drivers[0]);

		hintString = LNG_UKA_UKA_HINTS;
		if (boolGoodGuy != 0)
		{
			hintString = LNG_AKU_AKU_HINTS;
		}

		rows[1].stringIndex = hintString;
		// NOTE(aalhendi): Materialize the menu address independently of the row
		// pointer; the low-offset add fills the retail return branch delay slot.
		CTR_PSX_LOAD_SYMBOL_PAGE(menuPage, "data+14824");
		CTR_PSX_ADD_PAGE_OFFSET(menuResult, menuPage, 0x4388, &data.menuAdvHub);
		return menuResult;
	}

	if ((gameMode & ADVENTURE_MODE) != 0)
	{
		if ((gameMode & ADVENTURE_CUP) != 0)
		{
			return &data.menuAdvCup;
		}

		return &data.menuAdvRace;
	}

	if ((gameMode & BATTLE_MODE) != 0)
	{
		return &data.menuBattle;
	}

	if ((gGT->gameMode2 & CUP_ANY_KIND) != 0)
	{
		return &data.menuArcadeCup;
	}

	return &data.menuArcadeRace;
}

void MainFreeze_IfPressStart(void)
{
	struct GameTracker *gGT;
	u32 gameMode1;
	struct RectMenu *menu;
	struct RectMenu *activeMenu;
	u32 absolutePage;
	s32 loadInProgress;

	if (RaceFlag_IsFullyOnScreen())
	{
		return;
	}

	gGT = GAME_TRACKER;

	if ((gGT->renderFlags & RENDER_FLAG_CHECKERED_FLAG) != 0)
	{
		return;
	}

	if (sdata->AkuAkuHintState != 0)
	{
		return;
	}

	// NOTE(aalhendi): These two fields use absolute pages in retail, unlike
	// the nearby tracker and hint fields accessed relative to $gp.
	CTR_PSX_LOAD_SYMBOL_PAGE(absolutePage, "sdata_static+2460");
	CTR_PSX_LOAD_WORD_FROM_PAGE(activeMenu, absolutePage, "sdata_static+2460", sdata->ptrActiveMenu);
	if (activeMenu != NULL)
	{
		return;
	}

	gameMode1 = gGT->gameMode1;

	if ((gameMode1 & (END_OF_RACE | PAUSE_ALL)) != 0)
	{
		return;
	}

	if (gGT->levelID == MAIN_MENU_LEVEL)
	{
		return;
	}

	if ((gameMode1 & GAME_CUTSCENE) != 0)
	{
		return;
	}

	if (gGT->boolDemoMode != 0)
	{
		return;
	}

	if ((u32)(gGT->levelID - OXIDE_ENDING) < 2)
	{
		return;
	}

	CTR_PSX_LOAD_SYMBOL_PAGE(absolutePage, "sdata_static+312");
	CTR_PSX_LOAD_WORD_FROM_PAGE(loadInProgress, absolutePage, "sdata_static+312", sdata->load_inProgress);
	if (loadInProgress != 0)
	{
		return;
	}

	if ((gGT->gameMode2 & VEH_FREEZE_PODIUM) != 0)
	{
		return;
	}

	gGT->gameMode1 = gameMode1 | PAUSE_1;

	menu = MainFreeze_GetMenuPtr();
	menu->rowSelected = 0;

	RECTMENU_Show(menu);
	MainFrame_TogglePauseAudio(1);
	OtherFX_Play(1, 1);
	ElimBG_Activate(GAME_TRACKER);
}
