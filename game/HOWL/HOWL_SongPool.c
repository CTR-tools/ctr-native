#include <common.h>

// Returns the CseqSongHeader for the given song ID from the parsed song data buffer.
static struct CseqSongHeader *GetCseqSongHeader(u16 songID)
{
	return (struct CseqSongHeader *)&sdata->ptrCseqSongData[sdata->ptrCseqSongStartOffset[songID]];
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002a63c-0x8002a678
struct SongSeq *SongPool_FindFreeChannel(void)
{
	struct SongSeq *seq;

	for (seq = &sdata->songSeq[0]; seq < &sdata->songSeq[NUM_SFX_CHANNELS]; seq++)
	{
		// if seq is not playing
		if ((seq->flags & 1) == 0)
		{
			return seq;
		}
	}

	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002a678-0x8002a6cc
u32 SongPool_CalculateTempo(s16 const60, s16 tpqn, s16 bpm)
{
	s32 ticksPerMinute = CTR_MipsMulLo(bpm, tpqn);
	u32 ticksPerSecond = CTR_MipsMulHiU((u32)ticksPerMinute, 0x88888889u) >> 5;

	return CTR_MipsDivU((u32)CTR_MipsSll((s32)ticksPerSecond, 16), (u32)(s32)const60);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002a6cc-0x8002a730
void SongPool_ChangeTempo(struct Song *song, s16 deltaBPM)
{
	struct CseqSongHeader *csh = GetCseqSongHeader(song->id);

	song->bpm = (s16)CTR_MipsAddLo((u16)csh->bpm, deltaBPM);

	song->tempo = SongPool_CalculateTempo(60, song->tpqn, song->bpm);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002a730-0x8002a9d8
void SongPool_Start(struct Song *song, u16 songID, s16 deltaBPM, b32 boolLoopAtEnd, struct SongSet *songSet, int songSetActiveBits)
{
	int i;
	int vol;
	u8 numSeqs;
	struct SongSeq *seqCurr;
	struct CseqSongHeader *csh;
	char *cnhFirst;
	struct SongNoteHeader *cnhCurr;

	// now playing
	song->flags = 1;
	song->id = songID;

	csh = GetCseqSongHeader(songID);
	numSeqs = csh->numSeqs;

	// advHub
	if (songSet != 0)
	{
		if (songSet->numSeqs != numSeqs)
		{
			return;
		}

		song->songSetActiveBits = songSetActiveBits;
	}

	song->tpqn = csh->tpqn;
	song->bpm = (s16)CTR_MipsAddLo((u16)csh->bpm, deltaBPM);

	song->tempo = SongPool_CalculateTempo(60, song->tpqn, song->bpm);

	song->unk10 = 0;
	song->timeSpentPlaying = 0;

	if ((u16)(songID - 1) < 2)
	{
		// full volume
		vol = 0xff;
	}

	else
	{
		// partial volume
		vol = 0xbe;
	}

	song->vol_Curr = vol;
	song->vol_New = vol;

	// Naughty Dog Box
	if (sdata->gGT->levelID == NAUGHTY_DOG_CRATE)
	{
		song->vol_Curr = 0xbe;
		song->vol_New = 0xbe;
	}

	song->vol_StepRate = 1;
	song->numSequences = 0;

	u16 *seqOffsetArr = (u16 *)SONGHEADER_GETSEQOFFARR(csh);

	// first note header comes after end of CseqSongHeader
	// and the full array of seqOffsets within the header
	cnhFirst = (char *)&seqOffsetArr[numSeqs];

	// align up by 4
	if (((uintptr_t)cnhFirst & 1) != 0)
	{
		cnhFirst += 1;
	}
	if (((uintptr_t)cnhFirst & 2) != 0)
	{
		cnhFirst += 2;
	}

	for (i = 0; i < numSeqs; i++)
	{
		cnhCurr = (struct SongNoteHeader *)&cnhFirst[seqOffsetArr[i]];

		seqCurr = SongPool_FindFreeChannel();
		if (seqCurr == NULL)
		{
			continue;
		}

		// now playing
		seqCurr->flags = 1;

		if ((cnhCurr->flags & 1) != 0)
		{
			// playing, and instrument/drums
			seqCurr->flags = 5;
		}

		if (boolLoopAtEnd != 0)
		{
			seqCurr->flags |= 2;
		}

		seqCurr->unk = cnhCurr->unk;

		seqCurr->instrumentID = 0;
		seqCurr->reverb = 0;

		// by default, have volume
		seqCurr->vol_Curr = 0xFF;
		seqCurr->vol_New = 0xFF;

		// songSet (advHub)
		if (songSet != 0)
		{
			// if seq is not in current songSet
			if ((songSet->ptrSongSetBits[i] & song->songSetActiveBits) == 0)
			{
				// disable the volume from the song
				seqCurr->vol_Curr = 0;
				seqCurr->vol_New = 0;
			}
		}

		seqCurr->vol_StepRate = 1;

		seqCurr->distort = 0x80;
		seqCurr->LR = 0x80;

		seqCurr->unk0A = 0;
		seqCurr->songPoolIndex = song->songPoolIndex;

		seqCurr->NoteLength = 0;
		seqCurr->NoteTimeElapsed = 0;

		seqCurr->firstNote = (u8 *)NOTEHEADER_GETNOTES(cnhCurr);

		seqCurr->currNote = howl_GetNextNote(seqCurr->firstNote, &seqCurr->NoteLength);

		song->CseqSequences[song->numSequences++] = seqCurr;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002a9d8-0x8002a9f0
void SongPool_Volume(struct Song *song, int newVol, int newStep, b32 boolImm)
{
	// if immediate change request,
	// without fading volume
	if (boolImm != 0)
	{
		song->vol_Curr = newVol;
	}

	song->vol_New = newVol;
	song->vol_StepRate = newStep;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002a9f0-0x8002aa44
void SongPool_AdvHub1(struct Song *song, int seqID, int vol, b32 boolImm)
{
	struct SongSeq *seq;

	struct CseqSongHeader *csh = GetCseqSongHeader(song->id);

	if (seqID >= (u8)csh->numSeqs)
	{
		return;
	}

	seq = song->CseqSequences[seqID];

	// if immediate change
	if (boolImm != 0)
	{
		seq->vol_Curr = vol;
	}

	seq->vol_New = vol;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002aa44-0x8002ab18
void SongPool_AdvHub2(struct Song *song, struct SongSet *songSet, int songSetActiveBits)
{
	int i;
	int vol;
	struct CseqSongHeader *csh = GetCseqSongHeader(song->id);
	u8 numSeqs = csh->numSeqs;

	// advHub
	if (songSet != 0)
	{
		if (songSet->numSeqs != numSeqs)
		{
			return;
		}

		song->songSetActiveBits = songSetActiveBits;
	}

	for (i = 0; i < numSeqs; i++)
	{
		// volume on
		vol = 0xff;

		if ((songSet->ptrSongSetBits[i] & song->songSetActiveBits) == 0)
		{
			vol = 0;
		}

		SongPool_AdvHub1(song, i, vol, 0);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002ab18-0x8002ac0c
void SongPool_StopCseq(struct SongSeq *seq)
{
	struct ChannelStats *curr, *backupNext;
	u32 *flagPtr;

	for (curr = (struct ChannelStats *)sdata->channelTaken.first; curr != NULL; curr = backupNext)
	{
		backupNext = curr->next;

		if (curr->type != HOWL_CHANNEL_TYPE_MUSIC)
		{
			continue;
		}

		if (curr->soundID != seq->soundID)
		{
			continue;
		}

		// enable OFF flag, disable ON flag
		flagPtr = &sdata->ChannelUpdateFlags[curr->channelID];
		*flagPtr |= HOWL_CHANNEL_UPDATE_OFF;
		*flagPtr &= ~HOWL_CHANNEL_UPDATE_KEY_ON;

		curr->flags &= (u8)~1;

		// recycle: remove from taken, put on free
		LIST_RemoveMember(&sdata->channelTaken, (struct Item *)curr);
		LIST_AddBack(&sdata->channelFree, (struct Item *)curr);
	}

	// not playing
	seq->flags &= ~(1);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002ac0c-0x8002ac94
void SongPool_StopAllCseq(struct Song *song)
{
	int i;

	// if song is not playing, skip
	if ((song->flags & 1) == 0)
	{
		return;
	}

	for (i = 0; i < song->numSequences; i++)
	{
		SongPool_StopCseq(song->CseqSequences[i]);
	}

	// stop song
	song->flags &= (u8)~1;
}
