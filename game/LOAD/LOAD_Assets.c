#include <common.h>

// The AI roster and the extended-multiplayer grid follow the shared race-field
// rules, so the loader and the driver spawner cannot disagree on the field size.
#include <race_field.h>

#if defined(CTR_INTERNAL)
#include <platform/native_checkpoint.h>
#include <platform/native_config.h>
#endif

enum
{
	LOAD_EXTRA_CHARACTER_MODEL_CAPACITY = LOAD_CHARACTER_ID_COUNT,
	LOAD_DEFAULT_CHARACTER_COUNT = 8,
	// Pura is the eighth base racer, so the 1P arcade MPK only carries her when
	// the player picks her; see LOAD_RacerNeedsStandaloneModel.
	LOAD_UNCOVERED_BASE_RACER_ID = 7,
};

static DriverModelExtraSlot s_extraCharacterModels[LOAD_EXTRA_CHARACTER_MODEL_CAPACITY];
static s16 s_extraCharacterModelIDs[LOAD_EXTRA_CHARACTER_MODEL_CAPACITY];
static int s_extraCharacterModelCount;

// The Purple Gem Cup fields the bosses of four of its tracks instead of a normal AI
// lineup, in this order.
static const s16 LOAD_PurpleGemCupBosses[LOAD_2P_AI_SET_RACER_COUNT] = {
	RIPPER_ROO,
	PAPU_PAPU,
	KOMODO_JOE,
	PINSTRIPE,
};

static b32 LOAD_IsRandomBotSelectionEnabled(void)
{
	int mode = g_config.botSelectionMode;
	return mode == BOT_SELECTION_RANDOM_UNLOCKED || mode == BOT_SELECTION_RANDOM_ALL;
}

static void LOAD_ShuffleCharacterIDs(s16 *characterIDs)
{
	for (int i = GAME_CHARACTER_COUNT - 1; i > 0; i--)
	{
		int swapIndex = (u32)RngDeadCoed(&sdata->advRng) % (i + 1);
		s16 characterID = characterIDs[i];
		characterIDs[i] = characterIDs[swapIndex];
		characterIDs[swapIndex] = characterID;
	}
}

// The hub keeps the Adventure co-op partner in driver slot 1 while the tracker
// still counts a single player, so the human drivers are not always the players
// of the current game.
static int LOAD_HumanDriverSlotCount(void)
{
	struct GameTracker *gGT = sdata->gGT;

	if ((gGT->gameMode1 & ADVENTURE_ARENA) != 0 && sdata->advMultiplayer.numPlayers > 1)
	{
		return sdata->advMultiplayer.numPlayers;
	}

	return gGT->numPlyrCurrGame;
}

// A boss race is one racer per player plus the boss, so the boss drives in the
// first driver slot past the players. Retail only ever plays a boss race solo,
// where that is the second slot, so a solo race keeps the boss where it always was.
int LOAD_AdventureBossDriverSlot(void)
{
	int playerCount = LOAD_HumanDriverSlotCount();

	if (playerCount > ADV_MULTIPLAYER_DEFAULT_PLAYERS)
	{
		return playerCount;
	}

	return LOAD_ADVENTURE_BOSS_DRIVER_SLOT_SOLO;
}

// Only a co-op boss race changes who occupies the AI driver slots; a solo boss
// race keeps the layout it has always had.
static b32 LOAD_IsAdventureCoopBossRace(void)
{
	return (sdata->gGT->gameMode1 & ADVENTURE_BOSS) != 0 &&
	       LOAD_AdventureBossDriverSlot() > LOAD_ADVENTURE_BOSS_DRIVER_SLOT_SOLO;
}

