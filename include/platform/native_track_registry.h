#ifndef CTR_NATIVE_TRACK_REGISTRY_H
#define CTR_NATIVE_TRACK_REGISTRY_H

#include <stddef.h>

/* Runtime IDs are not persistent identities and must not index retail tables. */
enum { NATIVE_TRACK_CUSTOM_ID_BASE = 0x10000 };

struct NativeTrackMetadata
{
	const char *identity;
	const char *title;
	const char *levPath;
	const char *vrmPath;
	const char *scaPath;
	unsigned int laps;
};

struct NativeTrackRegistry;

struct NativeTrackRegistry *NativeTrackRegistry_Create(void);
void NativeTrackRegistry_Destroy(struct NativeTrackRegistry *registry);

/* Copies metadata only. An identity must be unique within the registry.
   Returned metadata remains valid until the registry is destroyed. */
int NativeTrackRegistry_Add(struct NativeTrackRegistry *registry, const struct NativeTrackMetadata *metadata, int *levelID);
const struct NativeTrackMetadata *NativeTrackRegistry_Get(const struct NativeTrackRegistry *registry, int levelID);
int NativeTrackRegistry_Find(const struct NativeTrackRegistry *registry, const char *identity, int *levelID);
size_t NativeTrackRegistry_Count(const struct NativeTrackRegistry *registry);

#endif
