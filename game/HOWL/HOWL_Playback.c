#include <common.h>

#ifndef GAME_PLAYBACK_VOLUME_FX
#define GAME_PLAYBACK_VOLUME_FX          (sdata->vol_FX)
#define GAME_PLAYBACK_VOLUME_VOICE       (sdata->vol_Voice)
#define GAME_PLAYBACK_ENGINE_DISTORTIONS (data.distortConst_Engine)
#define GAME_PLAYBACK_ENGINE_FX_METADATA (sdata->howl_metaEngineFX)
#define GAME_PLAYBACK_OTHER_FX_METADATA  (sdata->howl_metaOtherFX)
#define GAME_PLAYBACK_CHANNEL_BACKUPS    (sdata->channelStatsCurr)
#define GAME_PLAYBACK_BACKUP_COUNT       (sdata->numBackup_ChannelStats)
#endif

void Cutscene_VolumeBackup(void)
{
	// enter critical section
	Smart_EnterCriticalSection();

	// make another copy volume of FX and clamp to 0x100
	sdata->storedVolume = howl_VolumeGet(0) & 0xff;
	sdata->currentVolume = sdata->storedVolume;

	// copy exists
	sdata->boolStoringVolume = 1;
	// NOTE(aalhendi): Keep the state store ahead of the next call on PSX.
	CTR_PSX_MEMORY_BARRIER();

	// exit critical section
	Smart_ExitCriticalSection();

	return;
}

void Cutscene_VolumeRestore(void)
{
	int storedVolume;

	// enter critical section
	Smart_EnterCriticalSection();

	storedVolume = sdata->storedVolume;
	// copy does not exist
	sdata->boolStoringVolume = 0;
	// NOTE(aalhendi): Keep the state store ahead of the next call on PSX.
	CTR_PSX_MEMORY_BARRIER();

	// Set volume of FX
	howl_VolumeSet(0, storedVolume);

	// exit critical section
	Smart_ExitCriticalSection();

	return;
}

void howl_PlayAudio_Update(void)
{
	struct ChannelStats *curr, *backupNext;
	if (GAME_AUDIO_ENABLED != 0)
	{
		if (sdata->boolStoringVolume != 0)
		{
			s16 volume = sdata->currentVolume - 2;
			sdata->currentVolume = volume;
			if (volume < 0)
			{
				sdata->currentVolume = 0;
			}
			{
				// NOTE(aalhendi): Retail reloads the low byte. PSX and native i686
				// use little-endian storage, and unsigned-char access may alias it.
				int currentVolume = *(u8 *)&sdata->currentVolume;
				sdata->criticalSectionCount = 1;
				// NOTE(aalhendi): Keep the critical-section store before the call.
				CTR_PSX_OBSERVE_MEMORY(sdata->criticalSectionCount);
				howl_VolumeSet(0, currentVolume);
			}
			sdata->criticalSectionCount = 0;
		}
		for (curr = (struct ChannelStats *)GAME_CHANNEL_TAKEN.first; curr != NULL; curr = backupNext)
		{
			backupNext = curr->link.links.next;
			if ((curr->flags & 4) != 0)
			{
				continue;
			}
			{
				s16 timeLeft = curr->timeLeft - 5;
				curr->timeLeft = timeLeft;
				if (timeLeft > 0)
				{
					continue;
				}
			}
			GAME_CHANNEL_UPDATE_FLAGS[curr->channelID] |= HOWL_CHANNEL_UPDATE_OFF;
			// NOTE(aalhendi): Retail performs separate indexed OFF and KEY_ON updates.
			*(volatile u32 *)&GAME_CHANNEL_UPDATE_FLAGS[curr->channelID] &= ~HOWL_CHANNEL_UPDATE_KEY_ON;
			curr->flags &= (u8)~1;
			LIST_RemoveMember(&GAME_CHANNEL_TAKEN, (struct Item *)curr);
			LIST_AddBack(&GAME_CHANNEL_FREE, (struct Item *)curr);
		}
		Channel_ParseSongToChannels();
	}
	Channel_UpdateChannels();
}

