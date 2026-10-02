#include <common.h>

void MainStats_ClearBattleVS(void)
{
	// NOTE(aalhendi): Keep the ascending row/rank counters in their retail
	// registers while stepping the canonical standings byte offsets.
	register s32 team CTR_PSX_REGISTER("$6");
	register s32 rank CTR_PSX_REGISTER("$4");
	s32 rowByteOffset;
	s32 cellByteOffset;
	struct GameTracker *gGT;
	team = 0;
	gGT = sdata->gGT;
	for (rowByteOffset = team; team < 4; team++, rowByteOffset += 3 * sizeof(gGT->standingsPoints[0]))
	{
		for (rank = 0, cellByteOffset = rowByteOffset; rank < 3; rank++, cellByteOffset += sizeof(gGT->standingsPoints[0]))
		{
			*(s32 *)((u8 *)gGT + cellByteOffset + offsetof(struct GameTracker, standingsPoints)) = 0;
		}
	}
}

void MainStats_RestartRaceCountLoss(void)
{
	s32 i;
	s32 index;
	s8 *countPtr;
	s8 *lossBase;
	s32 gameMode1;
	struct GameTracker *initialTracker;
	struct GameTracker *teamTracker;
	struct GameTracker *gGT;
	// NOTE(aalhendi): The level reload uses v1 while the profile page stays
	// in v0 across the saturated-boss fallthrough.
	register struct GameTracker *levelTracker CTR_PSX_REGISTER("$3");
	initialTracker = sdata->gGT;
	initialTracker->hudFlags &= HUD_FLAG_CLEAR_RACE_HUD_MASK;
	teamTracker = sdata->gGT;
	for (i = 0; i < 4; i++)
	{
		if ((teamTracker->battleSetup.teamFlags & (1 << i)) != 0)
			teamTracker->battleSetup.pointsPerTeam[i] = 0;
		else
			// Inactive teams must stay below legitimate same-team kill scores.
			teamTracker->battleSetup.pointsPerTeam[i] = -500;
	}
	gGT = sdata->gGT;
	gameMode1 = gGT->gameMode1;
	sdata->TurboDisplayPos_Only1P = 0;
	sdata->numPlayersFinishedRace = 0;
	gGT->numWinners = 0;
	gGT->numCrystalsInLEV = 0;
	gGT->timeCratesInLEV = 0;
	gGT->gameModeEnd = 0;
	gGT->frozenTimeRemaining = 0;
	gGT->numMissiles = 0;
	gGT->numPlayersWith3Missiles = 0;
	if ((gameMode1 & ADVENTURE_MODE) == 0)
		return;
#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Native already returns from excluded adventure modes
	// before looking up a driver, including when driver 0 is NULL.
	if ((gameMode1 & (RELIC_RACE | ADVENTURE_CUP)) != 0)
		return;
#endif
	if (gGT->drivers[0]->lapIndex != 2)
		return;
	if ((gameMode1 & (RELIC_RACE | ADVENTURE_CUP)) != 0)
		return;
	// NOTE(aalhendi): Retail joins both loss paths through the same canonical
	// profile-byte view. Derive their offsets from the signed counter fields.
	if (IS_BOSS_RACE(gameMode1))
	{
		lossBase = (s8 *)&GAME_ADV_PROGRESS;
		index = gGT->bossID + offsetof(struct AdvProgress, timesLostBossRace) - offsetof(struct AdvProgress, timesLostRacePerLev);
		countPtr = lossBase + index;
		if (countPtr[offsetof(struct AdvProgress, timesLostRacePerLev)] < 10)
		{
			countPtr[offsetof(struct AdvProgress, timesLostRacePerLev)]++;
			return;
		}
	}
	levelTracker = sdata->gGT;
	lossBase = (s8 *)&GAME_ADV_PROGRESS;
	index = levelTracker->levelID;
	countPtr = lossBase + index;
	if (countPtr[offsetof(struct AdvProgress, timesLostRacePerLev)] < 10)
		countPtr[offsetof(struct AdvProgress, timesLostRacePerLev)]++;
}
