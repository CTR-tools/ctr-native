#include <common.h>
#ifndef HOWL_VOICE_ENABLED
#define HOWL_VOICE_ENABLED (sdata->boolCanPlayVoicelines)
#endif

#ifndef HOWL_VOICE_STATS
#define HOWL_VOICE_STATS (sdata->channelStatsPrev)
#endif
#ifndef HOWL_VOICE_ATTR_CUR
#define HOWL_VOICE_ATTR_CUR (sdata->channelAttrCur)
#endif
#ifndef HOWL_VOICE_TIMES1
#define HOWL_VOICE_TIMES1 (sdata->timeSet1)
#endif
#ifndef HOWL_VOICE_TIMES2
#define HOWL_VOICE_TIMES2 (sdata->timeSet2)
#endif
#ifndef HOWL_VOICE_FREE
#define HOWL_VOICE_FREE (sdata->Voiceline1)
#endif
#ifndef HOWL_VOICE_TAKEN
#define HOWL_VOICE_TAKEN (sdata->Voiceline2)
#endif
#ifndef HOWL_VOICE_POOL
#define HOWL_VOICE_POOL (sdata->voicelinePool)
#endif

#ifndef HOWL_VOICE_BACKUP
#define HOWL_VOICE_BACKUP (sdata->backupParams_FUN_8002cf28)
#endif
#ifndef HOWL_VOICE_RNG
#define HOWL_VOICE_RNG (sdata->audioRNG)
#endif
#ifndef HOWL_VOICE_TYPES
#define HOWL_VOICE_TYPES (data.voiceID)
#endif
#ifndef HOWL_VOICE_SETS
#define HOWL_VOICE_SETS (data.voiceSetPtr)
#endif

// NOTE(aalhendi): This word view preserves the retail 16-byte backup copy.
struct VoicelineBackup
{
	u32 words[4];
} CTR_MAY_ALIAS;

// Initialize all audio pools.
void Voiceline_PoolInit(void)
{
	s32 index;

	sdata->criticalSectionCount = 0;

	sdata->numBackup_ChannelStats = 0;

	sdata->ptrCseqHeader = 0;
	// NOTE(aalhendi): Retail resets these fields before preparing the free list.
	CTR_PSX_MEMORY_BARRIER();

	Bank_ResetAllocator();

	Audio_SetDefaults();

	LIST_Clear(&GAME_CHANNEL_FREE);
	LIST_Clear(&GAME_CHANNEL_TAKEN);

	LIST_Init(&GAME_CHANNEL_FREE, &HOWL_VOICE_STATS[0].link.item, 0x20, 0x18);

	SpuSetReverbVoice(0, 0xffffff);

	// initialize all members in sound list
	for (index = 0; index < 24; index++)
	{
		struct ChannelStats *stats = &HOWL_VOICE_STATS[index];
		struct ChannelAttr *curr = &HOWL_VOICE_ATTR_CUR[index];

		SpuSetVoiceADSRAttr(index, 0, 0xf, 0x7f, 2, 0xf, 5, 1, 3);

		stats->flags = 0;
		stats->channelID = index;

		// NOTE(aalhendi): Retail materializes unsigned values before these halfword stores.
		*(u16 *)&stats->ad = 0x80ff;
		stats->sr = 0x1fc2;

		GAME_CHANNEL_UPDATE_FLAGS[index] = 0;

		curr->spuStartAddr = (void *)-1;

		*(u16 *)&curr->ad = 0x80ff;
		curr->sr = 0x1fc2;

		*(u16 *)&curr->pitch = 0xffff;
		*(u16 *)&curr->reverb = 0xffff;
		curr->audioL = -1;
		curr->audioR = -1;
	}

	for (index = 0; index < 2; index++)
	{
		struct Song *pool = &GAME_SONG_POOL[index];

		// not playing
		pool->flags = 0;

		pool->songPoolIndex = index;
	}

	for (index = 0; index < 24; index++)
	{
		struct SongSeq *seq = &GAME_SONG_SEQUENCES[index];

		// not playing
		seq->flags = 0;

		seq->soundID = index;
	}
}

void Voiceline_ClearTimeStamp(void)
{
	s32 i;

	for (i = 0; i < 16; i++)
	{
		// Clear audio timestamps arrays
		HOWL_VOICE_TIMES1[i] = 0;
		HOWL_VOICE_TIMES2[i] = 0;
	}
}

