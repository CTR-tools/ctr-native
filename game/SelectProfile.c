#include <common.h>

// NOTE(aalhendi): Resident bindings name the same canonical native storage.
#ifndef SELECT_PROFILE_SPIN_RATE
#define SELECT_PROFILE_SPIN_RATE sdata->LoadSave_SpinRateY
#endif
#ifndef SELECT_PROFILE_LOAD_SAVE_META
#define SELECT_PROFILE_LOAD_SAVE_META data.MetaDataLoadSave
#endif
#ifndef SELECT_PROFILE_PADDED_FORMAT
#define SELECT_PROFILE_PADDED_FORMAT sdata->stringFormat1
#endif
#ifndef SELECT_PROFILE_INTEGER_FORMAT
#define SELECT_PROFILE_INTEGER_FORMAT sdata->stringFormat2
#endif
#ifndef SELECT_PROFILE_LOAD_SAVE_OBJECT
#define SELECT_PROFILE_LOAD_SAVE_OBJECT sdata->ptrLoadSaveObj
#endif
#ifndef SELECT_PROFILE_GREY_COLOR
#define SELECT_PROFILE_GREY_COLOR sdata->greyColor
#endif
#ifndef SELECT_PROFILE_GREEN_COLOR
#define SELECT_PROFILE_GREEN_COLOR sdata->greenColor
#endif
#ifndef SELECT_PROFILE_PERCENT_TEXT
#define SELECT_PROFILE_PERCENT_TEXT ((char *)&sdata->s_percent_sign)
#endif
#ifndef SELECT_PROFILE_HIGHLIGHT_NORMAL
#define SELECT_PROFILE_HIGHLIGHT_NORMAL sdata->menuRowHighlight_Normal
#endif
#ifndef SELECT_PROFILE_HIGHLIGHT_GREEN
#define SELECT_PROFILE_HIGHLIGHT_GREEN sdata->menuRowHighlight_Green
#endif
#ifndef SELECT_PROFILE_INSTANCE_NAME
#define SELECT_PROFILE_INSTANCE_NAME rdata.s_loadsave
#endif
#ifndef SELECT_PROFILE_MENU_GREEN_LOAD_SAVE
#define SELECT_PROFILE_MENU_GREEN_LOAD_SAVE data.menuGreenLoadSave
#endif
#ifndef SELECT_PROFILE_LOAD_SAVE_ICONS
#define SELECT_PROFILE_LOAD_SAVE_ICONS sdata->LoadSaveData
#endif
#ifndef SELECT_PROFILE_SPIN_OFFSET
#define SELECT_PROFILE_SPIN_OFFSET data.spinOffset_LoadSave
#endif
#ifndef SELECT_PROFILE_DESIRED_MENU
#define SELECT_PROFILE_DESIRED_MENU sdata->ptrDesiredMenu
#endif
#ifndef SELECT_PROFILE_MENU_ADVENTURE
#define SELECT_PROFILE_MENU_ADVENTURE data.menuFourAdvProfiles
#endif
#ifndef SELECT_PROFILE_RED_COLOR
#define SELECT_PROFILE_RED_COLOR sdata->redColor
#endif
#ifndef SELECT_PROFILE_GHOST_COLOR
#define SELECT_PROFILE_GHOST_COLOR sdata->ghostIconColor
#endif
#ifndef SELECT_PROFILE_MENU_GHOST
#define SELECT_PROFILE_MENU_GHOST data.menuGhostSelection
#endif
#ifndef SELECT_PROFILE_MENU_WARNING
#define SELECT_PROFILE_MENU_WARNING data.menuWarning2
#endif
#ifndef SELECT_PROFILE_MENU_OVERWRITE
#define SELECT_PROFILE_MENU_OVERWRITE data.menuOverwriteAdv
#endif
#ifndef SELECT_PROFILE_MODE
#define SELECT_PROFILE_MODE sdata->selectProfileState.mode
#endif
#ifndef SELECT_PROFILE_ACTION_ACTIVE
#define SELECT_PROFILE_ACTION_ACTIVE sdata->selectProfileState.actionActive
#endif
#ifndef SELECT_PROFILE_EXIT_TO_PREVIOUS
#define SELECT_PROFILE_EXIT_TO_PREVIOUS sdata->selectProfileState.exitToPrevious
#endif
#ifndef SELECT_PROFILE_ACTION_DONE
#define SELECT_PROFILE_ACTION_DONE sdata->selectProfileState.actionDone
#endif
#ifndef SELECT_PROFILE_OVERWRITE_PROMPT
#define SELECT_PROFILE_OVERWRITE_PROMPT sdata->selectProfileState.overwritePrompt
#endif
#ifndef SELECT_PROFILE_TIMEOUT_PROMPT
#define SELECT_PROFILE_TIMEOUT_PROMPT sdata->selectProfileState.timeoutPrompt
#endif
#ifndef SELECT_PROFILE_TIMER_SAVE_COMPLETE
#define SELECT_PROFILE_TIMER_SAVE_COMPLETE sdata->selectProfileState.timerSaveComplete
#endif
#ifndef SELECT_PROFILE_ACTION
#define SELECT_PROFILE_ACTION sdata->memcardAction
#endif
#ifndef SELECT_PROFILE_SCREEN_TEXT
#define SELECT_PROFILE_SCREEN_TEXT sdata->mcScreenText
#endif
#ifndef SELECT_PROFILE_BUTTON_TAP
#define SELECT_PROFILE_BUTTON_TAP sdata->buttonTapPerPlayer[0]
#endif
#ifndef SELECT_PROFILE_ROW_BACKUP
#define SELECT_PROFILE_ROW_BACKUP sdata->unk_8008d73C_relatedToRowHighlighted
#endif

void SelectProfile_QueueLoadHub_MenuProc(struct RectMenu *menu)
{
	struct GameTracker *gGT = GAME_TRACKER;

	// NOTE(aalhendi): Retail stores 0x27 before LOAD_LevelFile records prevLEV.
	gGT->levelID = MAIN_MENU_LEVEL;

	GAME_CHARACTER_IDS[0] = GAME_ADV_PROGRESS.characterID;
	MainRaceTrack_RequestLoad(gGT->currLEV);
	RECTMENU_Hide(menu);
	return;
}


void SelectProfile_ThTick(struct Thread *t)
{
	struct SelectProfileLoadSaveObj *obj;
	struct SelectProfileLoadSaveIcon *icon;
	u16 i;

	obj = (struct SelectProfileLoadSaveObj *)t->object;
	icon = obj->icons;

	for (i = 0; i < 12; i++, icon++)
	{
		s16 index = (s16)i;
		s16 slot = index % 3;

		icon->rot.y = (s16)(icon->rot.y + SELECT_PROFILE_SPIN_RATE[slot]);

#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Menu-storage can keep this thread alive when the
		// load/save icon models were not published for the current level.
		if (icon->inst == NULL)
		{
			continue;
		}
#endif

		ConvertRotToMatrix(&icon->inst->matrix, &icon->rot);

		if (slot != 1)
		{
			SVec3 lightDir;

			lightDir.x = SELECT_PROFILE_LOAD_SAVE_META[index].vec3_specular_inverted.x;
			lightDir.y = SELECT_PROFILE_LOAD_SAVE_META[index].vec3_specular_inverted.y;
			lightDir.z = SELECT_PROFILE_LOAD_SAVE_META[index].vec3_specular_inverted.z;
			Vector_SpecLightSpin3D(icon->inst, &icon->rot, &lightDir);
		}
	}
}


// NOTE(aalhendi): Retail compares the signed halfword formatting flag with one.
void SelectProfile_PrintInteger(int value, int posX, int posY, b16 usePaddedFormat, int color)
{
	char text[64];
	u16 drawColor = color;

	if (usePaddedFormat == 1)
	{
		sprintf(text, SELECT_PROFILE_PADDED_FORMAT, value);
	}
	else
	{
		sprintf(text, SELECT_PROFILE_INTEGER_FORMAT, value);
	}

	DecalFont_DrawLine(text, posX, posY, FONT_BIG, (s16)drawColor);
}


int SelectProfile_UI_ConvertX(int screenX, int scale)
{
	int projectedX = (screenX - SELECT_PROFILE_UI_CENTER_X) * scale;
	if (projectedX < 0)
	{
		projectedX += 0xff;
	}
	return projectedX >> 8;
}


int SelectProfile_UI_ConvertY(int screenY, int scale)
{
	int projectedY = (screenY - SELECT_PROFILE_UI_CENTER_Y) * scale;
	if (projectedY < 0)
	{
		projectedY += 0xff;
	}
	return projectedY >> 8;
}


static __inline__ void SelectProfile_DrawAdvProfile_UpdateIcon(s16 index, int posX, int posY)
{
#if defined(CTR_NATIVE)
	// NOTE(aalhendi): The native menu thread may outlive unavailable icon models.
	if (((struct SelectProfileLoadSaveObj *)SELECT_PROFILE_LOAD_SAVE_OBJECT)->icons[index].inst == NULL)
	{
		return;
	}
#endif
	((struct SelectProfileLoadSaveObj *)SELECT_PROFILE_LOAD_SAVE_OBJECT)->icons[index].inst->matrix.t[0] =
	    SelectProfile_UI_ConvertX(posX, SELECT_PROFILE_UI_SCALE);
	((struct SelectProfileLoadSaveObj *)SELECT_PROFILE_LOAD_SAVE_OBJECT)->icons[index].inst->matrix.t[1] =
	    SelectProfile_UI_ConvertY(posY, SELECT_PROFILE_UI_SCALE);
	((struct SelectProfileLoadSaveObj *)SELECT_PROFILE_LOAD_SAVE_OBJECT)->icons[index].inst->matrix.t[2] = SELECT_PROFILE_UI_DEPTH;
	((struct SelectProfileLoadSaveObj *)SELECT_PROFILE_LOAD_SAVE_OBJECT)->icons[index].inst->flags &= ~HIDE_MODEL;
}

