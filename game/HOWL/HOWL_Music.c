#include <common.h>

void Music_SetIntro(void)
{
	struct Bank thisBank;
	// NOTE(aalhendi): Retail prepares the bank ID before clearing the one-byte loading state.
	register int bankID CTR_PSX_REGISTER("$4") = 33;

	*(u8 *)&sdata->audioDefaults[7] = 0;
	CTR_PSX_MEMORY_BARRIER();

	Bank_Load(bankID, &thisBank);

	while (Bank_AssignSpuAddrs() == 0)
	{
	}

	howl_SetSong(HOWL_SONG_ND_CRATE);

	while (howl_LoadSong() == 0)
	{
	}
}

void Music_LoadBanks(void)
{
	u32 bankID;
	struct Bank thisBank;
	struct GameTracker *gGT = GAME_TRACKER;
	s32 level;

	Audio_SetReverbMode(
	    // Level ID
	    gGT->levelID,

	    (u32)gGT->gameMode1 & ADVENTURE_BOSS,

	    // Boss ID
	    gGT->bossID);

	// NOTE(aalhendi): Retail reloads the tracker after configuring reverb.
	level = GAME_TRACKER->levelID;
	if (level == INTRO_RACE_TODAY)
	{
		Bank_DestroyAll();
		bankID = 0x22;
	}

	else if (level == NAUGHTY_DOG_CRATE)
	{
		Bank_DestroyAll();
		bankID = 0x21;
	}

	else
	{
		goto normalBanks;
	}

	// NOTE(aalhendi): Publish the bank state before preparing the load call.
	((u8 *)&sdata->audioDefaults[7])[0] = 0;
	CTR_PSX_OBSERVE_MEMORY(((u8 *)&sdata->audioDefaults[7])[0]);

	Bank_Load(bankID, &thisBank);

	// loading state of song (one byte)
	((u8 *)&sdata->audioDefaults[7])[1] = 3;
	CTR_PSX_OBSERVE_MEMORY(((u8 *)&sdata->audioDefaults[7])[1]);
	return;

normalBanks:
	if (((u8 *)&sdata->audioDefaults[7])[0] != 0)
	{
		Bank_DestroyUntilIndex(0);
	}
	else
	{
		Bank_DestroyAll();
		Bank_Load(0, &thisBank);
		((u8 *)&sdata->audioDefaults[7])[0] = 1;
	}

	// loading state of song (one byte)
	// NOTE(aalhendi): Both normal-bank paths join before this stage write.
	CTR_PSX_OBSERVE_MEMORY(((u8 *)&sdata->audioDefaults[7])[1]);
	((u8 *)&sdata->audioDefaults[7])[1] = 0;
	return;
}