// The extended multiplayer option fills every grid slot with a racer, in Arcade
// and in Adventure alike. MainInit_Drivers spawns one driver per racer the roster
// names, so this is the single definition of what the option means for the
// current race: whoever writes character IDs and whoever counts AI slots have to
// agree on it, or the extra slots spawn drivers on IDs nobody assigned.
b32 LOAD_UsesExtendedMultiplayerGrid(void)
{
	struct GameTracker *gGT = sdata->gGT;

	return RaceField_UsesExtendedGrid(gGT->gameMode1, gGT->cup.cupID, g_config.extendedMultiplayer);
}

static b32 LOAD_IsRandomBotRace(void)
{
	u32 gameMode = sdata->gGT->gameMode1;

	if (!LOAD_IsRandomBotSelectionEnabled())
	{
		return false;
	}

	if ((gameMode & (ARCADE_MODE | ADVENTURE_MODE)) == 0)
	{
		return false;
	}

	return !RaceField_HasManagedAIRoster(gameMode, sdata->gGT->cup.cupID);
}

// Retail ran the Purple Gem Cup solo and wrote the bosses to driver slots 1-4.
// Split-screen puts a human in slot 1, so the bosses take the slots after the humans,
// which is where MainInit_Drivers spawns the cup's AI from.
static void LOAD_SelectPurpleGemCupBosses(void)
{
	for (int bossIndex = 0; bossIndex < LOAD_2P_AI_SET_RACER_COUNT; bossIndex++)
	{
		data.characterIDs[LOAD_HumanDriverSlotCount() + bossIndex] = LOAD_PurpleGemCupBosses[bossIndex];
	}
}

static b32 LOAD_IsCharacterUsedByPlayer(s16 characterID)
{
	for (int playerIndex = 0; playerIndex < LOAD_HumanDriverSlotCount(); playerIndex++)
	{
		if (data.characterIDs[playerIndex] == characterID)
		{
			return true;
		}
	}

	return false;
}

// The same count MainInit_Drivers spawns drivers from. Random selection has to
// fill exactly those slots: choosing more racers than the field holds leaves the
// extra character IDs unreferenced, and choosing fewer leaves grid slots empty.
static int LOAD_GetRandomBotCount(void)
{
	struct GameTracker *gGT = sdata->gGT;

	return RaceField_BotCount(gGT->gameMode1, gGT->cup.cupID, gGT->numPlyrCurrGame, g_config.extendedMultiplayer);
}

static b32 LOAD_ShouldPreserveRandomBotRoster(void)
{
	struct GameTracker *gGT = sdata->gGT;
	b32 cupRace = (gGT->gameMode1 & ADVENTURE_CUP) != 0 || (gGT->gameMode2 & CUP_ANY_KIND) != 0;

	return cupRace && gGT->cup.trackIndex > 0;
}

static void LOAD_ResetExtraCharacterModels(void)
{
	for (int i = 0; i < LOAD_EXTRA_CHARACTER_MODEL_CAPACITY; i++)
	{
		s_extraCharacterModels[i].fileBase = NULL;
		s_extraCharacterModelIDs[i] = -1;
	}

	s_extraCharacterModelCount = 0;
}

// The 1P arcade MPK carries eight driver models: the player plus a fixed
// seven-racer AI set drawn from the base roster. When the player is a base racer
// that set is the other seven base racers, so the pack covers all eight. When
// the player is a boss the set is crash..polar, which leaves LOAD_UNCOVERED_BASE_RACER_ID
// without any model, and a bot drawn from her renders as an invisible ghost.
static b32 LOAD_RacerNeedsStandaloneModel(s16 characterID)
{
	if (characterID >= LOAD_DEFAULT_CHARACTER_COUNT)
	{
		return true;
	}

	return (characterID == LOAD_UNCOVERED_BASE_RACER_ID) && (data.characterIDs[0] >= LOAD_DEFAULT_CHARACTER_COUNT);
}

