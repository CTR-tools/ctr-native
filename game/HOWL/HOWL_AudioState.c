#include <common.h>

#ifndef GAME_AUDIO_STATE
#define GAME_AUDIO_STATE        (sdata->audioState)
#define GAME_ADV_HUB_SONG_SET   (&sdata->advHubSongSet)
#define GAME_REVERB_BOSS        (sdata->reverbModeBossID)
#define GAME_REVERB_LEVEL       (data.reverbMode)
#define GAME_SOUND_FADE_0       (sdata->SoundFadeInput[0])
#define GAME_SOUND_FADE_1       (sdata->SoundFadeInput[1])
#define GAME_AUDIO_DEFAULT_0    (sdata->audioDefaults[0])
#define GAME_AUDIO_DEFAULT_1    (sdata->audioDefaults[1])
#define GAME_AUDIO_DEFAULT_2    (sdata->audioDefaults[2])
#define GAME_AUDIO_DEFAULT_3    (sdata->audioDefaults[3])
#define GAME_AUDIO_DEFAULT_4    (sdata->audioDefaults[4])
#define GAME_AUDIO_DEFAULT_5    (sdata->audioDefaults[5])
#define GAME_AUDIO_DEFAULT_6    (sdata->audioDefaults[6])
#define GAME_AUDIO_DEFAULT_7    (*(u8 *)&sdata->audioDefaults[7])
#define GAME_AUDIO_DEFAULT_8    (sdata->audioDefaults[8])
#define GAME_XA_CATEGORY        (sdata->XA_Playing_Category)
#define GAME_XA_PAUSE_FRAME     (sdata->XA_PauseFrame)
#define GAME_XA_OFFSET          (sdata->XA_CurrOffset)
#define GAME_NTROPY_XA          (data.nTropyXA)
#define GAME_NTROPY_VOICE_COUNT (sdata->nTropyVoiceCount)
#endif

void Audio_SetState(u32 state)
{
	s32 xaType;
	s32 xaIndex;
	switch (state)
	{
	case AUDIO_LOADING:
		// NOTE(aalhendi): Keep the seek flag store before the pause callback.
		sdata->boolNeedXASeek = 0;
		CTR_PSX_MEMORY_BARRIER();
		CDSYS_XAPauseRequest();
		Music_Restart();
		howl_StopAudio(1, 0, 1);
		break;
	case AUDIO_ADV_HUB:
		CseqMusic_StopAll();
		{
			u32 hubIndex = (u32)GAME_TRACKER->levelID - GEM_STONE_VALLEY;
			if (hubIndex < 5)
			{
				Music_Adjust(0, 0, GAME_ADV_HUB_SONG_SET, 1u << hubIndex);
			}
		}
		break;
	case AUDIO_STOP_ALL:
	case AUDIO_GARAGE_ENTRY:
		CseqMusic_StopAll();
		Music_Adjust(0, 0, 0, 0);
		break;
	case AUDIO_RACE_INTRO:
		CDSYS_XAPlay(CDSYS_XA_TYPE_MUSIC, sdata->desiredXA_RaceIntroIndex);
		sdata->desiredXA_RaceIntroIndex = (sdata->desiredXA_RaceIntroIndex + 1) % 4;
		// NOTE(aalhendi): Keep the halfword store before the shared exit jump.
		CTR_PSX_MEMORY_BARRIER();
		break;
	case AUDIO_TRAFFIC:
		Music_Stop();
		CseqMusic_StopAll();
		break;
	case AUDIO_RACING:
		GAME_WRONG_WAY_DIRECTION = 0;
		GAME_SAME_DIRECTION_FRAMES = 0;
		Voiceline_ToggleEnable(1);
		break;
	case AUDIO_FINAL_LAP:
		// NOTE(aalhendi): Keep the seek flag store before lowering the volume.
		sdata->boolNeedXASeek = 0;
		CTR_PSX_MEMORY_BARRIER();
		Music_LowerVolume();
		xaType = CDSYS_XA_TYPE_MUSIC;
		xaIndex = 6;
		goto playXA;
	case AUDIO_POST_LAST_LAP:
		GAME_WRONG_WAY_DIRECTION = 0;
		GAME_SAME_DIRECTION_FRAMES = 0;
		Music_RaiseVolume();
		Voiceline_ToggleEnable(1);
		break;
	case AUDIO_PRE_LAST_LAP:
	case AUDIO_LAST_LAP:
		Voiceline_ToggleEnable(0);
		sdata->boolNeedXASeek = 1;
		// NOTE(aalhendi): Keep the flag store before the shared exit jump.
		CTR_PSX_MEMORY_BARRIER();
		break;
	case AUDIO_RACE_END:
		// NOTE(aalhendi): Keep the seek flag store before restarting the music.
		sdata->boolNeedXASeek = 0;
		CTR_PSX_MEMORY_BARRIER();
		Music_Restart();
		xaIndex = sdata->desiredXA_RaceEndIndex;
		xaType = CDSYS_XA_TYPE_MUSIC;
		if (xaIndex >= 62)
		{
			xaType = CDSYS_XA_TYPE_EXTRA;
			// NOTE(aalhendi): Retain the category branch instead of XOR lowering.
			CTR_PSX_KEEP_VALUE_RELAXED(xaType);
		}
	playXA:
		CDSYS_XAPlay(xaType, xaIndex);
	}
}

