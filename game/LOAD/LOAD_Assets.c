#include <common.h>

#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
#include <platform/native_checkpoint.h>
#endif

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

enum
{
    LOOSE_RACER_CHARACTER_COUNT = NITROS_OXIDE + 1,
};

#if defined(CTR_NATIVE)
#include <platform/native_assets.h>

static void *LOAD_ReadLooseRacerModel(int characterID)
{
    static const char *const paths[LOOSE_RACER_CHARACTER_COUNT] = {
		"mods/racers/crash.ctr",    // 0
		"mods/racers/cortex.ctr",   // 1
		"mods/racers/tiny.ctr",     // 2
		"mods/racers/coco.ctr",     // 3
		"mods/racers/ngin.ctr",     // 4
		"mods/racers/dingo.ctr",    // 5
		"mods/racers/polar.ctr",    // 6
		"mods/racers/pura.ctr",     // 7
		"mods/racers/pinstripe.ctr",// 8
		"mods/racers/papu.ctr",     // 9
		"mods/racers/roo.ctr",      // 10
		"mods/racers/joe.ctr",      // 11
		"mods/racers/ntropy.ctr",   // 12
		"mods/racers/pen.ctr",      // 13
		"mods/racers/fake.ctr",     // 14
		"mods/racers/oxide.ctr",    // 15
	};

	if ((u32)characterID >= len(paths))
	{
		return NULL;
	}

    struct NativeAssetsByteBuffer file;
    if (!NativeAssets_ReadBytes(paths[characterID],
                                NATIVE_ASSET_READ_DATA_FILE, &file))
    {
        return NULL;
    }

    void *modelFile = MEMPACK_AllocMem(file.size);
	memcpy(modelFile, file.data, file.size);
	NativeAssets_FreeBytes(&file);

	int ptrMapOffset = *(int *)modelFile;
	char *modelData = (char *)modelFile + LOAD_MODEL_FILE_HEADER_BYTES;

	if (ptrMapOffset >= 0)
	{
		struct DramPointerMap *pointerMap =
			(struct DramPointerMap *)&modelData[ptrMapOffset];

		LOAD_RunPtrMap(
			modelData,
			DRAM_GETOFFSETS(pointerMap),
			pointerMap->numBytes >> DRAM_POINTER_MAP_WORD_SHIFT);
	}

    return modelData;
}
#endif

static void *sLooseRacerFileBases[LOAD_CHARACTER_ID_COUNT];
static struct Model *sLooseRacerModels[LOAD_CHARACTER_ID_COUNT + 1];
static int sLooseRacerModelCount;

void LOAD_LoadLooseRacerModels(int racerCount)
{
    int outputIndex = 0;

    if (racerCount > LOAD_CHARACTER_ID_COUNT)
    {
        racerCount = LOAD_CHARACTER_ID_COUNT;
    }

    memset(sLooseRacerFileBases, 0, sizeof(sLooseRacerFileBases));
    memset(sLooseRacerModels, 0, sizeof(sLooseRacerModels));

    for (int racerIndex = 0; racerIndex < racerCount; racerIndex++)
    {
        void *fileBase = LOAD_ReadLooseRacerModel(data.characterIDs[racerIndex]);

        if (fileBase != NULL)
        {
            sLooseRacerFileBases[outputIndex] = fileBase;
            outputIndex++;
        }
    }

    sLooseRacerModelCount = outputIndex;
}

void LOAD_FinalizeLooseRacerModels(void)
{
    for (int i = 0; i < sLooseRacerModelCount; i++)
    {
        sLooseRacerModels[i] =
			(struct Model *)sLooseRacerFileBases[i];
    }

    sLooseRacerModels[sLooseRacerModelCount] = NULL;
}

struct Model **LOAD_GetLooseRacerModelList(void)
{
    return sLooseRacerModels;
}

void LOAD_ClearLooseRacerModels(void)
{
    memset(sLooseRacerFileBases, 0, sizeof(sLooseRacerFileBases));
    memset(sLooseRacerModels, 0, sizeof(sLooseRacerModels));
    sLooseRacerModelCount = 0;
}

