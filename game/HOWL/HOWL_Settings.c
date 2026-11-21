#include <common.h>

#define HOWL_AUDIO_ENABLED   sdata->boolAudioEnabled
#define HOWL_STEREO          sdata->boolStereoEnabled
#define HOWL_VOLUME_FX       sdata->vol_FX
#define HOWL_VOLUME_MUSIC    sdata->vol_Music
#define HOWL_VOLUME_VOICE    sdata->vol_Voice
#define HOWL_CHANNELS        sdata->channelTaken
#define HOWL_CHANNEL_UPDATES sdata->ChannelUpdateFlags
#define HOWL_CHANNEL_ATTRS   sdata->channelAttrNew
#define HOWL_SONG_SEQUENCES  sdata->songSeq
#define HOWL_SONGS           sdata->songPool
#define HOWL_SLIDER_PLAYING  sdata->OptionSlider_BoolPlay
#define HOWL_SLIDER_ROW      sdata->OptionSlider_Index
#define HOWL_SLIDER_SOUND    sdata->OptionSlider_soundID

b32 howl_Disable(void)
{
	if (HOWL_AUDIO_ENABLED == 0)
	{
		return 0;
	}

	HOWL_AUDIO_ENABLED = 0;
	return 1;
}

void UpdateChannelVol_EngineFX(struct EngineFX *engineFX, struct ChannelAttr *attr, u32 vol, s32 LR)
{
	u32 fxVolume = HOWL_VOLUME_FX;
	Channel_SetVolume(attr, (fxVolume * engineFX->volume * vol) >> 10, LR);
}

void UpdateChannelVol_OtherFX(struct OtherFX *otherFX, struct ChannelAttr *attr, u32 vol, s32 LR)
{
	register struct ChannelAttr *output CTR_PSX_REGISTER("$8") = attr;
	u32 otherVol;
	u32 finalVolume;

	if ((otherFX->flags & 4) != 0)
	{
		otherVol = HOWL_VOLUME_VOICE;
	}
	else
	{
		otherVol = HOWL_VOLUME_FX;
	}

	// NOTE(aalhendi): Retail finishes the wrapping gain before preparing call arguments.
	finalVolume = (otherVol * otherFX->volume * vol) >> 10;
	CTR_PSX_OBSERVE_VALUE(finalVolume);
	Channel_SetVolume(output, finalVolume, LR);
}

void UpdateChannelVol_Music(struct SongSeq *seq, struct ChannelAttr *attr, s32 index, u32 vol)
{
	register struct ChannelAttr *output CTR_PSX_REGISTER("$9") = attr;
	struct Song *song = &HOWL_SONGS[seq->songPoolIndex];
	register u32 product CTR_PSX_REGISTER("$10");
	register s32 mix CTR_PSX_REGISTER("$5");
	s32 sampleVol;
	u32 finalVolume;

	// NOTE(aalhendi): Keep the shift between sequence and sample gain: moving it
	// changes rounding. The bindings preserve retail's sample-load scheduling;
	// their constraints emit no instructions and are inert on native.
	if ((seq->flags & 4) != 0)
	{
		register s32 master CTR_PSX_REGISTER("$3");
		register s32 level CTR_PSX_REGISTER("$2");
		register struct SampleDrums *sample CTR_PSX_REGISTER("$2");

		master = HOWL_VOLUME_MUSIC;
		level = song->vol_Curr;
		product = master * level;
		level = seq->vol_Curr;
		product *= level;
		sample = sdata->ptrCseqShortSamples;
		sample += index;
		CTR_PSX_DEPEND_VALUE(sample, product);
		sampleVol = sample->volume;
		mix = (s32)product >> 10;
	}
	else
	{
		register s32 master CTR_PSX_REGISTER("$3");
		register s32 level CTR_PSX_REGISTER("$2");
		register struct SampleInstrument *sample CTR_PSX_REGISTER("$3");
		register s32 offset CTR_PSX_REGISTER("$2");
		register s32 instrument CTR_PSX_REGISTER("$3");

		master = HOWL_VOLUME_MUSIC;
		level = song->vol_Curr;
		product = master * level;
		level = seq->vol_Curr;
		product *= level;
		instrument = seq->instrumentID;
		CTR_PSX_KEEP_VALUE_RELAXED(instrument);
		offset = instrument * ((s32)sizeof(struct SampleInstrument) / (s32)sizeof(u32));
		CTR_PSX_KEEP_VALUE_RELAXED(offset);
		sample = sdata->ptrCseqLongSamples;
		offset *= (s32)sizeof(u32);
		sample = (struct SampleInstrument *)((u8 *)sample + offset);
		CTR_PSX_DEPEND_VALUE(sample, product);
		sampleVol = sample->volume;
		CTR_PSX_ORDER_VALUES(product, sampleVol);
		mix = (s32)product >> 10;
	}

	// The final channel gain is a wrapping 32-bit product.
	product = mix * sampleVol;
	product *= vol;
	finalVolume = product >> 15;
	CTR_PSX_OBSERVE_VALUE(finalVolume);
	Channel_SetVolume(output, finalVolume, seq->LR);
}

