#include <common.h>

void Bank_ResetAllocator()
{
	sdata->numAudioBanks = 0;
	sdata->audioAllocPtr = 0x202;
	sdata->bankLoadStage = 4; // Stage 4: Finished
	CTR_PSX_OBSERVE_MEMORY(sdata->bankLoadStage);
}

int Bank_Alloc(int bankID, struct Bank *ptrBank)
{
	struct SampleBlockHeader *sampleBlock;
	int bankOffset;
	register int result CTR_PSX_REGISTER("$2");

	if (GAME_AUDIO_ENABLED == 0)
	{
		// Stage 4: Complete
		sdata->bankLoadStage = 4;
		return 1;
	}

	// is last bank needed for level?
	if (ptrBank->flags & 1)
	{
		sdata->bankFlags = 1;
		// NOTE(aalhendi): Keep this store ahead of the join's jump delay slot.
		CTR_PSX_OBSERVE_MEMORY(sdata->bankFlags);
	}
	else
	{
		sdata->bankFlags = 0;
	}

	bankOffset = GAME_HOWL_BANK_OFFSETS[bankID & 0xffff];
	sdata->ptrLastBank = ptrBank;
	sdata->bankSectorOffset = bankOffset;
	// NOTE(aalhendi): Retail publishes the sector offset before pushing the allocator state.
	CTR_PSX_OBSERVE_MEMORY(sdata->bankSectorOffset);

	// temporary for loading banks to RAM,
	// sending data to SPU, then erasing RAM
	MEMPACK_PushState();

	sampleBlock = MEMPACK_AllocMem(0x800, GAME_HOWL_SAMPLE_BLOCK_NAME);
	sdata->ptrSampleBlock2 = sampleBlock;
	// NOTE(aalhendi): Retail stores the allocation before testing it for failure.
	CTR_PSX_MEMORY_BARRIER();
	if (sampleBlock == 0)
	{
		MEMPACK_PopState();
		return 0;
	}

	// Stage 0: Start loading the bank into RAM, then transfer it to SPU.
	result = 1;
	// NOTE(aalhendi): Keep success in v0 for the allocation-failure branch slot.
	CTR_PSX_OBSERVE_VALUE(result);
	sdata->ptrSampleBlock1 = sampleBlock;
	sdata->bankLoadStage = 0;
	// NOTE(aalhendi): Finish the stage write before the return jump.
	CTR_PSX_MEMORY_BARRIER();
	return result;
}

