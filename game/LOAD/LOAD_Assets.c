#include <common.h>

#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
#include <platform/native_checkpoint.h>
#endif

void LOAD_RunPtrMap(char *origin, int *patchArr, int numPtrs)
{
	int *ptrCurrOffset = patchArr;
	int *ptrEndOffset = &patchArr[numPtrs];

	for (; ptrCurrOffset < ptrEndOffset; ptrCurrOffset++)
	{
		int offset = (*ptrCurrOffset >> 2) << 2;
		// NOTE(aalhendi): Retail adds the aligned offset before the 32-bit base address.
		int *location = (int *)((u32)offset + (u32)origin);
		*location = *location + (int)origin;
#if defined(CTR_NATIVE) && defined(CTR_INTERNAL)
		NativeCheckpoint_RegisterPointerSlot((char *)location);
#endif
	}
}

void LOAD_Robots2P(struct BigHeader *bigfile, int p1, int p2, void (*callback)(struct LoadQueueSlot *))
{
	int setIndex = 0;
	int setOffset;
	int racerIndex;
	s16 *characterIDs = GAME_CHARACTER_IDS;
	u8 *robotSetBase = GAME_2P_AI_SETS[0];
	// NOTE(aalhendi): Retail keeps the set pointer in t1 while scanning via a separate table offset.
	register u8(*robotSet)[LOAD_2P_AI_SET_RACER_COUNT] CTR_PSX_REGISTER("$9") = GAME_2P_AI_SETS;
	b32 boolFoundRepeat;
	setOffset = 0;

	// 8 sets, but only check 7 because the last is the Gem Cups pack (4 bosses).
	for (; setIndex < LOAD_2P_AI_SET_COUNT; robotSet++, setIndex++, setOffset += LOAD_2P_AI_SET_RACER_COUNT)
	{
		boolFoundRepeat = false;
		racerIndex = 0;
		do
		{
			// NOTE(aalhendi): The offset-first 32-bit address keeps retail's two-add order.
			u8 racerID = *(u8 *)((u32)(racerIndex + setOffset) + (u32)robotSetBase);
			if ((p1 == racerID) || (p2 == racerID))
			{
				boolFoundRepeat = true;
			}
			racerIndex++;
		} while ((racerIndex < LOAD_2P_AI_SET_RACER_COUNT) && !boolFoundRepeat);

		if (!boolFoundRepeat)
		{
			characterIDs[2] = (*robotSet)[0];
			characterIDs[3] = (*robotSet)[1];
			characterIDs[4] = (*robotSet)[2];
			characterIDs[5] = (*robotSet)[3];

			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_2PARCADEPACK + setIndex, NULL, callback);
			return;
		}
	}
}

void LOAD_Robots1P(int characterID)
{
	int slotIndex = 1;
	int candidateID;
	s16 *slot;

	GAME_CHARACTER_IDS[0] = characterID;
	candidateID = 0;
	slot = &GAME_CHARACTER_IDS[1];

	// NOTE(aalhendi): Retail scans candidate IDs and advances the slot only for IDs kept.
	for (; candidateID < LOAD_CHARACTER_ID_COUNT; candidateID++)
	{
		if (slotIndex >= LOAD_CHARACTER_ID_COUNT)
		{
			break;
		}

		if (candidateID != characterID)
		{
			*slot++ = candidateID;
			slotIndex++;
		}
	}
}

static void (*const LOAD_DriverMPK_SetPointer)(struct LoadQueueSlot *) = LOAD_QUEUE_CALLBACK_SET_POINTER;

// NOTE(aalhendi): Retail queues fixed LOD slots and pack files in branch-local calls.
int LOAD_DriverMPK(struct BigHeader *bigfile, int levelLOD, void (*callback)(struct LoadQueueSlot *))
{
	int gameMode1;
	struct GameTracker *gGT;

	// 3P/4P
	if ((u32)(levelLOD - LOAD_LEVEL_LOD_3P) < LOAD_LEVEL_LOD_3P4P_COUNT)
	{
		// low lod CTR models
		LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELLOW + GAME_CHARACTER_IDS[0], &GAME_DRIVER_MODEL_EXTRAS[0].fileBase, LOAD_DriverMPK_SetPointer);
		LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELLOW + GAME_CHARACTER_IDS[1], &GAME_DRIVER_MODEL_EXTRAS[1].fileBase, LOAD_DriverMPK_SetPointer);
		LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELLOW + GAME_CHARACTER_IDS[2], &GAME_DRIVER_MODEL_EXTRAS[2].fileBase, LOAD_DriverMPK_SetPointer);

		// load 4P MPK of fourth player
		LOAD_AppendQueue(bigfile, LT_GETADDR, BI_4PARCADEPACK + GAME_CHARACTER_IDS[3], NULL, callback);
		return sdata->ptrMPK;
	}

	if (levelLOD == LOAD_LEVEL_LOD_1P)
	{
		gGT = GAME_TRACKER;
		gameMode1 = gGT->gameMode1;
		if ((gameMode1 & (TIME_TRIAL | MAIN_MENU)) == TIME_TRIAL)
		{
			goto CheckHighAndPack;
		}

		if (
		    // adv/cutscene mpk when we just need text from MPK
		    ((gameMode1 & (GAME_CUTSCENE | ADVENTURE_ARENA)) != 0) ||

		    // credits
		    ((gGT->gameMode2 & CREDITS) != 0) ||

		    // adventure character select
		    (gGT->levelID == ADVENTURE_GARAGE))
		{
			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_ADVENTUREPACK + GAME_CHARACTER_IDS[0], NULL, callback);
			return sdata->ptrMPK;
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
			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELHI + GAME_CHARACTER_IDS[0], &GAME_DRIVER_MODEL_EXTRAS[0].fileBase, LOAD_DriverMPK_SetPointer);

			// pack of four AIs with bosses
			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_2PARCADEPACK + LOAD_PURPLE_GEM_CUP_AI_SET_INDEX, NULL, callback);

			GAME_CHARACTER_IDS[1] = RIPPER_ROO;
			GAME_CHARACTER_IDS[2] = PAPU_PAPU;
			GAME_CHARACTER_IDS[3] = KOMODO_JOE;
			GAME_CHARACTER_IDS[4] = PINSTRIPE;

			return sdata->ptrMPK;
		}

		// NOTE(aalhendi): Retail rereads the tracker slot before deciding whether to choose AI drivers.
		if ((GAME_TRACKER_RELOAD()->gameMode1 & (TIME_TRIAL | MAIN_MENU)) != MAIN_MENU)
		{
			LOAD_Robots1P(GAME_CHARACTER_IDS[0]);
		}

		// arcade mpk
		LOAD_AppendQueue(bigfile, LT_GETADDR, BI_1PARCADEPACK + GAME_CHARACTER_IDS[0], NULL, callback);
		return sdata->ptrMPK;
	}