static void LOAD_QueueExtraCharacterModels(struct BigHeader *bigfile, const s16 *characterIDs, int characterCount, int modelFileIndex)
{
	for (int characterIndex = 0; characterIndex < characterCount; characterIndex++)
	{
		if (s_extraCharacterModelCount >= LOAD_EXTRA_CHARACTER_MODEL_CAPACITY)
		{
			break;
		}

		// LOAD_AppendQueue drops anything past LOAD_QUEUE_SLOT_COUNT, and a
		// dropped request never hands back a model pointer. Stop here instead,
		// otherwise the racer is queued as loaded but stays empty.
		if (sdata->queueLength >= LOAD_QUEUE_SLOT_COUNT)
		{
			break;
		}

		int modelIndex = s_extraCharacterModelCount++;
		s_extraCharacterModelIDs[modelIndex] = characterIDs[characterIndex];
		LOAD_AppendQueue(bigfile, LT_GETADDR, modelFileIndex + characterIDs[characterIndex], &s_extraCharacterModels[modelIndex].fileBase,
		                 LOAD_QUEUE_CALLBACK_SET_POINTER);
	}
}

static void LOAD_SelectRandomBots(void)
{
	s16 shuffledCharacterIDs[GAME_CHARACTER_COUNT];
	int botCount = LOAD_GetRandomBotCount();
	int botIndex = 0;

	for (int characterID = 0; characterID < GAME_CHARACTER_COUNT; characterID++)
	{
		shuffledCharacterIDs[characterID] = (s16)characterID;
	}

	LOAD_ShuffleCharacterIDs(shuffledCharacterIDs);

	for (int shuffledIndex = 0; shuffledIndex < GAME_CHARACTER_COUNT && botIndex < botCount; shuffledIndex++)
	{
		s16 characterID = shuffledCharacterIDs[shuffledIndex];

		if (LOAD_IsCharacterUsedByPlayer(characterID))
		{
			continue;
		}

		if (g_config.botSelectionMode == BOT_SELECTION_RANDOM_UNLOCKED && !MM_Characters_IsCharacterUnlocked(characterID))
		{
			continue;
		}

		data.characterIDs[sdata->gGT->numPlyrCurrGame + botIndex] = characterID;

		botIndex++;
	}
}

static void LOAD_QueueRandomBotModels(struct BigHeader *bigfile)
{
	b32 extendedMultiplayerGrid = LOAD_UsesExtendedMultiplayerGrid();
	s16 standaloneCharacterIDs[LOAD_EXTRA_CHARACTER_MODEL_CAPACITY];
	int botCount = LOAD_GetRandomBotCount();
	int standaloneCharacterCount = 0;
	int modelFileIndex = sdata->gGT->numPlyrCurrGame == 2 && !extendedMultiplayerGrid ? BI_RACERMODELMED : BI_RACERMODELHI;

	for (int botIndex = 0; botIndex < botCount; botIndex++)
	{
		s16 characterID = data.characterIDs[sdata->gGT->numPlyrCurrGame + botIndex];

		// A randomized bot can land on any of the sixteen racers, while the
		// 1P arcade pack only holds the player plus its own AI set, so ask
		// LOAD_RacerNeedsStandaloneModel rather than assuming the pack covers
		// every base racer. The regular 2P pack has only its predefined AI set,
		// so every randomized 2P bot needs a standalone model regardless.
		if (LOAD_RacerNeedsStandaloneModel(characterID) || (sdata->gGT->numPlyrCurrGame == 2 && !extendedMultiplayerGrid))
		{
			standaloneCharacterIDs[standaloneCharacterCount++] = characterID;
		}
	}

	LOAD_QueueExtraCharacterModels(bigfile, standaloneCharacterIDs, standaloneCharacterCount, modelFileIndex);
}

struct Model *LOAD_GetExtraCharacterModelByName(char *searchName)
{
	for (int i = 0; i < s_extraCharacterModelCount; i++)
	{
		if (strcmp(GAME_CHARACTER_METADATA[s_extraCharacterModelIDs[i]].name_Debug, searchName) == 0)
		{
			return s_extraCharacterModels[i].model;
		}
	}