u32 Music_AsyncParseBanks(void)
{
	struct Bank thisBank;
	// NOTE(aalhendi): Retail prepares the stack-bank argument before each character lookup finishes.
	register struct Bank *bank CTR_PSX_REGISTER("$5");
	u32 nextStage;
	u32 bankID;
	u32 songID;
	u8 podiumModel;

	// NOTE(aalhendi): The entry binding preserves the early byte load; later reads use the shared storage.
	if (GAME_MUSIC_LOAD_STAGE_INITIAL == 5)
	{
		return 1;
	}

	switch (((u8 *)&sdata->audioDefaults[7])[1])
	{
	// Finish the previous bank, then start the level FX bank.
	case 0:
	{
		struct GameTracker *gGT;
		s32 level;
		if (Bank_AssignSpuAddrs() == 0)
			goto PARSE_FINISH;
		gGT = GAME_TRACKER;
		if (IS_BOSS_RACE(gGT->gameMode1))
		{
			if (gGT->bossID >= 6)
				goto LEVEL_FX_DONE;
			bankID = GAME_BOSS_FX_BANKS[gGT->bossID];
		}
		else
		{
			level = gGT->levelID;
			if (level < INTRO_RACE_TODAY)
			{
				register u8 *entry CTR_PSX_REGISTER("$2");
				entry = (u8 *)GAME_LEVEL_FX_BANKS;
				// NOTE(aalhendi): The table contains unsigned IDs and retail adds the index before its base.
				CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(entry, entry, level);
				bankID = *entry;
			}
			else if (level == MAIN_MENU_LEVEL)
				bankID = 0x20;
			else if (level == OXIDE_ENDING)
				bankID = 0x23;
			else if (level == OXIDE_TRUE_ENDING)
				bankID = 0x24;
			else if (level == CREDITS_CRASH)
				bankID = 0x25;
			else
				goto LEVEL_FX_DONE;
		}
		Bank_Load(bankID, &thisBank);
	LEVEL_FX_DONE:
		nextStage = 1;
		goto STORE_STAGE;
	}

	// Finish the level bank and optionally load the original eight drivers.
	case 1:
	{
		struct GameTracker *gGT;
		s32 level;
		u32 mode;
		u32 modeBits;
		if (Bank_AssignSpuAddrs() == 0)
			goto PARSE_FINISH;
		gGT = GAME_TRACKER;
		level = gGT->levelID;
		sdata->bankCount = 0;
		sdata->bankPodiumStage = 0;
		sdata->bankLoad54 = 0;
		CTR_PSX_OBSERVE_MEMORY(sdata->bankLoad54);
		if (level >= GEM_STONE_VALLEY)
			goto DEFAULT_DRIVERS_DONE;
		mode = gGT->gameMode1;
		// NOTE(aalhendi): Keep the separate retail mask tests instead of a combined comparison.
		if (CTR_PSX_AND_MASK_NONZERO(modeBits, ADVENTURE_MODE, mode))
		{
			if (!CTR_PSX_AND_MASK_NONZERO(modeBits, ADVENTURE_BOSS | ADVENTURE_ARENA | CRYSTAL_CHALLENGE | RELIC_RACE, mode) &&
			    (!CTR_PSX_AND_MASK_NONZERO(modeBits, ADVENTURE_CUP, mode) || gGT->cup.cupID != 4))
				goto LOAD_DEFAULT8;
		}
		if ((GAME_TRACKER_RELOAD()->gameMode1 & ARCADE_MODE) != 0)
		LOAD_DEFAULT8:
		{
			Bank_Load(54, &thisBank);
			sdata->bankLoad54 = 1;
		}
		DEFAULT_DRIVERS_DONE:
			nextStage = 2;
		goto STORE_STAGE;
	}

	// Load each character bank, followed by any podium character banks.
	case 2:
	{
		struct GameTracker *gGT;
		s32 level;
		u32 mode;
		u32 modeBits;
		if (Bank_AssignSpuAddrs() == 0)
			goto PARSE_FINISH;
		gGT = GAME_TRACKER;
		level = gGT->levelID;
		if (level < GEM_STONE_VALLEY)
		{
			// Purple gem cup loads five character banks; other races use their player count.
			mode = gGT->gameMode1;
			if (!CTR_PSX_AND_MASK_NONZERO(modeBits, ADVENTURE_MODE, mode))
				goto ORDINARY_PLAYERS;
			if (!CTR_PSX_AND_MASK_NONZERO(modeBits, ADVENTURE_CUP, mode) || gGT->cup.cupID != 4)
				goto ORDINARY_PLAYERS;
			{
				s32 characterIndex = sdata->bankCount;
				if (characterIndex < 5)
				{
					register u16 *character CTR_PSX_REGISTER("$2");
					s16 *characterBase = GAME_CHARACTER_IDS;
					character = (u16 *)&characterBase[characterIndex];
					bankID = *character;
					bank = &thisBank;
					// NOTE(aalhendi): Consume the character address before forming the bank argument.
					CTR_PSX_DEPEND_VALUE(bankID, character);
					bankID = (u16)(bankID + 0x37);
					Bank_Load(bankID, &thisBank);
					goto CHARACTER_DONE;
				}
				{
					// NOTE(aalhendi): Keep the purple-cup completion separate from the normal-player exit.
					CTR_PSX_OBSERVE_VALUE(characterIndex);
					nextStage = 3;
					goto STORE_STAGE;
				}
			}
		ORDINARY_PLAYERS:
		{
			s32 characterIndex;
			u16 *character;
			s16 *characterBase;
			struct GameTracker *playerGT = GAME_TRACKER_RELOAD();
			characterIndex = sdata->bankCount;
			if (characterIndex >= playerGT->numPlyrCurrGame)
			{
				nextStage = 3;
				goto STORE_STAGE;
			}
			if (sdata->bankLoad54 != 0 && GAME_CHARACTER_IDS[characterIndex] < PINSTRIPE)
				goto CHARACTER_DONE;
			characterBase = GAME_CHARACTER_IDS;
			character = (u16 *)&characterBase[characterIndex];
			bankID = *character;
			bank = &thisBank;
			// NOTE(aalhendi): Consume the character address before forming the bank argument.
			CTR_PSX_DEPEND_VALUE(bankID, character);
			bankID = (u16)(bankID + 0x37);
			Bank_Load(bankID, &thisBank);
			goto CHARACTER_DONE;
		}
		}
		else if ((u32)level - GEM_STONE_VALLEY < INTRO_RACE_TODAY - GEM_STONE_VALLEY)
		{
			if (sdata->bankCount == 0)
			{
				if (sdata->bankLoad54 != 0 || GAME_CHARACTER_IDS[0] >= PINSTRIPE)
					goto CHARACTER_DONE;
				bankID = (u16)(GAME_CHARACTER_IDS[0] + 0x37);
				bank = &thisBank;
				Bank_Load(bankID, bank);
			CHARACTER_DONE:
				sdata->bankCount++;
				CTR_PSX_OBSERVE_MEMORY(sdata->bankCount);
				goto PARSE_FINISH;
			}
			if (gGT->podiumRewardID == NOFUNC)
			{
				nextStage = 3;
				goto STORE_STAGE;
			}
			if ((mode = sdata->bankPodiumStage) == 0)
			{
				podiumModel = gGT->podium_modelIndex_First;
				bank = &thisBank;
			}
			else if (mode == 1)
			{
				bank = &thisBank;
				if (gGT->podium_modelIndex_Second == 0)
					goto PODIUM_DONE;
				// NOTE(aalhendi): Retail rereads a nonzero podium model for the bank argument.
				CTR_PSX_RELOAD(gGT->podium_modelIndex_Second);
				podiumModel = gGT->podium_modelIndex_Second;
			}
			else if (mode == 2)
			{
				bank = &thisBank;
				if (gGT->podium_modelIndex_Third == 0)
					goto PODIUM_DONE;
				CTR_PSX_RELOAD(gGT->podium_modelIndex_Third);
				podiumModel = gGT->podium_modelIndex_Third;
			}
			else
			{
				nextStage = 3;
				goto STORE_STAGE;
			}
			Bank_Load((u16)(podiumModel - 88), bank);
		PODIUM_DONE:
			sdata->bankPodiumStage++;
			CTR_PSX_OBSERVE_MEMORY(sdata->bankPodiumStage);
			goto PARSE_FINISH;
		}
		nextStage = 3;
		goto STORE_STAGE;
	}

	// Finish character loading and select the level song.
	case 3:
	{
		struct GameTracker *gGT;
		s32 level;
		if (Bank_AssignSpuAddrs() == 0)
			goto PARSE_FINISH;
		gGT = GAME_TRACKER;
		if (IS_BOSS_RACE(gGT->gameMode1))
		{
			if (gGT->bossID >= 6)
				goto LEVEL_SONG_DONE;
			songID = HOWL_SONG_BOSS_RACE;
		}
		else
		{
			level = gGT->levelID;
			if (level < INTRO_RACE_TODAY)
			{
				register u8 *entry CTR_PSX_REGISTER("$2");
				entry = (u8 *)GAME_LEVEL_SONG_BANKS;
				// NOTE(aalhendi): The table contains unsigned IDs and retail adds the index before its base.
				CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(entry, entry, level);
				songID = *entry;
			}
			else if (level == MAIN_MENU_LEVEL)
				songID = HOWL_SONG_MAIN_MENU;
			else if (level == INTRO_RACE_TODAY)
				songID = HOWL_SONG_INTRO_RACE;
			else if (level == NAUGHTY_DOG_CRATE)
				songID = HOWL_SONG_ND_CRATE;
			else if (level == OXIDE_ENDING)
				songID = HOWL_SONG_OXIDE_END;
			else if (level == OXIDE_TRUE_ENDING)
				songID = HOWL_SONG_OXIDE_TRUE;
			else if (level == CREDITS_CRASH)
				songID = HOWL_SONG_CREDITS;
			else
				goto LEVEL_SONG_DONE;
		}
		howl_SetSong(songID);
	LEVEL_SONG_DONE:
		nextStage = 4;
		goto STORE_STAGE;
	}

	// Finish the level song.
	case 4:
		if (howl_LoadSong() == 0)
			goto PARSE_FINISH;
		nextStage = 5;
		goto STORE_STAGE;
	default:
		goto PARSE_FINISH;
	}
STORE_STAGE:
	((u8 *)&sdata->audioDefaults[7])[1] = nextStage;
PARSE_FINISH:
	return ((u8 *)&sdata->audioDefaults[7])[1] == 5;
}

