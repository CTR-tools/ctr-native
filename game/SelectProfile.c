#include <common.h>
#include <chaos_save.h>
#include <adventure_difficulty_save.h>

// Defined with the profile load and save helpers further down.
static void SelectProfile_ApplyAdvMultiplayerCharacter(void);
static const struct AdvMultiplayerSettings *SelectProfile_AdvSettingsToShow(const struct AdvProgress *adv, int slot);
static b32 SelectProfile_CardSlotIsChaos(int slot);
static enum AdventureDifficulty SelectProfile_AdvDifficultyToShow(const struct AdvProgress *adv, int slot);

static const char *SelectProfile_DifficultyLabel(enum AdventureDifficulty difficulty)
{
	switch (difficulty)
	{
	case ADV_DIFFICULTY_EASY: return LANG_GetString(LNG_EASY);
	case ADV_DIFFICULTY_HARD: return LANG_GetString(LNG_HARD);
	case ADV_DIFFICULTY_SUPER_HARD: return LANG_GetString(LNG_EXPANDED_SUPER_HARD);
	case ADV_DIFFICULTY_ULTRA_HARD: return LANG_GetString(LNG_EXPANDED_ULTRA_HARD);
	case ADV_DIFFICULTY_NORMAL:
	case ADV_DIFFICULTY_COUNT:
	default: return NULL;
	}
}

void SelectProfile_QueueLoadHub_MenuProc(struct RectMenu *menu)
{
	struct GameTracker *gGT = sdata->gGT;

	// NOTE(aalhendi): Retail stores 0x27 before LOAD_LevelFile records prevLEV.
	gGT->levelID = MAIN_MENU_LEVEL;

	data.characterIDs[0] = sdata->advProgress.characterID;
	SelectProfile_ApplyAdvMultiplayerCharacter();
	MainRaceTrack_RequestLoad(gGT->currLEV);
	RECTMENU_Hide(menu);
	return;
}


void SelectProfile_ThTick(struct Thread *t)
{
	struct SelectProfileLoadSaveObj *obj;
	struct SelectProfileLoadSaveIcon *icon;
	int i;

	obj = (struct SelectProfileLoadSaveObj *)t->object;
	icon = obj->icons;

	for (i = 0; i < 12; i++, icon++)
	{
		int slot = i % 3;
		struct Instance *inst = icon->inst;

		icon->rot.y = (s16)(icon->rot.y + sdata->LoadSave_SpinRateY[slot]);

		// NOTE(aalhendi): Menu-storage can keep this thread alive when the
		// load/save icon models were not published for the current level.
		if (inst == NULL)
		{
			continue;
		}

		ConvertRotToMatrix(&inst->matrix, &icon->rot);

		if (slot != 1)
		{
			Vector_SpecLightSpin3D(inst, &icon->rot, &data.MetaDataLoadSave[i].vec3_specular_inverted);
		}
	}
}


