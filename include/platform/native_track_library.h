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

/* Publish a complete scan at a safe transition, when no caller retains a
   pointer into the previous registry. Failure leaves the previous scan live. */
int NativeTrackLibrary_Refresh(const char *directory, struct NativeTrackScanResult *result);
const struct NativeTrackRegistry *NativeTrackLibrary_Current(void);
void NativeTrackLibrary_Shutdown(void);

#endif
