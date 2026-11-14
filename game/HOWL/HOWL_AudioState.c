#include <common.h>

void Audio_SetState(u32 state)
{
	u8 XA_type;
	u8 XA_index;

	struct GameTracker *gGT = sdata->gGT;
	u16 level;

	XA_type = CDSYS_XA_TYPE_MUSIC;

	switch (state)
	{
	// stop/pause cseq music (main: case 2)
	case AUDIO_LOADING:
		sdata->boolNeedXASeek = 0;

		CDSYS_XAPauseRequest();

		Music_Restart();

		// erase backup, keep music, stop all fx
		howl_StopAudio(1, 0, 1);
		break;
	case AUDIO_STOP_ALL:
	case AUDIO_GARAGE_ENTRY:

		CseqMusic_StopAll();

		Music_Adjust(0, 0, 0, 0);

		break;
	case AUDIO_ADV_HUB:

		CseqMusic_StopAll();

		level = gGT->levelID;

		// Level ID on Adventure Arena
		if (level - 0x19U < 5)
		{
			// convert levelID to a bitshifted flag
			Music_Adjust(0, 0, &sdata->advHubSongSet, 1 << (level - 0x19U));
		}
		break;
	case AUDIO_RACE_INTRO:
		XA_index = sdata->desiredXA_RaceIntroIndex;
		sdata->desiredXA_RaceIntroIndex += 1;
		sdata->desiredXA_RaceIntroIndex &= 3;
		goto PLAY_XA;

	case AUDIO_TRAFFIC:

		Music_Stop();

		CseqMusic_StopAll();
		break;
	case AUDIO_RACING:

		sdata->WrongWayDirection_bool = false;

		sdata->framesDrivingSameDirection = 0;

		Voiceline_ToggleEnable(1);
		break;

	case AUDIO_PRE_LAST_LAP:

	// last lap, distToFinish < 9000
	case AUDIO_LAST_LAP:

		Voiceline_ToggleEnable(0);

		sdata->boolNeedXASeek = 1;
		break;

	// if you are on last lap
	case AUDIO_FINAL_LAP:
		sdata->boolNeedXASeek = 0;

		Music_LowerVolume();

		// MUSIC_LAST_LAP
		XA_index = 6;

		goto PLAY_XA;
	case AUDIO_POST_LAST_LAP:

		sdata->WrongWayDirection_bool = false;

		sdata->framesDrivingSameDirection = 0;

		Music_RaiseVolume();

		Voiceline_ToggleEnable(1);

		break;
	case AUDIO_RACE_END:
		sdata->boolNeedXASeek = 0;

		Music_Restart();

		// set XA
		XA_index = sdata->desiredXA_RaceEndIndex;

		if (61 < XA_index)
		{
			XA_type = CDSYS_XA_TYPE_EXTRA;
		}

	PLAY_XA:

		CDSYS_XAPlay(XA_type, XA_index);
	}
}

void Audio_SetState_Safe(int state)
{
	// If this sound isn't already playing
	if (state != sdata->audioState)
	{
		Voiceline_EmptyFunc();

		Audio_SetState(state);

		// set which sound is playing
		sdata->audioState = state;
	}
}

void Audio_AdvHub_SwapSong(int levelID)
{
	if ((sdata->audioState == AUDIO_ADV_HUB) &&
	    // If you're on a map in the Adventure Arena
	    (levelID - GEM_STONE_VALLEY < 5))
	{
		CseqMusic_AdvHubSwap(0, &sdata->advHubSongSet, (1 << (levelID - GEM_STONE_VALLEY)));
	}
}

void Audio_SetMaskSong(u32 tempo)
{
	s32 i;
	u8 isMaskUsed;
	u32 songID;
	struct GameTracker *gGT = sdata->gGT;
	u32 gameMode = gGT->gameMode1;

	// Assume no player is using a mask
	isMaskUsed = false;

	for (i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		if ((gGT->drivers[i]->actionsFlagSet & ACTION_MASK_WEAPON) != 0)
		{
			// There is at least one player using a mask
			isMaskUsed = true;
		}
	}

	// If any player is using a mask
	if (isMaskUsed)
	{
		// Uka song is playing
		songID = 2;

		if (
		    // If Uka song is playing
		    ((gameMode & UKA_SONG) != 0) ||

		    (
		        // Aku song is playing
		        songID = 1,

		        // If Aku song is playing
		        (gameMode & AKU_SONG) != 0))
		{
			Music_Adjust(songID, tempo, 0, 0);
		}
	}

	// If no players are using mask
	else
	{
		Music_Adjust(0, tempo, 0, 0);

		if ((gameMode & (UKA_SONG | AKU_SONG)) != 0)
		{
			gGT->gameMode1 &= ~(UKA_SONG | AKU_SONG);
		}
	}
}

