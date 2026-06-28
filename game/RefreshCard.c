#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800469f0-0x80046a74.
int RefreshCard_BoolGhostForLEV(u16 trackID)
{
	int i;
	int count = 0;
	int numGhosts = *(s16 *)&sdata->numGhostProfilesSaved;
	s16 levelID = trackID;

	for (i = 0; i < numGhosts; i++)
	{
		if (sdata->ghostProfile_memcard[i].trackID == levelID)
		{
			count++;
		}
	}

	return (s16)count;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80046a74-0x80046a90.
void RefreshCard_Unknown1(void)
{
	sdata->memcardUnk1 = (sdata->memcardUnk1 | 6) & ~8;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80046a90-0x80046b1c.
int RefreshCard_GetResult(int result)
{
	s16 result16 = result;

	if (result16 == 8)
	{
		if ((sdata->memcardUnk1 & 6) != 0)
		{
			return 1;
		}
	}

	if ((sdata->memcardUnk1 & 6) != 0)
	{
		return 0;
	}

	if (sdata->frame3_memcardAction != sdata->frame4_memcardAction)
	{
		return 0;
	}

	if (sdata->frame3_memcardSlot != sdata->frame4_memcardSlot)
	{
		return 0;
	}

	return sdata->desired_memcardResult == result16;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80046b60-0x80046bc0.
u32 RefreshCard_GhostEncodeByte(int currByte)
{
	s16 byte = currByte;

	if (byte < 10)
	{
		return (currByte + '0') & 0xff;
	}

	if (byte < 0x24)
	{
		return (currByte + 0x37) & 0xff;
	}

	if (byte < 0x3e)
	{
		return (currByte + 0x3d) & 0xff;
	}

	if (byte == 0x3e)
	{
		return '-';
	}

	return '_';
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80046b1c-0x80046b60.
void RefreshCard_NextMemcardAction(int slot, int action, char *fileName, char *fileIconHeader, struct GhostHeader *ptrGhostHeader, int fileSize)
{
	sdata->frame4_memcardAction = action;
	sdata->frame2_memcardAction = action;
	sdata->frame4_memcardSlot = slot;
	sdata->frame2_memcardSlot = slot;
	sdata->ghostProfile_fileName = fileName;
	sdata->ghostProfile_fileIconHeader = fileIconHeader;
	sdata->ghostProfile_ptrGhostHeader = ptrGhostHeader;
	sdata->ghostProfile_size3E00 = fileSize;
	sdata->memcardUnk1 &= ~8;
}


static int RefreshCard_GhostProfileNameExists(char *profileName)
{
	int i;

	for (i = 0; i < sdata->numGhostProfilesSaved; i++)
	{
		if (strcmp(sdata->ghostProfile_memcard[i].profile_name, profileName) == 0)
		{
			return 1;
		}
	}

	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80046c30-0x80047034.
void RefreshCard_GhostEncodeProfile(u32 slotIndex, u16 characterID, u16 levelID, int time, char *name)
{
	char description[0x80];
	char scrambled[0x80] = {0};
	s32 characterID32 = (s16)characterID;
	u32 packed;
	int isUnique;

	do
	{
		isUnique = 1;

		if (time > 0x8c9ff)
		{
			time = 0x8c9ff;
		}

		packed = (u32)characterID32 | ((u32)((s32)(s16)levelID << 4)) | ((u32)time << 9) | (slotIndex << 0x1d);
		data.s_BASCUS_94426G_Question[13] = RefreshCard_GhostEncodeByte(packed & 0x3f);
		data.s_BASCUS_94426G_Question[14] = RefreshCard_GhostEncodeByte((packed >> 6) & 0x3f);
		data.s_BASCUS_94426G_Question[15] = RefreshCard_GhostEncodeByte((packed >> 0xc) & 0x3f);
		data.s_BASCUS_94426G_Question[16] = RefreshCard_GhostEncodeByte((packed >> 0x12) & 0x3f);
		data.s_BASCUS_94426G_Question[17] = RefreshCard_GhostEncodeByte((packed >> 0x18) & 0x3f);
		data.s_BASCUS_94426G_Question[18] = RefreshCard_GhostEncodeByte(packed >> 0x1e);
		data.s_BASCUS_94426G_Question[19] = '\0';

		if (RefreshCard_GhostProfileNameExists(data.s_BASCUS_94426G_Question) != 0)
		{
			isUnique = 0;
		}

		slotIndex = (slotIndex + 1) & 7;
	} while (isUnique == 0);

	description[0] = '\0';

	strcat(&description[strlen(description)], sdata->lngStrings[data.metaDataLEV[(s16)levelID].name_LNG]);
	strcat(description, sdata->strcatData1_colon);
	strcat(&description[strlen(description)], sdata->lngStrings[data.MetaDataCharacters[(s16)characterID].name_LNG_long]);
	strcat(description, sdata->strcatData1_colon);
	strcat(description, (char *)RECTMENU_DrawTime(time));

	CTR_ScrambleGhostString(scrambled, description);
	memcpy(sdata->memcardIcon_HeaderGHOST, scrambled, 0x3e);

	struct GhostProfile *profile = &sdata->ghostProfile_current;
	memcpy(profile->profile_name, data.s_BASCUS_94426G_Question, sizeof(data.s_BASCUS_94426G_Question));
	profile->profile_name[sizeof(data.s_BASCUS_94426G_Question)] = data.s_BASCUS_94426G_Star[0];

	memcpy(profile->SubmitName_name, name, sizeof(profile->SubmitName_name));

	*(u8 *)&profile->alwaysOne = 1;
	profile->trackID = levelID;
	profile->characterID = characterID;
	*(s16 *)&profile->memcardProfileIndex = slotIndex;
	profile->trackTime = time;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80046bc0-0x80046c30.
int RefreshCard_GhostDecodeByte(int value)
{
	u8 byte = value;

	if (byte == '-')
	{
		return 0x3e;
	}

	if (byte == '_')
	{
		return 0x3f;
	}

	if (byte < ':')
	{
		return byte - '0';
	}

	if (byte < '[')
	{
		return (s16)(byte - 0x37);
	}

	return (s16)(byte - 0x3d);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80047034-0x80047198.
void RefreshCard_GhostDecodeProfile(struct GhostProfile *profile, char *fileName)
{
	int packed;

	packed = (s16)RefreshCard_GhostDecodeByte(fileName[13]);
	packed |= (s16)RefreshCard_GhostDecodeByte(fileName[14]) << 6;
	packed |= (s16)RefreshCard_GhostDecodeByte(fileName[15]) << 12;
	packed |= RefreshCard_GhostDecodeByte(fileName[16]) << 18;
	packed |= RefreshCard_GhostDecodeByte(fileName[17]) << 24;
	packed |= RefreshCard_GhostDecodeByte(fileName[18]) << 30;

	profile->characterID = packed & 0xf;
	profile->trackID = (packed >> 4) & 0x1f;
	profile->trackTime = (packed >> 9) & 0xfffff;
	profile->memcardProfileIndex = (u32)packed >> 29;

	*(u8 *)&profile->alwaysOne = 0;
	memcpy(profile->profile_name, fileName, sizeof(profile->profile_name));
	*((u8 *)&profile->trackID + 1) = 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80047198-0x800471ac.
void RefreshCard_StartMemcardAction(int action)
{
	sdata->mcStart = action;
	sdata->unk8008d964 = 0;
	sdata->boolError = 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800471ac-0x800471c4.
void RefreshCard_StopMemcardAction(void)
{
	sdata->unk8008d964 = 1;
	sdata->mcStart = 2;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800471c4-0x800471e8.
void RefreshCard_SetScreenText(int screenText)
{
	sdata->mcScreenText = screenText;
	RefreshCard_Unknown1();
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800471e8-0x80047224.
void RefreshCard_Unknown2(void)
{
	if (*(s16 *)&sdata->boolAdvProfilesChecked == 0)
	{
		GAMEPROG_InitFullMemcard((struct MemcardProfile *)sdata->ptrToMemcardBuffer1);
		*(s16 *)&sdata->boolAdvProfilesChecked = 1;
	}

	*(s16 *)&sdata->unk8008d95c = 1;
	*(s16 *)&sdata->unk_memcardRelated_8008d928[0] = 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80047224-0x80047230.
void RefreshCard_GetNumGhostsTotal(void)
{
	*(s16 *)&sdata->numGhostProfilesSaved = 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80047230-0x800472d0.
void RefreshCard_GameProgressAndOptions(void)
{
	struct MemcardProfile *memcard;

	*(s16 *)&sdata->unk8008d95c = 1;
	*(s16 *)&sdata->unk_memcardRelated_8008d928[0] = 1;
	*(s16 *)&sdata->advProfileIndex = -1;

	memcard = (struct MemcardProfile *)sdata->ptrToMemcardBuffer1;

	GAMEPROG_SyncGameAndCard(&memcard->gameProgress, &sdata->gameProgress);
	memcpy(&sdata->gameProgress, &memcard->gameProgress, sizeof(struct GameProgress) + sizeof(struct GameOptions));
	RaceConfig_LoadGameOptions();
}


static void RefreshCard_QueueGetInfo(void)
{
	RefreshCard_NextMemcardAction(0, MC_ACTION_GetInfo, data.s_BASCUS_94426_SLOTS, NULL, NULL, 0);
}

static void RefreshCard_QueueMainLoad(void)
{
	RefreshCard_NextMemcardAction(0, MC_ACTION_Load, data.s_BASCUS_94426_SLOTS, NULL, (struct GhostHeader *)sdata->ptrToMemcardBuffer1, 0x1680);
}

static void RefreshCard_QueueMainSave(void)
{
	RefreshCard_NextMemcardAction(0, MC_ACTION_Save, data.s_BASCUS_94426_SLOTS, (char *)data.memcardIcon_HeaderSLOTS,
	                              (struct GhostHeader *)sdata->ptrToMemcardBuffer1, 0x1680);
}

static void RefreshCard_QueueGhostSave(void)
{
	RefreshCard_NextMemcardAction(0, MC_ACTION_Save, data.s_BASCUS_94426G_Question, sdata->memcardIcon_HeaderGHOST, sdata->GhostRecording.ptrGhost, 0x3e00);
}

static void RefreshCard_QueueGhostLoad(void)
{
	RefreshCard_NextMemcardAction(0, MC_ACTION_Load, sdata->ghostProfile_memcard[sdata->ghostProfile_indexLoad].profile_name, NULL, sdata->ptrGhostTapePlaying,
	                              0x3e00);
}

static void RefreshCard_SetScreenAndPoll(int screenText)
{
	RefreshCard_SetScreenText(screenText);
	RefreshCard_QueueGetInfo();
	sdata->boolError = 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800472d0-0x80047a58.
void RefreshCard_Unknown3(void)
{
	int keepPolling = 0;

	switch (sdata->mcScreenText)
	{
	case MC_SCREEN_WARNING_NOCARD:
		keepPolling = 1;
		sdata->mcStart = 2;
		sdata->boolError = 1;
		break;

	case MC_SCREEN_WARNING_UNFORMATTED:
		if (sdata->mcStart != 7)
		{
			sdata->mcStart = 2;
			keepPolling = 1;
			sdata->boolError = 1;
			break;
		}

		sdata->mcStart = 2;
		RefreshCard_SetScreenText(MC_SCREEN_FORMATTING);
		RefreshCard_NextMemcardAction(0, MC_ACTION_Format, data.s_BASCUS_94426_SLOTS, NULL, NULL, 0);
		sdata->boolError = 0;
		break;

	case MC_SCREEN_CHECKING:
	case MC_SCREEN_ERROR_FULL:
		keepPolling = 1;
		break;

	case MC_SCREEN_ERROR_TIMEOUT:
	case MC_SCREEN_NULL:
	case MC_SCREEN_ERROR_NODATA:
		if (sdata->mcStart == 5)
		{
			RefreshCard_SetScreenText(MC_SCREEN_LOADING);
			RefreshCard_QueueGhostLoad();
		}
		else if (sdata->mcStart == 3)
		{
			sdata->mcStart = 2;
			RefreshCard_SetScreenText(MC_SCREEN_SAVING);
			RefreshCard_QueueMainSave();
			sdata->boolError = 0;
			break;
		}
		else if (sdata->mcStart == 6)
		{
			if (sdata->ghostProfile_rowSelect >= 0)
			{
				int remaining;

				memcpy(data.s_BASCUS_94426G_Question, sdata->ghostProfile_memcard[sdata->ghostProfile_rowSelect].profile_name,
				       sizeof(data.s_BASCUS_94426G_Question));
				RefreshCard_SetScreenText(MC_SCREEN_SAVING);
				RefreshCard_NextMemcardAction(0, MC_ACTION_Erase, data.s_BASCUS_94426G_Question, NULL, NULL, 0);
				sdata->boolError = 0;

				remaining = (sdata->numGhostProfilesSaved - 1) - sdata->ghostProfile_rowSelect;
				if (remaining != 0)
				{
					memmove(&sdata->ghostProfile_memcard[sdata->ghostProfile_rowSelect], &sdata->ghostProfile_memcard[sdata->ghostProfile_rowSelect + 1],
					        remaining * sizeof(struct GhostProfile));
				}
				sdata->numGhostProfilesSaved--;
				break;
			}

			RefreshCard_SetScreenText(MC_SCREEN_SAVING);
			RefreshCard_QueueGhostSave();
			sdata->boolError = 0;
			break;
		}

		keepPolling = 1;
		sdata->boolError = 1;
		break;
	}

	if (RefreshCard_GetResult(MC_RESULT_NEWCARD) != 0)
	{
		RefreshCard_Unknown2();
		RefreshCard_GetNumGhostsTotal();
		goto done;
	}

	if (RefreshCard_GetResult(MC_RESULT_ERROR_NOCARD) != 0)
	{
		RefreshCard_Unknown2();
		RefreshCard_GetNumGhostsTotal();
		RefreshCard_SetScreenAndPoll(MC_SCREEN_WARNING_NOCARD);
		keepPolling = 0;
		goto done;
	}

	if (RefreshCard_GetResult(MC_RESULT_FULL) != 0)
	{
		RefreshCard_Unknown2();
		RefreshCard_SetScreenAndPoll(MC_SCREEN_ERROR_FULL);
		keepPolling = 0;
		goto done;
	}

	if (RefreshCard_GetResult(MC_RESULT_ERROR_TIMEOUT) != 0)
	{
		RefreshCard_Unknown2();
		RefreshCard_SetScreenAndPoll(MC_SCREEN_ERROR_TIMEOUT);
		keepPolling = 0;
		goto done;
	}

	if (RefreshCard_GetResult(MC_RESULT_ERROR_NODATA) != 0)
	{
		sdata->boolMemcardDataValid = 0;
		RefreshCard_Unknown2();
		sdata->boolError = 1;

		if ((sdata->memcardAction >= 0) && (sdata->memcardAction < 3))
		{
			RefreshCard_SetScreenText(MC_SCREEN_ERROR_NODATA);
			RefreshCard_QueueGetInfo();
			keepPolling = 0;
		}
		goto done;
	}

	if (RefreshCard_GetResult(MC_RESULT_READY_LOAD) != 0)
	{
		RefreshCard_Unknown2();
		*(s16 *)&sdata->unk8008d95c = 0;
		*(s16 *)&sdata->boolAdvProfilesChecked = 0;
		RefreshCard_SetScreenText(MC_SCREEN_LOADING);
		RefreshCard_QueueMainLoad();
		sdata->boolError = 0;
		keepPolling = 0;
		goto done;
	}

	if (RefreshCard_GetResult(MC_RESULT_ERROR_UNFORMATTED) != 0)
	{
		RefreshCard_GetNumGhostsTotal();
		RefreshCard_Unknown2();
		RefreshCard_SetScreenAndPoll(MC_SCREEN_WARNING_UNFORMATTED);
		keepPolling = 0;
		goto done;
	}

	if (RefreshCard_GetResult(MC_RESULT_READY_SAVE) == 0)
	{
		goto done;
	}

	sdata->boolError = 1;

	if (sdata->mcScreenText == MC_SCREEN_SAVING)
	{
		if (sdata->mcStart == 6)
		{
			if (sdata->ghostProfile_rowSelect >= 0)
			{
				sdata->ghostProfile_rowSelect = -1;
				RefreshCard_SetScreenText(MC_SCREEN_SAVING);
				RefreshCard_QueueGhostSave();
				sdata->boolError = 0;
				keepPolling = 0;
				goto done;
			}

			{
				int remaining = (sdata->numGhostProfilesSaved - 1) - sdata->ghostProfile_indexSave;

				if (remaining != 0)
				{
					memmove(&sdata->ghostProfile_memcard[sdata->ghostProfile_indexSave + 1], &sdata->ghostProfile_memcard[sdata->ghostProfile_indexSave],
					        remaining * sizeof(struct GhostProfile));
				}

				sdata->numGhostProfilesSaved++;
				sdata->ghostProfile_memcard[sdata->ghostProfile_indexSave] = sdata->ghostProfile_current;
			}
		}

		*(s16 *)&sdata->unk8008d964 = 1;
		sdata->mcStart = 2;
		RefreshCard_SetScreenText(MC_SCREEN_NULL);
		RefreshCard_QueueGetInfo();
		keepPolling = 0;
		goto done;
	}

	if (sdata->mcScreenText < MC_SCREEN_LOADING)
	{
		if (sdata->mcScreenText == MC_SCREEN_FORMATTING)
		{
			RefreshCard_GetNumGhostsTotal();
			RefreshCard_Unknown2();
			*(s16 *)&sdata->unk_memcardRelated_8008d928[0] = 1;

			if (sdata->memcardAction >= 0)
			{
				if (sdata->memcardAction < 2)
				{
					GAMEPROG_InitFullMemcard(sdata->ptrToMemcardBuffer2);
					RefreshCard_SetScreenText(MC_SCREEN_NULL);
					RefreshCard_QueueGetInfo();
					keepPolling = 0;
				}
				else if (sdata->memcardAction == 2)
				{
					sdata->boolError = 1;
				}
			}
		}
		goto done;
	}

	if (sdata->mcScreenText != MC_SCREEN_LOADING)
	{
		if (sdata->mcScreenText == MC_SCREEN_CHECKING)
		{
			RefreshCard_GetNumGhostsTotal();
			RefreshCard_Unknown2();
			*(s16 *)&sdata->unk8008d95c = 0;
			*(s16 *)&sdata->boolAdvProfilesChecked = 0;
			RefreshCard_SetScreenText(MC_SCREEN_LOADING);
			RefreshCard_QueueMainLoad();
			sdata->boolError = 0;
			keepPolling = 0;
		}
		goto done;
	}

	if (sdata->mcStart == 5)
	{
		sdata->boolReplayHumanGhost = 1;
		*(s16 *)&sdata->unk8008d964 = 1;
		RefreshCard_SetScreenText(MC_SCREEN_NULL);
	}
	else if (*(int *)sdata->ptrToMemcardBuffer2 == 0x1600ffee)
	{
		sdata->boolMemcardDataValid = 0;
		RefreshCard_GameProgressAndOptions();
		RefreshCard_SetScreenText(MC_SCREEN_NULL);
	}
	else
	{
		sdata->boolMemcardDataValid = 1;
		RefreshCard_Unknown2();
		RefreshCard_SetScreenText(MC_SCREEN_ERROR_NODATA);
	}

	sdata->mcStart = 2;
	RefreshCard_QueueGetInfo();
	keepPolling = 0;

done:
	if ((keepPolling != 0) && (RefreshCard_GetResult(MC_RESULT_PENDING) == 0))
	{
		RefreshCard_QueueGetInfo();
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80047a58-0x80047d64.
void RefreshCard_Unknown4(void)
{
	int result = -1;

	if ((sdata->memcardUnk1 & 1) == 0)
	{
		if (sdata->frame1_memcardAction != 0)
		{
			result = MEMCARD_HandleEvent();
			sdata->frame3_memcardAction = sdata->frame1_memcardAction;
			sdata->frame3_memcardSlot = sdata->frame1_memcardSlot;
		}
	}
	else
	{
		sdata->frame3_memcardAction = sdata->frame1_memcardAction;
		sdata->frame3_memcardSlot = sdata->frame1_memcardSlot;

		sdata->memcardUnk1 &= ~1;
		if ((sdata->memcardUnk1 & 2) == 0)
		{
			sdata->memcardUnk1 &= ~4;
		}

		switch (sdata->frame1_memcardAction)
		{
		case MC_ACTION_GetInfo:
			result = MEMCARD_GetInfo(sdata->frame1_memcardSlot);
			break;
		case MC_ACTION_Save:
			result = MEMCARD_Save(sdata->frame1_memcardSlot, sdata->ghostProfile_fileName, sdata->ghostProfile_fileIconHeader,
			                      (u8 *)sdata->ghostProfile_ptrGhostHeader, sdata->ghostProfile_size3E00, 0);
			break;
		case MC_ACTION_Load:
			result = MEMCARD_Load(sdata->frame1_memcardSlot, sdata->ghostProfile_fileName, (u8 *)sdata->ghostProfile_ptrGhostHeader,
			                      sdata->ghostProfile_size3E00, 0);
			break;
		case MC_ACTION_Format:
			result = MEMCARD_Format(sdata->frame1_memcardSlot);
			break;
		case MC_ACTION_Erase:
			result = MEMCARD_EraseFile(sdata->frame1_memcardSlot, sdata->ghostProfile_fileName);
			break;
		}
	}

	if ((sdata->frame1_memcardAction == MC_ACTION_GetInfo) && (result == MC_RETURN_NEWCARD))
	{
		char *fileName;
		int totalGhosts = 0;

		sdata->numGhostProfilesSaved = 0;
		fileName = MEMCARD_FindFirstGhost(sdata->frame1_memcardSlot, data.s_BASCUS_94426G_Star);

		while (fileName != NULL)
		{
			if (totalGhosts < 7)
			{
				RefreshCard_GhostDecodeProfile(&sdata->ghostProfile_memcard[totalGhosts], fileName);
				sdata->numGhostProfilesSaved++;
			}

			totalGhosts++;
			fileName = MEMCARD_FindNextGhost();
		}

		MEMCARD_IsFile(sdata->frame1_memcardSlot, sdata->ghostProfile_fileName);
		sdata->memcardUnk1 |= 8;
		result = MEMCARD_IsFile(sdata->frame1_memcardSlot, sdata->ghostProfile_fileName);
	}

	switch (result)
	{
	case MC_RETURN_IOE:
		sdata->desired_memcardResult = MC_RESULT_READY_SAVE;
		if (sdata->frame1_memcardAction == MC_ACTION_GetInfo)
		{
			sdata->desired_memcardResult = MC_RESULT_READY_LOAD;
			if ((sdata->memcardUnk1 & 8) == 0)
			{
				sdata->desired_memcardResult = MC_RESULT_READY_SAVE;
			}
		}
		break;
	case MC_RETURN_TIMEOUT:
		sdata->desired_memcardResult = MC_RESULT_ERROR_TIMEOUT;
		break;
	case MC_RETURN_NOCARD:
		sdata->desired_memcardResult = MC_RESULT_ERROR_NOCARD;
		sdata->frame1_memcardAction = 0;
		goto try_next_action;
	case MC_RETURN_NEWCARD:
		sdata->desired_memcardResult = MC_RESULT_NEWCARD;
		if (sdata->frame1_memcardAction == MC_ACTION_Format)
		{
			sdata->desired_memcardResult = MC_RESULT_READY_SAVE;
		}
		break;
	case MC_RETURN_FULL:
		sdata->desired_memcardResult = MC_RESULT_FULL;
		break;
	case MC_RETURN_UNFORMATTED:
		sdata->desired_memcardResult = MC_RESULT_ERROR_UNFORMATTED;
		break;
	case MC_RETURN_NODATA:
		sdata->desired_memcardResult = MC_RESULT_ERROR_NODATA;
		break;
	case MC_RETURN_PENDING:
		sdata->desired_memcardResult = MC_RESULT_PENDING;
		goto try_next_action;
	default:
		goto try_next_action;
	}

	sdata->frame1_memcardAction = 0;

try_next_action:
	if ((sdata->frame1_memcardAction == 0) && (sdata->frame2_memcardAction != 0))
	{
		sdata->frame1_memcardAction = sdata->frame2_memcardAction;
		sdata->frame2_memcardAction = 0;
		sdata->frame1_memcardSlot = sdata->frame2_memcardSlot;
		sdata->memcardUnk1 = (sdata->memcardUnk1 & ~2) | 1;
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80047d64-0x80047da8.
void RefreshCard_Entry(void)
{
	if ((sdata->gGT->gameMode1 & DEBUG_MENU) == 0)
	{
		RefreshCard_Unknown4();
		RefreshCard_Unknown3();
	}
}
