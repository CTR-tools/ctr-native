#include <common.h>

// NOTE(aalhendi): The resident entry load uses a scalar binding; native keeps canonical storage.
#ifndef HOWL_CHANNEL_CRITICAL_INITIAL
#define HOWL_CHANNEL_CRITICAL_INITIAL (sdata->criticalSectionCount)
#endif

#ifndef HOWL_CHANNEL_AUDIO_INITIAL
#define HOWL_CHANNEL_AUDIO_INITIAL  (sdata->boolAudioEnabled)
#define HOWL_CHANNEL_ATTR_CUR       (sdata->channelAttrCur)
#define HOWL_CHANNEL_OPCODE_FUNCS   (data.opcodeFunc)
#define HOWL_CHANNEL_OPCODE_OFFSETS (data.opcodeOffset)
#endif

void Smart_EnterCriticalSection(void)
{
	int count = HOWL_CHANNEL_CRITICAL_INITIAL;
	int previous;

	previous = count;
	// NOTE(aalhendi): Preserve the pre-increment copy and the increment operand.
	CTR_PSX_KEEP_VALUE_RELAXED(count);

	count = CTR_MipsAddLo(count, 1);
	sdata->criticalSectionCount = count;
	// Complete the store before testing whether to enter or leave the SDK section.
	CTR_PSX_MEMORY_BARRIER();

	if (previous == 0)
	{
		EnterCriticalSection();
	}
}

void Smart_ExitCriticalSection(void)
{
	int count = HOWL_CHANNEL_CRITICAL_INITIAL;

	if (count == 0)
	{
		return;
	}

	count = CTR_MipsSubLo(count, 1);
	sdata->criticalSectionCount = count;
	// Complete the store before testing whether to enter or leave the SDK section.
	CTR_PSX_MEMORY_BARRIER();

	if (count == 0)
	{
		ExitCriticalSection();
	}
}

void Channel_SetVolume(struct ChannelAttr *attr, int volume, int LR)
{
	if ((u32)volume >= 0x4000)
	{
		volume = 0x3fff;
	}

	if (sdata->boolStereoEnabled == 1)
	{
		// NOTE(aalhendi): The clamp makes volume nonnegative; retail shifts the products logically.
		attr->audioL = (u32)(volume * GAME_VOLUME_LR[0xFF - LR]) >> 8;
		attr->audioR = (u32)(volume * GAME_VOLUME_LR[0x00 + LR]) >> 8;
		return;
	}

	attr->audioL = volume;
	attr->audioR = volume;
	return;
}

int Channel_FindSound(int soundID)
{
	struct ChannelStats *curr, *backupNext;

	for (curr = (struct ChannelStats *)GAME_CHANNEL_TAKEN.first; curr != NULL; curr = backupNext)
	{
		backupNext = curr->link.links.next;

		if ((curr->type == HOWL_CHANNEL_TYPE_OTHER_FX) &&

		    // matching low 16-bit sound ID
		    ((curr->soundID & 0xffff) == (soundID & 0xffff)))
		{
			// sound already playing
			return 1;
		}
	}

	// sound not playing
	return 0;
}

// NOTE(aalhendi): Keep both helper definitions before AntiSpam so GCC 2.8.1 inlines their retail paths.
inline void Channel_DestroySelf(struct ChannelStats *stats)
{
	// NOTE(aalhendi): Retail performs separate flag updates and reloads the channel index.
	// Set the channel to OFF, then remove the PLAYING bit.
	GAME_CHANNEL_UPDATE_FLAGS[stats->channelID] |= HOWL_CHANNEL_UPDATE_OFF;
	GAME_CHANNEL_UPDATE_FLAGS[stats->channelID] &= ~HOWL_CHANNEL_UPDATE_KEY_ON;

	stats->flags &= ~(1);

	// recycle
	LIST_RemoveMember(&GAME_CHANNEL_TAKEN, (struct Item *)stats);
	LIST_AddBack(&GAME_CHANNEL_FREE, (struct Item *)stats);
}

