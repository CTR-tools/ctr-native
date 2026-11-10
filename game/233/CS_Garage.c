#include <common.h>

enum GarageInputMask
{
	GARAGE_INPUT_HORIZONTAL = BTN_LEFT | BTN_RIGHT,
	GARAGE_INPUT_MENU = BTN_TRIANGLE | BTN_CROSS_one | BTN_CIRCLE | BTN_SQUARE_one,
	GARAGE_INPUT_CONFIRM = BTN_CROSS_one | BTN_CIRCLE,
	GARAGE_INPUT_BACK = BTN_TRIANGLE | BTN_SQUARE_one,
};

enum
{
	GARAGE_STAT_BAR_RATE = 3,
	GARAGE_STAT_BAR_SEGMENT_COUNT = 6,
	GARAGE_STAT_BAR_SEGMENT_WIDTH = 13,
	GARAGE_STAT_BAR_ROW_STEP = 15,
	GARAGE_STAT_BAR_POLY_G4_CODE = 0x38,
	GARAGE_STAT_BAR_POLY_G4_COLOR_CODE = GARAGE_STAT_BAR_POLY_G4_CODE << 24,
	GARAGE_STAT_NAME_SPEED_Y = 30,
	GARAGE_STAT_NAME_ACCEL_Y = 0x2d,
	GARAGE_STAT_NAME_TURN_Y = 60,
	GARAGE_CLASS_NAME_Y = 15,
	GARAGE_STAT_BAR_SHADOW_Y = 34,
	GARAGE_STAT_BAR_HEIGHT = 7,
	GARAGE_STAT_BAR_SHADOW_HEIGHT = 5,
	GARAGE_STAT_BOX_Y = 11,
	GARAGE_STAT_BOX_HEIGHT = 68,
	GARAGE_STAT_BOX_PADDING_X = 6,
	GARAGE_SCREEN_WIDTH = 512,
	GARAGE_CLASS_COUNT = 3,
	GARAGE_ENGINE_COUNT = 4,
	GARAGE_STAT_COUNT = 3,
	GARAGE_OSK_DELAY_FRAMES = 60,
	GARAGE_CAMERA_FRAME_STEP = 30,
	GARAGE_CAMERA_WRAP_RIGHT_FRAME = 240,
	GARAGE_CAMERA_WRAP_LEFT_FRAME = 210,
	GARAGE_CHARACTER_NAME_X = 0x100,
	GARAGE_CHARACTER_NAME_Y = 0xb4,
	GARAGE_CHARACTER_ARROW_Y = 187,
	GARAGE_CHARACTER_ARROW_LEFT_BASE_X = 236,
	GARAGE_CHARACTER_ARROW_RIGHT_BASE_X = 274,
	GARAGE_CHARACTER_ARROW_ICON_INDEX = 0x38,
	GARAGE_CHARACTER_ARROW_SCALE = 0x1000,
	GARAGE_CHARACTER_ARROW_ROT_LEFT = 0x800,
};

void CS_Garage_ZoomOut(s16 zoomState)
{
	if (zoomState != 0)
	{
		// number of frames to zoom in, or out,
		// when selecting or cancelling OSK
		CS_GARAGE_ZOOM_IN = CS_GARAGE_ZOOM_FRAMES;
		CS_GARAGE_ZOOM_OUT = CS_GARAGE_ZOOM_FRAMES;
	}
	else
	{
		CS_GARAGE_ZOOM_IN = 0;
		CS_GARAGE_ZOOM_OUT = 0;
	}

	CS_GARAGE_MOVE_FRAME = 0;
	CS_GARAGE_SELECTED = 0;
	CS_GARAGE_DELAY = 0;

	GAME_TRACKER->gameMode2 &= ~(GARAGE_OSK);

	// if just entered garage
	if (zoomState == 0)
	{
		Garage_Init();
		Garage_Enter((s16)sdata->advCharSelectIndex_curr);

		Audio_SetState_Safe(AUDIO_GARAGE);
	}
}

