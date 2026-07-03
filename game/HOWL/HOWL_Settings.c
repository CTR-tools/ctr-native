#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002ac94-0x8002acb8
int howl_Disable(void)
{
	if (sdata->boolAudioEnabled == 0)
	{
		return 0;
	}

	sdata->boolAudioEnabled = 0;
	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002acb8-0x8002ad04
void UpdateChannelVol_EngineFX(struct EngineFX *engineFX, struct ChannelAttr *attr, int vol, int LR)
{
	Channel_SetVolume(attr, (sdata->vol_FX * engineFX->volume * vol) >> 10, LR);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002ad04-0x8002ad70
void UpdateChannelVol_OtherFX(struct OtherFX *otherFX, struct ChannelAttr *attr, int vol, int LR)
{
	int otherVol;

	otherVol = sdata->vol_FX;

	if ((otherFX->flags & 4) != 0)
	{
		otherVol = sdata->vol_Voice;
	}

	Channel_SetVolume(attr, (otherVol * otherFX->volume * vol) >> 10, LR);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002ad70-0x8002ae64
void UpdateChannelVol_Music(struct SongSeq *songSeq, struct ChannelAttr *attr, int index, int vol)
{
	int sampleVol;

	int newVol = (sdata->vol_Music * sdata->songPool[songSeq->songPoolIndex].vol_Curr * songSeq->vol_Curr) >> 10;

	if ((songSeq->flags & 4) == 0)
	{
		sampleVol = sdata->ptrCseqLongSamples[songSeq->instrumentID].volume;
	}

	else
	{
		sampleVol = sdata->ptrCseqShortSamples[index].volume;
	}

	Channel_SetVolume(attr, (newVol * sampleVol * vol) >> 0xf, songSeq->LR);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002ae64-0x8002af6c
void UpdateChannelVol_EngineFX_All()
{
	struct ChannelStats *curr;

	for (curr = (struct ChannelStats *)sdata->channelTaken.first; curr != NULL; curr = curr->next)
	{
		if (curr->type == HOWL_CHANNEL_TYPE_MUSIC)
		{
			continue;
		}

		// update volume
		sdata->ChannelUpdateFlags[curr->channelID] |= HOWL_CHANNEL_UPDATE_VOLUME;

		// just the sound, not the instance of sound
		int soundID = curr->soundID & 0xffff;

		if (curr->type == HOWL_CHANNEL_TYPE_ENGINE_FX)
		{
			UpdateChannelVol_EngineFX(&sdata->howl_metaEngineFX[soundID], &sdata->channelAttrNew[curr->channelID], curr->vol, curr->LR);
		}

		// type == OtherFX
		else
		{
			UpdateChannelVol_OtherFX(&sdata->howl_metaOtherFX[soundID], &sdata->channelAttrNew[curr->channelID], curr->vol, curr->LR);
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002af6c-0x8002b030
void UpdateChannelVol_Music_All()
{
	struct ChannelStats *curr, *backupNext;

	for (curr = (struct ChannelStats *)sdata->channelTaken.first; curr != NULL; curr = backupNext)
	{
		backupNext = curr->next;

		if (curr->type != HOWL_CHANNEL_TYPE_MUSIC)
		{
			continue;
		}

		// update volume
		sdata->ChannelUpdateFlags[curr->channelID] |= HOWL_CHANNEL_UPDATE_VOLUME;

		UpdateChannelVol_Music(&sdata->songSeq[curr->soundID & 0xffff], &sdata->channelAttrNew[curr->channelID], curr->drumIndex_pitchIndex, curr->vol);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002b030-0x8002b0e0
void UpdateChannelVol_OtherFX_All()
{
	struct ChannelStats *curr, *backupNext;

	for (curr = (struct ChannelStats *)sdata->channelTaken.first; curr != NULL; curr = backupNext)
	{
		backupNext = curr->next;

		if (curr->type != HOWL_CHANNEL_TYPE_OTHER_FX)
		{
			continue;
		}

		// update volume
		sdata->ChannelUpdateFlags[curr->channelID] |= HOWL_CHANNEL_UPDATE_VOLUME;

		UpdateChannelVol_OtherFX(&sdata->howl_metaOtherFX[curr->soundID & 0xffff], &sdata->channelAttrNew[curr->channelID], curr->vol, curr->LR);
	}
}

// real Naughty Dog name
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002b0e0-0x8002b130
int howl_VolumeGet(int type)
{
	if (type == HOWL_VOLUME_TYPE_MUSIC)
	{
		return sdata->vol_Music;
	}

	if (type == HOWL_VOLUME_TYPE_FX)
	{
		return sdata->vol_FX;
	}

	if (type == HOWL_VOLUME_TYPE_VOICE)
	{
		return sdata->vol_Voice;
	}

	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002b130-0x8002b1f0
void howl_VolumeSet(int type, u8 vol)
{
	if (type == HOWL_VOLUME_TYPE_MUSIC)
	{
		if (sdata->vol_Music == vol)
		{
			return;
		}

		sdata->vol_Music = vol;

		Smart_EnterCriticalSection();

		UpdateChannelVol_Music_All();
	}
	else if (type == HOWL_VOLUME_TYPE_FX)
	{
		if (sdata->vol_FX == vol)
		{
			return;
		}

		sdata->vol_FX = vol;

		Smart_EnterCriticalSection();

		UpdateChannelVol_EngineFX_All();
	}
	else
	{
		if (type != HOWL_VOLUME_TYPE_VOICE)
		{
			return;
		}

		if (sdata->vol_Voice == vol)
		{
			return;
		}

		sdata->vol_Voice = vol;

		Smart_EnterCriticalSection();

		UpdateChannelVol_OtherFX_All();
	}

	Smart_ExitCriticalSection();
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002b1f0-0x8002b1fc
int howl_ModeGet(void)
{
	return sdata->boolStereoEnabled;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002b1fc-0x8002b208
void howl_ModeSet(int newMode)
{
	sdata->boolStereoEnabled = newMode;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002b208-0x8002b4d0
void OptionsMenu_TestSound(int newRow, int newBoolPlay)
{
	int oldRow = sdata->OptionSlider_Index;
	int oldBoolPlay = sdata->OptionSlider_BoolPlay;

	if ((oldBoolPlay != 0) && (oldRow != newRow))
	{
		// FX row
		if (oldRow == 0)
		{
			OtherFX_Stop2(0x48);
		}

		// Music row
		else if (oldRow == 1)
		{
			// end Aku or Uka song
			// 0=level, 1=aku, 2=uka

			if (Music_GetHighestSongPlayIndex() == 1)
			{
				CseqMusic_Stop(CSEQ_SONG_UKA);
			}

			else
			{
				CseqMusic_Stop(CSEQ_SONG_AKU);
			}
		}

		// Voice row
		else if (oldRow == 2)
		{
			if (sdata->OptionSlider_soundID != 0)
			{
				OtherFX_Stop1(sdata->OptionSlider_soundID);
				sdata->OptionSlider_soundID = 0;
			}
		}
	}

	if ((newBoolPlay != oldBoolPlay) || (oldRow != newRow))
	{
		if (newBoolPlay != 0)
		{
			// FX row
			if (newRow == 0)
			{
				OtherFX_Play(0x48, 0);
			}

			// Music row
			else if (newRow == 1)
			{
				// end Aku or Uka song
				// 0=level, 1=aku, 2=uka

				int val = 1;
				if (Music_GetHighestSongPlayIndex() == 1)
				{
					val = 2;
				}

				CseqMusic_Start(val, 0, NULL, 0, 1);
			}
		}
		else
		{
			// FX row
			if (oldRow == 0)
			{
				OtherFX_Stop2(0x48);
			}

			// Music row
			else if (oldRow == 1)
			{
				// end Aku or Uka song
				// 0=level, 1=aku, 2=uka

				if (Music_GetHighestSongPlayIndex() == 1)
				{
					CseqMusic_Stop(CSEQ_SONG_UKA);
				}

				else
				{
					CseqMusic_Stop(CSEQ_SONG_AKU);
				}
			}

			// Voice row
			else if (oldRow == 2)
			{
				if (sdata->OptionSlider_soundID != 0)
				{
					OtherFX_Stop1(sdata->OptionSlider_soundID);
					sdata->OptionSlider_soundID = 0;
				}
			}
		}

		sdata->OptionSlider_BoolPlay = newBoolPlay;
		sdata->OptionSlider_Index = newRow;
	}

	// Voice row
	if ((sdata->OptionSlider_BoolPlay != 0) && (sdata->OptionSlider_Index == 2))
	{
		// OG game does this, instead of gGT->drivers[0]?
		int driverID = sdata->gGT->cameraDC[0].driverToFollow->driverID;

		int characterID = data.characterIDs[driverID];

		int frameCount = sdata->gGT->frameTimer_MainFrame_ResetDB;

		int sampleVoiceID;

		// every 25th frame
		if (frameCount == (frameCount / 25) * 25)
		{
			// every 50th frame (0, 50, 100, 150)
			if (frameCount == (frameCount / 50) * 50)
			{
				sampleVoiceID = characterID + 0x1c;
			}

			// every 50th frame (25, 75, 125, 175)
			else
			{
				sampleVoiceID = characterID + 0x2c;
			}

			sdata->OptionSlider_soundID = OtherFX_Play(sampleVoiceID, 0);
		}
	}
}