void howl_InitChannelAttr_EngineFX(struct EngineFX *engineFX, struct ChannelAttr *attr, int vol, int LR, int distort)
{
	register struct EngineFX *source CTR_PSX_REGISTER("$17") = engineFX;
	register struct ChannelAttr *output CTR_PSX_REGISTER("$18") = attr;
	u32 volume = GAME_PLAYBACK_VOLUME_FX;
	register u32 pitch CTR_PSX_REGISTER("$2");

	// Retail reads pitch after the volume callback through the captured metadata pointer.
	Channel_SetVolume(output, (volume * source->volume * (u32)vol) >> 10, LR);

	if (distort != HOWL_SFX_DISTORTION_NONE)
	{
		pitch = ((u32)source->pitch * (u32)GAME_PLAYBACK_ENGINE_DISTORTIONS[distort]) >> 16;
	}
	else
	{
		pitch = source->pitch;
	}

	output->pitch = pitch;
	{
		u32 sampleAddr = GAME_HOWL_SPU_ADDRS[source->spuIndex].spuAddr;
		u32 ad;
		ad = 0x80ff;
		output->ad = ad;
		output->sr = 0x1fc2;
		output->spuStartAddr = (void *)(sampleAddr << 3);
	}
}

void howl_InitChannelAttr_OtherFX(struct OtherFX *otherFX, struct ChannelAttr *attr, int vol, int LR, int distort)
{
	struct OtherFX *source = otherFX;
	struct ChannelAttr *output = attr;
	register u32 master CTR_PSX_REGISTER("$3");
	register int distortion CTR_PSX_REGISTER("$4") = distort;
	register u32 product CTR_PSX_REGISTER("$8");
	u32 volume;
	u32 pitch;

	if ((source->flags & 4) != 0)
	{
		master = GAME_PLAYBACK_VOLUME_VOICE;
	}
	else
	{
		master = GAME_PLAYBACK_VOLUME_FX;
	}
	{
		register u32 sampleVolume CTR_PSX_REGISTER("$2") = source->volume;
		product = master * sampleVolume;
		product *= (u32)vol;
		// NOTE(aalhendi): Finish the wrapping volume product before preparing the pitch branch.
		CTR_PSX_OBSERVE_VALUE(product);
		volume = product >> 10;
	}

	if (distortion != HOWL_SFX_DISTORTION_NONE)
	{
		s32 *table = GAME_DISTORT_CONST_OTHER_FX;
		register u32 offset CTR_PSX_REGISTER("$3");
		s32 *factor;
		register u32 rawPitch CTR_PSX_REGISTER("$4");
		register u32 coefficient CTR_PSX_REGISTER("$2");
		// NOTE(aalhendi): Retail forms the table base before scaling the saved index.
		CTR_PSX_DEPEND_VALUE(distortion, table);
		offset = (u32)distortion << 2;
		factor = (s32 *)((u8 *)table + offset);
		rawPitch = source->pitch;
		coefficient = (u32)*factor;
		product = rawPitch * coefficient;
		pitch = product >> 16;
	}
	else
	{
		pitch = source->pitch;
	}

	// Retail captures unsigned pitch before this callback and reloads the SPU index afterward.
	Channel_SetVolume(output, volume, LR);
	output->pitch = pitch;
	{
		u32 sampleAddr = GAME_HOWL_SPU_ADDRS[source->spuIndex].spuAddr;
		u32 ad;
		ad = 0x80ff;
		output->ad = ad;
		output->sr = 0x1fc2;
		output->spuStartAddr = (void *)(sampleAddr << 3);
	}
}

void howl_PauseAudio(void)
{
	struct ChannelStats *curr, *backupNext;
	CDSYS_XAPauseRequest();
	if (GAME_PLAYBACK_BACKUP_COUNT != 0)
	{
		return;
	}
	CseqMusic_Pause();
	Smart_EnterCriticalSection();
	for (curr = (struct ChannelStats *)GAME_CHANNEL_TAKEN.first; curr != NULL; curr = backupNext)
	{
		struct LinkedList *takenList = &GAME_CHANNEL_TAKEN;
		struct Item *member;
		// NOTE(aalhendi): The clear mask reuses the retail channel-ID scratch.
		register u32 clearMask CTR_PSX_REGISTER("$3");
		int backupIndex;
		struct ChannelStats *snapshot;
		u32 *updateFlags;
		u32 *pendingFlags;
		int oldCount;
		// NOTE(aalhendi): Keep the initial channel-ID load in the same scratch.
		register int channelID CTR_PSX_REGISTER("$3");
		channelID = curr->channelID;
		member = (struct Item *)curr;
		updateFlags = &GAME_CHANNEL_UPDATE_FLAGS[channelID];
		backupNext = curr->link.links.next;
		*updateFlags |= HOWL_CHANNEL_UPDATE_OFF;
		clearMask = ~HOWL_CHANNEL_UPDATE_KEY_ON;
		oldCount = GAME_PLAYBACK_BACKUP_COUNT;
		pendingFlags = &GAME_CHANNEL_UPDATE_FLAGS[curr->channelID];
		backupIndex = oldCount;
		oldCount++;
		GAME_PLAYBACK_BACKUP_COUNT = oldCount;
		snapshot = &GAME_PLAYBACK_CHANNEL_BACKUPS[backupIndex];
		// NOTE(aalhendi): Retain the second indexed flag read before the snapshot.
		*(volatile u32 *)pendingFlags &= clearMask;
		*snapshot = *curr;
		LIST_RemoveMember(takenList, member);
		LIST_AddBack(&GAME_CHANNEL_FREE, (struct Item *)curr);
	}
	Smart_ExitCriticalSection();
}

