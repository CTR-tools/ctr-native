#include <platform/native_track_library.h>

#include <SDL3/SDL.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

enum
{
	NATIVE_TRACK_PATH_MAX = 1024,
	NATIVE_TRACK_LINE_MAX = 512,
	NATIVE_TRACK_FIELD_MAX = 256,
};

struct NativeTrackLibraryScan
{
	struct NativeTrackRegistry *registry;
	struct NativeTrackScanResult result;
};

static struct NativeTrackRegistry *s_nativeTrackLibrary;

static int NativeTrackLibrary_Join(char *output, size_t size, const char *directory, const char *name)
{
	int length = snprintf(output, size, "%s/%s", directory, name);
	return length >= 0 && (size_t)length < size;
}

static char *NativeTrackLibrary_Trim(char *text)
{
	while (isspace((unsigned char)*text))
	{
		text++;
	}
	char *end = text + strlen(text);
	while (end > text && isspace((unsigned char)end[-1]))
	{
		*--end = '\0';
	}
	return text;
}

static int NativeTrackLibrary_ReadTrack(struct NativeTrackRegistry *registry, const char *directory, const char *name)
{
	char manifest[NATIVE_TRACK_PATH_MAX];
	if (!NativeTrackLibrary_Join(manifest, sizeof(manifest), directory, "track.ini"))
	{
		return 0;
	}
	FILE *file = fopen(manifest, "rb");
	if (file == NULL && errno != ENOENT)
	{
		return 0;
	}
	char fields[3][NATIVE_TRACK_FIELD_MAX] = {{0}};
	const char *keys[] = {"id", "title", "author"};
	unsigned int seen = 0;
	char line[NATIVE_TRACK_LINE_MAX];
	int valid = 1;
	while (file != NULL && fgets(line, sizeof(line), file) != NULL)
	{
		if (strchr(line, '\n') == NULL && !feof(file))
		{
			valid = 0;
			break;
		}
		char *key = NativeTrackLibrary_Trim(line);
		if (*key == '\0' || *key == '#' || *key == ';')
		{
			continue;
		}
		char *separator = strchr(key, '=');
		if (separator == NULL)
		{
			valid = 0;
			break;
		}
		*separator = '\0';
		key = NativeTrackLibrary_Trim(key);
		char *value = NativeTrackLibrary_Trim(separator + 1);
		size_t index = 0;
		while (index < SDL_arraysize(keys) && strcmp(key, keys[index]) != 0)
		{
			index++;
		}
		unsigned int bit = 1u << index;
		if ((seen & bit) != 0 || *value == '\0')
		{
			valid = 0;
			break;
		}
		seen |= bit;
		if (index < SDL_arraysize(keys))
		{
			if (strlen(value) >= sizeof(fields[index]))
			{
				valid = 0;
				break;
			}
			strcpy(fields[index], value);
		}
		else
		{
			valid = 0;
			break;
		}
	}
	if (file != NULL && ferror(file))
	{
		valid = 0;
	}
	if (file != NULL)
	{
		fclose(file);
	}

	char paths[3][NATIVE_TRACK_PATH_MAX] = {{0}};
	const char *fileNames[] = {"track.lev", "track.vrm", "track.sca"};
	for (size_t i = 0; valid && i < SDL_arraysize(paths); i++)
	{
		SDL_PathInfo info;
		if (!NativeTrackLibrary_Join(paths[i], sizeof(paths[i]), directory, fileNames[i]))
		{
			valid = 0;
			break;
		}
		if (!SDL_GetPathInfo(paths[i], &info))
		{
			if (i == 2)
			{
				paths[i][0] = '\0';
				continue;
			}
			valid = 0;
		}
		else if (info.type != SDL_PATHTYPE_FILE)
		{
			valid = 0;
		}
	}
	if (!valid)
	{
		return 0;
	}
	char localID[NATIVE_TRACK_FIELD_MAX + sizeof("local:")];
	int length = snprintf(localID, sizeof(localID), "local:%s", name);
	if (length < 0 || (size_t)length >= sizeof(localID))
	{
		return 0;
	}
	struct NativeTrackMetadata metadata = {fields[0][0] ? fields[0] : localID, fields[1][0] ? fields[1] : name,
	                                       paths[0], paths[1], paths[2][0] ? paths[2] : NULL, fields[2][0] ? fields[2] : NULL};
	return NativeTrackRegistry_Add(registry, &metadata, NULL);
}

static SDL_EnumerationResult SDLCALL NativeTrackLibrary_Visit(void *userdata, const char *directory, const char *name)
{
	struct NativeTrackLibraryScan *scan = userdata;
	char path[NATIVE_TRACK_PATH_MAX];
	SDL_PathInfo info;
	if (!NativeTrackLibrary_Join(path, sizeof(path), directory, name))
	{
		scan->result.rejected++;
		return SDL_ENUM_CONTINUE;
	}
	if (!SDL_GetPathInfo(path, &info) || info.type != SDL_PATHTYPE_DIRECTORY)
	{
		return SDL_ENUM_CONTINUE;
	}
	if (NativeTrackLibrary_ReadTrack(scan->registry, path, name))
	{
		scan->result.installed++;
	}
	else
	{
		scan->result.rejected++;
	}
	return SDL_ENUM_CONTINUE;
}

struct NativeTrackRegistry *NativeTrackLibrary_Scan(const char *directory, struct NativeTrackScanResult *result)
{
	if (result != NULL)
	{
		memset(result, 0, sizeof(*result));
	}
	if (directory == NULL || directory[0] == '\0')
	{
		return NULL;
	}
	struct NativeTrackLibraryScan scan = {0};
	scan.registry = NativeTrackRegistry_Create();
	if (scan.registry == NULL)
	{
		return NULL;
	}
	if (!SDL_EnumerateDirectory(directory, NativeTrackLibrary_Visit, &scan))
	{
		NativeTrackRegistry_Destroy(scan.registry);
		return NULL;
	}
	if (result != NULL)
	{
		*result = scan.result;
	}
	return scan.registry;
}

int NativeTrackLibrary_Refresh(const char *directory, struct NativeTrackScanResult *result)
{
	struct NativeTrackRegistry *replacement = NativeTrackLibrary_Scan(directory, result);
	if (replacement == NULL)
	{
		return 0;
	}
	NativeTrackRegistry_Destroy(s_nativeTrackLibrary);
	s_nativeTrackLibrary = replacement;
	return 1;
}

const struct NativeTrackRegistry *NativeTrackLibrary_Current(void)
{
	return s_nativeTrackLibrary;
}

void NativeTrackLibrary_Shutdown(void)
{
	NativeTrackRegistry_Destroy(s_nativeTrackLibrary);
	s_nativeTrackLibrary = NULL;
}
