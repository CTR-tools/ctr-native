#include <common.h>

#ifndef MAIN_HIGH_SCORES
#define MAIN_HIGH_SCORES (sdata->ptrActiveHighScoreEntry)
#endif
#ifndef MAIN_LOAD_PROGRESS_PAGE_AFTER
#define MAIN_LOAD_PROGRESS_PAGE_AFTER(page, mode)     ((page) = 0, (void)(mode))
#define MAIN_PROGRESS_FROM_PAGE(result, page)         ((void)(page), (result) = &GAME_PROGRESS)
#define MAIN_LOAD_TROPY_PAGE(page)                    ((page) = 0)
#define MAIN_LOAD_TROPY_LOW(result, page)             ((result) = (u16)data.bitIndex_timeTrialFlags_saveData.nTropyOpen)
#define MAIN_READ_OXIDE_METADATA(metadata)            ((void)sizeof(metadata), (u16)data.bitIndex_timeTrialFlags_saveData.nOxideOpen)
#define MAIN_LOAD_TROPY_FLASH_PAGE_AFTER(page, flags) ((page) = 0, (void)(flags))
#define MAIN_TROPY_FLASH_FROM_PAGE(page)              (data.bitIndex_timeTrialFlags_flashingText.nTropyOpen)
#define MAIN_LOAD_OXIDE_FLASH_PAGE_AFTER(page, flags) ((page) = 0, (void)(flags))
#define MAIN_OXIDE_FLASH_FROM_PAGE(page)              (data.bitIndex_timeTrialFlags_flashingText.nOxideOpen)
#endif


#ifndef MAIN_KART_SPAWN_ORDER
#define MAIN_KART_SPAWN_ORDER   (sdata->kartSpawnOrderArray)
#define MAIN_REPLAY_HUMAN_GHOST (sdata->boolReplayHumanGhost)
#define MAIN_GHOST_TOO_BIG      (sdata->boolGhostTooBigToSave)
#define MAIN_PLAYING_GHOST      (sdata->ptrGhostTapePlaying)
#endif

struct MainGameEndTieSlots
{
	s16 slots[4];
};
#ifndef MAIN_TIE_SEED
static const struct MainGameEndTieSlots MainGameEnd_TieSeed = {{-1, -1, -1, -1}};
#define MAIN_TIE_SEED MainGameEnd_TieSeed
#endif

