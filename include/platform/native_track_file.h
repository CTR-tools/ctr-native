#ifndef CTR_NATIVE_TRACK_FILE_H
#define CTR_NATIVE_TRACK_FILE_H

#include <stddef.h>
#include <platform/native_track_registry.h>

enum NativeTrackFileRole
{
	NATIVE_TRACK_FILE_LEV,
	NATIVE_TRACK_FILE_VRM,
	NATIVE_TRACK_FILE_SCA,
};

struct NativeTrackFile;

/* Open once at load time; the returned handle owns the open file. The size is
   bounded so sector rounding and the loader's signed allocations cannot wrap. */
struct NativeTrackFile *NativeTrackFile_Open(const struct NativeTrackRegistry *registry, int levelID, enum NativeTrackFileRole role);
size_t NativeTrackFile_Size(const struct NativeTrackFile *file);
size_t NativeTrackFile_SectorBytes(const struct NativeTrackFile *file);
/* Reads into caller-owned memory and zeroes sector padding. LEV and VRM also
   pass the structural bounds checks in native_track_validate.h. SCA is read
   without parsing; its audio consumer must validate it separately. A failed
   read may have modified the buffer and must not be passed to the loader. */
int NativeTrackFile_Read(struct NativeTrackFile *file, void *destination, size_t capacity);
void NativeTrackFile_Close(struct NativeTrackFile *file);

#endif