void Music_SetDefaults(void)
{
	// no music playing
	sdata->cseqBoolPlay = false;
	sdata->cseqHighestIndex = -1;
	sdata->cseqTempo = 0;
	// NOTE(aalhendi): Retail leaves the return delay slot empty after these stores.
	CTR_PSX_MEMORY_BARRIER();
}

// param_1 - SongID (playing)
// param_2 - deltaBPM
// param_3 - 8008d068 for AdvHub
// param_4 - songSetActiveBits
void Music_Adjust(u32 songID, s32 newTempo, struct SongSet *set, u32 songSetActiveBits)
{
	u32 selectedSong;
	// NOTE(aalhendi): Keep the raw input live until the final masked-ID comparison.
	register u32 rawSong CTR_PSX_REGISTER("$16") = songID;

	// NOTE(aalhendi): The entry binding keeps the playing-state load before the stack frame.
	if (GAME_MUSIC_PLAYING_INITIAL != 0)
	{
		u32 currentSong = sdata->cseqHighestIndex;
		if (currentSong != (u16)rawSong)
		{
			CseqMusic_Stop(sdata->cseqHighestIndex);
		}
		else if (sdata->cseqTempo != newTempo)
		{
			CseqMusic_ChangeTempo((u16)rawSong, newTempo);
			sdata->cseqTempo = newTempo;
		}
	}

	selectedSong = (u16)rawSong;
	if (sdata->cseqHighestIndex != selectedSong)
	{
		CseqMusic_Start(selectedSong, newTempo, set, songSetActiveBits, 1);
		sdata->cseqBoolPlay = true;
		sdata->cseqHighestIndex = selectedSong;
		sdata->cseqTempo = newTempo;
	}
}

