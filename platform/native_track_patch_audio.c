#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "native_track_patch_audio_map.c"

enum { PATCH_HWL_LIMIT = 32 * 1024 * 1024, PATCH_EXE_LIMIT = 2 * 1024 * 1024,
       PATCH_MENU_LIMIT = 2 * 1024 * 1024, PATCH_SCA_LIMIT = 1024 * 1024,
       PATCH_SCA_SECTOR = 2048, PATCH_SCA_SAMPLES = 1023 };

struct PatchHowl
{
	struct PatchMusicBytes bytes;
	uint32_t samples, banks, songs;
	size_t offsets;
};

struct PatchBank
{
	size_t offset, size;
	unsigned int count;
};

struct PatchMusic
{
	struct PatchMusicBytes originalExe, exe, menu, all;
	struct PatchHowl original, patched;
	int available, skipped, generated;
};

static unsigned int PatchMusic_U16(const unsigned char *p)
{
	return p[0] | (unsigned int)p[1] << 8;
}

static int PatchMusic_Range(size_t size, size_t offset, size_t length)
{
	return offset <= size && length <= size - offset;
}

static int PatchMusic_Howl(struct PatchHowl *h)
{
	const unsigned char *p = h->bytes.data;
	size_t n = h->bytes.size;
	if (n < 40 || memcmp(p, "HOWL", 4) != 0) { return 0; }
	h->samples = PatchDisc_Read32(p + 16);
	uint32_t fx = PatchDisc_Read32(p + 20), engines = PatchDisc_Read32(p + 24);
	h->banks = PatchDisc_Read32(p + 28);
	h->songs = PatchDisc_Read32(p + 32);
	if (h->samples == 0 || h->samples > 4096 || fx > 4096 || engines > 4096 ||
	    h->banks == 0 || h->banks > 4096 || h->songs == 0 || h->songs > 4096) { return 0; }
	h->offsets = 40 + (size_t)h->samples * 4 + ((size_t)fx + engines) * 8;
	return PatchMusic_Range(n, h->offsets, ((size_t)h->banks + h->songs) * 2);
}

static int PatchMusic_Bank(const struct PatchHowl *h, unsigned int id, struct PatchBank *bank)
{
	if (id >= h->banks) { return 0; }
	const unsigned char *p = h->bytes.data;
	size_t offset = (size_t)PatchMusic_U16(p + h->offsets + id * 2) * PATCH_SCA_SECTOR;
	if (!PatchMusic_Range(h->bytes.size, offset, PATCH_SCA_SECTOR)) { return 0; }
	unsigned int count = PatchMusic_U16(p + offset);
	if (count == 0 || count > PATCH_SCA_SAMPLES) { return 0; }
	size_t size = PATCH_SCA_SECTOR;
	for (unsigned int i = 0; i < count; i++)
	{
		unsigned int sample = PatchMusic_U16(p + offset + 2 + i * 2);
		if (sample >= h->samples) { return 0; }
		unsigned int units = PatchMusic_U16(p + 40 + sample * 4 + 2);
		if (units == 0 || (units & 1) != 0) { return 0; }
		for (unsigned int j = 0; j < i; j++)
		{
			if (PatchMusic_U16(p + offset + 2 + j * 2) == sample) { return 0; }
		}
		size += (size_t)units * 8;
	}
	if (!PatchMusic_Range(h->bytes.size, offset, size)) { return 0; }
	bank->offset = offset;
	bank->size = size;
	bank->count = count;
	return 1;
}

static int PatchMusic_InBank(const struct PatchHowl *h, const struct PatchBank *b, unsigned int sample)
{
	for (unsigned int i = 0; i < b->count; i++)
	{
		if (PatchMusic_U16(h->bytes.data + b->offset + 2 + i * 2) == sample) { return 1; }
	}
	return 0;
}

static const unsigned char *PatchMusic_Sample(const struct PatchHowl *h, const struct PatchBank *b,
                                              unsigned int sample, size_t *size)
{
	size_t offset = b->offset + PATCH_SCA_SECTOR;
	for (unsigned int i = 0; i < b->count; i++)
	{
		unsigned int id = PatchMusic_U16(h->bytes.data + b->offset + 2 + i * 2);
		size_t length = (size_t)PatchMusic_U16(h->bytes.data + 42 + id * 4) * 8;
		if (id == sample) { *size = length; return h->bytes.data + offset; }
		offset += length;
	}
	return NULL;
}

