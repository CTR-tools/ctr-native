#include <common.h>
#include <platform/native_cd.h>
#include <platform/native_spu_memory.h>

enum { HOWL_SAMPLE_SPU_UNIT = 8 };

static bool Bank_FindSample(u16 sampleIndex, u32 *sampleOffset)
{
	u16 sector[LOAD_CD_DATA_SECTOR_SIZE / sizeof(u16)];
	for (int bank = 0; bank < sdata->ptrHowlHeader->numBanks; bank++)
	{
		const u32 firstSector = sdata->howl_bankOffsets[bank];
		if (!NativeCD_ReadFileSectorsAt(&sdata->KartHWL_CdFile, firstSector, 1, sector))
		{
			return false;
		}
		const u16 count = sector[0];
		if (count > sizeof(sector) / sizeof(sector[0]) - 1)
		{
			return false;
		}
		u32 offset = (firstSector + 1) * LOAD_CD_DATA_SECTOR_SIZE;
		for (int i = 0; i < count; i++)
		{
			const u16 index = sector[i + 1];
			if (index >= sdata->ptrHowlHeader->numSpuAddrs)
			{
				return false;
			}
			if (index == sampleIndex)
			{
				*sampleOffset = offset;
				return true;
			}
			offset += sdata->howl_spuAddrs[index].spuSize * HOWL_SAMPLE_SPU_UNIT;
		}
	}
	return false;
}

static bool Bank_UploadSample(u32 source, u32 destination, u32 size)
{
	u8 sector[LOAD_CD_DATA_SECTOR_SIZE];
	while (size != 0)
	{
		const u32 skip = source % LOAD_CD_DATA_SECTOR_SIZE;
		u32 count = LOAD_CD_DATA_SECTOR_SIZE - skip;
		if (count > size)
		{
			count = size;
		}
		if (!NativeCD_ReadFileSectorsAt(&sdata->KartHWL_CdFile, source / LOAD_CD_DATA_SECTOR_SIZE, 1, sector))
		{
			return false;
		}
		SpuSetTransferStartAddr(destination);
		if (SpuWrite(sector + skip, count) != count)
		{
			return false;
		}
		while (!SpuIsTransferCompleted(SPU_TRANSFER_PEEK))
		{
		}
		source += count;
		destination += count;
		size -= count;
	}
	return true;
}

bool Bank_EnsureOtherFX(u16 soundID)
{
	if (!sdata->boolAudioEnabled || sdata->ptrHowlHeader == NULL ||
	    soundID >= sdata->ptrHowlHeader->numOtherFX)
	{
		return false;
	}
	const u16 index = sdata->howl_metaOtherFX[soundID].spuIndex;
	if (index >= sdata->ptrHowlHeader->numSpuAddrs)
	{
		return false;
	}
	struct SpuAddrEntry *sample = &sdata->howl_spuAddrs[index];
	if (sample->spuAddr != 0)
	{
		return true;
	}
	if (sdata->bankLoadStage != 4 || sdata->songLoadStage != 3 || sdata->numAudioBanks == 0)
	{
		return false;
	}

	// A dedicated bank gives cached samples the existing level-teardown lifetime.
	const int last = sdata->numAudioBanks - 1;
	const bool create = sdata->bank[last].bankID != HOWL_BANK_LEVEL_CACHE;
	if (create && sdata->numAudioBanks >= (int)len(sdata->bank))
	{
		return false;
	}
	const u32 size = sample->spuSize * HOWL_SAMPLE_SPU_UNIT;
	const u32 destination = CTR_SPU_BYTES_AT((u32)sdata->audioAllocPtr);
	if (size == 0 || size % (1 << CTR_SPU_ADDR_SHIFT) != 0 || destination + size >= CTR_SPU_CEILING)
	{
		return false;
	}
	u32 source;
	if (!Bank_FindSample(index, &source) || !Bank_UploadSample(source, destination, size))
	{
		return false;
	}
	if (create)
	{
		sdata->bank[sdata->numAudioBanks++] = (struct Bank){
			.bankID = HOWL_BANK_LEVEL_CACHE, .flags = 2, .min = sdata->audioAllocPtr,
		};
	}
	sample->spuAddr = sdata->audioAllocPtr;
	sdata->audioAllocPtr += CTR_SPU_SIZE_UNITS(sample->spuSize);
	sdata->bank[sdata->numAudioBanks - 1].max += CTR_SPU_SIZE_UNITS(sample->spuSize);
	return true;
}
