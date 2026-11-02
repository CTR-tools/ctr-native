#ifndef CTR_NATIVE_TRACK_RECORDS_H
#define CTR_NATIVE_TRACK_RECORDS_H

#include <stddef.h>
#include <stdint.h>
#include <platform/native_track_registry.h>

struct NativeTrackRecordKey
{
	uint64_t identity;
	uint64_t lev;
	uint64_t vrm;
};

int NativeTrackRecords_Key(const struct NativeTrackMetadata *track, struct NativeTrackRecordKey *key);
int NativeTrackRecords_Load(const char *directory, const struct NativeTrackMetadata *track,
                            const struct NativeTrackRecordKey *key, void *data, size_t size);
int NativeTrackRecords_Save(const char *directory, const struct NativeTrackMetadata *track,
                            const struct NativeTrackRecordKey *key, const void *data, size_t size);

#endif