inline struct ChannelStats *Channel_AllocSlot(int flags, struct ChannelAttr *attr)
{
	struct ChannelAttr *newAttr;
	struct ChannelAttr *attrBase;
	// NOTE(aalhendi): Retail keeps the return pointer in v0 through the attribute copy,
	// both here and when this helper is inlined into the anti-spam allocator.
	register struct ChannelStats *result CTR_PSX_REGISTER("$2");
	struct ChannelStats *stats;

	// get free slot
	stats = (struct ChannelStats *)GAME_CHANNEL_FREE.first;

	// NOTE(aalhendi): The shared return preserves retail's null-path branch layout.
	if (stats == NULL)
	{
		result = NULL;
	}
	else
	{
		// allocate
		LIST_RemoveMember(&GAME_CHANNEL_FREE, (struct Item *)stats);
		LIST_AddBack(&GAME_CHANNEL_TAKEN, (struct Item *)stats);

		// start playing
		GAME_CHANNEL_UPDATE_FLAGS[stats->channelID] |= (flags | HOWL_CHANNEL_UPDATE_KEY_ON);

		result = stats;

		// NOTE(aalhendi): A separate attribute base preserves retail's address evaluation order.
		attrBase = GAME_CHANNEL_ATTR_NEW;
		newAttr = &attrBase[result->channelID];

		// NOTE(aalhendi): Retail copies four ChannelAttr words; keep the native
		// copy defined so optimization preserves the caller's field writes.
		*newAttr = *attr;

		result->flags = 1;
	}
	return result;
}

struct ChannelStats *Channel_AllocSlot_AntiSpam(u16 soundID, u8 boolUseAntiSpam, int flags, struct ChannelAttr *attr)
{
	struct ChannelStats *curr, *backupNext;

	// with AntiSpam, a new sound started within
	// 10 frames of another, will replace the older

	// without AntiSpam, sounds within 10 frames
	// of each other will play until they all finish

	if (boolUseAntiSpam == 1)
	{
		// NOTE(aalhendi): Retail snapshots the frame before traversing the list.
		u32 frame = GAME_TRACKER->frameTimer_MainFrame_ResetDB;
		for (curr = (struct ChannelStats *)GAME_CHANNEL_TAKEN.first; curr != NULL; curr = backupNext)
		{
			backupNext = curr->link.links.next;

			if ((curr->type == HOWL_CHANNEL_TYPE_OTHER_FX) &&

			    // matching ID
			    ((curr->soundID & 0xffff) == ((u16)soundID)))
			{
				u32 duration = frame - (u32)curr->startFrame;

				// if started within 10 frames, cancel old and start new,
				// otherwise you'll allocate too many sounds and overflow
				if (duration < 10)
				{
					Channel_DestroySelf(curr);
				}
			}
		}
	}

	return Channel_AllocSlot(flags, attr);
}

struct ChannelStats *Channel_SearchFX_EditAttr(int type, int soundID, int updateFlags, struct ChannelAttr *attr)
{
	struct ChannelAttr *editAttr;

	struct ChannelStats *curr;

	for (curr = (struct ChannelStats *)GAME_CHANNEL_TAKEN.first; curr != NULL; curr = curr->link.links.next)
	{
		if (
		    // matching type
		    (curr->type == (u8)type) &&

		    // matching ID
		    (curr->soundID == soundID))
		{
			// update flags
			GAME_CHANNEL_UPDATE_FLAGS[curr->channelID] |= updateFlags;

			// edit ChannelAttr
			editAttr = &GAME_CHANNEL_ATTR_NEW[curr->channelID];

			// change in spu addr
			if ((updateFlags & HOWL_CHANNEL_UPDATE_SPU_ADDR) != 0)
			{
				editAttr->spuStartAddr = attr->spuStartAddr;
			}

			// change in ADSR
			if ((updateFlags & HOWL_CHANNEL_UPDATE_ADSR) != 0)
			{
				editAttr->ad = attr->ad;
				editAttr->sr = attr->sr;
			}

			// change in pitch
			if ((updateFlags & HOWL_CHANNEL_UPDATE_PITCH) != 0)
			{
				editAttr->pitch = attr->pitch;
			}

			// change in reverb
			if ((updateFlags & HOWL_CHANNEL_UPDATE_REVERB) != 0)
			{
				editAttr->reverb = attr->reverb;
			}

			// change in volume
			if ((updateFlags & HOWL_CHANNEL_UPDATE_VOLUME) != 0)
			{
				// NOTE(aalhendi): Retail copies the adjacent volumes with one unaligned word.
				// The aliasing aggregate retains their two-byte alignment and permits a self-copy.
				struct ChannelVolumePair
				{
					s16 left;
					s16 right;
				} CTR_MAY_ALIAS;
				*(struct ChannelVolumePair *)&editAttr->audioL = *(const struct ChannelVolumePair *)&attr->audioL;
			}

			// OG code does this, but what if
			// there's multiple of the same sound?
			return curr;
		}
	}