void UpdateChannelVol_EngineFX_All(void)
{
	struct ChannelStats *curr;

	for (curr = (struct ChannelStats *)HOWL_CHANNELS.first; curr != NULL; curr = curr->link.links.next)
	{
		if (curr->type == HOWL_CHANNEL_TYPE_ENGINE_FX)
		{
			HOWL_CHANNEL_UPDATES[curr->channelID] |= HOWL_CHANNEL_UPDATE_VOLUME;
			UpdateChannelVol_EngineFX(&sdata->howl_metaEngineFX[curr->soundID & 0xffff], &HOWL_CHANNEL_ATTRS[curr->channelID], curr->vol, curr->LR);
		}
		else if (curr->type == HOWL_CHANNEL_TYPE_OTHER_FX)
		{
			HOWL_CHANNEL_UPDATES[curr->channelID] |= HOWL_CHANNEL_UPDATE_VOLUME;
			UpdateChannelVol_OtherFX(&sdata->howl_metaOtherFX[curr->soundID & 0xffff], &HOWL_CHANNEL_ATTRS[curr->channelID],
			    HowlChannel_EffectiveVolume(curr), curr->LR);
		}
	}
}

void UpdateChannelVol_Music_All(void)
{
	struct ChannelStats *curr;
	struct ChannelAttr *attr;

	for (curr = (struct ChannelStats *)HOWL_CHANNELS.first; curr != NULL; curr = curr->link.links.next)
	{
		if (curr->type != HOWL_CHANNEL_TYPE_MUSIC)
		{
			continue;
		}

		// update volume
		HOWL_CHANNEL_UPDATES[curr->channelID] |= HOWL_CHANNEL_UPDATE_VOLUME;

		attr = &HOWL_CHANNEL_ATTRS[curr->channelID];
		UpdateChannelVol_Music(&HOWL_SONG_SEQUENCES[curr->soundID], attr, curr->drumIndex_pitchIndex, curr->vol);
	}
}

void UpdateChannelVol_OtherFX_All(void)
{
	struct ChannelStats *curr;
	struct ChannelAttr *attr;

	for (curr = (struct ChannelStats *)HOWL_CHANNELS.first; curr != NULL; curr = curr->link.links.next)
	{
		if (curr->type != HOWL_CHANNEL_TYPE_OTHER_FX)
		{
			continue;
		}

		// update volume
		HOWL_CHANNEL_UPDATES[curr->channelID] |= HOWL_CHANNEL_UPDATE_VOLUME;

		attr = &HOWL_CHANNEL_ATTRS[curr->channelID];
		UpdateChannelVol_OtherFX(&sdata->howl_metaOtherFX[curr->soundID & 0xffff], attr,
		    HowlChannel_EffectiveVolume(curr), curr->LR);
	}
}

// real Naughty Dog name
s32 howl_VolumeGet(s32 type)
{
	switch ((u32)type)
	{
	case HOWL_VOLUME_TYPE_FX:
	{
		return HOWL_VOLUME_FX;
	}
	case HOWL_VOLUME_TYPE_MUSIC:
	{
		return HOWL_VOLUME_MUSIC;
	}
	case HOWL_VOLUME_TYPE_VOICE:
	{
		return HOWL_VOLUME_VOICE;
	}
	}
	return 0;
}

void howl_VolumeSet(s32 type, u8 vol)
{
	// Only changed sliders refresh active channels; store the new gain before
	// entering the audio critical section, and leave it once after the walk.
	switch ((u32)type)
	{
	case HOWL_VOLUME_TYPE_FX:
	{
		if (HOWL_VOLUME_FX == vol)
		{
			return;
		}
		HOWL_VOLUME_FX = vol;
		Smart_EnterCriticalSection();
		UpdateChannelVol_EngineFX_All();
		break;
	}
	case HOWL_VOLUME_TYPE_MUSIC:
	{
		if (HOWL_VOLUME_MUSIC == vol)
		{
			return;
		}
		HOWL_VOLUME_MUSIC = vol;
		Smart_EnterCriticalSection();
		UpdateChannelVol_Music_All();
		break;
	}
	case HOWL_VOLUME_TYPE_VOICE:
	{
		if (HOWL_VOLUME_VOICE == vol)
		{
			return;
		}
		HOWL_VOLUME_VOICE = vol;
		Smart_EnterCriticalSection();
		UpdateChannelVol_OtherFX_All();
		break;
	}
	default:
	{
		return;
	}
	}

	Smart_ExitCriticalSection();
}

