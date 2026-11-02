#ifndef CTR_NATIVE_TRACK_LIBRARY_H
#define CTR_NATIVE_TRACK_LIBRARY_H

#include <stddef.h>
#include <platform/native_track_registry.h>

struct NativeTrackScanResult
{
	size_t installed;
	size_t rejected;
};

/* Reads optional track.ini and filesystem metadata only, never track content.
   The caller owns the returned registry. An unreadable directory returns NULL. */
struct NativeTrackRegistry *NativeTrackLibrary_Scan(const char *directory, struct NativeTrackScanResult *result);

/* Add one prepared directory using name for its fallback title and identity. */
int NativeTrackLibrary_ReadDirectory(struct NativeTrackRegistry *registry, const char *directory, const char *name);

/* Publish a complete scan at a safe transition, when no caller retains a
   pointer into the previous registry. Failure leaves the previous scan live. */
int NativeTrackLibrary_Refresh(const char *directory, struct NativeTrackScanResult *result);
const struct NativeTrackRegistry *NativeTrackLibrary_Current(void);
int NativeTrackLibrary_IsEnabled(const struct NativeTrackMetadata *track);
int NativeTrackLibrary_SetEnabled(const struct NativeTrackMetadata *track, int enabled);
/* Unknown source modes remain selectable; explicit lists must include time_trial. */
int NativeTrackLibrary_AllowsTimeTrial(const struct NativeTrackMetadata *track);
void NativeTrackLibrary_Shutdown(void);

#endif