// NOTE(aalhendi): Time trial rechecks the high-LOD condition; boss races enter its body directly.
CheckHighAndPack:
	if ((levelLOD == LOAD_LEVEL_LOD_RELIC) || ((GAME_TRACKER->gameMode1 & TIME_TRIAL) != 0))
	{
	LoadHighAndPack:
		// Do NOT switch the order to optimize Relic,
		// if HI+IDs[1] and PACK+IDs[0] is loaded,
		// then mask-grab breaks for all characters
		// on Hot Air Skyway (except Crash Bandicoot)

		// Load Player 1 [0]
		LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELHI + GAME_CHARACTER_IDS[0], &GAME_DRIVER_MODEL_EXTRAS[0].fileBase, LOAD_DriverMPK_SetPointer);

		// Load boss or ghost [1]
		LOAD_AppendQueue(bigfile, LT_GETADDR, BI_TIMETRIALPACK + GAME_CHARACTER_IDS[1], NULL, callback);
		return sdata->ptrMPK;
	}

	// 2P and other standard LODs
	else
	{
		// med models
		LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELMED + GAME_CHARACTER_IDS[0], &GAME_DRIVER_MODEL_EXTRAS[0].fileBase, LOAD_DriverMPK_SetPointer);
		LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELMED + GAME_CHARACTER_IDS[1], &GAME_DRIVER_MODEL_EXTRAS[1].fileBase, LOAD_DriverMPK_SetPointer);

		LOAD_Robots2P(bigfile, GAME_CHARACTER_IDS[0], GAME_CHARACTER_IDS[1], callback);
		return sdata->ptrMPK;
	}
}

#include "LOAD_LangFile.c"

// NOTE(aalhendi): Variable-stride group bases come last to preserve retail's return scheduling.
int LOAD_GetBigfileIndex(u32 levelID, int lod, int fileIndexInGroup)
{
	// MultiTrack: custom track picked in the track list; never leak it into
	// adventure or cups, which reuse the host slot
	if ((levelID < NITRO_COURT + LOAD_BATTLE_TRACK_COUNT) && (MT_STATE->magic == MT_MAGIC) && (MT_STATE->redirectLevel == (int)levelID) &&
	    ((sdata->gGT->gameMode1 & (ADVENTURE_MODE | ADVENTURE_ARENA)) == 0) && ((sdata->gGT->gameMode2 & CUP_ANY_KIND) == 0))
	{
		return MT_STATE->redirectBase + GAME_LEVEL_BIG_LOD_INDEX[lod - 1] + fileIndexInGroup;
	}

	if (levelID < NITRO_COURT)
	{
		return BI_ARCADETRACKS + levelID * LOAD_TRACK_FILES_PER_LOD_GROUP + GAME_LEVEL_BIG_LOD_INDEX[lod - 1] + fileIndexInGroup;
	}

	if ((u32)(levelID - NITRO_COURT) < LOAD_BATTLE_TRACK_COUNT)
	{
		return (levelID - NITRO_COURT) * LOAD_TRACK_FILES_PER_LOD_GROUP + GAME_LEVEL_BIG_LOD_INDEX[lod - 1] + fileIndexInGroup + BI_BATTLETRACKS;
	}

	if ((u32)(levelID - INTRO_RACE_TODAY) < LOAD_INTRO_CUTSCENE_COUNT)
	{
		return (levelID - INTRO_RACE_TODAY) * LOAD_CUTSCENE_FILES_PER_LEVEL + fileIndexInGroup + BI_CUTSCENES_INTRO;
	}

	if ((u32)(levelID - OXIDE_ENDING) < LOAD_OUTRO_CUTSCENE_COUNT)
	{
		return (levelID - OXIDE_ENDING) * LOAD_OUTRO_FILES_PER_LEVEL + fileIndexInGroup + BI_CUTSCENES_OUTRO;
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
		return (levelID - CREDITS_CRASH) * LOAD_CUTSCENE_FILES_PER_LEVEL + fileIndexInGroup + BI_CREDITS;
	}

	if (levelID == MAIN_MENU_LEVEL)
	{
		return BI_MAINMENUFILE + fileIndexInGroup;
	}

	if (levelID == SCRAPBOOK)
	{
		return BI_SCRAPBOOK + fileIndexInGroup;
	}

	return (levelID - GEM_STONE_VALLEY) * LOAD_CUTSCENE_FILES_PER_LEVEL + fileIndexInGroup + BI_ADVENTUREHUB;
}