void LOAD_Robots2P(struct BigHeader *bigfile, int p1, int p2, void (*callback)(struct LoadQueueSlot *))
{
	int setIndex;
	u8 *robotSet;
	b32 boolFoundRepeat = false;

	// 8 sets, but only check 7 because the last is the Gem Cups pack (4 bosses).
	for (setIndex = 0; setIndex < LOAD_2P_AI_SET_COUNT; setIndex++)
	{
		robotSet = data.characterIDs_2P_AIs[setIndex];

		boolFoundRepeat = false;
		for (int racerIndex = 0; racerIndex < LOAD_2P_AI_SET_RACER_COUNT; racerIndex++)
		{
			if ((robotSet[racerIndex] == p1) || (robotSet[racerIndex] == p2))
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

	if (setIndex >= LOAD_2P_AI_SET_COUNT)
	{
		return;
	}

	data.characterIDs[2] = robotSet[0];
	data.characterIDs[3] = robotSet[1];
	data.characterIDs[4] = robotSet[2];
	data.characterIDs[5] = robotSet[3];

	LOAD_AppendQueue(bigfile, LT_GETADDR, BI_2PARCADEPACK + setIndex, NULL, callback);
	LOAD_LoadLooseRacerModels(6);
}

void LOAD_Robots1P(int characterID)
{
	int newCharacterID = 0;

	data.characterIDs[0] = characterID;

	for (int i = 1; i < LOAD_CHARACTER_ID_COUNT; i++, newCharacterID++)
	{
		if (newCharacterID == characterID)
		{
			newCharacterID++;
		}

		data.characterIDs[i] = newCharacterID;
	}
	LOAD_LoadLooseRacerModels(LOAD_CHARACTER_ID_COUNT);
}

static void (*const LOAD_DriverMPK_SetPointer)(struct LoadQueueSlot *) = LOAD_QUEUE_CALLBACK_SET_POINTER;

int LOAD_DriverMPK(struct BigHeader *bigfile, int levelLOD, void (*callback)(struct LoadQueueSlot *))
{
	int gameMode1;

	struct GameTracker *gGT = sdata->gGT;
	gameMode1 = gGT->gameMode1;

	int lastFileIndexMPK;

	// 3P/4P
	if ((u32)(levelLOD - LOAD_LEVEL_LOD_3P) < LOAD_LEVEL_LOD_3P4P_COUNT)
	{
		// load 4P MPK of fourth player
		lastFileIndexMPK = BI_4PARCADEPACK + data.characterIDs[3];
		LOAD_LoadLooseRacerModels(4);
	}

	else if (levelLOD == LOAD_LEVEL_LOD_1P)
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

			// pack of four AIs with bosses
			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_2PARCADEPACK + LOAD_PURPLE_GEM_CUP_AI_SET_INDEX, NULL, callback);

			data.characterIDs[1] = RIPPER_ROO;
			data.characterIDs[2] = PAPU_PAPU;
			data.characterIDs[3] = KOMODO_JOE;
			data.characterIDs[4] = PINSTRIPE;

			LOAD_LoadLooseRacerModels(5);

			return sdata->ptrMPK;
		}

		if ((gameMode1 & (TIME_TRIAL | MAIN_MENU)) != MAIN_MENU)
		{
			LOAD_Robots1P(data.characterIDs[0]);
		}

		// arcade mpk
		lastFileIndexMPK = BI_1PARCADEPACK + data.characterIDs[0];
	}

	else if ((levelLOD == LOAD_LEVEL_LOD_RELIC) || ((gameMode1 & TIME_TRIAL) != 0))
	{
	LoadHighAndPack:
		// Do NOT switch the order to optimize Relic,
		// if HI+IDs[1] and PACK+IDs[0] is loaded,
		// then mask-grab breaks for all characters
		// on Hot Air Skyway (except Crash Bandicoot)

		// Load boss or ghost [1]
		lastFileIndexMPK = BI_TIMETRIALPACK + data.characterIDs[1];
	}

	// else if (levelLOD == LOAD_LEVEL_LOD_2P)
	else
	{
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
void LOAD_LangFile(int bigfilePtr, int lang)
{
	struct LngFile *lngFile;
	u32 size;

	int i;
	int numStrings;
	char **strArray;


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
}

int LOAD_GetBigfileIndex(u32 levelID, int lod, int fileIndexInGroup)
{
	if (levelID < NITRO_COURT)
	{
		return BI_ARCADETRACKS + levelID * LOAD_TRACK_FILES_PER_LOD_GROUP + sdata->levBigLodIndex[lod - 1] + fileIndexInGroup;
	}

	if ((u32)(levelID - NITRO_COURT) < LOAD_BATTLE_TRACK_COUNT)
	{
		return BI_BATTLETRACKS + (levelID - NITRO_COURT) * LOAD_TRACK_FILES_PER_LOD_GROUP + sdata->levBigLodIndex[lod - 1] + fileIndexInGroup;
	}

	if ((u32)(levelID - INTRO_RACE_TODAY) < LOAD_INTRO_CUTSCENE_COUNT)
	{
		return BI_CUTSCENES_INTRO + (levelID - INTRO_RACE_TODAY) * LOAD_CUTSCENE_FILES_PER_LEVEL + fileIndexInGroup;
	}

	if ((u32)(levelID - OXIDE_ENDING) < LOAD_OUTRO_CUTSCENE_COUNT)
	{
		return BI_CUTSCENES_OUTRO + (levelID - OXIDE_ENDING) * LOAD_OUTRO_FILES_PER_LEVEL + fileIndexInGroup;
	}

	if (levelID == ADVENTURE_GARAGE)
	{
		return BI_MAINMENUFILE + LOAD_MAIN_MENU_GARAGE_FILE_OFFSET + fileIndexInGroup;
	}

	if (levelID == NAUGHTY_DOG_CRATE)
	{
		return BI_NDBOX + fileIndexInGroup;
	}

	if ((u32)(levelID - CREDITS_CRASH) < LOAD_CREDIT_LEVEL_COUNT)
	{
		return BI_CREDITS + (levelID - CREDITS_CRASH) * LOAD_CUTSCENE_FILES_PER_LEVEL + fileIndexInGroup;
	}

	if (levelID == MAIN_MENU_LEVEL)
	{
		return BI_MAINMENUFILE + fileIndexInGroup;
	}

	if (levelID == SCRAPBOOK)
	{
		return BI_SCRAPBOOK + fileIndexInGroup;
	}

	return BI_ADVENTUREHUB + (levelID - GEM_STONE_VALLEY) * LOAD_CUTSCENE_FILES_PER_LEVEL + fileIndexInGroup;
}