void SelectProfile_DrawAdvProfile(struct AdvProgress *adv, int posX, int posY, s16 isHighlighted, s16 slotIndex, s16 menuFlag)
{
	s16 x = posX;
	s16 y = posY;
	Color iconColor;
	RECT profileRect;
	int numberColor;
	int emptyColor;
	int nameColor;
	int percentColor;

	if ((menuFlag & SELECT_PROFILE_DRAW_STYLE_GREEN) != 0)
	{
		numberColor = LIGHT_GREEN;
		emptyColor = FOREST_GREEN;
		nameColor = LIGHT_GREEN;
		percentColor = LIGHT_GREEN;
	}
	else
	{
		numberColor = ORANGE;
		emptyColor = RED;
		nameColor = PERIWINKLE;
		percentColor = WHITE;
	}

	slotIndex *= 3;
	GAMEPROG_AdvPercent(adv);

	if (adv->characterID >= 0)
	{
		int profileTextColor;
		if (menuFlag & SELECT_PROFILE_DRAW_STYLE_GREEN)
			iconColor = ColorCode_Load(&SELECT_PROFILE_GREEN_COLOR);
		else
			iconColor = ColorCode_Load(&SELECT_PROFILE_GREY_COLOR);

		RECTMENU_DrawPolyGT4(GAME_TRACKER->ptrIcons[GAME_CHARACTER_METADATA[adv->characterID].iconID], x + 10, y + 6, &GAME_TRACKER->backBuffer->primMem,
		                     GAME_TRACKER->pushBuffer_UI.ptrOT, iconColor, iconColor, iconColor, iconColor, 1, 0x1000);
		DecalFont_DrawLine(adv->name, x + 0x6c, y + 0x29, FONT_BIG, JUSTIFY_CENTER | nameColor);
		profileTextColor = numberColor | JUSTIFY_RIGHT;
		SelectProfile_PrintInteger(GAME_TRACKER->currAdvProfile.completionPercent, (s16)(x + 0x6a), (s16)(y + 0x17), 0, profileTextColor);
		SelectProfile_PrintInteger(GAME_TRACKER->currAdvProfile.numTrophies, (s16)(x + 0x6a), (s16)(y + 5), 0, profileTextColor);
		SelectProfile_PrintInteger(GAME_TRACKER->currAdvProfile.numKeys, (s16)(x + 0xb5), (s16)(y + 5), 0, profileTextColor);
		SelectProfile_PrintInteger(GAME_TRACKER->currAdvProfile.numRelics, (s16)(x + 0xb5), (s16)(y + 0x17), 0, profileTextColor);
		DecalFont_DrawLine(SELECT_PROFILE_PERCENT_TEXT, x + 0x70, y + 0x17, FONT_BIG, percentColor);

		SelectProfile_DrawAdvProfile_UpdateIcon(slotIndex, x + 0xc3, y + 0x1f);
		slotIndex++;
		SelectProfile_DrawAdvProfile_UpdateIcon(slotIndex, x + 0x78, y + 0xd);
		slotIndex++;
		SelectProfile_DrawAdvProfile_UpdateIcon(slotIndex, x + 0xc3, y + 0xd);
	}
	else
	{
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_EMPTY], x + 0x6c, y + 0x17, FONT_BIG, JUSTIFY_CENTER | emptyColor);
	}

	profileRect.x = x;
	profileRect.y = y;
	profileRect.w = 0xdc;
	profileRect.h = 0x3d;

	if (isHighlighted != 0)
	{
		RECT highlightRect;
		Color *highlightColor = &SELECT_PROFILE_HIGHLIGHT_NORMAL;
		if (menuFlag & SELECT_PROFILE_DRAW_STYLE_GREEN)
			highlightColor = &SELECT_PROFILE_HIGHLIGHT_GREEN;

		highlightRect.x = x + 6;
		highlightRect.y = y + 4;
		highlightRect.w = 0xd0;
		highlightRect.h = 0x35;
		CTR_Box_DrawClearBox(&highlightRect, highlightColor, 1, &GAME_TRACKER->backBuffer->otMem.uiOT[3], &GAME_TRACKER->backBuffer->primMem);
	}
	RECTMENU_DrawInnerRect(&profileRect, (s16)menuFlag, &GAME_TRACKER->backBuffer->otMem.uiOT[3]);
}


void SelectProfile_GetTrackID()
{
	SELECT_PROFILE_MENU_GREEN_LOAD_SAVE.rowSelected = 1;
	GAME_ADV_PROGRESS.HubLevYouSavedOn = GAME_TRACKER->levelID;
}


#ifdef CTR_NATIVE
// NOTE(aalhendi): Retail stores these debug names in EXE RDATA; native does not
// expose the full retail RDATA struct.
static char s_SelectProfileThreadName[] = "LoadSave";
static char s_SelectProfileInstName[] = "loadsave";
#endif

// NOTE(aalhendi): Retail callers pass the draw style as a signed halfword.
void SelectProfile_Init(s16 flags)
{
	struct SelectProfileLoadSaveObj *setupObj;
	struct SelectProfileLoadSaveIcon *icon;
	__typeof__(SELECT_PROFILE_LOAD_SAVE_META[0]) *meta;
	__typeof__(SELECT_PROFILE_LOAD_SAVE_META[0]) *metaBase;
	s16 *spinOffset;
	s32 firstPlayer;
	struct Thread *t;
	u16 i;

	if (SELECT_PROFILE_LOAD_SAVE_OBJECT == 0)
	{
		struct SelectProfileLoadSaveObj *obj;
#ifdef CTR_NATIVE
		char *threadName = &s_SelectProfileThreadName[0];
#else
		char *threadName = rdata.s_LoadSave;
#endif

		t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(sizeof(struct SelectProfileLoadSaveObj), NONE, SMALL, OTHER), SelectProfile_ThTick, threadName,
		                         NULL);
		// NOTE(aalhendi): Native low-RAM audit candidate only. Retail writes
		// through this allocation result before its later null check; keep
		// unpatched until a valid menu repro proves the allocation can fail.
		obj = (struct SelectProfileLoadSaveObj *)t->object;
		SELECT_PROFILE_LOAD_SAVE_OBJECT = (int)obj;
		obj->icons = &SELECT_PROFILE_LOAD_SAVE_ICONS[0];
		memset(obj->icons, 0, sizeof(SELECT_PROFILE_LOAD_SAVE_ICONS));

		obj = (struct SelectProfileLoadSaveObj *)SELECT_PROFILE_LOAD_SAVE_OBJECT;
		obj->thread = t;
		if (obj == NULL)
		{
			return;
		}
	}

	// NOTE(aalhendi): Keep the fixed table bases distinct from the advancing
	// metadata pointer and retain the retail player-loop setup lifetimes.
	meta = SELECT_PROFILE_LOAD_SAVE_META;
	i = 0;
	firstPlayer = 1;
	CTR_PSX_OBSERVE_VALUE(firstPlayer);
	metaBase = meta;
	CTR_PSX_OBSERVE_VALUE(metaBase);
	setupObj = (struct SelectProfileLoadSaveObj *)SELECT_PROFILE_LOAD_SAVE_OBJECT;
	spinOffset = SELECT_PROFILE_SPIN_OFFSET;
	icon = setupObj->icons;
	for (; i < 12; i++, icon++, meta++)
	{
		struct Instance *inst;
		s16 slot;
		CTR_PSX_OBSERVE_VALUE(spinOffset);


		if (icon->inst == NULL)
		{
			struct Model *model = GAME_TRACKER->modelPtr[meta->modelID];

			if (model != NULL)
			{
#ifdef CTR_NATIVE
				char *instName = &s_SelectProfileInstName[0];
#else
				char *instName = SELECT_PROFILE_INSTANCE_NAME;
#endif

				inst = INSTANCE_Birth3D(model, instName, ((struct SelectProfileLoadSaveObj *)SELECT_PROFILE_LOAD_SAVE_OBJECT)->thread);
				icon->inst = inst;

				if (inst != NULL)
				{
					struct InstDrawPerPlayer *idpp;
					u16 player;

					slot = (s16)i % 3;

					inst->flags |= HIDE_MODEL | SCREENSPACE_INSTANCE;
					if (slot != firstPlayer)
					{
						inst->flags |= USE_SPECULAR_LIGHT;
					}

					idpp = INST_GETIDPP(inst);
					idpp[0].pushBuffer = &GAME_TRACKER->pushBuffer_UI;
					player = 1;
					if (firstPlayer < GAME_TRACKER->numPlyrCurrGame)
					{
						do
						{
							struct Instance *playerInst;
							CTR_PSX_ADD_POINTER_OFFSET(playerInst, inst, (s16)player * sizeof(struct InstDrawPerPlayer));
							INST_GETIDPP(playerInst)->pushBuffer = NULL;
							player++;
						} while ((s16)player < GAME_TRACKER->numPlyrCurrGame);
					}

					if ((flags & SELECT_PROFILE_DRAW_STYLE_GREEN) != 0)
					{
						inst->colorRGBA = (((u8)meta->r >> 1) << 0x14) | ((u8)meta->g << 0xc) | (((u8)meta->b >> 1) << 4);
					}
					else
					{
						inst->colorRGBA = ((u8)meta->r << 0x14) | ((u8)meta->g << 0xc) | ((u8)meta->b << 4);
					}
					inst->scale.x = inst->scale.y = inst->scale.z = metaBase[(s16)i].scale;

					icon->rot.x = icon->rot.y = 0;
					icon->rot.z = spinOffset[(s16)((s16)i % 3)];

					{
						// NOTE(aalhendi): Keep the identity value inside the icon loop.
						u32 unit = 0x1000;
						CTR_PSX_KEEP_VALUE(unit);
						CTR_WriteU32AlignedLE(&inst->matrix.m[0][0], unit);
						CTR_WriteU32AlignedLE(&inst->matrix.m[0][2], 0);
						CTR_WriteU32AlignedLE(&inst->matrix.m[1][1], unit);
						CTR_WriteU32AlignedLE(&inst->matrix.m[2][0], 0);
						inst->matrix.m[2][2] = unit;
					}
				}
			}
		}

		if (icon->inst != NULL)
		{
			icon->inst->flags |= HIDE_MODEL;
		}
	}
}


void SelectProfile_Destroy(void)
{
	if (SELECT_PROFILE_LOAD_SAVE_OBJECT != 0)
	{
		struct SelectProfileLoadSaveIcon *icon = ((struct SelectProfileLoadSaveObj *)SELECT_PROFILE_LOAD_SAVE_OBJECT)->icons;
		struct SelectProfileLoadSaveObj *obj;
		u16 i;

		for (i = 0; i < 12; i++, icon++)
		{
			if (icon->inst != NULL)
			{
				INSTANCE_Death(icon->inst);
#if defined(CTR_NATIVE)
				icon->inst = NULL;
#endif
			}
		}

		obj = (struct SelectProfileLoadSaveObj *)SELECT_PROFILE_LOAD_SAVE_OBJECT;
#if defined(CTR_NATIVE)
		obj->thread->funcThTick = NULL;
#endif
		obj->thread->flags |= THREAD_FLAG_DEAD;
		SELECT_PROFILE_LOAD_SAVE_OBJECT = 0;
	}
}


