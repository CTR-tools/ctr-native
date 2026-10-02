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


typedef struct
{
	int numGamepads;
	int numAnalogs;
	int gamepadId[4];
	int analogId[4];
	int isGamepadAnalog[4];
	int menuRowsToRemove;
} GAMEPAD_MainFreeze_MenuPtrOptions;

static void IDENTIFYGAMEPADS_MainFreeze_MenuPtrOptions(struct RectMenu *menu, GAMEPAD_MainFreeze_MenuPtrOptions *gamepad)
{
	struct GameTracker *gGT = sdata->gGT;
	b32 areBothControllerLabelsNecessary;
	int i;
	(void)menu;

	// get number of ordinary gamepads and/or "analog controllers" connected, and which players are using which

	for (i = 0; i < gGT->numPlyrCurrGame; i++)
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
	areBothControllerLabelsNecessary = false;
	if (gamepad->numGamepads != 0)
	{
		areBothControllerLabelsNecessary = (gamepad->numAnalogs != 0);
	}

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

static b32 PROCESSINPUTS_MainFreeze_MenuPtrOptions(struct RectMenu *menu, GAMEPAD_MainFreeze_MenuPtrOptions *gamepad)
{
	struct GameTracker *gGT = sdata->gGT;
	b32 exitMenu = false;

	if (sdata->AnyPlayerTap & (BTN_UP | BTN_DOWN))
	{
		// play sound for when you're moving around in the menu
		OtherFX_Play(0, 1);

		// there are only 9 rows total
		if (sdata->AnyPlayerTap & BTN_UP)
		{
			menu->rowSelected = (menu->rowSelected + (9 - 1)) % 9;
			if (menu->rowSelected == 7)
			{
				menu->rowSelected = gGT->numPlyrCurrGame + 3;
			}
		}
		else if (sdata->AnyPlayerTap & BTN_DOWN)
		{
			menu->rowSelected = (menu->rowSelected + 1) % 9;
			if (menu->rowSelected > (gGT->numPlyrCurrGame + 3))
			{
				menu->rowSelected = 8;
			}
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
				int volume = howl_VolumeGet(menu->rowSelected) & 0xff;

				if (sdata->AnyPlayerHold & BTN_LEFT)
				{
					volume -= 4;
				}
				else if (sdata->AnyPlayerHold & BTN_RIGHT)
				{
					volume += 4;
				}

				if (volume < 0)
				{
					volume = 0;
				}
				if (volume > 0xff)
				{
					volume = 0xff;
				}

				howl_VolumeSet(menu->rowSelected, volume);
			}
			break;

		// Mode	(Stereo/Mono)
		case 3:
			// clear test sound
			OptionsMenu_TestSound(0, 0);

			if (sdata->AnyPlayerTap & (BTN_CIRCLE | BTN_CROSS_one))
			{
				int mode;
				OtherFX_Play(1, 1);
				mode = howl_ModeGet();
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
				int gamepadRow;
				OtherFX_Play(1, 1);

				gamepadRow = menu->rowSelected - 4;

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
static void DISPLAYRECTMENU_MainFreeze_MenuPtrOptions(struct RectMenu *menu, GAMEPAD_MainFreeze_MenuPtrOptions *gamepad)
{
	struct GameTracker *gGT = sdata->gGT;
	int volumeSliderTriangleLeftMargin;
	int volumeSliderWidth;
	u32 *ot;
	struct PrimMem *primMem;
	Color color;
	int mode;
	int i;
	RECT cursor;
	RECT titleSeparatorLine;
	RECT menuBG;

	// note: multitap only works if it's connected to the P1 slot
	int multitapStringOffset = (sdata->gGamepads->slotBuffer[0].controller.controllerData == (PAD_ID_MULTITAP << 4)) ? 2 : 0;

	// a menu row is 10 pixels
	int menuRowsNegativePadding = gamepad->menuRowsToRemove * 10;

	int analogRowPosY = 0;
	if (gamepad->numGamepads != 0)
	{
		analogRowPosY = (gamepad->numGamepads + 1) * 10;
	}

	// cursor location for exit button, which will change depending on how many dualshock rows there need to be
	data.Options_HighlightBar[8].posY = gamepad->menuRowsToRemove * -10 + 119;

	// cursor location for dualshock rows
	if (gamepad->numGamepads > 0)
	{
		for (i = 0; i < gamepad->numGamepads; i++)
		{
			data.Options_HighlightBar[i + 4].posY = (i * 10) + 79;
		}
	}

	// cursor location for "analog" rows
	if (gamepad->numAnalogs > 0)
	{
		for (i = 0; i < gamepad->numAnalogs; i++)
		{
			b32 areBothControllerLabelsNecessary = false;
			if (gamepad->numGamepads != 0)
			{
				areBothControllerLabelsNecessary = (gamepad->numAnalogs != 0);
			}

			data.Options_HighlightBar[gamepad->numGamepads + i + 4].posY = ((gamepad->numGamepads + i + areBothControllerLabelsNecessary) * 10) + 79;
		}
	}

	menu->drawStyle &= ~RECTMENU_DRAW_STYLE_3P4P_LAYOUT;
	if (gGT->numPlyrCurrGame > 2)
	{
		menu->drawStyle |= RECTMENU_DRAW_STYLE_3P4P_LAYOUT;
	}

	volumeSliderTriangleLeftMargin = 0;

	for (i = 0; i < 3; i++)
	{
		//"FX:", "MUSIC:", "VOICE:"
		int lineWidth = DecalFont_GetLineWidth(sdata->lngStrings[data.Options_StringIDs_Audio[i]], FONT_SMALL);
		if (volumeSliderTriangleLeftMargin < lineWidth)
		{
			volumeSliderTriangleLeftMargin = lineWidth;
		}
	}

	DecalFont_DrawLine(sdata->lngStrings[LNG_OPTIONS_TITLE], 256, 26 + (menuRowsNegativePadding / 2), FONT_BIG, (JUSTIFY_CENTER | ORANGE));

	volumeSliderWidth = 380 - (30 + volumeSliderTriangleLeftMargin);

	ot = gGT->backBuffer->otMem.uiOT;
	primMem = &gGT->backBuffer->primMem;

	// draw volume sliders
	for (i = 0; i < 3; i++)
	{
		int volume;
		int volumeSliderValue;
		s16 volumeSliderPosY;
		int volumeSliderTriangleLeftPosX;
		int volumeSliderBarPosX;
		s16 volumeSliderTriangle[8];
		RECT volumeSliderBar;
		RECT volumeSliderBarOutline;

		volume = howl_VolumeGet(i) & 0xff;
		volumeSliderValue = volume * (volumeSliderWidth - 5);
		volumeSliderPosY = (menuRowsNegativePadding / 2) + (i * 10);

		if (volumeSliderValue < 0)
		{
			volumeSliderValue += 0xff;
		}

		volumeSliderTriangleLeftPosX = 30 + volumeSliderTriangleLeftMargin;
		volumeSliderBarPosX = 0x38 + volumeSliderTriangleLeftPosX + (s16)((u32)volumeSliderValue >> 8);

		volumeSliderTriangle[0] = volumeSliderTriangleLeftPosX + 56;
		volumeSliderTriangle[1] = volumeSliderPosY + 58;
		volumeSliderTriangle[2] = volumeSliderTriangleLeftPosX + volumeSliderWidth + 56;
		volumeSliderTriangle[3] = volumeSliderPosY + 48;
		volumeSliderTriangle[6] = 0;
		volumeSliderTriangle[7] = 0;

		volumeSliderTriangle[4] = volumeSliderTriangle[2];
		volumeSliderTriangle[5] = volumeSliderTriangle[1];

		volumeSliderBar.x = volumeSliderBarPosX + 1;
		volumeSliderBar.y = volumeSliderPosY + 48;
		volumeSliderBar.w = 3;
		volumeSliderBar.h = 10;
		color = *(Color *)(data.Options_VolumeSlider_Colors + 0xc);
		CTR_Box_DrawSolidBox(&volumeSliderBar, &color, ot, primMem);

		volumeSliderBarOutline.x = volumeSliderBarPosX;
		volumeSliderBarOutline.y = volumeSliderPosY + 47;
		volumeSliderBarOutline.w = 5;
		volumeSliderBarOutline.h = 12;

		color = *(Color *)(data.Options_VolumeSlider_Colors + 0x10);
		CTR_Box_DrawSolidBox(&volumeSliderBarOutline, &color, ot, primMem);

		RECTMENU_DrawRwdTriangle(volumeSliderTriangle, data.Options_VolumeSlider_Colors, ot, primMem);

		// "FX:" "MUSIC:" "VOICE:"
		DecalFont_DrawLine(sdata->lngStrings[data.Options_StringIDs_Audio[i]], 76, 50 + (menuRowsNegativePadding / 2) + (i * 10), FONT_SMALL, ORANGE);
	}

	DecalFont_DrawLine(sdata->lngStrings[LNG_MODE], 76, 80 + (menuRowsNegativePadding / 2), FONT_SMALL, ORANGE);

	// 333: MONO
	// 334: STEREO
	mode = howl_ModeGet();

	// "MONO", "STEREO"
	DecalFont_DrawLine(sdata->lngStrings[333 + mode], 436, 80 + (menuRowsNegativePadding / 2), FONT_SMALL, (JUSTIFY_RIGHT | WHITE));

	if (gamepad->numGamepads != 0)
	{
		int lineWidth_controller1A;
		int lineWidth_vibrateOff;
		int lineWidth_vibrateOn;
		DecalFont_DrawLine(sdata->lngStrings[LNG_DUAL_SHOCK], 76, 90 + (menuRowsNegativePadding / 2), FONT_SMALL, ORANGE);

		lineWidth_controller1A = DecalFont_GetLineWidth(sdata->lngStrings[data.Options_StringIDs_Gamepads[2]], FONT_SMALL);

		// width can change depending on language
		lineWidth_vibrateOff = DecalFont_GetLineWidth(sdata->lngStrings[LNG_VIBRATE_OFF], FONT_SMALL);
		lineWidth_vibrateOn = DecalFont_GetLineWidth(sdata->lngStrings[LNG_VIBRATE_ON], FONT_SMALL);
		if (lineWidth_vibrateOn < lineWidth_vibrateOff)
		{
			lineWidth_vibrateOn = lineWidth_vibrateOff;
		}

		lineWidth_vibrateOn = (lineWidth_controller1A + lineWidth_vibrateOn + 10);
		lineWidth_vibrateOn = 256 - (lineWidth_vibrateOn >> 1);

		for (i = 0; i < gamepad->numGamepads; i++)
		{
			int dualShockRowColor = ORANGE;
			int currPad = gamepad->gamepadId[i];
			struct ControllerPacket *ptrControllerPacket = sdata->gGamepads->gamepad[currPad].ptrControllerPacket;
			int rowY;
			b32 boolDisabled;

			if (ptrControllerPacket == 0 || ptrControllerPacket->plugged != PLUGGED)
			{
				dualShockRowColor = GRAY;
			}

			rowY = 100 + (menuRowsNegativePadding / 2) + (i * 10);

			// "CONTROLLER 1", "CONTROLLER 2",
			// "CONTROLLER 1A", "CONTROLLER 1B",
			// "CONTROLLER 1C", "CONTROLLER 1D"
			DecalFont_DrawLine(sdata->lngStrings[data.Options_StringIDs_Gamepads[currPad + multitapStringOffset]], lineWidth_vibrateOn, rowY, FONT_SMALL,
			                   dualShockRowColor);

			boolDisabled = (gGT->gameMode1 & data.gGT_gameMode1_VibPerPlayer[currPad]) != 0;

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

		for (i = 0; i < gamepad->numAnalogs; i++)
		{
			DecalFont_DrawLine(sdata->lngStrings[data.Options_StringIDs_Gamepads[gamepad->analogId[i] + multitapStringOffset]], 256,
			                   100 + (menuRowsNegativePadding / 2) + analogRowPosY + (i * 10), FONT_SMALL, (JUSTIFY_CENTER | ORANGE));
		}
	}

	DecalFont_DrawLine(sdata->lngStrings[LNG_OPTIONS_EXIT], 76, 140 - (menuRowsNegativePadding / 2), FONT_SMALL, ORANGE);

	cursor.x = 74;
	cursor.y = data.Options_HighlightBar[menu->rowSelected].posY + (menuRowsNegativePadding / 2) + 20;
	cursor.w = 364;
	cursor.h = data.Options_HighlightBar[menu->rowSelected].sizeY;

	CTR_Box_DrawClearBox(&cursor, &sdata->menuRowHighlight_Normal, TRANS_50_DECAL, ot, primMem);

	titleSeparatorLine.x = 66;
	titleSeparatorLine.y = (menuRowsNegativePadding / 2) + 43;
	titleSeparatorLine.w = 380;
	titleSeparatorLine.h = 2;

	ColorCode_SetPacked(&color, sdata->battleSetup_Color_UI_1);
	RECTMENU_DrawOuterRect_Edge(&titleSeparatorLine, &color, 0x20, ot);

	menuBG.x = 56;
	menuBG.y = (menuRowsNegativePadding / 2) + 20;
	menuBG.w = 400;
	menuBG.h = 135 - menuRowsNegativePadding;

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
	{
		GAMEPAD_MainFreeze_MenuPtrOptions gamepad = {0, 0, {-1, -1, -1, -1}, {-1, -1, -1, -1}, {false, false, false, false}, 0};
		b32 exitMenu;

		IDENTIFYGAMEPADS_MainFreeze_MenuPtrOptions(menu, &gamepad);
		exitMenu = PROCESSINPUTS_MainFreeze_MenuPtrOptions(menu, &gamepad);
		DISPLAYRECTMENU_MainFreeze_MenuPtrOptions(menu, &gamepad);

		if (exitMenu || (sdata->AnyPlayerTap & (BTN_TRIANGLE | BTN_START | BTN_SQUARE_one)))
		{
			OtherFX_Play(1, 1);
			OptionsMenu_TestSound(0, 0);
			RECTMENU_ClearInput();
			sdata->ptrDesiredMenu = MainFreeze_GetMenuPtr();
		}
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