void CS_Garage_MenuProc(struct RectMenu *menu)
{
	s32 garageFrames;
	s32 i;
	s16 *statBarLength;
	u16 classNamePosX;
	u32 statNamePosX;
	u32 statBarPosX;

	struct PushBuffer *pushBuffer;

	RECT r;
	RECT barRect;
	union
	{
		RECT rect;
		Color color;
	} scratch;
	s32 engineID;
	s16 classStringIndex;

	s32 rowIndex;
	s32 statBarShadows_Y;

	s32 arrowColor;

	s32 nameLen;

	SVec3 camPos;
	SVec3 camRot;
	s16 pathFlags;
	s32 zoom;
	(void)menu;

	GAME_TRACKER->cameraDC[0].cameraMode = CAMERA_MODE_FREECAM;

	// Draw and accept input only while the selection camera is stationary.
	if (CS_GARAGE_MOVE_FRAME == 0)
	{
		const int character = CS_GARAGE_CHARACTERS[CS_GARAGE_CURRENT];
		int selectedEngine = VehBirth_EngineClassForPlayer(0, GAME_CHARACTER_METADATA[character].engineID);
		if (g_config.engineSelection && CS_GARAGE_SELECTED == 0 && (GAME_ANY_PLAYER_TAP & (BTN_L1 | BTN_R1)) != 0)
		{
			selectedEngine = (selectedEngine + ((GAME_ANY_PLAYER_TAP & BTN_L1) != 0 ? NUM_CLASSES - 1 : 1)) % NUM_CLASSES;
			sdata->selectedEngineClass[0] = selectedEngine + 1;
			GAME_ANY_PLAYER_TAP &= ~(BTN_L1 | BTN_R1);
			OtherFX_Play(0, 1);
		}
		// Retain the unused garage-frame counter.
		CS_GARAGE_UNUSED_FRAMES++;

		// Animate each bar toward the selected engine class.
		{
			s16 *selectedID;
			const void *selectedBase = CS_GARAGE_CHARACTERS;
			selectedID = (s16 *)((const u8 *)selectedBase + CS_GARAGE_CURRENT * 2);
			for (i = 0; i < 3; i++)
			{
				const void *targetBase = CS_GARAGE_STAT_TARGETS;

				statBarLength = &CS_GARAGE_STAT_LENGTHS[i];

				const int target = selectedEngine == CUSTOM_MAX ? 80 : *(s16 *)((selectedEngine * 6 + i * 2) + (u8 *)targetBase);
				(void)selectedID;
				if (*statBarLength < target)
				{
					*statBarLength = *statBarLength + GARAGE_STAT_BAR_RATE;
				}
				if (*statBarLength > target)
				{
					*statBarLength = target;
				}
			}
		}

		if ((GAME_CHARACTER_METADATA[CS_GARAGE_CHARACTERS[CS_GARAGE_CURRENT]].name_LNG_long != LNG_TINY_TIGER) &&

		    (statNamePosX = 383,

		     GAME_CHARACTER_METADATA[CS_GARAGE_CHARACTERS[CS_GARAGE_CURRENT]].name_LNG_long != LNG_PURA_FULL))
		{
			classNamePosX = 384;
			statBarPosX = 393;
		}
		else
		{
			statNamePosX = 129;
			classNamePosX = 128;
			statBarPosX = 139;
		}

		// Keep the retail font size while fitting every class, label and stat bar.
		int classWidth = DecalFont_GetLineWidth("INTERMEDIATE", FONT_BIG);
		int statWidth = 0;
		int barWidth = 0;
		const int statStrings[] = {LNG_SPEED, LNG_ACCEL, LNG_TURN};
		for (int j = 0; j < GARAGE_CLASS_COUNT; j++)
		{
			int width = DecalFont_GetLineWidth(GAME_LANGUAGE_STRINGS[CS_GARAGE_CLASS_STRINGS[j]], FONT_BIG);
			if (width > classWidth)
			{
				classWidth = width;
			}
		}
		for (int j = 0; j < GARAGE_STAT_COUNT; j++)
		{
			int width = DecalFont_GetLineWidth(GAME_LANGUAGE_STRINGS[statStrings[j]], FONT_BIG);
			if (width > statWidth)
			{
				statWidth = width;
			}
		}
		for (int j = 0; j < GARAGE_ENGINE_COUNT * GARAGE_STAT_COUNT; j++)
		{
			if (CS_GARAGE_STAT_TARGETS[j] > barWidth)
			{
				barWidth = CS_GARAGE_STAT_TARGETS[j];
			}
		}
		int left = classNamePosX - (classWidth + 1) / 2;
		int right = classNamePosX + (classWidth + 1) / 2;
		int statLeft = (int)statNamePosX - statWidth;
		int barRight = (int)statBarPosX + barWidth;
		if (statLeft < left)
		{
			left = statLeft;
		}
		if (barRight > right)
		{
			right = barRight;
		}
		left -= GARAGE_STAT_BOX_PADDING_X;
		right += GARAGE_STAT_BOX_PADDING_X;
		int shift = 0;
		if (left < 0)
		{
			shift = -left;
		}
		else if (right > GARAGE_SCREEN_WIDTH)
		{
			shift = GARAGE_SCREEN_WIDTH - right;
		}
		classNamePosX += shift;
		statNamePosX += shift;
		statBarPosX += shift;
		r.x = left + shift;
		r.w = right - left;

		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_SPEED], statNamePosX, GARAGE_STAT_NAME_SPEED_Y, FONT_BIG, JUSTIFY_RIGHT | ORANGE_RED);
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_ACCEL], statNamePosX, GARAGE_STAT_NAME_ACCEL_Y, FONT_BIG, JUSTIFY_RIGHT | LIME_GREEN);
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_TURN], statNamePosX, GARAGE_STAT_NAME_TURN_Y, FONT_BIG, JUSTIFY_RIGHT | BLUE);

		engineID = selectedEngine;
		if (engineID == 3)
		{
			classStringIndex = 0;
		}
		else if ((u32)engineID < 2)
		{
			classStringIndex = 1;
		}
		else
		{
			classStringIndex = 2;
		}

		// Draw class name
		if (g_config.engineSelection)
		{
			static char *const names[NUM_CLASSES] = {"BALANCED", "ACCEL", "SPEED", "TURN", "CUSTOM MAX"};
			DecalFont_DrawLine(names[engineID], classNamePosX, GARAGE_CLASS_NAME_Y, FONT_BIG, JUSTIFY_CENTER | ORANGE);
		}
		else
		{
			DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[CS_GARAGE_CLASS_STRINGS[classStringIndex]], classNamePosX, GARAGE_CLASS_NAME_Y, FONT_BIG,
			                   JUSTIFY_CENTER | ORANGE);
		}

		// NOTE(aalhendi): Keep the drawing row separate from the loop count so
		// GCC advances the row coordinates before the length pointer, as in retail.
		i = 0;
		rowIndex = 0;
		for (statBarLength = CS_GARAGE_STAT_LENGTHS; i < 3; rowIndex++, statBarLength++, i++)
		{
			s32 segmentStart;
			s32 segmentEnd;

			statBarShadows_Y = GARAGE_STAT_BAR_SHADOW_Y + GARAGE_STAT_BAR_ROW_STEP * rowIndex;
			scratch.rect.x = statBarPosX;
			scratch.rect.y = statBarShadows_Y - 1;
			scratch.rect.w = *statBarLength;
			scratch.rect.h = GARAGE_STAT_BAR_HEIGHT;

			// Copy the white template before reusing the scratch color for shadows.
			barRect = scratch.rect;
			scratch.color = csGarageWhite;
			CTR_Box_DrawWireBox(&barRect, &scratch.color, GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);

			// bar shadows
			barRect.x = statBarPosX + 1;
			barRect.y = statBarShadows_Y;
			barRect.w = *statBarLength - 2;
			barRect.h = GARAGE_STAT_BAR_SHADOW_HEIGHT;

			// The shadow has the same code byte and black RGB.
			scratch.color.r = 0;
			scratch.color.g = 0;
			scratch.color.b = 0;
			CTR_Box_DrawWireBox(&barRect, &scratch.color, GAME_TRACKER->pushBuffer_UI.ptrOT, &GAME_TRACKER->backBuffer->primMem);

			{
				s16 *length;
				s32 top, bottom;
				s32 segmentIndex = 0;
				length = statBarLength;

				for (; segmentIndex < GARAGE_STAT_BAR_SEGMENT_COUNT; segmentIndex++)
				{
					// Each segment blends adjacent colors in the seven-color ramp.
					u32 *barColor = &CS_GARAGE_STAT_COLORS[segmentIndex];
					s32 segmentWidth = GARAGE_STAT_BAR_SEGMENT_WIDTH;
					s16 currSegmentLen;
					s32 primitiveColorCode = GARAGE_STAT_BAR_POLY_G4_COLOR_CODE;
					top = statBarShadows_Y - 1;
					bottom = statBarShadows_Y + 6;
					segmentEnd = segmentWidth + segmentWidth * segmentIndex;
					segmentStart = segmentEnd - GARAGE_STAT_BAR_SEGMENT_WIDTH;
					if (*length <= segmentEnd)
					{
						currSegmentLen = *length - segmentStart;
					}
					else
					{
						currSegmentLen = GARAGE_STAT_BAR_SEGMENT_WIDTH;
					}
					if ((s32)((u32)currSegmentLen << 16) < 0)
					{
						currSegmentLen = 0;
					}
					if (segmentStart + currSegmentLen <= *length)
					{
						// The guard reserves space for one complete primitive.
						struct DB *buffer = GAME_TRACKER->backBuffer;
						POLY_G4 *p = NULL;
						if (buffer->primMem.cursor <= buffer->primMem.guardEnd)
						{
							p = buffer->primMem.cursor;
							buffer->primMem.cursor = p + 1;
						}
						if (!p)
						{
							return;
						}
						// color data
						CtrGpu_WriteColorCode(&p->r0, barColor[0] | primitiveColorCode);
						CtrGpu_WriteColorCode(&p->r1, CS_GARAGE_STAT_COLORS[segmentIndex + 1] | primitiveColorCode);
						CtrGpu_WriteColorCode(&p->r2, barColor[0] | primitiveColorCode);
						CtrGpu_WriteColorCode(&p->r3, CS_GARAGE_STAT_COLORS[segmentIndex + 1] | primitiveColorCode);

						// NOTE(aalhendi): Narrow the signed offset before packing X; this keeps
						// GCC from creating a second, absolute-X induction counter.
						p->x0 = statBarPosX - (s16)-segmentStart;
						p->x2 = p->x0;
						p->x1 = p->x0 + currSegmentLen;
						p->y0 = top;
						p->y1 = top;
						p->y2 = bottom;
						p->y3 = bottom;
						p->x3 = p->x2 + currSegmentLen;
						{
							u32 *ot = GAME_TRACKER->pushBuffer_UI.ptrOT;
							u32 link = CtrGpu_PrimToOTLink24(p);
							p->tag = *ot | 0x08000000;
							*ot = link;
						}
					}
				}
			}
		}

		// Stats box
		r.y = GARAGE_STAT_BOX_Y;
		r.h = GARAGE_STAT_BOX_HEIGHT;

		// Draw 2D Menu rectangle background
		RECTMENU_DrawInnerRect(&r, 4, GAME_TRACKER->backBuffer->otMem.uiOT);

		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[GAME_CHARACTER_METADATA[CS_GARAGE_CHARACTERS[CS_GARAGE_CURRENT]].name_LNG_long], GARAGE_CHARACTER_NAME_X,
		                   GARAGE_CHARACTER_NAME_Y, FONT_BIG, JUSTIFY_CENTER | ORANGE);
		arrowColor = ORANGE;
		if ((CS_FRAME_COUNTER_LOW & 4) == 0)
		{
			arrowColor = RED;
		}
		nameLen =
		    (s16)DecalFont_GetLineWidth(GAME_LANGUAGE_STRINGS[GAME_CHARACTER_METADATA[CS_GARAGE_CHARACTERS[CS_GARAGE_CURRENT]].name_LNG_long], FONT_BIG) / 2;
		CS_DECALHUD_ARROW_2D((ICONGROUP_GETICONS(GAME_TRACKER->iconGroup[4]))[GARAGE_CHARACTER_ARROW_ICON_INDEX], GARAGE_CHARACTER_ARROW_LEFT_BASE_X - nameLen,
		                     GARAGE_CHARACTER_ARROW_Y, &GAME_TRACKER->backBuffer->primMem, GAME_TRACKER->pushBuffer_UI.ptrOT,
		                     ((Color *)CS_COLOR_POINTERS[arrowColor])[0], ((Color *)CS_COLOR_POINTERS[arrowColor])[1],
		                     ((Color *)CS_COLOR_POINTERS[arrowColor])[2], ((Color *)CS_COLOR_POINTERS[arrowColor])[3], 0, GARAGE_CHARACTER_ARROW_SCALE,
		                     GARAGE_CHARACTER_ARROW_ROT_LEFT);
		CS_DECALHUD_ARROW_2D((ICONGROUP_GETICONS(GAME_TRACKER->iconGroup[4]))[GARAGE_CHARACTER_ARROW_ICON_INDEX], GARAGE_CHARACTER_ARROW_RIGHT_BASE_X + nameLen,
		                     GARAGE_CHARACTER_ARROW_Y, &GAME_TRACKER->backBuffer->primMem, GAME_TRACKER->pushBuffer_UI.ptrOT,
		                     ((Color *)CS_COLOR_POINTERS[arrowColor])[0], ((Color *)CS_COLOR_POINTERS[arrowColor])[1],
		                     ((Color *)CS_COLOR_POINTERS[arrowColor])[2], ((Color *)CS_COLOR_POINTERS[arrowColor])[3], 0, GARAGE_CHARACTER_ARROW_SCALE, 0);

		if (((GAME_TRACKER->renderFlags & RENDER_FLAG_CHECKERED_FLAG) != 0) ||
		    (((GAME_ANY_PLAYER_TAP & GARAGE_INPUT_MENU) == 0) && ((CS_BUTTONS_HOLD & GARAGE_INPUT_HORIZONTAL) == 0)))
		{
			goto update_garage_camera;
		}

		if ((CS_BUTTONS_HOLD & GARAGE_INPUT_HORIZONTAL) != 0)
		{
			sdata->selectedEngineClass[0] = 0;
			// erase animated bars

			for (i = 2; i > -1; i--)
			{
				statBarLength = &CS_GARAGE_STAT_LENGTHS[i];
				*statBarLength = 0;
			}

			// Play Sound
			OtherFX_Play(0, 1);

			if (CS_BUTTONS_HOLD & BTN_LEFT)
			{
				CS_GARAGE_PREVIOUS = CS_GARAGE_CURRENT;
				CS_GARAGE_CURRENT = (CS_GARAGE_CURRENT - 1) & 7;
				Garage_MoveLR(CS_GARAGE_CURRENT);
			}
			else if (CS_BUTTONS_HOLD & BTN_RIGHT)
			{
				CS_GARAGE_PREVIOUS = CS_GARAGE_CURRENT;
				CS_GARAGE_CURRENT = (CS_GARAGE_CURRENT + 1) & 7;
				Garage_MoveLR(CS_GARAGE_CURRENT);
			}

			// reset frame counter to max number of frames
			CS_GARAGE_MOVE_FRAME = CS_GARAGE_MOVE_FRAMES;

			if (CS_GARAGE_ZOOM_IN < CS_GARAGE_ZOOM_FRAMES)
			{
				CS_GARAGE_ZOOM_OUT = CS_GARAGE_ZOOM_FRAMES - CS_GARAGE_ZOOM_IN;
			}

			CS_GARAGE_SELECTED = 0;
			GAME_TRACKER->gameMode2 &= ~GARAGE_OSK;
		}
		else
		{
			if ((GAME_ANY_PLAYER_TAP & GARAGE_INPUT_CONFIRM) != 0)
			{
				// First confirm selects the character; a second confirm skips the wait.
				switch (CS_GARAGE_SELECTED)
				{
				case 0:
				{
					CS_GARAGE_SELECTED = 1;
					break;
				}
				case 1:
				{
					// Confirm again to open the name-entry menu.
					CS_DESIRED_MENU = &data.menuSubmitName;

					{
						s16 character = CS_GARAGE_CHARACTERS[CS_GARAGE_CURRENT];
						sdata->advProgress.characterID = character;
						GAME_CHARACTER_IDS[0] = character;
					}

					SubmitName_RestoreName(0);
					OtherFX_Play(1, 1);
					break;
				}
				}
			}
			else
			{
				// If you press Triangle or Square
				if ((GAME_ANY_PLAYER_TAP & GARAGE_INPUT_BACK) != 0)
				{
					// Play Sound
					OtherFX_Play(2, 1);

					if (CS_GARAGE_SELECTED == 1)
					{
						CS_GARAGE_SELECTED = 0;
						GAME_TRACKER->gameMode2 &= ~GARAGE_OSK;

						if (CS_GARAGE_ZOOM_IN < CS_GARAGE_ZOOM_FRAMES)
						{
							CS_GARAGE_ZOOM_OUT = CS_GARAGE_ZOOM_FRAMES - CS_GARAGE_ZOOM_IN;
						}
					}
					else
					{
						// return to main menu
						CS_MAIN_MENU_STATE = MAIN_MENU_TITLE;

						Garage_Leave();

						// load main menu LEV
						MainRaceTrack_RequestLoad(MAIN_MENU_LEVEL);
					}
				}
			}
		}
		// clear gamepad input (for menus)
		RECTMENU_ClearInput();
	}
	else
	{
		CS_GARAGE_MOVE_FRAME--;
	}
