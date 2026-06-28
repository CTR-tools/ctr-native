#include "platform/native_checkpoint_file.h"

#include <macros.h>

#if defined(CTR_INTERNAL)
#include <stdio.h>
#include <string.h>

#define NATIVE_CHECKPOINT_FILE_FOURCC(a, b, c, d) ((u32)(a) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

// NOTE(aalhendi): `CTST` means CTR native State checkpoint container. The file
// can grow to rolling checkpoint records without changing the `.ctrstates`
// extension introduced by this single-record persistence slice.
#define NATIVE_CHECKPOINT_FILE_MAGIC              NATIVE_CHECKPOINT_FILE_FOURCC('C', 'T', 'S', 'T')
#define NATIVE_CHECKPOINT_FILE_VERSION            1u
#define NATIVE_CHECKPOINT_FILE_CHUNK_SIZE         4096u
#define NATIVE_CHECKPOINT_FILE_FNV_OFFSET         2166136261u
#define NATIVE_CHECKPOINT_FILE_FNV_PRIME          16777619u

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

internal u32 NativeCheckpointFile_Checksum(const void *src, int size)
{
	const u8 *bytes = (const u8 *)src;
	u32 hash = NATIVE_CHECKPOINT_FILE_FNV_OFFSET;
	int i;

	for (i = 0; i < size; i++)
	{
		hash ^= bytes[i];
		hash *= NATIVE_CHECKPOINT_FILE_FNV_PRIME;
	}

	return hash;
}

internal void NativeCheckpointFile_InitHeader(struct NativeCheckpointFileHeader *header)
{
	memset(header, 0, sizeof(*header));
	header->magic = NATIVE_CHECKPOINT_FILE_MAGIC;
	header->version = NATIVE_CHECKPOINT_FILE_VERSION;
	header->headerSize = (u32)sizeof(*header);
	header->recordHeaderSize = (u32)sizeof(struct NativeCheckpointFileRecordHeader);
}

internal int NativeCheckpointFile_WriteExact(FILE *file, const void *src, size_t size)
{
	return fwrite(src, 1, size, file) == size;
}

internal int NativeCheckpointFile_ReadExact(FILE *file, void *dst, size_t size)
{
	return fread(dst, 1, size, file) == size;
}

internal int NativeCheckpointFile_ValidateHeader(const struct NativeCheckpointFileHeader *header)
{
	return (header->magic == NATIVE_CHECKPOINT_FILE_MAGIC) && (header->version == NATIVE_CHECKPOINT_FILE_VERSION) && (header->headerSize == sizeof(*header)) &&
	       (header->recordHeaderSize == sizeof(struct NativeCheckpointFileRecordHeader));
}

internal int NativeCheckpointFile_WriteHeader(FILE *file, const struct NativeCheckpointFileHeader *header)
{
	long oldPos;

	if ((file == NULL) || (header == NULL))
	{
		return 0;
	}

	oldPos = ftell(file);
	if (oldPos < 0)
	{
		return 0;
	}

	if (fseek(file, 0, SEEK_SET) != 0)
	{
		return 0;
	}
	if (!NativeCheckpointFile_WriteExact(file, header, sizeof(*header)))
	{
		return 0;
	}
	if (fseek(file, oldPos, SEEK_SET) != 0)
	{
		return 0;
	}

	return 1;
}

internal void NativeCheckpointFile_FillInfo(struct NativeCheckpointFileRecordInfo *info, const struct NativeCheckpointFileRecordHeader *record)
{
	if (info == NULL)
	{
		return;
	}

	info->checkpointIndex = record->checkpointIndex;
	info->replayFrame = record->replayFrame;
	info->payloadOffset = record->payloadOffset;
	info->payloadSize = record->payloadSize;
	info->checksum = record->checksum;
}

internal int NativeCheckpointFile_ChecksumStream(FILE *file, u32 payloadSize, u32 *checksumOut)
{
	u8 buffer[NATIVE_CHECKPOINT_FILE_CHUNK_SIZE];
	u32 hash = NATIVE_CHECKPOINT_FILE_FNV_OFFSET;
	u32 remaining = payloadSize;

	if ((file == NULL) || (checksumOut == NULL))
	{
		return 0;
	}

	while (remaining != 0)
	{
		u32 chunkSize = remaining;
		u32 i;

		if (chunkSize > sizeof(buffer))
		{
			chunkSize = sizeof(buffer);
		}

		if (!NativeCheckpointFile_ReadExact(file, buffer, chunkSize))
		{
			return 0;
		}

		for (i = 0; i < chunkSize; i++)
		{
			hash ^= buffer[i];
			hash *= NATIVE_CHECKPOINT_FILE_FNV_PRIME;
		}

		remaining -= chunkSize;
	}

	*checksumOut = hash;
	return 1;
}