void howl_UnPauseChannel(struct ChannelStats *stats)
{
	struct ChannelAttr attr;
#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Retail FX leaves this halfword undefined. Native restores
	// the saved channel echo; Music overwrites it with its own reverb setting.
	attr.reverb = stats->echo;
#endif
	switch (stats->type)
	{
	case HOWL_CHANNEL_TYPE_OTHER_FX:
		howl_InitChannelAttr_OtherFX(&GAME_PLAYBACK_OTHER_FX_METADATA[(u16)stats->soundID], &attr, stats->vol, stats->LR, stats->distort);
		break;
	case HOWL_CHANNEL_TYPE_ENGINE_FX:
		howl_InitChannelAttr_EngineFX(&GAME_PLAYBACK_ENGINE_FX_METADATA[(u16)stats->soundID], &attr, stats->vol, stats->LR, stats->distort);
		break;
	case HOWL_CHANNEL_TYPE_MUSIC:
		howl_InitChannelAttr_Music(&GAME_SONG_SEQUENCES[stats->soundID], &attr, stats->drumIndex_pitchIndex, stats->vol);
		break;
	default:
		return;
	}
	GAME_CHANNEL_UPDATE_FLAGS[stats->channelID] |= HOWL_CHANNEL_UPDATE_RESUME;
	GAME_CHANNEL_ATTR_NEW[stats->channelID] = attr;
}

void howl_UnPauseAudio(void)
{
	// NOTE(aalhendi): Retail preserves the loop count here across callbacks.
	register int i CTR_PSX_REGISTER("$18");
	struct ChannelStats *curr, *backupNext;
	struct ChannelStats *snapshot;
	if (GAME_PLAYBACK_BACKUP_COUNT == 0)
	{
		return;
	}
	i = 0;
	Smart_EnterCriticalSection();
	curr = (struct ChannelStats *)GAME_CHANNEL_FREE.first;
	if (GAME_PLAYBACK_BACKUP_COUNT > 0)
	{
		snapshot = GAME_PLAYBACK_CHANNEL_BACKUPS;
		do
		{
			struct Item savedLinks;
			u8 savedID;
			// NOTE(aalhendi): Keep both list arguments live during the snapshot
			// so the ordinary struct copy uses the remaining argument registers.
			register struct LinkedList *freeList CTR_PSX_REGISTER("$4");
			register struct Item *member CTR_PSX_REGISTER("$5");
			if (curr == NULL)
			{
				break;
			}
			freeList = &GAME_CHANNEL_FREE;
			backupNext = curr->link.links.next;
			savedID = curr->channelID;
			// NOTE(aalhendi): Save the channel ID before preparing the list member.
			CTR_PSX_DEPEND_VALUE(curr, savedID);
			member = (struct Item *)curr;
			// NOTE(aalhendi): Keep the member argument live through the struct copy.
			CTR_PSX_KEEP_VALUE_RELAXED(member);
			savedLinks = curr->link.item;
			*curr = *snapshot++;
			curr->channelID = savedID;
			curr->link.item = savedLinks;
			// NOTE(aalhendi): Restore links before incrementing the loop count.
			CTR_PSX_MEMORY_BARRIER();
			i++;
			LIST_RemoveMember(freeList, member);
			{
				// NOTE(aalhendi): Rematerialize the list argument after the callback.
				register struct LinkedList *takenList CTR_PSX_REGISTER("$4") = &GAME_CHANNEL_TAKEN;
				CTR_PSX_KEEP_VALUE_RELAXED(takenList);
				LIST_AddBack(takenList, (struct Item *)curr);
			}
			howl_UnPauseChannel(curr);
			curr = backupNext;
		} while (i < GAME_PLAYBACK_BACKUP_COUNT);
	}
	Smart_ExitCriticalSection();
	CseqMusic_Resume();
	GAME_PLAYBACK_BACKUP_COUNT = 0;
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