void SelectProfile_AdvPickMode_MenuProc(struct RectMenu *menu)
{
	struct RectMenu *savedMenu = menu;
	s16 state = savedMenu->funcState;
	if (state == RECTMENU_FUNC_STATE_INPUT)
	{
		switch (menu->rowSelected)
		{
		case 0:
		case 1:
		case 2:
			SelectProfile_ToggleMode((s16)((u16)menu->rowSelected | SELECT_PROFILE_SCREEN_GREEN_LOAD_SAVE));
			SELECT_PROFILE_DESIRED_MENU = &SELECT_PROFILE_MENU_ADVENTURE;
			break;
		case -1:
		case 3:
			RECTMENU_Hide(savedMenu);
			SelectProfile_Destroy();
			break;
		}
	}
	else
	{
		SelectProfile_Init((s16)savedMenu->drawStyle);
		SelectProfile_DrawAdvProfile(&GAME_ADV_PROGRESS, 0x92, 0x32, 0, 0, SELECT_PROFILE_DRAW_STYLE_GREEN);
	}
}


void SelectProfile_DrawGhostProfile(struct GhostProfile *profile, int posX, int posY, u32 isHighlighted, int unused, s16 menuFlag, s16 isLoading,
                                    s16 isUnavailable)
{
	// NOTE(aalhendi): Retail consumes signed halfword UI coordinates.
	s16 x = (s16)posX;
	s16 y = (s16)posY;
	s16 highlighted = (s16)isHighlighted;
	RECT profileRect;
	RECT innerRect;

	(void)unused;

	profileRect.x = x;
	profileRect.y = y;
	profileRect.w = 0xc8;
	profileRect.h = 0x29;

	innerRect.x = x + 6;
	innerRect.y = y + 3;
	innerRect.w = 0xbc;
	innerRect.h = 0x23;

	if (isUnavailable != 0)
	{
#ifdef CTR_NATIVE
		// NOTE(aalhendi): Native needs a real Color object for packed palette bytes.
		Color redColorStorage;
		const Color *redColor = &redColorStorage;
#else
		const Color *redColor = (const Color *)&SELECT_PROFILE_RED_COLOR;
#endif

		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_NOT_AVAILABLE], x + 0x64, y + 0x11, FONT_SMALL, JUSTIFY_CENTER | SILVER);
#ifdef CTR_NATIVE
		redColorStorage = ColorCode_Load(&SELECT_PROFILE_RED_COLOR);
#endif
		CTR_Box_DrawClearBox(&innerRect, redColor, ADD_DECAL, GAME_TRACKER->backBuffer->otMem.uiOT, &GAME_TRACKER->backBuffer->primMem);
	}

	if (profile != NULL)
	{
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[GAME_LEVEL_METADATA[profile->trackID].name_LNG], x + 0x64, y + 0x1e, FONT_SMALL, JUSTIFY_CENTER | LIGHT_GREEN);
		DecalFont_DrawLine(RECTMENU_DrawTime(profile->trackTime), x + 0x78, y + 10, FONT_BIG, JUSTIFY_CENTER | PERIWINKLE);
		RECTMENU_DrawPolyGT4(GAME_TRACKER->ptrIcons[GAME_CHARACTER_METADATA[profile->characterID].iconID], x + 8, y + 5, &GAME_TRACKER->backBuffer->primMem,
		                     GAME_TRACKER->pushBuffer_UI.ptrOT, ColorCode_Load(&SELECT_PROFILE_GHOST_COLOR), ColorCode_Load(&SELECT_PROFILE_GHOST_COLOR),
		                     ColorCode_Load(&SELECT_PROFILE_GHOST_COLOR), ColorCode_Load(&SELECT_PROFILE_GHOST_COLOR), TRANS_50_DECAL, 0x1000);
	}
	else
	{
		DecalFont_DrawLine((isLoading != 0) ? GAME_LANGUAGE_STRINGS[0x6c] : GAME_LANGUAGE_STRINGS[0xb5], x + 0x64, y + 0x11, FONT_SMALL,
		                   (isLoading != 0) ? JUSTIFY_CENTER | PERIWINKLE : JUSTIFY_CENTER | RED);
	}

	if (highlighted != 0)
	{
		Color *highlight = &SELECT_PROFILE_HIGHLIGHT_NORMAL;
		if ((menuFlag & SELECT_PROFILE_DRAW_STYLE_GREEN) != 0)
			highlight = &SELECT_PROFILE_HIGHLIGHT_GREEN;
		CTR_Box_DrawClearBox(&innerRect, highlight, TRANS_50_DECAL, GAME_TRACKER->backBuffer->otMem.uiOT, &GAME_TRACKER->backBuffer->primMem);
	}

	RECTMENU_DrawInnerRect(&profileRect, (s16)menuFlag, GAME_TRACKER->backBuffer->otMem.uiOT);
}


void SelectProfile_MuteCursors(void)
{
	SELECT_PROFILE_MENU_ADVENTURE.state |= MUTE_SOUND_OF_MOVING_CURSOR;
	SELECT_PROFILE_MENU_GHOST.state |= MUTE_SOUND_OF_MOVING_CURSOR;
	SELECT_PROFILE_MENU_WARNING.state |= MUTE_SOUND_OF_MOVING_CURSOR;
}

void SelectProfile_UnMuteCursors(void)
{
	SELECT_PROFILE_MENU_ADVENTURE.state &= ~MUTE_SOUND_OF_MOVING_CURSOR;
	SELECT_PROFILE_MENU_GHOST.state &= ~MUTE_SOUND_OF_MOVING_CURSOR;
	SELECT_PROFILE_MENU_WARNING.state &= ~MUTE_SOUND_OF_MOVING_CURSOR;
}


static __inline__ s16 *SelectProfile_Mode(void)
{
	return &SELECT_PROFILE_MODE;
}

static __inline__ s16 *SelectProfile_TimerSaveComplete(void)
{
	return &SELECT_PROFILE_TIMER_SAVE_COMPLETE;
}

void SelectProfile_ToggleMode(u32 mode)
{
	struct RectMenu *advMenu;
	register s32 styleMask CTR_PSX_REGISTER("$4");

	SELECT_PROFILE_ACTION = mode & SELECT_PROFILE_ACTION_MASK;

	// High nibble selects the profile screen, low nibble selects the action.
	*SelectProfile_Mode() = mode & SELECT_PROFILE_SCREEN_MASK;
	SELECT_PROFILE_EXIT_TO_PREVIOUS = 0;
	SELECT_PROFILE_ACTION_DONE = 0;
	SELECT_PROFILE_OVERWRITE_PROMPT = 0;
	SELECT_PROFILE_TIMEOUT_PROMPT = 0;
	*SelectProfile_TimerSaveComplete() = 0;

	SelectProfile_UnMuteCursors();

	// NOTE(aalhendi): Keep the menu base before the shared full-width mask.
	advMenu = &SELECT_PROFILE_MENU_ADVENTURE;
	CTR_PSX_OBSERVE_VALUE(advMenu);
	styleMask = ~SELECT_PROFILE_DRAW_STYLE_GREEN;
	CTR_PSX_KEEP_VALUE_RELAXED(styleMask);
	advMenu->drawStyle &= styleMask;
	SELECT_PROFILE_MENU_OVERWRITE.drawStyle &= styleMask;
	if (*SelectProfile_Mode() == SELECT_PROFILE_SCREEN_GREEN_LOAD_SAVE)
	{
		advMenu->drawStyle |= SELECT_PROFILE_DRAW_STYLE_GREEN;
		SELECT_PROFILE_MENU_OVERWRITE.drawStyle |= SELECT_PROFILE_DRAW_STYLE_GREEN;
	}

	SelectProfile_Init((s16)advMenu->drawStyle);

	advMenu->rowSelected = SELECT_PROFILE_ROW_BACKUP;
	SELECT_PROFILE_ACTION_ACTIVE = 0;
}


u32 SelectProfile_InputLogic(struct RectMenu *menu, s16 numRows, u32 confirmFlags)
{
	u32 handled = 0;

	// D-Pad, Cross, Square, Triangle, Circle
	if ((SELECT_PROFILE_BUTTON_TAP & SELECT_PROFILE_INPUT_NAV_CONFIRM) == 0)
	{
		return 0;
	}

	if ((confirmFlags & 1) == 0)
	{
		u16 oldRow = menu->rowSelected;

		if ((SELECT_PROFILE_BUTTON_TAP & BTN_UP) != 0)
		{
			menu->rowSelected = oldRow - 2;
		}
		else
		{
			if ((SELECT_PROFILE_BUTTON_TAP & BTN_DOWN) != 0)
			{
				menu->rowSelected = oldRow + 2;
			}
			else
			{
				if ((SELECT_PROFILE_BUTTON_TAP & (BTN_LEFT | BTN_RIGHT)) != 0)
				{
					menu->rowSelected = oldRow ^ 1;
				}
			}
		}

		if (menu->rowSelected < 0)
		{
			menu->rowSelected = 0;
		}

		if (numRows <= menu->rowSelected)
		{
			menu->rowSelected = numRows - 1;
		}

		if (menu->rowSelected != (s16)oldRow)
		{
			OtherFX_Play(0, 1);
		}

		if (((SELECT_PROFILE_BUTTON_TAP & (BTN_CROSS_one | BTN_CIRCLE)) != 0) && ((numRows != 0) || (SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_SAVE)))
		{
			OtherFX_Play(1, 1);

			if (SELECT_PROFILE_SCREEN_TEXT == MC_SCREEN_WARNING_UNFORMATTED)
			{
				menu->rowSelected = 0;
			}
			handled = 1;
		}
		else
		{
			if ((SELECT_PROFILE_BUTTON_TAP & (BTN_TRIANGLE | BTN_SQUARE_one)) != 0)
			{
				OtherFX_Play(2, 1);
				handled = 1;
				menu->rowSelected = -1;
			}
		}
	}
	else
	{
		if ((SELECT_PROFILE_BUTTON_TAP & (BTN_TRIANGLE | BTN_SQUARE_one)) != 0)
		{
			OtherFX_Play(2, 1);
			handled = 1;
			menu->rowSelected = -1;
		}

		if (((confirmFlags & 2) != 0) && ((SELECT_PROFILE_BUTTON_TAP & (BTN_CROSS_one | BTN_CIRCLE)) != 0))
		{
			OtherFX_Play(1, 1);
			handled = 1;
		}
	}

	RECTMENU_ClearInput();
	return handled;
}


extern struct RectMenu menu224;
extern struct RectMenu menu224NoSave;