int NativeCheckpointFile_BeginWrite(struct NativeCheckpointFileWriter *writer, const char *path)
{
	struct NativeCheckpointFileHeader header;
	FILE *file;

	if ((writer == NULL) || (path == NULL))
	{
		return 0;
	}

	memset(writer, 0, sizeof(*writer));
	NativeCheckpointFile_InitHeader(&header);

	file = fopen(path, "wb+");
	if (file == NULL)
	{
		return 0;
	}

	if (!NativeCheckpointFile_WriteExact(file, &header, sizeof(header)))
	{
		fclose(file);
		remove(path);
		return 0;
	}

	writer->file = file;
	writer->recordCount = 0;
	return 1;
}

int NativeCheckpointFile_AppendRecord(struct NativeCheckpointFileWriter *writer, const void *payload, int payloadSize, u32 checkpointIndex, u32 replayFrame,
                                      struct NativeCheckpointFileRecordInfo *info)
{
	struct NativeCheckpointFileRecordHeader record;
	FILE *file;
	long recordOffset;
	long payloadOffset;

	if ((writer == NULL) || (writer->file == NULL) || (payload == NULL) || (payloadSize <= 0))
	{
		return 0;
	}

	file = (FILE *)writer->file;
	recordOffset = ftell(file);
	if (recordOffset < 0)
	{
		return 0;
	}
	payloadOffset = recordOffset + (long)sizeof(record);
	// TODO(aalhendi): move replay/checkpoint persistence to 64-bit offsets
	// before playtester packaging can produce multi-hour reports.
	if ((payloadOffset < 0) || ((unsigned long)payloadOffset > 0xffffffffUL))
	{
		return 0;
	}

	memset(&record, 0, sizeof(record));
	record.checkpointIndex = checkpointIndex;
	record.replayFrame = replayFrame;
	record.payloadOffset = (u32)payloadOffset;
	record.payloadSize = (u32)payloadSize;
	record.checksum = NativeCheckpointFile_Checksum(payload, payloadSize);

	if (!NativeCheckpointFile_WriteExact(file, &record, sizeof(record)))
	{
		return 0;
	}
	if (!NativeCheckpointFile_WriteExact(file, payload, (size_t)payloadSize))
	{
		return 0;
	}

	writer->recordCount++;
	{
		struct NativeCheckpointFileHeader header;

		NativeCheckpointFile_InitHeader(&header);
		header.recordCount = writer->recordCount;
		if (!NativeCheckpointFile_WriteHeader(file, &header))
		{
			return 0;
		}
		fflush(file);
	}
	NativeCheckpointFile_FillInfo(info, &record);
	return 1;
}

int NativeCheckpointFile_EndWrite(struct NativeCheckpointFileWriter *writer)
{
	struct NativeCheckpointFileHeader header;
	FILE *file;
	int ok = 1;

	if ((writer == NULL) || (writer->file == NULL))
	{
		return 1;
	}

	file = (FILE *)writer->file;
	NativeCheckpointFile_InitHeader(&header);
	header.recordCount = writer->recordCount;

	if (!NativeCheckpointFile_WriteHeader(file, &header))
	{
		ok = 0;
	}

	if (fclose(file) != 0)
	{
		ok = 0;
	}

	writer->file = NULL;
	writer->recordCount = 0;
	return ok;
}

int NativeCheckpointFile_Validate(const char *path, struct NativeCheckpointFileRecordInfo *records, int maxRecords, int *recordCount)
{
	struct NativeCheckpointFileHeader header;
	FILE *file;
	u32 i;
	int ok = 0;

	if ((path == NULL) || (maxRecords < 0))
	{
		return 0;
	}
	if ((maxRecords > 0) && (records == NULL))
	{
		return 0;
	}

	file = fopen(path, "rb");
	if (file == NULL)
	{
		return 0;
	}

	if (!NativeCheckpointFile_ReadExact(file, &header, sizeof(header)))
	{
		goto cleanup;
	}
	if (!NativeCheckpointFile_ValidateHeader(&header))
	{
		goto cleanup;
	}
	if ((records != NULL) && (header.recordCount > (u32)maxRecords))
	{
		goto cleanup;
	}

	for (i = 0; i < header.recordCount; i++)
	{
		struct NativeCheckpointFileRecordHeader record;
		long expectedPayloadOffset;
		u32 checksum;

		if (!NativeCheckpointFile_ReadExact(file, &record, sizeof(record)))
		{
			goto cleanup;
		}

		expectedPayloadOffset = ftell(file);
		if ((expectedPayloadOffset < 0) || (record.payloadOffset != (u32)expectedPayloadOffset) || (record.payloadSize == 0))
		{
			goto cleanup;
		}
		if (!NativeCheckpointFile_ChecksumStream(file, record.payloadSize, &checksum))
		{
			goto cleanup;
		}
		if (checksum != record.checksum)
		{
			goto cleanup;
		}
		if ((records != NULL) && (i < (u32)maxRecords))
		{
			NativeCheckpointFile_FillInfo(&records[i], &record);
		}
	}

	if (fgetc(file) != EOF)
	{
		goto cleanup;
	}

	if (recordCount != NULL)
	{
		*recordCount = (int)header.recordCount;
	}

	ok = 1;

cleanup:
	fclose(file);
	return ok;
}

