#ifndef PLATFORM_NATIVE_CHECKPOINT_FILE_H
#define PLATFORM_NATIVE_CHECKPOINT_FILE_H

#include <macros.h>

#if defined(CTR_INTERNAL)
struct NativeCheckpointFileRecordInfo
{
	u32 checkpointIndex;
	u32 replayFrame;
	u32 payloadSize;
	u32 checksum;
};

int NativeCheckpointFile_WriteSingle(const char *path, const void *payload, int payloadSize, u32 checkpointIndex, u32 replayFrame);
int NativeCheckpointFile_ReadSingle(const char *path, void *payload, int payloadSize, struct NativeCheckpointFileRecordInfo *info);
#endif

#endif
