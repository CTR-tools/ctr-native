#include <common.h>
#include <platform/native_spu_memory.h>

void Cutscene_VolumeBackup(void)
{
	// enter critical section
	Smart_EnterCriticalSection();

	// make another copy volume of FX and clamp to 0x100
	sdata->storedVolume = howl_VolumeGet(0) & 0xff;
	sdata->currentVolume = sdata->storedVolume;

	// copy exists
	sdata->boolStoringVolume = 1;

	// exit critical section
	Smart_ExitCriticalSection();

	return;
}

void Cutscene_VolumeRestore(void)
{
	// enter critical section
	Smart_EnterCriticalSection();

	// copy does not exist
	sdata->boolStoringVolume = 0;

	// Set volume of FX
	howl_VolumeSet(0, sdata->storedVolume);

	// exit critical section
	Smart_ExitCriticalSection();

	return;
}

void howl_PlayAudio_Update()
{
	u32 *ptrFlag;
	struct ChannelStats *curr, *backupNext;
	u8 statFlags;

	if (sdata->boolAudioEnabled != 0)
	{
		// if copy exists, make audio fade slowly
		if (sdata->boolStoringVolume != 0)
		{
			sdata->currentVolume -= 2;
			if (sdata->currentVolume < 0)
			{
				sdata->currentVolume = 0;
			}

			sdata->criticalSectionCount = 1;
			howl_VolumeSet(0, sdata->currentVolume);
			sdata->criticalSectionCount = 0;
		}

		for (curr = (struct ChannelStats *)sdata->channelTaken.first; curr != NULL; curr = backupNext)
		{
			backupNext = curr->link.links.next;
			if (Chaos_ShouldFreezeAudioChannel(curr))
			{
				continue;
			}

			// if sound has no timer (plays inf)
			statFlags = curr->flags;
			if ((statFlags & 4) != 0)
			{
				continue;
			}

			// play sound until timer runs out
			curr->timeLeft -= 5;
			if (curr->timeLeft > 0)
			{
				continue;
			}

			ptrFlag = &sdata->ChannelUpdateFlags[curr->channelID];
			*ptrFlag |= 1;
			*ptrFlag &= ~(2);

			curr->flags = statFlags & ~(1);

			LIST_RemoveMember(&sdata->channelTaken, (struct Item *)curr);
			LIST_AddBack(&sdata->channelFree, (struct Item *)curr);
		}

		Channel_ParseSongToChannels();
	}

	Channel_UpdateChannels();
}

void howl_InitChannelAttr_EngineFX(struct EngineFX *engineFX, struct ChannelAttr *attr, int vol, int LR, int distort)
{
	Channel_SetVolume(attr, (sdata->vol_FX * engineFX->volume * vol) >> 10, LR);

	s16 pitch = engineFX->pitch;

	if (distort != HOWL_SFX_DISTORTION_NONE)
	{
		pitch = ((u32)pitch * data.distortConst_Engine[distort]) >> 0x10;
	}

	attr->pitch = pitch;

	// ADSR
	attr->ad = 0x80ff;
	attr->sr = 0x1fc2;

	attr->spuStartAddr = (void *)CTR_SPU_BYTES_AT((u32)sdata->howl_spuAddrs[engineFX->spuIndex].spuAddr);
}

void howl_InitChannelAttr_OtherFX(struct OtherFX *otherFX, struct ChannelAttr *attr, int vol, int LR, int distort)
{
	UpdateChannelVol_OtherFX(otherFX, attr, vol, LR);

	s16 pitch = otherFX->pitch;

	if (distort != HOWL_SFX_DISTORTION_NONE)
	{
		pitch = ((int)pitch * (int)data.distortConst_OtherFX[distort]) >> 0x10;
	}

	attr->pitch = pitch;

	// ADSR
	attr->ad = 0x80ff;
	attr->sr = 0x1fc2;

	attr->spuStartAddr = (void *)CTR_SPU_BYTES_AT((u32)sdata->howl_spuAddrs[otherFX->spuIndex].spuAddr);
}