int NativeCheckpointFile_ReadRecord(const char *path, u32 checkpointIndex, void *payload, int payloadSize, struct NativeCheckpointFileRecordInfo *info)
{
	struct NativeCheckpointFileHeader header;
	FILE *file;
	u32 i;
	int ok = 0;

	if ((path == NULL) || (payload == NULL) || (payloadSize <= 0))
	{
		return 0;
	}

	file = fopen(path, "rb");
	if (file == NULL)
	{
		return 0;
	}

	if (!NativeCheckpointFile_ReadExact(file, &header, sizeof(header)))
	{
		goto cleanup;
	}
	if (!NativeCheckpointFile_ValidateHeader(&header))
	{
		goto cleanup;
	}

	for (i = 0; i < header.recordCount; i++)
	{
		struct NativeCheckpointFileRecordHeader record;
		long expectedPayloadOffset;

		if (!NativeCheckpointFile_ReadExact(file, &record, sizeof(record)))
		{
			goto cleanup;
		}

		expectedPayloadOffset = ftell(file);
		if ((expectedPayloadOffset < 0) || (record.payloadOffset != (u32)expectedPayloadOffset) || (record.payloadSize == 0))
		{
			goto cleanup;
		}

		if (record.checkpointIndex == checkpointIndex)
		{
			if (record.payloadSize != (u32)payloadSize)
			{
				goto cleanup;
			}
			if (!NativeCheckpointFile_ReadExact(file, payload, (size_t)payloadSize))
			{
				goto cleanup;
			}
			if (NativeCheckpointFile_Checksum(payload, payloadSize) != record.checksum)
			{
				goto cleanup;
			}

			NativeCheckpointFile_FillInfo(info, &record);
			ok = 1;
			goto cleanup;
		}

		if (fseek(file, (long)record.payloadSize, SEEK_CUR) != 0)
		{
			goto cleanup;
		}
	}

cleanup:
	fclose(file);
	return ok;
}

int NativeCheckpointFile_WriteSingle(const char *path, const void *payload, int payloadSize, u32 checkpointIndex, u32 replayFrame)
{
	struct NativeCheckpointFileWriter writer;
	int ok;

	if ((path == NULL) || (payload == NULL) || (payloadSize <= 0))
	{
		return 0;
	}

	memset(&writer, 0, sizeof(writer));
	ok = NativeCheckpointFile_BeginWrite(&writer, path) &&
	     NativeCheckpointFile_AppendRecord(&writer, payload, payloadSize, checkpointIndex, replayFrame, NULL) && NativeCheckpointFile_EndWrite(&writer);

	if (!ok)
	{
		NativeCheckpointFile_EndWrite(&writer);
		remove(path);
	}

	return ok;
}

int NativeCheckpointFile_ReadSingle(const char *path, void *payload, int payloadSize, struct NativeCheckpointFileRecordInfo *info)
{
	struct NativeCheckpointFileHeader header;
	struct NativeCheckpointFileRecordHeader record;
	FILE *file;
	int ok = 0;

	if ((path == NULL) || (payload == NULL) || (payloadSize <= 0))
	{
		return 0;
	}

	file = fopen(path, "rb");
	if (file == NULL)
	{
		return 0;
	}

	if (!NativeCheckpointFile_ReadExact(file, &header, sizeof(header)))
	{
		goto cleanup;
	}
	if (!NativeCheckpointFile_ValidateHeader(&header))
	{
		goto cleanup;
	}
	if (header.recordCount != 1u)
	{
		goto cleanup;
	}
	if (!NativeCheckpointFile_ReadExact(file, &record, sizeof(record)))
	{
		goto cleanup;
	}
	if ((record.payloadOffset != (u32)(sizeof(header) + sizeof(record))) || (record.payloadSize != (u32)payloadSize))
	{
		goto cleanup;
	}
	if (!NativeCheckpointFile_ReadExact(file, payload, (size_t)payloadSize))
	{
		goto cleanup;
	}
	if (NativeCheckpointFile_Checksum(payload, payloadSize) != record.checksum)
	{
		goto cleanup;
	}
	if (fgetc(file) != EOF)
	{
		goto cleanup;
	}

	NativeCheckpointFile_FillInfo(info, &record);

	ok = 1;

cleanup:
	fclose(file);
	return ok;
}
#endif
