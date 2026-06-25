#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80039fa8-0x8003a2b4
void MainGameEnd_SoloRaceGetReward(int subtractTimeCrateBonus)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *driver = gGT->drivers[0];
	struct Driver *player = gGT->threadBuckets[PLAYER].thread->object;

	gGT->newHighScoreIndex = -1;
	gGT->gameModeEnd &= 0x7bffffff;

	int timeBonus = 0;

	if (((s8)driver->numTimeCrates == gGT->timeCratesInLEV) && (subtractTimeCrateBonus != 0))
	{
		timeBonus = 0x2580;
	}

	int raceTime = driver->timeElapsedInRace - timeBonus;

	for (s16 i = 0; i < 5; i++)
	{
		if (raceTime < (s32)sdata->ptrActiveHighScoreEntry[i + 1].time)
		{
			gGT->newHighScoreIndex = i;
			gGT->gameModeEnd |= 0x88000000;
			break;
		}
	}

	gGT->bestLapTime = sdata->ptrActiveHighScoreEntry[0].time;

	for (s16 i = 0; i < (s8)gGT->numLaps; i++)
	{
		if (gGT->lapTime[i] < gGT->bestLapTime)
		{
			gGT->bestLapTime = gGT->lapTime[i];
			gGT->lapIndexNewBest = i;
			gGT->gameModeEnd |= 0x8c000000;
		}
	}

	if ((gGT->gameMode1 & TIME_TRIAL) == 0)
	{
		return;
	}

	if ((gGT->gameModeEnd & 4) != 0)
	{
		return;
	}

	gGT->gameModeEnd |= 4;

	struct HighScoreTrack *track = &sdata->gameProgress.highScoreTracks[gGT->levelID];
	int playerTime = player->timeElapsedInRace;

	if (((track->timeTrialFlags >> data.bitIndex_timeTrialFlags_saveData.nTropyOpen) & 1) == 0)
	{
		if ((track->timeTrialFlags & 1) == 0)
		{
			if (data.metaDataLEV[gGT->levelID].timeTrial <= playerTime)
			{
				goto CheckOxideAllTracks;
			}

			track->timeTrialFlags |= 1;
			gGT->gameModeEnd |= 0x08008000;
		}
		else
		{
			if (gGT->timeToBeatInTimeTrial_ForCurrentEvent <= playerTime)
			{
				goto CheckOxideAllTracks;
			}

			track->timeTrialFlags |= 1 << (data.bitIndex_timeTrialFlags_saveData.nTropyOpen & 0x1f);
			gGT->gameModeEnd |= data.bitIndex_timeTrialFlags_flashingText.nOxideOpen;
		}
	}
	else
	{
		if (gGT->timeToBeatInTimeTrial_ForCurrentEvent <= playerTime)
		{
			goto CheckOxideAllTracks;
		}

		track->timeTrialFlags |= 1 << (data.bitIndex_timeTrialFlags_saveData.nOxideOpen & 0x1f);
		gGT->gameModeEnd |= data.bitIndex_timeTrialFlags_flashingText.nTropyOpen;
	}