void MainGameEnd_SoloRaceGetReward(int subtractTimeCrateBonus)
{
	register s32 subtractBonus CTR_PSX_REGISTER("$6") = subtractTimeCrateBonus;
	struct GameTracker *initialTracker;
	register struct Driver *player;
	u32 gameModeEnd;
	register struct GameProgress *progress CTR_PSX_REGISTER("$9");
	register struct GameTracker *gGT CTR_PSX_REGISTER("$4");
	struct HighScoreEntry *raceEntries;
	register s32 timeBonus;
	register s32 raceTime;
	s16 i;
	u32 *trialFlags;
	register u8 *trackBase CTR_PSX_REGISTER("$9");
	register u32 trophyBit CTR_PSX_REGISTER("$4");
	register u16 oxideBit CTR_PSX_REGISTER("$5");
	register u32 levelByteOffset;
	u32 levelStride;
	register s32 levelID;
	u32 trialMode;
	u32 progressPage;
	register u32 bitPage CTR_PSX_REGISTER("$2");
	u8 *oxideBase;
	register s32 trophyOffset CTR_PSX_REGISTER("$3");
	s32 oxideOffset;
	u32 bitFlags;
	u8 *bitBase;
	u16 *oxideMetadata;
	u32 newFlags;
	register u32 oxideFlags CTR_PSX_REGISTER("$4");
	u32 flashPage;
	u32 oxideMask;

	CTR_PSX_BIND_VALUE_CLOBBER(subtractBonus, "$4");
	initialTracker = GAME_TRACKER;
	player = initialTracker->threadBuckets[PLAYER].thread->object;
	*(s8 *)&initialTracker->newHighScoreIndex = -1;
	gameModeEnd = initialTracker->gameModeEnd & 0x7bffffff;
	gGT = GAME_TRACKER;
	initialTracker->gameModeEnd = gameModeEnd;
	timeBonus = 0;
	if ((gGT->drivers[0]->numTimeCrates == gGT->timeCratesInLEV) && (subtractBonus != 0))
	{
		timeBonus = 0x2580;
	}
	gGT = GAME_TRACKER;
	raceTime = gGT->drivers[0]->timeElapsedInRace - timeBonus;
	raceEntries = MAIN_HIGH_SCORES + 1;
	for (i = 0; i < 5; i++)
	{
		register s32 entryIndex = i;
		CTR_PSX_KEEP_VALUE_RELAXED(entryIndex);
		if (raceTime < (s32)raceEntries[entryIndex].time)
		{
			gGT->newHighScoreIndex = i;
			gGT->gameModeEnd |= 0x88000000;
			break;
		}
	}
	gGT = GAME_TRACKER;
	gGT->bestLapTime = MAIN_HIGH_SCORES[0].time;
	for (i = 0; i < gGT->numLaps;)
	{
		if (gGT->lapTime[i] < gGT->bestLapTime)
		{
			gGT->bestLapTime = gGT->lapTime[i];
			gGT->lapIndexNewBest = i;
			gGT->gameModeEnd |= 0x8c000000;
		}
		i++;
		CTR_PSX_RELOAD(gGT->numLaps);
	}
	{
		register struct GameTracker *trialTracker CTR_PSX_REGISTER("$6") = GAME_TRACKER;
		if ((trialTracker->gameMode1 & TIME_TRIAL) == 0)
			return;
		if ((trialTracker->gameModeEnd & 4) != 0)
			return;
		trialMode = trialTracker->gameModeEnd | 4;
		MAIN_LOAD_PROGRESS_PAGE_AFTER(progressPage, trialMode);
		trialTracker->gameModeEnd = trialMode;
		CTR_PSX_OBSERVE_MEMORY(trialTracker->gameModeEnd);
		MAIN_LOAD_TROPY_PAGE(bitPage);
		MAIN_PROGRESS_FROM_PAGE(progress, progressPage);
		MAIN_LOAD_TROPY_LOW(trophyBit, bitPage);
		levelID = trialTracker->levelID;
		CTR_PSX_KEEP_VALUE_RELAXED(levelID);
		trophyOffset = ((s16)trophyBit >> 5) * sizeof(u32);
		levelStride = levelID * (sizeof(struct HighScoreTrack) / sizeof(u32));
		levelByteOffset = levelStride * sizeof(u32);
		CTR_PSX_DEPEND_VALUE(levelByteOffset, levelStride);
		trophyOffset += levelByteOffset;
		CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(bitBase, progress, trophyOffset);
		bitFlags = *(u32 *)(bitBase + offsetof(struct GameProgress, highScoreTracks[0].timeTrialFlags));
		CTR_PSX_DEPEND_VALUE(levelID, bitFlags);
		trophyBit &= 0x1f;
		if (((bitFlags >> trophyBit) & 1) != 0)
		{
			if (!(player->timeElapsedInRace < trialTracker->timeToBeatInTimeTrial_ForCurrentEvent))
				goto CheckOxideAllTracks;
			oxideBit = MAIN_READ_OXIDE_METADATA(oxideMetadata);
			CTR_PSX_LOAD_IMMEDIATE(oxideMask, 1);
			oxideOffset = ((s16)oxideBit >> 5) * sizeof(u32) + levelByteOffset;
			CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(oxideBase, progress, oxideOffset);
			oxideFlags = *(u32 *)(oxideBase + offsetof(struct GameProgress, highScoreTracks[0].timeTrialFlags)) | (oxideMask << (oxideBit & 0x1f));
			MAIN_LOAD_TROPY_FLASH_PAGE_AFTER(flashPage, oxideFlags);
			CTR_PSX_ORDER_VALUES(oxideFlags, flashPage);
			*(u32 *)(oxideBase + offsetof(struct GameProgress, highScoreTracks[0].timeTrialFlags)) = oxideFlags;
			trialTracker->gameModeEnd |= MAIN_TROPY_FLASH_FROM_PAGE(flashPage);
		}
		else
		{
			CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(trackBase, progress, levelByteOffset);
			trialFlags = (u32 *)(trackBase + offsetof(struct GameProgress, highScoreTracks[0].timeTrialFlags));
			if ((*trialFlags & 1) != 0)
			{
				if (!(player->timeElapsedInRace < trialTracker->timeToBeatInTimeTrial_ForCurrentEvent))
					goto CheckOxideAllTracks;
				newFlags = bitFlags | (1u << trophyBit);
				MAIN_LOAD_OXIDE_FLASH_PAGE_AFTER(flashPage, newFlags);
				CTR_PSX_ORDER_VALUES(newFlags, flashPage);
				*(u32 *)(bitBase + offsetof(struct GameProgress, highScoreTracks[0].timeTrialFlags)) = newFlags;
				trialTracker->gameModeEnd |= MAIN_OXIDE_FLASH_FROM_PAGE(flashPage);
			}
			else
			{
				if (!(player->timeElapsedInRace < GAME_LEVEL_METADATA[levelID].timeTrial))
					goto CheckOxideAllTracks;
				*trialFlags |= 1;
				trialTracker->gameModeEnd |= 0x08008000;
			}
		}
	}
CheckOxideAllTracks:
	if ((s16)GAMEPROG_CheckGhostsBeaten(2))
	{
		UNLOCK_ADV_BIT(GAME_PROGRESS.unlocks, GAME_UNLOCK_BIT_SCRAPBOOK);
	}
}


