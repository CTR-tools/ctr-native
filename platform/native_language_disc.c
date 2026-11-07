#include <platform/native_language_disc.h>
#include <platform/native_disc_image.h>
#include <namespace_Lng.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum
{
	NATIVE_LANGUAGE_DISC_SECTOR_SIZE = 2048,
	NATIVE_LANGUAGE_DISC_BIG_HEADER_SIZE = 8,
	NATIVE_LANGUAGE_DISC_BIG_ENTRY_SIZE = 8,
	NATIVE_LANGUAGE_DISC_FIRST_ENTRY = 234,
	NATIVE_LANGUAGE_DISC_FIRST_SLOT = NATIVE_LANGUAGE_FRENCH,
	NATIVE_LANGUAGE_DISC_LAST_SLOT = NATIVE_LANGUAGE_DUTCH,
	NATIVE_LANGUAGE_DISC_FILE_HEADER_SIZE = 8,
	NATIVE_LANGUAGE_DISC_MAX_TEXT_SIZE = 1024 * 1024,
	NATIVE_LANGUAGE_DISC_US_STRINGS = 588,
	NATIVE_LANGUAGE_DISC_PAL_INSERT_FIRST = 547,
	NATIVE_LANGUAGE_DISC_PAL_INSERT_SECOND = 548,
	NATIVE_LANGUAGE_DISC_PAL_INSERT_THIRD = 553,
	NATIVE_LANGUAGE_DISC_VOICE_CHARACTERS = 16,
	NATIVE_LANGUAGE_DISC_US_VOICES_PER_CHARACTER = 19,
	NATIVE_LANGUAGE_DISC_PAL_VOICES_PER_CHARACTER = 16,
	NATIVE_LANGUAGE_DISC_DUPLICATED_VOICE_CHARACTER = 10,
	NATIVE_LANGUAGE_DISC_DUPLICATED_VOICE_COUNT = 2,
};

int NativeLanguageDisc_PalIndex(int usIndex)
{
	if (usIndex < 0 || usIndex >= NATIVE_LANGUAGE_DISC_US_STRINGS)
	{
		return -1;
	}
	return usIndex + (usIndex >= NATIVE_LANGUAGE_DISC_PAL_INSERT_FIRST) + (usIndex >= NATIVE_LANGUAGE_DISC_PAL_INSERT_SECOND) +
	       (usIndex >= NATIVE_LANGUAGE_DISC_PAL_INSERT_THIRD);
}

static int NativeLanguageDisc_PalVoiceIndex(int usIndex)
{
	// The retail executable's per-character voice tables have eight pairs in
	// both regions; NTSC-U has three additional single-clip sets per character.
	// PAL omits those sets, so they must use the original English resources.
	if (usIndex < 0 || usIndex >= NATIVE_LANGUAGE_DISC_VOICE_CHARACTERS * NATIVE_LANGUAGE_DISC_US_VOICES_PER_CHARACTER)
	{
		return -1;
	}
	int character = usIndex / NATIVE_LANGUAGE_DISC_US_VOICES_PER_CHARACTER;
	int voice = usIndex % NATIVE_LANGUAGE_DISC_US_VOICES_PER_CHARACTER;
	if (voice >= NATIVE_LANGUAGE_DISC_PAL_VOICES_PER_CHARACTER ||
	    (character == NATIVE_LANGUAGE_DISC_DUPLICATED_VOICE_CHARACTER && voice < NATIVE_LANGUAGE_DISC_DUPLICATED_VOICE_COUNT))
	{
		return -1;
	}
	return character * NATIVE_LANGUAGE_DISC_PAL_VOICES_PER_CHARACTER + voice;
}

