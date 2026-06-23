#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80047da8-0x80047dfc.
void SelectProfile_QueueLoadHub_MenuProc(struct RectMenu *menu)
{
	struct GameTracker *gGT = sdata->gGT;

	// NOTE(aalhendi): Retail stores 0x27 before LOAD_LevelFile records prevLEV.
	gGT->levelID = MAIN_MENU_LEVEL;

	data.characterIDs[0] = sdata->advProgress.characterID;
	MainRaceTrack_RequestLoad(gGT->currLEV);
	RECTMENU_Hide(menu);
	return;
}


struct SelectProfileLoadSaveIcon
{
	struct Instance *inst;
	SVec3 rot;
	s16 padding;
};

struct SelectProfileLoadSaveObj
{
	struct Thread *thread;
	struct SelectProfileLoadSaveIcon *icons;
};

CTR_STATIC_ASSERT_LAYOUT(sizeof(struct SelectProfileLoadSaveIcon) == 0xc);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct SelectProfileLoadSaveObj) == 0x8);

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80047dfc-0x80047f20 for the retail path.
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

#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Menu-storage can keep this thread alive when the
		// load/save icon models were not published for the current level.
		if (inst == NULL)
			continue;
#endif

		ConvertRotToMatrix(&inst->matrix, &icon->rot);

		if (slot != 1)
		{
			Vector_SpecLightSpin3D(inst, &icon->rot.x, &data.MetaDataLoadSave[i].vec3_specular_inverted);
		}
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80047f20-0x80047fb8.
void SelectProfile_PrintInteger(int value, int posX, int posY, b32 usePaddedFormat, int color)
{
	char text[64];
	char *format;

	if (usePaddedFormat == 1)
		format = &sdata->stringFormat1[0];
	else
		format = &sdata->stringFormat2[0];

	sprintf(text, format, value);
	DecalFont_DrawLine(text, posX, posY, FONT_BIG, color);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80047fb8-0x80047fd8.
int SelectProfile_UI_ConvertX(int param_1, int param_2)
{
	param_2 = (param_1 - 0x100) * param_2;
	if (param_2 < 0)
	{
		param_2 = param_2 + 0xff;
	}
	return param_2 >> 8;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80047fd8-0x80047ff8.
int SelectProfile_UI_ConvertY(int param_1, int param_2)
{
	param_2 = (param_1 + -0x6c) * param_2;
	if (param_2 < 0)
	{
		param_2 = param_2 + 0xff;
	}
	return param_2 >> 8;
}


static void SelectProfile_DrawAdvProfile_UpdateIcon(struct SelectProfileLoadSaveObj *obj, int index, int posX, int posY)
{
	struct Instance *inst = obj->icons[index].inst;

#if defined(CTR_NATIVE)
	if (inst == NULL)
		return;
#endif

	inst->matrix.t[0] = SelectProfile_UI_ConvertX(posX, 0x100);
	inst->matrix.t[1] = SelectProfile_UI_ConvertY(posY, 0x100);
	inst->matrix.t[2] = 0x100;
	inst->flags &= ~HIDE_MODEL;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80047ff8-0x800485a8.
void SelectProfile_DrawAdvProfile(struct AdvProgress *adv, int posX, int posY, s16 isHighlighted, s16 slotIndex, u16 menuFlag)
{
	struct GameTracker *gGT = sdata->gGT;
	RECT profileRect;

	int iconColor;
	int numberColor;
	int emptyColor;
	int nameColor;
	int percentColor;

	if ((menuFlag & 0x10) != 0)
	{
		iconColor = sdata->greenColor;
		numberColor = 0x1d;
		emptyColor = 0x1e;
		nameColor = 0x1d;
		percentColor = 0x1d;
	}
	else
	{
		iconColor = sdata->greyColor;
		numberColor = 0;
		emptyColor = 3;
		nameColor = 1;
		percentColor = 4;
	}

	slotIndex *= 3;
	GAMEPROG_AdvPercent(adv);

	if (adv->characterID < 0)
	{
		DecalFont_DrawLine(sdata->lngStrings[LNG_EMPTY], posX + 0x6c, posY + 0x17, FONT_BIG, emptyColor | 0xffff8000);
	}
	else
	{
		int profileTextColor = numberColor | 0x4000;
		int characterID = adv->characterID;
		int iconID = data.MetaDataCharacters[characterID].iconID;
		struct SelectProfileLoadSaveObj *obj = (struct SelectProfileLoadSaveObj *)sdata->ptrLoadSaveObj;

		RECTMENU_DrawPolyGT4(gGT->ptrIcons[iconID], posX + 10, posY + 6, &gGT->backBuffer->primMem, gGT->backBuffer->otMem.uiOT, iconColor, iconColor,
		                     iconColor, iconColor, 1, 0x1000);

		DecalFont_DrawLine(adv->name, posX + 0x6c, posY + 0x29, FONT_BIG, nameColor | 0xffff8000);

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
		Color *highlightColor = ((menuFlag & 0x10) != 0) ? &sdata->menuRowHighlight_Green : &sdata->menuRowHighlight_Normal;

		highlightRect.x = posX + 6;
		highlightRect.y = posY + 4;
		highlightRect.w = 0xd0;
		highlightRect.h = 0x35;

		CTR_Box_DrawClearBox(&highlightRect, highlightColor, 1, &gGT->backBuffer->otMem.uiOT[3]);
	}

	RECTMENU_DrawInnerRect(&profileRect, (s16)menuFlag, &gGT->backBuffer->otMem.uiOT[3]);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800485a8-0x800485cc.
void SelectProfile_GetTrackID()
{
	data.menuGreenLoadSave.rowSelected = 1;
	sdata->advProgress.HubLevYouSavedOn = sdata->gGT->levelID;
}


#ifdef CTR_NATIVE
// NOTE(aalhendi): Retail stores these debug names in EXE RDATA; native does not
// expose the full retail RDATA struct.
static char s_SelectProfileThreadName[] = "LoadSave";
static char s_SelectProfileInstName[] = "loadsave";
#endif

static u32 SelectProfile_LoadSave_Color(int index, u32 flags)
{
	u32 red = (u8)data.MetaDataLoadSave[index].r;
	u32 green = (u8)data.MetaDataLoadSave[index].g;
	u32 blue = (u8)data.MetaDataLoadSave[index].b;

	if ((flags & 0x10) != 0)
	{
		red >>= 1;
		blue >>= 1;
	}

	return (red << 0x14) | (green << 0xc) | (blue << 4);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800485cc-0x800488e0.
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
		sdata->ptrLoadSaveObj = (int)obj;
		obj->icons = (struct SelectProfileLoadSaveIcon *)&sdata->LoadSaveData[0];
		memset(obj->icons, 0, sizeof(sdata->LoadSaveData));

		if (obj == NULL)
			return;

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
#ifdef CTR_NATIVE
			char *instName = &s_SelectProfileInstName[0];
#else
			char *instName = rdata.s_loadsave;
#endif

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

					*(int *)&inst->matrix.m[0][0] = 0x1000;
					*(int *)&inst->matrix.m[0][2] = 0;
					*(int *)&inst->matrix.m[1][1] = 0x1000;
					*(int *)&inst->matrix.m[2][0] = 0;
					inst->matrix.m[2][2] = 0x1000;
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


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800488e0-0x80048960 for the retail path.
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
#if defined(CTR_NATIVE)
				icon->inst = NULL;
#endif
			}
		}

#if defined(CTR_NATIVE)
		obj->thread->funcThTick = NULL;
#endif
		obj->thread->flags |= THREAD_FLAG_DEAD;
		sdata->ptrLoadSaveObj = 0;
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80048960-0x80048a30.
void SelectProfile_AdvPickMode_MenuProc(struct RectMenu *menu)
{
	if (menu->unk1e != 0)
	{
		SelectProfile_Init(menu->drawStyle);
		SelectProfile_DrawAdvProfile(&sdata->advProgress, 0x92, 0x32, 0, 0, 0x10);
		return;
	}

	s16 row = menu->rowSelected;

	if ((row >= 0) && (row < 3))
	{
		SelectProfile_ToggleMode((u16)menu->rowSelected | 0x20);
		sdata->ptrDesiredMenu = &data.menuFourAdvProfiles;
		return;
	}

	if ((row == -1) || (row == 3))
	{
		RECTMENU_Hide(menu);
		SelectProfile_Destroy();
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80048a30-0x80048da0.
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
		DecalFont_DrawLine(sdata->lngStrings[LNG_NOT_AVAILABLE], posX + 0x64, posY + 0x11, FONT_SMALL, 0xffff8016);
		CTR_Box_DrawClearBox(&innerRect, (Color *)&sdata->redColor, ADD_DECAL, gGT->backBuffer->otMem.uiOT);
	}

	if (profile != NULL)
	{
		struct MetaDataLEV *mdLev = &data.metaDataLEV[profile->trackID];
		int iconID = data.MetaDataCharacters[profile->characterID].iconID;

		DecalFont_DrawLine(sdata->lngStrings[mdLev->name_LNG], posX + 0x64, posY + 0x1e, FONT_SMALL, 0xffff801d);
		DecalFont_DrawLine(RECTMENU_DrawTime(profile->trackTime), posX + 0x78, posY + 10, FONT_BIG, 0xffff8001);
		RECTMENU_DrawPolyGT4(gGT->ptrIcons[iconID], posX + 8, posY + 5, &gGT->backBuffer->primMem, gGT->backBuffer->otMem.uiOT, sdata->ghostIconColor,
		                     sdata->ghostIconColor, sdata->ghostIconColor, sdata->ghostIconColor, TRANS_50_DECAL, 0x1000);
	}
	else
	{
		int lngIndex = (isLoading != 0) ? 0x6c : 0xb5;
		int color = (isLoading != 0) ? 0xffff8001 : 0xffff8003;

		DecalFont_DrawLine(sdata->lngStrings[lngIndex], posX + 0x64, posY + 0x11, FONT_SMALL, color);
	}

	if (isHighlighted != 0)
	{
		Color *highlight = ((menuFlag & 0x10) != 0) ? &sdata->menuRowHighlight_Green : &sdata->menuRowHighlight_Normal;
		CTR_Box_DrawClearBox(&innerRect, highlight, TRANS_50_DECAL, gGT->backBuffer->otMem.uiOT);
	}

	RECTMENU_DrawInnerRect(&profileRect, (s16)menuFlag, gGT->backBuffer->otMem.uiOT);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80048da0-0x80048de4.
void SelectProfile_MuteCursors(void)
{
	data.menuFourAdvProfiles.state |= MUTE_SOUND_OF_MOVING_CURSOR;
	data.menuGhostSelection.state |= MUTE_SOUND_OF_MOVING_CURSOR;
	data.menuWarning2.state |= MUTE_SOUND_OF_MOVING_CURSOR;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80048de4-0x80048e2c.
void SelectProfile_UnMuteCursors(void)
{
	data.menuFourAdvProfiles.state &= ~MUTE_SOUND_OF_MOVING_CURSOR;
	data.menuGhostSelection.state &= ~MUTE_SOUND_OF_MOVING_CURSOR;
	data.menuWarning2.state &= ~MUTE_SOUND_OF_MOVING_CURSOR;
}


static s16 *SelectProfile_Mode(void)
{
	return (s16 *)&sdata->data10_bbb[0];
}

static s16 *SelectProfile_TimerSaveComplete(void)
{
	return (s16 *)&sdata->data10_bbb[12];
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80048e2c-0x80048edc.
void SelectProfile_ToggleMode(u32 mode)
{
	sdata->memcardAction = mode & 0xf;

	// 0x00 AdvNew, 0x10 AdvLoad, 0x20 green load/save, 0x30 ghost, 0x40 slot selected.
	*SelectProfile_Mode() = mode & 0xf0;
	*(s16 *)&sdata->data10_bbb[4] = 0;
	*(s16 *)&sdata->data10_bbb[6] = 0;
	*(s16 *)&sdata->data10_bbb[8] = 0;
	*(s16 *)&sdata->data10_bbb[10] = 0;
	*SelectProfile_TimerSaveComplete() = 0;

	SelectProfile_UnMuteCursors();

	data.menuFourAdvProfiles.drawStyle &= ~0x10;
	data.menuOverwriteAdv.drawStyle &= ~0x10;
	if (*SelectProfile_Mode() == 0x20)
	{
		data.menuFourAdvProfiles.drawStyle |= 0x10;
		data.menuOverwriteAdv.drawStyle |= 0x10;
	}

	SelectProfile_Init(data.menuFourAdvProfiles.drawStyle);

	data.menuFourAdvProfiles.rowSelected = sdata->unk_8008d73C_relatedToRowHighlighted;
	*(s16 *)&sdata->data10_bbb[2] = 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80048f0c-0x800490c4
u32 SelectProfile_InputLogic(struct RectMenu *menu, s16 numRows, u32 confirmFlags)
{
	u32 handled = 0;
	u32 tap = sdata->buttonTapPerPlayer[0];

	// D-Pad, Cross, Square, Triangle, Circle
	if ((tap & 0x4007f) == 0)
		return 0;

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
					menu->rowSelected = nextRow;
			}
		}

		selectedRow = menu->rowSelected;
		if (selectedRow < 0)
		{
			menu->rowSelected = 0;
			selectedRow = menu->rowSelected;
		}

		if (numRows <= selectedRow)
			menu->rowSelected = numRows - 1;

		if ((u16)menu->rowSelected != oldRow)
			OtherFX_Play(0, 1);

		if (((tap & (BTN_CROSS | BTN_CIRCLE)) == 0) || ((numRows == 0) && (sdata->memcardAction != 1)))
		{
			if ((tap & (BTN_TRIANGLE | BTN_SQUARE)) != 0)
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
				menu->rowSelected = 0;
		}
	}
	else
	{
		u32 cancel = (tap & (BTN_TRIANGLE | BTN_SQUARE)) != 0;

		if (cancel)
		{
			OtherFX_Play(2, 1);
			menu->rowSelected = -1;
		}

		handled = cancel;

		if (((confirmFlags & 2) != 0) && ((tap & (BTN_CROSS | BTN_CIRCLE)) != 0))
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
	return (s16 *)&sdata->data10_bbb[0];
}

static s16 *SelectProfile_AllProfiles_ActionActive(void)
{
	return (s16 *)&sdata->data10_bbb[2];
}

static s16 *SelectProfile_AllProfiles_ExitToPrevious(void)
{
	return (s16 *)&sdata->data10_bbb[4];
}

static s16 *SelectProfile_AllProfiles_ActionDone(void)
{
	return (s16 *)&sdata->data10_bbb[6];
}

static s16 *SelectProfile_AllProfiles_OverwritePrompt(void)
{
	return (s16 *)&sdata->data10_bbb[8];
}

static s16 *SelectProfile_AllProfiles_TimeoutPrompt(void)
{
	return (s16 *)&sdata->data10_bbb[10];
}

static s16 *SelectProfile_AllProfiles_TimerSaveComplete(void)
{
	return (s16 *)&sdata->data10_bbb[12];
}

static struct MemcardProfile *SelectProfile_MemcardProfile(void)
{
	return (struct MemcardProfile *)sdata->ptrToMemcardBuffer2;
}

static int SelectProfile_IsGhostMode(void)
{
	return *SelectProfile_AllProfiles_Mode() == 0x30;
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
	GAMEPROG_SyncGameAndCard(&memcard->gameProgress, &sdata->gameProgress);
	memcpy(&memcard->gameProgress, &sdata->gameProgress, sizeof(struct GameProgress) + sizeof(struct GameOptions));
}

static void SelectProfile_LoadAdvProfile(int slot)
{
	struct GameTracker *gGT = sdata->gGT;
	struct MemcardProfile *memcard = SelectProfile_MemcardProfile();

	GAMEPROG_SyncGameAndCard(&memcard->gameProgress, &sdata->gameProgress);
	sdata->advProgress = memcard->advProgress[slot];
	data.characterIDs[0] = sdata->advProgress.characterID;
	memmove(gGT->prevNameEntered, sdata->advProgress.name, sizeof(gGT->prevNameEntered));
}

static void SelectProfile_SaveAdvProfile(int slot)
{
	struct MemcardProfile *memcard = SelectProfile_MemcardProfile();

	sdata->unk_8008d73C_relatedToRowHighlighted = slot;
	SelectProfile_CopyGameProgressToCard();
	memcard->advProgress[slot] = sdata->advProgress;
	MEMCARD_SetIcon(0);
	RefreshCard_StartMemcardAction(3);
	*(s16 *)&sdata->unk_memcardRelated_8008d928[0] = 1;
	*SelectProfile_AllProfiles_ActionActive() = 1;
}

static void SelectProfile_StartGhostSave(struct RectMenu *menu)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *driver = gGT->drivers[0];
	int time = 0x8ca00;

	if (driver != NULL)
		time = driver->timeElapsedInRace;

	RefreshCard_GhostEncodeProfile(menu->rowSelected, data.characterIDs[0], gGT->levelID, time, gGT->prevNameEntered);

	sdata->ghostProfile_indexSave = menu->rowSelected;
	sdata->ghostProfile_rowSelect = -1;
	if (menu->rowSelected < sdata->numGhostProfilesSaved)
		sdata->ghostProfile_rowSelect = menu->rowSelected;

	MEMCARD_SetIcon(1);
	RefreshCard_StartMemcardAction(6);
	*SelectProfile_AllProfiles_ActionActive() = 1;
	gGT->gameModeEnd |= PLAYER_GHOST_BEAT;
}

static int SelectProfile_GhostRowCount(int *savedCount, b32 *canChooseEmptySlot)
{
	int count = sdata->numGhostProfilesSaved;

	if (count < 0)
		count = 0;

	if (count > 7)
		count = 7;

	*savedCount = count;

	if (sdata->memcardAction == 1)
	{
		*canChooseEmptySlot = sdata->memoryCard_SizeRemaining >= 0x3e00;
		count += *canChooseEmptySlot;

		if (count > 7)
		{
			count = 7;
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
		menu->rowSelected = 0;

	if ((rowCount > 0) && (menu->rowSelected >= rowCount))
		menu->rowSelected = rowCount - 1;
}

static int SelectProfile_DisableAdvInputFlags(void)
{
	int flags = 0;

	if ((sdata->mcScreenText < MC_SCREEN_FORMATTING) ||
	    ((sdata->memoryCard_SizeRemaining < 0x1680) && (*(s16 *)&sdata->unk_memcardRelated_8008d928[0] == 0) && (sdata->memcardAction == 1)))
		flags = 1;

	if (sdata->mcScreenText < MC_SCREEN_FORMATTING)
		flags |= 2;

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

	subtitleVisible = strlen(sdata->lngStrings[data.lngIndex_LoadSave[(sdata->memcardAction * 2) + 1]]) != 0;

	if (rowCount < 7)
	{
		lineGap = 0x10;
		yBase = 0x12;
		if (sdata->memcardAction != 1)
			DecalFont_DrawMultiLine(sdata->lngStrings[LNG_INSERT_ANY_MEMORY_CARD_WITH_GHOST_DATA_IN], 0x100, 0xbe, 0x1ce, FONT_SMALL, color | 0xffff8000);
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
			profile = NULL;

		if ((i < rowCount - 1) || ((i & 1) != 0))
			x = ((i & 1) * 0xd4) + 0x2e;
		else
			x = 0x98;

		y = titleEndY + 6 + (pair * ((rowCount > 6) ? 0x2b : 0x2f));

		if (rowCount > 6)
			drawStyle |= 0x40;

		// NOTE(aalhendi): Retail compares against GameTracker.currLEV
		// (0x1eb0). Ghost selection happens before QueueLoadTrack, so
		// levelID still refers to the previously loaded level.
		if ((profile != NULL) && (profile->trackID != sdata->gGT->currLEV))
			isWrongTrack = sdata->memcardAction != 1;

		SelectProfile_DrawGhostProfile(profile, x, y, i == menu->rowSelected, i, drawStyle, sdata->memcardAction == 0, isWrongTrack);

		if (profile != NULL)
			profile++;
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
		SelectProfile_DrawAdvProfile(&memcard->advProgress[i], ((i & 1) * 0xea) + 0x1a, ((i >> 1) * 0x43) + 0x3c, i == menu->rowSelected, i, menu->drawStyle);
	}

	if ((sdata->memcardAction == 1) && (sdata->boolMemcardDataValid != 0))
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
		SelectProfile_DrawAdvProfile(&SelectProfile_MemcardProfile()->advProgress[menu->rowSelected], 0x92, 0x3c, 0, menu->rowSelected, menu->drawStyle);
	}
}

static int SelectProfile_ProcessOverwritePrompt(void)
{
	struct RectMenu *overwriteMenu = &data.menuOverwriteAdv;
	u32 tap = sdata->buttonTapPerPlayer[0];
	int confirm = 0;

	if ((tap & 0x4007f) == 0)
		return 0;

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
	else if ((tap & (BTN_CROSS | BTN_CIRCLE | BTN_TRIANGLE | BTN_SQUARE)) != 0)
	{
		if ((tap & (BTN_CROSS | BTN_CIRCLE)) != 0)
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

static void SelectProfile_HandleNoCardOrSpace(struct RectMenu *menu)
{
	int mode = *SelectProfile_AllProfiles_Mode();

	if (sdata->mcScreenText == MC_SCREEN_WARNING_NOCARD)
	{
		if (sdata->memcardAction == 1)
		{
			*SelectProfile_AllProfiles_ActionActive() = sdata->memcardAction;
			*SelectProfile_AllProfiles_ActionDone() = sdata->memcardAction;
			sdata->boolError = 1;
		}
		else if ((sdata->memcardAction == 0) && (mode == 0x30))
		{
			*SelectProfile_AllProfiles_ActionActive() = 1;
			*SelectProfile_AllProfiles_ActionDone() = 1;
			sdata->boolError = 1;
		}
	}

	if ((sdata->mcScreenText != MC_SCREEN_WARNING_UNFORMATTED) && (sdata->memoryCard_SizeRemaining < 0x1680) &&
	    (*(s16 *)&sdata->unk_memcardRelated_8008d928[0] == 0) && (sdata->memcardAction == 1))
	{
		*SelectProfile_AllProfiles_ActionActive() = 1;
		*SelectProfile_AllProfiles_ActionDone() = 1;
		sdata->boolError = 1;
	}

	if (menu->rowSelected == -1)
	{
		*SelectProfile_AllProfiles_ActionActive() = 1;
		*SelectProfile_AllProfiles_ExitToPrevious() = 1;
		sdata->boolError = 1;
	}
}

static void SelectProfile_StartLoadGhost(struct RectMenu *menu, int rowCount)
{
	if (menu->rowSelected >= rowCount - 1)
	{
		if (sdata->ptrGhostTapePlaying != NULL)
			memset(sdata->ptrGhostTapePlaying, 0, 0x28);

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

	if (menu->rowSelected == -1)
	{
		*SelectProfile_AllProfiles_ActionActive() = 1;
		*SelectProfile_AllProfiles_ExitToPrevious() = 1;
		sdata->boolError = 1;
		return 0;
	}

	if (sdata->mcScreenText == MC_SCREEN_WARNING_UNFORMATTED)
	{
		RefreshCard_StartMemcardAction(7);
		return 0;
	}

	if ((*(s16 *)&sdata->unk8008d95c == 0) && (*(s16 *)&sdata->unk_memcardRelated_8008d928[0] == 0))
		return 0;

	if (sdata->memcardAction == 1)
	{
		if (mode == 0x30)
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

	if (sdata->memcardAction == 0)
	{
		if (mode == 0x30)
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
	else if ((sdata->memcardAction == 2) && (mode != 0x30) && SelectProfile_AdvProfileOccupied(menu->rowSelected))
	{
		GAMEPROG_NewProfile_InsideAdv(&SelectProfile_MemcardProfile()->advProgress[menu->rowSelected]);
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
		return;

	if ((screen == MC_SCREEN_ERROR_NODATA) && (*SelectProfile_AllProfiles_Mode() == 0x40))
		return;

	descriptor = data.messageScreens[screen];
	firstString = descriptor & 0xffff;
	multiLine = (descriptor >> 16) & 0xffff;

	if ((*SelectProfile_AllProfiles_ActionActive() != 0) && (*(s16 *)&sdata->unk8008d964 != 0))
		firstString = 0xffff;

	if (firstString >= 0xffff)
		return;

	if ((firstString == 0x10f) && (sdata->memcardAction == 1))
		firstString = 0x106;

	if ((sdata->memcardAction == 2) && (firstString == 0xea))
		firstString = 0xfc;

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
				int y = (i == 0) ? 0x26 : 0x2e + (i * 10);
				int lineColor = color | 0xffff8000;

				if (((sdata->frameCounter & 4) == 0) && (i == 0))
					lineColor = RED | 0xffff8000;

				DecalFont_DrawLine(line, 0x100, y, font, lineColor);
			}
		}
	}

	RECTMENU_DrawInnerRect((RECT *)&sdata->unk_BeforeTokenMenu[0], menuFlag, sdata->gGT->backBuffer->otMem.uiOT);
}

static void SelectProfile_DrawAll(struct RectMenu *menu, int rowCount, int savedGhostCount, b32 canChooseEmptySlot, int color)
{
	b32 canDrawProfiles =
	    (*SelectProfile_AllProfiles_ActionActive() == 0) &&
	    ((*(s16 *)&sdata->unk8008d95c != 0) || (*(s16 *)&sdata->unk_memcardRelated_8008d928[0] != 0) || (sdata->mcScreenText == MC_SCREEN_NULL));

	if ((sdata->memcardAction == 0) && SelectProfile_IsGhostMode() &&
	    ((sdata->mcScreenText == MC_SCREEN_ERROR_NODATA) || (sdata->mcScreenText == MC_SCREEN_WARNING_NOCARD)) && (rowCount != 0))
		canDrawProfiles = true;

	if (sdata->memcardAction == 1)
	{
		if ((sdata->mcScreenText == MC_SCREEN_NULL) || (sdata->mcScreenText == MC_SCREEN_ERROR_NODATA))
		{
			canDrawProfiles = SelectProfile_IsGhostMode()
			                      ? !((sdata->memoryCard_SizeRemaining < 0x3e00) && (savedGhostCount == 0))
			                      : !((sdata->memoryCard_SizeRemaining < 0x1680) && (*(s16 *)&sdata->unk_memcardRelated_8008d928[0] == 0));
		}

		if ((sdata->mcScreenText == MC_SCREEN_ERROR_TIMEOUT) && (*SelectProfile_AllProfiles_TimeoutPrompt() != 0))
			canDrawProfiles = true;
	}

	SelectProfile_Init(menu->drawStyle);

	if (canDrawProfiles && (*SelectProfile_AllProfiles_ActionActive() == 0))
	{
		if (SelectProfile_IsGhostMode())
		{
			if (*SelectProfile_AllProfiles_OverwritePrompt() == 0)
				SelectProfile_DrawGhostRows(menu, rowCount, savedGhostCount, canChooseEmptySlot, color);
			else
				SelectProfile_DrawOverwriteMenu(menu);
		}
		else
		{
			if (*SelectProfile_AllProfiles_OverwritePrompt() == 0)
				SelectProfile_DrawAdvRows(menu, color);
			else
				SelectProfile_DrawOverwriteMenu(menu);
		}
	}
	else
	{
		*SelectProfile_AllProfiles_OverwritePrompt() = 0;

		if ((*SelectProfile_AllProfiles_ActionActive() != 0) && (*(s16 *)&sdata->unk8008d964 != 0) && (*SelectProfile_AllProfiles_ExitToPrevious() == 0) &&
		    (*SelectProfile_AllProfiles_ActionDone() == 0) && (*SelectProfile_AllProfiles_TimerSaveComplete() != 0))
		{
			int saveColor = ((sdata->frameCounter & 4) == 0) ? (JUSTIFY_CENTER | ORANGE) : (JUSTIFY_CENTER | WHITE);
			DecalFont_DrawLine(sdata->lngStrings[LNG_SAVE_COMPLETED], 0x108, 0x64, FONT_BIG, saveColor);
		}
		else
		{
			SelectProfile_DrawMemcardMessage(sdata->mcScreenText, color, menu->drawStyle);
		}
	}
}

static int SelectProfile_ShouldFinalize(void)
{
	if (*SelectProfile_AllProfiles_ActionActive() == 0)
		return 0;

	if (sdata->boolError == 0)
		return 0;

	if ((*(s16 *)&sdata->unk8008d964 == 0) && (*SelectProfile_AllProfiles_ExitToPrevious() == 0) && (*SelectProfile_AllProfiles_ActionDone() == 0))
		return 0;

	if ((*SelectProfile_AllProfiles_ActionActive() != 0) && (*(s16 *)&sdata->unk8008d964 != 0) && (*SelectProfile_AllProfiles_ExitToPrevious() == 0) &&
	    (*SelectProfile_AllProfiles_ActionDone() == 0) && (*SelectProfile_AllProfiles_TimerSaveComplete() != 0))
	{
		(*SelectProfile_AllProfiles_TimerSaveComplete())--;
		return 0;
	}

	return 1;
}

static void SelectProfile_FinalizeGhost(struct RectMenu *menu)
{
	SelectProfile_Destroy();

	if (sdata->memcardAction == 1)
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
		data.characterIDs[1] = sdata->ptrGhostTapePlaying->characterID;

	sdata->ptrDesiredMenu = QueueLoadTrack_GetMenuPtr();
	(void)menu;
}

static void SelectProfile_FinalizeAdventure(struct RectMenu *menu)
{
	int mode = *SelectProfile_AllProfiles_Mode();
	struct GameTracker *gGT = sdata->gGT;

	if (mode == 0x20)
	{
		if ((*SelectProfile_AllProfiles_ExitToPrevious() == 0) && (sdata->memcardAction == 0))
		{
			sdata->advProfileIndex = menu->rowSelected;
			GAMEPROG_AdvPercent(&sdata->advProgress);
			sdata->ptrDesiredMenu = &data.menuQueueLoadHub;
			// NOTE(aalhendi): Retail 0x8004a8a0-0x8004a8c4 queues through currLEV.
			gGT->currLEV = sdata->advProgress.HubLevYouSavedOn;
			data.menuQueueLoadHub.rowSelected = 3;
		}
		else
		{
			sdata->ptrDesiredMenu = &data.menuGreenLoadSave;
			data.menuGreenLoadSave.rowSelected = 3;
		}
		return;
	}

	if (mode == 0)
	{
		if (*SelectProfile_AllProfiles_ExitToPrevious() != 0)
		{
			sdata->ptrDesiredMenu = &data.menuSubmitName;
			SubmitName_RestoreName(0);
			return;
		}

		sdata->advProfileIndex = menu->rowSelected;
		// NOTE(aalhendi): Retail 0x8004a75c-0x8004a778 queues new Adventure through currLEV.
		gGT->currLEV = 0x1a;
		Garage_Leave();
		sdata->ptrDesiredMenu = QueueLoadTrack_GetMenuPtr();
		return;
	}

	if (mode == 0x10)
	{
		if (*SelectProfile_AllProfiles_ExitToPrevious() != 0)
		{
			struct RectMenu *advMenu;

			MM_JumpTo_Title_Returning();
			advMenu = MM_AdvNewLoad_GetMenuPtr();
			advMenu->state &= ~4;
			return;
		}

		sdata->advProfileIndex = menu->rowSelected;
		// NOTE(aalhendi): Retail 0x8004a848-0x8004a864 stores saved/fallback hub in currLEV.
		if (sdata->advProgress.HubLevYouSavedOn != 0)
			gGT->currLEV = sdata->advProgress.HubLevYouSavedOn;
		else
			gGT->currLEV = 0x1a;
		memmove(gGT->prevNameEntered, sdata->advProgress.name, sizeof(gGT->prevNameEntered));
		memmove(gGT->currNameEntered, sdata->advProgress.name, sizeof(gGT->currNameEntered));
		sdata->ptrDesiredMenu = QueueLoadTrack_GetMenuPtr();
		return;
	}

	if (mode == 0x40)
	{
		SelectProfile_Destroy();
		if (sdata->boolSaveCupProgress == 0)
		{
			gGT->gameModeEnd &= ~(NEW_NAME | NEW_HIGH_SCORE);
		}
		else if (*SelectProfile_AllProfiles_ExitToPrevious() != 0)
		{
			sdata->ptrDesiredMenu = &data.menuWarning2;
			return;
		}

		RECTMENU_Hide(menu);
	}
}

// NOTE(aalhendi): Partial retail audit only; this large structured rewrite is not fully ASM-stamped.
void SelectProfile_AllProfiles_MenuProc(struct RectMenu *menu)
{
	int color = ((menu->drawStyle & 0x10) != 0) ? LIGHT_GREEN : ORANGE;
	int savedGhostCount = sdata->numGhostProfilesSaved;
	b32 canChooseEmptySlot = false;
	int rowCount = SelectProfile_IsGhostMode() ? SelectProfile_GhostRowCount(&savedGhostCount, &canChooseEmptySlot) : 4;
	b32 handled = false;
	b32 doSave = false;

	if (sdata->mcScreenText == MC_SCREEN_WARNING_NOCARD)
		*SelectProfile_AllProfiles_OverwritePrompt() = 0;

	if (sdata->mcScreenText < MC_SCREEN_FORMATTING)
		*SelectProfile_AllProfiles_ActionActive() = 0;

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
		SelectProfile_ClampRow(menu, rowCount);

	if (*SelectProfile_AllProfiles_ActionActive() == 0)
	{
		if (SelectProfile_IsGhostMode())
		{
			handled = SelectProfile_InputLogic(menu, rowCount, 0);
		}
		else if (*SelectProfile_AllProfiles_Mode() == 0x40)
		{
			if (sdata->mcScreenText == MC_SCREEN_WARNING_NOCARD)
			{
				if ((sdata->buttonTapPerPlayer[0] & (BTN_CROSS | BTN_CIRCLE)) != 0)
				{
					OtherFX_Play(1, 1);
					if (sdata->boolSaveCupProgress == 0)
						MainGameEnd_SoloRaceSaveHighScore();
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
			else if (((sdata->memoryCard_SizeRemaining >= 0x1680) || (*(s16 *)&sdata->unk_memcardRelated_8008d928[0] != 0) || (sdata->memcardAction != 1)) &&
			         ((*(s16 *)&sdata->unk8008d95c != 0) || (*(s16 *)&sdata->unk_memcardRelated_8008d928[0] != 0)))
			{
				if (sdata->boolSaveCupProgress == 0)
					MainGameEnd_SoloRaceSaveHighScore();

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
			SelectProfile_HandleNoCardOrSpace(menu);
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
			SelectProfile_StartGhostSave(menu);
		else
			SelectProfile_SaveAdvProfile(menu->rowSelected);

		*SelectProfile_AllProfiles_TimerSaveComplete() = 0x3c;
	}

	if (menu->unk1e == 1)
		SelectProfile_DrawAll(menu, rowCount, savedGhostCount, canChooseEmptySlot, color);

	if (SelectProfile_ShouldFinalize())
	{
		SelectProfile_InitAndDestroy();
		RefreshCard_StopMemcardAction();

		if (*SelectProfile_AllProfiles_Mode() == 0x30)
			SelectProfile_FinalizeGhost(menu);
		else
			SelectProfile_FinalizeAdventure(menu);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80048edc-0x80048f0c.
void SelectProfile_InitAndDestroy(void)
{
	SelectProfile_Init(data.menuFourAdvProfiles.drawStyle);
	SelectProfile_Destroy();
}