void MainGameEnd_SoloRaceSaveHighScore(void)
{
	struct Driver *player = GAME_TRACKER->drivers[0];
	register struct GameTracker *gGT;
	u32 gameModeEnd;
	u32 savedMode;
	s16 characterID;
	struct HighScoreEntry *entry;
	register int highScoreIndex;
	register int shiftIndex;

	MainGameEnd_SoloRaceGetReward(0);
	gGT = GAME_TRACKER;

	gameModeEnd = gGT->gameModeEnd;

	if ((gameModeEnd & HIGH_SCORE_SAVED) != 0)
	{
		return;
	}

	savedMode = gameModeEnd | HIGH_SCORE_SAVED;
	gGT->gameModeEnd = savedMode;


	if ((savedMode & NEW_BEST_LAP) != 0)
	{
		register struct HighScoreEntry *lapEntry = MAIN_HIGH_SCORES;
		lapEntry[0].time = gGT->bestLapTime;
		characterID = GAME_CHARACTER_IDS[(u8)player->driverID];
		lapEntry[0].name[0] = 0;
		lapEntry[0].characterID = characterID;
		memmove(MAIN_HIGH_SCORES[0].name, GAME_TRACKER->prevNameEntered, 0x11);
	}

	highScoreIndex = (s8)GAME_TRACKER->newHighScoreIndex;
	CTR_PSX_KEEP_VALUE_RELAXED(highScoreIndex);

	if (highScoreIndex < 0)
	{
		return;
	}

	entry = &MAIN_HIGH_SCORES[highScoreIndex + 1];

	CTR_PSX_DEPEND_VALUE(highScoreIndex, entry);
	CTR_PSX_COPY_VALUE(shiftIndex, highScoreIndex);
	if (shiftIndex < 4)
	{
		memmove(&entry[1], entry, (4 - shiftIndex) * sizeof(struct HighScoreEntry));
	}

	entry->time = player->timeElapsedInRace;
	characterID = GAME_CHARACTER_IDS[(u8)player->driverID];
	entry->name[0] = 0;
	entry->characterID = characterID;
	memmove(entry->name, GAME_TRACKER->prevNameEntered, 0x11);
}