#ifndef SELECT_PROFILE_MEMCARD_BUFFER
#define SELECT_PROFILE_MEMCARD_BUFFER sdata->ptrToMemcardBuffer2
#endif
#ifndef SELECT_PROFILE_MESSAGE_RECT_BYTES
#define SELECT_PROFILE_MESSAGE_RECT_BYTES sdata->unk_BeforeTokenMenu
#endif
#ifndef SELECT_PROFILE_GHOST_TAPE
#define SELECT_PROFILE_GHOST_TAPE sdata->ptrGhostTapePlaying
#endif
#ifndef SELECT_PROFILE_FREE_BYTES
#define SELECT_PROFILE_FREE_BYTES sdata->memoryCard_SizeRemaining
#endif
#ifndef SELECT_PROFILE_SAVE_CUP
#define SELECT_PROFILE_SAVE_CUP sdata->boolSaveCupProgress
#endif
#ifndef SELECT_PROFILE_UNK_8008D928
#define SELECT_PROFILE_UNK_8008D928 sdata->unk_memcardRelated_8008d928
#endif
#ifndef SELECT_PROFILE_DATA_OUTDATED
#define SELECT_PROFILE_DATA_OUTDATED sdata->boolMemcardDataOutdated
#endif
#ifndef SELECT_PROFILE_UNK_8008D95C
#define SELECT_PROFILE_UNK_8008D95C sdata->unk8008d95c
#endif
#ifndef SELECT_PROFILE_UNK_8008D964
#define SELECT_PROFILE_UNK_8008D964 sdata->unk8008d964
#endif
#ifndef SELECT_PROFILE_ADV_INDEX
#define SELECT_PROFILE_ADV_INDEX sdata->advProfileIndex
#endif
#ifndef SELECT_PROFILE_FRAME_COUNTER
#define SELECT_PROFILE_FRAME_COUNTER sdata->frameCounter
#endif
#ifndef SELECT_PROFILE_ERROR_FLAG
#define SELECT_PROFILE_ERROR_FLAG sdata->boolError
#endif
#ifndef SELECT_PROFILE_FONT_HEIGHT
#define SELECT_PROFILE_FONT_HEIGHT data.font_charPixHeight
#endif
#ifndef SELECT_PROFILE_MESSAGE_SCREENS
#define SELECT_PROFILE_MESSAGE_SCREENS data.messageScreens
#endif
#ifndef SELECT_PROFILE_GHOST_TEXT
#define SELECT_PROFILE_GHOST_TEXT data.lngIndex_LoadSave
#endif
#ifndef SELECT_PROFILE_ADV_TEXT
#define SELECT_PROFILE_ADV_TEXT data.lngStringsSaveLoadDelete
#endif
#ifndef SELECT_PROFILE_MENU_SUBMIT_NAME
#define SELECT_PROFILE_MENU_SUBMIT_NAME data.menuSubmitName
#endif
#ifndef SELECT_PROFILE_MENU_QUEUE_LOAD_HUB
#define SELECT_PROFILE_MENU_QUEUE_LOAD_HUB data.menuQueueLoadHub
#endif
#ifndef SELECT_PROFILE_MENU_OVERWRITE_GHOST
#define SELECT_PROFILE_MENU_OVERWRITE_GHOST data.menuOverwriteGhost
#endif
#ifndef SELECT_PROFILE_MENU_SAVE_GAME
#define SELECT_PROFILE_MENU_SAVE_GAME data.menuSaveGame
#endif

static __inline__ s16 *SelectProfile_AllProfiles_Mode(void)
{
	return &SELECT_PROFILE_MODE;
}

static __inline__ s16 *SelectProfile_AllProfiles_ActionActive(void)
{
	return &SELECT_PROFILE_ACTION_ACTIVE;
}

static __inline__ s16 *SelectProfile_AllProfiles_ExitToPrevious(void)
{
	return &SELECT_PROFILE_EXIT_TO_PREVIOUS;
}

static __inline__ s16 *SelectProfile_AllProfiles_ActionDone(void)
{
	return &SELECT_PROFILE_ACTION_DONE;
}

static __inline__ s16 *SelectProfile_AllProfiles_OverwritePrompt(void)
{
	return &SELECT_PROFILE_OVERWRITE_PROMPT;
}

static __inline__ s16 *SelectProfile_AllProfiles_TimeoutPrompt(void)
{
	return &SELECT_PROFILE_TIMEOUT_PROMPT;
}

static __inline__ s16 *SelectProfile_AllProfiles_TimerSaveComplete(void)
{
	return &SELECT_PROFILE_TIMER_SAVE_COMPLETE;
}

static __inline__ struct MemcardProfile *SelectProfile_MemcardProfile(void)
{
	return (struct MemcardProfile *)SELECT_PROFILE_MEMCARD_BUFFER;
}


static __inline__ int SelectProfile_AdvProfileOccupied(int slot)
{
	return SelectProfile_MemcardProfile()->advProgress[slot].characterID >= 0;
}

static __inline__ void SelectProfile_CopyGameProgressToCard(void)
{
	RaceConfig_SaveGameOptions();
	GAMEPROG_SaveCupProgress();
	GAMEPROG_SyncGameAndCard(&SelectProfile_MemcardProfile()->gameSave.progress, &GAME_SAVE.progress);
	SelectProfile_MemcardProfile()->gameSave = GAME_SAVE;
}

static __inline__ void SelectProfile_LoadAdvProfile(struct RectMenu *menu)
{
	GAMEPROG_SyncGameAndCard(&SelectProfile_MemcardProfile()->gameSave.progress, &GAME_SAVE.progress);
	memcpy(&GAME_ADV_PROGRESS, &SelectProfile_MemcardProfile()->advProgress[menu->rowSelected], sizeof(GAME_ADV_PROGRESS));
	GAME_CHARACTER_IDS[0] = GAME_ADV_PROGRESS.characterID;
	memcpy(GAME_TRACKER->prevNameEntered, GAME_ADV_PROGRESS.name, sizeof(GAME_TRACKER->prevNameEntered));
}

static __inline__ void SelectProfile_SaveAdvProfile(struct RectMenu *menu)
{
	register u16 selectedRow CTR_PSX_REGISTER("$2") = menu->rowSelected;
	CTR_PSX_KEEP_VALUE(selectedRow);
	CTR_PSX_STORE_HALF(SELECT_PROFILE_ROW_BACKUP, selectedRow);
	RaceConfig_SaveGameOptions();
	GAMEPROG_SaveCupProgress();
	GAMEPROG_SyncGameAndCard(&SelectProfile_MemcardProfile()->gameSave.progress, &GAME_SAVE.progress);
	memcpy(&SelectProfile_MemcardProfile()->advProgress[menu->rowSelected], &GAME_ADV_PROGRESS, sizeof(GAME_ADV_PROGRESS));
	memcpy(&SelectProfile_MemcardProfile()->gameSave, &GAME_SAVE, sizeof(GAME_SAVE));
	MEMCARD_SetIcon(0);
	RefreshCard_StartMemcardAction(3);
	SELECT_PROFILE_UNK_8008D928 = 1;
	*SelectProfile_AllProfiles_ActionActive() = 1;
}

static __inline__ void SelectProfile_StartGhostSave(struct RectMenu *menu)
{
	struct GameTracker *gGT = GAME_TRACKER;
	struct Driver *driver = gGT->drivers[0];
	int time = SELECT_PROFILE_DEFAULT_GHOST_TIME;

	if (driver != NULL)
	{
		time = driver->timeElapsedInRace;
	}

	RefreshCard_GhostEncodeProfile(menu->rowSelected, GAME_CHARACTER_IDS[0], gGT->levelID, time, gGT->prevNameEntered);

	GAME_MEMCARD_STATE.ghostProfile_indexSave = menu->rowSelected;
	GAME_MEMCARD_STATE.ghostProfile_rowSelect = (menu->rowSelected < GAME_MEMCARD_STATE.numGhostProfilesSaved) ? menu->rowSelected : -1;

	MEMCARD_SetIcon(1);
	RefreshCard_StartMemcardAction(6);
	*SelectProfile_AllProfiles_ActionActive() = 1;
	GAME_TRACKER->gameModeEnd |= PLAYER_GHOST_BEAT;
}

#if defined(CTR_NATIVE)
// NOTE(aalhendi): Native bounds signed unpublished counts before indexing Ghost slots.
static __inline__ u16 SelectProfile_ClampNativeGhostCount(u16 count)
{
	if ((s16)count < 0)
		count = 0;
	if (count > SELECT_PROFILE_GHOST_SLOT_COUNT)
		count = SELECT_PROFILE_GHOST_SLOT_COUNT;
	return count;
}
#endif

static __inline__ u16 SelectProfile_GhostRowCount(u16 *savedCount, b16 *canChooseEmptySlot)
{
	u16 count = GAME_MEMCARD_STATE.numGhostProfilesSaved;
#if defined(CTR_NATIVE)
	count = SelectProfile_ClampNativeGhostCount(count);
#endif
	*savedCount = count;
	if (SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_SAVE)
	{
		*canChooseEmptySlot = SELECT_PROFILE_FREE_BYTES >= SELECT_PROFILE_GHOST_SAVE_BYTES;
		count += *canChooseEmptySlot;
		if ((s16)count > SELECT_PROFILE_GHOST_SLOT_COUNT)
		{
			count = SELECT_PROFILE_GHOST_SLOT_COUNT;
			*canChooseEmptySlot = false;
		}
	}
	else
	{
		*canChooseEmptySlot = true;
		count++;
	}
	return count;
}

static __inline__ void SelectProfile_ClampRow(struct RectMenu *menu, s16 rowCount)
{
	if (menu->rowSelected < 0)
	{
		menu->rowSelected = 0;
	}

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Empty native menus keep row zero instead of selecting -1.
	if ((rowCount > 0) && (menu->rowSelected >= rowCount))
#else
	if (menu->rowSelected >= rowCount)
#endif
	{
		menu->rowSelected = rowCount - 1;
	}
}

static __inline__ s16 SelectProfile_DisableAdvInputFlags(void)
{
	u16 flags = 0;

	if (((u16)SELECT_PROFILE_SCREEN_TEXT < MC_SCREEN_FORMATTING) ||
	    (((u32)SELECT_PROFILE_FREE_BYTES < SELECT_PROFILE_ADV_SAVE_BYTES) && (SELECT_PROFILE_UNK_8008D928 == 0) &&
	     (SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_SAVE)))
	{
		flags = 1;
	}

	if ((u16)SELECT_PROFILE_SCREEN_TEXT < MC_SCREEN_FORMATTING)
	{
		flags |= 2;
	}

	return flags;
}

