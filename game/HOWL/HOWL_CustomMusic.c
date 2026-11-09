/* Saphi SCA bank loading, adapted from ctr-native-ap HOWL_CustomMusic.c. */
#include <platform/native_track_file.h>
#include <platform/native_log.h>
#include <platform/native_spu_memory.h>
#include <stdlib.h>
#include <string.h>

enum { CUSTOM_SCA_SECTOR = 2048, CUSTOM_SCA_LIMIT = 1024 * 1024,
       CUSTOM_SCA_MAX_SAMPLES = 1023, CUSTOM_SCA_MAX_SONG = 65536,
       CUSTOM_SCA_SECTOR_BASE = 0x10000000, CUSTOM_SCA_MAX_PLAYERS = 4 };

static unsigned char *s_scaBytes;
static const unsigned char *s_scaBank, *s_scaSong, *s_scaSizes;
static size_t s_scaBankSize, s_scaSongSize;
static u16 s_scaIDs[CUSTOM_SCA_MAX_SAMPLES], s_scaOldSizes[CUSTOM_SCA_MAX_SAMPLES];
static unsigned int s_scaCount;
static int s_scaServing, s_scaLoaded;
static s16 s_scaLater[1 + CUSTOM_SCA_MAX_PLAYERS][CUSTOM_SCA_SECTOR / 2];
static union
{
	struct CseqHeader header;
	unsigned char bytes[CUSTOM_SCA_MAX_SONG];
} s_scaSongBuffer;

static unsigned int CustomSca_U16(const unsigned char *p)
{
	return p[0] | ((unsigned int)p[1] << 8);
}

static u32 CustomSca_U32(const unsigned char *p)
{
	return CustomSca_U16(p) | ((u32)CustomSca_U16(p + 2) << 16);
}

static void CustomSca_Reset(void)
{
	while (s_scaCount != 0)
	{
		s_scaCount--;
		sdata->howl_spuAddrs[s_scaIDs[s_scaCount]].spuSize = s_scaOldSizes[s_scaCount];
	}
	free(s_scaBytes);
	s_scaBytes = NULL;
	s_scaBank = NULL;
	s_scaSong = NULL;
	s_scaSizes = NULL;
	s_scaServing = 0;
	s_scaLoaded = 0;
}

static int CustomSca_Parse(size_t size, unsigned int slots, unsigned int *countOut, unsigned int *unitsOut)
{
	const unsigned char *bank = NULL, *song = NULL, *sizes = NULL;
	size_t bankSize = 0, songSize = 0, sizesSize = 0, pos = 4;
	if (size < 4 || memcmp(s_scaBytes, "SCA\1", 4) != 0)
	{
		return 0;
	}
	while (pos < size)
	{
		if (size - pos < 8)
		{
			return 0;
		}
		const unsigned char *tag = s_scaBytes + pos;
		size_t chunk = CustomSca_U32(tag + 4);
		if (chunk > size - pos - 8)
		{
			return 0;
		}
		const unsigned char **target = NULL;
		size_t *targetSize = NULL;
		if (memcmp(tag, "BANK", 4) == 0) { target = &bank; targetSize = &bankSize; }
		else if (memcmp(tag, "CSEQ", 4) == 0) { target = &song; targetSize = &songSize; }
		else if (memcmp(tag, "SIZE", 4) == 0) { target = &sizes; targetSize = &sizesSize; }
		if (target != NULL)
		{
			if (*target != NULL) { return 0; }
			*target = tag + 8;
			*targetSize = chunk;
		}
		pos += 8 + chunk;
		if (pos < size)
		{
			pos = (pos + 3) & ~(size_t)3;
			if (pos > size) { return 0; }
		}
	}
	if (bank == NULL || song == NULL || sizes == NULL || bankSize <= CUSTOM_SCA_SECTOR || songSize < 8)
	{
		return 0;
	}
	unsigned int count = CustomSca_U16(bank), units = 0;
	if (count == 0 || count > CUSTOM_SCA_MAX_SAMPLES || sizesSize != count * 2u)
	{
		return 0;
	}
	for (unsigned int i = 0; i < count; i++)
	{
		unsigned int id = CustomSca_U16(bank + 2 + i * 2), n = CustomSca_U16(sizes + i * 2);
		if (id >= slots || n == 0 || (n & 1) != 0) { return 0; }
		for (unsigned int j = 0; j < i; j++)
		{
			if (CustomSca_U16(bank + 2 + j * 2) == id) { return 0; }
		}
		units += n;
	}
	if ((size_t)units * 8 > bankSize - CUSTOM_SCA_SECTOR) { return 0; }
	size_t realSongSize = CustomSca_U32(song);
	size_t tables = 8 + 12u * song[4] + 8u * song[5] + 2u * CustomSca_U16(song + 6);
	if (realSongSize < 8 || realSongSize > songSize || realSongSize > CUSTOM_SCA_MAX_SONG ||
	    CustomSca_U16(song + 6) == 0 || tables >= realSongSize) { return 0; }
	s_scaBank = bank;
	s_scaBankSize = bankSize;
	s_scaSong = song;
	s_scaSongSize = realSongSize;
	s_scaSizes = sizes;
	*countOut = count;
	*unitsOut = units;
	return 1;
}

