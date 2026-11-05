#ifndef CTR_NATIVE_TRACK_REGISTRY_H
#define CTR_NATIVE_TRACK_REGISTRY_H

#include <stddef.h>

/* MainRaceTrack_RequestLoad currently takes a signed 16-bit level ID. Runtime
   IDs are not persistent identities and must not index retail tables. */
enum
{
	NATIVE_TRACK_CUSTOM_ID_BASE = 0x100,
	NATIVE_TRACK_CUSTOM_ID_MAX = 0x7fff,
};

enum NativeTrackDetectedMode
{
	NATIVE_TRACK_DETECTED_TIME_TRIAL = 1 << 0,
	NATIVE_TRACK_DETECTED_ARCADE = 1 << 1,
	NATIVE_TRACK_DETECTED_RELIC = 1 << 2,
	NATIVE_TRACK_DETECTED_CTR = 1 << 3,
	NATIVE_TRACK_DETECTED_CRYSTAL = 1 << 4,
	NATIVE_TRACK_DETECTED_VS = 1 << 5,
};

struct NativeTrackMetadata
{
	const char *identity;
	const char *title;
	const char *levPath;
	const char *vrmPath;
	const char *scaPath;
	const char *author;
	unsigned int detectedModes;
	int detectionKnown;
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
