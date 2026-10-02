#include <common.h>

// NOTE(aalhendi): The resident unit binds the initial music-volume byte as a small-data scalar.
#ifndef HOWL_CSEQ_MUSIC_VOLUME_INITIAL
#define HOWL_CSEQ_MUSIC_VOLUME_INITIAL (sdata->vol_Music)
#endif

void cseq_opcode01_noteoff(struct SongSeq *seq)
{
	struct ChannelStats *curr, *backupNext;
	u8 *currNote = seq->currNote;

	for (curr = (struct ChannelStats *)GAME_CHANNEL_TAKEN.first; curr != NULL; curr = backupNext)
	{
		backupNext = curr->link.links.next;

		if (curr->type != HOWL_CHANNEL_TYPE_MUSIC)
		{
			continue;
		}

		// only update this sequence's channels
		if (curr->soundID != seq->soundID)
		{
			continue;
		}

		// need a struct for Note offset 0x1
		if (curr->drumIndex_pitchIndex != currNote[1])
		{
			continue;
		}

		// enable OFF flag, disable ON flag
		GAME_CHANNEL_UPDATE_FLAGS[curr->channelID] |= HOWL_CHANNEL_UPDATE_OFF;
		// NOTE(aalhendi): Retail rereads the word for the second flag update.
		*(volatile u32 *)&GAME_CHANNEL_UPDATE_FLAGS[curr->channelID] &= ~HOWL_CHANNEL_UPDATE_KEY_ON;

		curr->flags &= (u8)~1;

		// recycle: remove from taken, put on free
		LIST_RemoveMember(&GAME_CHANNEL_TAKEN, (struct Item *)curr);
		LIST_AddBack(&GAME_CHANNEL_FREE, (struct Item *)curr);
	}
}

void cseq_opcode02_empty(struct SongSeq *seq)
{
	(void)seq;
	// left empty by ND
}

// "end of song" opcode
void cseq_opcode03(struct SongSeq *seq)
{
	// if song loops
	if ((seq->flags & 2) != 0)
	{
		seq->flags |= 8;
	}

	// if song does not loop
	else
	{
		SongPool_StopAllCseq(&GAME_SONG_POOL[seq->songPoolIndex]);
	}
}

void cseq_opcode04_empty(struct SongSeq *seq)
{
	(void)seq;
	// left empty by ND
}

