#include <platform/native_track_records.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

/* Hashes identify track revisions; the full identity is also stored in each file. */
#define TRACK_RECORD_HASH_OFFSET UINT64_C(14695981039346656037)
#define TRACK_RECORD_HASH_PRIME UINT64_C(1099511628211)
enum { TRACK_RECORD_ID_MAX = 512, TRACK_RECORD_PATH_MAX = 1024, TRACK_RECORD_LAPS = 3 };

static uint64_t TrackRecords_Hash(uint64_t value, const void *bytes, size_t size)
{
	const unsigned char *p = bytes;
	for (size_t i = 0; i < size; i++)
	{
		value = (value ^ p[i]) * TRACK_RECORD_HASH_PRIME;
	}
	return value;
}

static int TrackRecords_HashFile(const char *path, uint64_t *result)
{
	FILE *file = fopen(path, "rb");
	if (file == NULL)
	{
		return 0;
	}
	unsigned char buffer[8192];
	uint64_t value = TRACK_RECORD_HASH_OFFSET;
	size_t count;
	while ((count = fread(buffer, 1, sizeof(buffer), file)) != 0)
	{
		value = TrackRecords_Hash(value, buffer, count);
	}
	int ok = !ferror(file);
	fclose(file);
	*result = value;
	return ok;
}

int NativeTrackRecords_Key(const struct NativeTrackMetadata *track, struct NativeTrackRecordKey *key)
{
	if (track == NULL || key == NULL || track->identity == NULL || track->levPath == NULL || track->vrmPath == NULL ||
	    strlen(track->identity) >= TRACK_RECORD_ID_MAX)
	{
		return 0;
	}
	key->identity = TrackRecords_Hash(TRACK_RECORD_HASH_OFFSET, track->identity, strlen(track->identity));
	return TrackRecords_HashFile(track->levPath, &key->lev) && TrackRecords_HashFile(track->vrmPath, &key->vrm);
}

static int TrackRecords_Path(char *path, size_t capacity, const char *directory, const struct NativeTrackRecordKey *key)
{
	int written = snprintf(path, capacity, "%s/%016llx-%016llx-%016llx-%dl.dat", directory,
	                       (unsigned long long)key->identity, (unsigned long long)key->lev,
	                       (unsigned long long)key->vrm, TRACK_RECORD_LAPS);
	return written >= 0 && (size_t)written < capacity;
}

int NativeTrackRecords_Load(const char *directory, const struct NativeTrackMetadata *track,
                            const struct NativeTrackRecordKey *key, void *data, size_t size)
{
	char path[TRACK_RECORD_PATH_MAX], identity[TRACK_RECORD_ID_MAX];
	unsigned int storedSize, identitySize;
	if (directory == NULL || track == NULL || key == NULL || data == NULL ||
	    !TrackRecords_Path(path, sizeof(path), directory, key))
	{
		return 0;
	}
	FILE *file = fopen(path, "rb");
	if (file == NULL) { return 0; }
	char magic[8];
	int ok = fread(magic, 1, sizeof(magic), file) == sizeof(magic) && memcmp(magic, "CTRTIM01", 8) == 0 &&
	         fread(&identitySize, sizeof(identitySize), 1, file) == 1 && identitySize > 0 && identitySize < sizeof(identity) &&
	         fread(&storedSize, sizeof(storedSize), 1, file) == 1 && storedSize == size &&
	         fread(identity, 1, identitySize, file) == identitySize;
	if (ok)
	{
		identity[identitySize] = '\0';
		ok = strcmp(identity, track->identity) == 0 && fread(data, 1, size, file) == size && fgetc(file) == EOF && !ferror(file);
	}
	fclose(file);
	return ok;
}

int NativeTrackRecords_Save(const char *directory, const struct NativeTrackMetadata *track,
                            const struct NativeTrackRecordKey *key, const void *data, size_t size)
{
	char path[TRACK_RECORD_PATH_MAX], temporary[TRACK_RECORD_PATH_MAX];
	if (directory == NULL || track == NULL || key == NULL || data == NULL || size > UINT32_MAX ||
	    !SDL_CreateDirectory(directory) || !TrackRecords_Path(path, sizeof(path), directory, key))
	{
		return 0;
	}
	int n = snprintf(temporary, sizeof(temporary), "%s.tmp", path);
	if (n < 0 || (size_t)n >= sizeof(temporary)) { return 0; }
	FILE *file = fopen(temporary, "wb");
	if (file == NULL) { return 0; }
	unsigned int identitySize = (unsigned int)strlen(track->identity), dataSize = (unsigned int)size;
	int ok = fwrite("CTRTIM01", 1, 8, file) == 8 && fwrite(&identitySize, sizeof(identitySize), 1, file) == 1 &&
	         fwrite(&dataSize, sizeof(dataSize), 1, file) == 1 && fwrite(track->identity, 1, identitySize, file) == identitySize &&
	         fwrite(data, 1, size, file) == size;
	if (fclose(file) != 0) { ok = 0; }
	if (ok) { ok = SDL_RenamePath(temporary, path); }
	if (!ok) { SDL_RemovePath(temporary); }
	return ok;
}
