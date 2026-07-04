#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800265c0-0x8002689c.
void GAMEPROG_AdvPercent(struct AdvProgress *adv)
{
	struct GameTracker *gGT = sdata->gGT;
	struct MetaDataLEV *mdLev = &data.metaDataLEV[0];

	// start counter
	s32 percent = 0;
	s32 oxidePercent = 0;
	b32 allGoldOrPlatinumRelics = true;
	s32 numGems = 0;

	// erase counters
	for (s32 i = 0; i < 9; i++)
	{
		((int *)&gGT->currAdvProfile.numTrophies)[i] = 0;
	}

	// check all tracks generically
	for (s32 i = 0; i < ADV_REWARD_RELIC_TRACK_COUNT; i++)
	{
		// first bit of blue relic
		s32 bitIndex = ADV_REWARD_FIRST_SAPPHIRE_RELIC + i;
		if (CHECK_ADV_BIT(adv->rewards, bitIndex))
		{
			gGT->currAdvProfile.numRelics++;
		}

		// check 16 trophies
		if (i < ADV_REWARD_RACE_TRACK_COUNT)
		{
			// first bit of trophy
			bitIndex = ADV_REWARD_FIRST_TROPHY + i;
			if (CHECK_ADV_BIT(adv->rewards, bitIndex))
			{
				gGT->currAdvProfile.numTrophies++;
			}

			// first bit of token
			bitIndex = ADV_REWARD_FIRST_CTR_TOKEN + i;
			if (CHECK_ADV_BIT(adv->rewards, bitIndex))
			{
				// increment number of tokens, based on
				// the tokenID of this level (red, green, blue, etc)
				((int *)&gGT->currAdvProfile.numCtrTokens.red)[mdLev[i].ctrTokenGroupID]++;

				// increment number of total tokens
				gGT->currAdvProfile.numCtrTokens.total++;
			}
		}

		// check 4 keys, and 4 purple tokens
		if (i < ADV_REWARD_HUB_COUNT)
		{
			// first bit of key
			bitIndex = ADV_REWARD_FIRST_BOSS_KEY + i;
			if (CHECK_ADV_BIT(adv->rewards, bitIndex))
			{
				gGT->currAdvProfile.numKeys++;
			}

			// first bit of purple tokens
			bitIndex = ADV_REWARD_FIRST_PURPLE_TOKEN + i;
			if (CHECK_ADV_BIT(adv->rewards, bitIndex))
			{
				gGT->currAdvProfile.numCtrTokens.purple++;
			}
		}

		// check 5 gems
		if (i < ADV_REWARD_GEM_COUNT)
		{
			// first bit of gem
			bitIndex = ADV_REWARD_FIRST_GEM + i;
			if (CHECK_ADV_BIT(adv->rewards, bitIndex))
			{
				numGems++;
			}
		}

		// first bit is 2%, second bit upgrades the total Oxide bonus to 3%
		if (i < ADV_REWARD_OXIDE_BEAT_COUNT)
		{
			// first bit of beating oxide
			bitIndex = ADV_REWARD_BEAT_OXIDE_FIRST + i;
			if (CHECK_ADV_BIT(adv->rewards, bitIndex))
			{
				oxidePercent = (i == 0) ? 2 : 3;
			}
		}
	}

	// check whether all tracks have gold or platinum relic
	for (s32 i = 0; i < ADV_REWARD_RELIC_TRACK_COUNT; i++)
	{
		// first bit of gold relic
		s32 bitIndex = ADV_REWARD_FIRST_GOLD_RELIC + i;
		if (allGoldOrPlatinumRelics && CHECK_ADV_BIT(adv->rewards, bitIndex))
		{
			// check next relic
			continue;
		}

		// if relic is not unlocked,
		// then extra 1% is not earned
		allGoldOrPlatinumRelics = false;
	}

	percent += gGT->currAdvProfile.numRelics * 2 + gGT->currAdvProfile.numTrophies * 2 + gGT->currAdvProfile.numKeys + gGT->currAdvProfile.numCtrTokens.total +
	           gGT->currAdvProfile.numCtrTokens.purple + numGems + oxidePercent + allGoldOrPlatinumRelics;

	gGT->currAdvProfile.completionPercent = percent;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002689c-0x80026ae4.
void GAMEPROG_ResetHighScores(struct GameProgress *gameProg)
{
	// for every track
	for (s32 i = 0; i < MEMCARD_HIGH_SCORE_TRACK_COUNT; i++)
	{
		struct HighScoreTrack *track = &gameProg->highScoreTracks[i];

#if 0
		// all but two tracks
		if(i > 1)
		{
			// temporary test
			track->timeTrialFlags = 7;
		}
#endif

		// for time trial and relic
		for (s32 j = 0; j < MEMCARD_HIGH_SCORE_MODE_COUNT; j++)
		{
			// for every entry
			for (s32 k = 0; k < MEMCARD_HIGH_SCORE_ENTRIES_PER_MODE; k++)
			{
				s32 characterID = i + j + k;
				characterID = characterID - PENTA_PENGUIN * (characterID / PENTA_PENGUIN);

				struct HighScoreEntry *entry = &track->scoreEntry[j * MEMCARD_HIGH_SCORE_ENTRIES_PER_MODE + k];
				entry->time = MEMCARD_HIGH_SCORE_DEFAULT_TIME;
				entry->characterID = characterID;

				char *name = sdata->lngStrings[data.MetaDataCharacters[characterID].name_LNG_short];

				// can't do an int-copy,
				// strings in LNG are unaligned
				strcpy(entry->name, name);
			}
		}
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80026ae4-0x80026bf0
b32 GAMEPROG_CheckGhostsBeaten(int ghostID)
{
	struct GameTracker *gGT = sdata->gGT;
	b32 result = true;
	s16 levelID = gGT->levelID;
	int flagWordIndex = (s16)ghostID >> 5;

	for (s32 i = 0; i < MEMCARD_HIGH_SCORE_TRACK_COUNT; i++)
	{
		gGT->levelID = i;
		GAMEPROG_GetPtrHighScoreTrack();

		if (result)
		{
			u32 *timeTrialFlags = &sdata->gameProgress.highScoreTracks[gGT->levelID].timeTrialFlags;
			result = (timeTrialFlags[flagWordIndex] >> (ghostID & 0x1f)) & 1;
		}
	}

	gGT->levelID = levelID;
	GAMEPROG_GetPtrHighScoreTrack();

	return result;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80026bf0-0x80026c24.
void GAMEPROG_NewProfile_OutsideAdv(struct GameProgress *gameProg)
{
	// GameOptions is probably a struct "inside"
	// of GameProgress, still working on it

	// GameProgress and GameOptions
	memset(gameProg, 0, sizeof(struct GameProgress) + sizeof(struct GameOptions));

	GAMEPROG_ResetHighScores(gameProg);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80026c24-0x80026cb8.
void GAMEPROG_InitFullMemcard(struct MemcardProfile *mcp)
{
	// clear
	memset(mcp, 0, sizeof(struct MemcardProfile));

	// header
	mcp->header[0] = MEMCARD_PROFILE_VERSION; // version (-18)
	mcp->header[1] = sizeof(struct MemcardProfile);

	// GameProgress and GameOptions
	GAMEPROG_NewProfile_OutsideAdv(&mcp->gameProgress);

	// 4 profiles
	for (s32 i = 0; i < MEMCARD_ADV_PROFILE_COUNT; i++)
	{
		// no character selected
		mcp->advProgress[i].characterID = -1;

		// N Sane Beach
		mcp->advProgress[i].HubLevYouSavedOn = N_SANITY_BEACH;
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80026cb8-0x80026cf4.
void GAMEPROG_NewProfile_InsideAdv(struct AdvProgress *adv)
{
	// clear
	memset(adv, 0x0, sizeof(struct AdvProgress));

	// no character selected
	adv->characterID = -1;

	// N Sane Beach
	adv->HubLevYouSavedOn = N_SANITY_BEACH;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80026cf4-0x80026d7c.
void GAMEPROG_SaveCupProgress(void)
{
	u32 *prog = &sdata->gameProgress.unlocks[0];

	// 4 cups, 3 difficulties
	for (s32 i = 0; i < GAME_PROGRESS_CUP_WIN_COUNT; i++)
	{
		// if cup is "currently" beaten
		s32 bitIndex1 = i + GAME_PROGRESS_CUP_CURRENT_WIN_FIRST_BIT;
		if (CHECK_MEMCARD_BIT(prog, bitIndex1))
		{
			// set if cup was "previously" beaten
			s32 bitIndex2 = bitIndex1 + GAME_PROGRESS_CUP_PREVIOUS_WIN_OFFSET;
			UNLOCK_MEMCARD_BIT(prog, bitIndex2);
		}
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80026d7c-0x80026e48.
void GAMEPROG_SyncGameAndCard(struct GameProgress *memcardProg, struct GameProgress *currentProg)
{
	// combine progress of cups,
	// characters, track, scrapbook
	for (s32 i = 0; i < GAME_PROGRESS_UNLOCK_WORD_COUNT; i++)
	{
		u32 memcardFlags = memcardProg->unlocks[i];
		u32 currentFlags = currentProg->unlocks[i];

		u32 joinFlags = memcardFlags | currentFlags;

		memcardProg->unlocks[i] = joinFlags;
		currentProg->unlocks[i] = joinFlags;
	}

	// combine progress of beaten ghosts
	for (s32 i = 0; i < MEMCARD_HIGH_SCORE_TRACK_COUNT; i++)
	{
		u32 memcardFlags = memcardProg->highScoreTracks[i].timeTrialFlags;
		u32 currentFlags = currentProg->highScoreTracks[i].timeTrialFlags;

		u32 joinFlags = memcardFlags | currentFlags;

		memcardProg->highScoreTracks[i].timeTrialFlags = joinFlags;
		currentProg->highScoreTracks[i].timeTrialFlags = joinFlags;
	}

	// Naughty Dog left this incomplete
	// What if the game beat half of N Tropy's ghosts
	// and the new memory card beat the other half?
	// Now you have all ghosts beaten, but N Tropy
	// is still locked, and can't possibly be unlocked

	// Need to check cup flags for an unlocked battle track,
	// and n tropy ghosts for n tropy,
	// and oxide ghosts for scrapbook
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80026e48-0x80026e80.
void GAMEPROG_NewGame_OnBoot()
{
	GAMEPROG_NewProfile_OutsideAdv(&sdata->gameProgress);
	GAMEPROG_NewProfile_InsideAdv(&sdata->advProgress);
	GAMEPROG_GetPtrHighScoreTrack();
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80026e80-0x80026ed8
void GAMEPROG_GetPtrHighScoreTrack(void)
{
	struct GameTracker *gGT = sdata->gGT;
	s32 gameMode1 = gGT->gameMode1;

	sdata->ptrActiveHighScoreEntry =
	    &sdata->gameProgress.highScoreTracks[gGT->levelID].scoreEntry[MEMCARD_HIGH_SCORE_ENTRIES_PER_MODE * ((gameMode1 & RELIC_RACE) != 0)];
}