	return NULL;
}

void LOAD_FinalizeExtraCharacterModels(void)
{
	for (int i = 0; i < s_extraCharacterModelCount; i++)
	{
		if (s_extraCharacterModels[i].fileBase != NULL)
		{
			s_extraCharacterModels[i].model = (struct Model *)((u8 *)s_extraCharacterModels[i].fileBase + LOAD_MODEL_FILE_HEADER_BYTES);
		}
	}
}

void LOAD_RunPtrMap(char *origin, int *patchArr, int numPtrs)
{
	int *ptrCurrOffset = patchArr;

	for (ptrCurrOffset = &patchArr[0]; ptrCurrOffset < &patchArr[numPtrs]; ptrCurrOffset++)
	{
		int offset = (*ptrCurrOffset >> 2) << 2;
		*(int *)&origin[offset] = *(int *)&origin[offset] + (int)origin;
#if defined(CTR_INTERNAL)
		NativeCheckpoint_RegisterPointerSlot(&origin[offset]);
#endif
	}
}

static int LOAD_SelectRobots2P(int p1, int p2)
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
		return -1;
	}

	data.characterIDs[2] = robotSet[0];
	data.characterIDs[3] = robotSet[1];
	data.characterIDs[4] = robotSet[2];
	data.characterIDs[5] = robotSet[3];
	return setIndex;
}

void LOAD_Robots2P(struct BigHeader *bigfile, int p1, int p2, void (*callback)(struct LoadQueueSlot *))
{
	int setIndex = LOAD_SelectRobots2P(p1, p2);
	if (setIndex < 0)
	{
		return;
	}

	LOAD_AppendQueue(bigfile, LT_GETADDR, BI_2PARCADEPACK + setIndex, NULL, callback);
}

void LOAD_Robots1P(int characterID)
{
	int nextCharacterID = 0;

	data.characterIDs[0] = characterID;

	for (int driverID = LOAD_HumanDriverSlotCount(); driverID < LOAD_CHARACTER_ID_COUNT; driverID++)
	{
		while (LOAD_IsCharacterUsedByPlayer(nextCharacterID))
		{
			nextCharacterID++;
		}

		data.characterIDs[driverID] = nextCharacterID++;
	}
}

// LOAD_Robots1P walks the base roster upwards, so with one player it stops
// before Pura and every bot is covered by the 1P arcade pack. With two or more
// players the skipped player entries push the last bot onto a racer the pack
// does not carry, so give those bots their own model file.
static void LOAD_QueueExtendedBotModels(struct BigHeader *bigfile)
{
	struct GameTracker *gGT = sdata->gGT;
	s16 standaloneCharacterIDs[LOAD_EXTRA_CHARACTER_MODEL_CAPACITY];
	int standaloneCharacterCount = 0;

	for (int driverID = gGT->numPlyrCurrGame; driverID < LOAD_CHARACTER_ID_COUNT; driverID++)
	{
		s16 characterID = data.characterIDs[driverID];

		if (LOAD_RacerNeedsStandaloneModel(characterID))
		{
			standaloneCharacterIDs[standaloneCharacterCount++] = characterID;
		}
	}

	LOAD_QueueExtraCharacterModels(bigfile, standaloneCharacterIDs, standaloneCharacterCount, BI_RACERMODELHI);
}

static void (*const LOAD_DriverMPK_SetPointer)(struct LoadQueueSlot *) = LOAD_QUEUE_CALLBACK_SET_POINTER;