int Bank_AssignSpuAddrs()
{
	s32 i;
	s32 ret;
	register s32 audioAllocPtr CTR_PSX_REGISTER("$6");
	register u32 numSectors CTR_PSX_REGISTER("$2");
	register u32 roundedSize CTR_PSX_REGISTER("$4");
	s32 spuAddrStart;
	struct SampleBlockHeader *sampleBlock;
	u32 sampleCount;
	struct SpuAddrEntry *sae;
	struct Bank *lastBank;
	register s32 stageOne CTR_PSX_REGISTER("$16");
	register s32 stageDone CTR_PSX_REGISTER("$17") = 4;

	// if Stage 4: Complete
	if (GAME_BANK_LOAD_STAGE_INITIAL == stageDone)
	{
		return 1;
	}

	// NOTE(aalhendi): Explicit dispatch keeps the saved stage-one comparison in retail order.
	{
		s32 stage = sdata->bankLoadStage;
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
		if (stage == 3)
			goto stage3;
		goto stageExit;
	}
// Stage 0: Load to RAM (1/2)
stage0:
	ret = LOAD_HowlSectorChainStart(&GAME_HOWL_CD_FILE,             // CdLoc of HOWL
	                                (void *)sdata->ptrSampleBlock2, // destination in RAM for banks
	                                sdata->bankSectorOffset,        // bank offset on disc, from CdLoc
	                                1                               // one sector
	);

	if (ret != 0)
	{
		// go to next stage
		sdata->bankLoadStage = stageOne;
		CTR_PSX_OBSERVE_MEMORY(sdata->bankLoadStage);
	}

	goto stageExit;

// Stage 1: Load to RAM (2/2) and assign SPU Addrs
stage1:
	if (LOAD_HowlSectorChainEnd() == 0)
	{
		goto stageExit;
	}

	sampleBlock = sdata->ptrSampleBlock1;
	sampleCount = sampleBlock->numSamples;
	sdata->audioAllocSize = 0;
	CTR_PSX_MEMORY_BARRIER();

	i = 0;
	if (sampleCount != 0)
	{
		register s32 count CTR_PSX_REGISTER("$3");
		do
		{
			u32 byteOffset = (u32)i << 1;
			u16 *sampleEntry;
			u16 sampleIndex;
			register u32 sampleSize CTR_PSX_REGISTER("$4");
			struct SpuAddrEntry *entry;
			// NOTE(aalhendi): Keep the retail indexed lookup; GCC otherwise advances a pointer.
			CTR_PSX_ADD_POINTER_OFFSET(sampleEntry, sampleBlock, byteOffset);
			// NOTE(aalhendi): The index advances after its address has been formed.
			CTR_PSX_DEPEND_VALUE(i, sampleEntry);
			i++;
			sampleIndex = sampleEntry[1];
			count = sampleBlock->numSamples;
			// NOTE(aalhendi): Both targets use 32-bit pointers; offset-first addition preserves retail operands.
			entry = (struct SpuAddrEntry *)(((u32)sampleIndex << 2) + (u32)GAME_HOWL_SPU_ADDRS);
			count = i < count;
			// NOTE(aalhendi): Retail compares the next index before reading the sample size.
			CTR_PSX_DEPEND_VALUE(entry, count);
			sampleSize = entry->spuSize;
			sdata->audioAllocSize += sampleSize << 3;
			CTR_PSX_OBSERVE_MEMORY(sdata->audioAllocSize);
		} while (count);
	}

	// not last bank needed for level
	if (sdata->bankFlags == 0)
	{
		sdata->ptrLastBank->max = (u32)sdata->audioAllocSize >> 3;
	}

	// last bank needed for level
	else
	{
		// NOTE(aalhendi): Retail compares this bank limit directly with the byte count.
		if ((u32)sdata->ptrLastBank->max < (u32)sdata->audioAllocSize)
		{
			// Stage 4: Complete
			sdata->bankLoadStage = 4;
			CTR_PSX_OBSERVE_MEMORY(sdata->bankLoadStage);
			return 1;
		}
	}

	// === more banks needed ===

	numSectors = ((u32)sdata->audioAllocSize + 0x7ff) >> 0xb;
	roundedSize = numSectors << 0xb;
	sdata->numAudioSectors = numSectors;
	CTR_PSX_OBSERVE_MEMORY(sdata->numAudioSectors);

	MEMPACK_ReallocMem(roundedSize + 0x800);

	ret = LOAD_HowlSectorChainStart(&GAME_HOWL_CD_FILE,                            // CdLoc of HOWL
	                                (void *)((int)sdata->ptrSampleBlock2 + 0x800), // destination
	                                sdata->bankSectorOffset + 1,                   // offset of howl
	                                sdata->numAudioSectors                         // number of sectors
	);

	if (ret == 0)
	{
		return 0;
	}

	// last bank needed?
	if (sdata->bankFlags != 0)
	{
		audioAllocPtr = sdata->ptrLastBank->min;
	}
	else
	{
		audioAllocPtr = (u16)sdata->audioAllocPtr;
		sdata->ptrLastBank->min = audioAllocPtr;
	}

	// === Assign SpuEntry for all "new" samples ===

	{
		s32 secondIndex;
		struct SampleBlockHeader *loopHeader;
		struct SpuAddrEntry *spuBase;
		loopHeader = sdata->ptrSampleBlock1;
		secondIndex = 0;
		if (loopHeader->numSamples != 0)
		{
			u32 more;
			spuBase = GAME_HOWL_SPU_ADDRS;
			do
			{
				u16 *spuIndexArr = SBHEADER_GETARR(loopHeader);
				u32 spuOffset = (u32)spuIndexArr[secondIndex] << 2;
				CTR_PSX_ADD_POINTER_OFFSET_OFFSET_FIRST(sae, spuBase, spuOffset);

				if (sae->spuAddr == 0)
				{
					sae->spuAddr = audioAllocPtr;
				}
				secondIndex++;
				loopHeader = sdata->ptrSampleBlock1;
				more = secondIndex < loopHeader->numSamples;
				audioAllocPtr += sae->spuSize;

			} while (more);
		}
	}

	sdata->bankLoadStage = 2;
	CTR_PSX_OBSERVE_MEMORY(sdata->bankLoadStage);

	goto stageExit;

// Stage 2: Spu Transfer Start
stage2:
	if (LOAD_HowlSectorChainEnd() == 0)
	{
		goto stageExit;
	}

	spuAddrStart = (u32)sdata->ptrLastBank->min * 8;

	// Keep the transfer below retail's SPU allocation limit.
	if ((u32)spuAddrStart + (u32)sdata->audioAllocSize < 0x7e000)
	{
		// start transfer
		SpuSetTransferStartAddr(spuAddrStart);

		SpuWrite((u8 *)((int)sdata->ptrSampleBlock2 + 0x800), (u32)sdata->audioAllocSize);
	}

	sdata->bankLoadStage = 3;
	CTR_PSX_OBSERVE_MEMORY(sdata->bankLoadStage);

	goto stageExit;

// Stage 3: Spu Transfer End
stage3:
	if (SpuIsTransferCompleted(SPU_TRANSFER_PEEK) == 0)
	{
		goto stageExit;
	}

	if (sdata->bankFlags == 0)
	{
		sdata->audioAllocPtr += (u32)sdata->audioAllocSize >> 3;
	}

	lastBank = sdata->ptrLastBank;
	lastBank->flags |= 2;

	// SPU Transfer done, remove bank from RAM
	MEMPACK_PopState();

	sdata->bankLoadStage = stageDone;
	goto stageExit;

stageExit:
	return sdata->bankLoadStage == 4;
}