// Retail starts the end-of-race sequence from AUDIO_LAST_LAP, a state it
// only reaches while the human driver is on the final lap next to the finish
// line. Ending the race on request can leave that driver still out on track,
// so pick the same sequence up from whichever racing state is active instead
// of waiting for transitions that will never happen.
static void Audio_ResumeRaceEndSequence(struct GameTracker *gGT)
{
	struct Driver *human = NULL;

	// Only an event that may auto-end can have been ended this way
	if (!PlayLevel_CanAutoEndRace(gGT))
	{
		return;
	}

	if ((gGT->gameMode1 & END_OF_RACE) == 0)
	{
		return;
	}

	switch (sdata->audioState)
	{
	case AUDIO_RACING:
	case AUDIO_PRE_LAST_LAP:
	case AUDIO_FINAL_LAP:
	case AUDIO_POST_LAST_LAP:
		break;

	// AUDIO_LAST_LAP finishes the sequence on its own, AUDIO_RACE_END has
	// already started it
	default:
		return;
	}

	int driverSlotCount = gGT->numPlyrNextGame + gGT->numBotsNextGame;

	for (int driverIndex = 0; driverIndex < driverSlotCount; driverIndex++)
	{
		struct Driver *candidate = gGT->driversInRaceOrder[driverIndex];

		if ((candidate != NULL) && (candidate->instSelf->thread->modelIndex == DYNAMIC_PLAYER))
		{
			human = candidate;
			break;
		}
	}

	// Nothing to resume until the race is over for that driver
	if ((human == NULL) || ((human->actionsFlagSet & ACTION_RACE_FINISHED) == 0))
	{
		return;
	}

	// AUDIO_FINAL_LAP lowers the level song, and only its AUDIO_POST_LAST_LAP
	// step raises it back
	bool restoreVolume = (sdata->audioState == AUDIO_FINAL_LAP);

	Audio_SetState_Safe(AUDIO_LAST_LAP);

	// The race is already over, so skip retail's seek to the last-lap XA
	sdata->boolNeedXASeek = 0;

	if (restoreVolume)
	{
		Music_RaiseVolume();
	}
}

