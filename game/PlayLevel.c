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


void PlayLevel_UpdateLapStats(void)
{
	s32 bestDriverIndex;
	struct Driver *farthestHuman;
	struct Driver *currDriver;
	s32 distToFinish_prev;
	s32 minDistance;
	s32 bestLapIndex;
	s32 driverIndex;
	s32 finishedHumanCount;
	s32 firstUnfinishedRank;
	struct GameTracker *gGT = GAME_TRACKER;

	firstUnfinishedRank = 0;
	finishedHumanCount = 0;

	// Driver leading the previous race order, not a rank index.
	farthestHuman = NULL;

	// find farthest-ahead human
	for (driverIndex = 0; driverIndex < PLAYLEVEL_DRIVER_COUNT; driverIndex++)
	{
		struct Driver *orderedDriver;

		orderedDriver = gGT->driversInRaceOrder[driverIndex];

		if ((orderedDriver != 0) && ((orderedDriver->actionsFlagSet & ACTION_BOT) == 0))
		{
			farthestHuman = orderedDriver;
			break;
		}
	}

	for (driverIndex = 0; driverIndex < PLAYLEVEL_DRIVER_COUNT; driverIndex++)
	{
		s32 newRank;

		gGT->driversInRaceOrder[driverIndex] = NULL;

		currDriver = gGT->drivers[driverIndex];

		if (currDriver == NULL)
		{
			continue;
		}

		// before and after
		distToFinish_prev = currDriver->distanceToFinish_curr;
		VehLap_UpdateProgress(currDriver);

		// NOTE(aalhendi): Publish progress before clamping; lap checks reread the driver owner.
		currDriver->distanceDrivenBackwards += currDriver->distanceToFinish_curr - (u32)distToFinish_prev;

		if ((s32)currDriver->distanceDrivenBackwards < 0)
		{
			currDriver->distanceDrivenBackwards = 0;
		}
		else if ((s32)currDriver->distanceDrivenBackwards > PLAYLEVEL_REVERSE_DISTANCE_MAX)
		{
			currDriver->distanceDrivenBackwards = PLAYLEVEL_REVERSE_DISTANCE_MAX;
		}

		// === Natty Video ===
		// https://www.youtube.com/watch?v=lDaT2rY6GKI

		// Part A: Start-line -> 32000 distToFinish
		// Part B: 32000 distToFinish -> 1200 distToFinish
		// Part C: 1200 distToFinish -> Finish-line

		if (
		    // crossed finishline (forwards)
		    (distToFinish_prev < PLAYLEVEL_FINISHLINE_NEAR_DISTANCE) && (((s32)currDriver->distanceToFinish_curr) > PLAYLEVEL_FINISHLINE_FAR_DISTANCE))
		{
			// Set racer's distance driven backwards to zero
			currDriver->distanceDrivenBackwards = 0;

			if ((currDriver->actionsFlagSet & ACTION_BEHIND_START_LINE) != 0)
			{
				currDriver->actionsFlagSet &= ~ACTION_BEHIND_START_LINE;

				goto UpdateFinishedDriverRank;
			}

			// update checkpoint with distToFinish
			currDriver->distanceToFinish_checkpoint = currDriver->distanceToFinish_curr;

			if (gGT->numLaps >= (currDriver->lapIndex + 1))
			{
				if (
				    // If you're in Arcade, or
				    // If you're in Adventure, or
				    // If you're in Time Trial
				    ((gGT->gameMode1 & GAME_MODE_SAVE_LAP_TIME_MASK) != 0) &&

				    // player of any kind
				    (currDriver->instSelf->thread->modelIndex == DYNAMIC_PLAYER))
				{
					UI_SaveLapTime(currDriver->lapIndex, (s32)((u32)gGT->elapsedEventTime - (u32)currDriver->lapTime), currDriver->driverID);

					gGT->lapTime[currDriver->lapIndex] = (s32)((u32)gGT->elapsedEventTime - (u32)currDriver->lapTime);
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

				// If Final Lap
				if (currDriver->lapIndex == (GAME_TRACKER->numLaps - 1))
				{
					if ((currDriver->actionsFlagSet & ACTION_BOT) == 0)
					{
						// frames, so the animation lasts 3 seconds
						GAME_SDATA_NAME(finalLapTextTimer)[driverIndex] = PLAYLEVEL_FINAL_LAP_TEXT_FRAMES;
					}
				}
			}

			// If did not just finish race
			if (currDriver->lapIndex != gGT->numLaps)
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
					currDriver->driverRank = GAME_SDATA_NAME(numPlayersFinishedRace);
					GAME_SDATA_NAME(numPlayersFinishedRace)++;
				}

				// you have no weapon
				currDriver->heldItemID = HELD_ITEM_NONE;

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
						gGT->winnerIndex[PLAYLEVEL_FIRST_WINNER_INDEX] = currDriver->driverID;
						gGT->numWinners = PLAYLEVEL_SINGLE_WINNER_COUNT;

						// edit window variables for confetti
						gGT->pushBuffer[gGT->winnerIndex[PLAYLEVEL_FIRST_WINNER_INDEX]].fadeFromBlack_currentValue = PLAYLEVEL_WINNER_FADE_CURRENT;
						gGT->pushBuffer[gGT->winnerIndex[PLAYLEVEL_FIRST_WINNER_INDEX]].fadeFromBlack_desiredResult = PLAYLEVEL_WINNER_FADE_DESIRED;
						gGT->pushBuffer[gGT->winnerIndex[PLAYLEVEL_FIRST_WINNER_INDEX]].fade_step = PLAYLEVEL_WINNER_FADE_STEP;
					}
					if (currDriver->noItemTimer != 0)
					{
						currDriver->noItemTimer = 0;
						currDriver->heldItemID = HELD_ITEM_NONE;
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
			    (((s32)currDriver->distanceToFinish_curr) < PLAYLEVEL_FINISHLINE_NEAR_DISTANCE) && (distToFinish_prev > PLAYLEVEL_FINISHLINE_FAR_DISTANCE))
			{
				// automatic backwards penalty
				currDriver->distanceDrivenBackwards = PLAYLEVEL_REVERSE_CROSSING_PENALTY;
				currDriver->actionsFlagSet |= ACTION_BEHIND_START_LINE;
			}

			// if player did not JUST cross finish backwards
			else
			{
				if (
				    // if player did not EVER cross finish backwards
				    ((currDriver->actionsFlagSet & ACTION_BEHIND_START_LINE) == 0) &&

				    (
				        // if distance driven this frame is less than...
				        (currDriver->distanceToFinish_checkpoint - ((s32)currDriver->distanceToFinish_curr)) <=

				        // level's distance to finish
				        (u32)((gGT->level1->ptr_restart_points[0].distToFinish >> 2) << 3)))
				{
					// save distance for next frame
					currDriver->distanceToFinish_checkpoint = currDriver->distanceToFinish_curr;
				}
			}

		UpdateFinishedDriverRank:
			if ((currDriver->actionsFlagSet & ACTION_RACE_FINISHED) == 0)
			{
				// set rank to "unsorted"
				currDriver->driverRank = PLAYLEVEL_UNSORTED_RANK;

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
		newRank = currDriver->driverRank + 1;

		// get human in last
		if (firstUnfinishedRank < newRank)
		{
			firstUnfinishedRank = newRank;
		}
	}

	// sort all drivers that have NOT finished race
	for (driverIndex = firstUnfinishedRank; driverIndex < PLAYLEVEL_DRIVER_COUNT; driverIndex++)
	{
		s32 candidateIndex;

		// set "min" distance to max
		minDistance = PLAYLEVEL_DISTANCE_SENTINEL;

		// set "highest" lap to min
		bestDriverIndex = PLAYLEVEL_UNSORTED_RANK;

		// lap index
		bestLapIndex = PLAYLEVEL_LOWEST_LAP_SENTINEL;

		// look for "next" farthest driver,
		// out of all unsorted drivers remaining
		for (candidateIndex = 0; candidateIndex < PLAYLEVEL_DRIVER_COUNT; candidateIndex++)
		{
			struct Driver *candidateDriver;
			s32 effectiveLapIndex;

			// get current driver
			candidateDriver = gGT->drivers[candidateIndex];

			if (candidateDriver == NULL)
			{
				continue;
			}

			if (candidateDriver->driverRank != PLAYLEVEL_UNSORTED_RANK)
			{
				continue;
			}

			// NOTE(aalhendi): Being behind the start line subtracts one effective lap.
			effectiveLapIndex = candidateDriver->lapIndex;
			effectiveLapIndex -= (candidateDriver->actionsFlagSet & ACTION_BEHIND_START_LINE) != 0;

			if (
			    // new highest lap
			    (effectiveLapIndex > bestLapIndex) ||

			    // OR

			    (
			        // same lap
			        (effectiveLapIndex == bestLapIndex) &&

			        // AND

			        // new lowest distance (max progress)
			        ((s32)candidateDriver->distanceToFinish_curr < minDistance)))
			{
				// set new min distToFinish (max progress)
				minDistance = candidateDriver->distanceToFinish_curr;

				// index of driver closest to finish
				bestDriverIndex = candidateIndex;

				// highest lap
				bestLapIndex = effectiveLapIndex;
			}
		}

		if (bestDriverIndex != PLAYLEVEL_UNSORTED_RANK)
		{
			if (gGT->trafficLightsTimer > 0)
			{
				gGT->drivers[bestDriverIndex]->driverRank = GAME_SDATA_NAME(kartSpawnOrderArray)[bestDriverIndex];
				gGT->humanPlayerPositions[bestDriverIndex] = GAME_SDATA_NAME(kartSpawnOrderArray)[bestDriverIndex];
			}
			else
			{
				gGT->drivers[bestDriverIndex]->driverRank = driverIndex;
			}
		}
	}

	for (driverIndex = 0; driverIndex < PLAYLEVEL_DRIVER_COUNT; driverIndex++)
	{
		struct Driver *rankedDriver;

		// get pointer to each player structure
		rankedDriver = gGT->drivers[driverIndex];

		if (rankedDriver == NULL)
		{
			continue;
		}

		// should be impossible to be -1 here
		if (rankedDriver->driverRank > PLAYLEVEL_UNSORTED_RANK)
		{
			gGT->driversInRaceOrder[rankedDriver->driverRank] = rankedDriver;
		}
	}

	for (driverIndex = 0; driverIndex < GAME_TRACKER->numPlyrCurrGame; driverIndex++)
	{
		// pointer to each player structure
		currDriver = gGT->drivers[driverIndex];

		if (currDriver == NULL)
		{
			continue;
		}

		if ((PLAYLEVEL_UNSORTED_RANK < currDriver->driverRank) && (PLAYLEVEL_PASS_VOICELINE_DELAY < gGT->elapsedEventTime) &&
		    ((s8)gGT->humanPlayerPositions[driverIndex] < currDriver->driverRank))
		{
			s32 characterID = GAME_CHARACTER_IDS[GAME_TRACKER->driversInRaceOrder[currDriver->driverRank - 1]->driverID];

			// Make driver talk
			Voiceline_RequestPlay(PLAYLEVEL_PASS_VOICELINE, characterID, PLAYLEVEL_VOICELINE_FLAGS);
		}
		gGT->humanPlayerPositions[driverIndex] = currDriver->driverRank;
	}

	// If already finished race
	if ((gGT->gameMode1 & END_OF_RACE) != 0)
	{
		return;
	}


	// Check if race should end
	if ((
	        // 1P game, with 1 human finished
	        (gGT->numPlyrCurrGame == 1) && (finishedHumanCount > 0)

	            ) ||

	    (
	        // Multiplayer VS, all finished except one
	        (finishedHumanCount >= (gGT->numPlyrCurrGame - 1)) && (gGT->numPlyrCurrGame > 1) && ((gGT->gameMode1 & ARCADE_MODE) == 0)) ||

	    (
	        // Arcade mode, all humans finished
	        ((gGT->gameMode1 & ARCADE_MODE) != 0) && (gGT->numPlyrCurrGame <= finishedHumanCount)))
	{
		// End race for all drivers
		for (driverIndex = 0; driverIndex < PLAYLEVEL_DRIVER_COUNT; driverIndex++)
		{
			// Get address of each player structure
			currDriver = gGT->drivers[driverIndex];

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