void Audio_SetState_Safe(int state)
{
	s32 savedState = state;
	// NOTE(aalhendi): Load the old state in a0 while preserving the input across calls.
	register s32 currentState CTR_PSX_REGISTER("$4");
	currentState = GAME_AUDIO_STATE;
	if (savedState != currentState)
	{
		Voiceline_EmptyFunc();
		Audio_SetState(savedState);
		GAME_AUDIO_STATE = savedState;
	}
}

void Audio_AdvHub_SwapSong(int levelID)
{
	if ((GAME_AUDIO_STATE == AUDIO_ADV_HUB) && ((u32)levelID - GEM_STONE_VALLEY < 5))
	{
		CseqMusic_AdvHubSwap(0, GAME_ADV_HUB_SONG_SET, 1 << ((u32)levelID - GEM_STONE_VALLEY));
	}
}

void Audio_SetMaskSong(u32 tempo)
{
	s32 i;
	s32 hasMask;
	s32 isMaskUsed = 0;
	struct GameTracker *entryGT = GAME_TRACKER;
	// NOTE(aalhendi): Keep tracker capture and accumulator initialization ahead of the scan.
	CTR_PSX_MEMORY_BARRIER();
	CTR_PSX_KEEP_VALUE_RELAXED(isMaskUsed);
	for (i = 0; i < entryGT->numPlyrCurrGame; i++)
	{
		if ((entryGT->drivers[i]->actionsFlagSet & ACTION_MASK_WEAPON) != 0)
		{
			isMaskUsed = 1;
		}
	}
	hasMask = isMaskUsed;
	// NOTE(aalhendi): Keep the scan result as a separate value for the song branch.
	CTR_PSX_KEEP_VALUE_RELAXED(hasMask);
	if (hasMask)
	{
		u32 gameMode;
		u32 songID;
		gameMode = GAME_TRACKER->gameMode1;
		songID = 2;
		if (((gameMode & UKA_SONG) != 0) || (songID = 1, (gameMode & AKU_SONG) != 0))
		{
			Music_Adjust(songID, tempo, 0, 0);
		}
	}
	else
	{
		struct GameTracker *gGT;
		u32 gameMode;

		// NOTE(aalhendi): Keep the completed scan ahead of the no-mask callback.
		CTR_PSX_MEMORY_BARRIER();
		Music_Adjust(0, tempo, 0, 0);
		gGT = GAME_TRACKER_RELOAD();
		gameMode = gGT->gameMode1;
		if ((gameMode & UKA_SONG) != 0)
		{
			gGT->gameMode1 = gameMode & ~UKA_SONG;
			// NOTE(aalhendi): Retail reloads tracker and mode between the separate flag clears.
			gGT = GAME_TRACKER_RELOAD();
			gameMode = gGT->gameMode1;
		}
		if ((gameMode & AKU_SONG) != 0)
		{
			gGT->gameMode1 = gameMode & ~AKU_SONG;
		}
	}
}