int LOAD_DriverMPK(struct BigHeader *bigfile, int levelLOD, void (*callback)(struct LoadQueueSlot *))
{
	int i;
	int gameMode1;
	b32 extendedMultiplayerGrid;
	b32 randomBotRace;

	struct GameTracker *gGT = sdata->gGT;
	gameMode1 = gGT->gameMode1;
	extendedMultiplayerGrid = LOAD_UsesExtendedMultiplayerGrid();

	randomBotRace = LOAD_IsRandomBotRace();
	LOAD_ResetExtraCharacterModels();
	if (randomBotRace)
	{
		if (!LOAD_ShouldPreserveRandomBotRoster())
		{
			LOAD_SelectRandomBots();
		}

		LOAD_QueueRandomBotModels(bigfile);
	}

	if (extendedMultiplayerGrid && gGT->numPlyrCurrGame > 1)
	{
		// The 1P arcade pack contains the full racer roster for AI opponents.
		if (!randomBotRace)
		{
			LOAD_Robots1P(data.characterIDs[0]);
			LOAD_QueueExtendedBotModels(bigfile);
		}
	}

	int lastFileIndexMPK;
	if (LOAD_IsAdventureCoopBossRace())
	{
		// A co-op boss race still only runs the players and the boss, so it takes
		// the boss pack a solo race uses instead of an AI roster. This has to be
		// decided before the split-screen branches below, because those hand the
		// AI driver slots to the 2P lineup and would replace the boss.
		// Player 1 keeps the solo boss detail, the partner takes the detail a
		// split-screen racer of their position has everywhere else.
		LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELHI + data.characterIDs[0], &data.driverModelExtras[0].fileBase,
		                 LOAD_DriverMPK_SetPointer);

		int partnerModelFile = sdata->highDetailSplitScreenLevel ? BI_RACERMODELHI : BI_RACERMODELMED;
		for (i = 1; (i < gGT->numPlyrCurrGame) && (i < LOAD_DRIVER_MODEL_EXTRA_COUNT); i++)
		{
			LOAD_AppendQueue(bigfile, LT_GETADDR, partnerModelFile + data.characterIDs[i], &data.driverModelExtras[i].fileBase,
			                 LOAD_DriverMPK_SetPointer);
		}

		lastFileIndexMPK = BI_TIMETRIALPACK + data.characterIDs[LOAD_AdventureBossDriverSlot()];
		goto QueueLastPack;
	}

	if (RaceField_IsPurpleGemCupRace(gameMode1, gGT->cup.cupID))
	{
		// The cup packs its four bosses into the AI driver slots, so this has to be
		// decided before the split-screen branches below, because those hand those
		// slots to a generic 2P lineup and would replace the bosses.
		// Player 1 keeps the solo cup detail, the partner takes the detail a
		// split-screen racer of their position has everywhere else.
		LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELHI + data.characterIDs[0], &data.driverModelExtras[0].fileBase,
		                 LOAD_DriverMPK_SetPointer);

		int partnerModelFile = sdata->highDetailSplitScreenLevel ? BI_RACERMODELHI : BI_RACERMODELMED;
		for (i = 1; (i < gGT->numPlyrCurrGame) && (i < LOAD_DRIVER_MODEL_EXTRA_COUNT); i++)
		{
			LOAD_AppendQueue(bigfile, LT_GETADDR, partnerModelFile + data.characterIDs[i], &data.driverModelExtras[i].fileBase,
			                 LOAD_DriverMPK_SetPointer);
		}

		LOAD_SelectPurpleGemCupBosses();

		// pack of four AIs with bosses
		lastFileIndexMPK = BI_2PARCADEPACK + LOAD_PURPLE_GEM_CUP_AI_SET_INDEX;
		goto QueueLastPack;
	}

	if (sdata->highDetailSplitScreenLevel)
	{
		// The 1P arcade pack contains P1 at player quality. Use the standalone
		// slots for P2-P4 instead of redundantly loading P1 again.
		for (i = 1; (i < gGT->numPlyrCurrGame) && (i <= LOAD_DRIVER_MODEL_EXTRA_COUNT); i++)
		{
			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELHI + data.characterIDs[i], &data.driverModelExtras[i - 1].fileBase, LOAD_DriverMPK_SetPointer);
		}

		if (!randomBotRace && !extendedMultiplayerGrid && (gGT->numPlyrCurrGame == 2) && (LOAD_SelectRobots2P(data.characterIDs[0], data.characterIDs[1]) < 0))
		{
			return sdata->ptrMPK;
		}

		lastFileIndexMPK = BI_1PARCADEPACK + data.characterIDs[0];
		goto QueueLastPack;
	}

	// 3P/4P
	if ((u32)(levelLOD - LOAD_LEVEL_LOD_3P) < LOAD_LEVEL_LOD_3P4P_COUNT)
	{
		if (extendedMultiplayerGrid)
		{
			// P1 and all AI models come from the arcade pack. The standalone slots
			// provide the remaining human selections at multiplayer LOD.
			for (i = 1; i < gGT->numPlyrCurrGame; i++)
			{
				// low lod CTR model
				LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELLOW + data.characterIDs[i], &data.driverModelExtras[i - 1].fileBase,
				                 LOAD_DriverMPK_SetPointer);
			}

			lastFileIndexMPK = BI_1PARCADEPACK + data.characterIDs[0];
		}
		else
		{
			for (i = 0; i < LOAD_DRIVER_MODEL_EXTRA_COUNT; i++)
			{
				LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELLOW + data.characterIDs[i], &data.driverModelExtras[i].fileBase,
				                 LOAD_DriverMPK_SetPointer);
			}
			// load 4P MPK of fourth player
			lastFileIndexMPK = BI_4PARCADEPACK + data.characterIDs[3];
		}
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

		if (!randomBotRace && ((gameMode1 & (TIME_TRIAL | MAIN_MENU)) != MAIN_MENU))
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

		// Load Player 1 [0]
		LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELHI + data.characterIDs[0], &data.driverModelExtras[0].fileBase, LOAD_DriverMPK_SetPointer);

		// The relic bundle carries a single level of detail and only ever shipped a
		// model for the first racer, so a co-op relic race has to pull the partner's
		// model in from the standalone multiplayer slots. A time trial only ever has
		// one racer, so this loop is skipped there and its second ID stays the ghost.
		for (i = 1; (i < gGT->numPlyrCurrGame) && (i < LOAD_DRIVER_MODEL_EXTRA_COUNT); i++)
		{
			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELMED + data.characterIDs[i], &data.driverModelExtras[i].fileBase, LOAD_DriverMPK_SetPointer);
		}

		// Load boss or ghost [1]
		lastFileIndexMPK = BI_TIMETRIALPACK + data.characterIDs[1];
	}

	// else if (levelLOD == LOAD_LEVEL_LOD_2P)
	else
	{
		if (extendedMultiplayerGrid)
		{
			// P1 and every AI model come from the 1P arcade pack. P2 keeps the
			// normal 2P medium-detail model.
			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELMED + data.characterIDs[1], &data.driverModelExtras[0].fileBase,
			                 LOAD_DriverMPK_SetPointer);
			lastFileIndexMPK = BI_1PARCADEPACK + data.characterIDs[0];
			goto QueueLastPack;
		}

		// med models
		for (i = 0; i < LOAD_MED_LOD_DRIVER_MODEL_EXTRA_COUNT; i++)
		{
			// med lod CTR model
			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_RACERMODELMED + data.characterIDs[i], &data.driverModelExtras[i].fileBase, LOAD_DriverMPK_SetPointer);
		}

		if (randomBotRace)
		{
			// This pack supplies the shared 2P arcade assets; bot models above
			// are loaded individually and do not need its predefined roster.
			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_2PARCADEPACK, NULL, callback);
		}
		else
		{
			LOAD_Robots2P(bigfile, data.characterIDs[0], data.characterIDs[1], callback);
		}
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
		sdata->lngFile = MEMPACK_AllocMem(sdata->langBufferSize, NULL /* "lang buffer" */);
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
