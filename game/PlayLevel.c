#include <common.h>
#include <extended_lap_times.h>

enum PlayLevelConstants
{
	PLAYLEVEL_DRIVER_COUNT = 8,
	PLAYLEVEL_FINISHLINE_NEAR_DISTANCE = 1200,
	PLAYLEVEL_FINISHLINE_FAR_DISTANCE = 32000,
	PLAYLEVEL_REVERSE_DISTANCE_MAX = 1000,
	PLAYLEVEL_REVERSE_CROSSING_PENALTY = 600,
	PLAYLEVEL_UNSORTED_RANK = -1,
	PLAYLEVEL_FIRST_PLACE_RANK = 0,
	PLAYLEVEL_LOWEST_LAP_SENTINEL = -10,
	PLAYLEVEL_DISTANCE_SENTINEL = 0x3fffffff,
	PLAYLEVEL_FINAL_LAP_SOUND = 0x66,
	PLAYLEVEL_FINAL_LAP_TEXT_FRAMES = CTR_SECONDS_TO_FRAMES(3),
	PLAYLEVEL_CONFETTI_PARTICLES = 250,
	PLAYLEVEL_SINGLE_WINNER_COUNT = 1,
	PLAYLEVEL_FIRST_WINNER_INDEX = 0,
	PLAYLEVEL_WINNER_FADE_CURRENT = 0x1fff,
	PLAYLEVEL_WINNER_FADE_DESIRED = 0x1000,
	PLAYLEVEL_WINNER_FADE_STEP = 0xff78,
	PLAYLEVEL_PASS_VOICELINE_DELAY = 0x4b00,
	PLAYLEVEL_PASS_VOICELINE = 8,
	PLAYLEVEL_VOICELINE_FLAGS = 0x10,
	PLAYLEVEL_BLASTED_DAMAGE = 2,
};

// Events where the driver shares the track with nobody else, so
// "all other racers have arrived" would already be true on lap one.
static bool PlayLevel_IsSoloOnlyRaceMode(u32 gameMode1)
{
	return ((gameMode1 & TIME_TRIAL) != 0) || ((gameMode1 & RELIC_RACE) != 0) || ((gameMode1 & CRYSTAL_CHALLENGE) != 0);
}

// Whether the auto-end option is allowed to decide the end of this event
bool PlayLevel_CanAutoEndRace(struct GameTracker *gGT)
{
	// Off keeps the retail rules only
	if (!g_config.autoEndRaceWhenOthersFinish)
	{
		return false;
	}

	// Battle ends through its own rules and never runs the lap stats
	if ((gGT->gameMode1 & BATTLE_MODE) != 0)
	{
		return false;
	}

	if (PlayLevel_IsSoloOnlyRaceMode(gGT->gameMode1))
	{
		return false;
	}

	// Levels that are not a race, where nobody can arrive at all. Menus, the
	// adventure hub and cutscenes reach the lap stats in some level builds,
	// and the attract demo races on its own
	if ((gGT->gameMode1 & (MAIN_MENU | ADVENTURE_ARENA | GAME_CUTSCENE)) != 0 || (gGT->boolDemoMode != 0))
	{
		return false;
	}

	return true;
}

// Optional generalization of the multiplayer VS rule: end the race as soon
// as every other racer has arrived, instead of waiting for the last kart
static bool PlayLevel_ShouldEndWhenOthersArrived(struct GameTracker *gGT)
{
	// Off, solo-only events and battle keep the retail rules only
	if (!PlayLevel_CanAutoEndRace(gGT))
	{
		return false;
	}

	int totalRacerCount = gGT->numPlyrNextGame + gGT->numBotsNextGame;

	// Everyone arrived except at most the one kart still driving
	return sdata->numPlayersFinishedRace >= (totalRacerCount - 1);
}

// Development aid hack, enabled by the "End Race With L3" option. A race otherwise
// takes a whole route to run, which makes reaching the podium, the results and the
// reward flow slow to check. The placings handed over are the ones the field is
// currently standing on, so this ends the race rather than winning it.
static bool PlayLevel_ShouldEndRaceOnL3(struct GameTracker *gGT)
{
	if (!g_config.endRaceWithL3)
	{
		return false;
	}

	// Menus, the adventure hub and cutscenes are not races to end, a pause menu
	// belongs to the race it interrupted, and an attract demo race has to run its
	// own length.
	if ((gGT->gameMode1 & (MAIN_MENU | ADVENTURE_ARENA | GAME_CUTSCENE | PAUSE_ALL)) != 0 ||
	    (gGT->boolDemoMode != 0))
	{
		return false;
	}

	return (GAMEPADS->anyoneTapped & BTN_L3) != 0;
}

