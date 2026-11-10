#ifndef CTR_NATIVE_TRACK_IMPORT_H
#define CTR_NATIVE_TRACK_IMPORT_H

#include <namespace_Lng.h>

/* Returns LNG_TRACK_RESULT_OK or an Expanded error ID. */
enum ExpandedLngString NativeTrackImport_Directory(const char *library, const char *source, const char *name);

#endif