void MainGameEnd_Initialize(void)
{
	struct Driver *player;

	if ((GAME_TRACKER->gameMode1 & END_OF_RACE) == 0)
	{
		int i;
		int j;
		int k;
		// Both battle modes seed this scratch table before sorting tied teams.
		struct MainGameEndTieSlots battleTies;

		player = GAME_TRACKER->drivers[0];

		for (i = 0; i < 8; i++)
		{
			struct Driver *driver = GAME_TRACKER->drivers[i];

			if (driver != NULL)
			{
				MAIN_KART_SPAWN_ORDER[i] = driver->driverRank;
			}
		}

		{
			s32 noHighScore = -1;

			// NOTE(aalhendi): Keep the signed sentinel before the retail byte store.
			CTR_PSX_KEEP_VALUE_RELAXED(noHighScore);
			GAME_TRACKER->newHighScoreIndex = noHighScore;
		}

		if ((GAME_TRACKER->gameMode1 & ROLLING_ITEM) != 0)
		{
			OtherFX_Stop2(0x5d);
			GAME_TRACKER->gameMode1 &= ~ROLLING_ITEM;
		}

		if ((GAME_TRACKER->gameMode1 & (ADVENTURE_CUP | RELIC_RACE | ADVENTURE_MODE)) == ADVENTURE_MODE)
		{
			if (player->driverRank != 0)
			{
				register s8 *lossCounter;
				register s8 *lossBase CTR_PSX_REGISTER("v0");
				s32 lossIndex;

				if (IS_BOSS_RACE(GAME_TRACKER->gameMode1))
				{
					lossBase = (s8 *)&GAME_ADV_PROGRESS;
					CTR_PSX_KEEP_VALUE_RELAXED(lossBase);
					lossIndex = GAME_TRACKER->bossID + offsetof(struct AdvProgress, timesLostBossRace) - offsetof(struct AdvProgress, timesLostRacePerLev);
				}
				else
				{
					lossBase = (s8 *)&GAME_ADV_PROGRESS;
					CTR_PSX_KEEP_VALUE_RELAXED(lossBase);
					lossIndex = GAME_TRACKER->levelID;
				}

				CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(lossCounter, lossBase, lossIndex);
				CTR_PSX_KEEP_VALUE_RELAXED(lossCounter);
				if (lossCounter[offsetof(struct AdvProgress, timesLostRacePerLev)] < 10)
				{
					lossCounter[offsetof(struct AdvProgress, timesLostRacePerLev)]++;
				}
			}
			else
			{
				if (IS_BOSS_RACE(GAME_TRACKER->gameMode1))
				{
					GAME_ADV_PROGRESS.timesLostBossRace[GAME_TRACKER->bossID] = 0;
				}
				else
				{
					GAME_ADV_PROGRESS.timesLostRacePerLev[GAME_TRACKER->levelID] = 0;
				}
			}
		}

		GAME_TRACKER->gameMode1 |= END_OF_RACE;
		GAME_TRACKER->gameModeEnd = GAME_TRACKER->gameMode1 & GAME_MODE_END_RETAINED_MODE_MASK;

		if ((GAME_TRACKER->gameMode1 & BATTLE_MODE) == 0)
		{
			struct GameTracker *tracker = GAME_TRACKER;
			register s32 rowByteOffset CTR_PSX_REGISTER("a2");

			if (tracker->numPlyrCurrGame != 0)
			{
				i = 0;
				CTR_PSX_COPY_VALUE(rowByteOffset, i);
				do
				{
					struct Driver *driver = tracker->drivers[i];
					int rank = driver->driverRank;

					if (rank < 3)
					{
						(*(s32 *)((u8 *)tracker + (rank * sizeof(tracker->standingsPoints[0]) + rowByteOffset) +
						          offsetof(struct GameTracker, standingsPoints)))++;
					}
					rowByteOffset += 3 * sizeof(tracker->standingsPoints[0]);
					i++;
				} while (i < tracker->numPlyrCurrGame);
			}
		}
		else if ((GAME_TRACKER->gameMode1 & POINT_LIMIT) != 0)
		{
			s16 bestScore = -400;
			s16 numTies = 0;
			register u8 usedTeams = 0;
			register u32 teamMaskBit;

			CTR_PSX_KEEP_VALUE_RELAXED(usedTeams);
			i = 0;
			CTR_PSX_OBSERVE_VALUE(i);
			{
			// NOTE(aalhendi): Keep the retail seed-copy pointer lifetime within initialization.
				register struct MainGameEndTieSlots *pointTieTarget CTR_PSX_REGISTER("t1") = &battleTies;

				CTR_PSX_ORDER_VALUES(i, pointTieTarget);
				CTR_PSX_LOAD_IMMEDIATE(teamMaskBit, 1);
				*pointTieTarget = MAIN_TIE_SEED;
			}

			for (; i < 4; i++)
			{
				for (j = 3; j >= 0; j--)
				{
					if ((GAME_TRACKER->battleSetup.pointsPerTeam[j] == bestScore) && (((usedTeams >> j) & 1) == 0))
					{
						numTies++;
						battleTies.slots[numTies] = j;
					}
					else
					{
						if (GAME_TRACKER->battleSetup.pointsPerTeam[j] < bestScore)
						{
							continue;
						}

						if (((usedTeams >> j) & 1) != 0)
						{
							continue;
						}

						bestScore = GAME_TRACKER->battleSetup.pointsPerTeam[j];

						for (k = 0; k < numTies + 1; k++)
						{
							int oldTeam = battleTies.slots[k];

							if (oldTeam != -1)
							{
								usedTeams &= ~(teamMaskBit << oldTeam);
							}
						}

						numTies = 0;
						battleTies.slots[0] = j;
					}

					usedTeams |= teamMaskBit << j;
				}

				if (battleTies.slots[0] != -1)
				{
					for (j = 0; j < numTies + 1; j++)
					{
						GAME_TRACKER->battleSetup.finishedRankOfEachTeam[battleTies.slots[j]] = i;
					}
				}

				i += numTies;
				bestScore = -400;
				battleTies.slots[0] = -1;
				numTies = 0;
			}

			{
				s32 rowByteOffset;

				for (i = 0, rowByteOffset = i; i < 4; i++, rowByteOffset += 3 * sizeof(GAME_TRACKER->standingsPoints[0]))
				{
					int finishedRank = GAME_TRACKER->battleSetup.finishedRankOfEachTeam[i];
					struct Driver *driver;

					if (finishedRank != 3)
					{
						(*(s32 *)((u8 *)GAME_TRACKER + (finishedRank * sizeof(GAME_TRACKER->standingsPoints[0]) + rowByteOffset) +
						          offsetof(struct GameTracker, standingsPoints)))++;
					}

					driver = GAME_TRACKER->drivers[i];

					if ((driver != NULL) && (GAME_TRACKER->battleSetup.finishedRankOfEachTeam[driver->BattleHUD.teamID] == 0))
					{
						GAME_TRACKER->winnerIndex[GAME_TRACKER->numWinners] = driver->driverID;
						GAME_TRACKER->pushBuffer[GAME_TRACKER->winnerIndex[GAME_TRACKER->numWinners]].fadeFromBlack_currentValue = 0x1fff;
						GAME_TRACKER->pushBuffer[GAME_TRACKER->winnerIndex[GAME_TRACKER->numWinners]].fadeFromBlack_desiredResult = 0x1000;
						GAME_TRACKER->pushBuffer[GAME_TRACKER->winnerIndex[GAME_TRACKER->numWinners]].fade_step = 0xff78;
						BOTS_Driver_Convert(GAME_TRACKER->drivers[i]);
						GAME_TRACKER->numWinners++;
					}

					GAME_TRACKER->confetti.numParticles_max = 0xfa;
					GAME_TRACKER->confetti.vanishRate = 0xfa;
				}
			}
		}
		else if ((GAME_TRACKER->gameMode1 & LIFE_LIMIT) != 0)
		{
			s16 bestLives = -400;

			s16 numTies;
			u8 usedSlots;
			register s16 *tieBase CTR_PSX_REGISTER("s5");

			battleTies = MAIN_TIE_SEED;
			numTies = 0;
			usedSlots = 0;

			i = 0;
			CTR_PSX_OBSERVE_VALUE(i);
			tieBase = battleTies.slots;

			for (; i < 4; i++)
			{
				struct GameTracker *scanTracker = GAME_TRACKER;

				for (j = 3; j >= 0; j--)
				{
					struct Driver *driver = scanTracker->drivers[j];
					int lives;

#if defined(CTR_NATIVE)
					// NOTE(aalhendi): Native treats an unused slot as zero lives and skips it.
					if (driver == NULL)
					{
						lives = 0;
					}
					else
#endif
					{
						lives = driver->BattleHUD.numLives;
					}

					if (lives == 0)
					{
						continue;
					}

					if ((lives == bestLives) && (((usedSlots >> j) & 1) == 0))
					{
						numTies++;
						{
							s16 *nextTie;

							CTR_PSX_ADD_POINTER_OFFSET(nextTie, tieBase, (s32)numTies * 2);
							*nextTie = j;
						}
						usedSlots |= 1 << j;
						continue;
					}

					if (scanTracker->drivers[j]->BattleHUD.numLives < bestLives)
					{
						continue;
					}

					if (((usedSlots >> j) & 1) != 0)
					{
						continue;
					}

					bestLives = scanTracker->drivers[j]->BattleHUD.numLives;

					k = 0;
					if (numTies + 1 > 0)
					{
						s16 *clearCursor;
						s32 clearLimit = numTies + 1;

						clearCursor = tieBase;
						do
						{
							int oldSlot = *clearCursor;

							if (oldSlot != -1)
							{
								usedSlots &= ~(1 << oldSlot);
							}
							k++;
							clearCursor++;
						} while (k < clearLimit);
					}

					numTies = 0;
					{
						struct Driver *teamDriver = scanTracker->drivers[j];
						u16 team;

						{
							team = teamDriver->BattleHUD.teamID;
						}

						battleTies.slots[0] = team;
					}
					usedSlots |= 1 << battleTies.slots[0];
				}

				if (battleTies.slots[0] != -1)
				{
					u32 rankByteOffset;
					s16 *tieCursor;

					if (numTies + 1 > 0)
					{
						j = 0;
						rankByteOffset = i * sizeof(GAME_TRACKER->standingsPoints[0]);
						tieCursor = tieBase;
						do
						{
							struct GameTracker *outputTracker = GAME_TRACKER;
							struct Driver *teamDriver = outputTracker->drivers[*tieCursor];
							int team;

#if defined(CTR_NATIVE)
							// NOTE(aalhendi): Native keeps a zero-team fallback for an unused
							// representative in the mixed teamID/slot algorithm.
							if (teamDriver == NULL)
							{
								team = 0;
							}
							else
#endif
							{
								team = teamDriver->BattleHUD.teamID;
							}

#if defined(CTR_NATIVE)
							// NOTE(aalhendi): Retail index 12 reaches the adjacent currLEV word.
							// Native selects that existing field without indexing past standingsPoints.
							if (team * 3 + i == (s32)len(GAME_TRACKER->standingsPoints))
							{
								GAME_TRACKER->currLEV = CTR_MipsAddLo(GAME_TRACKER->currLEV, 1);
							}
							else
#endif
							{
								u32 standingsOffset;
								register void *standingsRow CTR_PSX_REGISTER("v0");

								CTR_PSX_ADD_U32(standingsOffset, rankByteOffset, team * 3 * sizeof(outputTracker->standingsPoints[0]));
								CTR_PSX_ADD_POINTER_OFFSET(standingsRow, outputTracker, standingsOffset);
								(*(s32 *)((u8 *)standingsRow + offsetof(struct GameTracker, standingsPoints)))++;
							}

							if (i == 0)
							{
								struct Driver *driver = outputTracker->drivers[*tieCursor];

								if (driver != NULL)
								{
									outputTracker->winnerIndex[outputTracker->numWinners] = driver->driverID;
									outputTracker->pushBuffer[outputTracker->winnerIndex[outputTracker->numWinners]].fadeFromBlack_currentValue = 0x1fff;
									outputTracker->pushBuffer[outputTracker->winnerIndex[outputTracker->numWinners]].fadeFromBlack_desiredResult = 0x1000;
									outputTracker->pushBuffer[outputTracker->winnerIndex[outputTracker->numWinners]].fade_step = 0xff78;
									BOTS_Driver_Convert(outputTracker->drivers[*tieCursor]);
									GAME_TRACKER->numWinners++;
								}
							}

							GAME_TRACKER->confetti.numParticles_max = 0xfa;
							GAME_TRACKER->confetti.vanishRate = 0xfa;
							GAME_TRACKER->battleSetup.finishedRankOfEachTeam[*tieCursor] = i;

							{
								register s32 numPlayers CTR_PSX_REGISTER("a0") = GAME_TRACKER->numPlyrCurrGame;

								k = 0;
								if (numPlayers != 0)
								{
									s32 markLimit;
									struct Driver *markDriver;
									int markTeam;

									CTR_PSX_COPY_VALUE(markLimit, numPlayers);
									markDriver = GAME_TRACKER->drivers[*tieCursor];

#if defined(CTR_NATIVE)
									// NOTE(aalhendi): Re-read after winner conversion; native keeps
									// its zero-team fallback for an unused slot.
									if (markDriver == NULL)
									{
										markTeam = 0;
									}
									else
#endif
									{
										markTeam = markDriver->BattleHUD.teamID;
									}

									do
									{
										struct Driver *slotDriver = GAME_TRACKER->drivers[k];

										if (slotDriver->BattleHUD.teamID == markTeam)
										{
											usedSlots |= 1 << k;
										}
										k++;
									} while (k < markLimit);
								}
							}
							j++;
							tieCursor++;
						} while (j < numTies + 1);
					}
				}
				i += numTies;
				bestLives = -400;
				battleTies.slots[0] = -1;
				numTies = 0;
			}
		}

		{
			s32 score;
			register s32 nextScore CTR_PSX_REGISTER("v1");
			register s16 bestScore;
			s16 bestTeam;
			register u8 usedTeams;
			s32 rowByteOffset;
			register s32 maxWeight;
			register struct GameTracker *standingsTracker;
			register u32 teamBit CTR_PSX_REGISTER("t0");

			score = 0;
			CTR_PSX_KEEP_VALUE_RELAXED(score);
			bestScore = score;
			bestTeam = -1;
			usedTeams = score;
			CTR_PSX_ORDER_VALUES(bestScore, usedTeams);

			i = 0;
			CTR_PSX_LOAD_IMMEDIATE(maxWeight, 3);
			CTR_PSX_DEPEND_MEMORY(&GAME_TRACKER, maxWeight);
			standingsTracker = GAME_TRACKER;
			CTR_PSX_OBSERVE_VALUE(standingsTracker);
			for (rowByteOffset = i; i < 4; i++, rowByteOffset += 3 * sizeof(standingsTracker->standingsPoints[0]))
			{
				s32 columnByteOffset;

				j = 0;
				CTR_PSX_OBSERVE_VALUE(j);
				columnByteOffset = rowByteOffset;
				for (; j < 3; columnByteOffset += sizeof(standingsTracker->standingsPoints[0]), j++)
				{
					nextScore = score + (*(s32 *)((u8 *)standingsTracker + columnByteOffset + offsetof(struct GameTracker, standingsPoints))) * (maxWeight - j);

					CTR_PSX_KEEP_VALUE_RELAXED(nextScore);
					score = nextScore;
				}

				GAME_TRACKER->battleSetup.standingsScore[i] = (s16)nextScore;
				CTR_PSX_OBSERVE_VALUE(nextScore);
				score = 0;
			}

			CTR_PSX_CLOBBER("a1");
			i = 0;
			CTR_PSX_OBSERVE_VALUE(i);
			CTR_PSX_LOAD_IMMEDIATE(teamBit, 1);

			for (; i < 4; i++)
			{
				for (j = 3; j >= 0; j--)
				{
					if (GAME_TRACKER->battleSetup.standingsScore[j] < bestScore)
					{
						continue;
					}

					if (((usedTeams >> j) & 1) != 0)
					{
						continue;
					}

					if (((GAME_TRACKER->battleSetup.teamFlags & (teamBit << j)) == 0) && ((GAME_TRACKER->gameMode1 & BATTLE_MODE) != 0))
					{
						continue;
					}

					bestScore = GAME_TRACKER->battleSetup.standingsScore[j];

					if (bestTeam != -1)
					{
						usedTeams &= ~(teamBit << bestTeam);
					}

					bestTeam = j;
					CTR_PSX_KEEP_VALUE_RELAXED(teamBit);
					usedTeams |= teamBit << j;
				}

				GAME_TRACKER->battleSetup.standingsOrder[i] = bestTeam;

				bestScore = 0;
				bestTeam = -1;
			}
		}

		{
			register struct Thread *thread CTR_PSX_REGISTER("s0") = GAME_TRACKER->threadBuckets[PLAYER].thread;

			CTR_PSX_OBSERVE_VALUE(thread);

			while (thread != NULL)
			{
				UI_RaceEnd_GetDriverClock(thread->object);
				thread = thread->siblingThread;
			}
		}

		UI_VsQuipAssignAll();
		SubmitName_RestoreName(2);

		if ((GAME_TRACKER->gameMode1 & (RELIC_RACE | TIME_TRIAL)) != 0)
		{
			MainGameEnd_SoloRaceGetReward(1);

			if ((GAME_TRACKER->gameMode1 & RELIC_RACE) != 0)
			{
				RR_EndEvent_UnlockAward();
			}
			else if (!((MAIN_REPLAY_HUMAN_GHOST != 0) && ((MAIN_GHOST_TOO_BIG != 0) || (player->timeElapsedInRace >= MAIN_PLAYING_GHOST->timeElapsedInRace))))
			{
				GhostTape_End();
				GAME_TRACKER->gameModeEnd |= PLAYER_GHOST_BEAT;
			}
		}
	}

	Podium_InitModels(GAME_TRACKER);
}