// Confetti celebrates the result of the round, so it only belongs to racers that
// actually won it. Retail gives it to whoever crosses the line first, which in
// Adventure co-op happens before the pair knows the outcome, so the co-op pair is
// celebrated from the race-end block instead.
static void PlayLevel_CelebrateWinner(struct GameTracker *gGT, struct Driver *winner, int winnerIndex, int winnerCount)
{
	u8 driverID = winner->driverID;

	// amount of confetti particles
	gGT->confetti.numParticles_max = PLAYLEVEL_CONFETTI_PARTICLES;
	gGT->confetti.vanishRate = PLAYLEVEL_CONFETTI_PARTICLES;

	// how many racers are being celebrated, and the slot this driver ID lands in
	gGT->numWinners = winnerCount;
	gGT->winnerIndex[winnerIndex] = driverID;

	// edit window variables for confetti
	gGT->pushBuffer[driverID].fadeFromBlack_currentValue = PLAYLEVEL_WINNER_FADE_CURRENT;
	gGT->pushBuffer[driverID].fadeFromBlack_desiredResult = PLAYLEVEL_WINNER_FADE_DESIRED;
	gGT->pushBuffer[driverID].fade_step = PLAYLEVEL_WINNER_FADE_STEP;
}

void PlayLevel_CrossFinishLine(struct Driver *driver, bool leadingHuman, bool coopRace)
{
	struct GameTracker *tracker = GAME_TRACKER;
	driver->distanceDrivenBackwards = 0;
	if ((driver->actionsFlagSet & ACTION_BEHIND_START_LINE) != 0)
	{
		driver->actionsFlagSet &= ~ACTION_BEHIND_START_LINE;
		return;
	}
	driver->distanceToFinish_checkpoint = driver->distanceToFinish_curr;
	if (driver->lapIndex < tracker->numLaps)
	{
		if ((tracker->gameMode1 & GAME_MODE_SAVE_LAP_TIME_MASK) != 0 && driver->instSelf->thread->modelIndex == DYNAMIC_PLAYER)
		{
			int lapTime = tracker->elapsedEventTime - driver->lapTime;
			if (tracker->numLaps > EXTENDED_LAP_HISTORY_CAPACITY)
			{
				ExtendedLapTimes_RecordBest(driver->driverID, driver->lapIndex, lapTime);
			}
			else
			{
				UI_SaveLapTime(driver->lapIndex, lapTime, driver->driverID);
				tracker->lapTime[driver->lapIndex] = lapTime;
			}
		}
		driver->lapTime = tracker->elapsedEventTime;
		driver->lapIndex++;
		if (leadingHuman)
		{
			OtherFX_Play(PLAYLEVEL_FINAL_LAP_SOUND, 1);
			Voiceline_ClearTimeStamp();
		}
		if (driver->lapIndex == tracker->numLaps - 1 && (driver->actionsFlagSet & ACTION_BOT) == 0)
		{
			sdata->finalLapTextTimer[driver->driverID] = PLAYLEVEL_FINAL_LAP_TEXT_FRAMES;
		}
	}
	if (driver->lapIndex != tracker->numLaps || (driver->actionsFlagSet & ACTION_RACE_FINISHED) != 0)
	{
		return;
	}
	driver->actionsFlagSet |= ACTION_RACE_FINISHED;
	if ((tracker->gameMode1 & END_OF_RACE) == 0)
	{
		driver->driverRank = sdata->numPlayersFinishedRace++;
	}
	driver->heldItemID = HELD_ITEM_NONE;
	if ((driver->actionsFlagSet & ACTION_BOT) == 0)
	{
		if (!coopRace && driver->driverRank == PLAYLEVEL_FIRST_PLACE_RANK)
		{
			PlayLevel_CelebrateWinner(tracker, driver, PLAYLEVEL_FIRST_WINNER_INDEX, PLAYLEVEL_SINGLE_WINNER_COUNT);
		}
		driver->noItemTimer = 0;
		BOTS_Driver_Convert(driver);
	}
}