s32 howl_ModeGet(void)
{
	return HOWL_STEREO;
}

void howl_ModeSet(s32 newMode)
{
	HOWL_STEREO = newMode;
}

void OptionsMenu_TestSound(s32 newRow, b32 newBoolPlay)
{
	const enum ChaosAudioOwner previousAudio = Chaos_SetAudioOwner(CHAOS_AUDIO_UI);
	// Stop the previous row's preview before changing the slider selection.
	if (HOWL_SLIDER_PLAYING && newRow != HOWL_SLIDER_ROW)
	{
		switch (HOWL_SLIDER_ROW)
		{
		case HOWL_VOLUME_TYPE_FX:
		{
			OtherFX_Stop2(0x48);
			break;
		}
		case HOWL_VOLUME_TYPE_MUSIC:
		{
			if (Music_GetHighestSongPlayIndex() == 1)
			{
				CseqMusic_Stop(CSEQ_SONG_UKA);
			}
			else
			{
				CseqMusic_Stop(CSEQ_SONG_AKU);
			}
			break;
		}
		case HOWL_VOLUME_TYPE_VOICE:
		{
			if (HOWL_SLIDER_SOUND)
			{
				OtherFX_Stop1(HOWL_SLIDER_SOUND);
				HOWL_SLIDER_SOUND = 0;
			}
			break;
		}
		}
	}
	// NOTE(aalhendi): Re-read preview state after stop calls; retail does not
	// cache the playing flag or selected row across those calls.
	if (newBoolPlay != HOWL_SLIDER_PLAYING || newRow != HOWL_SLIDER_ROW)
	{
		if (newBoolPlay)
		{
			switch (newRow)
			{
			case HOWL_VOLUME_TYPE_FX:
			{
				OtherFX_Play(0x48, 0);
				break;
			}
			case HOWL_VOLUME_TYPE_MUSIC:
			{
				if (Music_GetHighestSongPlayIndex() == 1)
				{
					CseqMusic_Start(CSEQ_SONG_UKA, 0, NULL, 0, 1);
				}
				else
				{
					CseqMusic_Start(CSEQ_SONG_AKU, 0, NULL, 0, 1);
				}
				break;
			}
			}
		}
		else
		{
			switch (HOWL_SLIDER_ROW)
			{
			case HOWL_VOLUME_TYPE_FX:
			{
				OtherFX_Stop2(0x48);
				break;
			}
			case HOWL_VOLUME_TYPE_MUSIC:
			{
				if (Music_GetHighestSongPlayIndex() == 1)
				{
					CseqMusic_Stop(CSEQ_SONG_UKA);
				}
				else
				{
					CseqMusic_Stop(CSEQ_SONG_AKU);
				}
				break;
			}
			case HOWL_VOLUME_TYPE_VOICE:
			{
				if (HOWL_SLIDER_SOUND)
				{
					OtherFX_Stop1(HOWL_SLIDER_SOUND);
					HOWL_SLIDER_SOUND = 0;
				}
				break;
			}
			}
		}
		HOWL_SLIDER_PLAYING = newBoolPlay;
		HOWL_SLIDER_ROW = newRow;
	}
	// Voice preview alternates the followed driver's clips every 25 frames.
	if (HOWL_SLIDER_PLAYING && HOWL_SLIDER_ROW == HOWL_VOLUME_TYPE_VOICE)
	{
		struct GameTracker *gGT = GAME_TRACKER;
		// The main menu has no followed racer. Preview player 1's character there.
		int driverID = 0;
		if ((gGT->gameMode1 & MAIN_MENU) == 0 && gGT->cameraDC[0].driverToFollow != NULL)
		{
			driverID = gGT->cameraDC[0].driverToFollow->driverID;
		}
		const u16 characterID = (u16)GAME_CHARACTER_IDS[driverID];
		u32 frames = gGT->frameTimer_MainFrame_ResetDB;
		if (frames == (frames / 25) * 25)
		{
			if (frames == (frames / 50) * 50)
			{
				HOWL_SLIDER_SOUND = OtherFX_Play((u16)(characterID + 0x1c), 0);
			}
			else
			{
				HOWL_SLIDER_SOUND = OtherFX_Play((u16)(characterID + 0x2c), 0);
			}
		}
	}
	Chaos_SetAudioOwner(previousAudio);
}