static int PatchMusic_Compatible(const struct PatchHowl *base, const struct PatchHowl *patched,
                                 const struct PatchBank *track, const unsigned char *song, size_t songSize)
{
	struct PatchBank common, patchedCommon;
	if (!PatchMusic_Bank(base, 0, &common) || !PatchMusic_Bank(patched, 0, &patchedCommon)) { return 0; }
	/* IDs from the common and later character banks may already be allocated
	 * using retail sample sizes. Reject incompatible replacements up front. */
	for (unsigned int b = 0; b < base->banks; b++)
	{
		if (b != 0 && b < 54) { continue; }
		struct PatchBank other;
		if (!PatchMusic_Bank(base, b, &other)) { return 0; }
		for (unsigned int i = 0; i < track->count; i++)
		{
			unsigned int sample = PatchMusic_U16(patched->bytes.data + track->offset + 2 + i * 2);
			if (sample >= base->samples) { return 0; }
			if (PatchMusic_InBank(base, &other, sample) &&
			    PatchMusic_U16(base->bytes.data + 42 + sample * 4) !=
			    PatchMusic_U16(patched->bytes.data + 42 + sample * 4)) { return 0; }
		}
	}
	if (songSize < 8) { return 0; }
	size_t longs = song[4], shorts = song[5], songs = PatchMusic_U16(song + 6);
	size_t tables = 8 + longs * 12 + shorts * 8 + songs * 2;
	if (songs == 0 || tables >= songSize) { return 0; }
	size_t songTable = 8 + longs * 12 + shorts * 8;
	size_t songData = (tables + 3) & ~(size_t)3;
	for (size_t i = 0; i < songs; i++)
	{
		int offset = (int16_t)PatchMusic_U16(song + songTable + i * 2);
		if (offset < 0 || !PatchMusic_Range(songSize, songData + (size_t)offset, 6)) { return 0; }
		size_t header = songData + (size_t)offset;
		unsigned int sequences = song[header + 1];
		size_t notes = (header + 6 + sequences * 2 + 3) & ~(size_t)3;
		if (!PatchMusic_Range(songSize, header + 6, sequences * 2) || notes > songSize) { return 0; }
		for (unsigned int j = 0; j < sequences; j++)
		{
			size_t note = notes + PatchMusic_U16(song + header + 6 + j * 2);
			if (!PatchMusic_Range(songSize, note, 2)) { return 0; }
		}
	}
	for (size_t i = 0; i < longs + shorts; i++)
	{
		size_t at = i < longs ? 8 + i * 12 + 6 : 8 + longs * 12 + (i - longs) * 8 + 4;
		unsigned int sample = PatchMusic_U16(song + at);
		if (sample >= base->samples || (!PatchMusic_InBank(patched, track, sample) &&
		    !PatchMusic_InBank(base, &common, sample))) { return 0; }
		if (PatchMusic_InBank(base, &common, sample))
		{
			size_t oldSize, newSize;
			const unsigned char *oldSample = PatchMusic_Sample(base, &common, sample, &oldSize);
			const unsigned char *newSample = PatchMusic_Sample(patched, &patchedCommon, sample, &newSize);
			if (newSample == NULL || oldSize != newSize || memcmp(oldSample, newSample, oldSize) != 0) { return 0; }
		}
	}
	return 1;
}

static void PatchMusic_Write32(unsigned char *p, size_t n)
{
	for (int i = 0; i < 4; i++) { p[i] = (unsigned char)(n >> (i * 8)); }
}

static int PatchMusic_WriteSca(const struct PatchMusic *m, int level, const char *path)
{
	const struct PatchHowl *h = &m->patched;
	unsigned int bankID = m->exe.data[PATCH_MUSIC_BANK_TABLE + level];
	unsigned int songID = m->exe.data[PATCH_MUSIC_SONG_TABLE + level];
	struct PatchBank bank;
	if (!PatchMusic_Bank(h, bankID, &bank) || songID >= h->songs) { return 0; }
	size_t songOffset = (size_t)PatchMusic_U16(h->bytes.data + h->offsets + (h->banks + songID) * 2) * PATCH_SCA_SECTOR;
	if (!PatchMusic_Range(h->bytes.size, songOffset, 8)) { return 0; }
	size_t songSize = PatchDisc_Read32(h->bytes.data + songOffset);
	if (songSize < 8 || songSize > 65536 || !PatchMusic_Range(h->bytes.size, songOffset, songSize) ||
	    !PatchMusic_Compatible(&m->original, h, &bank, h->bytes.data + songOffset, songSize)) { return 0; }
	size_t bankEnd = (12 + bank.size + 3) & ~(size_t)3;
	size_t songEnd = (bankEnd + 8 + songSize + 3) & ~(size_t)3;
	size_t total = songEnd + 8 + bank.count * 2;
	if (total > PATCH_SCA_LIMIT) { return 0; }
	unsigned char *out = calloc(1, total);
	if (out == NULL) { return 0; }
	memcpy(out, "SCA\1BANK", 8);
	PatchMusic_Write32(out + 8, bank.size);
	memcpy(out + 12, h->bytes.data + bank.offset, bank.size);
	memcpy(out + bankEnd, "CSEQ", 4);
	PatchMusic_Write32(out + bankEnd + 4, songSize);
	memcpy(out + bankEnd + 8, h->bytes.data + songOffset, songSize);
	memcpy(out + songEnd, "SIZE", 4);
	PatchMusic_Write32(out + songEnd + 4, bank.count * 2);
	for (unsigned int i = 0; i < bank.count; i++)
	{
		unsigned int id = PatchMusic_U16(h->bytes.data + bank.offset + 2 + i * 2);
		memcpy(out + songEnd + 8 + i * 2, h->bytes.data + 42 + id * 4, 2);
	}
	FILE *file = fopen(path, "wb");
	int ok = file != NULL && fwrite(out, 1, total, file) == total;
	if (file != NULL && fclose(file) != 0) { ok = 0; }
	free(out);
	if (!ok) { SDL_RemovePath(path); }
	return ok;
}