void SelectProfile_PrintInteger(int value, int posX, int posY, b32 usePaddedFormat, int color)
{
	char text[64];
	char *format;

	if (usePaddedFormat == 1)
	{
		format = &sdata->stringFormat1[0];
	}
	else
	{
		format = &sdata->stringFormat2[0];
	}

	sprintf(text, format, value);
	DecalFont_DrawLine(text, posX, posY, FONT_BIG, color);
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


static void SelectProfile_DrawAdvProfile_UpdateIcon(struct SelectProfileLoadSaveObj *obj, int index, int posX, int posY)
{
	struct Instance *inst = obj->icons[index].inst;

	if (inst == NULL)
	{
		return;
	}

	inst->matrix.t[0] = SelectProfile_UI_ConvertX(posX, SELECT_PROFILE_UI_SCALE);
	inst->matrix.t[1] = SelectProfile_UI_ConvertY(posY, SELECT_PROFILE_UI_SCALE);
	inst->matrix.t[2] = SELECT_PROFILE_UI_DEPTH;
	inst->flags &= ~HIDE_MODEL;
}

// A racer avatar is drawn at the size of its own texture, which is what the
// 0x1000 scale means for the decal draw helpers.
static void SelectProfile_DrawRacerIcon(struct GameTracker *gGT, int characterID, Color color, int posX, int posY)
{
	int iconID = data.MetaDataCharacters[characterID].iconID;

	RECTMENU_DrawPolyGT4(gGT->ptrIcons[iconID], posX, posY, &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT,
		color, color, color, color, 1, 0x1000);
}

void SelectProfile_DrawAdvProfile(struct AdvProgress *adv, int posX, int posY, s16 isHighlighted, s16 slotIndex, b32 isChaos, u16 menuFlag)
{
	struct GameTracker *gGT = sdata->gGT;
	RECT profileRect;

	Color iconColor;
	int numberColor;
	int emptyColor;
	int nameColor;
	int percentColor;
	const char *difficultyLabel = NULL;
	const char *chaosLabel = NULL;

	if ((menuFlag & SELECT_PROFILE_DRAW_STYLE_GREEN) != 0)
	{
		ColorCode_SetPacked(&iconColor, (u32)sdata->greenColor);
		numberColor = LIGHT_GREEN;
		emptyColor = FOREST_GREEN;
		nameColor = LIGHT_GREEN;
		percentColor = LIGHT_GREEN;
	}
	else
	{
		ColorCode_SetPacked(&iconColor, (u32)sdata->greyColor);
		numberColor = ORANGE;
		emptyColor = RED;
		nameColor = PERIWINKLE;
		percentColor = WHITE;
	}

	// The row is identified by its profile slot, while the spinning models of the
	// row come in threes.
	int profileSlot = slotIndex;

	slotIndex *= 3;
	GAMEPROG_AdvPercent(adv);

	if (adv->characterID < 0)
	{
		DecalFont_DrawLine(sdata->lngStrings[LNG_EMPTY], posX + 0x6c, posY + 0x17, FONT_BIG, JUSTIFY_CENTER | emptyColor);
	}
	else
	{
		int numPlayers = SelectProfile_AdvSettingsToShow(adv, profileSlot)->numPlayers;
		struct SelectProfileLoadSaveObj *obj = (struct SelectProfileLoadSaveObj *)sdata->ptrLoadSaveObj;
		// A co-op profile shows the partner next to the first racer, and the group
		// then moves left to keep clear of the text of the row.
		int firstIconX = posX + SELECT_PROFILE_ADV_ICON_X;
		int firstIconY = posY + SELECT_PROFILE_ADV_ICON_Y;

		if (numPlayers > 1)
		{
			firstIconX -= 8;
			firstIconY -= 2;
			int characterID2 = SelectProfile_AdvMultiplayerPartner(SelectProfile_AdvSettingsToShow(adv, profileSlot));
			SelectProfile_DrawRacerIcon(gGT, characterID2, iconColor, firstIconX, firstIconY + 28);
		}

		int characterID = adv->characterID;
		SelectProfile_DrawRacerIcon(gGT, characterID, iconColor, firstIconX, firstIconY);

		DecalFont_DrawLine(adv->name, posX + 0x6c, posY + 0x29, FONT_BIG, JUSTIFY_CENTER | nameColor);

		difficultyLabel = SelectProfile_DifficultyLabel(SelectProfile_AdvDifficultyToShow(adv, profileSlot));
		if (isChaos)
		{
		chaosLabel = LANG_GetString(LNG_EXPANDED_CHAOS);
		}

		int profileTextColor = JUSTIFY_RIGHT | numberColor;

		SelectProfile_PrintInteger(gGT->currAdvProfile.completionPercent, posX + 0x6a, posY + 0x17, 0, profileTextColor);
		SelectProfile_PrintInteger(gGT->currAdvProfile.numTrophies, posX + 0x6a, posY + 5, 0, profileTextColor);
		SelectProfile_PrintInteger(gGT->currAdvProfile.numKeys, posX + 0xb5, posY + 5, 0, profileTextColor);
		SelectProfile_PrintInteger(gGT->currAdvProfile.numRelics, posX + 0xb5, posY + 0x17, 0, profileTextColor);

		DecalFont_DrawLine((char *)&sdata->s_percent_sign, posX + 0x70, posY + 0x17, FONT_BIG, percentColor);

		SelectProfile_DrawAdvProfile_UpdateIcon(obj, slotIndex, posX + 0xc3, posY + 0x1f);
		SelectProfile_DrawAdvProfile_UpdateIcon(obj, slotIndex + 1, posX + 0x78, posY + 0xd);
		SelectProfile_DrawAdvProfile_UpdateIcon(obj, slotIndex + 2, posX + 0xc3, posY + 0xd);
	}

	profileRect.x = posX;
	profileRect.y = posY;
	profileRect.w = 0xdc;
	profileRect.h = 0x3d;

	if (isHighlighted != 0)
	{
		RECT highlightRect;
		Color *highlightColor = ((menuFlag & SELECT_PROFILE_DRAW_STYLE_GREEN) != 0) ? &sdata->menuRowHighlight_Green : &sdata->menuRowHighlight_Normal;

		highlightRect.x = posX + 6;
		highlightRect.y = posY + 4;
		highlightRect.w = 0xd0;
		highlightRect.h = 0x35;

		CTR_Box_DrawClearBox(&highlightRect, highlightColor, 1, &gGT->backBuffer->otMem.uiOT[3], &gGT->backBuffer->primMem);
	}

	enum { LABEL_GAP_PADDING = 6, TOP_LABEL_Y_OFFSET = 3, BOTTOM_LABEL_Y_OFFSET = 4 };
	int centerX = posX + profileRect.w / 2;
	int topGap = difficultyLabel != NULL ? DecalFont_GetLineWidth((char *)difficultyLabel, FONT_SMALL) + LABEL_GAP_PADDING : 0;
	int bottomGap = chaosLabel != NULL ? DecalFont_GetLineWidth((char *)chaosLabel, FONT_SMALL) + LABEL_GAP_PADDING : 0;
	if (difficultyLabel != NULL)
	{
		DecalFont_DrawLine((char *)difficultyLabel, centerX, posY - TOP_LABEL_Y_OFFSET, FONT_SMALL, JUSTIFY_CENTER | numberColor);
	}
	if (chaosLabel != NULL)
	{
		DecalFont_DrawLine((char *)chaosLabel, centerX, posY + profileRect.h - BOTTOM_LABEL_Y_OFFSET, FONT_SMALL, JUSTIFY_CENTER | numberColor);
	}
	RECTMENU_DrawInnerRectWithGaps(&profileRect, (s16)menuFlag, &gGT->backBuffer->otMem.uiOT[3], topGap, bottomGap);
}


void SelectProfile_GetTrackID()
{
	data.menuGreenLoadSave.rowSelected = 1;
	sdata->advProgress.HubLevYouSavedOn = sdata->gGT->levelID;
}

// NOTE(aalhendi): Retail stores these debug names in EXE RDATA; native does not
// expose the full retail RDATA struct.
static char s_SelectProfileThreadName[] = "LoadSave";
static char s_SelectProfileInstName[] = "loadsave";

static u32 SelectProfile_LoadSave_Color(int index, u32 flags)
{
	u32 red = (u8)data.MetaDataLoadSave[index].r;
	u32 green = (u8)data.MetaDataLoadSave[index].g;
	u32 blue = (u8)data.MetaDataLoadSave[index].b;

	if ((flags & SELECT_PROFILE_DRAW_STYLE_GREEN) != 0)
	{
		red >>= 1;
		blue >>= 1;
	}

	return (red << 0x14) | (green << 0xc) | (blue << 4);
}

void SelectProfile_Init(u16 flags)
{
	struct GameTracker *gGT;
	struct SelectProfileLoadSaveObj *obj;
	struct SelectProfileLoadSaveIcon *icon;
	struct Thread *t;
	int i;

	obj = (struct SelectProfileLoadSaveObj *)sdata->ptrLoadSaveObj;

	if (obj == NULL)
	{
		char *threadName = &s_SelectProfileThreadName[0];

		t = PROC_BirthWithObject(SIZE_RELATIVE_POOL_BUCKET(sizeof(struct SelectProfileLoadSaveObj), NONE, SMALL, OTHER), SelectProfile_ThTick, threadName,
		                         NULL);

		// NOTE(aalhendi): Native low-RAM audit candidate only. Retail writes
		// through this allocation result before its later null check; keep
		// unpatched until a valid menu repro proves the allocation can fail.
		// Proven, twice: PROC_BirthWithObject returns null both when no stack is
		// free and when objectSize reaches THREAD_SMALL_STACK_SIZE minus
		// sizeof(struct Item), and the menu is reachable from the main menu's
		// Load entry. Without this the whole menu dies on that path.
		if (t == NULL)
		{
			return;
		}

		obj = (struct SelectProfileLoadSaveObj *)t->object;
		sdata->ptrLoadSaveObj = (int)obj;

		// PROC_BirthWithObject hands back a recycled pool block without clearing it,
		// and SelectProfile_Init only fills the fields it goes on to read. The Chaos
		// marker is tested for NULL before it is born, so without this the stale
		// contents decide the outcome and the tick writes through whatever pointer
		// the block's previous owner left there.
		memset(obj, 0, sizeof(struct SelectProfileLoadSaveObj));

		obj->icons = &sdata->LoadSaveData[0];
		memset(obj->icons, 0, sizeof(sdata->LoadSaveData));

		if (obj == NULL)
		{
			return;
		}

		obj->thread = t;
	}

	gGT = sdata->gGT;
	icon = obj->icons;

	for (i = 0; i < 12; i++, icon++)
	{
		struct Instance *inst;
		int slot;

		if (icon->inst == NULL)
		{
			struct Model *model = gGT->modelPtr[data.MetaDataLoadSave[i].modelID];
			char *instName = &s_SelectProfileInstName[0];

			if (model != NULL)
			{
				inst = INSTANCE_Birth3D(model, instName, obj->thread);

				if (inst != NULL)
				{
					struct InstDrawPerPlayer *idpp;
					int player;

					icon->inst = inst;
					slot = i % 3;

					inst->flags |= HIDE_MODEL | SCREENSPACE_INSTANCE;
					if (slot != 1)
					{
						inst->flags |= USE_SPECULAR_LIGHT;
					}

					idpp = INST_GETIDPP(inst);
					idpp[0].pushBuffer = &gGT->pushBuffer_UI;
					for (player = 1; player < gGT->numPlyrCurrGame; player++)
					{
						idpp[player].pushBuffer = NULL;
					}

					inst->colorRGBA = SelectProfile_LoadSave_Color(i, flags);
					inst->scale.x = data.MetaDataLoadSave[i].scale;
					inst->scale.y = data.MetaDataLoadSave[i].scale;
					inst->scale.z = data.MetaDataLoadSave[i].scale;

					icon->rot.x = 0;
					icon->rot.y = 0;
					icon->rot.z = data.spinOffset_LoadSave[slot];

					CTR_MatrixSetRotIdentity(&inst->matrix);
				}
			}
		}

		inst = icon->inst;
		if (inst != NULL)
		{
			inst->flags |= HIDE_MODEL;
		}
	}

}


void SelectProfile_Destroy(void)
{
	struct SelectProfileLoadSaveObj *obj;

	obj = (struct SelectProfileLoadSaveObj *)sdata->ptrLoadSaveObj;
	if (obj != NULL)
	{
		struct SelectProfileLoadSaveIcon *icon = obj->icons;
		int i;

		for (i = 0; i < 12; i++, icon++)
		{
			if (icon->inst != NULL)
			{
				INSTANCE_Death(icon->inst);
				icon->inst = NULL;
			}
		}

		

		obj->thread->funcThTick = NULL;
		obj->thread->flags |= THREAD_FLAG_DEAD;
		sdata->ptrLoadSaveObj = 0;
	}
}


void SelectProfile_AdvPickMode_MenuProc(struct RectMenu *menu)
{
	if (menu->funcState != RECTMENU_FUNC_STATE_INPUT)
	{
		SelectProfile_Init(menu->drawStyle);
		// A file being written out has no slot yet, so the marker follows what was
		// picked on the creation path rather than what any slot currently holds.
		SelectProfile_DrawAdvProfile(&sdata->advProgress, 0x92, 0x32, 0, 0, sdata->chaosSettings.armed, SELECT_PROFILE_DRAW_STYLE_GREEN);
		return;
	}

	s16 row = menu->rowSelected;

	if ((row >= 0) && (row < 3))
	{
		SelectProfile_ToggleMode((u16)menu->rowSelected | SELECT_PROFILE_SCREEN_GREEN_LOAD_SAVE);
		sdata->ptrDesiredMenu = &data.menuFourAdvProfiles;
		return;
	}

	if ((row == -1) || (row == 3))
	{
		RECTMENU_Hide(menu);
		SelectProfile_Destroy();
	}
}


void SelectProfile_DrawGhostProfile(struct GhostProfile *profile, int posX, int posY, u32 isHighlighted, int unused, u16 menuFlag, s16 isLoading,
                                    s16 isUnavailable)
{
	struct GameTracker *gGT = sdata->gGT;
	RECT profileRect;
	RECT innerRect;

	(void)unused;

	profileRect.x = posX;
	profileRect.y = posY;
	profileRect.w = 0xc8;
	profileRect.h = 0x29;

	innerRect.x = posX + 6;
	innerRect.y = posY + 3;
	innerRect.w = 0xbc;
	innerRect.h = 0x23;

	if (isUnavailable != 0)
	{
		DecalFont_DrawLine(sdata->lngStrings[LNG_NOT_AVAILABLE], posX + 0x64, posY + 0x11, FONT_SMALL, JUSTIFY_CENTER | SILVER);
		Color redColor;
		ColorCode_SetPacked(&redColor, (u32)sdata->redColor);
		CTR_Box_DrawClearBox(&innerRect, &redColor, ADD_DECAL, gGT->backBuffer->otMem.uiOT, &gGT->backBuffer->primMem);
	}

	if (profile != NULL)
	{
		Color iconColor;
		struct MetaDataLEV *mdLev = &data.metaDataLEV[profile->trackID];
		int iconID = data.MetaDataCharacters[profile->characterID].iconID;

		ColorCode_SetPacked(&iconColor, (u32)sdata->ghostIconColor);

		DecalFont_DrawLine(sdata->lngStrings[mdLev->name_LNG], posX + 0x64, posY + 0x1e, FONT_SMALL, JUSTIFY_CENTER | LIGHT_GREEN);
		DecalFont_DrawLine(RECTMENU_DrawTime(profile->trackTime), posX + 0x78, posY + 10, FONT_BIG, JUSTIFY_CENTER | PERIWINKLE);
		RECTMENU_DrawPolyGT4(gGT->ptrIcons[iconID], posX + 8, posY + 5, &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT, iconColor, iconColor, iconColor,
		                     iconColor, TRANS_50_DECAL, 0x1000);
	}
	else
	{
		int lngIndex = (isLoading != 0) ? 0x6c : 0xb5;
		int color = JUSTIFY_CENTER | ((isLoading != 0) ? PERIWINKLE : RED);

		DecalFont_DrawLine(sdata->lngStrings[lngIndex], posX + 0x64, posY + 0x11, FONT_SMALL, color);
	}

	if (isHighlighted != 0)
	{
		Color *highlight = ((menuFlag & SELECT_PROFILE_DRAW_STYLE_GREEN) != 0) ? &sdata->menuRowHighlight_Green : &sdata->menuRowHighlight_Normal;
		CTR_Box_DrawClearBox(&innerRect, highlight, TRANS_50_DECAL, gGT->backBuffer->otMem.uiOT, &gGT->backBuffer->primMem);
	}

	RECTMENU_DrawInnerRect(&profileRect, (s16)menuFlag, gGT->backBuffer->otMem.uiOT);
}


void SelectProfile_MuteCursors(void)
{
	data.menuFourAdvProfiles.state |= MUTE_SOUND_OF_MOVING_CURSOR;
	data.menuGhostSelection.state |= MUTE_SOUND_OF_MOVING_CURSOR;
	data.menuWarning2.state |= MUTE_SOUND_OF_MOVING_CURSOR;
}

void SelectProfile_UnMuteCursors(void)
{
	data.menuFourAdvProfiles.state &= ~MUTE_SOUND_OF_MOVING_CURSOR;
	data.menuGhostSelection.state &= ~MUTE_SOUND_OF_MOVING_CURSOR;
	data.menuWarning2.state &= ~MUTE_SOUND_OF_MOVING_CURSOR;
}


static s16 *SelectProfile_Mode(void)
{
	return &sdata->selectProfileState.mode;
}

static s16 *SelectProfile_TimerSaveComplete(void)
{
	return &sdata->selectProfileState.timerSaveComplete;
}

void SelectProfile_ToggleMode(u32 mode)
{
	sdata->memcardAction = mode & SELECT_PROFILE_ACTION_MASK;

	// High nibble selects the profile screen, low nibble selects the action.
	*SelectProfile_Mode() = mode & SELECT_PROFILE_SCREEN_MASK;
	sdata->selectProfileState.exitToPrevious = 0;
	sdata->selectProfileState.actionDone = 0;
	sdata->selectProfileState.overwritePrompt = 0;
	sdata->selectProfileState.timeoutPrompt = 0;
	*SelectProfile_TimerSaveComplete() = 0;

	SelectProfile_UnMuteCursors();

	data.menuFourAdvProfiles.drawStyle &= ~SELECT_PROFILE_DRAW_STYLE_GREEN;
	data.menuOverwriteAdv.drawStyle &= ~SELECT_PROFILE_DRAW_STYLE_GREEN;
	if (*SelectProfile_Mode() == SELECT_PROFILE_SCREEN_GREEN_LOAD_SAVE)
	{
		data.menuFourAdvProfiles.drawStyle |= SELECT_PROFILE_DRAW_STYLE_GREEN;
		data.menuOverwriteAdv.drawStyle |= SELECT_PROFILE_DRAW_STYLE_GREEN;
	}

	SelectProfile_Init(data.menuFourAdvProfiles.drawStyle);

	data.menuFourAdvProfiles.rowSelected = sdata->unk_8008d73C_relatedToRowHighlighted;
	sdata->selectProfileState.actionActive = 0;
}


u32 SelectProfile_InputLogic(struct RectMenu *menu, s16 numRows, u32 confirmFlags)
{
	u32 handled = 0;
	u32 tap = sdata->buttonTapPerPlayer[0];

	// D-Pad, Cross, Square, Triangle, Circle
	if ((tap & SELECT_PROFILE_INPUT_NAV_CONFIRM) == 0)
	{
		return 0;
	}

	if ((confirmFlags & 1) == 0)
	{
		u16 oldRow = menu->rowSelected;
		u16 nextRow = oldRow - 2;
		s16 selectedRow;

		if ((tap & BTN_UP) != 0)
		{
			menu->rowSelected = nextRow;
		}
		else
		{
			nextRow = oldRow + 2;

			if ((tap & BTN_DOWN) != 0)
			{
				menu->rowSelected = nextRow;
			}
			else
			{
				nextRow = oldRow ^ 1;

				if ((tap & (BTN_LEFT | BTN_RIGHT)) != 0)
				{
					menu->rowSelected = nextRow;
				}
			}
		}

		selectedRow = menu->rowSelected;
		if (selectedRow < 0)
		{
			menu->rowSelected = 0;
			selectedRow = menu->rowSelected;
		}

		if (numRows <= selectedRow)
		{
			menu->rowSelected = numRows - 1;
		}

		if ((u16)menu->rowSelected != oldRow)
		{
			OtherFX_Play(0, 1);
		}

		if (((tap & (BTN_CROSS_one | BTN_CIRCLE)) == 0) || ((numRows == 0) && (sdata->memcardAction != SELECT_PROFILE_ACTION_SAVE)))
		{
			if ((tap & (BTN_TRIANGLE | BTN_SQUARE_one)) != 0)
			{
				OtherFX_Play(2, 1);
				handled = 1;
				menu->rowSelected = -1;
			}
		}
		else
		{
			OtherFX_Play(1, 1);
			handled = 1;

			if (sdata->mcScreenText == MC_SCREEN_WARNING_UNFORMATTED)
			{
				menu->rowSelected = 0;
			}
		}
	}
	else
	{
		u32 cancel = (tap & (BTN_TRIANGLE | BTN_SQUARE_one)) != 0;

		if (cancel)
		{
			OtherFX_Play(2, 1);
			menu->rowSelected = -1;
		}

		handled = cancel;

		if (((confirmFlags & 2) != 0) && ((tap & (BTN_CROSS_one | BTN_CIRCLE)) != 0))
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

static s16 *SelectProfile_AllProfiles_Mode(void)
{
	return &sdata->selectProfileState.mode;
}

static s16 *SelectProfile_AllProfiles_ActionActive(void)
{
	return &sdata->selectProfileState.actionActive;
}

static s16 *SelectProfile_AllProfiles_ExitToPrevious(void)
{
	return &sdata->selectProfileState.exitToPrevious;
}

static s16 *SelectProfile_AllProfiles_ActionDone(void)
{
	return &sdata->selectProfileState.actionDone;
}

static s16 *SelectProfile_AllProfiles_OverwritePrompt(void)
{
	return &sdata->selectProfileState.overwritePrompt;
}

static s16 *SelectProfile_AllProfiles_TimeoutPrompt(void)
{
	return &sdata->selectProfileState.timeoutPrompt;
}

static s16 *SelectProfile_AllProfiles_TimerSaveComplete(void)
{
	return &sdata->selectProfileState.timerSaveComplete;
}

static struct MemcardData *SelectProfile_MemcardData(void)
{
	return (struct MemcardData *)sdata->ptrToMemcardBuffer2;
}

static struct MemcardProfile *SelectProfile_MemcardProfile(void)
{
	return &SelectProfile_MemcardData()->profile;
}

static int SelectProfile_IsGhostMode(void)
{
	return *SelectProfile_AllProfiles_Mode() == SELECT_PROFILE_SCREEN_GHOST;
}

static int SelectProfile_AdvProfileOccupied(int slot)
{
	return SelectProfile_MemcardProfile()->advProgress[slot].characterID >= 0;
}

static void SelectProfile_CopyGameProgressToCard(void)
{
	struct MemcardProfile *memcard = SelectProfile_MemcardProfile();

	RaceConfig_SaveGameOptions();
	GAMEPROG_SaveCupProgress();
	GAMEPROG_SyncGameAndCard(&memcard->gameSave.progress, &sdata->gameSave.progress);
	memcpy(&memcard->gameSave, &sdata->gameSave, sizeof(struct GameSave));
}

static int SelectProfile_IsValidCharacterID(s16 characterID)
{
	return (characterID >= 0) && (characterID < GAME_CHARACTER_COUNT);
}

static void SelectProfile_ApplyAdvMultiplayerCharacter(void)
{
	if (SelectProfile_IsValidCharacterID(sdata->advMultiplayer.characterID2))
	{
		data.characterIDs[1] = sdata->advMultiplayer.characterID2;
	}
}

// Whether a co-op entry describes a profile that plays with a partner. A save
// without the co-op settings has zeroes there, which no racer count falls into:
// such a profile plays alone, and so does any count this port did not write.
int SelectProfile_AdvMultiplayerIsCoop(const struct AdvMultiplayerSettings *settings)
{
	const s16 numPlayers = settings->numPlayers;

	return numPlayers > ADV_MULTIPLAYER_DEFAULT_PLAYERS && numPlayers <= ADV_MULTIPLAYER_NUM_PLAYERS_MAX;
}

// The partner of a co-op entry, or ADV_MULTIPLAYER_NO_CHARACTER when it has none.
// The partner is only read once the racer count says two players are there, so
// what an older save left in the entry is never taken for one.
s16 SelectProfile_AdvMultiplayerPartner(const struct AdvMultiplayerSettings *settings)
{
	if (!SelectProfile_AdvMultiplayerIsCoop(settings))
	{
		return ADV_MULTIPLAYER_NO_CHARACTER;
	}

	if (!SelectProfile_IsValidCharacterID(settings->characterID2))
	{
		return ADV_MULTIPLAYER_NO_CHARACTER;
	}

	return settings->characterID2;
}

// The co-op settings a profile shows. The profile in play shows its live settings,
// which are what a save of it is about to write, while every other profile shows
// what the card holds for its slot. The summary of the profile in play draws the
// live profile itself, so it is recognised by that and not by the slot it passes
// for the models of the row.
static const struct AdvMultiplayerSettings *SelectProfile_AdvSettingsToShow(const struct AdvProgress *adv, int slot)
{
	if (adv == &sdata->advProgress || (slot == sdata->advProfileIndex))
	{
		return &sdata->advMultiplayer;
	}

	return &SelectProfile_MemcardData()->extension.adventure[slot];
}

static void SelectProfile_LoadAdvMultiplayer(int slot)
{
	struct AdvMultiplayerSettings *saved = &SelectProfile_MemcardData()->extension.adventure[slot];
	s16 characterID2 = SelectProfile_AdvMultiplayerPartner(saved);
	for (int player = 0; player < 2; player++)
	{
		const u8 encoded = SelectProfile_MemcardData()->extension.engines[slot][player];
		const u8 choice = encoded & 0x0f;
		sdata->selectedEngineClass[player] = ((encoded & 0xf0) == 0xa0 && choice <= NUM_CLASSES) ? choice : 0;
	}

	GAMEPROG_ResetAdvMultiplayer(&sdata->advMultiplayer);

	if (!SelectProfile_AdvMultiplayerIsCoop(saved))
	{
		return;
	}

	sdata->advMultiplayer.numPlayers = saved->numPlayers;
	sdata->advMultiplayer.characterID2 = characterID2;
}

static void SelectProfile_SaveAdvMultiplayer(int slot)
{
	struct AdvMultiplayerSettings *saved = &SelectProfile_MemcardData()->extension.adventure[slot];

	*saved = sdata->advMultiplayer;
	for (int player = 0; player < 2; player++)
	{
		SelectProfile_MemcardData()->extension.engines[slot][player] = 0xa0 | sdata->selectedEngineClass[player];
	}
}

static void SelectProfile_ClearAdvMultiplayer(int slot)
{
	memset(&SelectProfile_MemcardData()->extension.adventure[slot], 0, sizeof(struct AdvMultiplayerSettings));
	memset(SelectProfile_MemcardData()->extension.engines[slot], 0, sizeof(SelectProfile_MemcardData()->extension.engines[slot]));
}

// Whether the Adventure file sitting in this slot of the card was created with
// Chaos. Read from the card extension rather than from sdata because the card
// screen draws all four files at once while only one of them is ever loaded.
static b32 SelectProfile_CardSlotIsChaos(int slot)
{
	if ((slot < 0) || (slot >= MEMCARD_ADV_PROFILE_COUNT))
	{
		return false;
	}

	return ChaosSave_IsArmed(&SelectProfile_MemcardData()->extension.chaos[slot]);
}

// Decode the tagged bitset into live settings.
static void SelectProfile_LoadChaosSettings(int slot)
{
	ChaosSave_Decode(&SelectProfile_MemcardData()->extension.chaos[slot], &sdata->chaosSettings);
}

// The settings ride along with the profile rather than having a writer of their
// own, for the same reason the racer count does. Nothing edits
// sdata->chaosSettings once a file exists, so an ordinary in-game save writes
// back exactly what it loaded and a configured file is never disturbed.
static void SelectProfile_SaveChaosSettings(int slot)
{
	ChaosSave_Encode(&sdata->chaosSettings, &SelectProfile_MemcardData()->extension.chaos[slot]);
}

static void SelectProfile_ClearChaosSettings(int slot)
{
	memset(&SelectProfile_MemcardData()->extension.chaos[slot], 0, sizeof(struct ChaosSettingsSaved));
}

static enum AdventureDifficulty SelectProfile_AdvDifficultyToShow(const struct AdvProgress *adv, int slot)
{
	if (adv == &sdata->advProgress)
	{
		return sdata->advDifficulty;
	}
	if (slot < 0 || slot >= MEMCARD_ADV_PROFILE_COUNT)
	{
		return ADV_DIFFICULTY_NORMAL;
	}
	return AdvDifficultySave_Decode(SelectProfile_MemcardData()->extension.difficulty[slot]);
}

static void SelectProfile_LoadAdvProfile(int slot)
{
	struct GameTracker *gGT = sdata->gGT;
	struct MemcardProfile *memcard = SelectProfile_MemcardProfile();

	GAMEPROG_SyncGameAndCard(&memcard->gameSave.progress, &sdata->gameSave.progress);
	sdata->advProgress = memcard->advProgress[slot];
	SelectProfile_LoadAdvMultiplayer(slot);
	SelectProfile_LoadChaosSettings(slot);
	sdata->advDifficulty = AdvDifficultySave_Decode(SelectProfile_MemcardData()->extension.difficulty[slot]);
	data.characterIDs[0] = sdata->advProgress.characterID;
	SelectProfile_ApplyAdvMultiplayerCharacter();
	memmove(gGT->prevNameEntered, sdata->advProgress.name, sizeof(gGT->prevNameEntered));
}

static void SelectProfile_SaveAdvProfile(int slot)
{
	struct MemcardProfile *memcard = SelectProfile_MemcardProfile();

	sdata->unk_8008d73C_relatedToRowHighlighted = slot;
	SelectProfile_CopyGameProgressToCard();
	memcard->advProgress[slot] = sdata->advProgress;
	SelectProfile_SaveAdvMultiplayer(slot);
	SelectProfile_SaveChaosSettings(slot);
	SelectProfile_MemcardData()->extension.difficulty[slot] = AdvDifficultySave_Encode(sdata->advDifficulty);
	MEMCARD_SetIcon(0);
	RefreshCard_StartMemcardAction(3);
	*(s16 *)&sdata->unk_memcardRelated_8008d928[0] = 1;
	*SelectProfile_AllProfiles_ActionActive() = 1;
}

static void SelectProfile_StartGhostSave(struct RectMenu *menu)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *driver = gGT->drivers[0];
	int time = SELECT_PROFILE_DEFAULT_GHOST_TIME;

	if (driver != NULL)
	{
		time = driver->timeElapsedInRace;
	}

	RefreshCard_GhostEncodeProfile(menu->rowSelected, data.characterIDs[0], gGT->levelID, time, gGT->prevNameEntered);

	sdata->ghostProfile_indexSave = menu->rowSelected;
	sdata->ghostProfile_rowSelect = -1;
	if (menu->rowSelected < sdata->numGhostProfilesSaved)
	{
		sdata->ghostProfile_rowSelect = menu->rowSelected;
	}

	MEMCARD_SetIcon(1);
	RefreshCard_StartMemcardAction(6);
	*SelectProfile_AllProfiles_ActionActive() = 1;
	gGT->gameModeEnd |= PLAYER_GHOST_BEAT;
}

static int SelectProfile_GhostRowCount(int *savedCount, b32 *canChooseEmptySlot)
{
	int count = sdata->numGhostProfilesSaved;

	if (count < 0)
	{
		count = 0;
	}

	if (count > SELECT_PROFILE_GHOST_SLOT_COUNT)
	{
		count = SELECT_PROFILE_GHOST_SLOT_COUNT;
	}

	*savedCount = count;

	if (sdata->memcardAction == SELECT_PROFILE_ACTION_SAVE)
	{
		*canChooseEmptySlot = sdata->memoryCard_SizeRemaining >= SELECT_PROFILE_GHOST_SAVE_BYTES;
		count += *canChooseEmptySlot;

		if (count > SELECT_PROFILE_GHOST_SLOT_COUNT)
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

static void SelectProfile_ClampRow(struct RectMenu *menu, int rowCount)
{
	if (menu->rowSelected < 0)
	{
		menu->rowSelected = 0;
	}

	if ((rowCount > 0) && (menu->rowSelected >= rowCount))
	{
		menu->rowSelected = rowCount - 1;
	}
}

static int SelectProfile_DisableAdvInputFlags(void)
{
	int flags = 0;

	if ((sdata->mcScreenText < MC_SCREEN_FORMATTING) ||
	    ((sdata->memoryCard_SizeRemaining < SELECT_PROFILE_ADV_SAVE_BYTES) && (*(s16 *)&sdata->unk_memcardRelated_8008d928[0] == 0) &&
	     (sdata->memcardAction == SELECT_PROFILE_ACTION_SAVE)))
	{
		flags = 1;
	}

	if (sdata->mcScreenText < MC_SCREEN_FORMATTING)
	{
		flags |= 2;
	}

	return flags;
}

static void SelectProfile_DrawGhostRows(struct RectMenu *menu, int rowCount, int savedCount, b32 canChooseEmptySlot, int color)
{
	int i;
	int lineGap;
	int yBase;
	int titleEndY;
	int subtitleVisible;
	int rowCountWithEmpty = sdata->numGhostProfilesSaved + canChooseEmptySlot;
	struct GhostProfile *profile = &sdata->ghostProfile_memcard[0];

	// NOTE(aalhendi): Retail tests the Adventure subtitle table here, but
	// draws the Ghost subtitle table below.
	subtitleVisible = strlen(sdata->lngStrings[data.lngStringsSaveLoadDelete[(sdata->memcardAction * 2) + 1]]) != 0;

	if (rowCount < 7)
	{
		lineGap = 0x10;
		yBase = 0x12;
		if (sdata->memcardAction != SELECT_PROFILE_ACTION_SAVE)
		{
			DecalFont_DrawMultiLine(sdata->lngStrings[LNG_INSERT_ANY_MEMORY_CARD_WITH_GHOST_DATA_IN], 0x100, 0xbe, 0x1ce, FONT_SMALL, JUSTIFY_CENTER | color);
		}
	}
	else
	{
		lineGap = 8;
		yBase = subtitleVisible ? 0xc : 0x12;
	}

	titleEndY = yBase + lineGap;

	DecalFont_DrawLine(sdata->lngStrings[data.lngIndex_LoadSave[sdata->memcardAction * 2]], 0x100, yBase, rowCount < 7 ? FONT_BIG : FONT_SMALL,
	                   JUSTIFY_CENTER | color);

	if (subtitleVisible != 0)
	{
		DecalFont_DrawLine(sdata->lngStrings[data.lngIndex_LoadSave[(sdata->memcardAction * 2) + 1]], 0x100, yBase + lineGap,
		                   rowCount < 7 ? FONT_BIG : FONT_SMALL, JUSTIFY_CENTER | color);
		titleEndY += lineGap;
	}

	for (i = 0; i < rowCountWithEmpty; i++)
	{
		int pair = i >> 1;
		int x;
		int y;
		int drawStyle = menu->drawStyle;
		int isWrongTrack = 0;

		if ((i == savedCount) && (i < rowCountWithEmpty))
		{
			profile = NULL;
		}

		if ((i < rowCount - 1) || ((i & 1) != 0))
		{
			x = ((i & 1) * 0xd4) + 0x2e;
		}
		else
		{
			x = 0x98;
		}

		y = titleEndY + 6 + (pair * ((rowCount > 6) ? 0x2b : 0x2f));

		if (rowCount > 6)
		{
			drawStyle |= 0x40;
		}

		// NOTE(aalhendi): Retail compares against GameTracker.currLEV
		// (0x1eb0). Ghost selection happens before QueueLoadTrack, so
		// levelID still refers to the previously loaded level.
		if ((profile != NULL) && (profile->trackID != sdata->gGT->currLEV))
		{
			isWrongTrack = sdata->memcardAction != SELECT_PROFILE_ACTION_SAVE;
		}

		SelectProfile_DrawGhostProfile(profile, x, y, i == menu->rowSelected, i, drawStyle, sdata->memcardAction == SELECT_PROFILE_ACTION_LOAD, isWrongTrack);

		if (profile != NULL)
		{
			profile++;
		}
	}
}

static void SelectProfile_DrawAdvRows(struct RectMenu *menu, int color)
{
	int i;
	int subtitleVisible = strlen(sdata->lngStrings[data.lngStringsSaveLoadDelete[(sdata->memcardAction * 2) + 1]]) != 0;
	struct MemcardProfile *memcard = SelectProfile_MemcardProfile();

	DecalFont_DrawLine(sdata->lngStrings[data.lngStringsSaveLoadDelete[sdata->memcardAction * 2]], 0x100, subtitleVisible ? 0x12 : 0x1a, FONT_BIG,
	                   JUSTIFY_CENTER | color);

	if (subtitleVisible != 0)
	{
		DecalFont_DrawLine(sdata->lngStrings[data.lngStringsSaveLoadDelete[(sdata->memcardAction * 2) + 1]], 0x100, 0x22, FONT_BIG, JUSTIFY_CENTER | color);
	}

	for (i = 0; i < 4; i++)
	{
		SelectProfile_DrawAdvProfile(&memcard->advProgress[i], ((i & 1) * 0xea) + 0x1a, ((i >> 1) * 0x43) + 0x3c, i == menu->rowSelected, i, SelectProfile_CardSlotIsChaos(i), menu->drawStyle);
	}

	if ((sdata->memcardAction == SELECT_PROFILE_ACTION_SAVE) && (sdata->boolMemcardDataValid != 0))
	{
		DecalFont_DrawLine(sdata->lngStrings[LNG_DATA_ON_MEMORY_CARD_IS_OUT_OF_DATE], 0x100, 0xc3, FONT_SMALL, JUSTIFY_CENTER | RED);
	}
}

static void SelectProfile_DrawOverwriteMenu(struct RectMenu *menu)
{
	s16 width = 0;
	struct RectMenu *overwriteMenu = SelectProfile_IsGhostMode() ? &data.menuOverwriteGhost : &data.menuOverwriteAdv;

	RECTMENU_GetWidth(overwriteMenu, &width, 1);
	RECTMENU_DrawSelf(overwriteMenu, 0, 0, width);

	if (SelectProfile_IsGhostMode())
	{
		SelectProfile_DrawGhostProfile(&sdata->ghostProfile_memcard[sdata->ghostProfile_rowSelect], 0x9c, 0x3c, 0, 0, menu->drawStyle, 0, 0);
	}
	else
	{
		SelectProfile_DrawAdvProfile(&SelectProfile_MemcardProfile()->advProgress[menu->rowSelected], 0x92, 0x3c, 0, menu->rowSelected, SelectProfile_CardSlotIsChaos(menu->rowSelected), menu->drawStyle);
	}
}

static int SelectProfile_ProcessOverwritePrompt(void)
{
	struct RectMenu *overwriteMenu = &data.menuOverwriteAdv;
	u32 tap = sdata->buttonTapPerPlayer[0];
	int confirm = 0;

	if ((tap & SELECT_PROFILE_INPUT_NAV_CONFIRM) == 0)
	{
		return 0;
	}

	if ((tap & BTN_UP) != 0)
	{
		if (overwriteMenu->rowSelected > 0)
		{
			OtherFX_Play(0, 1);
			overwriteMenu->rowSelected--;
		}
	}
	else if ((tap & BTN_DOWN) != 0)
	{
		if (overwriteMenu->rowSelected < 1)
		{
			OtherFX_Play(0, 1);
			overwriteMenu->rowSelected++;
		}
	}
	else if ((tap & (BTN_CROSS_one | BTN_CIRCLE | BTN_TRIANGLE | BTN_SQUARE_one)) != 0)
	{
		if ((tap & (BTN_CROSS_one | BTN_CIRCLE)) != 0)
		{
			OtherFX_Play(1, 1);
			confirm = overwriteMenu->rowSelected == 0;
		}
		else
		{
			OtherFX_Play(2, 1);
		}
		*SelectProfile_AllProfiles_OverwritePrompt() = 0;
	}

	RECTMENU_ClearInput();
	data.menuOverwriteAdv.rowSelected = overwriteMenu->rowSelected;
	data.menuOverwriteGhost.rowSelected = overwriteMenu->rowSelected;
	return confirm;
}

static void SelectProfile_StartLoadGhost(struct RectMenu *menu, int rowCount)
{
	if (menu->rowSelected >= rowCount - 1)
	{
		if (sdata->ptrGhostTapePlaying != NULL)
		{
			memset(sdata->ptrGhostTapePlaying, 0, 0x28);
		}

		*SelectProfile_AllProfiles_ActionActive() = 1;
		*SelectProfile_AllProfiles_ActionDone() = 1;
		sdata->boolError = 1;
		return;
	}

	// NOTE(aalhendi): Retail uses currLEV here; levelID is not updated to the
	// selected Time Trial track until the queued load starts.
	if (sdata->ghostProfile_memcard[menu->rowSelected].trackID == sdata->gGT->currLEV)
	{
		sdata->ghostProfile_indexLoad = menu->rowSelected;
		RefreshCard_StartMemcardAction(5);
		*SelectProfile_AllProfiles_ActionActive() = 1;
		return;
	}

	OtherFX_Play(5, 1);
}

static int SelectProfile_HandleSelection(struct RectMenu *menu, int rowCount)
{
	int mode = *SelectProfile_AllProfiles_Mode();
	b32 proceed = true;

	if (menu->rowSelected == -1)
	{
		*SelectProfile_AllProfiles_ActionActive() = 1;
		*SelectProfile_AllProfiles_ExitToPrevious() = 1;
		sdata->boolError = 1;
		return 0;
	}

	if (sdata->mcScreenText == MC_SCREEN_WARNING_NOCARD)
	{
		proceed = false;

		if (sdata->memcardAction == SELECT_PROFILE_ACTION_SAVE)
		{
			*SelectProfile_AllProfiles_ActionActive() = 1;
			*SelectProfile_AllProfiles_ActionDone() = 1;
			sdata->boolError = 1;
		}
		else if ((sdata->memcardAction == SELECT_PROFILE_ACTION_LOAD) && (mode == SELECT_PROFILE_SCREEN_GHOST))
		{
			proceed = true;
		}
	}

	if ((mode != SELECT_PROFILE_SCREEN_GHOST) && (sdata->mcScreenText != MC_SCREEN_WARNING_UNFORMATTED) &&
	    (sdata->memoryCard_SizeRemaining < SELECT_PROFILE_ADV_SAVE_BYTES) && (*(s16 *)&sdata->unk_memcardRelated_8008d928[0] == 0) &&
	    (sdata->memcardAction == SELECT_PROFILE_ACTION_SAVE))
	{
		proceed = false;
	}

	if (!proceed)
	{
		return 0;
	}

	if (sdata->mcScreenText == MC_SCREEN_WARNING_UNFORMATTED)
	{
		RefreshCard_StartMemcardAction(7);
		return 0;
	}

	if (((s16)CTR_ReadU16LE(&sdata->unk8008d95c) == 0) && ((s16)CTR_ReadU16LE(&sdata->unk_memcardRelated_8008d928[0]) == 0))
	{
		return 0;
	}

	if (sdata->memcardAction == SELECT_PROFILE_ACTION_SAVE)
	{
		if (mode == SELECT_PROFILE_SCREEN_GHOST)
		{
			if (menu->rowSelected < sdata->numGhostProfilesSaved)
			{
				data.menuOverwriteAdv.rowSelected = 1;
				data.menuOverwriteGhost.rowSelected = 1;
				*SelectProfile_AllProfiles_OverwritePrompt() = 1;
				sdata->ghostProfile_rowSelect = menu->rowSelected;
				return 0;
			}
		}
		else
		{
			if ((*SelectProfile_AllProfiles_TimeoutPrompt() == 0) && (sdata->mcScreenText == MC_SCREEN_ERROR_TIMEOUT))
			{
				*SelectProfile_AllProfiles_TimeoutPrompt() = 1;
				return 0;
			}

			if (SelectProfile_AdvProfileOccupied(menu->rowSelected) && (menu->rowSelected != sdata->advProfileIndex))
			{
				data.menuOverwriteAdv.rowSelected = 1;
				data.menuOverwriteGhost.rowSelected = 1;
				*SelectProfile_AllProfiles_OverwritePrompt() = 1;
				return 0;
			}
		}

		return 1;
	}

	if (sdata->memcardAction == SELECT_PROFILE_ACTION_LOAD)
	{
		if (mode == SELECT_PROFILE_SCREEN_GHOST)
		{
			SelectProfile_StartLoadGhost(menu, rowCount);
		}
		else if (sdata->mcScreenText == MC_SCREEN_ERROR_TIMEOUT)
		{
			*SelectProfile_AllProfiles_ActionActive() = 1;
			*SelectProfile_AllProfiles_ExitToPrevious() = 1;
		}
		else if (SelectProfile_AdvProfileOccupied(menu->rowSelected))
		{
			SelectProfile_LoadAdvProfile(menu->rowSelected);
			sdata->unk_8008d73C_relatedToRowHighlighted = menu->rowSelected;
			*SelectProfile_AllProfiles_ActionActive() = 1;
			*SelectProfile_AllProfiles_ActionDone() = 1;
			sdata->boolError = 1;
		}
		else
		{
			OtherFX_Play(5, 1);
		}
	}
	else if ((sdata->memcardAction == SELECT_PROFILE_ACTION_DELETE) && (mode != SELECT_PROFILE_SCREEN_GHOST) &&
	         SelectProfile_AdvProfileOccupied(menu->rowSelected))
	{
		GAMEPROG_NewProfile_InsideAdv(&SelectProfile_MemcardProfile()->advProgress[menu->rowSelected]);
		SelectProfile_ClearAdvMultiplayer(menu->rowSelected);
		SelectProfile_ClearChaosSettings(menu->rowSelected);
		SelectProfile_MemcardData()->extension.difficulty[menu->rowSelected] = ADV_DIFFICULTY_NORMAL;
		MEMCARD_SetIcon(0);
		RefreshCard_StartMemcardAction(3);
		*(s16 *)&sdata->unk_memcardRelated_8008d928[0] = 1;
		*SelectProfile_AllProfiles_ActionActive() = 1;
	}

	return 0;
}

static void SelectProfile_DrawMemcardMessage(int screen, int color, int menuFlag)
{
	int descriptor;
	int firstString;
	int multiLine;
	int i;

	if ((screen < 0) || (screen >= 10))
	{
		return;
	}

	if ((screen == MC_SCREEN_ERROR_NODATA) && (*SelectProfile_AllProfiles_Mode() == SELECT_PROFILE_SCREEN_SLOT_SELECTED))
	{
		return;
	}

	descriptor = data.messageScreens[screen];
	firstString = descriptor & 0xffff;
	multiLine = (descriptor >> 16) & 0xffff;

	if ((*SelectProfile_AllProfiles_ActionActive() != 0) && ((s16)CTR_ReadU16LE(&sdata->unk8008d964) != 0))
	{
		firstString = 0xffff;
	}

	if (firstString >= 0xffff)
	{
		return;
	}

	if (*SelectProfile_AllProfiles_ActionDone() != 0)
	{
		return;
	}

	if ((firstString == 0x10f) && (sdata->memcardAction == SELECT_PROFILE_ACTION_SAVE))
	{
		firstString = 0x106;
	}

	if ((*SelectProfile_AllProfiles_Mode() != SELECT_PROFILE_SCREEN_GHOST) && (screen == MC_SCREEN_ERROR_NODATA) && (sdata->boolMemcardDataValid != 0))
	{
		DecalFont_DrawLine(sdata->lngStrings[LNG_DATA_ON_MEMORY_CARD_IS_OUT_OF_DATE], 0x100, 0xc3, FONT_SMALL, JUSTIFY_CENTER | RED);
	}

	if ((sdata->memcardAction == SELECT_PROFILE_ACTION_DELETE) && (firstString == 0xea))
	{
		firstString = 0xfc;
	}

	if (multiLine == 0)
	{
		DecalFont_DrawLine(sdata->lngStrings[firstString], 0x108, 0x12, FONT_BIG, JUSTIFY_CENTER | color);
	}
	else
	{
		for (i = 0; i < 9; i++)
		{
			char *line = sdata->lngStrings[firstString + i];

			if (strlen(line) != 0)
			{
				int font = (i == 0) ? FONT_BIG : FONT_SMALL;
				int y = (i == 0) ? 0x26 : 0x2e + (i * (data.font_charPixHeight[FONT_SMALL] + 2));
				int lineColor = JUSTIFY_CENTER | color;

				if (((sdata->frameCounter & 4) == 0) && (i == 0))
				{
					lineColor = JUSTIFY_CENTER | RED;
				}

				DecalFont_DrawLine(line, 0x100, y, font, lineColor);
			}
		}
	}

	RECTMENU_DrawInnerRect((RECT *)&sdata->unk_BeforeTokenMenu[0], menuFlag, sdata->gGT->backBuffer->otMem.uiOT);
}

static void SelectProfile_DrawAll(struct RectMenu *menu, int rowCount, int savedGhostCount, b32 canChooseEmptySlot, int color, b32 doSave)
{
	int screenOverride = sdata->mcScreenText;
	b32 canDrawProfiles = (*SelectProfile_AllProfiles_ActionActive() == 0) && ((s16)CTR_ReadU16LE(&sdata->unk8008d95c) != 0) &&
	                      (((s16)CTR_ReadU16LE(&sdata->unk_memcardRelated_8008d928[0]) != 0) || (sdata->mcScreenText == MC_SCREEN_NULL));

	if ((sdata->memcardAction == SELECT_PROFILE_ACTION_LOAD) && SelectProfile_IsGhostMode() &&
	    ((sdata->mcScreenText == MC_SCREEN_ERROR_NODATA) || (sdata->mcScreenText == MC_SCREEN_WARNING_NOCARD)) && (rowCount != 0))
	{
		canDrawProfiles = true;
	}

	if (sdata->memcardAction == SELECT_PROFILE_ACTION_SAVE)
	{
		if ((sdata->mcScreenText == MC_SCREEN_NULL) || (sdata->mcScreenText == MC_SCREEN_ERROR_NODATA))
		{
			canDrawProfiles = false;

			if (doSave)
			{
				screenOverride = MC_SCREEN_SAVING;
			}
			else if (SelectProfile_IsGhostMode())
			{
				canDrawProfiles = true;
				if ((sdata->memoryCard_SizeRemaining < SELECT_PROFILE_GHOST_SAVE_BYTES) && (savedGhostCount == 0))
				{
					canDrawProfiles = false;
					screenOverride = MC_SCREEN_ERROR_FULL;
				}
			}
			else
			{
				canDrawProfiles = true;
				if ((sdata->memoryCard_SizeRemaining < SELECT_PROFILE_ADV_SAVE_BYTES) && (*(s16 *)&sdata->unk_memcardRelated_8008d928[0] == 0))
				{
					canDrawProfiles = false;
					screenOverride = MC_SCREEN_ERROR_FULL;
				}
			}
		}

		if ((sdata->mcScreenText == MC_SCREEN_ERROR_TIMEOUT) && (*SelectProfile_AllProfiles_TimeoutPrompt() != 0))
		{
			canDrawProfiles = true;
		}
	}

	SelectProfile_Init(menu->drawStyle);

	if (canDrawProfiles && (*SelectProfile_AllProfiles_ActionActive() == 0))
	{
		if (SelectProfile_IsGhostMode())
		{
			if (*SelectProfile_AllProfiles_OverwritePrompt() == 0)
			{
				SelectProfile_DrawGhostRows(menu, rowCount, savedGhostCount, canChooseEmptySlot, color);
			}
			else
			{
				SelectProfile_DrawOverwriteMenu(menu);
			}
		}
		else
		{
			if (*SelectProfile_AllProfiles_OverwritePrompt() == 0)
			{
				SelectProfile_DrawAdvRows(menu, color);
			}
			else
			{
				SelectProfile_DrawOverwriteMenu(menu);
			}
		}
	}
	else
	{
		*SelectProfile_AllProfiles_OverwritePrompt() = 0;

		if ((*SelectProfile_AllProfiles_ActionActive() != 0) && ((s16)CTR_ReadU16LE(&sdata->unk8008d964) != 0) &&
		    (*SelectProfile_AllProfiles_ExitToPrevious() == 0) && (*SelectProfile_AllProfiles_ActionDone() == 0) &&
		    (*SelectProfile_AllProfiles_TimerSaveComplete() != 0))
		{
			int saveColor = ((sdata->frameCounter & 4) == 0) ? (JUSTIFY_CENTER | ORANGE) : (JUSTIFY_CENTER | WHITE);
			DecalFont_DrawLine(sdata->lngStrings[LNG_SAVE_COMPLETED], 0x108, 0x64, FONT_BIG, saveColor);
		}
		else
		{
			SelectProfile_DrawMemcardMessage(screenOverride, color, menu->drawStyle);
		}
	}
}

static int SelectProfile_ShouldFinalize(void)
{
	if (*SelectProfile_AllProfiles_ActionActive() == 0)
	{
		return 0;
	}

	if (sdata->boolError == 0)
	{
		return 0;
	}

	if (((s16)CTR_ReadU16LE(&sdata->unk8008d964) == 0) && (*SelectProfile_AllProfiles_ExitToPrevious() == 0) && (*SelectProfile_AllProfiles_ActionDone() == 0))
	{
		return 0;
	}

	if ((*SelectProfile_AllProfiles_ActionActive() != 0) && ((s16)CTR_ReadU16LE(&sdata->unk8008d964) != 0) &&
	    (*SelectProfile_AllProfiles_ExitToPrevious() == 0) && (*SelectProfile_AllProfiles_ActionDone() == 0) &&
	    (*SelectProfile_AllProfiles_TimerSaveComplete() != 0))
	{
		(*SelectProfile_AllProfiles_TimerSaveComplete())--;
		return 0;
	}

	return 1;
}

static void SelectProfile_FinalizeGhost(struct RectMenu *menu)
{
	SelectProfile_Destroy();

	if (sdata->memcardAction == SELECT_PROFILE_ACTION_SAVE)
	{
		sdata->ptrDesiredMenu = (*SelectProfile_AllProfiles_ExitToPrevious() != 0) ? &menu224 : &menu224NoSave;
		return;
	}

	if (*SelectProfile_AllProfiles_ExitToPrevious() != 0)
	{
		GhostTape_Destroy();
		sdata->ptrDesiredMenu = MM_TrackSelect_GetMenuPtr();
		MM_TrackSelect_Init();
		return;
	}

	if (sdata->ptrGhostTapePlaying != NULL)
	{
		data.characterIDs[1] = sdata->ptrGhostTapePlaying->characterID;
	}

	sdata->ptrDesiredMenu = QueueLoadTrack_GetMenuPtr();
	(void)menu;
}

static void SelectProfile_FinalizeAdventure(struct RectMenu *menu)
{
	int mode = *SelectProfile_AllProfiles_Mode();
	struct GameTracker *gGT = sdata->gGT;

	if (mode == SELECT_PROFILE_SCREEN_GREEN_LOAD_SAVE)
	{
		if (*SelectProfile_AllProfiles_ExitToPrevious() == 0)
		{
			sdata->advProfileIndex = menu->rowSelected;
		}

		if ((*SelectProfile_AllProfiles_ExitToPrevious() == 0) && (sdata->memcardAction == SELECT_PROFILE_ACTION_LOAD))
		{
			GAMEPROG_AdvPercent(&sdata->advProgress);
			sdata->ptrDesiredMenu = &data.menuQueueLoadHub;
			// NOTE(aalhendi): Retail 0x8004a8a0-0x8004a8c4 queues through currLEV.
			gGT->currLEV = sdata->advProgress.HubLevYouSavedOn;
			data.menuGreenLoadSave.rowSelected = 3;
		}
		else
		{
			sdata->ptrDesiredMenu = &data.menuGreenLoadSave;
			data.menuGreenLoadSave.rowSelected = 3;
		}
		return;
	}

	if (mode == SELECT_PROFILE_SCREEN_ADV_NEW)
	{
		if (*SelectProfile_AllProfiles_ExitToPrevious() != 0)
		{
			sdata->ptrDesiredMenu = &data.menuSubmitName;
			SubmitName_RestoreName(0);
			return;
		}

		sdata->advProfileIndex = menu->rowSelected;
		// NOTE(aalhendi): Retail 0x8004a75c-0x8004a778 queues new Adventure through currLEV.
		gGT->currLEV = N_SANITY_BEACH;
		Garage_Leave();
		sdata->ptrDesiredMenu = QueueLoadTrack_GetMenuPtr();
		return;
	}

	if (mode == SELECT_PROFILE_SCREEN_ADV_LOAD)
	{
		if (*SelectProfile_AllProfiles_ExitToPrevious() != 0)
		{
			MM_JumpTo_Title_Returning();
			struct RectMenu *advMenu = MM_AdvNewLoad_GetMenuPtr();
			advMenu->state &= ~ONLY_DRAW_TITLE;
			return;
		}

		sdata->advProfileIndex = menu->rowSelected;
		// NOTE(aalhendi): Retail 0x8004a848-0x8004a864 stores saved/fallback hub in currLEV.
		if (sdata->advProgress.HubLevYouSavedOn != 0)
		{
			gGT->currLEV = sdata->advProgress.HubLevYouSavedOn;
		}
		else
		{
			gGT->currLEV = N_SANITY_BEACH;
		}
		memmove(gGT->prevNameEntered, sdata->advProgress.name, sizeof(gGT->prevNameEntered));
		memmove(gGT->currNameEntered, sdata->advProgress.name, sizeof(gGT->currNameEntered));
		sdata->ptrDesiredMenu = QueueLoadTrack_GetMenuPtr();
		return;
	}

	if (mode == SELECT_PROFILE_SCREEN_SLOT_SELECTED)
	{
		SelectProfile_Destroy();
		if (sdata->boolSaveCupProgress == 0)
		{
			gGT->gameModeEnd &= ~(NEW_NAME | NEW_HIGH_SCORE);
		}
		else if (*SelectProfile_AllProfiles_ExitToPrevious() != 0)
		{
			sdata->ptrDesiredMenu = &data.menuSaveGame;
			return;
		}

		RECTMENU_Hide(menu);
	}
}

// NOTE(aalhendi): Partial retail audit only; this large structured rewrite is not fully ASM-stamped.
void SelectProfile_AllProfiles_MenuProc(struct RectMenu *menu)
{
	int color = ((menu->drawStyle & SELECT_PROFILE_DRAW_STYLE_GREEN) != 0) ? LIGHT_GREEN : ORANGE;
	int savedGhostCount = sdata->numGhostProfilesSaved;
	b32 canChooseEmptySlot = false;
	int rowCount = SelectProfile_IsGhostMode() ? SelectProfile_GhostRowCount(&savedGhostCount, &canChooseEmptySlot) : 4;
	b32 handled = false;
	b32 doSave = false;

	if (sdata->mcScreenText == MC_SCREEN_WARNING_NOCARD)
	{
		*SelectProfile_AllProfiles_OverwritePrompt() = 0;
	}

	if (sdata->mcScreenText < MC_SCREEN_FORMATTING)
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
		doSave = SelectProfile_ProcessOverwritePrompt();
		goto draw_and_finish;
	}

	if (SelectProfile_IsGhostMode())
	{
		SelectProfile_ClampRow(menu, rowCount);
	}

	if (*SelectProfile_AllProfiles_ActionActive() == 0)
	{
		if (SelectProfile_IsGhostMode())
		{
			handled = SelectProfile_InputLogic(menu, rowCount, 0);
		}
		else if (*SelectProfile_AllProfiles_Mode() == SELECT_PROFILE_SCREEN_SLOT_SELECTED)
		{
			if (sdata->mcScreenText == MC_SCREEN_WARNING_NOCARD)
			{
				if ((sdata->buttonTapPerPlayer[0] & (BTN_CROSS_one | BTN_CIRCLE)) != 0)
				{
					OtherFX_Play(1, 1);
					if (sdata->boolSaveCupProgress == 0)
					{
						MainGameEnd_SoloRaceSaveHighScore();
					}
					RECTMENU_ClearInput();
					*SelectProfile_AllProfiles_ActionActive() = 1;
					*SelectProfile_AllProfiles_ExitToPrevious() = 1;
					sdata->boolError = 1;
					handled = 0;
				}
			}
			else if (sdata->mcScreenText == MC_SCREEN_WARNING_UNFORMATTED)
			{
				if ((sdata->buttonTapPerPlayer[0] & BTN_CIRCLE) != 0)
				{
					RECTMENU_ClearInput();
					OtherFX_Play(1, 1);
					RefreshCard_StartMemcardAction(7);
				}
			}
			else if (((sdata->memoryCard_SizeRemaining >= SELECT_PROFILE_ADV_SAVE_BYTES) || ((s16)CTR_ReadU16LE(&sdata->unk_memcardRelated_8008d928[0]) != 0) ||
			          (sdata->memcardAction != SELECT_PROFILE_ACTION_SAVE)) &&
			         (((s16)CTR_ReadU16LE(&sdata->unk8008d95c) != 0) || ((s16)CTR_ReadU16LE(&sdata->unk_memcardRelated_8008d928[0]) != 0)))
			{
				if (sdata->boolSaveCupProgress == 0)
				{
					MainGameEnd_SoloRaceSaveHighScore();
				}

				SelectProfile_CopyGameProgressToCard();
				MEMCARD_SetIcon(0);
				RefreshCard_StartMemcardAction(3);
				*(s16 *)&sdata->unk_memcardRelated_8008d928[0] = 1;
				*SelectProfile_AllProfiles_ActionActive() = 1;
				handled = 0;
			}
		}
		else
		{
			handled = SelectProfile_InputLogic(menu, rowCount, SelectProfile_DisableAdvInputFlags());
		}

		if (handled != 0)
		{
			doSave = SelectProfile_HandleSelection(menu, rowCount);
		}
	}

draw_and_finish:
	if ((sdata->mcScreenText == MC_SCREEN_ERROR_TIMEOUT) && (*SelectProfile_AllProfiles_ExitToPrevious() == 0) &&
	    (*SelectProfile_AllProfiles_ActionDone() == 0))
	{
		*SelectProfile_AllProfiles_ActionActive() = 0;
		*SelectProfile_AllProfiles_OverwritePrompt() = 0;
	}

	if (doSave != 0)
	{
		*SelectProfile_AllProfiles_TimeoutPrompt() = 0;
		if (SelectProfile_IsGhostMode())
		{
			SelectProfile_StartGhostSave(menu);
		}
		else
		{
			SelectProfile_SaveAdvProfile(menu->rowSelected);
		}

		*SelectProfile_AllProfiles_TimerSaveComplete() = 0x3c;
	}

	if (menu->funcState == RECTMENU_FUNC_STATE_UPDATE)
	{
		SelectProfile_DrawAll(menu, rowCount, savedGhostCount, canChooseEmptySlot, color, doSave);
	}

	if (SelectProfile_ShouldFinalize())
	{
		SelectProfile_InitAndDestroy();
		RefreshCard_StopMemcardAction();

		if (*SelectProfile_AllProfiles_Mode() == SELECT_PROFILE_SCREEN_GHOST)
		{
			SelectProfile_FinalizeGhost(menu);
		}
		else
		{
			SelectProfile_FinalizeAdventure(menu);
		}
	}
}


void SelectProfile_InitAndDestroy(void)
{
	SelectProfile_Init(data.menuFourAdvProfiles.drawStyle);
	SelectProfile_Destroy();
}
