#include <common.h>

// NOTE(aalhendi): Resident bindings distinguish the entry scalar loads from
// later aggregate accesses and name the song buffer; native uses canonical storage.
#ifndef HOWL_LOAD_AUDIO_ENABLED_INITIAL
#define HOWL_LOAD_AUDIO_ENABLED_INITIAL (sdata->boolAudioEnabled)
#endif
#ifndef HOWL_LOAD_STAGE_INITIAL
#define HOWL_LOAD_STAGE_INITIAL (sdata->songLoadStage)
#endif
#ifndef HOWL_LOAD_SONG_BUFFER
#define HOWL_LOAD_SONG_BUFFER (sdata->sampleBlock1)
#endif

u32 howl_InstrumentPitch(int basePitch, int pitchIndex, u32 distort)
{
	u32 distortIndex;
	u32 freq;
	u32 distortScale;
	int noteOffset = CTR_MipsSra((s32)distort, 6) - 2;

	freq = CTR_MipsSrl(CTR_MipsMulLo(GAME_NOTE_FREQUENCY[pitchIndex + noteOffset], basePitch), 12);

	distortIndex = distort & 0x3f;

	if (distortIndex != 0)
	{
		distortScale = CTR_MipsAddLo(GAME_DISTORT_CONST_MUSIC[distortIndex], 0x100000);
		freq = CTR_MipsSrl(CTR_MipsMulLo((u16)freq, distortScale), 20);
	}

	return freq & 0xffff;
}