	// sound not playing
	return 0;
}

// depending on flags, you might have:
//	16-bit soundID, which destroys all of this type of sound
//	32-bit soundID_count, which destroys specific instance of sound
struct ChannelStats *Channel_SearchFX_Destroy(int type, int soundID, int flags)
{
	struct ChannelStats *curr, *backupNext;

	for (curr = (struct ChannelStats *)GAME_CHANNEL_TAKEN.first; curr != NULL; curr = backupNext)
	{
		backupNext = curr->link.links.next;

		if (
		    // matching type
		    (curr->type == (u8)type) &&

		    // matching ID and bit-shifted soundCount
		    ((curr->soundID & flags) == (soundID & flags)))
		{
			Channel_DestroySelf(curr);

			// OG code does this, but what if
			// there's multiple of the same sound?

			// This actually bugs if there's two ngin
			// characters as P2 and P3 in podium, you
			// still here the ssssshhhhhhh if both of
			// them are "steaming" and you leave podium

			return curr;
		}
	}

	return NULL;
}

// param_1 0: keep menu fx, 1: destroy all fx
// param_2 0: destroy music, 1: keep music
void Channel_DestroyAll_LowLevel(int opt1, b32 boolKeepMusic, u8 type)
{
	struct ChannelStats *curr, *backupNext;

	for (curr = (struct ChannelStats *)GAME_CHANNEL_TAKEN.first; curr != NULL; curr = backupNext)
	{
		backupNext = curr->link.links.next;

		if (
		    // destroy if not music
		    (boolKeepMusic == 0) || (curr->type != type))
		{
			if (
			    // if override
			    (opt1 != 0) ||

			    (
			        // if not otherFX, erase
			        (curr->type != HOWL_CHANNEL_TYPE_OTHER_FX) ||

			        // if otherFX and not menu sounds,
			        // cause those should ring out
			        ((u16)curr->soundID > 5)))
			{
				Channel_DestroySelf(curr);
			}
		}
	}
}