void Audio_Update1(void)
{
	s32 i;
	s32 raceOrderIndex;
	s16 raceEndXA;
	b32 tropyBeatenOnAllTracks;
	struct Driver *d = 0;
	u32 maskTempo;
	s32 voiceLineIndex;
	s32 lapCount;
	struct GameTracker *gGT;
	struct Driver *candidate;
#if defined(CTR_NATIVE)
	struct GameTracker *entryGT = GAME_TRACKER;
#endif

	switch ((s16)((u16)GAME_AUDIO_STATE - 1))
	{
	// NOTE(aalhendi): These idle cases retain the retail state interval.
	case AUDIO_LOADING - 1:
	case AUDIO_STOP_ALL - 1:
	case 2:
	case 3:
	case AUDIO_ADV_HUB - 1:
		break;
	case AUDIO_ADV_HUB_WAIT - 1:
		if ((GAME_XA_STATE == XA_IDLE) || (GAME_XA_CATEGORY != 0))
		{
			Audio_SetState_Safe(AUDIO_ADV_HUB);
		}
		break;
	case AUDIO_GARAGE - 1:
		Garage_LerpFX();
		break;
	case AUDIO_RACE_INTRO - 1:
		if (GAME_XA_STATE == XA_IDLE)
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
		if (GAME_TRACKER->trafficLightsTimer < 1)
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
		gGT = GAME_TRACKER;
		for (i = 0; i < 8; i++)
		{
			candidate = gGT->driversInRaceOrder[i];

			if ((candidate != NULL) && ((candidate->actionsFlagSet & ACTION_BOT) == 0))
			{
				d = candidate;
				break;
			}
		}

		Voiceline_Update();

		Level_AmbientSound();

#if defined(CTR_NATIVE)
		// NOTE(aalhendi): Demo mode converts P1 to ACTION_BOT, leaving retail's human-driver
		// scan empty. PS1 can read low RAM through a null pointer; host cannot.
		if (d == NULL)
		{
			break;
		}
#endif

		lapCount = GAME_TRACKER->numLaps;

		// if race has more than two laps
		if ((2 < lapCount) &&
		    // if you are on 2nd to last lap
		    ((d->lapIndex == lapCount - 2)) &&
		    // distToFinish is small
		    ((s32)d->distanceToFinish_curr < 9000))
		{
			Audio_SetState_Safe(AUDIO_PRE_LAST_LAP);
		}
		break;
	case AUDIO_PRE_LAST_LAP - 1:
		Audio_SetMaskSong(0);

		// human driver in the lead
		gGT = GAME_TRACKER;
		for (i = 0; i < 8; i++)
		{
			candidate = gGT->driversInRaceOrder[i];

			if ((candidate != NULL) && ((candidate->actionsFlagSet & ACTION_BOT) == 0))
			{
				d = candidate;
				break;
			}
		}

#if defined(CTR_NATIVE)
		// NOTE(aalhendi): The host cannot read the retail low-RAM null driver.
		if (d == NULL)
		{
			gGT = entryGT;
			if (((sdata->boolNeedXASeek != 0) && (GAME_XA_STATE == XA_IDLE)) && (9U < (u32)gGT->frameTimer_MainFrame_ResetDB - (u32)GAME_XA_PAUSE_FRAME))
			{
				sdata->boolNeedXASeek = 0;
			}

			Level_AmbientSound();
			break;
		}
#endif

		// if need to XASeek
		if (((sdata->boolNeedXASeek != 0) && (GAME_XA_STATE == XA_IDLE)) && (9U < (u32)GAME_TRACKER->frameTimer_MainFrame_ResetDB - (u32)GAME_XA_PAUSE_FRAME))
		{
			// far from finish line
			if (2000 < (s32)d->distanceToFinish_curr)
			{
				// (FINAL LAP! music)
				CDSYS_XASeek(1, 0, 6);
			}

			// dont need to XASeek
			sdata->boolNeedXASeek = 0;
		}

		Level_AmbientSound();

		// if driver is on final lap
		if (d->lapIndex == GAME_TRACKER->numLaps - 1)
		{
			// Play final lap sound
			Audio_SetState_Safe(AUDIO_FINAL_LAP);
		}
		break;
	case AUDIO_FINAL_LAP - 1:
		maskTempo = 0;

		// if XA has been playing more than a second
		if (0xe1 < GAME_XA_OFFSET)
		{
			Music_RaiseVolume();

			maskTempo = 20;
		}

		Audio_SetMaskSong(maskTempo);

		Level_AmbientSound();

		if (GAME_XA_STATE == XA_IDLE)
		{
			Audio_SetState_Safe(AUDIO_POST_LAST_LAP);
		}
		break;
	case AUDIO_POST_LAST_LAP - 1:
		Audio_SetMaskSong(20);

		// human driver in the lead
		gGT = GAME_TRACKER;
		for (i = 0; i < 8; i++)
		{
			candidate = gGT->driversInRaceOrder[i];

			if ((candidate != NULL) && ((candidate->actionsFlagSet & ACTION_BOT) == 0))
			{
				d = candidate;
				break;
			}
		}

		Voiceline_Update();

		Level_AmbientSound();

#if defined(CTR_NATIVE)
		// NOTE(aalhendi): The host cannot read the retail low-RAM null driver.
		if (d == NULL)
		{
			break;
		}
#endif

		if (
		    // if driver's lap is the last lap
		    (d->lapIndex == GAME_TRACKER->numLaps - 1) &&

		    // if finish line is close
		    ((s32)d->distanceToFinish_curr < 9000))
		{
			Audio_SetState_Safe(AUDIO_LAST_LAP);
		}

		break;
	case AUDIO_LAST_LAP - 1:
		raceOrderIndex = -1;

		// human driver in the lead
		gGT = GAME_TRACKER;
		for (i = 0; i < 8; i++)
		{
			candidate = gGT->driversInRaceOrder[i];

			if ((candidate != NULL) && (candidate->instSelf->thread->modelIndex == DYNAMIC_PLAYER))
			{
				raceOrderIndex = i;
				d = candidate;
				break;
			}
		}

#if defined(CTR_NATIVE)
		// NOTE(aalhendi): The host cannot read the retail low-RAM null driver.
		if (d == NULL)
		{
			gGT = entryGT;
			if (((sdata->boolNeedXASeek != 0) && (GAME_XA_STATE == XA_IDLE)) && (9U < (u32)gGT->frameTimer_MainFrame_ResetDB - (u32)GAME_XA_PAUSE_FRAME))
			{
				sdata->boolNeedXASeek = 0;
			}

			Level_AmbientSound();
			break;
		}
#endif

		// if need to XASeek
		if (((sdata->boolNeedXASeek != 0) && (GAME_XA_STATE == XA_IDLE)) && (9U < (u32)GAME_TRACKER->frameTimer_MainFrame_ResetDB - (u32)GAME_XA_PAUSE_FRAME))
		{
			// far from finish line
			if (2000 < (s32)d->distanceToFinish_curr)
			{
				CDSYS_XASeek(1, 0, 4);
			}

			// dont need to XASeek
			sdata->boolNeedXASeek = 0;
		}

		Level_AmbientSound();

		if ((d->actionsFlagSet & ACTION_RACE_FINISHED) != 0)
		{
			struct GameTracker *endGT = GAME_TRACKER;

			if ((endGT->gameModeEnd & NTROPY_JUST_OPENED) != 0)
			{
				OtherFX_Play(0x5f, 0);
				raceEndXA = 0x46;
			}
			else if ((endGT->gameModeEnd & NTROPY_JUST_BEAT) != 0)
			{
				tropyBeatenOnAllTracks = (s16)GAMEPROG_CheckGhostsBeaten(1);
				if (tropyBeatenOnAllTracks)
				{
					OtherFX_Play(0x5f, 0);
					raceEndXA = 0x43;
				}
				else
				{
					s16 selectedXA;
					b32 voiceCountInRange;
					OtherFX_Play(0x5f, 0);
					voiceLineIndex = GAME_NTROPY_VOICE_COUNT;
					selectedXA = GAME_NTROPY_XA[voiceLineIndex];
					GAME_NTROPY_VOICE_COUNT += 1;
					voiceCountInRange = GAME_NTROPY_VOICE_COUNT < 6;
					sdata->desiredXA_RaceEndIndex = selectedXA;
					// NOTE(aalhendi): Retail stores the line before branching to
					// reset the rotating counter; keep that halfword observable.
					CTR_PSX_OBSERVE_MEMORY(sdata->desiredXA_RaceEndIndex);
					if (!voiceCountInRange)
					{
						GAME_NTROPY_VOICE_COUNT = 0;
					}
					goto FINISHED;
				}
			}
			else
			{
				if ((raceOrderIndex == 0) || ((endGT->gameMode1 & ADVENTURE_CUP) != 0) || ((endGT->gameMode2 & CUP_ANY_KIND) != 0))
				{
					OtherFX_Play(0x5f, 0);
					raceEndXA = 4;
				}
				else
				{
					raceEndXA = 5;
				}
			}
			sdata->desiredXA_RaceEndIndex = raceEndXA;
		FINISHED:

			Audio_SetState_Safe(AUDIO_RACE_END);
		}
		break;
	case AUDIO_RACE_END - 1:

		Level_AmbientSound();

		if (GAME_XA_STATE == XA_IDLE)
		{
			Audio_SetMaskSong(0);
		}
	}
}