void Audio_Update1(void)
{
	s32 i;
	int raceOrderIndex;
	s16 uVar1;
	b32 humanWon;
	b32 tropyBeatenOnAllTracks;
	struct Driver *d = 0;
	u32 maskTempo;
	int iVar7;
	struct GameTracker *gGT = sdata->gGT;

	Audio_ResumeRaceEndSequence(gGT);

	switch (sdata->audioState - 1)
	{
	case AUDIO_ADV_HUB_WAIT - 1:
		if ((sdata->XA_State == XA_IDLE) || (sdata->XA_Playing_Category != 0))
		{
			Audio_SetState_Safe(AUDIO_ADV_HUB);
		}
		break;
	case AUDIO_GARAGE - 1:
		Garage_LerpFX();
		break;
	case AUDIO_RACE_INTRO - 1:
		if (sdata->XA_State == XA_IDLE)
		{
			// 9 means intro cutscene
			// 10 means traffic lights
			// 11 means racing

			// Change state to traffic lights
			Audio_SetState_Safe(AUDIO_TRAFFIC);
		}
		break;
	case AUDIO_TRAFFIC - 1:
		// If traffic lights finish counting down
		// from 0x3840 to zero
		if (gGT->trafficLightsTimer < 1)
		{
			// 9 means intro cutscene
			// 10 means traffic lights
			// 11 means racing

			// Change State to 11, which means racing
			Audio_SetState_Safe(AUDIO_RACING);
		}
		break;
	case AUDIO_RACING - 1:
		Audio_SetMaskSong(0);

		// human driver in the lead
		for (i = 0; i < 8; i++)
		{
			d = gGT->driversInRaceOrder[i];

			if ((d != NULL) && ((d->actionsFlagSet & ACTION_BOT) == 0))
			{
				break;
			}
			d = 0;
		}

		Voiceline_Update();

		Level_AmbientSound();

		// Demo mode converts P1 to ACTION_BOT, leaving retail's human-driver
		// scan empty. PS1 can read low RAM through a null pointer; host cannot.
		if (d == NULL)
		{
			break;
		}

		// if race has more than 2 laps
		if ((2 < sdata->gGT->numLaps) &&
		    // if you are on 2nd to last lap
		    ((d->lapIndex == sdata->gGT->numLaps - 2U)) &&
		    // distToFinish is small
		    (d->distanceToFinish_curr < 9000))
		{
			Audio_SetState_Safe(AUDIO_PRE_LAST_LAP);
		}
		break;
	case AUDIO_PRE_LAST_LAP - 1:
		Audio_SetMaskSong(0);

		// human driver in the lead
		for (i = 0; i < 8; i++)
		{
			d = gGT->driversInRaceOrder[i];

			if ((d != NULL) && ((d->actionsFlagSet & ACTION_BOT) == 0))
			{
				break;
			}
			d = 0;
		}

		if (d == NULL)
		{
			if (((sdata->boolNeedXASeek != 0) && (sdata->XA_State == XA_IDLE)) && (9 < gGT->frameTimer_MainFrame_ResetDB - sdata->XA_PauseFrame))
			{
				sdata->boolNeedXASeek = 0;
			}

			Level_AmbientSound();
			break;
		}

		// if need to XASeek
		if (((sdata->boolNeedXASeek != 0) && (sdata->XA_State == XA_IDLE)) && (9 < gGT->frameTimer_MainFrame_ResetDB - sdata->XA_PauseFrame))
		{
			// far from finish line
			if (2000 < d->distanceToFinish_curr)
			{
				// (FINAL LAP! music)
				CDSYS_XASeek(1, 0, 6);
			}

			// dont need to XASeek
			sdata->boolNeedXASeek = 0;
		}

		Level_AmbientSound();

		// if driver is on final lap
		if (d->lapIndex == gGT->numLaps - 1U)
		{
			// Play final lap sound
			Audio_SetState_Safe(AUDIO_FINAL_LAP);
		}
		break;
	case AUDIO_FINAL_LAP - 1:
		maskTempo = 0;

		// if XA has been playing more than a second
		if (0xe1 < sdata->XA_CurrOffset)
		{
			Music_RaiseVolume();

			maskTempo = 20;
		}

		Audio_SetMaskSong(maskTempo);

		Level_AmbientSound();

		if (sdata->XA_State == XA_IDLE)
		{
			Audio_SetState_Safe(AUDIO_POST_LAST_LAP);
		}
		break;
	case AUDIO_POST_LAST_LAP - 1:
		Audio_SetMaskSong(20);

		// human driver in the lead
		for (i = 0; i < 8; i++)
		{
			d = gGT->driversInRaceOrder[i];

			if ((d != NULL) && ((d->actionsFlagSet & ACTION_BOT) == 0))
			{
				break;
			}
			d = 0;
		}

		Voiceline_Update();

		Level_AmbientSound();

		if (d == NULL)
		{
			break;
		}

		if (
		    // if driver's lap is the last lap
		    (d->lapIndex == gGT->numLaps - 1U) &&

		    // if finish line is close
		    (d->distanceToFinish_curr < 9000))
		{
			Audio_SetState_Safe(AUDIO_LAST_LAP);
		}

		break;
	case AUDIO_LAST_LAP - 1:
		raceOrderIndex = -1;

		// human driver in the lead
		for (i = 0; i < 8; i++)
		{
			d = gGT->driversInRaceOrder[i];

			if ((d != NULL) && (d->instSelf->thread->modelIndex == DYNAMIC_PLAYER))
			{
				raceOrderIndex = i;
				break;
			}
			d = 0;
		}

		if (d == NULL)
		{
			if (((sdata->boolNeedXASeek != 0) && (sdata->XA_State == XA_IDLE)) && (9 < gGT->frameTimer_MainFrame_ResetDB - sdata->XA_PauseFrame))
			{
				sdata->boolNeedXASeek = 0;
			}

			Level_AmbientSound();
			break;
		}

		// if need to XASeek
		if (((sdata->boolNeedXASeek != 0) && (sdata->XA_State == XA_IDLE)) && (9 < gGT->frameTimer_MainFrame_ResetDB - sdata->XA_PauseFrame))
		{
			// far from finish line
			if (2000 < d->distanceToFinish_curr)
			{
				CDSYS_XASeek(1, 0, 4);
			}

			// dont need to XASeek
			sdata->boolNeedXASeek = 0;
		}

		Level_AmbientSound();

		// Adventure co-op holds the round open until the second racer is across
		// the line, so the race-end music waits for that instead of firing on the
		// first arrival. The state is re-evaluated every frame, so this only holds
		// the music back while the partner is still on track.
		if (MainGameEnd_IsCoopRace(gGT) && !MainGameEnd_AllHumansFinished(gGT))
		{
			break;
		}

		if ((d->actionsFlagSet & ACTION_RACE_FINISHED) != 0)
		{
			// if did not just open N Tropy
			if ((gGT->gameModeEnd & NTROPY_JUST_OPENED) == 0)
			{
				// If you did not just beat N Tropy in Time Trial
				if ((gGT->gameModeEnd & NTROPY_JUST_BEAT) == 0)
				{
					// defeat music
					uVar1 = 5;

					// Co-op races grade both racers; other races start with the
					// retail "human in front won" test, then apply event rules.
					humanWon = MainGameEnd_IsCoopRace(gGT) ? MainGameEnd_AdventureRaceWon(gGT) : (raceOrderIndex == 0);
					if ((gGT->gameMode1 & RELIC_RACE) != 0)
					{
						humanWon = (gGT->gameModeEnd & (NEW_RELIC | NEW_HIGH_SCORE)) != 0;
					}
					else if ((gGT->gameMode2 & TOKEN_RACE) != 0)
					{
						humanWon = humanWon && MainGameEnd_AllCTRLettersCollected(gGT);
					}

					if (humanWon || ((gGT->gameMode1 & ADVENTURE_CUP) != 0) || ((gGT->gameMode2 & CUP_ANY_KIND) != 0))
					{
						OtherFX_Play(0x5f, 0);

						// victory (fanfare) music
						uVar1 = 4;
					}
				}

				// If you just beat N Tropy in Time Trial
				else
				{
					// Check if N Tropy has been beaten on all tracks
					tropyBeatenOnAllTracks = GAMEPROG_CheckGhostsBeaten(1);

					// If there is a track where N Tropy has not been beaten
					if (!tropyBeatenOnAllTracks)
					{
						// OtherFX_Play
						OtherFX_Play(0x5f, 0);

						// count how many times N Tropy has been beaten
						// during this playthrough
						iVar7 = sdata->nTropyVoiceCount;
						sdata->nTropyVoiceCount = iVar7 + 1;

						// pick an N Tropy XA voiceline
						uVar1 = data.nTropyXA[iVar7];

						// if beaten more than 5 times
						if (5 < sdata->nTropyVoiceCount)
						{
							// go back to 0
							sdata->nTropyVoiceCount = 0;
						}
					}

					// If N Tropy has been beaten on all tracks
					else
					{
						OtherFX_Play(0x5f, 0);

						// N Tropy Unlocked XA
						// "you've earned the right to choose me in character selection"
						uVar1 = 0x43;
					}
				}
			}

			// if n tropy just opened
			else
			{
				// OtherFX_Play
				OtherFX_Play(0x5f, 0);

				// N Tropy opened
				// "Think your fast eh, well, lets see if you can beat my fastest time"
				uVar1 = 0x46;
			}

			// desired XA
			sdata->desiredXA_RaceEndIndex = uVar1;

			Audio_SetState_Safe(AUDIO_RACE_END);
		}
		break;
	case AUDIO_RACE_END - 1:

		Level_AmbientSound();

		if (sdata->XA_State == XA_IDLE)
		{
			Audio_SetMaskSong(0);
		}
	}
}

