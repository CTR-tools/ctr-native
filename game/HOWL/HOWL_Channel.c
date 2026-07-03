#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002b4d0-0x8002b508
void Smart_EnterCriticalSection(void)
{
	int count = sdata->criticalSectionCount;

	sdata->criticalSectionCount = count + 1;

	if (count == 0)
	{
		EnterCriticalSection();
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002b508-0x8002b540
void Smart_ExitCriticalSection(void)
{
	int count = sdata->criticalSectionCount;

	if (count == 0)
	{
		return;
	}

	count--;
	sdata->criticalSectionCount = count;

	if (count == 0)
	{
		ExitCriticalSection();
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002b540-0x8002b5b4
void Channel_SetVolume(struct ChannelAttr *attr, int volume, int LR)
{
	if ((u32)volume >= 0x4000)
	{
		volume = 0x3fff;
	}

	if (sdata->boolStereoEnabled == 1)
	{
		attr->audioL = (volume * data.volumeLR[0xFF - LR]) >> 8;
		attr->audioR = (volume * data.volumeLR[0x00 + LR]) >> 8;
		return;
	}

	attr->audioL = volume;
	attr->audioR = volume;
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002b5b4-0x8002b608
int Channel_FindSound(int soundID)
{
	struct ChannelStats *curr, *backupNext;

	for (curr = (struct ChannelStats *)sdata->channelTaken.first; curr != NULL; curr = backupNext)
	{
		backupNext = curr->next;

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

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002b608-0x8002b7d0
struct ChannelStats *Channel_AllocSlot_AntiSpam(s16 soundID, char boolUseAntiSpam, int flags, struct ChannelAttr *attr)
{
	struct ChannelStats *curr, *backupNext;

	// with AntiSpam, a new sound started within
	// 10 frames of another, will replace the older

	// without AntiSpam, sounds within 10 frames
	// of each other will play until they all finish

	if (boolUseAntiSpam == 1)
	{
		for (curr = (struct ChannelStats *)sdata->channelTaken.first; curr != NULL; curr = backupNext)
		{
			backupNext = curr->next;

			if ((curr->type == HOWL_CHANNEL_TYPE_OTHER_FX) &&

			    // matching ID
			    ((curr->soundID & 0xffff) == ((u16)soundID)))
			{
				int duration = sdata->gGT->frameTimer_MainFrame_ResetDB - curr->startFrame;

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

void Channel_DestroySelf(struct ChannelStats *stats)
{
	// set channel to OFF, and remove PLAYING bit
	u32 *flagPtr = &sdata->ChannelUpdateFlags[stats->channelID];
	*flagPtr |= HOWL_CHANNEL_UPDATE_OFF;
	*flagPtr &= ~HOWL_CHANNEL_UPDATE_KEY_ON;

	stats->flags &= ~(1);

	// recycle
	LIST_RemoveMember(&sdata->channelTaken, (struct Item *)stats);
	LIST_AddBack(&sdata->channelFree, (struct Item *)stats);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002b7d0-0x8002b898
struct ChannelStats *Channel_AllocSlot(int flags, struct ChannelAttr *attr)
{
	struct ChannelAttr *newAttr;
	struct ChannelStats *stats;

	// get free slot
	stats = (struct ChannelStats *)sdata->channelFree.first;

	// quit if no free slots
	if (stats == NULL)
	{
		return NULL;
	}

	// allocate
	LIST_RemoveMember(&sdata->channelFree, (struct Item *)stats);
	LIST_AddBack(&sdata->channelTaken, (struct Item *)stats);

	// start playing
	sdata->ChannelUpdateFlags[stats->channelID] |= (flags | HOWL_CHANNEL_UPDATE_KEY_ON);

	// make new ChanenlAttr
	newAttr = &sdata->channelAttrNew[stats->channelID];

	// NOTE(aalhendi): Retail copies four ChannelAttr words; keep the native
	// copy defined so optimization preserves the caller's field writes.
	*newAttr = *attr;

	stats->flags = 1;

	return stats;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002b898-0x8002b9b8
struct ChannelStats *Channel_SearchFX_EditAttr(int type, int soundID, int updateFlags, struct ChannelAttr *attr)
{
	struct ChannelAttr *editAttr;

	struct ChannelStats *curr, *backupNext;

	for (curr = (struct ChannelStats *)sdata->channelTaken.first; curr != NULL; curr = backupNext)
	{
		backupNext = curr->next;

		if (
		    // matching type
		    (curr->type == type) &&

		    // matching ID
		    (curr->soundID == soundID))
		{
			// update flags
			sdata->ChannelUpdateFlags[curr->channelID] |= updateFlags;

			// edit ChanenlAttr
			editAttr = &sdata->channelAttrNew[curr->channelID];

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
				// OG game treats this as one int
				editAttr->audioL = attr->audioL;
				editAttr->audioR = attr->audioR;
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
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002b9b8-0x8002ba90
struct ChannelStats *Channel_SearchFX_Destroy(int type, int soundID, int flags)
{
	struct ChannelStats *curr, *backupNext;

	for (curr = (struct ChannelStats *)sdata->channelTaken.first; curr != NULL; curr = backupNext)
	{
		backupNext = curr->next;

		if (
		    // matching type
		    (curr->type == type) &&

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
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002ba90-0x8002bbac
void Channel_DestroyAll_LowLevel(int opt1, b32 boolKeepMusic, char type)
{
	struct ChannelStats *curr, *backupNext;

	for (curr = (struct ChannelStats *)sdata->channelTaken.first; curr != NULL; curr = backupNext)
	{
		backupNext = curr->next;

		if (
		    // destroy if not music
		    (curr->type != type) ||

		    // override and remove music too
		    (boolKeepMusic == 0))
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

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002bbac-0x8002be9c
void Channel_ParseSongToChannels()
{
	struct Song *song;
	struct SongSeq *seq;
	struct SongSeq **seqEntry;
	b32 boolVolumeChange;

	if (sdata->boolAudioEnabled == 0)
	{
		return;
	}
	if (sdata->ptrCseqHeader == 0)
	{
		return;
	}

	boolVolumeChange = false;

	for (song = &sdata->songPool[0]; song < &sdata->songPool[2]; song++)
	{
		// if not playing, skip
		if ((song->flags & 1) == 0)
		{
			continue;
		}

		// if paused, skip
		if ((song->flags & 2) != 0)
		{
			continue;
		}

		// song playing offset?
		song->unk10 += song->tempo;
		int unk10_total = song->unk10;

		song->timeSpentPlaying += unk10_total >> 0x10;
		song->unk10 = (u16)unk10_total;

		int volCurr = song->vol_Curr;
		int volNew = song->vol_New;
		int volStepRate = song->vol_StepRate;
		int volStepped;
		b32 boolFinalStep;

		// === Copy/Paste ===
		if (volCurr != volNew)
		{
			if (volCurr < volNew)
			{
				volStepped = volCurr + volStepRate;
				boolFinalStep = volStepped > volNew;
			}

			else // volCurr > volNew
			{
				volStepped = volCurr - volStepRate;
				boolFinalStep = volStepped < volNew;
			}

			if (boolFinalStep)
			{
				volStepped = volNew;

				// song is over
				if ((song->flags & 4) != 0)
				{
					song->flags &= ~(4);

					SongPool_StopAllCseq(song);
					Music_End();
				}
			}

			song->vol_Curr = volStepped;
			boolVolumeChange = true;
		}

		for (seqEntry = &song->CseqSequences[0]; seqEntry < &song->CseqSequences[song->numSequences]; seqEntry++)
		{
			seq = seqEntry[0];

			volCurr = seq->vol_Curr;
			volNew = seq->vol_New;
			volStepRate = seq->vol_StepRate;

			// === Copy/Paste ===
			if (volCurr != volNew)
			{
				if (volCurr < volNew)
				{
					volStepped = volCurr + volStepRate;
					boolFinalStep = volStepped > volNew;
				}

				else // volCurr > volNew
				{
					volStepped = volCurr - volStepRate;
					boolFinalStep = volStepped < volNew;
				}

				if (boolFinalStep)
				{
					volStepped = volNew;
				}

				seq->vol_Curr = volStepped;
				boolVolumeChange = true;
			}

			// if sequence is playing
			if ((seq->flags & 1) != 0)
			{
				seq->NoteTimeElapsed += unk10_total >> 0x10;

				// === need to work on this variable naming ===

				while (seq->NoteLength <= seq->NoteTimeElapsed)
				{
					// if reached end, quit
					if ((seq->flags & 1) == 0)
					{
						break;
					}

					seq->NoteTimeElapsed -= seq->NoteLength;

					// currNote->opcode
					int opcode = (u8)seq->currNote[0];

					if (opcode < 0xb)
					{
						// call opcode from funcPtr array,
						// this is OG until DATA is rewritten
						(*data.opcodeFunc[opcode])(seq);

						// if reached end, quit
						if ((seq->flags & 1) == 0)
						{
							break;
						}

						// if song restarting (opcode03)
						if ((seq->flags & 8) != 0)
						{
							seq->flags &= ~(8);

							seq->currNote = seq->firstNote;
						}

						// if song not restarting (opcode03)
						else
						{
							seq->currNote += data.opcodeOffset[opcode];
						}

						seq->currNote = howl_GetNextNote(seq->currNote, &seq->NoteLength);
					}
				}
			}
		}
	}

	if (boolVolumeChange)
	{
		UpdateChannelVol_Music_All();
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002be9c-0x8002c18c
void Channel_UpdateChannels()
{
	// int voice_bit;
	int vNum;

	// find all OFF channels
	int voiceBitOn = 0;
	int voiceBitOff = 0;

	struct ChannelAttr *cur;
	struct ChannelAttr *new;
	u32 *ptrFlag;

	for (vNum = 0, ptrFlag = &sdata->ChannelUpdateFlags[0];

	     vNum < NUM_SFX_CHANNELS;

	     vNum++, ptrFlag++)
	{
		u32 updateFlags = *ptrFlag;

		// if need to turn off
		if ((updateFlags & HOWL_CHANNEL_UPDATE_OFF) != 0)
		{
			voiceBitOff |= 1 << vNum;

			if ((updateFlags & HOWL_CHANNEL_UPDATE_KEY_ON) != 0)
			{
				*ptrFlag &= ~HOWL_CHANNEL_UPDATE_OFF;
			}

			else
			{
				*ptrFlag = 0;
			}
		}
	}

	SpuSetKey(0, voiceBitOff);

	for (vNum = 0, cur = &sdata->channelAttrCur[0], new = &sdata->channelAttrNew[0], ptrFlag = &sdata->ChannelUpdateFlags[0];

	     vNum < NUM_SFX_CHANNELS;

	     vNum++, cur++, new++, ptrFlag++)
	{
		u32 updateFlags = *ptrFlag;

		// if need to turn on
		if ((updateFlags & HOWL_CHANNEL_UPDATE_KEY_ON) != 0)
		{
			voiceBitOn |= 1 << vNum;
		}

		// start address needs to change
		if ((updateFlags & HOWL_CHANNEL_UPDATE_SPU_ADDR) != 0)
		{
			void *startAddr = new->spuStartAddr;

			if (startAddr != cur->spuStartAddr)
			{
				cur->spuStartAddr = startAddr;

				SpuSetVoiceStartAddr(vNum, (int)startAddr);
			}
		}

		// ADSR needs to change
		if ((updateFlags & HOWL_CHANNEL_UPDATE_ADSR) != 0)
		{
			int adsr = (int)CTR_ReadU32LE(&new->ad);

			if (adsr != (int)CTR_ReadU32LE(&cur->ad))
			{
				int ad = new->ad;
				int sr = new->sr;

				cur->ad = ad;
				cur->sr = sr;

				int local_38;
				int local_34;
				int RRmode;

				if ((s16)ad < 0)
				{
					local_38 = 5;
				}
				else
				{
					local_38 = 1;
				}

				if ((s16)sr < 0)
				{
					if ((sr >> 0xe & 1) == 0)
					{
						local_34 = 5;
					}
					else
					{
						local_34 = 7;
					}
				}
				else
				{
					if ((sr >> 0xe & 1) == 0)
					{
						local_34 = 1;
					}
					else
					{
						local_34 = 3;
					}
				}

				if ((sr >> 5 & 1) == 0)
				{
					RRmode = 3;
				}
				else
				{
					RRmode = 7;
				}

				SpuSetVoiceADSRAttr(vNum, (ad >> 8) & 0x7f, (ad >> 4) & 0xf, (sr >> 6) & 0x7f, sr & 0x1f, ad & 0xf, local_38, local_34, RRmode);
			}
		}

		// pitch needs to change
		if ((updateFlags & HOWL_CHANNEL_UPDATE_PITCH) != 0)
		{
			int pitch = new->pitch;

			if (pitch != cur->pitch)
			{
				cur->pitch = pitch;
				SpuSetVoicePitch(vNum, pitch);
			}
		}

		// reverb needs to change
		if ((updateFlags & HOWL_CHANNEL_UPDATE_REVERB) != 0)
		{
			int reverb = new->reverb;

			if (reverb != cur->reverb)
			{
				cur->reverb = reverb;
				SpuSetReverbVoice(reverb != 0, 1 << vNum);
			}
		}

		// volume needs to change
		if ((updateFlags & HOWL_CHANNEL_UPDATE_VOLUME) != 0)
		{
			int audioLR = (int)CTR_ReadU32LE(&new->audioL);

			if (audioLR != (int)CTR_ReadU32LE(&cur->audioL))
			{
				int audioL = new->audioL;
				int audioR = new->audioR;

				cur->audioL = audioL;
				cur->audioR = audioR;

				SpuSetVoiceVolume(vNum, audioL, audioR);
			}
		}

		*ptrFlag = 0;
	}

	SpuSetKey(1, voiceBitOn);
}