static __inline__ void SelectProfile_DrawGhostRows(struct RectMenu *menu, u32 rowCount, u16 savedCount, b16 canChooseEmptySlot, u16 color)
{
	register u32 index CTR_PSX_REGISTER("$18") = 0;
	s16 subtitleVisible;
	register s16 lineGap CTR_PSX_REGISTER("$21");
	register s16 yBase CTR_PSX_REGISTER("$19");
	register s16 subtitleY CTR_PSX_REGISTER("$16");
	register s16 titleEndY CTR_PSX_REGISTER("$19");
	register s32 titleFont CTR_PSX_REGISTER("$22");
	register s32 titleColor CTR_PSX_REGISTER("$17");
	register const s16 *text CTR_PSX_REGISTER("$23");
	s16 font;
	u16 rowsWithEmpty = (u16)rowCount;
	u16 row;
	register struct GhostProfile *profile CTR_PSX_REGISTER("$16");

	// NOTE(aalhendi): Retail tests the Adventure subtitle, then draws the Ghost subtitle.
	{
		u32 subtitlePage;
		const s16 *subtitleTable;
		CTR_PSX_LOAD_SYMBOL_PAGE(subtitlePage, "data+21480");
		CTR_PSX_ADD_SYMBOL_LOW(subtitleTable, subtitlePage, "data+21480", SELECT_PROFILE_ADV_TEXT);
		subtitleVisible = strlen(GAME_LANGUAGE_STRINGS[subtitleTable[(SELECT_PROFILE_ACTION * 2) + 1]]) != 0;
	}
	if ((s16)rowsWithEmpty >= 7)
	{
		font = FONT_SMALL;
		yBase = subtitleVisible ? 0xc : 0x12;
		lineGap = 8;
	}
	else
	{
		font = FONT_BIG;
		yBase = 0x12;
		lineGap = 0x10;
		if (SELECT_PROFILE_ACTION != SELECT_PROFILE_ACTION_SAVE)
		{
			DecalFont_DrawMultiLine(GAME_LANGUAGE_STRINGS[LNG_INSERT_ANY_MEMORY_CARD_WITH_GHOST_DATA_IN], 0x100, 0xbe, 0x1ce, FONT_SMALL,
			                        JUSTIFY_CENTER | color);
		}
	}

	CTR_PSX_KEEP_VALUE_RELAXED(yBase);
	CTR_PSX_KEEP_VALUE_RELAXED(lineGap);
	{
		register s32 firstTitleX CTR_PSX_REGISTER("$5") = 0x100;
		register s32 firstTitleY CTR_PSX_REGISTER("$6") = (s16)yBase;
		CTR_PSX_OBSERVE_VALUE(firstTitleX);
		CTR_PSX_OBSERVE_VALUE(firstTitleY);
		{
			register s32 retainedFont CTR_PSX_REGISTER("$16") = font;
			CTR_PSX_OBSERVE_VALUE(retainedFont);
		}
		titleFont = font;
		{
			register s16 callFont CTR_PSX_REGISTER("$7") = titleFont;
			{
				register u32 titlePage CTR_PSX_REGISTER("$3");
				CTR_PSX_LOAD_SYMBOL_PAGE(titlePage, "data+21364");
				CTR_PSX_ADD_SYMBOL_LOW(text, titlePage, "data+21364", SELECT_PROFILE_GHOST_TEXT);
			}
			titleColor = JUSTIFY_CENTER | color;
			subtitleY = yBase + lineGap;
			DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[text[SELECT_PROFILE_ACTION * 2]], firstTitleX, firstTitleY, callFont, titleColor);
		}
	}
	titleEndY = subtitleY;
	if (subtitleVisible != 0)
	{
		{
			register s32 secondX CTR_PSX_REGISTER("$5") = 0x100;
			register s32 secondY CTR_PSX_REGISTER("$6") = (s16)subtitleY;
			CTR_PSX_OBSERVE_VALUE(secondY);
			{
				register s16 secondFont CTR_PSX_REGISTER("$7") = titleFont;
				titleEndY = lineGap + subtitleY;
				DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[(text + (SELECT_PROFILE_ACTION * 2))[1]], secondX, secondY, secondFont, titleColor);
			}
		}
	}

	{
		register u32 profilePage CTR_PSX_REGISTER("$2");
		CTR_PSX_LOAD_SYMBOL_PAGE(profilePage, "sdata_static+56052");
		CTR_PSX_ADD_SYMBOL_LOW(profile, profilePage, "sdata_static+56052", &GAME_MEMCARD_STATE.ghostProfile_memcard[0]);
	}
	rowsWithEmpty =
	    (u16)CTR_PSX_PAGE_LVALUE(s16, profile, offsetof(struct MemcardState, numGhostProfilesSaved) - offsetof(struct MemcardState, ghostProfile_memcard),
	                             GAME_MEMCARD_STATE.numGhostProfilesSaved) +
	    canChooseEmptySlot;
	row = 0;
	if ((s16)rowsWithEmpty > 0)
	{
		u32 shiftedCount = rowCount << 16;
		register s32 visibleCount CTR_PSX_REGISTER("$22") = (s32)shiftedCount >> 16;
		register s32 finalPairedRow CTR_PSX_REGISTER("$23") = visibleCount - 1;
		register b32 useLargeRows CTR_PSX_REGISTER("$21") = visibleCount < 7;
		CTR_PSX_OBSERVE_VALUE(shiftedCount);
		do
		{
			register int x CTR_PSX_REGISTER("$5");
			register int xResult CTR_PSX_REGISTER("$2");
			int y;
			register u32 shiftedIndex CTR_PSX_REGISTER("$3") = index << 16;
			CTR_PSX_OBSERVE_VALUE(shiftedIndex);
			{
				register u32 loopCount CTR_PSX_REGISTER("$2") = (u32)savedCount << 16;
				CTR_PSX_OBSERVE_VALUE(shiftedIndex);
				CTR_PSX_KEEP_VALUE(loopCount);
				if (shiftedIndex == loopCount)
				{
					profile = NULL;
				}
			}
			{
				register s32 signedIndex CTR_PSX_REGISTER("$4") = (s32)shiftedIndex >> 16;
				if ((signedIndex < finalPairedRow) || ((index & 1) != 0))
				{
					xResult = ((index & 1) * 0xd4) + 0x2e;
				}
				else
				{
					xResult = 0x98;
				}
				CTR_PSX_OBSERVE_VALUE(signedIndex);
			}
			x = xResult;
			y = (s16)(titleEndY + 6 + (((s16)index / 2) * (useLargeRows ? 0x2f : 0x2b)));

			SelectProfile_DrawGhostProfile(profile, x, y, (s16)index == menu->rowSelected, (s16)index, ((s16)menu->drawStyle | (useLargeRows ? 0 : 0x40)),
			                               SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_LOAD,
			                               (profile != NULL) && (profile->trackID != GAME_TRACKER->currLEV) &&
			                                   (SELECT_PROFILE_ACTION != SELECT_PROFILE_ACTION_SAVE));
			if ((s16)index++ == (s16)rowCount)
			{
				break;
			}
#if defined(CTR_NATIVE)
			// NOTE(aalhendi): Advancing an empty native row must not form a pointer from NULL.
			if (profile != NULL)
#endif
			{
				profile++;
			}
			row++;
		} while ((s16)row < (s16)rowsWithEmpty);
	}
}

static __inline__ void SelectProfile_DrawAdvWarning(void)
{
	if ((SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_SAVE) && (SELECT_PROFILE_DATA_OUTDATED != 0))
	{
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_DATA_ON_MEMORY_CARD_IS_OUT_OF_DATE], 0x100, 0xc3, FONT_SMALL, JUSTIFY_CENTER | RED);
	}
}

static __inline__ void SelectProfile_DrawAdvRows(struct RectMenu *menu, u16 color)
{
	u16 i;
	register s16 *textTable CTR_PSX_REGISTER("$18");
	register u32 textPage CTR_PSX_REGISTER("$3");
	int textOffset;
	register int subtitleGap CTR_PSX_REGISTER("$16");
	register int titleColor CTR_PSX_REGISTER("$17");
	int titleLength;
	register int titleX CTR_PSX_REGISTER("$5");

	CTR_PSX_LOAD_SYMBOL_PAGE(textPage, "data+21480");
	textOffset = SELECT_PROFILE_ACTION * 2;
	CTR_PSX_ADD_SYMBOL_LOW(textTable, textPage, "data+21480", SELECT_PROFILE_ADV_TEXT);
	titleLength = strlen(GAME_LANGUAGE_STRINGS[textTable[textOffset + 1]]);
	titleX = 0x100;
	CTR_PSX_OBSERVE_VALUE(titleX);
	subtitleGap = (titleLength == 0) << 3;
	CTR_PSX_OBSERVE_VALUE(titleLength);
	titleColor = JUSTIFY_CENTER | color;

	DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[textTable[SELECT_PROFILE_ACTION * 2]], 0x100, subtitleGap | 0x12, FONT_BIG, titleColor);
	if (subtitleGap == 0)
	{
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[textTable[(SELECT_PROFILE_ACTION * 2) + 1]], 0x100, 0x22, FONT_BIG, titleColor);
	}

	for (i = 0; (s16)i < 4; i++)
	{
		s16 index = (s16)i;
		SelectProfile_DrawAdvProfile(&SelectProfile_MemcardProfile()->advProgress[index], (s16)(((i & 1) * 0xea) + 0x1a), (s16)(((index / 2) * 0x43) + 0x3c),
		                             index == menu->rowSelected, index, (s16)menu->drawStyle);
	}

	CTR_PSX_OBSERVE_VALUE(subtitleGap);
#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Native draws this warning only on the regular Adventure list.
	SelectProfile_DrawAdvWarning();
#endif
}


static __inline__ void SelectProfile_DrawOverwriteAdv(struct RectMenu *menu)
{
	s16 width = 0;
	struct RectMenu *overwriteMenu = &SELECT_PROFILE_MENU_OVERWRITE;

	RECTMENU_GetWidth(overwriteMenu, &width, 1);
	RECTMENU_DrawSelf(overwriteMenu, 0, 0, width);
	SelectProfile_DrawAdvProfile(&SelectProfile_MemcardProfile()->advProgress[menu->rowSelected], 0x92, 0x3c, 0, menu->rowSelected, (s16)menu->drawStyle);
}

static __inline__ void SelectProfile_DrawOverwriteGhost(struct RectMenu *menu)
{
	s16 width = 0;
	struct RectMenu *overwriteMenu = &SELECT_PROFILE_MENU_OVERWRITE_GHOST;

	RECTMENU_GetWidth(overwriteMenu, &width, 1);
	RECTMENU_DrawSelf(overwriteMenu, 0, 0, width);
	SelectProfile_DrawGhostProfile(&GAME_MEMCARD_STATE.ghostProfile_memcard[GAME_MEMCARD_STATE.ghostProfile_rowSelect], 0x9c, 0x3c, 0, 0, (s16)menu->drawStyle,
	                               0, 0);
}


