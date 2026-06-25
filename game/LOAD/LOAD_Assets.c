#include <common.h>

#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
#include <platform/native_checkpoint.h>
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800326b4-0x80032700.
void LOAD_RunPtrMap(char *origin, int *patchArr, int numPtrs)
{
	int *ptrCurrOffset = patchArr;

	for (ptrCurrOffset = &patchArr[0]; ptrCurrOffset < &patchArr[numPtrs]; ptrCurrOffset++)
	{
		int offset = (*ptrCurrOffset >> 2) << 2;
		*(int *)&origin[offset] = *(int *)&origin[offset] + (int)origin;
#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
		NativeCheckpoint_RegisterPointerSlot(&origin[offset]);
#endif
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80032700-0x800327dc.
void LOAD_Robots2P(struct BigHeader *bigfile, int p1, int p2, void (*callback)(struct LoadQueueSlot *))
{
	int i;
	char *robotSet;
	b32 boolFoundRepeat = false;

	// 8 sets, but only check 7 cause
	// the last is Gem Cups pack (4 bosses)
	for (i = 0; i < 7; i++)
	{
		robotSet = &data.characterIDs_2P_AIs[4 * i];

		boolFoundRepeat = false;
		for (int j = 0; j < 4; j++)
		{
			if ((robotSet[j] == p1) || (robotSet[j] == p2))
			{
				boolFoundRepeat = true;
				break;
			}
		}

		if (!boolFoundRepeat)
		{
			break;
		}
	}

	if (i > 6)
	{
		return;
	}

	data.characterIDs[2] = robotSet[0];
	data.characterIDs[3] = robotSet[1];
	data.characterIDs[4] = robotSet[2];
	data.characterIDs[5] = robotSet[3];

	LOAD_AppendQueue(bigfile, LT_GETADDR, BI_2PARCADEPACK + i, NULL, callback);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800327dc-0x8003282c.
void LOAD_Robots1P(int characterID)
{
	int newCharacterID = 0;

	data.characterIDs[0] = characterID;

	for (int i = 1; i < 8; i++, newCharacterID++)
	{
		if (newCharacterID == characterID)
		{
			newCharacterID++;
		}

		data.characterIDs[i] = newCharacterID;
	}
}

static void (*const LOAD_DriverMPK_SetPointer)(struct LoadQueueSlot *) = (void (*)(struct LoadQueueSlot *))-2;

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003282c-0x80032b50.
int LOAD_DriverMPK(struct BigHeader *bigfile, int levelLOD, void (*callback)(struct LoadQueueSlot *))
{
	int i;
	int gameMode1;

	struct GameTracker *gGT = sdata->gGT;
	gameMode1 = gGT->gameMode1;

	int lastFileIndexMPK;

	// 3P/4P
	if (levelLOD - 3U < 2)
	{
		for (i = 0; i < 3; i++)
		{
			// low lod CTR model
			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELLOW + data.characterIDs[i], &data.driverModelExtras[i].fileBase, LOAD_DriverMPK_SetPointer);
		}

		// load 4P MPK of fourth player
		lastFileIndexMPK = BI_4PARCADEPACK + data.characterIDs[3];
	}

	else if (levelLOD == 1)
	{
		if ((gameMode1 & (TIME_TRIAL | MAIN_MENU)) == TIME_TRIAL)
		{
			goto LoadHighAndPack;
		}

		if (
		    // adv/cutscene mpk when we just need text from MPK
		    ((gameMode1 & (GAME_CUTSCENE | ADVENTURE_ARENA)) != 0) ||

		    // credits
		    ((gGT->gameMode2 & CREDITS) != 0) ||

		    // adventure character select
		    (gGT->levelID == ADVENTURE_GARAGE))
		{
			lastFileIndexMPK = BI_ADVENTUREPACK + data.characterIDs[0];
			goto QueueLastPack;
		}

		if ((gameMode1 & ADVENTURE_BOSS) != 0)
		{
			goto LoadHighAndPack;
		}

		if (
		    // If you are in Adventure cup
		    ((gameMode1 & ADVENTURE_CUP) != 0) &&

		    // purple gem cup
		    (gGT->cup.cupID == 4))
		{
			// high lod model
			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELHI + data.characterIDs[0], &data.driverModelExtras[0].fileBase, LOAD_DriverMPK_SetPointer);

			// pack of four AIs with bosses
			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_2PARCADEPACK + 7, NULL, callback);

			data.characterIDs[1] = RIPPER_ROO;
			data.characterIDs[2] = PAPU_PAPU;
			data.characterIDs[3] = KOMODO_JOE;
			data.characterIDs[4] = PINSTRIPE;

			return sdata->ptrMPK;
		}

		if ((gameMode1 & (TIME_TRIAL | MAIN_MENU)) != MAIN_MENU)
		{
			LOAD_Robots1P(data.characterIDs[0]);
		}

		// arcade mpk
		lastFileIndexMPK = BI_1PARCADEPACK + data.characterIDs[0];
	}

	else if ((levelLOD == 8) || ((gameMode1 & TIME_TRIAL) != 0))
	{
	LoadHighAndPack:
		// Do NOT switch the order to optimize Relic,
		// if HI+IDs[1] and PACK+IDs[0] is loaded,
		// then mask-grab breaks for all characters
		// on Hot Air Skyway (except Crash Bandicoot)

		// Load Player 1 [0]
		LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELHI + data.characterIDs[0], &data.driverModelExtras[0].fileBase, LOAD_DriverMPK_SetPointer);

		// Load boss or ghost [1]
		lastFileIndexMPK = BI_TIMETRIALPACK + data.characterIDs[1];
	}

	// else if (levelLOD == 2)
	else
	{
		// med models
		for (i = 0; i < 2; i++)
		{
			// med lod CTR model
			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELMED + data.characterIDs[i], &data.driverModelExtras[i].fileBase, LOAD_DriverMPK_SetPointer);
		}

		LOAD_Robots2P(bigfile, data.characterIDs[0], data.characterIDs[1], callback);
		return sdata->ptrMPK;
	}