static int CustomSca_LoadBankFrom(struct Bank *bank, const struct NativeTrackRegistry *registry, int level)
{
	struct NativeTrackFile *file = NativeTrackFile_Open(registry, level, NATIVE_TRACK_FILE_SCA);
	if (file == NULL) { return 0; }
	size_t size = NativeTrackFile_Size(file), padded = NativeTrackFile_SectorBytes(file);
	if (size > CUSTOM_SCA_LIMIT || (s_scaBytes = malloc(padded)) == NULL || !NativeTrackFile_Read(file, s_scaBytes, padded))
	{
		NativeTrackFile_Close(file);
		CustomSca_Reset();
		return 0;
	}
	NativeTrackFile_Close(file);
	unsigned int count, units;
	if (!CustomSca_Parse(size, sdata->ptrHowlHeader->numSpuAddrs, &count, &units))
	{
		Platform_LogWarn("[CUSTOM MUSIC] Invalid SCA; using default audio\n");
		CustomSca_Reset();
		return 0;
	}
	unsigned long long total = (unsigned int)sdata->audioAllocPtr * 2u + units;
	unsigned int laterCount = 0;
	int bankIDs[1 + CUSTOM_SCA_MAX_PLAYERS];
	/* Match the actual stage 1/2 bank choices. */
	int has54 = (sdata->gGT->gameMode1 & ARCADE_MODE) != 0;
	if (has54) { bankIDs[laterCount++] = 54; }
	for (int i = 0; i < sdata->gGT->numPlyrCurrGame && i < CUSTOM_SCA_MAX_PLAYERS; i++)
	{
		if (!has54 || data.characterIDs[i] >= PINSTRIPE) { bankIDs[laterCount++] = data.characterIDs[i] + 0x37; }
	}
	for (unsigned int i = 0; i < count; i++)
	{
		unsigned int id = CustomSca_U16(s_scaBank + 2 + i * 2);
		if (sdata->howl_spuAddrs[id].spuAddr != 0 && sdata->howl_spuAddrs[id].spuSize != CustomSca_U16(s_scaSizes + i * 2))
		{
			CustomSca_Reset(); return 0;
		}
	}
	for (unsigned int b = 0; b < laterCount; b++)
	{
		int id = bankIDs[b];
		if (id < 0 || id >= sdata->ptrHowlHeader->numBanks || !LOAD_HowlHeaderSectors(&sdata->KartHWL_CdFile, s_scaLater[b], sdata->howl_bankOffsets[id], 1))
		{
			CustomSca_Reset(); return 0;
		}
		int n = s_scaLater[b][0];
		if (n < 0 || n > CUSTOM_SCA_MAX_SAMPLES) { CustomSca_Reset(); return 0; }
		for (int j = 0; j < n; j++)
		{
			int slot = s_scaLater[b][j + 1];
			if (slot < 0 || slot >= sdata->ptrHowlHeader->numSpuAddrs) { CustomSca_Reset(); return 0; }
			for (unsigned int i = 0; i < count; i++)
			{
				if ((int)CustomSca_U16(s_scaBank + 2 + i * 2) == slot && CustomSca_U16(s_scaSizes + i * 2) != sdata->howl_spuAddrs[slot].spuSize)
				{
					CustomSca_Reset(); return 0;
				}
			}
			total += sdata->howl_spuAddrs[slot].spuSize;
		}
	}
	if (total * 8 >= CTR_SPU_CEILING) { CustomSca_Reset(); return 0; }
	for (unsigned int i = 0; i < count; i++)
	{
		unsigned int id = CustomSca_U16(s_scaBank + 2 + i * 2);
		s_scaIDs[s_scaCount] = (u16)id;
		s_scaOldSizes[s_scaCount++] = sdata->howl_spuAddrs[id].spuSize;
		sdata->howl_spuAddrs[id].spuSize = (u16)CustomSca_U16(s_scaSizes + i * 2);
	}
	if (Bank_Load(data.levBank_FX[CRASH_COVE], bank) == 0) { CustomSca_Reset(); return 0; }
	sdata->bankSectorOffset = CUSTOM_SCA_SECTOR_BASE;
	s_scaServing = 1;
	s_scaLoaded = 1;
	Platform_Log("[CUSTOM MUSIC] Loaded track audio (%u samples)\n", count);
	return 1;
}

static int CustomSca_Serve(void *destination, int sector, int count)
{
	if (!s_scaServing || sector < CUSTOM_SCA_SECTOR_BASE || count <= 0) { return 0; }
	size_t start = (size_t)(sector - CUSTOM_SCA_SECTOR_BASE) * CUSTOM_SCA_SECTOR;
	size_t length = (size_t)count * CUSTOM_SCA_SECTOR;
	memset(destination, 0, length);
	if (start < s_scaBankSize)
	{
		size_t available = s_scaBankSize - start;
		memcpy(destination, s_scaBank + start, available < length ? available : length);
	}
	return 1;
}

static int CustomSca_SetSong(void)
{
	if (!s_scaLoaded) { return 0; }
	howl_ErasePtrCseqHeader();
	memcpy(s_scaSongBuffer.bytes, s_scaSong, s_scaSongSize);
	howl_ParseCseqHeader(&s_scaSongBuffer.header);
	sdata->songLoadStage = 3;
	return 1;
}