// Regional XA correspondence checked against Turbocharged, revision 07d0c49:
// https://github.com/CameronRedmore/Crash-Team-Racing-Turbocharged/blob/07d0c49f01d62dcb9887b455be27ce65728ee893/platform/native_audio.c
int NativeLanguageDisc_PalXaIndex(int category, int usIndex)
{
	enum
	{
		XA_EXTRA = 1, XA_GAME = 2,
		EXTRA_LAST_UNCHANGED = 0x3d, EXTRA_FIRST_SHIFTED = 0x40, EXTRA_LAST_SHIFTED = 0x52,
		EXTRA_REMOVED_COUNT = 2, GAME_BOSS_FIRST = 0x130, GAME_BOSS_SECOND = 0x131,
		GAME_PAL_BOSS_FIRST = 0x100, GAME_PAL_BOSS_SECOND = 0x101,
		GAME_BOSS_LATER_FIRST = 0x134, GAME_BOSS_LAST = 0x139, GAME_BOSS_SHIFT = 0x32,
	};
	if (usIndex < 0)
	{
		return -1;
	}
	if (category == XA_EXTRA)
	{
		if (usIndex <= EXTRA_LAST_UNCHANGED) { return usIndex; }
		if (usIndex >= EXTRA_FIRST_SHIFTED && usIndex <= EXTRA_LAST_SHIFTED) { return usIndex - EXTRA_REMOVED_COUNT; }
		return -1;
	}
	if (category == XA_GAME)
	{
		if (usIndex < GAME_BOSS_FIRST) { return NativeLanguageDisc_PalVoiceIndex(usIndex); }
		if (usIndex == GAME_BOSS_FIRST) { return GAME_PAL_BOSS_FIRST; }
		if (usIndex == GAME_BOSS_SECOND) { return GAME_PAL_BOSS_SECOND; }
		if (usIndex >= GAME_BOSS_LATER_FIRST && usIndex <= GAME_BOSS_LAST) { return usIndex - GAME_BOSS_SHIFT; }
	}
	return -1;
}

static u32 NativeLanguageDisc_ReadLE32(const u8 *bytes)
{
	return (u32)bytes[0] | ((u32)bytes[1] << 8) | ((u32)bytes[2] << 16) | ((u32)bytes[3] << 24);
}

int NativeLanguageDisc_ReadText(int slot, u8 **bytesOut, u32 *sizeOut)
{
	struct NativeDiscImageFile bigfile;
	u8 header[NATIVE_LANGUAGE_DISC_SECTOR_SIZE];
	u32 index;
	u32 offset;
	u32 size;
	u32 sectors;
	u8 *bytes;

	if (bytesOut == NULL || sizeOut == NULL)
	{
		return 0;
	}
	*bytesOut = NULL;
	*sizeOut = 0;
	if (slot < NATIVE_LANGUAGE_DISC_FIRST_SLOT || slot > NATIVE_LANGUAGE_DISC_LAST_SLOT ||
	    !NativeDiscImage_FindFileOn(NATIVE_DISC_IMAGE_EUROPE, "BIGFILE.BIG", &bigfile) ||
	    !NativeDiscImage_ReadDataSectorsOn(NATIVE_DISC_IMAGE_EUROPE, &bigfile, 0, 1, header))
	{
		return 0;
	}
	index = NATIVE_LANGUAGE_DISC_FIRST_ENTRY + (u32)slot;
	if (NativeLanguageDisc_ReadLE32(&header[4]) <= index)
	{
		return 0;
	}
	const u8 *entry = &header[NATIVE_LANGUAGE_DISC_BIG_HEADER_SIZE + index * NATIVE_LANGUAGE_DISC_BIG_ENTRY_SIZE];
	offset = NativeLanguageDisc_ReadLE32(entry);
	size = NativeLanguageDisc_ReadLE32(entry + 4);
	if (size < NATIVE_LANGUAGE_DISC_FILE_HEADER_SIZE || size > NATIVE_LANGUAGE_DISC_MAX_TEXT_SIZE || size > bigfile.size ||
	    (u64)offset * NATIVE_LANGUAGE_DISC_SECTOR_SIZE + size > bigfile.size)
	{
		return 0;
	}
	sectors = (size + NATIVE_LANGUAGE_DISC_SECTOR_SIZE - 1) / NATIVE_LANGUAGE_DISC_SECTOR_SIZE;
	bytes = (u8 *)malloc((size_t)sectors * NATIVE_LANGUAGE_DISC_SECTOR_SIZE);
	if (bytes == NULL)
	{
		return 0;
	}
	if (!NativeDiscImage_ReadDataSectorsOn(NATIVE_DISC_IMAGE_EUROPE, &bigfile, offset, sectors, bytes))
	{
		free(bytes);
		return 0;
	}
	u32 count = NativeLanguageDisc_ReadLE32(bytes);
	u32 table = NativeLanguageDisc_ReadLE32(bytes + 4);
	if (count == 0 || table < NATIVE_LANGUAGE_DISC_FILE_HEADER_SIZE || table > size || count > (size - table) / 4)
	{
		free(bytes);
		return 0;
	}
	for (u32 i = 0; i < count; i++)
	{
		u32 stringOffset = NativeLanguageDisc_ReadLE32(bytes + table + i * 4);
		if (stringOffset < NATIVE_LANGUAGE_DISC_FILE_HEADER_SIZE || stringOffset >= table ||
		    memchr(bytes + stringOffset, 0, table - stringOffset) == NULL)
		{
			free(bytes);
			return 0;
		}
	}
	*bytesOut = bytes;
	*sizeOut = size;
	return 1;
}

