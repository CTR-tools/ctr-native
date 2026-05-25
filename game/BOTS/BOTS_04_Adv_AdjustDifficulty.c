#include <common.h>

static void BOTS_Adv_LerpDifficulty(s16 *dst, s16 factor)
{
	s16 *lo = sdata->difficultyParams[1];
	s16 *hi = sdata->difficultyParams[0];

	for (s32 i = 0; i < 14; i++)
	{
		dst[i] = lo[i] + (s16)((factor * (hi[i] - lo[i])) / 0xf0);
	}
}

static void BOTS_Adv_CopySpawnOrder(s32 first, s32 second)
{
	*(s32 *)&sdata->kartSpawnOrderArray[0] = first;
	*(s32 *)&sdata->kartSpawnOrderArray[4] = second;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80012598-0x80013374.
void BOTS_Adv_AdjustDifficulty(void)
{
	struct GameTracker *gGT = sdata->gGT;
	u32 gameMode1 = gGT->gameMode1;
	u32 gameMode2 = gGT->gameMode2;
	s32 currDifficulty;
	s16 cupDifficulty = 0;

	// NOTE(aalhendi): Retail stores params1 in slot 1 and params2 in slot 0.
	if ((gameMode1 & ADVENTURE_BOSS) != 0)
	{
		sdata->difficultyParams[1] = data.BossDifficulty[gGT->bossID].params1;
		sdata->difficultyParams[0] = data.BossDifficulty[gGT->bossID].params2;
	}
	else
	{
		sdata->difficultyParams[1] = data.ArcadeDifficulty[gGT->levelID].params1;
		sdata->difficultyParams[0] = data.ArcadeDifficulty[gGT->levelID].params2;
	}

	if ((gameMode1 & ARCADE_MODE) != 0)
	{
		currDifficulty = (u16)gGT->arcadeDifficulty;

		if ((gameMode2 & CHEAT_SUPERHARD) != 0)
		{
			currDifficulty = 0x140;
		}

		cupDifficulty = (s16)((u16)gGT->arcadeDifficulty + 0x50);
	}
	else if ((gameMode1 & ADVENTURE_CUP) != 0)
	{
		s32 track = gGT->cup.trackIndex;
		s32 lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostCupRace[track]);
		s32 maxDifficulty = track * 5;

		if (gGT->cup.cupID == 4)
		{
			lostModifier -= 0xe1;

			if ((gameMode2 & CHEAT_ADV) != 0)
			{
				maxDifficulty = track * 7;
				lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostCupRace[track]) - 0x141;
			}
		}
		else
		{
			lostModifier -= 0xcd;

			if ((gameMode2 & CHEAT_ADV) != 0)
			{
				maxDifficulty = track * 7;
				lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostCupRace[track]) - 300;
			}
		}

		currDifficulty = maxDifficulty - lostModifier;
		cupDifficulty = (s16)(currDifficulty + 0x50);
	}
	else if ((gameMode1 & ADVENTURE_BOSS) != 0)
	{
		s32 bossID = gGT->bossID;
		s32 lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostBossRace[bossID]) - 0xe1;
		s32 maxDifficulty = bossID * 5;

		if ((gameMode2 & CHEAT_ADV) != 0)
		{
			maxDifficulty = bossID * 7;
			lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostBossRace[bossID]) - 0x141;
		}

		currDifficulty = maxDifficulty - lostModifier;
	}
	else
	{
		s16 numTrophies = (s16)gGT->currAdvProfile.numTrophies + 1;
		s32 lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostRacePerLev[gGT->levelID]) - 0x3c;
		s32 maxDifficulty = numTrophies * 0x23;

		if (maxDifficulty < 0)
		{
			maxDifficulty += 3;
		}

		maxDifficulty >>= 2;

		if ((gameMode2 & CHEAT_ADV) != 0)
		{
			maxDifficulty = numTrophies * 0xc;
			lostModifier = BOTS_Adv_NumTimesLostEvent(sdata->advProgress.timesLostRacePerLev[gGT->levelID]) - 100;
		}

		currDifficulty = maxDifficulty - lostModifier;
	}

	if ((s16)currDifficulty < 0)
	{
		currDifficulty = 0;
	}

	BOTS_Adv_LerpDifficulty(sdata->arcade_difficultyParams, (s16)currDifficulty);

	if (((gameMode1 & ADVENTURE_CUP) != 0) || ((gameMode2 & CUP_ANY_KIND) != 0))
	{
		BOTS_Adv_LerpDifficulty(sdata->cup_difficultyParams, cupDifficulty);
	}

	sdata->unk_counter_upTo450 = 0;

	if ((sdata->const_0x30215400 == 0) && (sdata->const_0x493583fe == 0))
	{
		sdata->const_0x30215400 = 0x30215400;
		sdata->const_0x493583fe = 0x493583fe;
	}

	for (s16 i = 0; i < 3; i++)
	{
		LIST_Clear(&sdata->navBotList[i]);

		if ((gameMode1 & (GAME_CUTSCENE | MAIN_MENU)) == 0)
		{
			BOTS_InitNavPath(gGT, i);
		}
	}

	BOTS_SetGlobalNavData(0);

	gGT->numBotsNextGame = 0;

	if (((gameMode2 & CUP_ANY_KIND) == 0) || (gGT->cup.trackIndex == 0))
	{
		if (gGT->numPlyrCurrGame == 2)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.VS_2P_1, data.kartSpawnOrder.VS_2P_2);
		}
		else if ((u8)gGT->numPlyrCurrGame > 2)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.VS_3P_4P_1, data.kartSpawnOrder.VS_3P_4P_2);
		}

		if ((gameMode1 & (RELIC_RACE | TIME_TRIAL)) != 0)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.time_trial_1, data.kartSpawnOrder.time_trial_2);
		}
		else if ((gameMode1 & CRYSTAL_CHALLENGE) != 0)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.crystal_challenge_1, data.kartSpawnOrder.crystal_challenge_2);
		}
		else if ((gameMode1 & ADVENTURE_BOSS) != 0)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.boss_challenge_1, data.kartSpawnOrder.boss_challenge_2);
		}
		else if (((gameMode1 & ADVENTURE_CUP) != 0) && (gGT->cup.cupID == 4))
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.purple_cup_1, data.kartSpawnOrder.purple_cup_2);
		}
		else if ((gameMode1 & ADVENTURE_MODE) == 0)
		{
			BOTS_Adv_CopySpawnOrder(data.kartSpawnOrder.arcade_1, data.kartSpawnOrder.arcade_2);
		}
	}

	u8 pathOrder[8];
	pathOrder[0] = 0;
	pathOrder[4] = 0;
	pathOrder[3] = 2;
	pathOrder[7] = 2;

	u8 firstPath = (RngDeadCoed((u32 *)&sdata->const_0x30215400) >> 8) & 1;
	pathOrder[1] = firstPath;
	pathOrder[5] = firstPath ^ 1;

	u8 secondPath = (RngDeadCoed((u32 *)&sdata->const_0x30215400) >> 8) & 1;
	pathOrder[2] = secondPath + 1;
	pathOrder[6] = (secondPath ^ 1) + 1;

	for (s16 i = 0; i < 8; i++)
	{
		sdata->driver_pathIndexIDs[i] = pathOrder[(u8)sdata->kartSpawnOrderArray[i]];
	}

	if ((gameMode1 & ADVENTURE_BOSS) != 0)
	{
		sdata->driver_pathIndexIDs[0] = 0;
		sdata->driver_pathIndexIDs[1] = 1;
	}

	if ((gameMode1 & BATTLE_MODE) != 0)
	{
		for (s16 i = 0; i < 4; i++)
		{
			sdata->driver_pathIndexIDs[i] = (RngDeadCoed((u32 *)&sdata->const_0x30215400) & 0xfff) / 0x555;
		}
	}

	if ((((gameMode1 & ADVENTURE_CUP) == 0) && ((gameMode2 & CUP_ANY_KIND) == 0)) || (gGT->cup.trackIndex == 0))
	{
		u8 accelOrder[8];
		u32 accel = (RngDeadCoed((u32 *)&sdata->const_0x30215400) >> 8) & 3;
		u32 rearAccel = (RngDeadCoed((u32 *)&sdata->const_0x30215400) >> 8) & 3;

		for (s16 i = 0; i < 4; i++)
		{
			accelOrder[i] = accel;
			accel = (accel + 1) & 3;

			accelOrder[i + 4] = rearAccel + 4;
			rearAccel = (rearAccel - 1) & 3;
		}

		for (s16 i = 0; i < 8; i++)
		{
			sdata->accelerateOrder[i] = accelOrder[(u8)sdata->kartSpawnOrderArray[i]];
		}
	}

	if ((((gameMode1 & ADVENTURE_CUP) != 0) || ((gameMode2 & CUP_ANY_KIND) != 0)) && (gGT->cup.trackIndex > 0))
	{
		s16 bestPoints = -1;
		s16 bestDriverIndex = 0;
		s16 topAccelIndex = 0;

		for (s16 i = 0; i < 8; i++)
		{
			if (((u8)gGT->numPlyrCurrGame <= i) && (bestPoints < gGT->cup.points[i]))
			{
				bestPoints = (s16)gGT->cup.points[i];
				bestDriverIndex = i;
			}

			if (sdata->accelerateOrder[i] == 0)
			{
				topAccelIndex = i;
			}
		}

		char topAccel = sdata->accelerateOrder[topAccelIndex];
		sdata->accelerateOrder[topAccelIndex] = sdata->accelerateOrder[bestDriverIndex];
		sdata->accelerateOrder[bestDriverIndex] = topAccel;
	}
}
