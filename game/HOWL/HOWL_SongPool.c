#include <common.h>

// NOTE(aalhendi): Retail reads the song offset unsigned before adding it to the data base.
static inline struct CseqSongHeader *GetCseqSongHeader(u16 songID)
{
	return (struct CseqSongHeader *)&sdata->ptrCseqSongData[(u16)sdata->ptrCseqSongStartOffset[songID]];
}

struct SongSeq *SongPool_FindFreeChannel(void)
{
	struct SongSeq *seq;
	int i;

	for (i = 0; i < NUM_SFX_CHANNELS; i++)
	{
		seq = &GAME_SONG_SEQUENCES[i];
		// if seq is not playing
		if ((seq->flags & 1) == 0)
		{
			return seq;
		}
	}

	return 0;
}

u32 SongPool_CalculateTempo(s16 const60, s16 tpqn, s16 bpm)
{
	s32 ticksPerMinute = CTR_MipsMulLo(bpm, tpqn);
	u32 ticksPerSecond = (u32)ticksPerMinute / 60u;

	return CTR_MipsDivU((u32)CTR_MipsSll((s32)ticksPerSecond, 16), (u32)(s32)const60);
}

void SongPool_ChangeTempo(struct Song *song, s32 deltaBPM)
{
	struct CseqSongHeader *csh = GetCseqSongHeader(song->id);

	song->bpm = (s16)CTR_MipsAddLo((u16)csh->bpm, deltaBPM);

	song->tempo = SongPool_CalculateTempo(60, song->tpqn, song->bpm);
}

// NOTE(aalhendi): Retail passes the full tempo delta word; the song BPM store
// performs the 16-bit truncation after the addition. The loop and song-set
// arguments are reloaded from their incoming stack slots during sequence setup.
void SongPool_Start(struct Song *song, u16 songID, s32 deltaBPM, b32 volatile boolLoopAtEnd, struct SongSet *volatile songSet, int songSetActiveBits)
{
	s32 i;
	s32 vol;
	struct SongSeq *seqCurr;
	struct CseqSongHeader *csh;
	char *cnhFirst;
	struct SongNoteHeader *cnhCurr;
	u16 *seqOffsetArr;
	s16 *songOffsets;
	register struct SongSet *set CTR_PSX_REGISTER("$9");
	register b32 loop CTR_PSX_REGISTER("$9");
	u32 maxVolume;
	u32 playingFlag;

	// now playing
	song->flags = 1;
	// NOTE(aalhendi): Fetch the offset table before publishing the song ID.
	songOffsets = sdata->ptrCseqSongStartOffset;
	song->id = songID;
	csh = (struct CseqSongHeader *)&sdata->ptrCseqSongData[(u16)songOffsets[songID]];
	// advHub
	set = songSet;
	if (set != 0)
	{
		if (set->numSeqs != csh->numSeqs)
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
		// NOTE(aalhendi): Keep the volume selection in this branch before the shared stores.
		CTR_PSX_OBSERVE_MEMORY(song->vol_Curr);
		vol = 0xbe;
	}

	song->vol_Curr = vol;
	song->vol_New = vol;

	// Naughty Dog Box
	if (GAME_TRACKER->levelID == NAUGHTY_DOG_CRATE)
	{
		song->vol_Curr = 0xbe;
		song->vol_New = 0xbe;
	}

	song->vol_StepRate = 1;
	song->numSequences = 0;

	seqOffsetArr = (u16 *)SONGHEADER_GETSEQOFFARR(csh);

	// first note header comes after end of CseqSongHeader
	// and the full array of seqOffsets within the header
	cnhFirst = (char *)&seqOffsetArr[csh->numSeqs];

	// align up by 4
	if (((u32)cnhFirst & 1) != 0)
	{
		cnhFirst += 1;
	}
	if (((u32)cnhFirst & 2) != 0)
	{
		cnhFirst += 2;
	}

	i = 0;
	if (csh->numSeqs == 0)
	{
		return;
	}
	// NOTE(aalhendi): Prepare the two loop constants in retail order.
	playingFlag = 1;
	maxVolume = 0xff;
	CTR_PSX_KEEP_VALUE_RELAXED(maxVolume);
	do
	{
		cnhCurr = (struct SongNoteHeader *)&cnhFirst[seqOffsetArr[i]];

		seqCurr = SongPool_FindFreeChannel();
		if (seqCurr == NULL)
		{
			goto nextSequence;
		}

		// now playing
		seqCurr->flags = playingFlag;

		if ((cnhCurr->flags & 1) != 0)
		{
			// playing, and instrument/drums
			seqCurr->flags = 5;
		}

		loop = boolLoopAtEnd;
		if (loop != 0)
		{
			seqCurr->flags |= 2;
		}

		seqCurr->unk = cnhCurr->unk;

		seqCurr->instrumentID = 0;
		seqCurr->reverb = 0;

		set = songSet;
		if (set != 0 && (set->ptrSongSetBits[i] & song->songSetActiveBits) == 0)
		{
			seqCurr->vol_Curr = 0;
			seqCurr->vol_New = 0;
		}
		else
		{
			seqCurr->vol_Curr = maxVolume;
			seqCurr->vol_New = maxVolume;
		}

		seqCurr->vol_StepRate = playingFlag;

		seqCurr->distort = 0x80;
		seqCurr->LR = 0x80;

		seqCurr->unk0A = 0;
		seqCurr->songPoolIndex = song->songPoolIndex;

		seqCurr->NoteLength = 0;
		seqCurr->NoteTimeElapsed = 0;

		seqCurr->firstNote = (u8 *)NOTEHEADER_GETNOTES(cnhCurr);

		seqCurr->currNote = howl_GetNextNote(seqCurr->firstNote, &seqCurr->NoteLength);

		// Publish the sequence pointer before advancing the count.
		song->CseqSequences[song->numSequences] = seqCurr;
		song->numSequences++;
	nextSequence:
		i++;
	} while (i < csh->numSeqs);
}

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