void Voiceline_PoolClear(void)
{
	struct LinkedList *freeList = &HOWL_VOICE_FREE;

	sdata->boolCanPlayWrongWaySFX = false;

	sdata->voicelineCooldown = 0;

	sdata->boolCanPlayVoicelines = false;
	// NOTE(aalhendi): Keep the state resets before the first list call.
	CTR_PSX_MEMORY_BARRIER();

	LIST_Clear(freeList);

	LIST_Clear(&HOWL_VOICE_TAKEN);

	// put them all on free list
	LIST_Init(freeList, &HOWL_VOICE_POOL[0].item, sizeof(struct VoicelineItem), 8);

	Voiceline_ClearTimeStamp();
}

void Voiceline_StopAll(void)
{
	struct Item *voiceLine;

	while ((voiceLine = HOWL_VOICE_TAKEN.last) != 0)
	{
		LIST_RemoveMember(&HOWL_VOICE_TAKEN, voiceLine);
		LIST_AddFront(&HOWL_VOICE_FREE, voiceLine);
	}
}

void Voiceline_ToggleEnable(int toggle)
{
	// if this is disabling
	if (toggle == 0)
	{
		sdata->voicelineCooldown = 0;
		// NOTE(aalhendi): Retail clears the cooldown before stopping voices.
		CTR_PSX_MEMORY_BARRIER();

		Voiceline_StopAll();
	}
	HOWL_VOICE_ENABLED = toggle;
}

static inline u32 Voiceline_RequestPlay_NextAudioRNG(void)
{
	u32 rng = ((HOWL_VOICE_RNG >> 3) + HOWL_VOICE_RNG * 0x20000000) * 5 + 1;
	HOWL_VOICE_RNG = rng;
	return rng;
}

