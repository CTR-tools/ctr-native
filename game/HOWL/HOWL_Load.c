#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002991c-0x80029988
u32 howl_InstrumentPitch(int basePitch, int pitchIndex, u32 distort)
{
	// param_3
	// (>> 0) & 0x40 - distortion
	// (>> 6) & 0xXX - pitch/octave?

	u32 freq = CTR_MipsSrl(CTR_MipsMulLo(data.noteFrequency[pitchIndex + CTR_MipsSra((s32)distort, 6) - 2], basePitch), 12);

	distort &= 0x3f;
	freq &= 0xffff;

	if (distort != 0)
	{
		freq = CTR_MipsSrl(CTR_MipsMulLo(freq, CTR_MipsAddLo(data.distortConst_Music[distort], 0x100000)), 20);
	}

	return freq & 0xffff;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80029988-0x80029a50
int howl_InitGlobals(char *filename)
{
	if (sdata->boolAudioEnabled == 1)
	{
		return 0;
	}

	sdata->vol_FX = 215;
	sdata->vol_Music = 175;
	sdata->vol_Voice = 255;

	sdata->OptionSlider_BoolPlay = 0;
	sdata->OptionSlider_Index = 0;
	sdata->OptionSlider_soundID = 0;

	sdata->boolStereoEnabled = 1;
	sdata->boolAudioEnabled = 1;
	sdata->boolStoringVolume = 0;
	sdata->songLoadStage = 3;

	SpuInit();
	SpuSetTransferMode(0);
	SpuSetCommonMasterVolume(0x3fff, 0x3fff);

	SetReverbMode(5);

	SpuSetCommonCDReverb(0);
	SpuSetCommonCDMix(1);
	SpuSetCommonCDVolume(0, 0);

	Voiceline_PoolInit();
	Voiceline_SetDefaults();

	return howl_LoadHeader(filename) != 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80029a50-0x80029ab4
void howl_ParseHeader(struct HowlHeader *hh)
{
	u32 addr = (u32)hh;

	sdata->ptrHowlHeader = (struct HowlHeader *)addr;
	addr += sizeof(struct HowlHeader);

	sdata->howl_spuAddrs = (struct SpuAddrEntry *)addr;
	addr += sizeof(struct SpuAddrEntry) * hh->numSpuAddrs;

	sdata->howl_metaOtherFX = (struct OtherFX *)addr;
	addr += sizeof(struct OtherFX) * hh->numOtherFX;

	sdata->howl_metaEngineFX = (struct EngineFX *)addr;
	addr += sizeof(struct EngineFX) * hh->numEngineFX;

	sdata->howl_bankOffsets = (u16 *)addr;
	addr += sizeof(s16) * hh->numBanks;

	sdata->howl_songOffsets = (u16 *)addr;
	addr += sizeof(s16) * hh->numSequences;

	sdata->howl_endOfHowl = addr;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80029ab4-0x80029b2c
void howl_ParseCseqHeader(struct CseqHeader *ch)
{
	u32 addr = (u32)ch;

	sdata->ptrCseqHeader = (struct CseqHeader *)addr;
	addr += sizeof(struct CseqHeader);

	sdata->ptrCseqLongSamples = (struct SampleInstrument *)addr;
	addr += sizeof(struct SampleInstrument) * ch->numLongSamples;

	sdata->ptrCseqShortSamples = (struct SampleDrums *)addr;
	addr += sizeof(struct SampleDrums) * ch->numShortSamples;

	sdata->ptrCseqSongStartOffset = (s16 *)addr;
	addr += sizeof(s16) * ch->numSongs;

	addr = (addr + 3) & ~3;

	sdata->ptrCseqSongData = (char *)addr;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80029b2c-0x80029c40
int howl_LoadHeader(char *filename)
{
	struct HowlHeader *alloc;
	int howlHeaderSize;
	int numSector;
	int ret;

	if (LOAD_FindFile(filename, &sdata->KartHWL_CdFile) == 0)
	{
		return 0;
	}

	MEMPACK_PushState();

	// allocate room for one sector
	alloc = MEMPACK_AllocMem(0x800 /*, filename*/);

	if (alloc != 0)
	{
		// read sector #1 of HOWL, just for header
		ret = LOAD_HowlHeaderSectors(&sdata->KartHWL_CdFile, alloc, 0, 1);

		if (
		    // confirm first sector loaded properly
		    (ret != 0) && (alloc->magic == *(int *)&sdata->s_HOWL[0]) && (alloc->version == 0x80) // different in other CTR builds
		)
		{
			// allocate room for howlHeader + pointerTable
			howlHeaderSize = sizeof(struct HowlHeader) + alloc->headerSize;

			// align up for sector size
			numSector = CTR_MipsSra(CTR_MipsAddLo(howlHeaderSize, 0x7ff), 11);
			MEMPACK_ReallocMem(numSector << 0xb);

			// if header needs more sectors loaded, like CTR-U which needs 3 sectors
			if (numSector < 2 || LOAD_HowlHeaderSectors(&sdata->KartHWL_CdFile, (void *)((int)alloc + 0x800), 1, numSector - 1) != 0)
			{
				// initilaize header and pointer table
				howl_ParseHeader(alloc);

				// reallocate room just howlHeader + pointerTable,
				// deallocate sector-alignment padding
				MEMPACK_ReallocMem(howlHeaderSize);

				// do NOT PopState
				return 1;
			}
		}
	}

	MEMPACK_PopState();
	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80029c40-0x80029ca4
int howl_SetSong(int songID)
{
	if (sdata->boolAudioEnabled == 0)
	{
		// Stage 3: Finished
		sdata->songLoadStage = 3;

		return 1;
	}

	// === Reset Song ===

	howl_ErasePtrCseqHeader();

	// Stage 0: Start Loading
	sdata->songLoadStage = 0;

	sdata->songSectorOffset = sdata->howl_songOffsets[songID & 0xffff];
	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80029ca4-0x80029dc0
// similar to h23_Bank_AssignSpuAddrs, and h34_howl_LoadHeader
int howl_LoadSong()
{
	int ret;

	// Stage 3: Finished
	if (sdata->songLoadStage == 3)
	{
		return 1;
	}

	// Stage 0: Load 1/2
	if (sdata->songLoadStage == 0)
	{
		ret = LOAD_HowlSectorChainStart(&sdata->KartHWL_CdFile,  // CdLoc of HOWL
		                                sdata->sampleBlock1,     // destination in RAM for songs
		                                sdata->songSectorOffset, // song offset on disc, from CdLoc
		                                1                        // one sector
		);

		if (ret != 0)
		{
			// go to next stage
			sdata->songLoadStage++;
		}

		return 0;
	}

	// Stage 1: Load 2/2
	if (sdata->songLoadStage == 1)
	{
		if (LOAD_HowlSectorChainEnd() == 0)
		{
			return 0;
		}

		// CseqHeader->songSize, aligned up to sector size
		int numSector = CTR_MipsSrl(CTR_MipsAddLo(*(s32 *)&sdata->sampleBlock1[0], 0x7ff), 11);

		ret = LOAD_HowlSectorChainStart(&sdata->KartHWL_CdFile,      // CdLoc of HOWL
		                                sdata->tenSampleBlocks,      // (sampleBlock1+0x800) RAM destination
		                                sdata->songSectorOffset + 1, // song offset on disc, from CdLoc
		                                numSector - 1);

		if (ret != 0)
		{
			// go to next stage
			sdata->songLoadStage++;
		}

		return 0;
	}

	// Stage 2: Parsing Song
	if (sdata->songLoadStage == 2)
	{
		if (LOAD_HowlSectorChainEnd() == 0)
		{
			return 0;
		}

		howl_ParseCseqHeader((struct CseqHeader *)sdata->sampleBlock1);

		// go to next stage
		sdata->songLoadStage++;
		return 1;
	}

	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80029dc0-0x80029dcc
void howl_ErasePtrCseqHeader()
{
	// can not play a song anymore
	sdata->ptrCseqHeader = 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80029dcc-0x80029e18
u8 *howl_GetNextNote(u8 *currNote, int *noteLen)
{
	int var1;

	var1 = currNote[0] & 0x7f;

	// find the end opcode of currNote
	while ((currNote[0] & 0x80) != 0)
	{
		currNote++;

		// what on earth?
		// from DCxDemo: its delta time.
		// midi format uses a kind of compression. every byte is
		// 1 bit "has next byte flag".
		// 7 bits is number data
		// so that code skips proper amount of bytes it uses.
		// it allows to send only 1 byte for s16 events.
		var1 = CTR_MipsAddLo(CTR_MipsSll(var1, 7), currNote[0] & 0x7f);
	}

	*noteLen = var1;
	return currNote + 1;
}

void cseq_opcode00_empty(struct SongSeq *seq)
{
	// left empty by ND
}
