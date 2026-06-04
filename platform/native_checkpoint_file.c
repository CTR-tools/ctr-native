#include "platform/native_checkpoint_file.h"

#if defined(CTR_INTERNAL)
#include <stdio.h>
#include <string.h>

#define NATIVE_CHECKPOINT_FILE_FOURCC(a, b, c, d) ((u32)(a) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

// NOTE(aalhendi): `CTST` means CTR native State checkpoint container. The file
// can grow to rolling checkpoint records without changing the `.ctrstates`
// extension introduced by this single-record persistence slice.
#define NATIVE_CHECKPOINT_FILE_MAGIC              NATIVE_CHECKPOINT_FILE_FOURCC('C', 'T', 'S', 'T')
#define NATIVE_CHECKPOINT_FILE_VERSION            1u

struct NativeCheckpointFileHeader
{
	u32 magic;
	u32 version;
	u32 headerSize;
	u32 recordHeaderSize;
	u32 recordCount;
	u32 reserved[3];
};

struct NativeCheckpointFileRecordHeader
{
	u32 checkpointIndex;
	u32 replayFrame;
	u32 payloadOffset;
	u32 payloadSize;
	u32 checksum;
	u32 reserved[3];
};

static u32 NativeCheckpointFile_Checksum(const void *src, int size)
{
	const u8 *bytes = (const u8 *)src;
	u32 hash = 2166136261u;
	int i;

	for (i = 0; i < size; i++)
	{
		hash ^= bytes[i];
		hash *= 16777619u;
	}

	return hash;
}

static int NativeCheckpointFile_WriteExact(FILE *file, const void *src, size_t size)
{
	return fwrite(src, 1, size, file) == size;
}

static int NativeCheckpointFile_ReadExact(FILE *file, void *dst, size_t size)
{
	return fread(dst, 1, size, file) == size;
}

static int NativeCheckpointFile_ValidateHeader(const struct NativeCheckpointFileHeader *header)
{
	return (header->magic == NATIVE_CHECKPOINT_FILE_MAGIC) && (header->version == NATIVE_CHECKPOINT_FILE_VERSION) && (header->headerSize == sizeof(*header)) &&
	       (header->recordHeaderSize == sizeof(struct NativeCheckpointFileRecordHeader)) && (header->recordCount == 1u);
}

int NativeCheckpointFile_WriteSingle(const char *path, const void *payload, int payloadSize, u32 checkpointIndex, u32 replayFrame)
{
	struct NativeCheckpointFileHeader header;
	struct NativeCheckpointFileRecordHeader record;
	FILE *file;
	int ok = 0;

	if ((path == NULL) || (payload == NULL) || (payloadSize <= 0))
		return 0;

	memset(&header, 0, sizeof(header));
	header.magic = NATIVE_CHECKPOINT_FILE_MAGIC;
	header.version = NATIVE_CHECKPOINT_FILE_VERSION;
	header.headerSize = (u32)sizeof(header);
	header.recordHeaderSize = (u32)sizeof(record);
	header.recordCount = 1;

	memset(&record, 0, sizeof(record));
	record.checkpointIndex = checkpointIndex;
	record.replayFrame = replayFrame;
	record.payloadOffset = (u32)(sizeof(header) + sizeof(record));
	record.payloadSize = (u32)payloadSize;
	record.checksum = NativeCheckpointFile_Checksum(payload, payloadSize);

	file = fopen(path, "wb");
	if (file == NULL)
		return 0;

	ok = NativeCheckpointFile_WriteExact(file, &header, sizeof(header)) && NativeCheckpointFile_WriteExact(file, &record, sizeof(record)) &&
	     NativeCheckpointFile_WriteExact(file, payload, (size_t)payloadSize);

	if (fclose(file) != 0)
		ok = 0;

	if (!ok)
		remove(path);

	return ok;
}

int NativeCheckpointFile_ReadSingle(const char *path, void *payload, int payloadSize, struct NativeCheckpointFileRecordInfo *info)
{
	struct NativeCheckpointFileHeader header;
	struct NativeCheckpointFileRecordHeader record;
	FILE *file;
	int ok = 0;

	if ((path == NULL) || (payload == NULL) || (payloadSize <= 0))
		return 0;

	file = fopen(path, "rb");
	if (file == NULL)
		return 0;

	if (!NativeCheckpointFile_ReadExact(file, &header, sizeof(header)))
		goto cleanup;
	if (!NativeCheckpointFile_ValidateHeader(&header))
		goto cleanup;
	if (!NativeCheckpointFile_ReadExact(file, &record, sizeof(record)))
		goto cleanup;
	if ((record.payloadOffset != (u32)(sizeof(header) + sizeof(record))) || (record.payloadSize != (u32)payloadSize))
		goto cleanup;
	if (!NativeCheckpointFile_ReadExact(file, payload, (size_t)payloadSize))
		goto cleanup;
	if (NativeCheckpointFile_Checksum(payload, payloadSize) != record.checksum)
		goto cleanup;
	if (fgetc(file) != EOF)
		goto cleanup;

	if (info != NULL)
	{
		info->checkpointIndex = record.checkpointIndex;
		info->replayFrame = record.replayFrame;
		info->payloadSize = record.payloadSize;
		info->checksum = record.checksum;
	}

	ok = 1;

cleanup:
	fclose(file);
	return ok;
}
#endif
