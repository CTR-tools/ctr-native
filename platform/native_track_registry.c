#include <platform/native_track_registry.h>

#include <stdlib.h>
#include <string.h>

struct NativeTrackEntry
{
	struct NativeTrackMetadata metadata;
};

struct NativeTrackRegistry
{
	struct NativeTrackEntry **entries;
	size_t count;
	size_t capacity;
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
	free((void *)entry->metadata.author);
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
	for (size_t i = 0; i < registry->count; i++)
	{
		NativeTrackRegistry_FreeEntry(registry->entries[i]);
	}
	free(registry->entries);
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
	entry->metadata.author = metadata->author != NULL && metadata->author[0] != '\0' ? NativeTrackRegistry_Copy(metadata->author) : NULL;
	entry->metadata.detectedModes = metadata->detectedModes;
	entry->metadata.detectionKnown = metadata->detectionKnown;
	if (entry->metadata.identity == NULL || entry->metadata.title == NULL || entry->metadata.levPath == NULL ||
	    entry->metadata.vrmPath == NULL || (metadata->scaPath != NULL && metadata->scaPath[0] != '\0' && entry->metadata.scaPath == NULL) ||
	    (metadata->author != NULL && metadata->author[0] != '\0' && entry->metadata.author == NULL))
	{
		NativeTrackRegistry_FreeEntry(entry);
		return 0;
	}
	if (registry->count == registry->capacity)
	{
		size_t capacity = registry->capacity == 0 ? 16 : registry->capacity * 2;
		struct NativeTrackEntry **entries = realloc(registry->entries, capacity * sizeof(*entries));
		if (entries == NULL)
		{
			NativeTrackRegistry_FreeEntry(entry);
			return 0;
		}
		registry->entries = entries;
		registry->capacity = capacity;
	}
	registry->entries[registry->count] = entry;
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
	return &registry->entries[index]->metadata;
}

int NativeTrackRegistry_Find(const struct NativeTrackRegistry *registry, const char *identity, int *levelID)
{
	if (registry == NULL || identity == NULL)
	{
		return 0;
	}
	for (size_t i = 0; i < registry->count; i++)
	{
		if (strcmp(registry->entries[i]->metadata.identity, identity) == 0)
		{
			if (levelID != NULL)
			{
				*levelID = NATIVE_TRACK_CUSTOM_ID_BASE + (int)i;
			}
			return 1;
		}
	}
	return 0;
}

size_t NativeTrackRegistry_Count(const struct NativeTrackRegistry *registry)
{
	return registry != NULL ? registry->count : 0;
}
