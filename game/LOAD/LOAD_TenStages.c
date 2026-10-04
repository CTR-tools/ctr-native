#include <common.h>

#ifdef CTR_NATIVE
enum
{
	LOAD_NATIVE_NDBOX_INTRO_SONG_SYNC_TIME = 0x11c0,
};

static void LOAD_NativeAudio_SetStateAfterBankReload(u32 state)
{
	int isSameLatchedState = sdata->audioState == (s16)state;
	int isStoppedSong0State = (state == AUDIO_STOP_ALL) || (state == AUDIO_ADV_HUB) || (state == AUDIO_GARAGE_ENTRY);

	if ((sdata->cseqBoolPlay == 0) && isSameLatchedState && isStoppedSong0State)
	{
		// NOTE(aalhendi): Native can arrive here after LOAD_TenStages
		// stopped song-0 CSEQ music for a bank reload while the retail
		// audio-state latch still matches. Re-enter the same CSEQ state
		// so post-load menu/hub music is started again.
		Voiceline_EmptyFunc();
		Audio_SetState(state);
		sdata->audioState = (s16)state;
		return;
	}

	Audio_SetState_Safe(state);
}
#endif

int LOAD_TenStages(struct GameTracker *unusedGameTracker, int loadingStage, struct BigHeader *incomingBigfile)
{
	register struct BigHeader *bigfile CTR_PSX_REGISTER("$20") = incomingBigfile;

	// NOTE(aalhendi): Retail receives a tracker in a0 but reloads the global
	// tracker throughout this routine. The native caller passes that same pointer.
	(void)unusedGameTracker;

	// if game is loading
	if (GAME_LOAD_IN_PROGRESS != 0)
	{
		return loadingStage;
	}

	switch (loadingStage)
	{
	case 0:
	{
		u8 nextPlayerCount;
		int newLevelID;
		int bookmarkID;
		struct GameTracker *renderTracker;
		register struct GameTracker *modeTracker CTR_PSX_REGISTER("$7");
		register struct GameTracker *visTracker CTR_PSX_REGISTER("$8");
		register struct GameTracker *introTracker CTR_PSX_REGISTER("$3");
		register struct GameTracker *introMode2Tracker CTR_PSX_REGISTER("$5");
		register struct GameTracker *lodTracker CTR_PSX_REGISTER("$5");
		register struct GameTracker *primTracker CTR_PSX_REGISTER("$4");
		register u32 lodGameMode1 CTR_PSX_REGISTER("$4");
		register u32 lod CTR_PSX_REGISTER("$3");
		register u32 lodMaskResult CTR_PSX_REGISTER("$2");
		register char *prefixPage CTR_PSX_REGISTER("$5");

		if ((GAME_TRACKER->levelID != ADVENTURE_GARAGE) && (GAME_TRACKER->levelID != NAUGHTY_DOG_CRATE))
		{
			Cutscene_VolumeBackup();
		}
		CDSYS_XAPauseRequest();

		// On later loads, restore the main pack before clearing the prior level.
		if (sdata->boolFirstBoot == 0)
		{
			MEMPACK_SwapPacks(LOAD_MAIN_PACK_INDEX);

			newLevelID = MainInit_StringToLevID(GAME_TRACKER->levelName);
			bookmarkID = sdata->bookmarkID;
			sdata->levelID = newLevelID;
			CTR_PSX_MEMORY_BARRIER();

			MEMPACK_PopToState(bookmarkID);
		}
		// First boot: SCEA, copyright, and the Naughty Dog box.
		else
		{
			u32 vramSize;

			sdata->boolFirstBoot = 0;

			// Load Intro TIM for Copyright Page from VRAM file
			LOAD_VramFile(bigfile, LOAD_FIRST_BOOT_COPYRIGHT_TIM_BIGFILE_INDEX, NULL, &vramSize, -1);
			MainInit_VRAMDisplay();

#ifdef CTR_NATIVE
			// NOTE(aalhendi): SCEA is already held by XA playback in MainMain. The copyright
			// TIM has no XA, so keep it visible until the intro CSEQ reaches
			// the point retail normally reaches while loading the ND crate.
			// Present every wait tick so both host swapchain images are
			// overwritten with copyright instead of briefly revealing SCEA.
			while (((sdata->songPool[0].flags & 3) == 1) && (sdata->songPool[0].timeSpentPlaying < LOAD_NATIVE_NDBOX_INTRO_SONG_SYNC_TIME))
			{
				VSync(0);
				Platform_PresentVRAMDisplay();
			}
#endif

			GAME_TRACKER->db[0].drawEnv.isbg = 0;
			GAME_TRACKER->db[1].drawEnv.isbg = 0;
		}

		renderTracker = GAME_TRACKER;
		renderTracker->hudFlags &= HUD_FLAG_CLEAR_RACE_HUD_MASK;
		CTR_PSX_MEMORY_BARRIER();
		renderTracker->renderFlags &= RENDER_FLAG_CHECKERED_FLAG;
		GAME_TRACKER_RELOAD()->hudFlags &= HUD_FLAG_CLEAR_INTRO_RACE_TITLE_BARS_MASK;

		nextPlayerCount = GAME_TRACKER->numPlyrNextGame;
		GAME_TRACKER->level1 = 0;
		GAME_TRACKER->level2 = 0;
		GAME_TRACKER->numPlyrCurrGame = nextPlayerCount;
		strcpy(GAME_TRACKER->levelName, GAME_LEVEL_METADATA[GAME_TRACKER->levelID].name_Debug);

		// pop back here for every load, after first load,
		// this permanently reserves LNG, bigfile header, etc
		sdata->bookmarkID = MEMPACK_PushState();
		CTR_PSX_MEMORY_BARRIER();

		// Required for Scrapbook "Press Start",
		// may also be required for other edge-cases
		DrawSync(0);
		modeTracker = GAME_TRACKER;
		modeTracker->overlayTransition = 0;

		// ========== Start of flags ===============


		// disable certain game mode flags
		modeTracker->gameMode1 &= ~(GAME_CUTSCENE | END_OF_RACE | ADVENTURE_ARENA | MAIN_MENU);
		modeTracker->gameMode2 &= ~(LEV_SWAP | CREDITS | NO_LEV_INSTANCE);
		visTracker = GAME_TRACKER_RELOAD();
		visTracker->visMem1 = 0;
		visTracker->visMem2 = 0;

		// NOTE(aalhendi): Retail starts each prefix address in the preceding branch
		// delay slot. The page/low split preserves that schedule; native still reads
		// the shared prefix strings directly.
		if ((strncmp(visTracker->levelName, GAME_LEVEL_PREFIX_NDI, LOAD_LEVEL_PREFIX_NDI_LENGTH) == 0) ||
		    CTR_PSX_WITH_RETAIL_PAGE(prefixPage, RETAIL_LOAD_PREFIX_PAGE, RETAIL_LOAD_PREFIX_ENDING_LOW, GAME_LEVEL_PREFIX_ENDING,
		                             strncmp(GAME_TRACKER->levelName, prefixPage, LOAD_LEVEL_PREFIX_ENDING_LENGTH) == 0))
		{
			GAME_TRACKER->gameMode1 |= GAME_CUTSCENE;
			GAME_TRACKER->hudFlags &= HUD_FLAG_CLEAR_RACE_HUD_MASK;
		}
		else if (CTR_PSX_WITH_RETAIL_PAGE(prefixPage, RETAIL_LOAD_PREFIX_PAGE, RETAIL_LOAD_PREFIX_INTRO_LOW, GAME_LEVEL_PREFIX_INTRO,
		                                  strncmp(GAME_TRACKER->levelName, prefixPage, LOAD_LEVEL_PREFIX_INTRO_LENGTH) == 0))
		{
			introTracker = GAME_TRACKER;
			introTracker->hudFlags &= HUD_FLAG_CLEAR_RACE_HUD_MASK;
			introTracker->gameMode1 |= GAME_CUTSCENE;
			introMode2Tracker = GAME_TRACKER_RELOAD();
			introMode2Tracker->gameMode2 |= LEV_SWAP;
		}
		else if (CTR_PSX_WITH_RETAIL_PAGE(prefixPage, RETAIL_LOAD_PREFIX_PAGE, RETAIL_LOAD_PREFIX_SCREEN_LOW, GAME_LEVEL_PREFIX_SCREEN,
		                                  strncmp(GAME_TRACKER->levelName, prefixPage, LOAD_LEVEL_PREFIX_SCREEN_LENGTH) == 0) ||
		         CTR_PSX_WITH_RETAIL_PAGE(prefixPage, RETAIL_LOAD_PREFIX_PAGE, RETAIL_LOAD_PREFIX_GARAGE_LOW, GAME_LEVEL_PREFIX_GARAGE,
		                                  strncmp(GAME_TRACKER->levelName, prefixPage, LOAD_LEVEL_PREFIX_GARAGE_LENGTH) == 0))
		{
			register struct GameTracker *screenTracker CTR_PSX_REGISTER("$4") = GAME_TRACKER;
			register char *garagePrefix CTR_PSX_REGISTER("$5");
			register u8 hudFlags CTR_PSX_REGISTER("$3") = screenTracker->hudFlags;
			register u32 gameMode1 CTR_PSX_REGISTER("$2");
			CTR_PSX_LOAD_SYMBOL_PAGE(garagePrefix, RETAIL_LOAD_PREFIX_GARAGE_ASM_NAME);
			CTR_PSX_OBSERVE_VALUE(garagePrefix);
			CTR_PSX_LOAD_WORD(gameMode1, screenTracker->gameMode1);
			screenTracker->hudFlags = hudFlags & HUD_FLAG_CLEAR_RACE_HUD_MASK;
			screenTracker->gameMode1 = gameMode1 | MAIN_MENU;
			GAME_TRACKER->numPlyrNextGame = GAME_TRACKER->numPlyrCurrGame;
			GAME_TRACKER->numPlyrCurrGame = 4;

			CTR_PSX_ADD_SYMBOL_LOW_IN_PLACE(garagePrefix, RETAIL_LOAD_PREFIX_GARAGE_ASM_NAME, GAME_LEVEL_PREFIX_GARAGE);
			if (strncmp(GAME_TRACKER->levelName, garagePrefix, LOAD_LEVEL_PREFIX_GARAGE_LENGTH) == 0)
			{
				GAME_TRACKER->numPlyrCurrGame = 1;
				GAME_MAIN_MENU_STATE = MAIN_MENU_ADVENTURE;
			}
		}
		else if (CTR_PSX_WITH_RETAIL_PAGE(prefixPage, RETAIL_LOAD_PREFIX_PAGE, RETAIL_LOAD_PREFIX_HUB_LOW, GAME_LEVEL_PREFIX_HUB,
		                                  strncmp(GAME_TRACKER->levelName, prefixPage, LOAD_LEVEL_PREFIX_HUB_LENGTH) == 0))
		{
			struct GameTracker *countTracker = GAME_TRACKER;
			countTracker->numPlyrNextGame = 1;
			countTracker->numPlyrCurrGame = 1;
			GAME_TRACKER->gameMode1 |= ADVENTURE_ARENA;
			GAME_TRACKER->gameMode2 |= LEV_SWAP;
			GAME_TRACKER->hudFlags &= HUD_FLAG_CLEAR_RACE_HUD_MASK;
		}
		else if (CTR_PSX_WITH_RETAIL_PAGE(prefixPage, RETAIL_LOAD_PREFIX_PAGE, RETAIL_LOAD_PREFIX_CREDIT_LOW, GAME_LEVEL_PREFIX_CREDIT,
		                                  strncmp(GAME_TRACKER->levelName, prefixPage, LOAD_LEVEL_PREFIX_CREDIT_LENGTH) == 0))
		{
			struct GameTracker *countTracker = GAME_TRACKER;
			countTracker->numPlyrNextGame = 1;
			countTracker->numPlyrCurrGame = 1;
			countTracker->gameMode1 |= GAME_CUTSCENE;
			GAME_TRACKER->gameMode2 |= (LEV_SWAP | CREDITS);
			GAME_TRACKER->hudFlags &= HUD_FLAG_CLEAR_RACE_HUD_MASK;
		}
		else
		{
			GAME_TRACKER->hudFlags &= HUD_FLAG_CLEAR_RACE_HUD_MASK;
		}
		GAME_TRACKER->hudFlags |= HUD_FLAG_INIT_UI_INSTANCES;
		GAME_TRACKER->Debug_ToggleNormalSpawn = 1;


		// ========== End of setting numPlyr ================
		// ========== Set LevelLOD variables ================


		// main menu or adv garage
		lodTracker = GAME_TRACKER;
		lodGameMode1 = lodTracker->gameMode1;
		if ((lodGameMode1 & MAIN_MENU) != 0)
		{
			lod = LOAD_LEVEL_LOD_1P;
		}
		// if relic, or time trial
		else if (CTR_PSX_AND_MASK_NONZERO(lodMaskResult, TIME_TRIAL | RELIC_RACE, lodGameMode1))
		{
			lod = LOAD_LEVEL_LOD_RELIC;
		}
		else
		{
			lod = lodTracker->numPlyrCurrGame;
		}
		primTracker = GAME_TRACKER_RELOAD();
		sdata->levelLOD = lod;
		CTR_PSX_MEMORY_BARRIER();

		// ========== End of LevelLOD ================
		// ========== Alloc Prim + OT ================


		// OG game
		MainInit_PrimMem(primTracker);
		MainInit_OTMem(GAME_TRACKER);

		if (((GAME_TRACKER->gameMode1 & (GAME_CUTSCENE | ADVENTURE_ARENA)) != 0) || ((GAME_TRACKER->gameMode2 & CREDITS) != 0))
		{
			loadingStage++;
			MainInit_JitPoolsNew(GAME_TRACKER);
			CTR_PSX_CLOBBER("$3");
			return loadingStage;
		}

		goto advance_stage;
	}
	case 1:
	{
		register int ovrRegion1 CTR_PSX_REGISTER("$4");
		register struct GameTracker *tracker CTR_PSX_REGISTER("$4");
		register u32 mode1 CTR_PSX_REGISTER("$3");
		register u32 adventureMode CTR_PSX_REGISTER("$2");

		// if XA has not paused since CDSYS_XAPauseRequest in stage #0,
		// then quit the function and try again next frame
		if (GAME_XA_STATE == XA_FADING)
		{
			return loadingStage;
		}

		tracker = GAME_TRACKER;
		mode1 = tracker->gameMode1;

		if ((mode1 & CRYSTAL_CHALLENGE) != 0)
		{
			ovrRegion1 = 0;
		}
		else if ((mode1 & TIME_TRIAL) != 0)
		{
			ovrRegion1 = 3;
		}
		else if ((mode1 & ARCADE_MODE) != 0)
		{
			CTR_PSX_CLOBBER("$7");
			ovrRegion1 = 1;
		}
		else if ((mode1 & RELIC_RACE) != 0)
		{
			ovrRegion1 = 2;
		}
		else if ((adventureMode = mode1 & ADVENTURE_MODE) != 0)
		{
			ovrRegion1 = 1;
		}
		else
		{
			if ((tracker->gameMode2 & CUP_ANY_KIND) != 0)
			{
				goto advance_stage;
			}
			ovrRegion1 = 4;
		}

		loadingStage++;
		LOAD_OvrEndRace(ovrRegion1);
		return loadingStage;
	}
	case 2:
	{
		LOAD_OvrLOD(GAME_TRACKER->numPlyrCurrGame);
		CTR_PSX_MEMORY_BARRIER();
	}
	advance_stage:
		loadingStage++;
		CTR_PSX_OBSERVE_VALUE(loadingStage);
		return loadingStage;
	case 3:
	{
		int ovrRegion3;

		if (((GAME_TRACKER->gameMode1 & MAIN_MENU) != 0) && (GAME_TRACKER->levelID != ADVENTURE_GARAGE))
		{
			ovrRegion3 = 0;
		}
		else if ((GAME_TRACKER->gameMode1 & ADVENTURE_ARENA) != 0)
		{
			ovrRegion3 = 3;

			if (GAME_TRACKER->podiumRewardID == NOFUNC)
			{
				ovrRegion3 = 2;
			}
		}
		else if ((GAME_TRACKER->podiumRewardID != NOFUNC) || ((GAME_TRACKER->gameMode1 & GAME_CUTSCENE) != 0) || ((GAME_TRACKER->gameMode2 & CREDITS) != 0) ||
		         (GAME_TRACKER->levelID == ADVENTURE_GARAGE))
		{
			ovrRegion3 = 3;
		}
		else
		{
			register int racingRegion CTR_PSX_REGISTER("$2") = 1;

			if (GAME_TRACKER->overlayIndex_Threads == racingRegion)
			{
				goto advance_stage;
			}
			ovrRegion3 = racingRegion;
		}

		loadingStage++;
		LOAD_OvrThreads(ovrRegion3);
		return loadingStage;
	}
	case 4:
	{
		register struct BigHeader *driverBigfile CTR_PSX_REGISTER("$4");
		register void (*driverCallback)(struct LoadQueueSlot *) CTR_PSX_REGISTER("$6");
		register int driverLod CTR_PSX_REGISTER("$5");
		if ((GAME_TRACKER->levelID != ADVENTURE_GARAGE) && (GAME_TRACKER->levelID != NAUGHTY_DOG_CRATE))
		{
			Music_Restart();
		}

		// If in main menu (character selection, track selection, any part of it)
		if ((GAME_TRACKER->gameMode1 & MAIN_MENU) != 0)
		{
			switch (GAME_MAIN_MENU_STATE)
			{
			case MAIN_MENU_ADVENTURE:
			{
				CS_Garage_Init();
				break;
			}
			case MAIN_MENU_TITLE:
			{
				MM_JumpTo_Title_FirstTime();
				break;
			}
			case MAIN_MENU_CHARACTERS:
			{
				MM_JumpTo_Characters();
				break;
			}
			case MAIN_MENU_TRACK_SELECT:
			{
				MM_JumpTo_TrackSelect();
				break;
			}
			case MAIN_MENU_BATTLE_SETUP:
			{
				MM_JumpTo_BattleSetup();
				break;
			}
			case MAIN_MENU_SCRAPBOOK:
			{
				MM_JumpTo_Scrapbook();
				break;
			}
			}
		}

		driverBigfile = bigfile;
		CTR_PSX_OBSERVE_VALUE(driverBigfile);
		CTR_PSX_LOAD_SYMBOL_PAGE(driverCallback, RETAIL_LOAD_DRIVER_CALLBACK_ASM_NAME);
		CTR_PSX_OBSERVE_VALUE(driverCallback);
		driverLod = sdata->levelLOD;
		CTR_PSX_OBSERVE_VALUE(driverLod);
		// Clear driver extras
		GAME_DRIVER_MODEL_EXTRAS[0].fileBase = NULL;
		GAME_DRIVER_MODEL_EXTRAS[1].fileBase = NULL;
		GAME_DRIVER_MODEL_EXTRAS[2].fileBase = NULL;

		// Needed, or else Post-Boss Outro will break character animations.
		sdata->ptrMPK = 0;

		// NOTE(aalhendi): Retail gates stage advancement until the driver MPK callback sets ptrMPK.
		sdata->load_inProgress = 1;
		CTR_PSX_MEMORY_BARRIER();
		CTR_PSX_ADD_PAGE_OFFSET(driverCallback, driverCallback, RETAIL_LOAD_DRIVER_CALLBACK_LOW, LOAD_Callback_DriverModels);
		LOAD_DriverMPK(driverBigfile, driverLod, driverCallback);
		goto advance_stage;
	}
	case 5:
	{
		struct LevTexLookup *icons;
		register void *mpkForIcons CTR_PSX_REGISTER("$5");
		// clear and reset
		LibraryOfModels_Clear(GAME_TRACKER);

		if (sdata->ptrMPK != 0)
		{
			GAME_PLAYER_OBJECT_LIST = (struct Model **)((u32)sdata->ptrMPK + 4);
		}
		else
		{
			GAME_PLAYER_OBJECT_LIST = 0;
		}

		LOAD_GlobalModelPtrs_MPK();
		DecalGlobal_Clear(GAME_TRACKER);

		mpkForIcons = (void *)sdata->ptrMPK;
		if ((mpkForIcons != 0) && ((icons = *(struct LevTexLookup **)mpkForIcons) != 0))
		{
			DecalGlobal_Store(GAME_TRACKER, icons);
			GAME_TRACKER->mpkIcons = *(int *)sdata->ptrMPK;
		}
		else
		{
			GAME_TRACKER->mpkIcons = 0;
		}

		if ((GAME_TRACKER->levelID != ADVENTURE_GARAGE) && (GAME_TRACKER->levelID != NAUGHTY_DOG_CRATE))
		{
			loadingStage++;
			Music_Stop();
			CseqMusic_StopAll();
			Music_LoadBanks();
			return loadingStage;
		}

		goto advance_stage;
	}
	case 6:
	{
		int i;
		register int queuedFileIndex CTR_PSX_REGISTER("$6");
		register int queueType CTR_PSX_REGISTER("$5");
		register struct BigHeader *queueBigfile CTR_PSX_REGISTER("$4");
		void *queueDestination;
		void (*queueCallback)(struct LoadQueueSlot *);
		struct GameTracker *packTracker;
		void *allocatedPatchMem;
		register struct GameTracker *packForSwap CTR_PSX_REGISTER("$3");
		register int activePackForSwap CTR_PSX_REGISTER("$4");
		register struct GameTracker *levelTracker CTR_PSX_REGISTER("$2");
		register int queueLevelID CTR_PSX_REGISTER("$4");
		register int queueLod CTR_PSX_REGISTER("$5");
		register int patchMemSize CTR_PSX_REGISTER("$4");
		register char *patchMemoryLabel CTR_PSX_REGISTER("$5");

		if ((GAME_TRACKER->levelID != ADVENTURE_GARAGE) && (GAME_TRACKER->levelID != NAUGHTY_DOG_CRATE))
		{
			int banksReady = Music_AsyncParseBanks();

			if (banksReady == 0)
			{
				// quit and restart stage 6 next frame
				return loadingStage;
			}

			Cutscene_VolumeRestore();
		}

		for (i = 0; i < LOAD_DRIVER_MODEL_EXTRA_COUNT; i++)
		{
			if (GAME_DRIVER_MODEL_EXTRAS[i].fileBase != NULL)
			{
				GAME_DRIVER_MODEL_EXTRAS[i].model = (struct Model *)((u8 *)GAME_DRIVER_MODEL_EXTRAS[i].fileBase + LOAD_MODEL_FILE_HEADER_BYTES);
			}
		}

		// == banks are done parsing ===

		// If this world is made of multiple LEVs
		if ((GAME_TRACKER->gameMode2 & LEV_SWAP) != 0)
		{
			// Cutscene Packs
			register int firstSubpackSize CTR_PSX_REGISTER("$18") = LOAD_CUTSCENE_FIRST_PACK_BYTES;
			register int secondSubpackSize CTR_PSX_REGISTER("$17");
			u8 *hubAlloc;
			register s16 activeSubpackIndex CTR_PSX_REGISTER("$2");
			register int mainPackIndex CTR_PSX_REGISTER("$4");
			int isAdventureArena = GAME_TRACKER->gameMode1 & ADVENTURE_ARENA;
			register struct GameTracker *packStoreTracker CTR_PSX_REGISTER("$16");

			if (isAdventureArena != 0)
			{
				firstSubpackSize = LOAD_ADV_ARENA_FIRST_PACK_BYTES;
			}
			secondSubpackSize = LOAD_CUTSCENE_SECOND_PACK_BYTES;
			if (isAdventureArena != 0)
			{
				secondSubpackSize = LOAD_ADV_ARENA_SECOND_PACK_BYTES;
			}

			// Allocate room for LEV swapping
			hubAlloc = MEMPACK_AllocMem(firstSubpackSize + secondSubpackSize, GAME_RDATA_NAME(s_HUB_ALLOC));
			sdata->ptrHubAlloc = hubAlloc;
			CTR_PSX_MEMORY_BARRIER();

			// Change active allocation system to #2
			// pack = [hubAlloc, hubAlloc+size1]
			MEMPACK_SwapPacks(LOAD_FIRST_SUBPACK_INDEX);
			MEMPACK_NewPack(hubAlloc, firstSubpackSize);

			// Change active allocation system to #3
			// pack = [hubAlloc+size1, hubAlloc+size1+size2]
			MEMPACK_SwapPacks(LOAD_SECOND_SUBPACK_INDEX);
			MEMPACK_NewPack(hubAlloc + firstSubpackSize, secondSubpackSize);
			packStoreTracker = GAME_TRACKER;

			// If you're in Adventure Arena
			if ((packStoreTracker->gameMode1 & ADVENTURE_ARENA) != 0)
			{
				// Get 1 or 2, depending on map
				activeSubpackIndex = LOAD_GetAdvPackIndex();

				// Then swap:
				// Turn 1 into 2
				// Turn 2 into 1
				activeSubpackIndex = LOAD_HUB_MEMPACK_PAIR_INDEX_SUM - activeSubpackIndex;
			}
			// Intro cutscene with oxide spaceship and all racers
			else
			{
				// Always start with pool 1
				activeSubpackIndex = LOAD_FIRST_SUBPACK_INDEX;
			}

			// keep track of subpack levels
			packTracker = GAME_TRACKER_RELOAD();
			CTR_PSX_ZERO_VALUE_AFTER(mainPackIndex, packTracker, activeSubpackIndex);
			packStoreTracker->activeMempackIndex = activeSubpackIndex;
			CTR_PSX_MEMORY_BARRIER();
			packTracker->levID_in_each_mempack[packTracker->activeMempackIndex] = packTracker->levelID;
			packTracker->levID_in_each_mempack[LOAD_HUB_MEMPACK_PAIR_INDEX_SUM - packTracker->activeMempackIndex] = LOAD_NO_LEVEL_IN_MEMPACK;

			// the rest of memory will load pointer maps,
			// loaded at HighMem in main pack, end of RAM,
			// so the pointer maps dont bloat subpacks
			MEMPACK_SwapPacks(mainPackIndex);

			patchMemSize = MEMPACK_GetFreeBytes();
			CTR_PSX_LOAD_SYMBOL_PAGE(patchMemoryLabel, RETAIL_LOAD_PATCH_MEMORY_ASM_NAME);
			CTR_PSX_OBSERVE_VALUE(patchMemoryLabel);
			sdata->PatchMem_Size = patchMemSize;
			CTR_PSX_MEMORY_BARRIER();
			CTR_PSX_ADD_PAGE_OFFSET(patchMemoryLabel, patchMemoryLabel, RETAIL_LOAD_PATCH_MEMORY_LOW, GAME_RDATA_NAME(s_Patch_Table_Memory));
			allocatedPatchMem = MEMPACK_AllocHighMem(patchMemSize, patchMemoryLabel);

			// For Oxide-Intro and Credits, set active pack
			packForSwap = GAME_TRACKER_RELOAD();
			activePackForSwap = packForSwap->activeMempackIndex;
			sdata->PatchMem_Ptr = allocatedPatchMem;
			CTR_PSX_MEMORY_BARRIER();
			MEMPACK_SwapPacks(activePackForSwap);
		}

		// add VRAM to loading queue
		levelTracker = GAME_TRACKER_RELOAD();
		queueLevelID = levelTracker->levelID;
		queueLod = sdata->levelLOD;
		// NOTE(aalhendi): Retail sets the load gate before queueing level files.
		sdata->load_inProgress = 1;
		CTR_PSX_MEMORY_BARRIER();
		LOAD_AppendQueue(bigfile, LT_VRAM, LOAD_GetBigfileIndex(queueLevelID, queueLod, LVI_VRAM), NULL, NULL);

		if (((u32)(GAME_TRACKER->levelID - GEM_STONE_VALLEY) < LOAD_PTR_MAP_ADV_LEVEL_COUNT) ||
		    ((u32)(GAME_TRACKER->levelID - CREDITS_CRASH) < LOAD_PTR_MAP_CREDIT_LEVEL_COUNT))
		{
			LOAD_AppendQueue(bigfile, LT_GETADDR, LOAD_GetBigfileIndex(GAME_TRACKER->levelID, sdata->levelLOD, LVI_LEV), NULL, LOAD_Callback_LEV);

			// add PTR file to loading queue
			{
				int indexResult = LOAD_GetBigfileIndex(GAME_TRACKER->levelID, sdata->levelLOD, LVI_PTR);
				// NOTE(aalhendi): Keep the call-result move after the bigfile/type arguments.
				CTR_PSX_PREPARE_QUEUE_ARGS(queueBigfile, queueType, queuedFileIndex, bigfile, LT_SETADDR, indexResult);
			}
			queueDestination = sdata->PatchMem_Ptr;
			queueCallback = LOAD_Callback_PatchMem;
		}
		else
		{
			{
				int indexResult = LOAD_GetBigfileIndex(GAME_TRACKER->levelID, sdata->levelLOD, LVI_LEV);
				CTR_PSX_PREPARE_QUEUE_ARGS(queueBigfile, queueType, queuedFileIndex, bigfile, LT_GETADDR, indexResult);
			}
			queueDestination = NULL;
			queueCallback = LOAD_Callback_LEV;
		}
		LOAD_AppendQueue(queueBigfile, queueType, queuedFileIndex, queueDestination, queueCallback);
		goto advance_stage;
	}
	case 7:
	{
		// get level pointer
		int podiumPackIndex;
		struct Model **podiumModels;
		register int i CTR_PSX_REGISTER("$16");
		register int activePackIndex CTR_PSX_REGISTER("$2");
		int fileIndex;
		void (*setPtrCb)(struct LoadQueueSlot *);

		{
			struct GameTracker *levelStoreTracker = GAME_TRACKER;
			struct Level *lev = sdata->ptrLevelFile;
			levelStoreTracker->level1 = lev;
			levelStoreTracker->visMem1 = lev->visMem;
		}

		if (GAME_TRACKER->level1 != 0)
		{
			DecalGlobal_Store(GAME_TRACKER, GAME_TRACKER->level1->levTexLookup);
		}

		DebugFont_Init(GAME_TRACKER);

		// if level is not nullptr
		if (GAME_TRACKER->level1 != 0)
		{
			LibraryOfModels_Store(GAME_TRACKER, GAME_TRACKER->level1->numModels, GAME_TRACKER->level1->ptrModelsPtrArray);

			GAME_TRACKER->ptrCircle = (u32)DecalGlobal_FindInLEV(GAME_TRACKER->level1, GAME_RDATA_NAME(s_circle));
			GAME_TRACKER->ptrClod = (u32)DecalGlobal_FindInLEV(GAME_TRACKER->level1, GAME_RDATA_NAME(s_clod));
			GAME_TRACKER->ptrDustpuff = (u32)DecalGlobal_FindInLEV(GAME_TRACKER->level1, GAME_RDATA_NAME(s_dustpuff));
			GAME_TRACKER->ptrSmoking = (u32)DecalGlobal_FindInLEV(GAME_TRACKER->level1, GAME_RDATA_NAME(s_smokering)); // "Smoke Ring"
			GAME_TRACKER->ptrSparkle = (u32)DecalGlobal_FindInLEV(GAME_TRACKER->level1, GAME_RDATA_NAME(s_sparkle));
		}

		// if linked list of icons exists
		if (GAME_TRACKER->mpkIcons != 0)
		{
			GAME_TRACKER->trafficLights.all[0] = DecalGlobal_FindInMPK((struct Icon *)*(u32 *)(GAME_TRACKER->mpkIcons + 4), GAME_RDATA_NAME(s_lightredoff));
			GAME_TRACKER->trafficLights.all[1] = DecalGlobal_FindInMPK((struct Icon *)*(u32 *)(GAME_TRACKER->mpkIcons + 4), GAME_RDATA_NAME(s_lightredon));
			GAME_TRACKER->trafficLights.all[2] = DecalGlobal_FindInMPK((struct Icon *)*(u32 *)(GAME_TRACKER->mpkIcons + 4), GAME_RDATA_NAME(s_lightgreenoff));
			GAME_TRACKER->trafficLights.all[3] = DecalGlobal_FindInMPK((struct Icon *)*(u32 *)(GAME_TRACKER->mpkIcons + 4), GAME_RDATA_NAME(s_lightgreenon));
		}

		GAME_TRACKER->gameMode1_prevFrame = 1;

		if (((GAME_TRACKER->gameMode1 & (GAME_CUTSCENE | ADVENTURE_ARENA)) == 0) && ((GAME_TRACKER->gameMode2 & CREDITS) == 0))
		{
			loadingStage++;
			MainInit_JitPoolsNew(GAME_TRACKER);
			return loadingStage;
		}

		// podium reward
		if (GAME_TRACKER->podiumRewardID == NOFUNC)
		{
			goto advance_stage;
		}

		// === Assume PodiumReward Active ===

		i = LOAD_PODIUM_LAST_MODEL_SLOT;
		podiumModels = &data.podiumModel_firstPlace;
		CTR_PSX_KEEP_VALUE_RELAXED(podiumModels);
		podiumModels += LOAD_PODIUM_LAST_MODEL_SLOT;
		for (; i >= 0; i--)
		{
			*podiumModels-- = NULL;
		}

		// Load model+vrm files on the VRAM page that does not overwrite the hub.
		podiumPackIndex = LOAD_GetAdvPackIndex();
		activePackIndex = GAME_TRACKER->activeMempackIndex;
		MEMPACK_SwapPacks(LOAD_HUB_MEMPACK_PAIR_INDEX_SUM - activePackIndex);

		// NOTE(aalhendi): Retail gates stage advancement until
		// LOAD_Callback_Podiums runs after the final podium file.
		sdata->load_inProgress = 1;

		// VRAM for podium and all related models
		LOAD_AppendQueue(bigfile, LT_VRAM, BI_PODIUMVRMS - 1 + podiumPackIndex, NULL, NULL);

		// podium first place
		if ((GAME_TRACKER->podium_modelIndex_First != 0) && (GAME_TRACKER->podium_modelIndex_First != STATIC_OXIDEDANCE))
		{
			fileIndex = podiumPackIndex + (GAME_TRACKER->podium_modelIndex_First - STATIC_CRASHDANCE) * LOAD_PODIUM_MODEL_FILE_STRIDE + (BI_DANCEMODELWIN - 1);
			LOAD_AppendQueue(bigfile, LT_GETADDR, fileIndex, &data.podiumModel_firstPlace, LOAD_QUEUE_CALLBACK_SET_POINTER);
		}

		// podium second place
		if (GAME_TRACKER->podium_modelIndex_Second != 0)
		{
			fileIndex =
			    podiumPackIndex + (GAME_TRACKER->podium_modelIndex_Second - STATIC_CRASHDANCE) * LOAD_PODIUM_MODEL_FILE_STRIDE + (BI_DANCEMODELLOSE - 1);
			LOAD_AppendQueue(bigfile, LT_GETADDR, fileIndex, &data.podiumModel_secondPlace, LOAD_QUEUE_CALLBACK_SET_POINTER);
		}

		// podium third place
		if (GAME_TRACKER->podium_modelIndex_Third != 0)
		{
			fileIndex = podiumPackIndex + (GAME_TRACKER->podium_modelIndex_Third - STATIC_CRASHDANCE) * LOAD_PODIUM_MODEL_FILE_STRIDE + (BI_DANCEMODELLOSE - 1);
			LOAD_AppendQueue(bigfile, LT_GETADDR, fileIndex, &data.podiumModel_thirdPlace, LOAD_QUEUE_CALLBACK_SET_POINTER);
		}

		// TAWNA
		fileIndex = podiumPackIndex + (GAME_TRACKER->podium_modelIndex_tawna - STATIC_TAWNA1) * LOAD_PODIUM_MODEL_FILE_STRIDE + (BI_DANCETAWNAGIRL - 1);
		setPtrCb = LOAD_QUEUE_CALLBACK_SET_POINTER;

		// add TAWNA to loading queue
		LOAD_AppendQueue(bigfile, LT_GETADDR, fileIndex, (void *)&data.podiumModel_tawna, setPtrCb);

		// if 0x7e+5 (dingo)
		if (GAME_TRACKER->podium_modelIndex_First == STATIC_DINGODANCE)
		{
			// add "DingoFire" to loading queue
			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_DINGOFIRE - 1 + podiumPackIndex, (void *)&data.podiumModel_dingoFire, setPtrCb);
		}

		// add Podium
		LOAD_AppendQueue(bigfile, LT_GETADDR, BI_PODIUM - 1 + podiumPackIndex, NULL, LOAD_Callback_Podiums);

		// Disable LEV instances on Adv Hub, for podium scene
		GAME_TRACKER->gameMode2 = GAME_TRACKER->gameMode2 | NO_LEV_INSTANCE;
		goto advance_stage;
	}
	case 8:
	{
		int currentLevelID;
		int audioState;

		// If going to the podium
		if (((GAME_TRACKER->gameMode1 & ADVENTURE_ARENA) != 0) && (GAME_TRACKER->podiumRewardID != NOFUNC) // 0
		)
		{
			int i = 0;
			register int invalidModelId CTR_PSX_REGISTER("$7") = -1;
			struct GameTracker *podiumTracker = GAME_TRACKER;
			struct Model **modelPtrArr = &data.podiumModel_firstPlace;

			for (; i < LOAD_PODIUM_MODEL_SLOT_COUNT; i++)
			{
				struct Model *m = modelPtrArr[i];

				if (m == 0)
				{
					continue;
				}

				if (i < LOAD_PODIUM_MODELS_WITH_FILE_HEADER)
				{
					modelPtrArr[i] = (struct Model *)((u8 *)m + LOAD_MODEL_FILE_HEADER_BYTES);
					CTR_PSX_MEMORY_BARRIER();
					m = modelPtrArr[i];
				}

				if (m->id == invalidModelId)
				{
					continue;
				}

				podiumTracker->modelPtr[m->id] = m;
			}

			MEMPACK_SwapPacks(GAME_TRACKER->activeMempackIndex);
		}

		// Level ID
		currentLevelID = GAME_TRACKER->levelID;

		// Main Menu
		if (currentLevelID == MAIN_MENU_LEVEL)
		{
			audioState = AUDIO_GARAGE_ENTRY;
		}

		// One of the maps on Adventure Arena
		else if ((u32)(currentLevelID - GEM_STONE_VALLEY) < LOAD_ADV_HUB_COUNT)
		{
			audioState = AUDIO_ADV_HUB_WAIT;

			// podium reward
			if (GAME_TRACKER->podiumRewardID == NOFUNC) // 0
			{
				audioState = AUDIO_ADV_HUB;
			}
		}

		// oxide intro
		else if (currentLevelID == INTRO_RACE_TODAY)
		{
			audioState = LOAD_POSTLOAD_AUDIO_OXIDE_INTRO;
		}

		// credits
		else if (currentLevelID == CREDITS_CRASH)
		{
			audioState = AUDIO_STOP_ALL;
		}

		// Naughty Dog Box
		else if (currentLevelID == NAUGHTY_DOG_CRATE)
		{
			audioState = LOAD_POSTLOAD_AUDIO_NDBOX;
		}

		else if ((u32)(currentLevelID - OXIDE_ENDING) < LOAD_OUTRO_CUTSCENE_COUNT)
		{
			audioState = AUDIO_LOADING;
		}
		else
		{
			goto advance_stage;
		}

		loadingStage++;
#if defined(CTR_NATIVE)
		LOAD_NativeAudio_SetStateAfterBankReload(audioState);
#else
		Audio_SetState_Safe(audioState);
#endif
		return loadingStage;
	}
	case 9:
	{
		if (GAME_XA_STATE == XA_STARTING)
		{
			return loadingStage;
		}

		// MAIN_MENU is used for main menu, scrapbook, and adventure garage.
		if (((GAME_TRACKER->gameMode1 & MAIN_MENU) != 0) && (GAME_TRACKER->levelID != ADVENTURE_GARAGE))
		{
			// disable rendering everything, draw loading screen and instances
			GAME_TRACKER->renderFlags = (GAME_TRACKER->renderFlags & RENDER_FLAG_CHECKERED_FLAG) | RENDER_FLAG_RENDER_BUCKET;

			if (RaceFlag_IsFullyOffScreen() == 1)
			{
				RaceFlag_BeginTransition(1);
			}
		}

		else if ((GAME_TRACKER->gameMode2 & CREDITS) != 0)
		{
			// disable rendering everything, draw loading screen and instances
			GAME_TRACKER->renderFlags = (GAME_TRACKER->renderFlags & RENDER_FLAG_CHECKERED_FLAG) | RENDER_FLAG_RENDER_BUCKET;
		}

		// Normal level
		else
		{
			// enable all flags except loading screen
			GAME_TRACKER->renderFlags |= RENDER_FLAG_ALL_EXCEPT_CHECKERED_FLAG_MASK;
		}

		GAME_TRACKER->hudFlags |= HUD_FLAG_INTRO_RACE_TITLE_BARS;
		GAME_TRACKER->framesInThisLEV = 0;
		GAME_TRACKER->msInThisLEV = 0;

		ElimBG_Deactivate(GAME_TRACKER);
		loadingStage = -2;

		// signify end of load
		break;
	}
	default:
		return loadingStage;
	}

	return loadingStage;
}