update_garage_camera:

	// Advance the selection zoom and delayed name-entry transition.
	if (0 < CS_GARAGE_ZOOM_IN)
	{
		// decrease zoom frame timer
		CS_GARAGE_ZOOM_IN--;
	}

	// if pressed X once, and waited for countdown clock
	if ((CS_GARAGE_SELECTED == 1) && (CS_GARAGE_ZOOM_IN == 0))
	{
		if (
		    // frames remaining for animation
		    (GARAGE_OSK_DELAY_FRAMES <= CS_GARAGE_DELAY) || ((GAME_TRACKER->gameMode2 & GARAGE_OSK) != 0))
		{
			// set desiredMenu to OSK (on-screen keyboard)
			CS_DESIRED_MENU = &data.menuSubmitName;

			{
				s16 character = CS_GARAGE_CHARACTERS[CS_GARAGE_CURRENT];
				sdata->advProgress.characterID = character;
				GAME_CHARACTER_IDS[0] = character;
			}

			SubmitName_RestoreName(0);
			OtherFX_Play(1, 1);
		}
		else
		{
			CS_GARAGE_DELAY++;
		}
	}

	if (CS_DESIRED_MENU == &data.menuSubmitName)
	{
		// NOTE(aalhendi): Flush a pending native Enter press so it cannot
		// submit the name immediately when the keyboard opens.
		NikoGetEnterKey();
	}

	if (CS_GARAGE_SELECTED == 0)
	{
		CS_GARAGE_ZOOM_IN = CS_GARAGE_ZOOM_FRAMES;
	}

	if (CS_GARAGE_ZOOM_OUT != 0)
	{
		CS_GARAGE_ZOOM_OUT--;
	}

	pushBuffer = &GAME_TRACKER->pushBuffer[0];

	// Pura->Crash
	if ((CS_GARAGE_CURRENT == 0) && (CS_GARAGE_PREVIOUS == 7))
	{
		garageFrames = GARAGE_CAMERA_WRAP_RIGHT_FRAME - CS_GARAGE_MOVE_FRAME;
	}
	// Crash->Pura
	else if ((CS_GARAGE_CURRENT == 7) && (CS_GARAGE_PREVIOUS == 0))
	{
		garageFrames = CS_GARAGE_MOVE_FRAME + GARAGE_CAMERA_WRAP_LEFT_FRAME;
	}
	// Move Right
	else if (CS_GARAGE_CURRENT > CS_GARAGE_PREVIOUS)
	{
		garageFrames = CS_GARAGE_CURRENT * GARAGE_CAMERA_FRAME_STEP - CS_GARAGE_MOVE_FRAME;
	}
	// Move Left
	else
	{
		garageFrames = CS_GARAGE_CURRENT * GARAGE_CAMERA_FRAME_STEP + CS_GARAGE_MOVE_FRAME;
	}

	// animation frame index,
	// pointer to position,
	// pointer to rotation

	CAM_Path_Move((s16)garageFrames, CTR_VECTOR_DATA(&(camPos)), CTR_VECTOR_DATA(&(camRot)), &pathFlags);

	// set position and rotation to pushBuffer
	pushBuffer->pos.x = camPos.x;
	pushBuffer->pos.y = camPos.y;
	pushBuffer->pos.z = camPos.z;

	pushBuffer->rot.x = camRot.x;
	pushBuffer->rot.y = camRot.y;
	pushBuffer->rot.z = camRot.z;

	if (CS_GARAGE_ZOOM_OUT != 0)
	{
		zoom = CS_GARAGE_FOV_MIN + CS_GARAGE_ZOOM_OUT * (CS_GARAGE_FOV_MAX - CS_GARAGE_FOV_MIN) / CS_GARAGE_ZOOM_FRAMES;
	}
	else
	{
		zoom = CS_GARAGE_FOV_MIN + (CS_GARAGE_ZOOM_FRAMES - CS_GARAGE_ZOOM_IN) * (CS_GARAGE_FOV_MAX - CS_GARAGE_FOV_MIN) / CS_GARAGE_ZOOM_FRAMES;
	}
	pushBuffer->distanceToScreen_CURR = zoom;
	pushBuffer->distanceToScreen_PREV = zoom;
}

struct RectMenu *CS_Garage_GetMenuPtr(void)
{
	return &gGarage.menuGarage;
}

void CS_Garage_Init(void)
{
	struct RectMenu *menu = &gGarage.menuGarage;

	// go to 3D character selection
	sdata->ptrActiveMenu = menu;

	menu->state &= ~ONLY_DRAW_TITLE;

	// 0 = just entered garage
	CS_Garage_ZoomOut(0);
}

const Color csGarageWhite CTR_PSX_MATCH_SECTION(".CS_garage_white") = {.r = 255, .g = 255, .b = 255};