void Channel_ParseSongToChannels(void)
{
	struct Song *song;
	struct SongSeq *seq;
	int songIndex;
	int seqIndex;
	b32 boolVolumeChange;
	u32 ticks;
	int volCurr, volStepped;
	// NOTE(aalhendi): Retail reuses v1 for the timer total and fade target.
	register int volNew CTR_PSX_REGISTER("$3");
	b32 boolFinalStep;

	if (HOWL_CHANNEL_AUDIO_INITIAL == 0)
		return;
	if (sdata->ptrCseqHeader == 0)
		return;
	boolVolumeChange = 0;
	for (songIndex = 0; songIndex < 2; songIndex++)
	{
		register u32 total CTR_PSX_REGISTER("$3");
		u32 played;
		register u32 low CTR_PSX_REGISTER("$4");
		song = &GAME_SONG_POOL[songIndex];
		if ((song->flags & 1) == 0)
			continue;
		if ((song->flags & 2) != 0)
			continue;
		volCurr = song->vol_Curr;
		// The fixed-point timer wraps at 32 bits and yields unsigned whole ticks.
		total = (u32)song->unk10;
		total += (u32)song->tempo;
		played = song->timeSpentPlaying;
		// NOTE(aalhendi): Keep the elapsed-time load before the timer split.
		CTR_PSX_DEPEND_VALUE(total, played);
		song->unk10 = total;
		ticks = total >> 16;
		low = (u16)song->unk10;
		song->timeSpentPlaying = CTR_MipsAddLo(played, ticks);
		song->unk10 = low;
		volNew = song->vol_New;
		if (volCurr != volNew)
		{
			volStepped = volCurr;
			if (volStepped < volNew)
			{
				volStepped += song->vol_StepRate;
				boolFinalStep = volStepped > volNew;
			}
			else
			{
				volStepped -= song->vol_StepRate;
				boolFinalStep = volStepped < volNew;
			}
			if (boolFinalStep)
			{
				volStepped = volNew;
				if ((song->flags & 4) != 0)
				{
					SongPool_StopAllCseq(song);
					Music_End();
					// Retail clears the stop flag after stopping sequences and ending music.
					song->flags &= ~4;
				}
			}
			song->vol_Curr = volStepped;
			boolVolumeChange = 1;
		}
		for (seqIndex = 0; seqIndex < song->numSequences; seqIndex++)
		{
			int seqCurrent;
			seq = song->CseqSequences[seqIndex];
			seqCurrent = seq->vol_Curr;
			if (seqCurrent != seq->vol_New)
			{
				volStepped = seqCurrent;
				volNew = seq->vol_New;
				if (volStepped < volNew)
				{
					volStepped += seq->vol_StepRate;
					boolFinalStep = volStepped > volNew;
				}
				else
				{
					volStepped -= seq->vol_StepRate;
					boolFinalStep = volStepped < volNew;
				}
				if (boolFinalStep)
					volStepped = volNew;
				seq->vol_Curr = volStepped;
				boolVolumeChange = 1;
			}
			if ((seq->flags & 1) != 0)
			{
				seq->NoteTimeElapsed = CTR_MipsAddLo(seq->NoteTimeElapsed, ticks);
				while ((u32)seq->NoteLength <= (u32)seq->NoteTimeElapsed)
				{
					u32 opcode;
					if ((seq->flags & 1) == 0)
						break;
					seq->NoteTimeElapsed = CTR_MipsSubLo(seq->NoteTimeElapsed, seq->NoteLength);
					opcode = seq->currNote[0];
					if (opcode < 11u)
					{
						(*HOWL_CHANNEL_OPCODE_FUNCS[opcode])(seq);
						if ((seq->flags & 1) != 0)
						{
							register u8 *note CTR_PSX_REGISTER("$4");
							int *length = &seq->NoteLength;
							if ((seq->flags & 8) != 0)
							{
								seq->flags &= ~8;
								note = seq->firstNote;
							}
							else
							{
								seq->currNote += HOWL_CHANNEL_OPCODE_OFFSETS[opcode];
								note = seq->currNote;
							}
							seq->currNote = howl_GetNextNote(note, length);
						}
					}
				}
			}
		}
	}
	if (boolVolumeChange)
		UpdateChannelVol_Music_All();
}