static __inline__ void SelectProfile_StartLoadGhost(struct RectMenu *menu, s16 rowCount)
{
	if (menu->rowSelected >= rowCount - 1)
	{
		*SelectProfile_AllProfiles_ActionActive() = 1;
		*SelectProfile_AllProfiles_ActionDone() = 1;
#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Native menu-only loads may have no Ghost tape to clear.
		if (SELECT_PROFILE_GHOST_TAPE != NULL)
#endif
		{
			memset(SELECT_PROFILE_GHOST_TAPE, 0, 0x28);
		}

#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Complete native menu-only operations without a card callback.
		SELECT_PROFILE_ERROR_FLAG = 1;
#endif
		return;
	}

	// NOTE(aalhendi): Retail uses currLEV here; levelID is not updated to the
	// selected Time Trial track until the queued load starts.
	if (GAME_MEMCARD_STATE.ghostProfile_memcard[menu->rowSelected].trackID == GAME_TRACKER->currLEV)
	{
		GAME_MEMCARD_STATE.ghostProfile_indexLoad = menu->rowSelected;
		RefreshCard_StartMemcardAction(5);
		*SelectProfile_AllProfiles_ActionActive() = 1;
		return;
	}

	OtherFX_Play(5, 1);
}


static __inline__ void SelectProfile_DrawMemcardMessage(u16 screen, u16 color, struct RectMenu *menu)
{
	s32 *descriptor;
	u16 firstString;
	s32 multiLine;
	u16 i;
#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Native message drawing only indexes the ten canonical screens.
	if (screen >= 10)
	{
		return;
	}
#endif
	descriptor = &SELECT_PROFILE_MESSAGE_SCREENS[(s16)screen];
	CTR_PSX_LOAD_UNSIGNED_HALF(firstString, descriptor, 0, (u16)*descriptor);
	if (((s16)screen == MC_SCREEN_ERROR_NODATA) && (SELECT_PROFILE_MODE == SELECT_PROFILE_SCREEN_SLOT_SELECTED))
	{
		CTR_PSX_LOAD_IMMEDIATE(firstString, -1);
	}
	if ((SELECT_PROFILE_ACTION_ACTIVE != 0) && (SELECT_PROFILE_UNK_8008D964 != 0))
	{
		CTR_PSX_LOAD_IMMEDIATE(firstString, -1);
	}
	if (((s16)firstString < 0) || (SELECT_PROFILE_ACTION_DONE != 0))
	{
		return;
	}
	if (((s16)firstString == 0x10f) && (SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_SAVE))
	{
		firstString = 0x106;
	}
	if ((SELECT_PROFILE_MODE != SELECT_PROFILE_SCREEN_GHOST) && ((s16)screen == MC_SCREEN_ERROR_NODATA) && (SELECT_PROFILE_DATA_OUTDATED != 0))
	{
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_DATA_ON_MEMORY_CARD_IS_OUT_OF_DATE], 0x100, 0xc3, FONT_SMALL, ((s16)JUSTIFY_CENTER) | RED);
	}
	if ((SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_DELETE) && ((s16)firstString == 0xea))
	{
		firstString = 0xfc;
	}
	CTR_PSX_LOAD_SIGNED_HALF(multiLine, descriptor, 2, CTR_PSX_PAGE_LVALUE(s16, descriptor, 2, (s16)((u32)*descriptor >> 16)));
	if (multiLine == 0)
	{
		DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[(s16)firstString], 0x108, 0x12, FONT_BIG, ((s16)JUSTIFY_CENTER) | color);
	}
	else
	{
		s32 firstStringIndex;
		u32 firstStringBits = (u32)firstString << 16;
		register const s16 *fontHeights CTR_PSX_REGISTER("$22");
		s32 normalLineFlags;
		u32 fontPage;
		CTR_PSX_ZERO_VALUE_AFTER(i, firstStringBits, descriptor);
		CTR_PSX_ORDER_VALUES(i, descriptor);
		firstStringIndex = CTR_MipsSra(firstStringBits, 16);
		CTR_PSX_LOAD_SYMBOL_PAGE(fontPage, "data+6600");
		CTR_PSX_ADD_SYMBOL_LOW(fontHeights, fontPage, "data+6600", SELECT_PROFILE_FONT_HEIGHT);
		normalLineFlags = (s16)(JUSTIFY_CENTER | color);
		do
		{
			s32 lineIndex = (s16)i;
			char *line = GAME_LANGUAGE_STRINGS[firstStringIndex + lineIndex];
			if (strlen(line) != 0)
			{
				s32 fontOffset = sizeof(s16) * FONT_BIG;
				const s16 *fontHeight;
				s32 scaledY;
				s32 signedY;
				s32 defaultY;
				s32 fontLineIndex;
				s32 fontType;
				if (lineIndex != 0)
				{
					fontOffset = sizeof(s16) * FONT_SMALL;
				}
				CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(fontHeight, fontHeights, fontOffset);
				scaledY = lineIndex * (*fontHeight + 2);
				CTR_PSX_KEEP_VALUE_RELAXED(scaledY);
				defaultY = scaledY + 0x26;

				if (lineIndex > 0)
				{
					scaledY += 0x2e;
					CTR_PSX_KEEP_VALUE_RELAXED(scaledY);
					signedY = (s16)scaledY;
				}
				else
				{
					signedY = (s16)defaultY;
				}

				CTR_PSX_OBSERVE_VALUE(signedY);
				CTR_PSX_KEEP_VALUE_RELAXED(i);
				fontLineIndex = (s16)i;
				fontType = FONT_BIG;
				if (fontLineIndex != 0)
				{
					CTR_PSX_KEEP_VALUE_RELAXED(fontLineIndex);
					fontType = FONT_SMALL;
				}
				if ((fontLineIndex != 0) || (((u16)SELECT_PROFILE_FRAME_COUNTER & 4) != 0))
				{
					DecalFont_DrawLine(line, 0x100, signedY, fontType, normalLineFlags);
				}
				else
				{
					DecalFont_DrawLine(line, 0x100, signedY, fontType, (s16)(JUSTIFY_CENTER | RED));
				}
			}
		} while (i++, (s16)i < 9);
	}
#if defined(CTR_NATIVE)
	{
		// NOTE(aalhendi): Native loads the canonical byte rectangle through a real RECT.
		RECT rect;
		memcpy(&rect, SELECT_PROFILE_MESSAGE_RECT_BYTES, sizeof(rect));
		RECTMENU_DrawInnerRect(&rect, (s16)menu->drawStyle, GAME_TRACKER->backBuffer->otMem.uiOT);
	}
#else
	{
		u32 *uiOT = GAME_TRACKER->backBuffer->otMem.uiOT;
		s32 drawStyle = (s16)menu->drawStyle;
		RECT *rect;
		CTR_PSX_OBSERVE_VALUE(uiOT);
		rect = (RECT *)SELECT_PROFILE_MESSAGE_RECT_BYTES;
		CTR_PSX_OBSERVE_VALUE(rect);
		RECTMENU_DrawInnerRect(rect, drawStyle, uiOT);
	}
#endif
}


static __inline__ void SelectProfile_DrawAll(struct RectMenu *menu, u32 rowCount, u16 savedGhostCount, b16 canChooseEmptySlot, u16 color, b16 doSave)
{
	s16 screenOverride = (u16)SELECT_PROFILE_SCREEN_TEXT;
	int canDrawProfiles = (*SelectProfile_AllProfiles_ActionActive() == 0) && (SELECT_PROFILE_UNK_8008D95C != 0) &&
	                      ((SELECT_PROFILE_UNK_8008D928 != 0) || (screenOverride == MC_SCREEN_NULL));
	register int profileEligibility CTR_PSX_REGISTER("$2");

	if ((SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_LOAD) && (SELECT_PROFILE_MODE == SELECT_PROFILE_SCREEN_GHOST) &&
	    ((screenOverride == MC_SCREEN_ERROR_NODATA) || (screenOverride == MC_SCREEN_WARNING_NOCARD)) && ((s16)rowCount != 0))
	{
		canDrawProfiles = true;
	}

	if (SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_SAVE)
	{
		if ((screenOverride == MC_SCREEN_NULL) || (screenOverride == MC_SCREEN_ERROR_NODATA))
		{
			canDrawProfiles = false;

			if (doSave)
			{
				screenOverride = MC_SCREEN_SAVING;
			}
			else if ((SELECT_PROFILE_MODE == SELECT_PROFILE_SCREEN_GHOST))
			{
				canDrawProfiles = true;
				if ((SELECT_PROFILE_FREE_BYTES < SELECT_PROFILE_GHOST_SAVE_BYTES) && (savedGhostCount == 0))
				{
					canDrawProfiles = false;
					screenOverride = MC_SCREEN_ERROR_FULL;
				}
			}
			else
			{
				canDrawProfiles = true;
				if (((u32)SELECT_PROFILE_FREE_BYTES < SELECT_PROFILE_ADV_SAVE_BYTES) && (SELECT_PROFILE_UNK_8008D928 == 0))
				{
					screenOverride = MC_SCREEN_ERROR_FULL;
					canDrawProfiles = false;
				}
			}
		}
	}

	if ((SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_SAVE) && (screenOverride == MC_SCREEN_ERROR_TIMEOUT) &&
	    (*SelectProfile_AllProfiles_TimeoutPrompt() != 0))
	{
		canDrawProfiles = true;
	}

	SelectProfile_Init(menu->drawStyle);
	// NOTE(aalhendi): Retail consumes eligibility in v0 after initialization.
	profileEligibility = canDrawProfiles;

	if (profileEligibility && (*SelectProfile_AllProfiles_ActionActive() == 0))
	{
		if (SELECT_PROFILE_MODE != SELECT_PROFILE_SCREEN_GHOST)
		{
			if (*SelectProfile_AllProfiles_OverwritePrompt() != 0)
			{
				SelectProfile_DrawOverwriteAdv(menu);
			}
			else
			{
				SelectProfile_DrawAdvRows(menu, color);
			}
#if !defined(CTR_NATIVE)
			// NOTE(aalhendi): Retail also draws the warning after the Adventure overwrite prompt.
			SelectProfile_DrawAdvWarning();
#endif
		}
		else
		{
			if (*SelectProfile_AllProfiles_OverwritePrompt() != 0)
			{
				SelectProfile_DrawOverwriteGhost(menu);
			}
			else
			{
				SelectProfile_DrawGhostRows(menu, rowCount, savedGhostCount, canChooseEmptySlot, color);
			}
		}
	}
	else
	{
		*SelectProfile_AllProfiles_OverwritePrompt() = 0;

		if ((*SelectProfile_AllProfiles_ActionActive() != 0) && (SELECT_PROFILE_UNK_8008D964 != 0) && (*SelectProfile_AllProfiles_ExitToPrevious() == 0) &&
		    (*SelectProfile_AllProfiles_ActionDone() == 0) && (*SelectProfile_AllProfiles_TimerSaveComplete() != 0))
		{
			DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[LNG_SAVE_COMPLETED], 0x108, 0x64, FONT_BIG,
			                   (((u16)SELECT_PROFILE_FRAME_COUNTER & 4) == 0) ? (s16)(JUSTIFY_CENTER | ORANGE) : (s16)(JUSTIFY_CENTER | WHITE));
		}
		else
		{
			SelectProfile_DrawMemcardMessage(screenOverride, color, menu);
		}
	}
}


