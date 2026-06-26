#include <common.h>

// does not really touch voiceline
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002c918-0x8002caa8
void Voiceline_PoolInit(void)
{
	char index;

	sdata->criticalSectionCount = 0;

	sdata->numBackup_ChannelStats = 0;

	sdata->ptrCseqHeader = 0;

	Bank_ResetAllocator();

	Audio_SetDefaults();

	LIST_Clear(&sdata->channelFree);
	LIST_Clear(&sdata->channelTaken);

	LIST_Init(&sdata->channelFree, (struct Item *)&sdata->channelStatsPrev[0], 0x20, 0x18);

	SpuSetReverbVoice(0, 0xffffff);

	// initialize all members in sound list
	for (index = 0; index < 24; index++)
	{
		struct ChannelStats *stats = &sdata->channelStatsPrev[index];
		sdata->ChannelUpdateFlags[index] = 0;

		SpuSetVoiceADSRAttr(index, 0, 0xf, 0x7f, 2, 0xf, 5, 1, 3);

		stats->flags = 0;
		stats->channelID = index;

		stats->ad = 0x80ff;
		stats->sr = 0x1fc2;

		struct ChannelAttr *curr = &sdata->channelAttrCur[index];

		curr->spuStartAddr = (void *)-1;

		curr->ad = 0x80ff;
		curr->sr = 0x1fc2;

		curr->pitch = -1;
		curr->reverb = -1;
		curr->audioL = -1;
		curr->audioR = -1;
	}

	for (index = 0; index < 2; index++)
	{
		struct Song *pool = &sdata->songPool[index];

		// not playing
		pool->flags = 0;

		pool->songPoolIndex = index;
	}

	for (index = 0; index < 24; index++)
	{
		struct SongSeq *seq = &sdata->songSeq[index];

		// not playing
		seq->flags = 0;

		seq->soundID = index;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002caa8-0x8002cae0
void Voiceline_ClearTimeStamp(void)
{
	for (char i = 0; i < 16; i++)
	{
		// Clear audio timestamps arrays
		sdata->timeSet1[i] = 0;
		sdata->timeSet2[i] = 0;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002cae0-0x8002cb44
void Voiceline_PoolClear(void)
{
	sdata->boolCanPlayWrongWaySFX = false;

	sdata->voicelineCooldown = 0;

	sdata->boolCanPlayVoicelines = false;

	LIST_Clear(&sdata->Voiceline1);

	LIST_Clear(&sdata->Voiceline2);

	// put them all on free list
	LIST_Init(&sdata->Voiceline1, (struct Item *)&sdata->voicelinePool[0].next, 0x10, 8);

	Voiceline_ClearTimeStamp();
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002cb44-0x8002cbb4
void Voiceline_StopAll(void)
{
	while (sdata->Voiceline2.last != 0)
	{
		struct Item *voiceLine = sdata->Voiceline2.last;

		LIST_RemoveMember(&sdata->Voiceline2, voiceLine);
		LIST_AddFront(&sdata->Voiceline1, voiceLine);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002cbb4-0x8002cbe8
void Voiceline_ToggleEnable(int toggle)
{
	// if this is disabling
	if (toggle == 0)
	{
		sdata->voicelineCooldown = 0;

		Voiceline_StopAll();
	}
	sdata->boolCanPlayVoicelines = toggle;
}

static u32 Voiceline_RequestPlay_NextAudioRNG(void)
{
	sdata->audioRNG = ((sdata->audioRNG >> 3) + sdata->audioRNG * 0x20000000) * 5 + 1;
	return sdata->audioRNG;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002cbe8-0x8002cf28
void Voiceline_RequestPlay(u32 voiceID, u32 characterID, u32 characterID2)
{
	u8 voiceType;
	u32 elapsedFrames;
	u32 canImmediate;
	u32 canQueue;

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

	if ((sdata->gGT->gameMode1 & END_OF_RACE) != 0)
	{
		return;
	}

	voiceType = data.voiceID[voiceID];

	if ((s32)voiceID >= 8)
	{
		u32 alreadyPlayed = sdata->timeSet1[characterID] & (1 << (voiceID & 0x1f));
		u32 rng = Voiceline_RequestPlay_NextAudioRNG();

		if (alreadyPlayed != 0)
		{
			rng &= 7;
		}
		else
		{
			rng &= 3;
		}

		if (rng != 0)
		{
			return;
		}
	}

	elapsedFrames = (u32)CTR_MipsSubLo(sdata->gGT->frameTimer_MainFrame_ResetDB, sdata->timeSet2[characterID]);
	canImmediate = 0;
	if (elapsedFrames >= 0x3d)
	{
		canImmediate = voiceType < 2;
	}

	canQueue = 1;
	if (sdata->boolCanPlayVoicelines == 0)
	{
		canQueue = 0;
	}
	else if ((sdata->voicelineCooldown != 0) && (((u8 *)sdata->backupParams_FUN_8002cf28)[0xa] == characterID))
	{
		canQueue = 0;
	}
	else if (elapsedFrames < 0x3c)
	{
		canQueue = 0;
	}

	if (canQueue != 0)
	{
		if (canImmediate != 0)
		{
			u32 rng = Voiceline_RequestPlay_NextAudioRNG();

			canImmediate = 0;
			if ((rng & 1) != 0)
			{
				canQueue = 0;
				goto playImmediate;
			}
		}
	}
	else
	{
		if (canImmediate == 0)
		{
			return;
		}
	}

	if (canImmediate == 0)
	{
		goto queueVoiceline;
	}

playImmediate:
	if (voiceType == 0)
	{
		OtherFX_Play((characterID + 0x1c) & 0xffff, 2);
	}
	else if (voiceType == 1)
	{
		OtherFX_Play((characterID + 0x2c) & 0xffff, 2);
	}

	sdata->timeSet2[characterID] = sdata->gGT->frameTimer_MainFrame_ResetDB;
	return;

queueVoiceline:
	if (canQueue == 0)
	{
		return;
	}

	sdata->timeSet1[characterID] |= 1 << (voiceID & 0x1f);

	for (struct Item *item = sdata->Voiceline2.first; item != NULL; item = item->next)
	{
		u8 *itemBytes = (u8 *)item;

		if ((voiceID == (u32) * (s16 *)(itemBytes + 8)) && (characterID == itemBytes[0xa]))
		{
			return;
		}
	}

	struct Item *item = sdata->Voiceline1.first;
	if (item != NULL)
	{
		LIST_RemoveMember(&sdata->Voiceline1, item);
	}
	else
	{
		item = sdata->Voiceline2.last;
		if (item != NULL)
		{
			LIST_RemoveMember(&sdata->Voiceline2, item);
		}
	}

	LIST_AddFront(&sdata->Voiceline2, item);

	{
		u8 *itemBytes = (u8 *)item;

		itemBytes[0xa] = characterID;
		itemBytes[0xb] = characterID2;
		*(s16 *)(itemBytes + 8) = voiceID;
		*(s32 *)(itemBytes + 0xc) = sdata->gGT->timer;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002cf28-0x8002d0f8
void Voiceline_StartPlay(struct Item *voiceLine)
{
	u8 *voiceLineBytes = (u8 *)voiceLine;
	int *voiceLineWords = (int *)voiceLine;
	u32 voiceID = *(u16 *)(voiceLineBytes + 8);
	u32 characterID = voiceLineBytes[0xa];
	u32 voiceSetIndex;

	sdata->backupParams_FUN_8002cf28[0] = voiceLineWords[0];
	sdata->backupParams_FUN_8002cf28[1] = voiceLineWords[1];
	sdata->backupParams_FUN_8002cf28[2] = voiceLineWords[2];
	sdata->backupParams_FUN_8002cf28[3] = voiceLineWords[3];

	if ((IS_BOSS_RACE(sdata->gGT->gameMode1)) && ((u32)(voiceID - 10) < 6) && (((u32)(characterID - 8) < 4) || (characterID == 0xf)))
	{
		u32 rng = Voiceline_RequestPlay_NextAudioRNG();
		voiceSetIndex = (rng & 3) + 4;
	}
	else
	{
		voiceSetIndex = data.voiceID[(s16)voiceID];
	}

	s16 *voiceIDs = data.voiceData[characterID].voiceSet[voiceSetIndex].ptr;
	u16 numVoiceIDs = data.voiceData[characterID].voiceSet[voiceSetIndex].num;

	if (numVoiceIDs == 0)
	{
		Voiceline_StopAll();
		return;
	}

	u32 rng = Voiceline_RequestPlay_NextAudioRNG();
	u32 voiceIndex = (rng % numVoiceIDs) * 2;
	u32 xaID = *(u16 *)((u8 *)voiceIDs + voiceIndex);

	if (CDSYS_XAPlay(CDSYS_XA_TYPE_GAME, xaID) == 0)
	{
		sdata->voicelineCooldown = 0x1e;
		return;
	}

	sdata->voicelineCooldown = (s16)(CDSYS_XAGetTrackLength(CDSYS_XA_TYPE_GAME, xaID) / 5) + 0x1e;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002d0f8-0x8002d2a8
void Voiceline_Update(void)
{
	struct GameTracker *gGT = sdata->gGT;

	if (sdata->boolCanPlayVoicelines == 0)
	{
		return;
	}

	if (sdata->voicelineCooldown != 0)
	{
		sdata->voicelineCooldown = (s16)CTR_MipsSubLo((u16)sdata->voicelineCooldown, 1);
		if (sdata->voicelineCooldown != 0)
		{
			return;
		}
	}

	if (sdata->XA_State != 0)
	{
		return;
	}

	if (sdata->boolCanPlayWrongWaySFX != 0)
	{
		if ((sdata->WrongWayDirection_bool != 0) && (sdata->framesDrivingSameDirection > 0x1e))
		{
			u32 voiceID;

			sdata->boolCanPlayWrongWaySFX = false;

			if (gGT->numPlyrCurrGame == 1)
			{
				if ((VehPickupItem_MaskBoolGoodGuy(gGT->drivers[0]) & 0xffff) == 0)
				{
					voiceID = 0x3d;
				}
				else
				{
					voiceID = 0x1e;
				}

				if (CDSYS_XAPlay(CDSYS_XA_TYPE_EXTRA, voiceID) == 0)
				{
					sdata->voicelineCooldown = 0x1e;
					return;
				}

				sdata->voicelineCooldown = (s16)(CDSYS_XAGetTrackLength(CDSYS_XA_TYPE_EXTRA, voiceID) / 5) + 0x1e;
				return;
			}
		}

		if (sdata->boolCanPlayWrongWaySFX != 0)
		{
			goto playQueuedVoice;
		}
	}

	if ((sdata->WrongWayDirection_bool == 0) && (sdata->framesDrivingSameDirection > 0x1e))
	{
		sdata->boolCanPlayWrongWaySFX = true;
	}

playQueuedVoice:
	if (sdata->Voiceline2.first != NULL)
	{
		struct Item *first = sdata->Voiceline2.first;

		LIST_RemoveMember(&sdata->Voiceline2, first);
		LIST_AddBack(&sdata->Voiceline1, first);
		Voiceline_StartPlay(first);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002d2a8-0x8002d2b0
void Voiceline_EmptyFunc(void)
{
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002d2b0-0x8002d2f4
void Voiceline_SetDefaults(void)
{
	sdata->audioState = AUDIO_NONE;
	sdata->desiredXA_RaceIntroIndex = 0;

	sdata->WrongWayDirection_bool = false;

	sdata->framesDrivingSameDirection = 0;
	sdata->nTropyVoiceCount = 0;
	sdata->boolNeedXASeek = 0;

	Music_SetDefaults();
}
