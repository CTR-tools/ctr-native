#include <common.h>

void (*mainMenuInit[])() = {MM_JumpTo_Title_FirstTime, MM_JumpTo_Characters, MM_JumpTo_TrackSelect, MM_JumpTo_BattleSetup, CS_Garage_Init, MM_JumpTo_Scrapbook};

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

int LOAD_TenStages(struct GameTracker *gGT, int loadingStage, struct BigHeader *bigfile)
{
	int levelID;
	int ovrRegion1;
	int ovrRegion3;

	// if game is loading
	if (sdata->load_inProgress != 0)
	{
		return loadingStage;
	}

	levelID = gGT->levelID;

	// Used in stage 0, 4, 5, 6
	b32 boolPlayMusicDuringLoading = (levelID == ADVENTURE_GARAGE) || (levelID == NAUGHTY_DOG_CRATE);

	switch (loadingStage)
	{
	case 0:
	{
		if (!boolPlayMusicDuringLoading)
		{
			Cutscene_VolumeBackup();
		}
		CDSYS_XAPauseRequest();

		// if first boot (SCEA + Copyright + ND Box)
		if (sdata->boolFirstBoot != 0)
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

			gGT->db[0].drawEnv.isbg = 0;
			gGT->db[1].drawEnv.isbg = 0;
		}

		// if not first boot (dont do this for ND Box)
		else
		{
			// change active allocation system to #1
			// used for whole game (except adventure arena)
			MEMPACK_SwapPacks(LOAD_MAIN_PACK_INDEX);

			sdata->levelID = MainInit_StringToLevID(gGT->levelName);

			// erase all memory loaded after first boot
			MEMPACK_PopToState(sdata->bookmarkID);
		}

		gGT->level1 = 0;
		gGT->level2 = 0;
		gGT->numPlyrCurrGame = gGT->numPlyrNextGame;
		strcpy(gGT->levelName, data.metaDataLEV[levelID].name_Debug);

		// pop back here for every load, after first load,
		// this permanently reserves LNG, bigfile header, etc
		sdata->bookmarkID = MEMPACK_PushState();

		// Reset HUD
		gGT->hudFlags &= HUD_FLAG_LOAD_RESET_MASK;

		// disable all rendering except loading screen
		// no overlay transition (advhub),  use normal spawn
		gGT->renderFlags &= RENDER_FLAG_CHECKERED_FLAG;
		gGT->overlayTransition = 0;
		gGT->Debug_ToggleNormalSpawn = 1;
		gGT->visMem1 = 0;
		gGT->visMem2 = 0;

		// Required for Scrapbook "Press Start",
		// may also be required for other edge-cases
		DrawSync(0);

		// ========== Start of flags ===============


		// disable certain game mode flags
		gGT->gameMode1 &= ~(GAME_CUTSCENE | END_OF_RACE | ADVENTURE_ARENA | MAIN_MENU);
		gGT->gameMode2 &= ~(LEV_SWAP | CREDITS | NO_LEV_INSTANCE);

		if ((strncmp(gGT->levelName, sdata->s_ndi, LOAD_LEVEL_PREFIX_NDI_LENGTH) == 0) ||
		    (strncmp(gGT->levelName, sdata->s_ending, LOAD_LEVEL_PREFIX_ENDING_LENGTH) == 0))
		{
			gGT->gameMode1 |= GAME_CUTSCENE;
		}
		else if (strncmp(gGT->levelName, sdata->s_intro, LOAD_LEVEL_PREFIX_INTRO_LENGTH) == 0)
		{
			gGT->gameMode1 |= GAME_CUTSCENE;
			gGT->gameMode2 |= LEV_SWAP;
		}
		else if ((strncmp(gGT->levelName, sdata->s_screen, LOAD_LEVEL_PREFIX_SCREEN_LENGTH) == 0) ||
		         (strncmp(gGT->levelName, sdata->s_garage, LOAD_LEVEL_PREFIX_GARAGE_LENGTH) == 0))
		{
			gGT->gameMode1 |= MAIN_MENU;
			gGT->numPlyrNextGame = gGT->numPlyrCurrGame;
			gGT->numPlyrCurrGame = 4;

			if (strncmp(gGT->levelName, sdata->s_garage, LOAD_LEVEL_PREFIX_GARAGE_LENGTH) == 0)
			{
				gGT->numPlyrCurrGame = 1;
				sdata->mainMenuState = MAIN_MENU_ADVENTURE;
			}
		}
		else if (strncmp(gGT->levelName, sdata->s_hub, LOAD_LEVEL_PREFIX_HUB_LENGTH) == 0)
		{
			gGT->numPlyrNextGame = 1;
			gGT->numPlyrCurrGame = 1;
			gGT->gameMode1 |= ADVENTURE_ARENA;
			gGT->gameMode2 |= LEV_SWAP;
		}
		else if (strncmp(gGT->levelName, sdata->s_credit, LOAD_LEVEL_PREFIX_CREDIT_LENGTH) == 0)
		{
			gGT->numPlyrNextGame = 1;
			gGT->numPlyrCurrGame = 1;
			gGT->gameMode1 |= GAME_CUTSCENE;
			gGT->gameMode2 |= (LEV_SWAP | CREDITS);
		}


		// ========== End of setting numPlyr ================
		// ========== Set LevelLOD variables ================


		// main menu or adv garage
		if ((gGT->gameMode1 & MAIN_MENU) != 0)
		{
			sdata->levelLOD = LOAD_LEVEL_LOD_1P;
		}
		// if relic, or time trial
		else if ((gGT->gameMode1 & (TIME_TRIAL | RELIC_RACE)) != 0)
		{
			sdata->levelLOD = LOAD_LEVEL_LOD_RELIC;
		}
		else
		{
			sdata->levelLOD = gGT->numPlyrCurrGame;
		}

		gGT->hudFlags |= HUD_FLAG_INIT_UI_INSTANCES;


		// ========== End of LevelLOD ================
		// ========== Alloc Prim + OT ================


		// OG game
		MainInit_PrimMem(gGT);
		MainInit_OTMem(gGT);

		if (((gGT->gameMode1 & (GAME_CUTSCENE | ADVENTURE_ARENA)) != 0) || ((gGT->gameMode2 & CREDITS) != 0))
		{
			MainInit_JitPoolsNew(gGT);
			return loadingStage + 1;
		}

		break;
	}
	case 1:
	{
		// if XA has not paused since CDSYS_XAPauseRequest in stage #0,
		// then quit the function and try again next frame
		if (sdata->XA_State == XA_FADING)
		{
			return loadingStage;
		}

		if ((gGT->gameMode1 & CRYSTAL_CHALLENGE) != 0)
		{
			ovrRegion1 = 0;
		}
		else if ((gGT->gameMode1 & TIME_TRIAL) != 0)
		{
			ovrRegion1 = 3;
		}
		else if ((gGT->gameMode1 & ARCADE_MODE) != 0)
		{
			ovrRegion1 = 1;
		}
		else if ((gGT->gameMode1 & RELIC_RACE) != 0)
		{
			ovrRegion1 = 2;
		}
		else if ((gGT->gameMode1 & ADVENTURE_MODE) != 0)
		{
			ovrRegion1 = 1;
		}
		else
		{
			ovrRegion1 = 4;

			if ((gGT->gameMode2 & CUP_ANY_KIND) != 0)
			{
				break;
			}
		}

		LOAD_OvrEndRace(ovrRegion1);
		break;
	}
	case 2:
	{
		LOAD_OvrLOD(gGT->numPlyrCurrGame);
		break;
	}
	case 3:
	{
		if ((levelID != ADVENTURE_GARAGE) && ((gGT->gameMode1 & MAIN_MENU) != 0))
		{
			ovrRegion3 = 0;
		}
		else if ((gGT->gameMode1 & ADVENTURE_ARENA) != 0)
		{
			ovrRegion3 = 3;

			if (gGT->podiumRewardID == NOFUNC)
			{
				ovrRegion3 = 2;
			}
		}
		else if ((gGT->podiumRewardID != NOFUNC) || ((gGT->gameMode1 & GAME_CUTSCENE) != 0) || ((gGT->gameMode2 & CREDITS) != 0) ||
		         (levelID == ADVENTURE_GARAGE))
		{
			ovrRegion3 = 3;
		}
		else
		{
			ovrRegion3 = 1;

			if (gGT->overlayIndex_Threads == OVERLAY_INDEX_RACING_OR_BATTLE)
			{
				break;
			}
		}

		LOAD_OvrThreads(ovrRegion3);
		break;
	}
	case 4:
	{
		if (!boolPlayMusicDuringLoading)
		{
			Music_Restart();
		}

		// If in main menu (character selection, track selection, any part of it)
		if ((gGT->gameMode1 & MAIN_MENU) != 0)
		{
			if ((u32)sdata->mainMenuState < len(mainMenuInit))
			{
				mainMenuInit[sdata->mainMenuState]();
			}
		}

		// Needed, or else Post-Boss Outro
		// will break the character animations
		sdata->ptrMPK = 0;

		// Clear driver extras
		for (int i = 0; i < LOAD_DRIVER_MODEL_EXTRA_COUNT; i++)
		{
			data.driverModelExtras[i].fileBase = NULL;
		}

		LOAD_ClearLooseRacerModels();

		// NOTE(aalhendi): Retail gates stage advancement until the driver MPK callback sets ptrMPK.
		sdata->load_inProgress = 1;
		LOAD_DriverMPK(bigfile, sdata->levelLOD, LOAD_Callback_DriverModels);
		break;
	}
	case 5:
	{
		// clear and reset
		LibraryOfModels_Clear(gGT);

		sdata->PLYROBJECTLIST = (int **)((u32)sdata->ptrMPK + 4);
		if (sdata->ptrMPK == 0)
		{
			sdata->PLYROBJECTLIST = 0;
		}

		LOAD_GlobalModelPtrs_MPK();
		DecalGlobal_Clear(gGT);

		gGT->mpkIcons = 0;
		if (sdata->ptrMPK != 0)
		{
			gGT->mpkIcons = *(int *)sdata->ptrMPK;

			if (gGT->mpkIcons != 0)
			{
				DecalGlobal_Store(gGT, (struct LevTexLookup *)gGT->mpkIcons);
			}
		}

		if (!boolPlayMusicDuringLoading)
		{
			Music_Stop();
			CseqMusic_StopAll();
			Music_LoadBanks();
		}

		break;
	}
	case 6:
	{
		if (!boolPlayMusicDuringLoading)
		{
			int banksReady = Music_AsyncParseBanks();

			if (banksReady == 0)
			{
				// quit and restart stage 6 next frame
				return loadingStage;
			}

			Cutscene_VolumeRestore();
		}

		for (int i = 0; i < LOAD_DRIVER_MODEL_EXTRA_COUNT; i++)
		{
			if (data.driverModelExtras[i].fileBase != NULL)
			{
				data.driverModelExtras[i].model = (struct Model *)((u8 *)data.driverModelExtras[i].fileBase + LOAD_MODEL_FILE_HEADER_BYTES);
			}
		}

		LOAD_FinalizeLooseRacerModels();

		// == banks are done parsing ===

		// If this world is made of multiple LEVs
		if ((gGT->gameMode2 & LEV_SWAP) != 0)
		{
			// Cutscene Packs
			int firstSubpackSize = LOAD_CUTSCENE_FIRST_PACK_BYTES;
			int secondSubpackSize = LOAD_CUTSCENE_SECOND_PACK_BYTES;

			// If you're in Adventure Arena
			if ((gGT->gameMode1 & ADVENTURE_ARENA) != 0)
			{
				// Adv Arena Packs
				firstSubpackSize = LOAD_ADV_ARENA_FIRST_PACK_BYTES;
				secondSubpackSize = LOAD_ADV_ARENA_SECOND_PACK_BYTES;
			}

			// Allocate room for LEV swapping
			u8 *hubAlloc = MEMPACK_AllocMem(firstSubpackSize + secondSubpackSize); // "HUB ALLOC"
			sdata->ptrHubAlloc = hubAlloc;

			// Change active allocation system to #2
			// pack = [hubAlloc, hubAlloc+size1]
			MEMPACK_SwapPacks(LOAD_FIRST_SUBPACK_INDEX);
			MEMPACK_NewPack(hubAlloc, firstSubpackSize);

			// Change active allocation system to #3
			// pack = [hubAlloc+size1, hubAlloc+size1+size2]
			MEMPACK_SwapPacks(LOAD_SECOND_SUBPACK_INDEX);
			MEMPACK_NewPack(hubAlloc + firstSubpackSize, secondSubpackSize);

			s16 activeSubpackIndex;

			// Intro cutscene with oxide spaceship and all racers
			if ((gGT->gameMode1 & ADVENTURE_ARENA) == 0)
			{
				// Always start with pool 1
				activeSubpackIndex = LOAD_FIRST_SUBPACK_INDEX;
			}

			// If you're in Adventure Arena
			else
			{
				// Get 1 or 2, depending on map
				activeSubpackIndex = LOAD_GetAdvPackIndex();

				// Then swap:
				// Turn 1 into 2
				// Turn 2 into 1
				activeSubpackIndex = LOAD_HUB_MEMPACK_PAIR_INDEX_SUM - activeSubpackIndex;
			}

			// keep track of subpack levels
			gGT->activeMempackIndex = activeSubpackIndex;
			gGT->levID_in_each_mempack[activeSubpackIndex] = gGT->levelID;
			gGT->levID_in_each_mempack[LOAD_HUB_MEMPACK_PAIR_INDEX_SUM - activeSubpackIndex] = LOAD_NO_LEVEL_IN_MEMPACK;

			// the rest of memory will load pointer maps,
			// loaded at HighMem in main pack, end of RAM,
			// so the pointer maps dont bloat subpacks
			MEMPACK_SwapPacks(LOAD_MAIN_PACK_INDEX);

			sdata->PatchMem_Size = MEMPACK_GetFreeBytes();
			sdata->PatchMem_Ptr = MEMPACK_AllocHighMem(sdata->PatchMem_Size); //, "Patch Table Memory");

			// For Oxide-Intro and Credits, set active pack
			MEMPACK_SwapPacks(gGT->activeMempackIndex);
		}

		// NOTE(aalhendi): Retail sets the load gate before queueing level files.
		sdata->load_inProgress = 1;

		// add VRAM to loading queue
		LOAD_AppendQueue(bigfile, LT_VRAM, LOAD_GetBigfileIndex(gGT->levelID, sdata->levelLOD, LVI_VRAM), NULL, NULL);

		// add LEV to loading queue
		LOAD_AppendQueue(bigfile, LT_GETADDR, LOAD_GetBigfileIndex(gGT->levelID, sdata->levelLOD, LVI_LEV), NULL, LOAD_Callback_LEV);

		if (((u32)(levelID - GEM_STONE_VALLEY) < LOAD_PTR_MAP_ADV_LEVEL_COUNT) || ((u32)(levelID - CREDITS_CRASH) < LOAD_PTR_MAP_CREDIT_LEVEL_COUNT))
		{
			// add PTR file to loading queue
			LOAD_AppendQueue(bigfile, LT_SETADDR, LOAD_GetBigfileIndex(gGT->levelID, sdata->levelLOD, LVI_PTR), sdata->PatchMem_Ptr, LOAD_Callback_PatchMem);
		}
		break;
	}
	case 7:
	{
		// get level pointer
		struct Level *lev = sdata->ptrLevelFile;

		gGT->level1 = lev;
		gGT->visMem1 = lev->visMem;

		if (lev != 0)
		{
			DecalGlobal_Store(gGT, lev->levTexLookup);
		}

		DebugFont_Init(gGT);

		// if level is not nullptr
		if (lev != 0)
		{
			LibraryOfModels_Store(gGT, lev->numModels, lev->ptrModelsPtrArray);

			gGT->ptrCircle = (u32)DecalGlobal_FindInLEV(lev, rdata.s_circle);
			gGT->ptrClod = (u32)DecalGlobal_FindInLEV(lev, rdata.s_clod);
			gGT->ptrDustpuff = (u32)DecalGlobal_FindInLEV(lev, rdata.s_dustpuff);
			gGT->ptrSmoking = (u32)DecalGlobal_FindInLEV(lev, rdata.s_smokering); // "Smoke Ring"
			gGT->ptrSparkle = (u32)DecalGlobal_FindInLEV(lev, rdata.s_sparkle);
		}

		// if linked list of icons exists
		if (gGT->mpkIcons != 0)
		{
			u32 *mpkIconList = (u32 *)*(u32 *)(gGT->mpkIcons + 4);

			gGT->trafficLightIcon[0] = (struct Icon *)DecalGlobal_FindInMPK(mpkIconList, rdata.s_lightredoff);
			gGT->trafficLightIcon[1] = (struct Icon *)DecalGlobal_FindInMPK(mpkIconList, rdata.s_lightredon);
			gGT->trafficLightIcon[2] = (struct Icon *)DecalGlobal_FindInMPK(mpkIconList, rdata.s_lightgreenoff);
			gGT->trafficLightIcon[3] = (struct Icon *)DecalGlobal_FindInMPK(mpkIconList, rdata.s_lightgreenon);
		}

		gGT->gameMode1_prevFrame = 1;

		if (((gGT->gameMode1 & (GAME_CUTSCENE | ADVENTURE_ARENA)) == 0) && ((gGT->gameMode2 & CREDITS) == 0))
		{
			MainInit_JitPoolsNew(gGT);
			return loadingStage + 1;
		}

		// podium reward
		if (gGT->podiumRewardID == NOFUNC)
		{
			break;
		}

		// === Assume PodiumReward Active ===

		// Set Pack of the hub you're NOT on
		MEMPACK_SwapPacks(LOAD_HUB_MEMPACK_PAIR_INDEX_SUM - gGT->activeMempackIndex);

		// Load model+vrm files on the VRAM page
		// that does NOT overwrite the hub VRAM
		int podiumFileVariant = LOAD_GetAdvPackIndex() - 1;

		struct Model **podiumModels = &data.podiumModel_firstPlace;
		for (int i = LOAD_PODIUM_LAST_MODEL_SLOT; i >= 0; i--)
		{
			podiumModels[i] = NULL;
		}

		// NOTE(aalhendi): Retail gates stage advancement until
		// LOAD_Callback_Podiums runs after the final podium file.
		sdata->load_inProgress = 1;

		// VRAM for podium and all related models
		LOAD_AppendQueue(bigfile, LT_VRAM, BI_PODIUMVRMS + podiumFileVariant, NULL, NULL);

		int fileIndex;
		u8 *ptrIndexArr = &gGT->podium_modelIndex_First;
		struct Model **ptrModelPtrArr = podiumModels;
		void (*setPtrCb)(struct LoadQueueSlot *) = LOAD_QUEUE_CALLBACK_SET_POINTER;

		// podium first place
		if ((ptrIndexArr[0] != 0) && (ptrIndexArr[0] != STATIC_OXIDEDANCE))
		{
			fileIndex = BI_DANCEMODELWIN + podiumFileVariant + (ptrIndexArr[0] - STATIC_CRASHDANCE) * LOAD_PODIUM_MODEL_FILE_STRIDE;
			LOAD_AppendQueue(bigfile, LT_GETADDR, fileIndex, &ptrModelPtrArr[0], setPtrCb);
		}

		// podium second place
		if (ptrIndexArr[1] != 0)
		{
			fileIndex = BI_DANCEMODELLOSE + podiumFileVariant + (ptrIndexArr[1] - STATIC_CRASHDANCE) * LOAD_PODIUM_MODEL_FILE_STRIDE;
			LOAD_AppendQueue(bigfile, LT_GETADDR, fileIndex, &ptrModelPtrArr[1], setPtrCb);
		}

		// podium third place
		if (ptrIndexArr[2] != 0)
		{
			fileIndex = BI_DANCEMODELLOSE + podiumFileVariant + (ptrIndexArr[2] - STATIC_CRASHDANCE) * LOAD_PODIUM_MODEL_FILE_STRIDE;
			LOAD_AppendQueue(bigfile, LT_GETADDR, fileIndex, &ptrModelPtrArr[2], setPtrCb);
		}

		// TAWNA
		fileIndex = BI_DANCETAWNAGIRL + podiumFileVariant + (gGT->podium_modelIndex_tawna - STATIC_TAWNA1) * LOAD_PODIUM_MODEL_FILE_STRIDE;

		// add TAWNA to loading queue
		LOAD_AppendQueue(bigfile, LT_GETADDR, fileIndex, (void *)&data.podiumModel_tawna, setPtrCb);

		// if 0x7e+5 (dingo)
		if (gGT->podium_modelIndex_First == STATIC_DINGODANCE)
		{
			// add "DingoFire" to loading queue
			LOAD_AppendQueue(bigfile, LT_GETADDR, BI_DINGOFIRE + podiumFileVariant, (void *)&data.podiumModel_dingoFire, setPtrCb);
		}

		// add Podium
		LOAD_AppendQueue(bigfile, LT_GETADDR, BI_PODIUM + podiumFileVariant, NULL, LOAD_Callback_Podiums);

		// Disable LEV instances on Adv Hub, for podium scene
		gGT->gameMode2 = gGT->gameMode2 | NO_LEV_INSTANCE;
		break;
	}
	case 8:
	{
		// If going to the podium
		if (((gGT->gameMode1 & ADVENTURE_ARENA) != 0) && (gGT->podiumRewardID != NOFUNC) // 0
		)
		{
			struct Model **modelPtrArr = &data.podiumModel_firstPlace;

			for (int i = 0; i < LOAD_PODIUM_MODEL_SLOT_COUNT; i++)
			{
				struct Model *m = modelPtrArr[i];

				if (m == 0)
				{
					continue;
				}

				if (i < LOAD_PODIUM_MODELS_WITH_FILE_HEADER)
				{
					m = (struct Model *)((u8 *)m + LOAD_MODEL_FILE_HEADER_BYTES);
					modelPtrArr[i] = m;
				}

				if (m->id == -1)
				{
					continue;
				}

				gGT->modelPtr[m->id] = m;
			}

			MEMPACK_SwapPacks(gGT->activeMempackIndex);
		}

		// Level ID
		int currentLevelID = gGT->levelID;
		int audioState;

		// Main Menu
		if (currentLevelID == MAIN_MENU_LEVEL)
		{
			audioState = AUDIO_GARAGE_ENTRY;
		LAB_800346b0:
#if defined(CTR_NATIVE)
			LOAD_NativeAudio_SetStateAfterBankReload(audioState);
#else
			Audio_SetState_Safe(audioState);
#endif
			return loadingStage + 1;
		}

		// One of the maps on Adventure Arena
		if ((u32)(currentLevelID - GEM_STONE_VALLEY) < LOAD_ADV_HUB_COUNT)
		{
			audioState = AUDIO_ADV_HUB_WAIT;

			// podium reward
			if (gGT->podiumRewardID == NOFUNC) // 0
			{
				audioState = AUDIO_ADV_HUB;
			}
			goto LAB_800346b0;
		}

		// oxide intro
		if (currentLevelID == INTRO_RACE_TODAY)
		{
			audioState = LOAD_POSTLOAD_AUDIO_OXIDE_INTRO;
			goto LAB_800346b0;
		}

		// credits
		if (currentLevelID == CREDITS_CRASH)
		{
			audioState = AUDIO_STOP_ALL;
			goto LAB_800346b0;
		}

		// Naughty Dog Box
		if (currentLevelID == NAUGHTY_DOG_CRATE)
		{
			audioState = LOAD_POSTLOAD_AUDIO_NDBOX;
			goto LAB_800346b0;
		}

		// stop/pause cseq music
		audioState = AUDIO_LOADING;

		if ((u32)(currentLevelID - OXIDE_ENDING) < LOAD_OUTRO_CUTSCENE_COUNT)
		{
			goto LAB_800346b0;
		}
		break;
	}
	case 9:
	{
		if (sdata->XA_State == XA_STARTING)
		{
			return loadingStage;
		}

		// MAIN_MENU is used for main menu, scrapbook, and adventure garage.
		if (((gGT->gameMode1 & MAIN_MENU) != 0) && (gGT->levelID != ADVENTURE_GARAGE))
		{
			// disable rendering everything, draw loading screen and instances
			gGT->renderFlags = (gGT->renderFlags & RENDER_FLAG_CHECKERED_FLAG) | RENDER_FLAG_RENDER_BUCKET;

			if (RaceFlag_IsFullyOffScreen())
			{
				RaceFlag_BeginTransition(1);
			}
		}

		else if ((gGT->gameMode2 & CREDITS) != 0)
		{
			// disable rendering everything, draw loading screen and instances
			gGT->renderFlags = (gGT->renderFlags & RENDER_FLAG_CHECKERED_FLAG) | RENDER_FLAG_RENDER_BUCKET;
		}

		// Normal level
		else
		{
			// enable all flags except loading screen
			gGT->renderFlags |= RENDER_FLAG_ALL_EXCEPT_CHECKERED_FLAG_MASK;
		}

		gGT->hudFlags |= HUD_FLAG_INTRO_RACE_TITLE_BARS;
		gGT->framesInThisLEV = 0;
		gGT->msInThisLEV = 0;

		ElimBG_Deactivate(gGT);

		// signify end of load
		return -2;
	}
	default:
		return loadingStage;
	}

	loadingStage++;
	return loadingStage;
}