int NativeLanguageDisc_HasText(int slot)
{
	static u32 generation;
	static int availability[NATIVE_LANGUAGE_DISC_LAST_SLOT + 1];
	u32 currentGeneration = NativeDiscImage_Generation();
	if (slot < NATIVE_LANGUAGE_DISC_FIRST_SLOT || slot > NATIVE_LANGUAGE_DISC_LAST_SLOT)
	{
		return 0;
	}
	if (generation != currentGeneration)
	{
		memset(availability, 0, sizeof(availability));
		generation = currentGeneration;
	}
	if (availability[slot] != 0)
	{
		return availability[slot] > 0;
	}
	u8 *bytes;
	u32 size;
	int available = NativeLanguageDisc_ReadText(slot, &bytes, &size);
	free(bytes);
	availability[slot] = available ? 1 : -1;
	return available;
}

const char *NativeLanguageDisc_TextString(const u8 *bytes, u32 size, int index)
{
	if (bytes == NULL || size < NATIVE_LANGUAGE_DISC_FILE_HEADER_SIZE || index < 0)
	{
		return NULL;
	}
	u32 count = NativeLanguageDisc_ReadLE32(bytes);
	u32 table = NativeLanguageDisc_ReadLE32(bytes + 4);
	if ((u32)index >= count || table < NATIVE_LANGUAGE_DISC_FILE_HEADER_SIZE || table > size || count > (size - table) / 4)
	{
		return NULL;
	}
	u32 offset = NativeLanguageDisc_ReadLE32(bytes + table + (u32)index * 4);
	if (offset < NATIVE_LANGUAGE_DISC_FILE_HEADER_SIZE || offset >= table || memchr(bytes + offset, 0, table - offset) == NULL)
	{
		return NULL;
	}
	return (const char *)bytes + offset;
}

const char *NativeLanguageDisc_SelfName(int slot)
{
	static u32 generation;
	static u8 *tables[NATIVE_LANGUAGE_DISC_LAST_SLOT + 1];
	static u32 sizes[NATIVE_LANGUAGE_DISC_LAST_SLOT + 1];
	static int attempted[NATIVE_LANGUAGE_DISC_LAST_SLOT + 1];
	u32 currentGeneration = NativeDiscImage_Generation();
	if (generation != currentGeneration)
	{
		for (int i = 0; i <= NATIVE_LANGUAGE_DISC_LAST_SLOT; i++)
		{
			free(tables[i]);
			tables[i] = NULL;
			sizes[i] = 0;
			attempted[i] = 0;
		}
		generation = currentGeneration;
	}
	if (slot < NATIVE_LANGUAGE_DISC_FIRST_SLOT || slot > NATIVE_LANGUAGE_DISC_LAST_SLOT)
	{
		return NULL;
	}
	if (!attempted[slot])
	{
		attempted[slot] = 1;
		NativeLanguageDisc_ReadText(slot, &tables[slot], &sizes[slot]);
	}
	int index = NativeLanguageDisc_PalIndex(LNG_FRENCH + slot - NATIVE_LANGUAGE_FRENCH);
	return NativeLanguageDisc_TextString(tables[slot], sizes[slot], index);
}