void Voiceline_RequestPlay(u32 voiceID, u32 characterID, u32 characterID2)
{
	// NOTE(aalhendi): Retail keeps the type and decision flags in the argument registers.
	register u32 voiceType CTR_PSX_REGISTER("$6");
	u32 elapsedFrames;
	register u32 canImmediate CTR_PSX_REGISTER("$5");
	register u32 canQueue CTR_PSX_REGISTER("$4");
	struct Item *item;

	if (voiceID >= 0x18)
	{
		return;
	}

	if (characterID >= 0x10)
	{
		return;
	}

	if (characterID2 >= 0x11)
	{
		return;
	}

	if ((GAME_TRACKER->gameMode1 & END_OF_RACE) != 0)
	{
		return;
	}

	voiceType = HOWL_VOICE_TYPES[voiceID];

	if ((s32)voiceID >= 8)
	{
		u32 alreadyPlayed = HOWL_VOICE_TIMES1[characterID] & (1u << voiceID);
		u32 rng;

		// NOTE(aalhendi): Keep separate RNG stores for the two retail masking branches.
		if (alreadyPlayed != 0)
		{
			rng = Voiceline_RequestPlay_NextAudioRNG();
			CTR_PSX_MEMORY_BARRIER();
			rng &= 7;
		}
		else
		{
			rng = Voiceline_RequestPlay_NextAudioRNG();
			CTR_PSX_MEMORY_BARRIER();
			rng &= 3;
		}

		if (rng != 0)
		{
			return;
		}
	}

	elapsedFrames = (u32)GAME_TRACKER->frameTimer_MainFrame_ResetDB - (u32)HOWL_VOICE_TIMES2[characterID];
	canImmediate = 0;
	if (elapsedFrames >= 0x3d)
	{
		canImmediate = voiceType < 2;
	}

	if (HOWL_VOICE_ENABLED == 0 || (sdata->voicelineCooldown != 0 && ((u8 *)HOWL_VOICE_BACKUP)[0xa] == characterID) || elapsedFrames < 0x3c)
	{
		canQueue = 0;
	}
	else
	{
		canQueue = 1;
	}

	if (canQueue == 0 && canImmediate == 0)
	{
		return;
	}
	if (canQueue != 0 && canImmediate != 0)
	{
		u32 rng = Voiceline_RequestPlay_NextAudioRNG();
		canImmediate = 0;
		if ((rng & 1) != 0)
		{
			// NOTE(aalhendi): Keep the zero assignment in the RNG branch delay slot.
			CTR_PSX_FORGET_VALUE(canImmediate);
			canQueue = 0;
			goto playImmediate;
		}
	}
	if (canImmediate == 0)
	{
		goto queueVoiceline;
	}

playImmediate:
	// NOTE(aalhendi): Retain the decision flags until the immediate playback branch.
	CTR_PSX_OBSERVE_VALUE(canQueue);
	if (voiceType == 0)
	{
		OtherFX_Play((characterID + 0x1c) & 0xffff, 2);
	}
	else if (voiceType == 1)
	{
		OtherFX_Play((characterID + 0x2c) & 0xffff, 2);
	}

	HOWL_VOICE_TIMES2[characterID] = GAME_TRACKER->frameTimer_MainFrame_ResetDB;
	return;

queueVoiceline:
	if (canQueue == 0)
	{
		return;
	}

	HOWL_VOICE_TIMES1[characterID] |= 1u << voiceID;

	for (item = HOWL_VOICE_TAKEN.first; item != NULL;)
	{
		// NOTE(aalhendi): Retail shares this traversal value with the parameter-store register.
		register struct VoicelineItem *voiceLine CTR_PSX_REGISTER("$5") = (struct VoicelineItem *)item;
		CTR_PSX_KEEP_VALUE_RELAXED(voiceLine);

		if ((voiceID == (u32)voiceLine->voiceID) && (characterID == voiceLine->characterID))
		{
			return;
		}
		item = voiceLine->item.next;
	}

	// NOTE(aalhendi): Separate insertion paths preserve the retail address scheduling.
	item = HOWL_VOICE_FREE.first;
	if (item != NULL)
	{
		LIST_RemoveMember(&HOWL_VOICE_FREE, item);
		LIST_AddFront(&HOWL_VOICE_TAKEN, item);
	}
	else
	{
		struct LinkedList *taken = &HOWL_VOICE_TAKEN;
		item = taken->last;
		if (item != NULL)
		{
			LIST_RemoveMember(taken, item);
			// NOTE(aalhendi): Keep the recycled list address live through removal.
			CTR_PSX_OBSERVE_VALUE(taken);
			LIST_AddFront(&HOWL_VOICE_TAKEN, item);
		}
		else
		{
			goto setParams;
		}
	}

setParams:
{
	// NOTE(aalhendi): Retail writes all parameters through the argument register.
	register struct VoicelineItem *voiceLine CTR_PSX_REGISTER("$5") = (struct VoicelineItem *)item;
	CTR_PSX_KEEP_VALUE_RELAXED(voiceLine);

	voiceLine->characterID = characterID;
	voiceLine->secondaryCharacterID = characterID2;
	voiceLine->voiceID = voiceID;
	voiceLine->startFrame = GAME_TRACKER->timer;
}
}

void Voiceline_StartPlay(struct Item *voiceLine)
{
	struct VoicelineItem *voiceLineItem = (struct VoicelineItem *)voiceLine;
	struct GameTracker *gGT = GAME_TRACKER;
	u32 voiceSetIndex;
	__typeof__(data.voiceData[0].voiceSet[0]) *voiceSet;
	u32 voiceIndex;
	u32 numVoiceIDs;
	*(struct VoicelineBackup *)HOWL_VOICE_BACKUP = *(struct VoicelineBackup *)voiceLine;
	if (IS_BOSS_RACE(gGT->gameMode1) && (u32)((u16)voiceLineItem->voiceID - 10) < 6 &&
	    ((u32)(voiceLineItem->characterID - 8) < 4 || voiceLineItem->characterID == 15))
	{
		voiceSetIndex = (Voiceline_RequestPlay_NextAudioRNG() & 3) + 4;
	}
	else
	{
		voiceSetIndex = HOWL_VOICE_TYPES[voiceLineItem->voiceID];
	}
	// NOTE(aalhendi): Keep this entry across XAPlay, then reread its sample pointer.
	voiceSet = (__typeof__(voiceSet))HOWL_VOICE_SETS[voiceLineItem->characterID] + voiceSetIndex;
	numVoiceIDs = (u16)voiceSet->num;
	if (numVoiceIDs != 0)
	{
		voiceIndex = Voiceline_RequestPlay_NextAudioRNG() % numVoiceIDs;
		if (CDSYS_XAPlay(CDSYS_XA_TYPE_GAME, ((u16 *)voiceSet->ptr)[voiceIndex]) != 0)
		{
			sdata->voicelineCooldown = CDSYS_XAGetTrackLength(CDSYS_XA_TYPE_GAME, ((u16 *)voiceSet->ptr)[voiceIndex]) / 5 + 30;
			// NOTE(aalhendi): Keep the cooldown store before the exit jump.
			CTR_PSX_MEMORY_BARRIER();
		}
		else
		{
			sdata->voicelineCooldown = 30;
			CTR_PSX_MEMORY_BARRIER();
		}
	}
	else
	{
		Voiceline_StopAll();
	}
}

