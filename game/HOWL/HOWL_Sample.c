#include <common.h>
#include <platform/native_cd.h>
#include <platform/native_spu_memory.h>
#include <platform/native_language_disc.h>

enum { HOWL_SAMPLE_SPU_UNIT = 8 };

enum { LOCAL_VOICE_FIRST = 28, LOCAL_VOICE_COUNT = 32, LOCAL_VOICE_FIRST_LANGUAGE = NATIVE_LANGUAGE_FRENCH, LOCAL_VOICE_LANGUAGES = NATIVE_LANGUAGE_DUTCH - NATIVE_LANGUAGE_FRENCH + 1 };

static bool Bank_CacheCanAllocate(void)
{
	if (sdata->bankLoadStage != 4 || sdata->songLoadStage != 3 || sdata->numAudioBanks == 0)
	{
		return false;
	}
	return sdata->bank[sdata->numAudioBanks - 1].bankID == HOWL_BANK_LEVEL_CACHE ||
	       sdata->numAudioBanks < (int)len(sdata->bank);
}

static void Bank_CacheCommit(u32 allocation)
{
	// The cache bank shares the existing level-teardown lifetime.
	if (sdata->bank[sdata->numAudioBanks - 1].bankID != HOWL_BANK_LEVEL_CACHE)
	{
		sdata->bank[sdata->numAudioBanks++] = (struct Bank){
			.bankID = HOWL_BANK_LEVEL_CACHE, .flags = 2, .min = sdata->audioAllocPtr,
		};
	}
	sdata->audioAllocPtr += CTR_SPU_UNITS(allocation);
	sdata->bank[sdata->numAudioBanks - 1].max += CTR_SPU_UNITS(allocation);
}

struct BankLocalizedVoice
{
	struct OtherFX meta;
	u16 addr;
};

static struct BankLocalizedVoice s_bankLocalizedVoices[LOCAL_VOICE_LANGUAGES][LOCAL_VOICE_COUNT];

void Bank_InvalidateLocalizedVoices(void)
{
	memset(s_bankLocalizedVoices, 0, sizeof(s_bankLocalizedVoices));
}

void Bank_ClearLocalizedVoices(u16 min, u16 size)
{
	u32 end = (u32)min + size;
	for (int language = 0; language < LOCAL_VOICE_LANGUAGES; language++)
	{
		for (int i = 0; i < LOCAL_VOICE_COUNT; i++)
		{
			struct BankLocalizedVoice *voice = &s_bankLocalizedVoices[language][i];
			if (voice->addr >= min && voice->addr < end)
			{
				voice->addr = 0;
			}
		}
	}
}

bool Bank_LocalizedVoice(u16 soundID, struct OtherFX *meta, u16 *spuAddr)
{
	int language = g_config.voiceLanguage - LOCAL_VOICE_FIRST_LANGUAGE;
	int index = (int)soundID - LOCAL_VOICE_FIRST;
	if (language < 0 || language >= LOCAL_VOICE_LANGUAGES || index < 0 || index >= LOCAL_VOICE_COUNT ||
	    meta == NULL || spuAddr == NULL || !sdata->boolAudioEnabled)
	{
		return false;
	}
	struct BankLocalizedVoice *cached = &s_bankLocalizedVoices[language][index];
	if (cached->addr == 0)
	{
		if (!Bank_CacheCanAllocate())
		{
			return false;
		}
		struct NativeLanguageVoiceSample sample;
		if (!NativeLanguageDisc_ReadVoiceSample(g_config.voiceLanguage, soundID, &sample))
		{
			return false;
		}
		u32 destination = CTR_SPU_BYTES_AT((u32)sdata->audioAllocPtr);
		u32 allocation = (sample.size + (1u << CTR_SPU_ADDR_SHIFT) - 1) & ~((1u << CTR_SPU_ADDR_SHIFT) - 1);
		if (sample.size == 0 || sample.size % HOWL_SAMPLE_SPU_UNIT != 0 || destination + allocation >= CTR_SPU_CEILING)
		{
			free(sample.data);
			return false;
		}
		SpuSetTransferStartAddr(destination);
		u32 written = SpuWrite(sample.data, sample.size);
		free(sample.data);
		if (written != sample.size)
		{
			return false;
		}
		while (!SpuIsTransferCompleted(SPU_TRANSFER_PEEK)) {}
		cached->addr = sdata->audioAllocPtr;
		cached->meta = (struct OtherFX){.flags = sample.flags, .volume = sample.volume, .pitch = sample.pitch, .duration = sample.duration};
		Bank_CacheCommit(allocation);
	}
	*meta = cached->meta;
	*spuAddr = cached->addr;
	return true;
}

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
	if (!Bank_CacheCanAllocate())
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
	sample->spuAddr = sdata->audioAllocPtr;
	Bank_CacheCommit(size);
	return true;
}