int NativeLanguageDisc_HasVoice(int slot)
{
	char path[32];
	u8 *bytes;
	int size;
	if (slot < NATIVE_LANGUAGE_DISC_FIRST_SLOT || slot > NATIVE_LANGUAGE_DISC_LAST_SLOT)
	{
		return 0;
	}
	snprintf(path, sizeof(path), "XA/%s.XNF", NativeLanguageDisc_VoiceCode(slot));
	if (!NativeDiscImage_ReadFileBytesOn(NATIVE_DISC_IMAGE_EUROPE, path, 0, &bytes, &size))
	{
		return 0;
	}
	int valid = size >= 68 && NativeLanguageDisc_ReadLE32(bytes) == 0x464e4958 && NativeLanguageDisc_ReadLE32(bytes + 4) == 102 &&
	            NativeLanguageDisc_ReadLE32(bytes + 8) == 3;
	free(bytes);
	if (!valid)
	{
		return 0;
	}
	struct NativeDiscImageFile file;
	snprintf(path, sizeof(path), "XA/%s/GAME/S00.XA", NativeLanguageDisc_VoiceCode(slot));
	return NativeDiscImage_FindFileOn(NATIVE_DISC_IMAGE_EUROPE, path, &file) && file.size != 0;
}

static u16 NativeLanguageDisc_ReadLE16(const u8 *bytes)
{
	return (u16)((u16)bytes[0] | (u16)bytes[1] << 8);
}

