#ifndef CTR_NATIVE_TRACK_LIBRARY_H
#define CTR_NATIVE_TRACK_LIBRARY_H

#include <stddef.h>
#include <platform/native_track_registry.h>

/* Retail level ID of Crash Cove, the fallback when no local SCA exists. */
enum { NATIVE_TRACK_DEFAULT_RETAIL_MUSIC = 3 };

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

/* Replace the scan outside gameplay, with no retained metadata pointers.
   Failure leaves the previous registry live. */
int NativeTrackLibrary_Refresh(const char *directory, struct NativeTrackScanResult *result);
/* Takes ownership on the main thread, after an off-thread scan completes.
   Invalid explicit music values are replaced by the local SCA or Crash Cove;
   manifest write failures are logged and retried on the next publication. */
void NativeTrackLibrary_Replace(struct NativeTrackRegistry *replacement);
const struct NativeTrackRegistry *NativeTrackLibrary_Current(void);
int NativeTrackLibrary_IsEnabled(const struct NativeTrackMetadata *track);
int NativeTrackLibrary_SetEnabled(const struct NativeTrackMetadata *track, int enabled);
/* Delete an installed track directory and refresh the registry. */
int NativeTrackLibrary_Delete(const struct NativeTrackMetadata *track, const char *directory);
/* Missing or invalid selections retain the original custom-audio fallback. */
int NativeTrackLibrary_Music(const struct NativeTrackMetadata *track);
int NativeTrackLibrary_SetDetails(const struct NativeTrackMetadata *track, const char *title, const char *author);
int NativeTrackLibrary_ValidText(const char *text, int allowEmpty);
int NativeTrackLibrary_WriteMetadata(const struct NativeTrackMetadata *track, const char *title, const char *author,
                                     int retail, int custom);
int NativeTrackLibrary_SetMusic(const struct NativeTrackMetadata *track, int retail, int custom);
/* Unknown source modes remain selectable; explicit lists must include time_trial. */
/* Arcade and VS require positive LEV detection, not just source tags. */
int NativeTrackLibrary_AllowsDetectedMode(const struct NativeTrackMetadata *track, unsigned int mode);
void NativeTrackLibrary_Shutdown(void);

#endif