void Voiceline_Update(void)
{
	if (HOWL_VOICE_ENABLED == 0)
	{
		return;
	}

	{
		// NOTE(aalhendi): Preserve the signed check and unsigned decrement loads.
		s32 oldCooldown = sdata->voicelineCooldown;
		u16 cooldownBits;
		cooldownBits = (u16)sdata->voicelineCooldown;
		if (oldCooldown != 0)
		{
			s16 cooldown = cooldownBits - 1;
			sdata->voicelineCooldown = cooldown;
			if (cooldown != 0)
			{
				return;
			}
		}
	}
	if (GAME_XA_STATE != 0)
	{
		return;
	}
	if (sdata->boolCanPlayWrongWaySFX != 0)
	{
		if (GAME_WRONG_WAY_DIRECTION != 0 && GAME_SAME_DIRECTION_FRAMES > 30)
		{
			struct GameTracker *gGT = GAME_TRACKER;
			sdata->boolCanPlayWrongWaySFX = 0;
			if (gGT->numPlyrCurrGame == 1)
			{
				// NOTE(aalhendi): Retail tests the low halfword of the result.
				if ((s16)VehPickupItem_MaskBoolGoodGuy(gGT->drivers[0]) != 0)
				{
					if (CDSYS_XAPlay(CDSYS_XA_TYPE_EXTRA, 30) == 0)
					{
						goto cooldown;
					}
					sdata->voicelineCooldown = CDSYS_XAGetTrackLength(CDSYS_XA_TYPE_EXTRA, 30) / 5 + 30;
				}
				else
				{
					if (CDSYS_XAPlay(CDSYS_XA_TYPE_EXTRA, 61) == 0)
					{
						goto cooldown;
					}
					sdata->voicelineCooldown = CDSYS_XAGetTrackLength(CDSYS_XA_TYPE_EXTRA, 61) / 5 + 30;
				}
				CTR_PSX_MEMORY_BARRIER();
				return;
			cooldown:
				sdata->voicelineCooldown = 30;
				CTR_PSX_MEMORY_BARRIER();
				return;
			}
		}
		if (sdata->boolCanPlayWrongWaySFX != 0)
		{
			goto playQueuedVoice;
		}
	}
	if (GAME_WRONG_WAY_DIRECTION == 0 && GAME_SAME_DIRECTION_FRAMES > 30)
	{
		sdata->boolCanPlayWrongWaySFX = 1;
	}
playQueuedVoice:
	if (HOWL_VOICE_TAKEN.first != NULL)
	{
		struct Item *first = HOWL_VOICE_TAKEN.first;
		LIST_RemoveMember(&HOWL_VOICE_TAKEN, first);
		LIST_AddBack(&HOWL_VOICE_FREE, first);
		Voiceline_StartPlay(first);
	}
}

void Voiceline_EmptyFunc(void)
{
}

void Voiceline_SetDefaults(void)
{
	sdata->audioState = AUDIO_NONE;
	sdata->desiredXA_FinalLapIndex = 0;
	sdata->desiredXA_RaceIntroIndex = 0;

	GAME_WRONG_WAY_DIRECTION = false;

	GAME_SAME_DIRECTION_FRAMES = 0;
	sdata->nTropyVoiceCount = 0;
	sdata->boolNeedXASeek = 0;
	// NOTE(aalhendi): Retail stores the seek flag before calling music defaults.
	CTR_PSX_MEMORY_BARRIER();

	Music_SetDefaults();
}