int NativeLanguageDisc_ReadVoiceSample(int language, int soundID, struct NativeLanguageVoiceSample *sample)
{
	enum
	{
		US_FIRST_VOICE = 28, US_VOICE_COUNT = 32, PAL_FIRST_FRENCH_FX = 258,
		PAL_LANGUAGE_FX_COUNT = 34, HOWL_HEADER_SIZE = 40, HOWL_MAX_HEADER = 65536,
		HOWL_BANK_SAMPLE_LIMIT = 1023, HOWL_SAMPLE_UNIT = 8,
	};
	struct NativeDiscImageFile file;
	u8 sector[NATIVE_LANGUAGE_DISC_SECTOR_SIZE];
	u8 *header = NULL;
	u8 *buffer = NULL;
	u32 sampleOffset = 0, sampleSize = 0;
	int found = 0;
	if (sample == NULL)
	{
		return 0;
	}
	memset(sample, 0, sizeof(*sample));
	if (language < NATIVE_LANGUAGE_DISC_FIRST_SLOT || language > NATIVE_LANGUAGE_DISC_LAST_SLOT || soundID < US_FIRST_VOICE || soundID >= US_FIRST_VOICE + US_VOICE_COUNT ||
	    !NativeDiscImage_FindFileOn(NATIVE_DISC_IMAGE_EUROPE, "SOUNDS/KART.HWL", &file) ||
	    !NativeDiscImage_ReadDataSectorsOn(NATIVE_DISC_IMAGE_EUROPE, &file, 0, 1, sector))
	{
		return 0;
	}
	u32 numSamples = NativeLanguageDisc_ReadLE32(sector + 16);
	u32 numFx = NativeLanguageDisc_ReadLE32(sector + 20);
	u32 numEngines = NativeLanguageDisc_ReadLE32(sector + 24);
	u32 numBanks = NativeLanguageDisc_ReadLE32(sector + 28);
	u32 headerSize = NativeLanguageDisc_ReadLE32(sector + 36);
	u32 fxIndex = PAL_FIRST_FRENCH_FX + (u32)(language - NATIVE_LANGUAGE_DISC_FIRST_SLOT) * PAL_LANGUAGE_FX_COUNT + (u32)(soundID - US_FIRST_VOICE);
	u64 tablesSize = (u64)HOWL_HEADER_SIZE + (u64)numSamples * 4 + (u64)numFx * 8 + (u64)numEngines * 8 + (u64)numBanks * 2;
	if (NativeLanguageDisc_ReadLE32(sector) != 0x4c574f48u || NativeLanguageDisc_ReadLE32(sector + 4) != 128 ||
	    headerSize > HOWL_MAX_HEADER - HOWL_HEADER_SIZE || tablesSize > HOWL_HEADER_SIZE + headerSize ||
	    tablesSize > file.size || fxIndex >= numFx)
	{
		return 0;
	}
	u32 headerSectors = ((u32)tablesSize + NATIVE_LANGUAGE_DISC_SECTOR_SIZE - 1) / NATIVE_LANGUAGE_DISC_SECTOR_SIZE;
	header = (u8 *)malloc((size_t)headerSectors * NATIVE_LANGUAGE_DISC_SECTOR_SIZE);
	if (header == NULL || !NativeDiscImage_ReadDataSectorsOn(NATIVE_DISC_IMAGE_EUROPE, &file, 0, headerSectors, header))
	{
		free(header);
		return 0;
	}
	u8 *sampleTable = header + HOWL_HEADER_SIZE;
	u8 *fx = sampleTable + numSamples * 4 + fxIndex * 8;
	u8 *banks = sampleTable + numSamples * 4 + numFx * 8 + numEngines * 8;
	u32 wanted = NativeLanguageDisc_ReadLE16(fx + 4);
	if (wanted >= numSamples)
	{
		free(header);
		return 0;
	}
	sampleSize = NativeLanguageDisc_ReadLE16(sampleTable + wanted * 4 + 2) * HOWL_SAMPLE_UNIT;
	for (u32 bank = 0; bank < numBanks && !found; bank++)
	{
		u32 first = NativeLanguageDisc_ReadLE16(banks + bank * 2);
		if (!NativeDiscImage_ReadDataSectorsOn(NATIVE_DISC_IMAGE_EUROPE, &file, first, 1, sector)) { break; }
		u32 count = NativeLanguageDisc_ReadLE16(sector);
		if (count > HOWL_BANK_SAMPLE_LIMIT) { break; }
		u64 offset = ((u64)first + 1) * NATIVE_LANGUAGE_DISC_SECTOR_SIZE;
		for (u32 i = 0; i < count; i++)
		{
			u32 index = NativeLanguageDisc_ReadLE16(sector + 2 + i * 2);
			if (index >= numSamples) { break; }
			if (index == wanted && offset + sampleSize <= file.size)
			{
				sampleOffset = (u32)offset;
				found = 1;
				break;
			}
			offset += NativeLanguageDisc_ReadLE16(sampleTable + index * 4 + 2) * HOWL_SAMPLE_UNIT;
		}
	}
	if (found && sampleSize != 0)
	{
		u32 skip = sampleOffset % NATIVE_LANGUAGE_DISC_SECTOR_SIZE;
		u32 sectors = (skip + sampleSize + NATIVE_LANGUAGE_DISC_SECTOR_SIZE - 1) / NATIVE_LANGUAGE_DISC_SECTOR_SIZE;
		buffer = (u8 *)malloc((size_t)sectors * NATIVE_LANGUAGE_DISC_SECTOR_SIZE);
		if (buffer != NULL && NativeDiscImage_ReadDataSectorsOn(NATIVE_DISC_IMAGE_EUROPE, &file,
		                                                        sampleOffset / NATIVE_LANGUAGE_DISC_SECTOR_SIZE, sectors, buffer))
		{
			memmove(buffer, buffer + skip, sampleSize);
			sample->data = buffer;
			sample->size = sampleSize;
			sample->flags = fx[0];
			sample->volume = fx[1];
			sample->pitch = NativeLanguageDisc_ReadLE16(fx + 2);
			sample->duration = NativeLanguageDisc_ReadLE16(fx + 6);
		}
		else { free(buffer); }
	}
	free(header);
	return sample->data != NULL;
}