// happens during "FINAL LAP!"
void Music_LowerVolume(void)
{
	if (GAME_MUSIC_PLAYING_INITIAL != 0)
	{
		// NOTE(aalhendi): Retail loads the song argument in each volume arm before their shared call.
		if ((sdata->cseqHighestIndex - 1u) < 2u)
		{
			CseqMusic_ChangeVolume((u16)sdata->cseqHighestIndex, 150, 8);
		}
		else
		{
			CseqMusic_ChangeVolume((u16)sdata->cseqHighestIndex, 90, 8);
		}
	}
}

// after "FINAL LAP!" is done
void Music_RaiseVolume(void)
{
	if (GAME_MUSIC_PLAYING_INITIAL != 0)
	{
		if ((sdata->cseqHighestIndex - 1u) < 2u)
		{
			CseqMusic_ChangeVolume((u16)sdata->cseqHighestIndex, 255, 8);
		}
		else
		{
			CseqMusic_ChangeVolume((u16)sdata->cseqHighestIndex, 190, 8);
		}
	}
}

void Music_Restart(void)
{
	// if cseq music is playing
	if (GAME_MUSIC_PLAYING_INITIAL != 0)
	{
		CseqMusic_Restart(sdata->cseqHighestIndex, 8);
	}
}

void Music_Stop(void)
{
	// quit if no music is playing
	if (GAME_MUSIC_PLAYING_INITIAL == 0)
	{
		return;
	}

	CseqMusic_Stop(sdata->cseqHighestIndex & 0xffff);

	sdata->cseqBoolPlay = 0;
	sdata->cseqHighestIndex = -1;
}

void Music_Start(u32 songID)
{
	sdata->cseqBoolPlay = true;

	// set highest song index
	sdata->cseqHighestIndex = songID & 0xffff;
	// NOTE(aalhendi): Keep the final store before retail's empty return delay slot.
	CTR_PSX_MEMORY_BARRIER();
}

void Music_End(void)
{
	sdata->cseqBoolPlay = false;

	// no songs are playing
	sdata->cseqHighestIndex = -1;
	// NOTE(aalhendi): Keep the final store before retail's empty return delay slot.
	CTR_PSX_MEMORY_BARRIER();
}

u32 Music_GetHighestSongPlayIndex(void)
{
	// 0xffff - no cseq music
	// 0x0000 - song[0] (level music)
	// 0x0001 - song[1] (game aku)

	// "could" be 2 from [2] (menu aku),
	// but the game never sets it

	return sdata->cseqHighestIndex;
}
