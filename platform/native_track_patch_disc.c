#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Independent reader: importing must not change the live game's CD stream.
 * Same NTSC-U MODE2/2352 contract as native_disc_image.c. */
enum { PATCH_DISC_RAW_BYTES = 2352, PATCH_DISC_DATA_BYTES = 2048, PATCH_DISC_DATA_OFFSET = 24 };

static uint32_t PatchDisc_Read32(const unsigned char *p)
{
	return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int PatchDisc_Sector(FILE *file, uint32_t lba, unsigned char *data)
{
	unsigned char sector[PATCH_DISC_RAW_BYTES];
	if ((uint64_t)lba * PATCH_DISC_RAW_BYTES > INT32_MAX ||
	    fseek(file, (long)((uint64_t)lba * PATCH_DISC_RAW_BYTES), SEEK_SET) != 0 ||
	    fread(sector, 1, sizeof(sector), file) != sizeof(sector) || sector[0] != 0 ||
	    sector[11] != 0 || sector[15] != 2)
	{
		return 0;
	}
	for (int i = 1; i < 11; i++)
	{
		if (sector[i] != 255)
		{
			return 0;
		}
	}
	memcpy(data, sector + PATCH_DISC_DATA_OFFSET, PATCH_DISC_DATA_BYTES);
	return 1;
}

static int PatchDisc_FindBig(FILE *file, uint32_t *lba, uint32_t *bytes)
{
	unsigned char data[PATCH_DISC_DATA_BYTES];
	if (!PatchDisc_Sector(file, 16, data) || data[0] != 1 || data[6] != 1 || memcmp(data + 1, "CD001", 5) != 0 ||
	    data[156] < 34 || (data[181] & 2) == 0)
	{
		return 0;
	}
	uint32_t root = PatchDisc_Read32(data + 158), size = PatchDisc_Read32(data + 166);
	if (size == 0 || size > 1024 * 1024)
	{
		return 0;
	}
	for (uint32_t offset = 0; offset < size; offset += PATCH_DISC_DATA_BYTES)
	{
		if (!PatchDisc_Sector(file, root + offset / PATCH_DISC_DATA_BYTES, data))
		{
			return 0;
		}
		for (size_t at = 0; at < sizeof(data) && at < size - offset && data[at] != 0; at += data[at])
		{
			size_t length = data[at];
			if (length < 34 || length > sizeof(data) - at || length > size - offset - at ||
			    data[at + 32] > length - 33)
			{
				return 0;
			}
			if ((data[at + 25] & 2) == 0 && data[at + 32] == 13 &&
			    SDL_strncasecmp((const char *)data + at + 33, "BIGFILE.BIG;1", 13) == 0)
			{
				*lba = PatchDisc_Read32(data + at + 2);
				*bytes = PatchDisc_Read32(data + at + 10);
				return *bytes >= 8 && *bytes <= 512 * 1024 * 1024;
			}
		}
	}
	return 0;
}

static int PatchDisc_ExtractBig(const char *image, const char *output, SDL_AtomicInt *cancel)
{
	FILE *disc = fopen(image, "rb");
	if (disc == NULL)
	{
		return 0;
	}
	uint32_t lba, bytes;
	int ok = PatchDisc_FindBig(disc, &lba, &bytes);
	FILE *out = ok ? fopen(output, "wb") : NULL;
	ok = ok && out != NULL;
	unsigned char sector[PATCH_DISC_DATA_BYTES];
	for (uint32_t offset = 0; ok && offset < bytes; offset += PATCH_DISC_DATA_BYTES)
	{
		size_t count = bytes - offset < sizeof(sector) ? bytes - offset : sizeof(sector);
		ok = !SDL_GetAtomicInt(cancel) && PatchDisc_Sector(disc, lba + offset / PATCH_DISC_DATA_BYTES, sector) &&
		     fwrite(sector, 1, count, out) == count;
	}
	if (out != NULL && fclose(out) != 0)
	{
		ok = 0;
	}
	fclose(disc);
	return ok;
}