void PlayLevel_UpdateLapStats(void)
{
	int bestDriverIndex;
	int effectiveLapIndex;
	struct Driver *farthestHuman;
	struct Driver *currDriver;
	int distToFinish_prev;
	int distToFinish_curr;
	int minDistance;
	int bestLapIndex;
	int driverIndex;
	int finishedHumanCount;
	int currRank;
	b32 coopRace;
	struct GameTracker *gGT = sdata->gGT;

	finishedHumanCount = 0;
	Chaos_ReturnBodySwapsNearFinish(gGT, PLAYLEVEL_FINISHLINE_NEAR_DISTANCE);
	currRank = 0;
	coopRace = MainGameEnd_IsCoopRace(gGT);

	// driver pointer,
	// unlike other "rank" index variables
	farthestHuman = NULL;

	// find farthest-ahead human
	for (int raceOrderIndex = 0; raceOrderIndex < PLAYLEVEL_DRIVER_COUNT; raceOrderIndex++)
	{
		currDriver = gGT->driversInRaceOrder[raceOrderIndex];

		if ((currDriver != 0) && ((currDriver->actionsFlagSet & ACTION_BOT) == 0))
		{
			farthestHuman = currDriver;
			break;
		}
	}

	for (driverIndex = 0; driverIndex < PLAYLEVEL_DRIVER_COUNT; driverIndex++)
	{
		gGT->driversInRaceOrder[driverIndex] = NULL;

		currDriver = gGT->drivers[driverIndex];

		if (currDriver == NULL)
		{
			continue;
		}

		// before and after
		distToFinish_prev = currDriver->distanceToFinish_curr;
		VehLap_UpdateProgress(currDriver);
		distToFinish_curr = currDriver->distanceToFinish_curr;
		const bool teleportedAcrossFinish = Chaos_ConsumeTeleportLapCrossing(currDriver);

		int drivenBackwards = currDriver->distanceDrivenBackwards + (distToFinish_curr - distToFinish_prev);

		// clamp minimum
		if (drivenBackwards < 0)
		{
			drivenBackwards = 0;
		}

		// clamp to max
		else if (drivenBackwards > PLAYLEVEL_REVERSE_DISTANCE_MAX)
		{
			drivenBackwards = PLAYLEVEL_REVERSE_DISTANCE_MAX;
		}

		// update distance driven backwards
		currDriver->distanceDrivenBackwards = drivenBackwards;

		// === Natty Video ===
		// https://www.youtube.com/watch?v=lDaT2rY6GKI

		// Part A: Start-line -> 32000 distToFinish
		// Part B: 32000 distToFinish -> 1200 distToFinish
		// Part C: 1200 distToFinish -> Finish-line

		if ((gGT->gameMode1 & BATTLE_MODE) == 0 &&
		    // crossed finishline (forwards)
		    (teleportedAcrossFinish ||
		     ((distToFinish_prev < PLAYLEVEL_FINISHLINE_NEAR_DISTANCE) && (distToFinish_curr > PLAYLEVEL_FINISHLINE_FAR_DISTANCE))))
		{
			PlayLevel_CrossFinishLine(currDriver, currDriver == farthestHuman, coopRace);
		}

		// if player did not just finish a lap (correctly)
		else
		{
			if (
			    // crossed startline backwards
			    (distToFinish_curr < PLAYLEVEL_FINISHLINE_NEAR_DISTANCE) && (distToFinish_prev > PLAYLEVEL_FINISHLINE_FAR_DISTANCE))
			{
				// automatic backwards penalty
				currDriver->distanceDrivenBackwards = PLAYLEVEL_REVERSE_CROSSING_PENALTY;
				currDriver->actionsFlagSet |= ACTION_BEHIND_START_LINE;
			}

			// if player did not JUST cross finish backwards
			else
			{
				u32 trackLen = gGT->level1->ptr_restart_points[0].distToFinish;

				if (
				    // if player did not EVER cross finish backwards
				    ((currDriver->actionsFlagSet & ACTION_BEHIND_START_LINE) == 0) &&

				    (
				        // if distance driven this frame is less than...
				        (currDriver->distanceToFinish_checkpoint - distToFinish_curr) <=

				        // level's distance to finish
				        ((trackLen >> 2) << 3)))
				{
					// save distance for next frame
					currDriver->distanceToFinish_checkpoint = distToFinish_curr;
				}
			}
		}
		if ((currDriver->actionsFlagSet & ACTION_RACE_FINISHED) == 0)
		{
			currDriver->driverRank = PLAYLEVEL_UNSORTED_RANK;
			continue;
		}

		// === Driver Finished Race ===

		if (currDriver->instSelf->thread->modelIndex == DYNAMIC_PLAYER)
		{
			// count humans to finish race
			finishedHumanCount = finishedHumanCount + 1;
		}

		// increase your rank in the race, someone passed you
		int newRank = currDriver->driverRank + 1;

		// get human in last
		if (currRank < newRank)
		{
			currRank = newRank;
		}
	}

	// sort all drivers that have NOT finished race
	for (; currRank < PLAYLEVEL_DRIVER_COUNT; currRank++)
	{
		// set "min" distance to max
		minDistance = PLAYLEVEL_DISTANCE_SENTINEL;

		// set "highest" lap to min
		bestDriverIndex = PLAYLEVEL_UNSORTED_RANK;

		// lap index
		bestLapIndex = PLAYLEVEL_LOWEST_LAP_SENTINEL;

		// look for "next" farthest driver,
		// out of all unsorted drivers remaining
		for (driverIndex = 0; driverIndex < PLAYLEVEL_DRIVER_COUNT; driverIndex++)
		{
			// get current driver
			currDriver = gGT->drivers[driverIndex];

			if (currDriver == NULL)
			{
				continue;
			}

			if (currDriver->driverRank != PLAYLEVEL_UNSORTED_RANK)
			{
				continue;
			}

			// driver lap index
			effectiveLapIndex = currDriver->lapIndex;

			if ((currDriver->actionsFlagSet & ACTION_BEHIND_START_LINE) != 0)
			{
				effectiveLapIndex -= 1;
			}

			if (
			    // new highest lap
			    (effectiveLapIndex > bestLapIndex) ||

			    // OR

			    (
			        // same lap
			        (bestLapIndex == effectiveLapIndex) &&

			        // AND

			        // new lowest distance (max progress)
			        ((s32)currDriver->distanceToFinish_curr < minDistance)))
			{
				// set new min distToFinish (max progress)
				minDistance = currDriver->distanceToFinish_curr;

				// highest lap
				bestLapIndex = effectiveLapIndex;

				// index of driver closest to finish
				bestDriverIndex = driverIndex;
			}
		}

		if (bestDriverIndex != PLAYLEVEL_UNSORTED_RANK)
		{
			// If traffic lights run out
			if (gGT->trafficLightsTimer < 1)
			{
				gGT->drivers[bestDriverIndex]->driverRank = currRank;
			}

			// if traffic lights >= 1
			else
			{
				// set every driver position rank,
				// to the order that they spawn on the starting line
				gGT->drivers[bestDriverIndex]->driverRank = sdata->kartSpawnOrderArray[bestDriverIndex];
				gGT->humanPlayerPositions[bestDriverIndex] = sdata->kartSpawnOrderArray[bestDriverIndex];
			}
		}
	}

	for (driverIndex = 0; driverIndex < PLAYLEVEL_DRIVER_COUNT; driverIndex++)
	{
		// get pointer to each player structure
		currDriver = gGT->drivers[driverIndex];

		if (currDriver == NULL)
		{
			continue;
		}

		// should be impossible to be -1 here
		if (currDriver->driverRank > PLAYLEVEL_UNSORTED_RANK)
		{
			gGT->driversInRaceOrder[currDriver->driverRank] = currDriver;
		}
	}

	for (driverIndex = 0; driverIndex < gGT->numPlyrCurrGame; driverIndex++)
	{
		// pointer to each player structure
		currDriver = gGT->drivers[driverIndex];

		if (currDriver == NULL)
		{
			continue;
		}

		int currRank = currDriver->driverRank;

		if ((PLAYLEVEL_UNSORTED_RANK < currRank) && (PLAYLEVEL_PASS_VOICELINE_DELAY < gGT->elapsedEventTime) &&
		    ((s8)gGT->humanPlayerPositions[driverIndex] < currRank))
		{
			int characterID = data.characterIDs[gGT->driversInRaceOrder[currRank - 1]->driverID];

			// Make driver talk
			Voiceline_RequestPlay(PLAYLEVEL_PASS_VOICELINE, characterID, PLAYLEVEL_VOICELINE_FLAGS);
		}
		gGT->humanPlayerPositions[driverIndex] = currRank;
	}

	// If already finished race
	if ((gGT->gameMode1 & END_OF_RACE) != 0)
	{
		return;
	}

	int humanPlayerCount = gGT->numPlyrCurrGame;

	// Check if race should end
	bool shouldEndRace = false;

	// Retail only ever leaves a human driving after the trigger below in
	// multiplayer VS, and that is where the end-of-race blast belongs
	bool endedByOthersArrived = false;

	// Retail rules: 1P ends when the human finishes, multiplayer VS ends one
	// human early, and arcade waits for every human player
	if ((
	        // 1P game, with 1 human finished
	        (humanPlayerCount == 1) && (finishedHumanCount > 0)

	            ) ||

	    (
	        // Multiplayer VS, all finished except one; an Adventure co-op race is
	        // not a versus round, so it waits for the last racer below instead
	        (humanPlayerCount > 1) && !coopRace && ((gGT->gameMode1 & ARCADE_MODE) == 0) &&
	            (finishedHumanCount >= (humanPlayerCount - 1))) ||

	    (
	        // Arcade mode or Adventure co-op, all humans finished
	        ((gGT->gameMode1 & ARCADE_MODE) != 0 || coopRace) && (humanPlayerCount <= finishedHumanCount))
	  )
	{
		shouldEndRace = true;
	}

	// Optional: also end the race once every other racer has arrived
	else if (PlayLevel_ShouldEndWhenOthersArrived(gGT))
	{
		shouldEndRace = true;
		endedByOthersArrived = true;
	}

	// Developer hack: L3 ends the race on request. endedByOthersArrived stays clear
	// here, so a racer still out on track is not blasted, which per the block below
	// belongs to a retail VS ending only.
	else if (PlayLevel_ShouldEndRaceOnL3(gGT))
	{
		shouldEndRace = true;
	}

	if (shouldEndRace)
	{
		// End race for all drivers
		for (currRank = 0; currRank < PLAYLEVEL_DRIVER_COUNT; currRank++)
		{
			// Get address of each player structure
			currDriver = gGT->drivers[currRank];

			if (currDriver == NULL)
			{
				continue;
			}

			if ((currDriver->actionsFlagSet & ACTION_RACE_FINISHED) != 0)
			{
				continue;
			}

			currDriver->actionsFlagSet |= ACTION_RACE_FINISHED;

			// remove weapon
			currDriver->heldItemID = HELD_ITEM_NONE;

			// skip AIs
			if ((currDriver->actionsFlagSet & ACTION_BOT) != 0)
			{
				continue;
			}

			// Blasting the driver still out on track is a retail VS ending
			// only, it is not part of ending early on request
			if (endedByOthersArrived)
			{
				continue;
			}

			// === VS Mode ===

			// Adventure co-op has no losing racer, so nobody is blasted
			if (coopRace)
			{
				continue;
			}

			// Make the player Blasted
			VehPickState_NewState(currDriver, PLAYLEVEL_BLASTED_DAMAGE, currDriver, 0);

			// Reduce counters for AttackingPlayer and AttackedByPlayer
			currDriver->numTimesAttackedByPlayer[currDriver->driverID]--;
			currDriver->numTimesAttackingPlayer[currDriver->driverID]--;
		}

		// Adventure co-op has one result for the pair, so the confetti waits until
		// this frame, where it is known whether the pair took the top two places.
		if (coopRace && MainGameEnd_AdventureRaceWon(gGT))
		{
			for (currRank = 0; currRank < humanPlayerCount; currRank++)
			{
				currDriver = gGT->drivers[currRank];

				if (currDriver == NULL)
				{
					continue;
				}

				PlayLevel_CelebrateWinner(gGT, currDriver, currRank, humanPlayerCount);
			}
		}

		MainGameEnd_Initialize();
	}
}