static struct PatchMusicBytes PatchMusic_Load(const char *disc, const char *path, size_t limit, SDL_AtomicInt *cancel)
{
	struct PatchMusicBytes result = {0};
	result.data = PatchDisc_Load(disc, path, limit, &result.size, cancel);
	return result;
}

static struct PatchMusicBytes PatchMusic_Menu(const char *disc, SDL_AtomicInt *cancel)
{
	struct PatchMusicBytes result = {0};
	FILE *file = fopen(disc, "rb");
	uint32_t lba, bytes;
	unsigned char sector[PATCH_DISC_DATA_BYTES];
	if (file != NULL && PatchDisc_FindBig(file, &lba, &bytes) && PatchDisc_Sector(file, lba, sector) &&
	    PatchDisc_Read32(sector + 4) > 230)
	{
		uint32_t offset = PatchDisc_Read32(sector + 8 + 230 * 8);
		uint32_t size = PatchDisc_Read32(sector + 12 + 230 * 8);
		if (size != 0 && size <= PATCH_MENU_LIMIT && (uint64_t)offset * PATCH_DISC_DATA_BYTES + size <= bytes)
		{
			result.data = SDL_malloc(size);
			for (uint32_t at = 0; result.data != NULL && at < size; at += PATCH_DISC_DATA_BYTES)
			{
				size_t n = size - at < sizeof(sector) ? size - at : sizeof(sector);
				if (SDL_GetAtomicInt(cancel) || !PatchDisc_Sector(file, lba + offset + at / PATCH_DISC_DATA_BYTES, sector))
				{
					SDL_free(result.data); result.data = NULL; break;
				}
				memcpy(result.data + at, sector, n);
			}
			if (result.data != NULL) { result.size = size; }
		}
	}
	if (file != NULL) { fclose(file); }
	return result;
}

static void PatchMusic_Close(struct PatchMusic *m)
{
	SDL_free(m->originalExe.data); SDL_free(m->exe.data); SDL_free(m->menu.data); SDL_free(m->all.data);
	SDL_free(m->original.bytes.data); SDL_free(m->patched.bytes.data);
}

static void PatchMusic_Open(struct PatchMusic *m, const char *original, const char *patched, SDL_AtomicInt *cancel)
{
	memset(m, 0, sizeof(*m));
	m->originalExe = PatchMusic_Load(original, "SCUS_944.26;1", PATCH_EXE_LIMIT, cancel);
	m->exe = PatchMusic_Load(patched, "SCUS_944.26;1", PATCH_EXE_LIMIT, cancel);
	m->menu = PatchMusic_Menu(patched, cancel);
	m->all = PatchMusic_Load(patched, "ALL.BIN;1", PATCH_EXE_LIMIT, cancel);
	m->original.bytes = PatchMusic_Load(original, "SOUNDS/KART.HWL;1", PATCH_HWL_LIMIT, cancel);
	m->patched.bytes = PatchMusic_Load(patched, "SOUNDS/KART.HWL;1", PATCH_HWL_LIMIT, cancel);
	m->available = m->originalExe.data != NULL && m->exe.data != NULL && m->original.bytes.data != NULL &&
	               m->patched.bytes.data != NULL && PatchMusic_Howl(&m->original) && PatchMusic_Howl(&m->patched);
}

static int PatchMusic_SelectLevel(const struct PatchMusic *m, int slot)
{
	if (!m->available || slot < 0 || slot >= 18) { return -1; }
	int level = PatchMusic_Level(&m->originalExe, &m->exe, &m->menu, slot);
	if (level < 0) { level = PatchMusic_AllBinLevel(&m->exe, &m->all, slot); }
	/* Unknown loaders still get the audio of their physical slot. */
	return level < 0 ? slot : level;
}