CheckOxideAllTracks:
	if ((GAMEPROG_CheckGhostsBeaten(2) & 0xffff) != 0)
	{
		UNLOCK_ADV_BIT(sdata->gameProgress.unlocks, GAME_UNLOCK_BIT_SCRAPBOOK);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003a2b4-0x8003a3fc.
void MainGameEnd_SoloRaceSaveHighScore(void)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *player = gGT->drivers[0];

	MainGameEnd_SoloRaceGetReward(0);

	u32 gameModeEnd = gGT->gameModeEnd;

	if ((gameModeEnd & HIGH_SCORE_SAVED) != 0)
	{
		return;
	}

	gGT->gameModeEnd = gameModeEnd | HIGH_SCORE_SAVED;

	struct HighScoreEntry *entry = sdata->ptrActiveHighScoreEntry;

	if ((gameModeEnd & NEW_BEST_LAP) != 0)
	{
		entry[0].time = gGT->bestLapTime;
		entry[0].name[0] = 0;
		entry[0].characterID = data.characterIDs[(u8)player->driverID];
		memmove(entry[0].name, gGT->prevNameEntered, 0x11);
	}

	int highScoreIndex = (s8)gGT->newHighScoreIndex;

	if (highScoreIndex < 0)
	{
		return;
	}

	entry = &sdata->ptrActiveHighScoreEntry[highScoreIndex + 1];

	if (highScoreIndex < 4)
	{
		memmove(&entry[1], entry, (4 - highScoreIndex) * sizeof(struct HighScoreEntry));
	}

	entry->time = player->timeElapsedInRace;
	entry->name[0] = 0;
	entry->characterID = data.characterIDs[(u8)player->driverID];
	memmove(entry->name, gGT->prevNameEntered, 0x11);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003a3fc-0x8003aee8

static void MainGameEnd_AddBattleWinner(struct GameTracker *gGT, struct Driver *driver)
{
	u32 winnerIndex = gGT->numWinners;
	u8 driverID = driver->driverID;
	gGT->winnerIndex[winnerIndex] = driverID;

	struct PushBuffer *pb = &gGT->pushBuffer[driverID];
	pb->fadeFromBlack_currentValue = 0x1fff;
	pb->fadeFromBlack_desiredResult = 0x1000;
	pb->fade_step = 0xff78;

	BOTS_Driver_Convert(driver);
	gGT->numWinners++;
}

static void MainGameEnd_SetBattleConfetti(struct GameTracker *gGT)
{
	gGT->confetti.numParticles_max = 0xfa;
	gGT->confetti.unk2 = 0xfa;
}

static void MainGameEnd_UpdateAdventureLosses(struct GameTracker *gGT, struct Driver *player)
{
	if ((gGT->gameMode1 & (ADVENTURE_CUP | RELIC_RACE | ADVENTURE_MODE)) != ADVENTURE_MODE)
	{
		return;
	}

	if (player->driverRank == 0)
	{
		if (IS_BOSS_RACE(gGT->gameMode1))
		{
			sdata->advProgress.timesLostBossRace[gGT->bossID] = 0;
		}
		else
		{
			sdata->advProgress.timesLostRacePerLev[gGT->levelID] = 0;
		}

		return;
	}

	char *lossCounter;

	if (IS_BOSS_RACE(gGT->gameMode1))
	{
		lossCounter = &sdata->advProgress.timesLostBossRace[gGT->bossID];
	}
	else
	{
		lossCounter = &sdata->advProgress.timesLostRacePerLev[gGT->levelID];
	}

	if (*lossCounter < 10)
	{
		(*lossCounter)++;
	}
}

static void MainGameEnd_RecordNonBattleStandings(struct GameTracker *gGT)
{
	for (int i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		struct Driver *driver = gGT->drivers[i];

		int rank = driver->driverRank;

		if (rank < 3)
		{
			gGT->standingsPoints[i * 3 + rank]++;
		}
	}
}

static int MainGameEnd_BattleTeamActive(struct GameTracker *gGT, int team)
{
	if ((gGT->gameMode1 & BATTLE_MODE) == 0)
	{
		return 1;
	}

	return (gGT->battleSetup.teamFlags & (1u << team)) != 0;
}

static void MainGameEnd_RankBattlePointLimit(struct GameTracker *gGT)
{
	u8 usedTeams = 0;

	for (int rank = 0; rank < 4; rank++)
	{
		int bestScore = -400;
		s16 tiedTeams[4] = {-1, -1, -1, -1};
		int numTies = 0;

		for (int team = 3; team >= 0; team--)
		{
			if ((usedTeams & (1u << team)) != 0)
			{
				continue;
			}

			int score = gGT->battleSetup.pointsPerTeam[team];

			if (score < bestScore)
			{
				continue;
			}

			if (score > bestScore)
			{
				bestScore = score;
				numTies = 0;
			}

			tiedTeams[numTies++] = team;
		}

		if (numTies == 0)
		{
			break;
		}

		for (int i = 0; i < numTies; i++)
		{
			int team = tiedTeams[i];

			usedTeams |= 1u << team;
			gGT->battleSetup.finishedRankOfEachTeam[team] = rank;

			if (rank < 3)
			{
				gGT->standingsPoints[team * 3 + rank]++;
			}
		}

		rank += numTies - 1;
	}
}

static void MainGameEnd_UpdateBattleWinners_PointLimit(struct GameTracker *gGT)
{
	for (int i = 0; i < 4; i++)
	{
		struct Driver *driver = gGT->drivers[i];

		if ((driver != NULL) && (gGT->battleSetup.finishedRankOfEachTeam[driver->BattleHUD.teamID] == 0))
		{
			MainGameEnd_AddBattleWinner(gGT, driver);
		}

		MainGameEnd_SetBattleConfetti(gGT);
	}
}

static void MainGameEnd_UpdateBattlePointLimit(struct GameTracker *gGT)
{
	MainGameEnd_RankBattlePointLimit(gGT);
	MainGameEnd_UpdateBattleWinners_PointLimit(gGT);
}

static int MainGameEnd_BattleLifeSlotNumLives(struct GameTracker *gGT, int slot)
{
	struct Driver *driver = gGT->drivers[slot];

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Retail NULL slots read low mirrored RAM instead of
	// faulting. Native preserves the unused-slot zero-lives effect directly.
	if (driver == NULL)
	{
		return 0;
	}
#endif

	return driver->BattleHUD.numLives;
}

static int MainGameEnd_BattleLifeSlotTeam(struct GameTracker *gGT, int slot)
{
	struct Driver *driver = gGT->drivers[slot];

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Preserve the same low-RAM NULL-slot quirk for retail's
	// later team read without changing the mixed teamID/slot algorithm.
	if (driver == NULL)
	{
		return 0;
	}
#endif

	return driver->BattleHUD.teamID;
}

static void MainGameEnd_MarkBattleTeamSlotsUsed(struct GameTracker *gGT, int representativeSlot, u8 *usedSlots)
{
	int team = MainGameEnd_BattleLifeSlotTeam(gGT, representativeSlot);

	for (int i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		struct Driver *driver = gGT->drivers[i];

		if (driver->BattleHUD.teamID == team)
		{
			*usedSlots |= 1u << i;
		}
	}
}

static void MainGameEnd_UpdateBattleLifeLimit(struct GameTracker *gGT)
{
	u8 usedSlots = 0;

	for (int rank = 0; rank < 4; rank++)
	{
		int bestLives = -400;
		s16 tiedSlots[4] = {-1, -1, -1, -1};
		int numTies = 0;

		for (int slot = 3; slot >= 0; slot--)
		{
			int lives = MainGameEnd_BattleLifeSlotNumLives(gGT, slot);

			if (lives == 0)
			{
				continue;
			}

			if (lives == bestLives)
			{
				if ((usedSlots & (1u << slot)) != 0)
				{
					continue;
				}

				numTies++;
				tiedSlots[numTies] = slot;
				usedSlots |= 1u << slot;
				continue;
			}

			if (lives < bestLives)
			{
				continue;
			}

			if ((usedSlots & (1u << slot)) != 0)
			{
				continue;
			}

			for (int i = 0; i < numTies + 1; i++)
			{
				int oldSlot = tiedSlots[i];

				if (oldSlot != -1)
				{
					usedSlots &= ~(1u << oldSlot);
				}
			}

			bestLives = lives;
			numTies = 0;
			tiedSlots[0] = MainGameEnd_BattleLifeSlotTeam(gGT, slot);
			usedSlots |= 1u << tiedSlots[0];
		}

		if (tiedSlots[0] == -1)
		{
			rank += numTies;
			continue;
		}

		for (int i = 0; i < numTies + 1; i++)
		{
			int representativeSlot = tiedSlots[i];
			struct Driver *driver = gGT->drivers[representativeSlot];
			int team = MainGameEnd_BattleLifeSlotTeam(gGT, representativeSlot);

			gGT->standingsPoints[team * 3 + rank]++;

			if ((rank == 0) && (driver != NULL))
			{
				MainGameEnd_AddBattleWinner(gGT, driver);
			}

			MainGameEnd_SetBattleConfetti(gGT);
			gGT->battleSetup.finishedRankOfEachTeam[representativeSlot] = rank;
			MainGameEnd_MarkBattleTeamSlotsUsed(gGT, representativeSlot, &usedSlots);
		}

		rank += numTies;
	}
}

static void MainGameEnd_UpdateStandingsOrder(struct GameTracker *gGT)
{
	for (int team = 0; team < 4; team++)
	{
		int score = 0;

		for (int rank = 0; rank < 3; rank++)
		{
			score += gGT->standingsPoints[team * 3 + rank] * (3 - rank);
		}

		gGT->battleSetup.standingsScore[team] = (s16)score;
	}

	u8 usedTeams = 0;

	for (int rank = 0; rank < 4; rank++)
	{
		int bestScore = 0;
		int bestTeam = -1;

		for (int team = 3; team >= 0; team--)
		{
			if ((usedTeams & (1u << team)) != 0)
			{
				continue;
			}

			if (!MainGameEnd_BattleTeamActive(gGT, team))
			{
				continue;
			}

			if (gGT->battleSetup.standingsScore[team] < bestScore)
			{
				continue;
			}

			bestScore = gGT->battleSetup.standingsScore[team];
			bestTeam = team;
		}

		gGT->battleSetup.standingsOrder[rank] = bestTeam;

		if (bestTeam >= 0)
		{
			usedTeams |= 1u << bestTeam;
		}
	}
}

static void MainGameEnd_FinalizeDriverClocks(struct GameTracker *gGT)
{
	struct Thread *thread = gGT->threadBuckets[PLAYER].thread;

	while (thread != NULL)
	{
		UI_RaceEnd_GetDriverClock(thread->object);
		thread = thread->siblingThread;
	}
}

static void MainGameEnd_CheckTimeTrialGhost(struct GameTracker *gGT, struct Driver *player)
{
	if ((gGT->gameMode1 & RELIC_RACE) != 0)
	{
		RR_EndEvent_UnlockAward();
		return;
	}

	if ((sdata->boolReplayHumanGhost != 0) &&
	    ((sdata->boolGhostTooBigToSave != 0) || (player->timeElapsedInRace >= sdata->ptrGhostTapePlaying->timeElapsedInRace)))
	{
		return;
	}

	GhostTape_End();
	gGT->gameModeEnd |= PLAYER_GHOST_BEAT;
}

void MainGameEnd_Initialize(void)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *player = gGT->drivers[0];

	if ((gGT->gameMode1 & END_OF_RACE) == 0)
	{
		for (int i = 0; i < 8; i++)
		{
			struct Driver *driver = gGT->drivers[i];

			if (driver != NULL)
			{
				sdata->kartSpawnOrderArray[i] = driver->driverRank;
			}
		}

		gGT->newHighScoreIndex = -1;

		if ((gGT->gameMode1 & ROLLING_ITEM) != 0)
		{
			OtherFX_Stop2(0x5d);
			gGT->gameMode1 &= ~ROLLING_ITEM;
		}

		MainGameEnd_UpdateAdventureLosses(gGT, player);

		gGT->gameMode1 |= END_OF_RACE;
		gGT->gameModeEnd = gGT->gameMode1 & 0x3e0020;

		if ((gGT->gameMode1 & BATTLE_MODE) == 0)
		{
			if (gGT->numPlyrCurrGame != 0)
			{
				MainGameEnd_RecordNonBattleStandings(gGT);
			}
		}
		else if ((gGT->gameMode1 & POINT_LIMIT) != 0)
		{
			MainGameEnd_UpdateBattlePointLimit(gGT);
		}
		else if ((gGT->gameMode1 & LIFE_LIMIT) != 0)
		{
			MainGameEnd_UpdateBattleLifeLimit(gGT);
		}

		MainGameEnd_UpdateStandingsOrder(gGT);
		MainGameEnd_FinalizeDriverClocks(gGT);

		UI_VsQuipAssignAll();
		SubmitName_RestoreName(2);

		if ((gGT->gameMode1 & (RELIC_RACE | TIME_TRIAL)) != 0)
		{
			MainGameEnd_SoloRaceGetReward(1);
			MainGameEnd_CheckTimeTrialGhost(gGT, player);
		}
	}

	Podium_InitModels(gGT);
}