void Channel_UpdateChannels(void)
{
	int vNum;
	u32 voiceBits = 0;
	struct ChannelAttr *cur;
	struct ChannelAttr *new;
	u32 *ptrFlag;
	u32 *firstFlag;
	u32 *flagsBase;
	u32 updateFlags;
	u32 voiceBit;
	u32 keepFlags;

	vNum = voiceBits;
	voiceBit = 1u;
	keepFlags = ~HOWL_CHANNEL_UPDATE_OFF;
	for (firstFlag = GAME_CHANNEL_UPDATE_FLAGS; vNum < NUM_SFX_CHANNELS; vNum++, firstFlag++)
	{
		updateFlags = *firstFlag;
		if (updateFlags == 0)
			continue;
		if ((updateFlags & HOWL_CHANNEL_UPDATE_OFF) != 0)
		{
			voiceBits |= voiceBit << vNum;
			if ((updateFlags & HOWL_CHANNEL_UPDATE_KEY_ON) != 0)
				*firstFlag = updateFlags & keepFlags;
			else
				*firstFlag = 0;
		}
	}
	SpuSetKey(0, voiceBits);
	vNum = voiceBits = 0;
	flagsBase = GAME_CHANNEL_UPDATE_FLAGS;
	for (; vNum < NUM_SFX_CHANNELS; vNum++)
	{
		register u32 *entry CTR_PSX_REGISTER("$2");
		// NOTE(aalhendi): Keep indexed traversal and the retail index-before-base operand order.
		CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(entry, flagsBase, vNum * sizeof(u32));
		ptrFlag = entry;
		updateFlags = *entry;
		if (updateFlags == 0)
			continue;
		cur = &HOWL_CHANNEL_ATTR_CUR[vNum];
		new = &GAME_CHANNEL_ATTR_NEW[vNum];
		if ((updateFlags & HOWL_CHANNEL_UPDATE_KEY_ON) != 0)
			voiceBits |= 1u << vNum;
		if ((updateFlags & HOWL_CHANNEL_UPDATE_SPU_ADDR) != 0)
		{
			void *oldAddr = cur->spuStartAddr;
			void *startAddr = new->spuStartAddr;
			if (oldAddr != startAddr)
			{
				cur->spuStartAddr = startAddr;
				SpuSetVoiceStartAddr(vNum, (u32)startAddr);
			}
		}
		if ((updateFlags & HOWL_CHANNEL_UPDATE_ADSR) != 0)
		{
			if (CTR_ReadU32AlignedLE(&cur->ad) != CTR_ReadU32AlignedLE(&new->ad))
			{
				u16 ad, sr;
				ad = (u16)new->ad;
				cur->ad = ad;
				sr = (u16)(cur->sr = new->sr);
				// NOTE(aalhendi): Direct mode expressions preserve retail outgoing-argument order.
				SpuSetVoiceADSRAttr(vNum, (ad >> 8) & 127, (ad >> 4) & 15, (sr >> 6) & 127, sr & 31, ad & 15, (ad >> 15) ? 5 : 1,
				                    (sr >> 15) ? (((sr >> 14) & 1) ? 7 : 5) : (((sr >> 14) & 1) ? 3 : 1), ((sr >> 5) & 1) ? 7 : 3);
			}
		}
		if ((updateFlags & HOWL_CHANNEL_UPDATE_PITCH) != 0)
		{
			u32 pitch = (u16)new->pitch;
			u32 pitchArg;
			// NOTE(aalhendi): Preserve separate full-width comparison and narrowed SDK argument lifetimes.
			pitchArg = pitch & 0xffff;
			if ((u16)cur->pitch != pitch)
			{
				cur->pitch = pitch;
				SpuSetVoicePitch(vNum, pitchArg);
			}
		}
		if ((updateFlags & HOWL_CHANNEL_UPDATE_REVERB) != 0)
		{
			register u16 reverb CTR_PSX_REGISTER("$4") = (u16)new->reverb;
			if ((u16)cur->reverb != reverb)
			{
				cur->reverb = reverb;
				// Normalize the SDK argument only after publishing the original reverb value.
				{
					int enabled = 0;
					if (reverb != 0)
					{
						enabled = 1;
						CTR_PSX_KEEP_VALUE_RELAXED(enabled);
					}
					SpuSetReverbVoice(enabled, 1u << vNum);
				}
			}
		}
		if ((updateFlags & HOWL_CHANNEL_UPDATE_VOLUME) != 0)
		{
			if (CTR_ReadU32AlignedLE(&cur->audioL) != CTR_ReadU32AlignedLE(&new->audioL))
			{
				cur->audioL = new->audioL;
				cur->audioR = new->audioR;
				SpuSetVoiceVolume(vNum, cur->audioL, cur->audioR);
			}
		}
		*ptrFlag = 0;
	}
	SpuSetKey(1, voiceBits);
}
