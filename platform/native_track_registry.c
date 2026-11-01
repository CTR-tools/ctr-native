#include <platform/native_track_registry.h>

#include <stdlib.h>
#include <string.h>

struct NativeTrackEntry
{
	struct NativeTrackMetadata metadata;
	struct NativeTrackEntry *next;
};

struct NativeTrackRegistry
{
	struct NativeTrackEntry *first;
	struct NativeTrackEntry *last;
	size_t count;
};

static char *NativeTrackRegistry_Copy(const char *text)
{
	size_t size = strlen(text) + 1;
	char *copy = malloc(size);
	if (copy != NULL)
	{
		memcpy(copy, text, size);
	}
	return copy;
}

static void NativeTrackRegistry_FreeEntry(struct NativeTrackEntry *entry)
{
	free((void *)entry->metadata.identity);
	free((void *)entry->metadata.title);
	free((void *)entry->metadata.levPath);
	free((void *)entry->metadata.vrmPath);
	free((void *)entry->metadata.scaPath);
	free(entry);
}

struct NativeTrackRegistry *NativeTrackRegistry_Create(void)
{
	return calloc(1, sizeof(struct NativeTrackRegistry));
}

void NativeTrackRegistry_Destroy(struct NativeTrackRegistry *registry)
{
	if (registry == NULL)
	{
		return;
	}
	struct NativeTrackEntry *entry = registry->first;
	while (entry != NULL)
	{
		struct NativeTrackEntry *next = entry->next;
		NativeTrackRegistry_FreeEntry(entry);
		entry = next;
	}
	free(registry);
}

int NativeTrackRegistry_Add(struct NativeTrackRegistry *registry, const struct NativeTrackMetadata *metadata, int *levelID)
{
	if (registry == NULL || metadata == NULL || metadata->identity == NULL || metadata->identity[0] == '\0' ||
	    metadata->title == NULL || metadata->title[0] == '\0' || metadata->levPath == NULL || metadata->levPath[0] == '\0' ||
	    metadata->vrmPath == NULL || metadata->vrmPath[0] == '\0' ||
	    registry->count > (size_t)(NATIVE_TRACK_CUSTOM_ID_MAX - NATIVE_TRACK_CUSTOM_ID_BASE) ||
	    NativeTrackRegistry_Find(registry, metadata->identity, NULL))
	{
		return 0;
	}

	struct NativeTrackEntry *entry = calloc(1, sizeof(*entry));
	if (entry == NULL)
	{
		return 0;
	}
	entry->metadata.identity = NativeTrackRegistry_Copy(metadata->identity);
	entry->metadata.title = NativeTrackRegistry_Copy(metadata->title);
	entry->metadata.levPath = NativeTrackRegistry_Copy(metadata->levPath);
	entry->metadata.vrmPath = NativeTrackRegistry_Copy(metadata->vrmPath);
	entry->metadata.scaPath = metadata->scaPath != NULL && metadata->scaPath[0] != '\0' ? NativeTrackRegistry_Copy(metadata->scaPath) : NULL;
	if (entry->metadata.identity == NULL || entry->metadata.title == NULL || entry->metadata.levPath == NULL ||
	    entry->metadata.vrmPath == NULL || (metadata->scaPath != NULL && metadata->scaPath[0] != '\0' && entry->metadata.scaPath == NULL))
	{
		NativeTrackRegistry_FreeEntry(entry);
		return 0;
	}
	if (registry->last != NULL)
	{
		registry->last->next = entry;
	}
	else
	{
		registry->first = entry;
	}
	registry->last = entry;
	if (levelID != NULL)
	{
		*levelID = NATIVE_TRACK_CUSTOM_ID_BASE + (int)registry->count;
	}
	registry->count++;
	return 1;
}

const struct NativeTrackMetadata *NativeTrackRegistry_Get(const struct NativeTrackRegistry *registry, int levelID)
{
	if (registry == NULL || levelID < NATIVE_TRACK_CUSTOM_ID_BASE)
	{
		return NULL;
	}
	size_t index = (size_t)(levelID - NATIVE_TRACK_CUSTOM_ID_BASE);
	if (index >= registry->count)
	{
		return NULL;
	}
	const struct NativeTrackEntry *entry = registry->first;
	while (index-- > 0)
	{
		entry = entry->next;
	}
	return &entry->metadata;
}

int NativeTrackRegistry_Find(const struct NativeTrackRegistry *registry, const char *identity, int *levelID)
{
	if (registry == NULL || identity == NULL)
	{
		return 0;
	}
	const struct NativeTrackEntry *entry = registry->first;
	int id = NATIVE_TRACK_CUSTOM_ID_BASE;
	while (entry != NULL)
	{
		if (strcmp(entry->metadata.identity, identity) == 0)
		{
			if (levelID != NULL)
			{
				*levelID = id;
			}
			return 1;
		}
		entry = entry->next;
		if (entry != NULL)
		{
			id++;
		}
	}
	return 0;
}

size_t NativeTrackRegistry_Count(const struct NativeTrackRegistry *registry)
{
	return registry != NULL ? registry->count : 0;
}
