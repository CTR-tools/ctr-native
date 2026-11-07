#include <common.h>

// does not really touch voiceline
void Voiceline_PoolInit(void)
{
	s32 index;

	sdata->criticalSectionCount = 0;

	sdata->numBackup_ChannelStats = 0;

	sdata->ptrCseqHeader = 0;

	Bank_ResetAllocator();

	Audio_SetDefaults();

	LIST_Clear(&sdata->channelFree);
	LIST_Clear(&sdata->channelTaken);

	LIST_Init(&sdata->channelFree, &sdata->channelStatsPrev[0].link.item, 0x20, 0x18);

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

void Voiceline_ClearTimeStamp(void)
{
	for (s32 i = 0; i < 16; i++)
	{
		// Clear audio timestamps arrays
		sdata->timeSet1[i] = 0;
		sdata->timeSet2[i] = 0;
	}
}

void Voiceline_PoolClear(void)
{
	sdata->boolCanPlayWrongWaySFX = false;

	sdata->voicelineCooldown = 0;

	sdata->boolCanPlayVoicelines = false;

	LIST_Clear(&sdata->Voiceline1);

	LIST_Clear(&sdata->Voiceline2);

	// put them all on free list
	LIST_Init(&sdata->Voiceline1, &sdata->voicelinePool[0].item, sizeof(struct VoicelineItem), 8);

	Voiceline_ClearTimeStamp();
}

void Voiceline_StopAll(void)
{
	while (sdata->Voiceline2.last != 0)
	{
		struct Item *voiceLine = sdata->Voiceline2.last;

		LIST_RemoveMember(&sdata->Voiceline2, voiceLine);
		LIST_AddFront(&sdata->Voiceline1, voiceLine);
	}
}

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
		struct VoicelineItem *voiceLine = (struct VoicelineItem *)item;

		if ((voiceID == (u32)voiceLine->voiceID) && (characterID == voiceLine->characterID))
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
		struct VoicelineItem *voiceLine = (struct VoicelineItem *)item;

		voiceLine->characterID = characterID;
		voiceLine->secondaryCharacterID = characterID2;
		voiceLine->voiceID = voiceID;
		voiceLine->startFrame = sdata->gGT->timer;
	}
}

void Voiceline_StartPlay(struct Item *voiceLine)
{
	struct VoicelineItem *voiceLineItem = (struct VoicelineItem *)voiceLine;
	u32 voiceID = (u16)voiceLineItem->voiceID;
	u32 characterID = voiceLineItem->characterID;
	u32 voiceSetIndex;

	CTR_WriteU32LE(&sdata->backupParams_FUN_8002cf28[0], CTR_ReadU32LE((u8 *)voiceLineItem + 0x0));
	CTR_WriteU32LE(&sdata->backupParams_FUN_8002cf28[1], CTR_ReadU32LE((u8 *)voiceLineItem + 0x4));
	CTR_WriteU32LE(&sdata->backupParams_FUN_8002cf28[2], CTR_ReadU32LE((u8 *)voiceLineItem + 0x8));
	CTR_WriteU32LE(&sdata->backupParams_FUN_8002cf28[3], CTR_ReadU32LE((u8 *)voiceLineItem + 0xc));

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
	u32 voiceIndex = rng % numVoiceIDs;
	u32 xaID = (u16)voiceIDs[voiceIndex];
	bool played = false;
	// PAL uses set 0 for these events, but an unavailable PAL clip must fall
	// back to the original NTSC-U set rather than to English from set 0.
	if (voiceSetIndex == 8 && g_config.voiceLanguage != 1)
	{
		u16 palCount = data.voiceData[characterID].voiceSet[0].num;
		if (palCount != 0)
		{
			s16 *palVoiceIDs = data.voiceData[characterID].voiceSet[0].ptr;
			u32 candidate = (u16)palVoiceIDs[rng % palCount];
			if (CDSYS_XAPlayLocalized(CDSYS_XA_TYPE_GAME, candidate))
			{
				xaID = candidate;
				played = true;
			}
		}
	}

	if (!played && CDSYS_XAPlay(CDSYS_XA_TYPE_GAME, xaID) == 0)
	{
		sdata->voicelineCooldown = 0x1e;
		return;
	}

	sdata->voicelineCooldown = (s16)(CDSYS_XAGetTrackLength(CDSYS_XA_TYPE_GAME, xaID) / 5) + 0x1e;
	sdata->chaosRuntime.humanXA = Chaos_IsHumanCharacter(characterID);
}

void Voiceline_PlayWrongWay(struct Driver *driver)
{
	const u32 voiceID = VehPickupItem_MaskBoolGoodGuy(driver) ? 0x1e : 0x3d;
	sdata->voicelineCooldown = 0x1e;
	if (CDSYS_XAPlay(CDSYS_XA_TYPE_EXTRA, voiceID) != 0)
	{
		sdata->chaosRuntime.humanXA = true;
		sdata->voicelineCooldown += CDSYS_XAGetTrackLength(CDSYS_XA_TYPE_EXTRA, voiceID) / 5;
	}
}

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

	if (Chaos_PlayWrongWayVoice())
	{
		return;
	}

	if (sdata->boolCanPlayWrongWaySFX != 0 && !Chaos_OverridesWrongWayWarning(gGT->drivers[0]))
	{
		if ((sdata->WrongWayDirection_bool != 0) && (sdata->framesDrivingSameDirection > 0x1e))
		{
			sdata->boolCanPlayWrongWaySFX = false;

			if (gGT->numPlyrCurrGame == 1)
			{
				Voiceline_PlayWrongWay(gGT->drivers[0]);
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
		if (Chaos_IsTimeStopped() && !Chaos_IsHumanCharacter(((struct VoicelineItem *)first)->characterID))
		{
			return;
		}

		LIST_RemoveMember(&sdata->Voiceline2, first);
		LIST_AddBack(&sdata->Voiceline1, first);
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

	sdata->WrongWayDirection_bool = false;

	sdata->framesDrivingSameDirection = 0;
	sdata->nTropyVoiceCount = 0;
	sdata->boolNeedXASeek = 0;

	Music_SetDefaults();
}