static __inline__ void SelectProfile_Finalize(struct RectMenu *menu)
{
	switch (*SelectProfile_AllProfiles_Mode())
	{
	case SELECT_PROFILE_SCREEN_ADV_NEW:
		if (*SelectProfile_AllProfiles_ExitToPrevious() != 0)
		{
			SELECT_PROFILE_DESIRED_MENU = &SELECT_PROFILE_MENU_SUBMIT_NAME;
			SubmitName_RestoreName(0);
			return;
		}

		SELECT_PROFILE_ADV_INDEX = menu->rowSelected;
		GAME_TRACKER->currLEV = N_SANITY_BEACH;
		Garage_Leave();
		SELECT_PROFILE_DESIRED_MENU = QueueLoadTrack_GetMenuPtr();
		return;

	case SELECT_PROFILE_SCREEN_ADV_LOAD:
		if (*SelectProfile_AllProfiles_ExitToPrevious() != 0)
		{
			struct RectMenu *advMenu;
			MM_JumpTo_Title_Returning();
			advMenu = MM_AdvNewLoad_GetMenuPtr();
			advMenu->state &= ~ONLY_DRAW_TITLE;
			return;
		}
		else
		{
			struct GameTracker *gGT = GAME_TRACKER;
			struct GameTracker *nameTracker = gGT;
			s32 hub;
			struct GameTracker *hubTracker;
			CTR_PSX_KEEP_VALUE_RELAXED(nameTracker);

			SELECT_PROFILE_ADV_INDEX = menu->rowSelected;
			CTR_PSX_OBSERVE_MEMORY(SELECT_PROFILE_ADV_INDEX);
			// NOTE(aalhendi): The canonical profile and tracker name buffers do not overlap.
			memcpy(gGT->prevNameEntered, GAME_ADV_PROGRESS.name, sizeof(gGT->prevNameEntered));
			memcpy(nameTracker->currNameEntered, GAME_ADV_PROGRESS.name, sizeof(nameTracker->currNameEntered));
			hubTracker = GAME_TRACKER;
			hub = N_SANITY_BEACH;
			if (GAME_ADV_PROGRESS.HubLevYouSavedOn != 0)
			{
				hub = GAME_ADV_PROGRESS.HubLevYouSavedOn;
			}
			hubTracker->currLEV = hub;
		}
		SELECT_PROFILE_DESIRED_MENU = QueueLoadTrack_GetMenuPtr();
		return;

	case SELECT_PROFILE_SCREEN_GREEN_LOAD_SAVE:
		if (*SelectProfile_AllProfiles_ExitToPrevious() == 0)
		{
			SELECT_PROFILE_ADV_INDEX = menu->rowSelected;
			if (SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_LOAD)
			{
				GAMEPROG_AdvPercent(&GAME_ADV_PROGRESS);
				SELECT_PROFILE_DESIRED_MENU = &SELECT_PROFILE_MENU_QUEUE_LOAD_HUB;
				GAME_TRACKER->currLEV = GAME_ADV_PROGRESS.HubLevYouSavedOn;
				SELECT_PROFILE_MENU_GREEN_LOAD_SAVE.rowSelected = 3;
				return;
			}
		}
		SELECT_PROFILE_DESIRED_MENU = &SELECT_PROFILE_MENU_GREEN_LOAD_SAVE;
		SELECT_PROFILE_MENU_GREEN_LOAD_SAVE.rowSelected = 3;
		return;

	case SELECT_PROFILE_SCREEN_GHOST:
		SelectProfile_Destroy();
		if (SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_SAVE)
		{
			SELECT_PROFILE_DESIRED_MENU = (*SelectProfile_AllProfiles_ExitToPrevious() != 0) ? &menu224 : &menu224NoSave;
			return;
		}
		if (*SelectProfile_AllProfiles_ExitToPrevious() != 0)
		{
			GhostTape_Destroy();
			SELECT_PROFILE_DESIRED_MENU = MM_TrackSelect_GetMenuPtr();
			MM_TrackSelect_Init();
			return;
		}
#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Native can finish an empty-ghost selection without a tape.
		if (SELECT_PROFILE_GHOST_TAPE != NULL)
#endif
		{
			GAME_CHARACTER_IDS[1] = SELECT_PROFILE_GHOST_TAPE->characterID;
		}
		SELECT_PROFILE_DESIRED_MENU = QueueLoadTrack_GetMenuPtr();
		return;

	case SELECT_PROFILE_SCREEN_SLOT_SELECTED:
		SelectProfile_Destroy();
		if (SELECT_PROFILE_SAVE_CUP != 0)
		{
			if (*SelectProfile_AllProfiles_ExitToPrevious() != 0)
			{
				SELECT_PROFILE_DESIRED_MENU = &SELECT_PROFILE_MENU_SAVE_GAME;
				return;
			}
			RECTMENU_Hide(menu);
			return;
		}
		else
		{
			GAME_TRACKER->gameModeEnd &= ~(NEW_NAME | NEW_HIGH_SCORE);
		}
		RECTMENU_Hide(menu);
		return;

	default:
		return;
	}
}