void howl_InitChannelAttr_Music(struct SongSeq *seq, struct ChannelAttr *attr, int index, int channelVol)
{
	// NOTE(aalhendi): Keep volume and intermediate products in their retail registers.
	register s32 volume CTR_PSX_REGISTER("$20") = channelVol;
	u32 pitch;
	struct Song *song = &GAME_SONG_POOL[seq->songPoolIndex];
	u32 finalVolume;
	register u32 gain CTR_PSX_REGISTER("$8");

	if ((seq->flags & 4) != 0)
	{
		struct SampleDrums *shortSample = &sdata->ptrCseqShortSamples[index];
		register s32 mix CTR_PSX_REGISTER("$5");
		s32 sampleVol;
		u32 ad;
		register u32 level CTR_PSX_REGISTER("$2");
		register u32 master CTR_PSX_REGISTER("$3");

		if (seq->distort != HOWL_SFX_DISTORTION_NONE)
		{
			u32 rawPitch = (u16)shortSample->pitch;
			u32 coefficient = GAME_DISTORT_CONST_OTHER_FX[seq->distort];
			// NOTE(aalhendi): Keep the coefficient separate from the multiplication result.
			CTR_PSX_BIND_VALUE_CLOBBER(coefficient, "$8");
			gain = rawPitch * coefficient;
			pitch = gain >> 16;
		}
		else
		{
			pitch = (u16)shortSample->pitch;
		}

		attr->spuStartAddr = (void *)(sdata->howl_spuAddrs[(u16)shortSample->spuIndex].spuAddr << 3);
		ad = 0x80ff;
		attr->ad = ad;
		attr->sr = 0x1fc2;
		master = sdata->vol_Music;
		level = song->vol_Curr;
		gain = master * level;
		level = seq->vol_Curr;
		gain = gain * level;
		// NOTE(aalhendi): Sample its volume after the sequence gain product.
		CTR_PSX_DEPEND_VALUE(shortSample, gain);
		sampleVol = shortSample->volume;
		mix = CTR_MipsSra(gain, 10);

		gain = (u32)mix * (u32)sampleVol;
		gain = gain * (u32)volume;
	}
	else
	{
		struct SampleInstrument *longSample = &sdata->ptrCseqLongSamples[seq->instrumentID];

		u32 result;
		u32 sampleAddr;
		register u32 master CTR_PSX_REGISTER("$4");
		register s32 mix CTR_PSX_REGISTER("$5");
		s32 sampleVol;
		register u32 level CTR_PSX_REGISTER("$3");

		// NOTE(aalhendi): The pitch call precedes volume sampling.
		result = howl_InstrumentPitch(longSample->basePitch, index, seq->distort);
		sampleAddr = sdata->howl_spuAddrs[(u16)longSample->spuIndex].spuAddr;
		master = sdata->vol_Music;
		attr->spuStartAddr = (void *)(sampleAddr << 3);
		attr->ad = longSample->ad;
		attr->sr = longSample->sr;
		level = song->vol_Curr;
		gain = master * level;
		level = seq->vol_Curr;
		gain = gain * level;
		// NOTE(aalhendi): Sample its volume after the sequence gain product.
		CTR_PSX_DEPEND_VALUE(longSample, gain);
		sampleVol = longSample->volume;
		mix = CTR_MipsSra(gain, 10);

		gain = (u32)mix * (u32)sampleVol;
		gain = gain * (u32)volume;
		// NOTE(aalhendi): Retain volume through its final multiply before truncating pitch.
		CTR_PSX_KEEP_VALUE_RELAXED(volume);
		CTR_PSX_DEPEND_VALUE(result, volume);
		pitch = (u16)result;
	}

	finalVolume = gain >> 15;
	// NOTE(aalhendi): Form the shifted gain before sampling LR.
	CTR_PSX_OBSERVE_VALUE(finalVolume);
	Channel_SetVolume(attr, finalVolume, seq->LR);
	attr->pitch = pitch;
	attr->reverb = seq->reverb;
}

// change volume
void cseq_opcode_from06and07(struct SongSeq *seq)
{
	struct ChannelStats *curr;

	for (curr = (struct ChannelStats *)GAME_CHANNEL_TAKEN.first; curr != NULL; curr = (struct ChannelStats *)curr->link.links.next)
	{
		int sampleVol;

		if (curr->type != HOWL_CHANNEL_TYPE_MUSIC)
		{
			continue;
		}

		// only update this sequence's channels
		if (curr->soundID != seq->soundID)
		{
			continue;
		}

		// NOTE(aalhendi): Retail samples the current song volume for each matching channel.
		sampleVol = CTR_MipsMulLo(CTR_MipsMulLo(sdata->vol_Music, GAME_SONG_POOL[seq->songPoolIndex].vol_Curr), seq->vol_Curr);
		Channel_SetVolume(&GAME_CHANNEL_ATTR_NEW[curr->channelID], CTR_MipsSra(CTR_MipsMulLo(sampleVol, curr->vol), 18), seq->LR);

		// update volume
		GAME_CHANNEL_UPDATE_FLAGS[curr->channelID] |= HOWL_CHANNEL_UPDATE_VOLUME;
	}
}