void SongPool_AdvHub2(struct Song *song, struct SongSet *songSet, int songSetActiveBits)
{
	int i;
	struct CseqSongHeader *csh = GetCseqSongHeader(song->id);

	// advHub
	if (songSet != 0)
	{
		if (songSet->numSeqs != csh->numSeqs)
		{
			return;
		}

		song->songSetActiveBits = songSetActiveBits;
	}

	for (i = 0; i < csh->numSeqs; i++)
	{
		// NOTE(aalhendi): Keep the two volume calls distinct for retail's branch layout.
		if ((songSet->ptrSongSetBits[i] & song->songSetActiveBits) != 0)
		{
			SongPool_AdvHub1(song, i, 0xff, 0);
		}
		else
		{
			SongPool_AdvHub1(song, i, 0, 0);
		}
	}
}

void SongPool_StopCseq(struct SongSeq *seq)
{
	struct ChannelStats *curr, *backupNext;

	for (curr = (struct ChannelStats *)GAME_CHANNEL_TAKEN.first; curr != NULL; curr = backupNext)
	{
		backupNext = curr->link.links.next;

		if (curr->type != HOWL_CHANNEL_TYPE_MUSIC)
		{
			continue;
		}

		if (curr->soundID != seq->soundID)
		{
			continue;
		}

		// enable OFF flag, disable ON flag
		GAME_CHANNEL_UPDATE_FLAGS[curr->channelID] |= HOWL_CHANNEL_UPDATE_OFF;
		// NOTE(aalhendi): Retail rereads the flag word for the second update.
		*(volatile u32 *)&GAME_CHANNEL_UPDATE_FLAGS[curr->channelID] &= ~HOWL_CHANNEL_UPDATE_KEY_ON;

		curr->flags &= (u8)~1;

		// recycle: remove from taken, put on free
		LIST_RemoveMember(&GAME_CHANNEL_TAKEN, (struct Item *)curr);
		LIST_AddBack(&GAME_CHANNEL_FREE, (struct Item *)curr);
	}

	// not playing
	seq->flags &= ~(1);
}

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