int howl_InitGlobals(char *filename)
{
	u32 enabled;

	if (HOWL_LOAD_AUDIO_ENABLED_INITIAL != 0)
	{
		return 0;
	}

	// NOTE(aalhendi): Retain the enabled constant across the default-volume stores.
	enabled = 1;
	CTR_PSX_KEEP_VALUE(enabled);
	sdata->vol_FX = 215;
	sdata->vol_Music = 175;
	sdata->vol_Voice = 255;

	sdata->OptionSlider_BoolPlay = 0;
	sdata->OptionSlider_Index = 0;
	sdata->OptionSlider_soundID = 0;

	sdata->boolStereoEnabled = enabled;
	sdata->boolAudioEnabled = enabled;
	sdata->boolStoringVolume = 0;
	sdata->songLoadStage = 3;
	// Finish all defaults before entering SPU initialization.
	CTR_PSX_MEMORY_BARRIER();

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

void howl_ParseHeader(struct HowlHeader *hh)
{
	u32 numSpuAddrs = hh->numSpuAddrs;
	u32 numOtherFX = hh->numOtherFX;
	u32 addr = (u32)hh + sizeof(struct HowlHeader);
	u32 numEngineFX;
	u32 numBanks;
	u32 numSequences;

	sdata->howl_spuAddrs = (struct SpuAddrEntry *)addr;
	addr += sizeof(struct SpuAddrEntry) * numSpuAddrs;

	sdata->howl_metaOtherFX = (struct OtherFX *)addr;
	addr += sizeof(struct OtherFX) * numOtherFX;

	numEngineFX = hh->numEngineFX;
	numBanks = hh->numBanks;
	// NOTE(aalhendi): Keep the header/count lifetimes in retail's load/store order.
	CTR_PSX_DEPEND_VALUE(hh, addr);
	sdata->ptrHowlHeader = hh;
	sdata->howl_metaEngineFX = (struct EngineFX *)addr;
	addr += sizeof(struct EngineFX) * numEngineFX;

	numSequences = hh->numSequences;
	CTR_PSX_KEEP_VALUE(hh);
	sdata->howl_bankOffsets = (u16 *)addr;
	addr += sizeof(s16) * numBanks;

	sdata->howl_songOffsets = (u16 *)addr;
	addr += sizeof(s16) * numSequences;

	// NOTE(aalhendi): Retail stores this before the return, not in its delay slot.
	*(volatile u32 *)&sdata->howl_endOfHowl = addr;
}

void howl_ParseCseqHeader(struct CseqHeader *ch)
{
	u32 addr = (u32)ch;
	u32 numSongs;
	u32 alignedAddr;

	sdata->ptrCseqHeader = (struct CseqHeader *)addr;
	addr += sizeof(struct CseqHeader);

	sdata->ptrCseqLongSamples = (struct SampleInstrument *)addr;
	addr += sizeof(struct SampleInstrument) * ch->numLongSamples;

	sdata->ptrCseqShortSamples = (struct SampleDrums *)addr;
	numSongs = ch->numSongs;
	addr += sizeof(struct SampleDrums) * ch->numShortSamples;

	sdata->ptrCseqSongStartOffset = (s16 *)addr;
	addr += sizeof(s16) * numSongs;

	// NOTE(aalhendi): Retail stores the unaligned pointer before either correction branch.
	*(char *volatile *)&sdata->ptrCseqSongData = (char *)addr;
	if (addr & 1)
	{
		sdata->ptrCseqSongData = (char *)(addr + 1);
	}
	alignedAddr = (u32)sdata->ptrCseqSongData;
	if (alignedAddr & 2)
	{
		sdata->ptrCseqSongData = (char *)(alignedAddr + 2);
	}
}

int howl_LoadHeader(char *filename)
{
	struct HowlHeader *alloc;
	struct HowlHeader *header;
	int howlHeaderSize;
	int numSector;
	int ret;
	u32 magicPage;
	u32 headerMagic;
	u32 expectedMagic;

	if (LOAD_FindFile(filename, &GAME_HOWL_CD_FILE) == 0)
	{
		return 0;
	}

	MEMPACK_PushState();

	// allocate room for one sector
	alloc = MEMPACK_AllocMem(0x800, filename);

	if (alloc != 0)
	{
		// read sector #1 of HOWL, just for header
		ret = LOAD_HowlHeaderSectors(&GAME_HOWL_CD_FILE, alloc, 0, 1);
		header = alloc;

		if (ret != 0)
		{
			// NOTE(aalhendi): Retail forms the magic address before reading the header word.
			CTR_PSX_LOAD_SYMBOL_PAGE(magicPage, RETAIL_HOWL_MAGIC_ASM_NAME);
			headerMagic = header->magic;
			CTR_PSX_LOAD_WORD_FROM_PAGE(expectedMagic, magicPage, RETAIL_HOWL_MAGIC_ASM_NAME, CTR_ReadU32LE(sdata->s_HOWL));
			if (headerMagic != expectedMagic || header->version != 0x80)
				goto invalidHeader;

			// allocate room for howlHeader + pointerTable
			howlHeaderSize = sizeof(struct HowlHeader) + header->headerSize;

			// align up for sector size
			numSector = CTR_MipsSra(CTR_MipsAddLo(howlHeaderSize, 0x7ff), 11);
			MEMPACK_ReallocMem(numSector << 0xb);

			// One sector suffices, or the remaining sectors must load successfully.
			if (numSector < 2 || LOAD_HowlHeaderSectors(&GAME_HOWL_CD_FILE, (void *)((int)header + 0x800), 1, numSector - 1) != 0)
				goto headerLoaded;
		}
	}

invalidHeader:
	MEMPACK_PopState();
	return 0;

headerLoaded:
	howl_ParseHeader(header);
	// Drop sector-alignment padding, but retain the allocation and push state.
	MEMPACK_ReallocMem(howlHeaderSize);
	return 1;
}

int howl_SetSong(int songID)
{
	// NOTE(aalhendi): The sector value remains in v1 until the return value is prepared.
	register u32 offset CTR_PSX_REGISTER("$3");

	if (HOWL_LOAD_AUDIO_ENABLED_INITIAL == 0)
	{
		// Stage 3: Finished
		sdata->songLoadStage = 3;

		return 1;
	}

	// === Reset Song ===

	howl_ErasePtrCseqHeader();

	// Stage 0: Start Loading
	sdata->songLoadStage = 0;

	offset = sdata->howl_songOffsets[songID & 0xffff];
	sdata->songSectorOffset = offset;
	// Publish the selected sector before the return jump.
	CTR_PSX_MEMORY_BARRIER();
	return 1;
}

int howl_LoadSong(void)
{
	int ret;
	s32 numSectors;
	register char *remaining CTR_PSX_REGISTER("$5");
	register CdlFILE *file CTR_PSX_REGISTER("$4");
	int stageOne;
	int stageDone = 3;

	if (HOWL_LOAD_STAGE_INITIAL == stageDone)
	{
		return 1;
	}

	// NOTE(aalhendi): Retail dispatches once, then rereads the stage at the shared exit.
	{
		int stage = sdata->songLoadStage;
		stageOne = 1;
		if (stage == stageOne)
			goto stage1;
		if (stage < 2)
		{
			if (stage == 0)
				goto stage0;
			goto stageExit;
		}
		if (stage == 2)
			goto stage2;
		goto stageExit;
	}

stage0:
	ret = LOAD_HowlSectorChainStart(&GAME_HOWL_CD_FILE, HOWL_LOAD_SONG_BUFFER, sdata->songSectorOffset, 1);
	if (ret != 0)
	{
		sdata->songLoadStage = stageOne;
		CTR_PSX_OBSERVE_MEMORY(sdata->songLoadStage);
	}
	goto stageExit;

stage1:
	if (LOAD_HowlSectorChainEnd() == 0)
		goto stageExit;

	// NOTE(aalhendi): Form the CD-file and buffer arguments before reading the size.
	// Native names the following array directly; retail adds one sector to the base.
	file = &GAME_HOWL_CD_FILE;
	remaining = HOWL_LOAD_SONG_BUFFER;
	CTR_PSX_ORDER_VALUES(file, remaining);
	CTR_PSX_ADD_POINTER_IMMEDIATE(remaining, remaining, 0x800, sdata->tenSampleBlocks);
	// Round with 32-bit wrap, then retain zero or negative remaining-sector counts.
	numSectors = (CTR_ReadU32AlignedLE(HOWL_LOAD_SONG_BUFFER) + 0x7ffu) >> 11;
	ret = LOAD_HowlSectorChainStart(file, remaining, CTR_MipsAddLo(sdata->songSectorOffset, 1), numSectors - 1);
	if (ret != 0)
	{
		sdata->songLoadStage = 2;
		CTR_PSX_OBSERVE_MEMORY(sdata->songLoadStage);
	}
	goto stageExit;

stage2:
	if (LOAD_HowlSectorChainEnd() == 0)
		goto stageExit;

	howl_ParseCseqHeader((struct CseqHeader *)HOWL_LOAD_SONG_BUFFER);
	sdata->songLoadStage = stageDone;

stageExit:
	return sdata->songLoadStage == 3;
}

void howl_ErasePtrCseqHeader()
{
	// NOTE(aalhendi): Retail clears this pointer before the return delay slot.
	*(struct CseqHeader *volatile *)&sdata->ptrCseqHeader = 0;
}

u8 *howl_GetNextNote(u8 *currNote, int *noteLen)
{
	u8 *cursor = currNote;
	u32 noteLength = cursor[0];
	u32 nextByte;

	cursor++;
	if ((noteLength & 0x80) != 0)
	{
		// NOTE(aalhendi): The top bit continues a variable-length Cseq note delay.
		noteLength &= 0x7f;
		do
		{
			nextByte = *cursor++;
			noteLength = CTR_MipsAddLo(CTR_MipsSll(noteLength, 7), nextByte & 0x7f);
		} while ((nextByte & 0x80) != 0);
	}

	*noteLen = noteLength;
	return cursor;
}

void cseq_opcode00_empty(struct SongSeq *seq)
{
	(void)seq;
	// left empty by ND
}
