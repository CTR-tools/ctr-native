#ifndef PLATFORM_NATIVE_CHECKPOINT_FILE_H
#define PLATFORM_NATIVE_CHECKPOINT_FILE_H

#include <macros.h>

#if defined(CTR_INTERNAL)
struct NativeCheckpointFileRecordInfo
{
	u32 checkpointIndex;
	u32 replayFrame;
	u32 payloadOffset;
	u32 payloadSize;
	u32 checksum;
};

struct NativeCheckpointFileWriter
{
	void *file;
	u32 recordCount;
};

int NativeCheckpointFile_BeginWrite(struct NativeCheckpointFileWriter *writer, const char *path);
int NativeCheckpointFile_AppendRecord(struct NativeCheckpointFileWriter *writer, const void *payload, int payloadSize, u32 checkpointIndex, u32 replayFrame,
                                      struct NativeCheckpointFileRecordInfo *info);
int NativeCheckpointFile_EndWrite(struct NativeCheckpointFileWriter *writer);
int NativeCheckpointFile_Validate(const char *path, struct NativeCheckpointFileRecordInfo *records, int maxRecords, int *recordCount);
int NativeCheckpointFile_ReadRecord(const char *path, u32 checkpointIndex, void *payload, int payloadSize, struct NativeCheckpointFileRecordInfo *info);
int NativeCheckpointFile_WriteSingle(const char *path, const void *payload, int payloadSize, u32 checkpointIndex, u32 replayFrame);
int NativeCheckpointFile_ReadSingle(const char *path, void *payload, int payloadSize, struct NativeCheckpointFileRecordInfo *info);
#endif

#endif