void Bank_Destroy(struct Bank *ptrLastBank)
{
	int flags;

	if (GAME_AUDIO_ENABLED == 0)
	{
		return;
	}

	flags = ptrLastBank->flags & 1;

	Bank_ClearInRange(ptrLastBank->min, ptrLastBank->max);

	if (flags == 0)
	{
		// NOTE(aalhendi): Only the last bank can release the allocation tail.
		sdata->audioAllocPtr = ptrLastBank->min;
	}

	ptrLastBank->flags &= ~2;
}

void Bank_ClearInRange(u16 min, u16 max)
{
	u32 i;
	u32 count;
	struct HowlHeader *header;
	struct HowlHeader *loopHeader;
	struct SpuAddrEntry *sae;
	u32 lower;
	register u32 upper CTR_PSX_REGISTER("$4");
	register u32 sum CTR_PSX_REGISTER("$5");

	header = GAME_HOWL_HEADER;
	// NOTE(aalhendi): Retail loads the header before adding these bounds in order.
	CTR_PSX_ADD_U32(sum, min, max);
	CTR_PSX_DEPEND_VALUE(sum, header);
	if (header->numSpuAddrs == 0)
	{
		return;
	}
	i = 0;
	lower = (u16)min;
	upper = (u16)sum;
	loopHeader = header;
	sae = GAME_HOWL_SPU_ADDRS;
	do
	{
		if (sae->spuAddr >= lower && sae->spuAddr < upper)
		{
			sae->spuAddr = 0;
		}
		count = (u32)loopHeader->numSpuAddrs;
		// NOTE(aalhendi): Reload the count before advancing the loop index.
		CTR_PSX_OBSERVE_VALUE(count);
		i++;
		sae++;
	} while (i < count);
}

int Bank_Load(int bankID, struct Bank *ptrBank)
{
	// NOTE(aalhendi): These register lifetimes preserve retail's argument copy and bank-array base.
	register int bankForAlloc CTR_PSX_REGISTER("$6");
	register struct Bank *bankBase CTR_PSX_REGISTER("$3");
	u32 countCheck;
	u32 numBanks;

	// NOTE(aalhendi): The pointer dependency leaves the first count load's
	// delay slot available for the a0-to-a2 copy.
	GAME_AUDIO_BANK_COUNT_LOAD_AFTER(countCheck, ptrBank);
	CTR_PSX_COPY_VALUE(bankForAlloc, bankID);

	if (countCheck >= 8)
	{
		return 0;
	}
	// NOTE(aalhendi): Retail rereads the count after checking capacity.
	CTR_PSX_RELOAD(sdata->numAudioBanks);
	numBanks = GAME_AUDIO_BANK_COUNT;
	bankBase = GAME_AUDIO_BANKS;
	bankBase[numBanks].bankID = bankID & 0xffff;
	if ((bankBase[numBanks].flags & 3) != 0)
	{
		return 0;
	}
	if (Bank_Alloc((u16)bankForAlloc, &bankBase[numBanks]) == 0)
	{
		return 0;
	}
	// NOTE(aalhendi): Retail returns the bank index in the first byte only.
	*(u8 *)ptrBank = GAME_AUDIO_BANK_COUNT++;
	return 1;
}

int Bank_DestroyLast()
{
	u8 count = GAME_AUDIO_BANK_COUNT;

	if (count != 0)
	{
		count--;
		GAME_AUDIO_BANK_COUNT = count;
		Bank_Destroy(&GAME_AUDIO_BANKS[count]);
		return 1;
	}
	return 0;
}

void Bank_DestroyUntilIndex(int index)
{
	struct Bank *ptrLastBank;
	u16 bankID = index;
	u32 slot;

	while (GAME_AUDIO_BANK_COUNT != 0)
	{
		slot = GAME_AUDIO_BANK_COUNT;
		slot--;
		// NOTE(aalhendi): Keep the decrement before the shift instead of folding it into the array base.
		CTR_PSX_OBSERVE_VALUE(slot);
		ptrLastBank = &GAME_AUDIO_BANKS[slot];

		if ((u16)ptrLastBank->bankID == bankID)
		{
			return;
		}

		Bank_DestroyLast();
	}
}

void Bank_DestroyAll()
{
	u8 count = GAME_AUDIO_BANK_COUNT;

	if (count == 0)
	{
		return;
	}
	do
	{
		Bank_DestroyLast();
		count = GAME_AUDIO_BANK_COUNT;
	} while (count != 0);
}
