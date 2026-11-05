#include <platform/native_track_file.h>
#include <platform/native_track_validate.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { NATIVE_TRACK_FILE_SECTOR_SIZE = 2048 };

struct NativeTrackFile
{
	FILE *stream;
	size_t size;
	size_t sectorBytes;
	enum NativeTrackFileRole role;
};

struct NativeTrackFile *NativeTrackFile_Open(const struct NativeTrackRegistry *registry, int levelID, enum NativeTrackFileRole role)
{
	const struct NativeTrackMetadata *metadata = NativeTrackRegistry_Get(registry, levelID);
	if (metadata == NULL)
	{
		return NULL;
	}
	const char *path = NULL;
	switch (role)
	{
	case NATIVE_TRACK_FILE_LEV: path = metadata->levPath; break;
	case NATIVE_TRACK_FILE_VRM: path = metadata->vrmPath; break;
	case NATIVE_TRACK_FILE_SCA: path = metadata->scaPath; break;
	default: return NULL;
	}
	if (path == NULL)
	{
		return NULL;
	}
	FILE *stream = fopen(path, "rb");
	if (stream == NULL)
	{
		return NULL;
	}
	if (fseek(stream, 0, SEEK_END) != 0)
	{
		fclose(stream);
		return NULL;
	}
	long length = ftell(stream);
	if (length <= 0 || length > INT_MAX - (NATIVE_TRACK_FILE_SECTOR_SIZE - 1) || fseek(stream, 0, SEEK_SET) != 0)
	{
		fclose(stream);
		return NULL;
	}
	struct NativeTrackFile *file = malloc(sizeof(*file));
	if (file == NULL)
	{
		fclose(stream);
		return NULL;
	}
	file->stream = stream;
	file->size = (size_t)length;
	file->sectorBytes = (file->size + NATIVE_TRACK_FILE_SECTOR_SIZE - 1) / NATIVE_TRACK_FILE_SECTOR_SIZE * NATIVE_TRACK_FILE_SECTOR_SIZE;
	file->role = role;
	return file;
}

size_t NativeTrackFile_Size(const struct NativeTrackFile *file)
{
	return file != NULL ? file->size : 0;
}

size_t NativeTrackFile_SectorBytes(const struct NativeTrackFile *file)
{
	return file != NULL ? file->sectorBytes : 0;
}

int NativeTrackFile_Read(struct NativeTrackFile *file, void *destination, size_t capacity)
{
	if (file == NULL || destination == NULL || capacity < file->sectorBytes || fseek(file->stream, 0, SEEK_SET) != 0)
	{
		return 0;
	}
	if (fread(destination, 1, file->size, file->stream) != file->size || fgetc(file->stream) != EOF || ferror(file->stream))
	{
		return 0;
	}
	if ((file->role == NATIVE_TRACK_FILE_LEV && !NativeTrackValidate_Lev(destination, file->size)) ||
	    (file->role == NATIVE_TRACK_FILE_VRM && !NativeTrackValidate_Vrm(destination, file->size)))
	{
		return 0;
	}
	memset((unsigned char *)destination + file->size, 0, file->sectorBytes - file->size);
	return 1;
}

void NativeTrackFile_Close(struct NativeTrackFile *file)
{
	if (file != NULL)
	{
		fclose(file->stream);
		free(file);
	}
}