void Audio_SetDefaults(void)
{
	sdata->audioDefaults[0] = 0;
	sdata->audioDefaults[1] = 0;
	sdata->audioDefaults[2] = 0;
	sdata->audioDefaults[3] = 0;

	sdata->audioDefaults[4] = 0x80;
	sdata->audioDefaults[5] = 0x80;
	sdata->audioDefaults[6] = 0;
	sdata->audioDefaults[7] = 0;
	sdata->audioDefaults[8] = 0;

	// erase both SoundFadeInput
	sdata->SoundFadeInput[0].unk = 0;
	sdata->SoundFadeInput[0].desiredVolume = 0;
	sdata->SoundFadeInput[0].currentVolume = 0;
	sdata->SoundFadeInput[0].soundID_soundCount = 0;
	sdata->SoundFadeInput[1].unk = 0;
	sdata->SoundFadeInput[1].desiredVolume = 0;
	sdata->SoundFadeInput[1].currentVolume = 0;
	sdata->SoundFadeInput[1].soundID_soundCount = 0;
}

void Audio_SetReverbMode(int levelID, u32 isBossRace, int bossID)
{
	u32 reverb;
	// if audio is enabled
	if (sdata->boolAudioEnabled != false)
	{
		// If this is not a boss race
		if (isBossRace == 0)
		{
			// Level ID < 30
			// If Level ID is any level you can drive on,
			// including adventure maps
			if (levelID < INTRO_RACE_TODAY)
			{
				// get reverb based on level ID
				reverb = data.reverbMode[levelID];
			}

			// If this is not a level you can drive on:
			// menu, cutscene, etc
			else
			{
				reverb = 4;
			}
		}

		// If this is a boss race
		else
		{
			// if invalid bossID
			if (5 < bossID)
			{
				// quit
				return;
			}

			// get reverb based on boss
			reverb = sdata->reverbModeBossID[bossID];
		}

		SetReverbMode(reverb);
	}
}