QueueLastPack:
	LOAD_AppendQueue(bigfile, LT_GETADDR, lastFileIndexMPK, NULL, callback);
	return sdata->ptrMPK;
}

struct LngFile
{
	int numStrings;
	int offsetToPtrArr;
	char strings[1];
};

// param_1 - Pointer to "cd position of bigfile"
// param_2 - language index - 0 ja, 1 en, 2 en2, 3 fr, 4 de, 5 it, 6 es, 7 ne
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80032b50-0x80032c24
void LOAD_LangFile(int bigfilePtr, int lang)
{
	struct LngFile *lngFile;
	int size;

	int i;
	int numStrings;
	char **strArray;

#if BUILD == EurRetail
	// This is to turn the screen black for a bit (optional)
	CTR_ErrorScreen(0, 0, 0);
	VSync(0);
#endif

	if (sdata->lngFile == 0)
	{
		sdata->lngFile = MEMPACK_AllocMem(sdata->langBufferSize /* "lang buffer" */);
	}

	lngFile = sdata->lngFile;

	lngFile = LOAD_ReadFile_ex((struct BigHeader *)bigfilePtr, LT_SETADDR, BI_LANGUAGEFILE + lang, lngFile, &size, NULL);
	if (lngFile == NULL)
	{
		return;
	}

	numStrings = lngFile->numStrings;
	strArray = (char **)((u32)lngFile + lngFile->offsetToPtrArr);

	sdata->numLngStrings = numStrings;
	sdata->lngStrings = strArray;

	for (i = 0; i < numStrings; i++)
	{
		strArray[i] = (char *)((u32)strArray[i] + (u32)lngFile);
	}
#if BUILD == EurRetail
	// set voicelines to new lang
	CDSYS_SetXAToLang(lang);
#endif
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80032c24-0x80032d30.
int LOAD_GetBigfileIndex(u32 levelID, int lod, int fileIndexInGroup)
{
	if (levelID < NITRO_COURT)
	{
		return BI_ARCADETRACKS + levelID * 8 + sdata->levBigLodIndex[lod - 1] + fileIndexInGroup;
	}

	if ((u32)(levelID - NITRO_COURT) < 7)
	{
		return BI_BATTLETRACKS + (levelID - NITRO_COURT) * 8 + sdata->levBigLodIndex[lod - 1] + fileIndexInGroup;
	}

	if ((u32)(levelID - INTRO_RACE_TODAY) < 9)
	{
		return BI_CUTSCENES_INTRO + (levelID - INTRO_RACE_TODAY) * 3 + fileIndexInGroup;
	}

	if ((u32)(levelID - OXIDE_ENDING) < 2)
	{
		return BI_CUTSCENES_OUTRO + (levelID - OXIDE_ENDING) * 2 + fileIndexInGroup;
	}

	if (levelID == ADVENTURE_GARAGE)
	{
		return BI_MAINMENUFILE + 2 + fileIndexInGroup;
	}

	if (levelID == NAUGHTY_DOG_CRATE)
	{
		return BI_NDBOX + fileIndexInGroup;
	}

	if ((u32)(levelID - CREDITS_CRASH) < 20)
	{
		return BI_CREDITS + (levelID - CREDITS_CRASH) * 3 + fileIndexInGroup;
	}

	if (levelID == MAIN_MENU_LEVEL)
	{
		return BI_MAINMENUFILE + fileIndexInGroup;
	}

	if (levelID == SCRAPBOOK)
	{
		return BI_SCRAPBOOK + fileIndexInGroup;
	}

	return BI_ADVENTUREHUB + (levelID - GEM_STONE_VALLEY) * 3 + fileIndexInGroup;
}