void cseq_opcode05_noteon(struct SongSeq *seq)
{
	u8 *currNote;
	s32 songIndex;
	s32 songOffset;
	struct ChannelStats *stats;
	struct ChannelAttr attr;

	if (HOWL_CSEQ_MUSIC_VOLUME_INITIAL == 0)
	{
		return;
	}
	if (seq->vol_Curr == 0)
	{
		return;
	}
	songIndex = seq->songPoolIndex;
	// NOTE(aalhendi): Keep the song stride separate from its base for retail address scheduling.
	songOffset = songIndex * sizeof(struct Song);
	if (((struct Song *)((u8 *)GAME_SONG_POOL + songOffset))->vol_Curr == 0)
	{
		return;
	}

	currNote = seq->currNote;

	howl_InitChannelAttr_Music(seq, &attr, currNote[1], currNote[2]);

	stats = Channel_AllocSlot(HOWL_CHANNEL_UPDATE_ALL_ATTRS, &attr);

	if (stats == 0)
	{
		return;
	}

	stats->flags |= 0xe;

	stats->unk1 = seq->unk;
	stats->type = HOWL_CHANNEL_TYPE_MUSIC;
	stats->unk2 = 0;

	// echo and reverb is same thing, needs rename
	stats->echo = seq->reverb;

	stats->vol = currNote[2];

	stats->distort = seq->distort;
	stats->LR = seq->LR;
	stats->drumIndex_pitchIndex = currNote[1];
	stats->timeLeft = 0;
	stats->soundID = seq->soundID;

	seq->unk0A = (u8)CTR_MipsAddLo(seq->unk0A, 1);
}

void cseq_opcode06(struct SongSeq *seq)
{
	u8 *note = seq->currNote;
	seq->vol_Curr = note[1];
	cseq_opcode_from06and07(seq);
}

void cseq_opcode07(struct SongSeq *seq)
{
	u8 *note = seq->currNote;
	seq->LR = note[1];
	cseq_opcode_from06and07(seq);
}

void cseq_opcode08(struct SongSeq *seq)
{
	struct ChannelStats *curr;
	u8 *currNote = seq->currNote;

	for (curr = (struct ChannelStats *)GAME_CHANNEL_TAKEN.first; curr != NULL; curr = (struct ChannelStats *)curr->link.links.next)
	{
		if (curr->type != HOWL_CHANNEL_TYPE_MUSIC)
		{
			continue;
		}

		// only update this sequence's channels
		if (curr->soundID != seq->soundID)
		{
			continue;
		}

		// set reverb
		GAME_CHANNEL_ATTR_NEW[curr->channelID].reverb = currNote[1];

		// update Reverb (reverberation = echo)
		GAME_CHANNEL_UPDATE_FLAGS[curr->channelID] |= HOWL_CHANNEL_UPDATE_REVERB;
	}
}

void cseq_opcode09(struct SongSeq *seq)
{
	u8 *currNote = seq->currNote;
	seq->instrumentID = currNote[1];
}

void cseq_opcode0a(struct SongSeq *seq)
{
	struct ChannelStats *curr;

	u8 *currNote = seq->currNote;
	seq->distort = currNote[1];

	for (curr = (struct ChannelStats *)GAME_CHANNEL_TAKEN.first; curr != NULL; curr = (struct ChannelStats *)curr->link.links.next)
	{
		if (curr->type != HOWL_CHANNEL_TYPE_MUSIC)
		{
			continue;
		}

		// only update this sequence's channels
		if (curr->soundID != seq->soundID)
		{
			continue;
		}

		// drums
		if ((seq->flags & 4) != 0)
		{
			struct SampleDrums *shortSample = &sdata->ptrCseqShortSamples[curr->drumIndex_pitchIndex];

			if (seq->distort != HOWL_SFX_DISTORTION_NONE)
			{
				// NOTE(aalhendi): Resolve the destination before the table lookup for retail page order.
				struct ChannelAttr *attr = &GAME_CHANNEL_ATTR_NEW[curr->channelID];
				attr->pitch = CTR_MipsSrl(CTR_MipsMulLo((u16)shortSample->pitch, GAME_DISTORT_CONST_OTHER_FX[seq->distort]), 16);
			}
			else
			{
				GAME_CHANNEL_ATTR_NEW[curr->channelID].pitch = (u16)shortSample->pitch;
			}
		}

		// instrument
		else
		{
			struct SampleInstrument *longSample = &sdata->ptrCseqLongSamples[seq->instrumentID];
			GAME_CHANNEL_ATTR_NEW[curr->channelID].pitch = howl_InstrumentPitch(longSample->basePitch, curr->drumIndex_pitchIndex, seq->distort);
		}

		// update pitch
		GAME_CHANNEL_UPDATE_FLAGS[curr->channelID] |= HOWL_CHANNEL_UPDATE_PITCH;
	}
}
