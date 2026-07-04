#include <common.h>

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
	PLAYLEVEL_NO_ITEM = 0xf,
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

CTR_STATIC_ASSERT(PLAYLEVEL_DRIVER_COUNT == 8);
CTR_STATIC_ASSERT(PLAYLEVEL_FINAL_LAP_TEXT_FRAMES == 90);

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800414f4-0x80041c84
void PlayLevel_UpdateLapStats(void)
{
	int bestDriverIndex;
	u8 lapCounter;
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
	struct GameTracker *gGT = sdata->gGT;

	finishedHumanCount = 0;
	currRank = 0;

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

		if (
		    // crossed finishline (forwards)
		    (distToFinish_prev < PLAYLEVEL_FINISHLINE_NEAR_DISTANCE) && (distToFinish_curr > PLAYLEVEL_FINISHLINE_FAR_DISTANCE))
		{
			// Set racer's distance driven backwards to zero
			currDriver->distanceDrivenBackwards = 0;

			if ((currDriver->actionsFlagSet & ACTION_BEHIND_START_LINE) != 0)
			{
				currDriver->actionsFlagSet &= ~ACTION_BEHIND_START_LINE;

				goto UpdateFinishedDriverRank;
			}

			// update checkpoint with distToFinish
			currDriver->distanceToFinish_checkpoint = distToFinish_curr;

			// If finished last lap, clamp
			if (gGT->numLaps < (currDriver->lapIndex + 1))
			{
				lapCounter = currDriver->lapIndex;
			}

			// if this is not final lap
			else
			{
				if (
				    // If you're in Arcade, or
				    // If you're in Adventure, or
				    // If you're in Time Trial
				    ((gGT->gameMode1 & GAME_MODE_SAVE_LAP_TIME_MASK) != 0) &&

				    // player of any kind
				    (currDriver->instSelf->thread->modelIndex == DYNAMIC_PLAYER))
				{
					UI_SaveLapTime(currDriver->lapIndex, gGT->elapsedEventTime - currDriver->lapTime, currDriver->driverID);

					gGT->lapTime[currDriver->lapIndex] = gGT->elapsedEventTime - currDriver->lapTime;
				}

				// time on the clock
				currDriver->lapTime = gGT->elapsedEventTime;

				// lap counter = lap counter + 1
				currDriver->lapIndex++;

				// if farthest-ahead human
				if (currDriver == farthestHuman)
				{
					OtherFX_Play(PLAYLEVEL_FINAL_LAP_SOUND, 1);
					Voiceline_ClearTimeStamp();
				}

				lapCounter = currDriver->lapIndex;

				// If Final Lap
				if (lapCounter == (gGT->numLaps - 1))
				{
					if ((currDriver->actionsFlagSet & ACTION_BOT) == 0)
					{
						// frames, so the animation lasts 3 seconds
						sdata->finalLapTextTimer[driverIndex] = PLAYLEVEL_FINAL_LAP_TEXT_FRAMES;
					}
				}
			}

			// If did not just finish race
			if (lapCounter != gGT->numLaps)
			{
				goto UpdateFinishedDriverRank;
			}

			// === If did just finish race ===

			if ((currDriver->actionsFlagSet & ACTION_RACE_FINISHED) == 0)
			{
				currDriver->actionsFlagSet |= ACTION_RACE_FINISHED;

				// === Run on first frame that race ends ===

				// if total event hasn't finished (gGT->gameMode1)
				if ((gGT->gameMode1 & END_OF_RACE) == 0)
				{
					// set driver placement rank, based on
					// how many drivers have finished the race
					currDriver->driverRank = sdata->numPlayersFinishedRace;
					sdata->numPlayersFinishedRace++;
				}

				// you have no weapon
				currDriver->heldItemID = PLAYLEVEL_NO_ITEM;

				if ((currDriver->actionsFlagSet & ACTION_BOT) == 0)
				{
					// If this racer is in first place
					if (currDriver->driverRank == PLAYLEVEL_FIRST_PLACE_RANK)
					{
						// amount of confetti particles
						gGT->confetti.numParticles_max = PLAYLEVEL_CONFETTI_PARTICLES;
						gGT->confetti.vanishRate = PLAYLEVEL_CONFETTI_PARTICLES;

						// one person won,
						// one person gets confetti
						gGT->numWinners = PLAYLEVEL_SINGLE_WINNER_COUNT;

						u8 driverID = currDriver->driverID;

						// add driver ID to array of confetti winners
						gGT->winnerIndex[PLAYLEVEL_FIRST_WINNER_INDEX] = driverID;

						// edit window variables for confetti
						gGT->pushBuffer[driverID].fadeFromBlack_currentValue = PLAYLEVEL_WINNER_FADE_CURRENT;
						gGT->pushBuffer[driverID].fadeFromBlack_desiredResult = PLAYLEVEL_WINNER_FADE_DESIRED;
						gGT->pushBuffer[driverID].fade_step = PLAYLEVEL_WINNER_FADE_STEP;
					}
					if (currDriver->noItemTimer != 0)
					{
						currDriver->noItemTimer = 0;
						currDriver->heldItemID = PLAYLEVEL_NO_ITEM;
					}

					// turn driver into robotcar
					BOTS_Driver_Convert(currDriver);
				}
				goto UpdateFinishedDriverRank;
			}
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

		UpdateFinishedDriverRank:
			if ((currDriver->actionsFlagSet & ACTION_RACE_FINISHED) == 0)
			{
				// set rank to "unsorted"
				currDriver->driverRank = PLAYLEVEL_UNSORTED_RANK;

				// skip next 5 lines of code
				continue;
			}
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
	if ((
	        // 1P game, with 1 human finished
	        (humanPlayerCount == 1) && (finishedHumanCount > 0)

	            ) ||

	    (
	        // Multiplayer VS, all finished except one
	        (humanPlayerCount > 1) && ((gGT->gameMode1 & ARCADE_MODE) == 0) && (finishedHumanCount >= (humanPlayerCount - 1))) ||

	    (
	        // Arcade mode, all humans finished
	        ((gGT->gameMode1 & ARCADE_MODE) != 0) && (humanPlayerCount <= finishedHumanCount)))
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
			currDriver->heldItemID = PLAYLEVEL_NO_ITEM;

			// skip AIs
			if ((currDriver->actionsFlagSet & ACTION_BOT) != 0)
			{
				continue;
			}

			// === VS Mode ===

			// Make the player Blasted
			VehPickState_NewState(currDriver, PLAYLEVEL_BLASTED_DAMAGE, currDriver, 0);

			// Reduce counters for AttackingPlayer and AttackedByPlayer
			currDriver->numTimesAttackedByPlayer[currDriver->driverID]--;
			currDriver->numTimesAttackingPlayer[currDriver->driverID]--;
		}

		MainGameEnd_Initialize();
	}
}
