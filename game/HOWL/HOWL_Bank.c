#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800292e0-0x800292fc
void Bank_ResetAllocator()
{
	sdata->numAudioBanks = 0;
	sdata->audioAllocPtr = 0x202;
	sdata->bankLoadStage = 4; // Stage 4: Finished
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800292fc-0x800293b8
int Bank_Alloc(int bankID, struct Bank *ptrBank)
{
	if (sdata->boolAudioEnabled == 0)
	{
		// Stage 4: Complete
		sdata->bankLoadStage = 4;
		return 1;
	}

	// is last bank needed for level?
	sdata->bankFlags = (ptrBank->flags & 1) != 0;

	sdata->bankSectorOffset = sdata->howl_bankOffsets[bankID & 0xffff];

	// ghidra makes this look like a pointer to stack memory,
	// game shows it's a pointer to ram bank[8], what's happening?
	sdata->ptrLastBank = ptrBank;

	// temporary for loading banks to RAM,
	// sending data to SPU, then erasing RAM
	MEMPACK_PushState();

	sdata->ptrSampleBlock2 = MEMPACK_AllocMem(0x800 /*, "SampleBlock"*/);

	if (sdata->ptrSampleBlock2 == 0)
	{
		// no data loaded, PopState
		MEMPACK_PopState();
		return 0;
	}

	// Stage 0: Start chain of events
	// to parse banks and ship to SPU
	sdata->bankLoadStage = 0;

	sdata->ptrSampleBlock1 = sdata->ptrSampleBlock2;
	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800293b8-0x800296c4
int Bank_AssignSpuAddrs()
{
	int i;
	int ret;
	int audioAllocPtr;

	// if Stage 4: Complete
	if (sdata->bankLoadStage == 4)
	{
		return 1;
	}

	// Stage 0: Load to RAM (1/2)
	if (sdata->bankLoadStage == 0)
	{
		ret = LOAD_HowlSectorChainStart(&sdata->KartHWL_CdFile,         // CdLoc of HOWL
		                                (void *)sdata->ptrSampleBlock2, // destination in RAM for banks
		                                sdata->bankSectorOffset,        // bank offset on disc, from CdLoc
		                                1                               // one sector
		);

		if (ret != 0)
		{
			// go to next stage
			sdata->bankLoadStage++;
		}

		return 0;
	}

	// Stage 1: Load to RAM (2/2) and assign SPU Addrs
	if (sdata->bankLoadStage == 1)
	{
		if (LOAD_HowlSectorChainEnd() == 0)
		{
			return 0;
		}

		sdata->audioAllocSize = 0;

		for (i = 0; i < sdata->ptrSampleBlock1->numSamples; i++)
		{
			s16 *spuIndexArr = SBHEADER_GETARR(sdata->ptrSampleBlock1);
			sdata->audioAllocSize += sdata->howl_spuAddrs[spuIndexArr[i]].spuSize;
		}

		// convert bit-shifted count to
		// real SPU byte count, with x8
		sdata->audioAllocSize *= 8;

		// not last bank needed for level
		if (sdata->bankFlags == 0)
		{
			sdata->ptrLastBank->max = sdata->audioAllocSize >> 3;
		}

		// last bank needed for level
		else
		{
			// Naughty Dog bug? No bitshift?
			if (sdata->ptrLastBank->max < sdata->audioAllocSize)
			{
				// Stage 4: Complete
				sdata->bankLoadStage = 4;
				return 1;
			}
		}

		// === more banks needed ===

		sdata->numAudioSectors = (sdata->audioAllocSize + 0x7ff) >> 0xb;

		MEMPACK_ReallocMem(((sdata->audioAllocSize + 0x7ff) & 0xfffff800) + 0x800);

		ret = LOAD_HowlSectorChainStart(&sdata->KartHWL_CdFile,                        // CdLoc of HOWL
		                                (void *)((int)sdata->ptrSampleBlock2 + 0x800), // destination
		                                sdata->bankSectorOffset + 1,                   // offset of howl
		                                sdata->numAudioSectors                         // number of sectors
		);

		if (ret == 0)
		{
			return 0;
		}

		// not last bank needed?
		if (sdata->bankFlags == 0)
		{
			sdata->ptrLastBank->min = sdata->audioAllocPtr;
			audioAllocPtr = sdata->audioAllocPtr;
		}

		// last bank needed
		else
		{
			audioAllocPtr = sdata->ptrLastBank->min;
		}

		// === Assign SpuEntry for all "new" samples ===

		struct SpuAddrEntry *sae;

#if 0
		printf("New\n");
		printf("%08x\n", sdata->audioAllocPtr);
#endif

		for (i = 0; i < sdata->ptrSampleBlock1->numSamples; i++)
		{
			s16 *spuIndexArr = SBHEADER_GETARR(sdata->ptrSampleBlock1);
			sae = &sdata->howl_spuAddrs[spuIndexArr[i]];

			if (sae->spuAddr == 0)
			{
				sae->spuAddr = audioAllocPtr;
			}
			audioAllocPtr += sae->spuSize;

#if 0
			printf("%08x\n", audioAllocPtr);
#endif
		}

		sdata->bankLoadStage++;

		return 0;
	}

	// Stage 2: Spu Transfer Start
	if (sdata->bankLoadStage == 2)
	{
		if (LOAD_HowlSectorChainEnd() == 0)
		{
			return 0;
		}

		int spuAddrStart = (u32)sdata->ptrLastBank->min * 8;

		// 0x7e000 = 512kb SPU memory
		if (spuAddrStart + sdata->audioAllocSize < 0x7e000)
		{
			// start transfer
			SpuSetTransferStartAddr(spuAddrStart);

			SpuWrite((u8 *)((int)sdata->ptrSampleBlock2 + 0x800), (size_t)sdata->audioAllocSize);
		}

		sdata->bankLoadStage++;

		return 0;
	}

	// Stage 3: Spu Transfer End
	if (sdata->bankLoadStage == 3)
	{
		if (SpuIsTransferCompleted(SPU_TRANSFER_PEEK) == 0)
		{
			return 0;
		}

		if (sdata->bankFlags == 0)
		{
			sdata->audioAllocPtr += sdata->audioAllocSize >> 3;
		}

		sdata->ptrLastBank->flags |= 2;

		// SPU Transfer done, remove bank from RAM
		MEMPACK_PopState();

		sdata->bankLoadStage++;
		return 1;
	}

	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800296c4-0x80029730
void Bank_Destroy(struct Bank *ptrLastBank)
{
	u16 flags;

	if (sdata->boolAudioEnabled == 0)
	{
		return;
	}

	flags = ptrLastBank->flags;

	Bank_ClearInRange(ptrLastBank->min, ptrLastBank->max);

	if ((flags & 1) == 0)
	{
		// this works cause Bank_Destroy
		// is only called on the "last" bank
		sdata->audioAllocPtr = ptrLastBank->min;
	}

	ptrLastBank->flags = flags & ~(2);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80029730-0x800297a0
void Bank_ClearInRange(u16 min, u16 max)
{
	int i;
	u16 end = min + max;
	struct SpuAddrEntry *sae;
	sae = &sdata->howl_spuAddrs[0];

	for (i = 0; i < sdata->ptrHowlHeader->numSpuAddrs; i++)
	{
		if (sae[i].spuAddr < min)
		{
			continue;
		}
		if (sae[i].spuAddr >= end)
		{
			continue;
		}
		sae[i].spuAddr = 0;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800297a0-0x80029824
int Bank_Load(int bankID, struct Bank *ptrBank)
{
	int numBanks = sdata->numAudioBanks;

	// if out of banks, quit
	if (numBanks >= 8)
	{
		return 0;
	}

	sdata->bank[numBanks].bankID = bankID & 0xffff;

	// if bank is in use, quit
	if ((sdata->bank[numBanks].flags & 3) != 0)
	{
		return 0;
	}

	if (Bank_Alloc(bankID, &sdata->bank[numBanks]) == 0)
	{
		return 0;
	}

	// starting to think this isn't really a bank...
	ptrBank->bankID = sdata->numAudioBanks++;
	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80029824-0x80029870
int Bank_DestroyLast()
{
	if (sdata->numAudioBanks == 0)
	{
		return 0;
	}

	Bank_Destroy(&sdata->bank[--sdata->numAudioBanks]);
	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80029870-0x800298e4
void Bank_DestroyUntilIndex(int index)
{
	struct Bank *ptrLastBank;
	u16 bankID = index;

	while (sdata->numAudioBanks != 0)
	{
		ptrLastBank = &sdata->bank[sdata->numAudioBanks - 1];

		if ((u16)ptrLastBank->bankID == bankID)
		{
			return;
		}

		Bank_DestroyLast();
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800298e4-0x8002991c
void Bank_DestroyAll()
{
	struct Bank *ptrLastBank;

	while (sdata->numAudioBanks != 0)
	{
		Bank_DestroyLast();
	}
}