void SelectProfile_AllProfiles_MenuProc(struct RectMenu *menu)
{
	u16 color = ((menu->drawStyle & SELECT_PROFILE_DRAW_STYLE_GREEN) != 0) ? LIGHT_GREEN : ORANGE;
	u16 savedGhostCount = GAME_MEMCARD_STATE.numGhostProfilesSaved;
	b16 canChooseEmptySlot = false;
	u32 rowCount = 0;

	b16 doSave = false;
#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Native keeps the initial Ghost count across overwrite prompt processing.
	if (SELECT_PROFILE_MODE == SELECT_PROFILE_SCREEN_GHOST)
		rowCount = SelectProfile_GhostRowCount(&savedGhostCount, &canChooseEmptySlot);
#endif

	if (SELECT_PROFILE_SCREEN_TEXT == MC_SCREEN_WARNING_NOCARD)
	{
		*SelectProfile_AllProfiles_OverwritePrompt() = 0;
	}

	if ((u16)SELECT_PROFILE_SCREEN_TEXT < MC_SCREEN_FORMATTING)
	{
		*SelectProfile_AllProfiles_ActionActive() = 0;
	}

	SelectProfile_UnMuteCursors();
	if ((*SelectProfile_AllProfiles_ExitToPrevious() != 0) || (*SelectProfile_AllProfiles_ActionDone() != 0) ||
	    (*SelectProfile_AllProfiles_ActionActive() != 0))
	{
		SelectProfile_MuteCursors();
	}

	if (*SelectProfile_AllProfiles_OverwritePrompt() != 0)
	{
		u32 tap = SELECT_PROFILE_BUTTON_TAP;
		if ((tap & SELECT_PROFILE_INPUT_NAV_CONFIRM) != 0)
		{
			if ((tap & BTN_UP) != 0)
			{
				if (SELECT_PROFILE_MENU_OVERWRITE.rowSelected > 0)
				{
					OtherFX_Play(0, 1);
					SELECT_PROFILE_MENU_OVERWRITE.rowSelected--;
				}
			}
			else if ((tap & BTN_DOWN) != 0)
			{
				if (SELECT_PROFILE_MENU_OVERWRITE.rowSelected < 1)
				{
					OtherFX_Play(0, 1);
					SELECT_PROFILE_MENU_OVERWRITE.rowSelected++;
				}
			}
			else if ((tap & (BTN_CROSS_one | BTN_CIRCLE | BTN_TRIANGLE | BTN_SQUARE_one)) != 0)
			{
				if ((tap & (BTN_CROSS_one | BTN_CIRCLE)) != 0)
				{
					OtherFX_Play(1, 1);
					if (SELECT_PROFILE_MENU_OVERWRITE.rowSelected == 0)
						doSave = true;
				}
				else
					OtherFX_Play(2, 1);
				*SelectProfile_AllProfiles_OverwritePrompt() = 0;
			}
			RECTMENU_ClearInput();
			SELECT_PROFILE_MENU_OVERWRITE_GHOST.rowSelected = SELECT_PROFILE_MENU_OVERWRITE.rowSelected;
		}
		goto draw_and_finish;
	}

	if ((SELECT_PROFILE_MODE == SELECT_PROFILE_SCREEN_GHOST))
	{
		if (SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_SAVE)
		{
			canChooseEmptySlot = SELECT_PROFILE_FREE_BYTES >= SELECT_PROFILE_GHOST_SAVE_BYTES;
			savedGhostCount = GAME_MEMCARD_STATE.numGhostProfilesSaved;
#if defined(CTR_NATIVE)
			savedGhostCount = SelectProfile_ClampNativeGhostCount(savedGhostCount);
#endif
			{
				register u32 expandedCount CTR_PSX_REGISTER("$2") = savedGhostCount + canChooseEmptySlot;
				CTR_PSX_KEEP_VALUE_RELAXED(expandedCount);
				rowCount = expandedCount;
				CTR_PSX_SHIFT_LEFT_IN_PLACE(expandedCount, 16);
				CTR_PSX_SHIFT_RIGHT_ARITHMETIC_IN_PLACE(expandedCount, 16);
				if ((s32)expandedCount > SELECT_PROFILE_GHOST_SLOT_COUNT)
				{
					rowCount = SELECT_PROFILE_GHOST_SLOT_COUNT;
					canChooseEmptySlot = false;
				}
			}
		}
		else
		{
#if defined(CTR_NATIVE)
			savedGhostCount = SelectProfile_ClampNativeGhostCount(savedGhostCount);
#endif
			canChooseEmptySlot = true;
			rowCount = savedGhostCount + 1;
		}
		SelectProfile_ClampRow(menu, (s16)rowCount);
	}

	if (*SelectProfile_AllProfiles_ActionActive() == 0)
	{
		s16 handled = false;
		if ((SELECT_PROFILE_MODE == SELECT_PROFILE_SCREEN_GHOST) && (*SelectProfile_AllProfiles_OverwritePrompt() == 0))
		{
			handled = SelectProfile_InputLogic(menu, (s16)rowCount, 0);
		}
		else if (*SelectProfile_AllProfiles_Mode() == SELECT_PROFILE_SCREEN_SLOT_SELECTED)
		{
			if (SELECT_PROFILE_SCREEN_TEXT == MC_SCREEN_WARNING_NOCARD)
			{
				if ((SELECT_PROFILE_BUTTON_TAP & (BTN_CROSS_one | BTN_CIRCLE)) != 0)
				{
					OtherFX_Play(1, 1);
					if (SELECT_PROFILE_SAVE_CUP == 0)
					{
						MainGameEnd_SoloRaceSaveHighScore();
					}
					RECTMENU_ClearInput();
					*SelectProfile_AllProfiles_ActionActive() = 1;
					*SelectProfile_AllProfiles_ExitToPrevious() = 1;

#if defined(CTR_NATIVE)
					// NOTE(aalhendi): Complete native menu-only operations without a card callback.
					SELECT_PROFILE_ERROR_FLAG = 1;
#endif
				}
			}
			else if (SELECT_PROFILE_SCREEN_TEXT == MC_SCREEN_WARNING_UNFORMATTED)
			{
				if ((SELECT_PROFILE_BUTTON_TAP & BTN_CIRCLE) != 0)
				{
					RECTMENU_ClearInput();
					OtherFX_Play(1, 1);
					RefreshCard_StartMemcardAction(7);
				}
			}
			else if ((((u32)SELECT_PROFILE_FREE_BYTES >= SELECT_PROFILE_ADV_SAVE_BYTES) || (SELECT_PROFILE_UNK_8008D928 != 0) ||
			          (SELECT_PROFILE_ACTION != SELECT_PROFILE_ACTION_SAVE)) &&
			         ((SELECT_PROFILE_UNK_8008D95C != 0) || (SELECT_PROFILE_UNK_8008D928 != 0)))
			{
				if (SELECT_PROFILE_SAVE_CUP == 0)
				{
					MainGameEnd_SoloRaceSaveHighScore();
				}

				SelectProfile_CopyGameProgressToCard();
				MEMCARD_SetIcon(0);
				RefreshCard_StartMemcardAction(3);
				SELECT_PROFILE_UNK_8008D928 = 1;
				*SelectProfile_AllProfiles_ActionActive() = 1;
			}
		}
		else
		{
			handled = SelectProfile_InputLogic(menu, 4, SelectProfile_DisableAdvInputFlags());
		}

		if (handled != 0)
		{
			{
				s16 proceed = true;

				if (menu->rowSelected == -1)
				{
					*SelectProfile_AllProfiles_ActionActive() = 1;
					*SelectProfile_AllProfiles_ExitToPrevious() = 1;

#if defined(CTR_NATIVE)
					// NOTE(aalhendi): Complete native menu-only operations without a card callback.
					SELECT_PROFILE_ERROR_FLAG = 1;
#endif
					goto draw_and_finish;
				}

				if (SELECT_PROFILE_SCREEN_TEXT == MC_SCREEN_WARNING_NOCARD)
				{
					proceed = false;

					if (SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_SAVE)
					{
						*SelectProfile_AllProfiles_ActionActive() = 1;
						*SelectProfile_AllProfiles_ActionDone() = 1;

#if defined(CTR_NATIVE)
						// NOTE(aalhendi): Complete native menu-only operations without a card callback.
						SELECT_PROFILE_ERROR_FLAG = 1;
#endif
					}
					else if ((SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_LOAD) && (SELECT_PROFILE_MODE == SELECT_PROFILE_SCREEN_GHOST))
					{
						proceed = true;
					}
				}

				if ((SELECT_PROFILE_MODE != SELECT_PROFILE_SCREEN_GHOST) && (SELECT_PROFILE_SCREEN_TEXT != MC_SCREEN_WARNING_UNFORMATTED) &&
				    ((u32)SELECT_PROFILE_FREE_BYTES < SELECT_PROFILE_ADV_SAVE_BYTES) && (SELECT_PROFILE_UNK_8008D928 == 0) &&
				    (SELECT_PROFILE_ACTION == SELECT_PROFILE_ACTION_SAVE))
				{
					proceed = false;
				}

				if (!proceed)
				{
					goto draw_and_finish;
				}

				if (SELECT_PROFILE_SCREEN_TEXT == MC_SCREEN_WARNING_UNFORMATTED)
				{
					RefreshCard_StartMemcardAction(7);
					goto draw_and_finish;
				}

				if ((SELECT_PROFILE_UNK_8008D95C == 0) && (SELECT_PROFILE_UNK_8008D928 == 0))
				{
					goto draw_and_finish;
				}

				switch (SELECT_PROFILE_ACTION)
				{
				case SELECT_PROFILE_ACTION_LOAD:

					if (SELECT_PROFILE_MODE == SELECT_PROFILE_SCREEN_GHOST)
					{
						SelectProfile_StartLoadGhost(menu, (s16)rowCount);
					}
					else if (SELECT_PROFILE_SCREEN_TEXT == MC_SCREEN_ERROR_TIMEOUT)
					{
						*SelectProfile_AllProfiles_ActionActive() = 1;
						*SelectProfile_AllProfiles_ExitToPrevious() = 1;
					}
					else if (SelectProfile_AdvProfileOccupied(menu->rowSelected))
					{
						register u16 selectedRow CTR_PSX_REGISTER("$3");
						SelectProfile_LoadAdvProfile(menu);
						selectedRow = menu->rowSelected;
						CTR_PSX_KEEP_VALUE(selectedRow);
						*SelectProfile_AllProfiles_ActionActive() = 1;
						*SelectProfile_AllProfiles_ActionDone() = 1;
						CTR_PSX_STORE_HALF(SELECT_PROFILE_ROW_BACKUP, selectedRow);

#if defined(CTR_NATIVE)
						// NOTE(aalhendi): Complete native menu-only operations without a card callback.
						SELECT_PROFILE_ERROR_FLAG = 1;
#endif
					}
					else
					{
						OtherFX_Play(5, 1);
					}

					break;
				case SELECT_PROFILE_ACTION_SAVE:

					if (SELECT_PROFILE_MODE == SELECT_PROFILE_SCREEN_GHOST)
					{
						if (menu->rowSelected < GAME_MEMCARD_STATE.numGhostProfilesSaved)
						{
							GAME_MEMCARD_STATE.ghostProfile_rowSelect = menu->rowSelected;
							SELECT_PROFILE_MENU_OVERWRITE.rowSelected = 1;
							*SelectProfile_AllProfiles_OverwritePrompt() = 1;
							SELECT_PROFILE_MENU_OVERWRITE_GHOST.rowSelected = 1;
							goto draw_and_finish;
						}
					}
					else
					{
						if ((*SelectProfile_AllProfiles_TimeoutPrompt() == 0) && (SELECT_PROFILE_SCREEN_TEXT == MC_SCREEN_ERROR_TIMEOUT))
						{
							*SelectProfile_AllProfiles_TimeoutPrompt() = 1;
							goto draw_and_finish;
						}

						if (!SelectProfile_AdvProfileOccupied(menu->rowSelected))
							goto save_selected_profile;
						if (menu->rowSelected != SELECT_PROFILE_ADV_INDEX)
							goto prompt_for_overwrite;
					}

				save_selected_profile:
					doSave = true;
					goto draw_and_finish;

				prompt_for_overwrite:
					SELECT_PROFILE_MENU_OVERWRITE.rowSelected = 1;
					SELECT_PROFILE_MENU_OVERWRITE_GHOST.rowSelected = 1;
					*SelectProfile_AllProfiles_OverwritePrompt() = 1;
					goto draw_and_finish;

					break;
				case SELECT_PROFILE_ACTION_DELETE:
					if ((SELECT_PROFILE_MODE != SELECT_PROFILE_SCREEN_GHOST) && SelectProfile_AdvProfileOccupied(menu->rowSelected))
					{
						GAMEPROG_NewProfile_InsideAdv(&SelectProfile_MemcardProfile()->advProgress[menu->rowSelected]);
						MEMCARD_SetIcon(0);
						RefreshCard_StartMemcardAction(3);
						SELECT_PROFILE_UNK_8008D928 = 1;
						*SelectProfile_AllProfiles_ActionActive() = 1;
					}
					break;
				}

				goto draw_and_finish;
			}
		}
	}

draw_and_finish:
	if ((SELECT_PROFILE_SCREEN_TEXT == MC_SCREEN_ERROR_TIMEOUT) && (*SelectProfile_AllProfiles_ExitToPrevious() == 0) &&
	    (*SelectProfile_AllProfiles_ActionDone() == 0))
	{
		*SelectProfile_AllProfiles_ActionActive() = 0;
		*SelectProfile_AllProfiles_OverwritePrompt() = 0;
	}

	if (doSave != 0)
	{
		*SelectProfile_AllProfiles_TimeoutPrompt() = 0;
		if ((SELECT_PROFILE_MODE == SELECT_PROFILE_SCREEN_GHOST))
		{
			SelectProfile_StartGhostSave(menu);
		}
		else
		{
			SelectProfile_SaveAdvProfile(menu);
		}

		*SelectProfile_AllProfiles_TimerSaveComplete() = 0x3c;
	}

	if (menu->funcState == RECTMENU_FUNC_STATE_UPDATE)
	{
		SelectProfile_DrawAll(menu, rowCount, savedGhostCount, canChooseEmptySlot, color, doSave);
	}

	if (*SelectProfile_AllProfiles_ActionActive() == 0)
	{
		return;
	}
	if (SELECT_PROFILE_ERROR_FLAG == 0)
	{
		return;
	}
	if ((SELECT_PROFILE_UNK_8008D964 == 0) && (*SelectProfile_AllProfiles_ExitToPrevious() == 0) && (*SelectProfile_AllProfiles_ActionDone() == 0))
	{
		return;
	}
	if ((*SelectProfile_AllProfiles_ActionActive() != 0) && (SELECT_PROFILE_UNK_8008D964 != 0) && (*SelectProfile_AllProfiles_ExitToPrevious() == 0) &&
	    (*SelectProfile_AllProfiles_ActionDone() == 0) && (*SelectProfile_AllProfiles_TimerSaveComplete() != 0))
	{
		(*SelectProfile_AllProfiles_TimerSaveComplete())--;
		return;
	}
	SelectProfile_InitAndDestroy();
	RefreshCard_StopMemcardAction();
	SelectProfile_Finalize(menu);
}


void SelectProfile_InitAndDestroy(void)
{
	SelectProfile_Init((s16)SELECT_PROFILE_MENU_ADVENTURE.drawStyle);
	SelectProfile_Destroy();
}
