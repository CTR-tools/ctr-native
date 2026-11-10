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

	struct PushBuffer *pushBuffer;



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

		const int panelX = character == TINY_TIGER || character == PURA ? 128 : 384;
		UI_EngineStats_Draw(panelX, GARAGE_STAT_BOX_Y, selectedEngine, ENGINE_STATS_LARGE, !g_config.engineSelection);

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
