#include <common.h>
#include <multitrack.h>
#include "platform/native_assets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MTState gMTState;

static struct MTHeader sEmptyTable = {MT_MAGIC, 0};
static struct MTHeader *sTable;

struct MTHeader *MT_Table(void)
{
	if (sTable != NULL)
	{
		return sTable;
	}

	sTable = &sEmptyTable;
	FILE *f = NativeAssets_OpenHost("track_table.bin", "rb");
	if (f == NULL)
	{
		printf("[MultiTrack] assets/track_table.bin missing: stock tracks only\n");
		return sTable;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	struct MTHeader *table = malloc(size);
	if ((size >= (long)sizeof(struct MTHeader)) && (fread(table, 1, size, f) == (size_t)size) && (table->magic == MT_MAGIC) &&
	    (sizeof(struct MTHeader) + table->count * sizeof(struct MTEntry) <= (size_t)size))
	{
		sTable = table;
		printf("[MultiTrack] %d custom tracks\n", table->count);
	}
	else
	{
		printf("[MultiTrack] assets/track_table.bin invalid: stock tracks only\n");
		free(table);
	}
	fclose(f);
	return sTable;
}

struct MTEntry *MT_ActiveEntry(void)
{
	struct GameTracker *gGT = sdata->gGT;
	struct MTHeader *table = MT_Table();

	if ((gMTState.magic != MT_MAGIC) || (gMTState.redirectLevel != (int)gGT->levelID))
	{
		return NULL;
	}

	for (int i = 0; i < table->count; i++)
	{
		if (table->entries[i].bigBase == gMTState.redirectBase)
		{
			return &table->entries[i];
		}
	}
	return NULL;
}

// CTR_BOOT_LEVEL=<levelID> boots into that stock level instead of the intro;
// CTR_BOOT_TRACK=<n> boots into custom track n of the table (on its host slot)
int MT_BootLevel(int defaultLevel)
{
	const char *track = getenv("CTR_BOOT_TRACK");
	const char *level = getenv("CTR_BOOT_LEVEL");

	if (track != NULL)
	{
		struct MTHeader *table = MT_Table();
		int n = atoi(track);
		if ((n >= 0) && (n < table->count))
		{
			gMTState.magic = MT_MAGIC;
			gMTState.redirectLevel = table->entries[n].slot;
			gMTState.redirectBase = table->entries[n].bigBase;
			printf("[MultiTrack] boot: custom track %d '%s' on slot %d\n", n, table->entries[n].name, table->entries[n].slot);
			return table->entries[n].slot;
		}
		printf("[MultiTrack] CTR_BOOT_TRACK=%s out of range\n", track);
	}

	if (level != NULL)
	{
		printf("[MultiTrack] boot: level %d\n", atoi(level));
		return atoi(level);
	}

	return defaultLevel;
}