void Audio_SetDefaults(void)
{
	GAME_SOUND_FADE_0.unk = 0;
	GAME_SOUND_FADE_0.desiredVolume = 0;
	GAME_SOUND_FADE_0.currentVolume = 0;
	GAME_SOUND_FADE_0.soundID_soundCount = 0;
	GAME_SOUND_FADE_1.unk = 0;
	GAME_SOUND_FADE_1.desiredVolume = 0;
	GAME_SOUND_FADE_1.currentVolume = 0;
	GAME_SOUND_FADE_1.soundID_soundCount = 0;
	GAME_AUDIO_DEFAULT_6 = 0;
	GAME_AUDIO_DEFAULT_0 = 0;
	GAME_AUDIO_DEFAULT_1 = 0;
	GAME_AUDIO_DEFAULT_2 = 0;
	GAME_AUDIO_DEFAULT_3 = 0;
	GAME_AUDIO_DEFAULT_4 = 0x80;
	GAME_AUDIO_DEFAULT_5 = 0x80;
	GAME_AUDIO_DEFAULT_8 = 0;
	// NOTE(aalhendi): Retail clears only the first byte of default slot 7.
	GAME_AUDIO_DEFAULT_7 = 0;
}

void Audio_SetReverbMode(int levelID, u32 isBossRace, int bossID)
{
	u32 reverb;
	if (GAME_AUDIO_ENABLED != 0)
	{
		if (isBossRace != 0)
		{
			if (bossID > 5)
			{
				return;
			}
			// NOTE(aalhendi): Keep the table page load out of the invalid-boss exit delay slot.
			CTR_PSX_MEMORY_BARRIER();
			reverb = (u8)GAME_REVERB_BOSS[bossID];
		}
		else
		{
			if (levelID < INTRO_RACE_TODAY)
			{
				reverb = (u8)GAME_REVERB_LEVEL[levelID];
			}
			else
			{
				reverb = 4;
			}
		}
		SetReverbMode(reverb);
	}
}