void howl_PauseAudio()
{
	u32 *ptrFlag;
	struct ChannelStats *curr, *backupNext;
	struct ChannelStats *pausedStats;

	CDSYS_XAPauseRequest();

	// if already paused, quit
	if (sdata->numBackup_ChannelStats != 0)
	{
		return;
	}

	pausedStats = &sdata->channelStatsCurr[0];

	CseqMusic_Pause();

	Smart_EnterCriticalSection();
	for (curr = (struct ChannelStats *)sdata->channelTaken.first; curr != NULL; curr = backupNext)
	{
		backupNext = curr->link.links.next;

		ptrFlag = &sdata->ChannelUpdateFlags[curr->channelID];
		*ptrFlag |= 1;
		*ptrFlag &= ~(2);

		int *dest = (int *)pausedStats++;
		int *src = (int *)curr;

		// psx's kernel memcpy does NOT work inside "critical" sections
		dest[0] = src[0];
		dest[1] = src[1];
		dest[2] = src[2];
		dest[3] = src[3];
		dest[4] = src[4];
		dest[5] = src[5];
		dest[6] = src[6];
		dest[7] = src[7];

		LIST_RemoveMember(&sdata->channelTaken, (struct Item *)curr);
		LIST_AddBack(&sdata->channelFree, (struct Item *)curr);

		sdata->numBackup_ChannelStats++;
	}
	Smart_ExitCriticalSection();
}

void howl_UnPauseChannel(struct ChannelStats *stats)
{
	int type;
	int soundID;
	struct ChannelAttr attr;

	type = stats->type;
	soundID = stats->soundID & 0xffff;

	if (type == HOWL_CHANNEL_TYPE_ENGINE_FX)
	{
		howl_InitChannelAttr_EngineFX(&sdata->howl_metaEngineFX[soundID], &attr, stats->vol, stats->LR, stats->distort);
	}

	else if (type == HOWL_CHANNEL_TYPE_OTHER_FX)
	{
		howl_InitChannelAttr_OtherFX(&sdata->howl_metaOtherFX[soundID], &attr,
		    HowlChannel_EffectiveVolume(stats), stats->LR, stats->distort);
	}

	else if (type == HOWL_CHANNEL_TYPE_MUSIC)
	{
		howl_InitChannelAttr_Music(&sdata->songSeq[soundID], &attr, stats->drumIndex_pitchIndex, stats->vol);
	}
	else
	{
		return;
	}

	// enable all bits in ChannelUpdate flag
	sdata->ChannelUpdateFlags[stats->channelID] |= HOWL_CHANNEL_UPDATE_RESUME;

	memcpy(&sdata->channelAttrNew[stats->channelID], &attr, sizeof(attr));
}

void howl_UnPauseAudio()
{
	int i;
	u8 backupID;
	struct ChannelStats *backupPrev;
	struct ChannelStats *curr, *backupNext;
	struct ChannelStats *pausedStats;

	// if no paused audio, skip
	if (sdata->numBackup_ChannelStats == 0)
	{
		return;
	}

	pausedStats = &sdata->channelStatsCurr[0];

	Smart_EnterCriticalSection();
	for (i = 0, curr = (struct ChannelStats *)sdata->channelFree.first; i < sdata->numBackup_ChannelStats; i++, curr = backupNext)
	{
		if (curr == NULL)
		{
			break;
		}

		backupID = curr->channelID;
		backupPrev = curr->link.links.prev;
		backupNext = curr->link.links.next;

		int *src = (int *)pausedStats++;
		int *dest = (int *)curr;

		// psx's kernel memcpy does NOT work inside "critical" sections
		dest[0] = src[0];
		dest[1] = src[1];
		dest[2] = src[2];
		dest[3] = src[3];
		dest[4] = src[4];
		dest[5] = src[5];
		dest[6] = src[6];
		dest[7] = src[7];

		curr->link.links.next = backupNext;
		curr->link.links.prev = backupPrev;
		curr->channelID = backupID;

		LIST_RemoveMember(&sdata->channelFree, (struct Item *)curr);
		LIST_AddBack(&sdata->channelTaken, (struct Item *)curr);

		howl_UnPauseChannel(curr);
	}
	Smart_ExitCriticalSection();

	CseqMusic_Resume();

	sdata->numBackup_ChannelStats = 0;
}

void howl_StopAudio(b32 boolErasePauseBackup, b32 boolEraseMusic, b32 boolDestroyAllFX)
{
	if (boolEraseMusic != 0)
	{
		CseqMusic_StopAll();
	}

	Smart_EnterCriticalSection();
	Channel_DestroyAll_LowLevel(boolDestroyAllFX, boolEraseMusic == 0, 2);
	Smart_ExitCriticalSection();

	if (boolErasePauseBackup != 0)
	{
		sdata->numBackup_ChannelStats = 0;
	}
}
