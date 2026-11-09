#include <platform/native_track_library.h>

#include <SDL3/SDL.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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

enum
{
	TRACK_DETECT_MAX_BYTES = 64 * 1024 * 1024,
	TRACK_DETECT_HEADER_BYTES = 0x18c,
	TRACK_DETECT_INST_BYTES = 0x40,
	TRACK_DETECT_NAV_TABLE_OFFSET = 0x188,
	TRACK_DETECT_NAV_PATHS = 3,
	TRACK_DETECT_NAV_HEADER_BYTES = 0x4c,
	TRACK_DETECT_NAV_FRAME_BYTES = 0x14,
	TRACK_DETECT_NAV_MAGIC = 0xecfd,
	TRACK_DETECT_MODEL_ID_OFFSET = 0x10,
	TRACK_DETECT_TIME_1 = 0x5c,
	TRACK_DETECT_TIME_2 = 0x64,
	TRACK_DETECT_TIME_3 = 0x65,
	TRACK_DETECT_CRYSTAL = 0x60,
	TRACK_DETECT_C = 0x93,
	TRACK_DETECT_R = 0x95,
};

static uint32_t TrackDetect_Read32(const unsigned char *p)
{
	return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int TrackDetect_BotPaths(const unsigned char *payload, uint32_t size)
{
	uint32_t table = TrackDetect_Read32(payload + TRACK_DETECT_NAV_TABLE_OFFSET);
	if (table == 0)
	{
		return 0;
	}
	if (table > size || size - table < TRACK_DETECT_NAV_PATHS * 4)
	{
		return -1;
	}
	int valid = 0;
	for (int i = 0; i < TRACK_DETECT_NAV_PATHS; i++)
	{
		uint32_t path = TrackDetect_Read32(payload + table + i * 4);
		if (path == 0) { continue; }
		if (path > size || size - path < TRACK_DETECT_NAV_HEADER_BYTES) { return -1; }
		unsigned int magic = payload[path] | (unsigned int)payload[path + 1] << 8;
		unsigned int points = payload[path + 2] | (unsigned int)payload[path + 3] << 8;
		if (magic != TRACK_DETECT_NAV_MAGIC || points < 2 || points > 0x7fff) { continue; }
		if (points + 1 > (size - path - TRACK_DETECT_NAV_HEADER_BYTES) / TRACK_DETECT_NAV_FRAME_BYTES) { return -1; }
		valid = 1;
	}
	return valid;
}

/* These are hints about placed objects, not a gameplay validation. LEV pointers
   are offsets from the payload, not from the beginning of the file. */
static int TrackDetect_Lev(const unsigned char *bytes, size_t size, unsigned int *modes)
{
	if (bytes == NULL || size < 4 + TRACK_DETECT_HEADER_BYTES)
	{
		return 0;
	}
	uint32_t payloadSize = TrackDetect_Read32(bytes);
	if (payloadSize > size - 4 || payloadSize < TRACK_DETECT_HEADER_BYTES)
	{
		return 0;
	}
	const unsigned char *payload = bytes + 4;
	int botPaths = TrackDetect_BotPaths(payload, payloadSize);
	if (botPaths < 0)
	{
		return 0;
	}
	uint32_t count = TrackDetect_Read32(payload + 0x0c);
	uint32_t instances = TrackDetect_Read32(payload + 0x10);
	if (instances > payloadSize || count > (payloadSize - instances) / TRACK_DETECT_INST_BYTES || (count != 0 && instances == 0))
	{
		return 0;
	}
	unsigned int found = NATIVE_TRACK_DETECTED_TIME_TRIAL | NATIVE_TRACK_DETECTED_VS;
	unsigned int letters = 0;
	for (uint32_t i = 0; i < count; i++)
	{
		const unsigned char *inst = payload + instances + i * TRACK_DETECT_INST_BYTES;
		uint32_t model = TrackDetect_Read32(inst + 0x10);
		if (model == 0 || model > payloadSize || payloadSize - model < TRACK_DETECT_MODEL_ID_OFFSET + 2)
		{
			return 0;
		}
		unsigned int id = payload[model + TRACK_DETECT_MODEL_ID_OFFSET] | (unsigned int)payload[model + TRACK_DETECT_MODEL_ID_OFFSET + 1] << 8;
		if (id == TRACK_DETECT_TIME_1 || id == TRACK_DETECT_TIME_2 || id == TRACK_DETECT_TIME_3) { found |= NATIVE_TRACK_DETECTED_RELIC; }
		if (id == TRACK_DETECT_CRYSTAL) { found |= NATIVE_TRACK_DETECTED_CRYSTAL; }
		if (id >= TRACK_DETECT_C && id <= TRACK_DETECT_R) { letters |= 1u << (id - TRACK_DETECT_C); }
	}
	if (botPaths)
	{
		found |= NATIVE_TRACK_DETECTED_ARCADE;
	}
	if (letters == 7) { found |= NATIVE_TRACK_DETECTED_CTR; }
	*modes = found;
	return 1;
}

static int TrackDetect_File(const char *path, unsigned int *modes)
{
	SDL_PathInfo info;
	if (!SDL_GetPathInfo(path, &info) || info.type != SDL_PATHTYPE_FILE || info.size > TRACK_DETECT_MAX_BYTES)
	{
		return 0;
	}
	size_t size = 0;
	unsigned char *bytes = SDL_LoadFile(path, &size);
	int known = TrackDetect_Lev(bytes, size, modes);
	SDL_free(bytes);
	return known;
}

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

int NativeTrackLibrary_ReadDirectory(struct NativeTrackRegistry *registry, const char *directory, const char *name)
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
	char fields[4][NATIVE_TRACK_FIELD_MAX + 7] = {{0}};
	const char *keys[] = {"id", "title", "author", "music"};
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
		if ((seen & bit) != 0 || (*value == '\0' && index != 3))
		{
			valid = 0;
			break;
		}
		seen |= bit;
		if (index < SDL_arraysize(keys))
		{
			if (strlen(value) >= (index == 3 ? sizeof(fields[index]) : NATIVE_TRACK_FIELD_MAX))
			{
				if (index == 3) { strcpy(fields[index], "invalid"); continue; }
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
	if ((seen & (1u << 3)) != 0 && fields[3][0] == '\0') { strcpy(fields[3], "invalid"); }
	char localID[NATIVE_TRACK_FIELD_MAX + sizeof("local:")];
	int length = snprintf(localID, sizeof(localID), "local:%s", name);
	if (length < 0 || (size_t)length >= sizeof(localID))
	{
		return 0;
	}
	struct NativeTrackMetadata metadata = {fields[0][0] ? fields[0] : localID, fields[1][0] ? fields[1] : name,
	                                       paths[0], paths[1], paths[2][0] ? paths[2] : NULL, fields[2][0] ? fields[2] : NULL,
	                                       0, 0, fields[3][0] ? fields[3] : NULL};
	metadata.detectionKnown = TrackDetect_File(paths[0], &metadata.detectedModes);
	return NativeTrackRegistry_Add(registry, &metadata, NULL);
}

static SDL_EnumerationResult SDLCALL NativeTrackLibrary_Visit(void *userdata, const char *directory, const char *name)
{
	struct NativeTrackLibraryScan *scan = userdata;
	if (strncmp(name, ".import-", 8) == 0)
	{
		return SDL_ENUM_CONTINUE;
	}
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
	if (NativeTrackLibrary_ReadDirectory(scan->registry, path, name))
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
	NativeTrackLibrary_Replace(replacement);
	return 1;
}

void NativeTrackLibrary_Replace(struct NativeTrackRegistry *replacement)
{
	if (replacement != NULL)
	{
		struct NativeTrackRegistry *normalized = NativeTrackRegistry_Create();
		int valid = normalized != NULL;
		char fallback[32];
		snprintf(fallback, sizeof(fallback), "retail:%d", NATIVE_TRACK_DEFAULT_RETAIL_MUSIC);
		for (size_t i = 0; valid && i < NativeTrackRegistry_Count(replacement); i++)
		{
			const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(replacement, NATIVE_TRACK_CUSTOM_ID_BASE + (int)i);
			struct NativeTrackMetadata copy = *track;
			if (track->music != NULL && NativeTrackLibrary_Music(track) < 0 &&
			    (strcmp(track->music, "custom") != 0 || track->scaPath == NULL))
			{
				copy.music = track->scaPath != NULL ? "custom" : fallback;
			}
			valid = NativeTrackRegistry_Add(normalized, &copy, NULL);
		}
		if (valid)
		{
			for (size_t i = 0; i < NativeTrackRegistry_Count(replacement); i++)
			{
				const struct NativeTrackMetadata *old = NativeTrackRegistry_Get(replacement, NATIVE_TRACK_CUSTOM_ID_BASE + (int)i);
				const struct NativeTrackMetadata *fixed = NativeTrackRegistry_Get(normalized, NATIVE_TRACK_CUSTOM_ID_BASE + (int)i);
				if (old->music != NULL && fixed->music != NULL && strcmp(old->music, fixed->music) != 0)
				{
					if (!NativeTrackLibrary_SetMusic(old, old->scaPath != NULL ? -1 : NATIVE_TRACK_DEFAULT_RETAIL_MUSIC, old->scaPath != NULL))
					{
						SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, "Cannot persist music fallback for track %s", old->identity);
					}
				}
			}
			NativeTrackRegistry_Destroy(replacement);
			replacement = normalized;
		}
		else { NativeTrackRegistry_Destroy(normalized); }
		NativeTrackRegistry_Destroy(s_nativeTrackLibrary);
		s_nativeTrackLibrary = replacement;
	}
}

const struct NativeTrackRegistry *NativeTrackLibrary_Current(void)
{
	return s_nativeTrackLibrary;
}

static int NativeTrackLibrary_DisabledPath(const struct NativeTrackMetadata *track, char *path, size_t size)
{
	if (track == NULL || track->levPath == NULL)
	{
		return 0;
	}
	const char *separator = strrchr(track->levPath, '/');
	const char *backslash = strrchr(track->levPath, '\\');
	if (backslash != NULL && (separator == NULL || backslash > separator))
	{
		separator = backslash;
	}
	if (separator == NULL || (size_t)(separator - track->levPath) + sizeof("/.ctr-disabled") > size)
	{
		return 0;
	}
	size_t prefix = (size_t)(separator - track->levPath);
	memcpy(path, track->levPath, prefix);
	memcpy(path + prefix, "/.ctr-disabled", sizeof("/.ctr-disabled"));
	return 1;
}

static int NativeTrackLibrary_TrackPath(const struct NativeTrackMetadata *track, const char *name, char *path, size_t size)
{
	if (track == NULL || track->levPath == NULL) { return 0; }
	const char *slash = strrchr(track->levPath, '/');
	const char *backslash = strrchr(track->levPath, '\\');
	if (backslash != NULL && (slash == NULL || backslash > slash)) { slash = backslash; }
	if (slash == NULL || (size_t)(slash - track->levPath) + 1 + strlen(name) + 1 > size) { return 0; }
	size_t length = (size_t)(slash - track->levPath);
	memcpy(path, track->levPath, length);
	path[length++] = '/';
	strcpy(path + length, name);
	return 1;
}

int NativeTrackLibrary_Music(const struct NativeTrackMetadata *track)
{
	if (track == NULL || track->music == NULL) { return -1; }
	const char *line = track->music;
	if (strncmp(line, "retail:", 7) == 0)
	{
		char *end;
		long level = strtol(line + 7, &end, 10);
		if (end != line + 7 && *end == '\0' && level >= 0 && level < 18) { return (int)level; }
	}
	return -1;
}

int NativeTrackLibrary_ValidText(const char *text, int allowEmpty)
{
	if (text == NULL || strlen(text) >= NATIVE_TRACK_FIELD_MAX) { return 0; }
	int visible = 0;
	for (const unsigned char *p = (const unsigned char *)text; *p != 0; p++)
	{
		if (*p < 32 || *p == 127) { return 0; }
		if (*p != ' ') { visible = 1; }
	}
	return visible || (allowEmpty && text[0] == '\0');
}

static int NativeTrackLibrary_WriteFields(const struct NativeTrackMetadata *track, const char *title, const char *author, const char *music)
{
	char path[NATIVE_TRACK_PATH_MAX], temporary[NATIVE_TRACK_PATH_MAX];
	if (!NativeTrackLibrary_TrackPath(track, "track.ini", path, sizeof(path)) ||
	    strlen(path) + sizeof(".tmp") > sizeof(temporary) ||
	    !NativeTrackLibrary_ValidText(title, 0) || !NativeTrackLibrary_ValidText(author, 1)) { return 0; }
	strcpy(temporary, path);
	strcat(temporary, ".tmp");
	FILE *file = fopen(temporary, "wb");
	if (file == NULL) { return 0; }
	int ok = fprintf(file, "id = %s\ntitle = %s\n", track->identity, title) > 0;
	if (ok && author[0] != '\0') { ok = fprintf(file, "author = %s\n", author) > 0; }
	if (ok && music != NULL && music[0] != '\0') { ok = fprintf(file, "music = %s\n", music) > 0; }
	if (fclose(file) != 0) { ok = 0; }
	if (ok) { ok = SDL_RenamePath(temporary, path); }
	if (!ok) { SDL_RemovePath(temporary); }
	return ok;
}

int NativeTrackLibrary_SetDetails(const struct NativeTrackMetadata *track, const char *title, const char *author)
{
	return track != NULL && NativeTrackLibrary_WriteFields(track, title, author, track->music);
}

int NativeTrackLibrary_WriteMetadata(const struct NativeTrackMetadata *track, const char *title, const char *author,
	                                     int retail, int custom)
{
	if (track == NULL || retail >= 18) { return 0; }
	char music[32];
	if (retail >= 0) { snprintf(music, sizeof(music), "retail:%d", retail); }
	else { snprintf(music, sizeof(music), "%s", custom ? "custom" : ""); }
	return NativeTrackLibrary_WriteFields(track, title, author, music);
}

int NativeTrackLibrary_SetMusic(const struct NativeTrackMetadata *track, int retail, int custom)
{
	if (track == NULL || (retail < 0 && !custom)) { return 0; }
	return NativeTrackLibrary_WriteMetadata(track, track->title, track->author != NULL ? track->author : "", retail, custom);
}

int NativeTrackLibrary_IsEnabled(const struct NativeTrackMetadata *track)
{
	char path[NATIVE_TRACK_PATH_MAX];
	SDL_PathInfo info;
	return NativeTrackLibrary_DisabledPath(track, path, sizeof(path)) && !SDL_GetPathInfo(path, &info);
}

int NativeTrackLibrary_AllowsDetectedMode(const struct NativeTrackMetadata *track, unsigned int mode)
{
	return track != NULL && (mode == NATIVE_TRACK_DETECTED_VS || mode == NATIVE_TRACK_DETECTED_TIME_TRIAL ||
	                         (mode == NATIVE_TRACK_DETECTED_ARCADE && track->detectionKnown && (track->detectedModes & mode) != 0));
}

int NativeTrackLibrary_SetEnabled(const struct NativeTrackMetadata *track, int enabled)
{
	char path[NATIVE_TRACK_PATH_MAX];
	SDL_PathInfo info;
	if (!NativeTrackLibrary_DisabledPath(track, path, sizeof(path)))
	{
		return 0;
	}
	if (enabled)
	{
		return !SDL_GetPathInfo(path, &info) || (info.type == SDL_PATHTYPE_FILE && SDL_RemovePath(path));
	}
	if (SDL_GetPathInfo(path, &info))
	{
		return info.type == SDL_PATHTYPE_FILE;
	}
	FILE *file = fopen(path, "wb");
	return file != NULL && fclose(file) == 0;
}

static SDL_EnumerationResult SDLCALL NativeTrackLibrary_DeleteEntry(void *userdata, const char *directory, const char *name)
{
	char path[NATIVE_TRACK_PATH_MAX];
	SDL_PathInfo info;
	/* Installed tracks contain files only. Refuse subdirectories rather than
	   traversing an unexpected directory or link. */
	if (!NativeTrackLibrary_Join(path, sizeof(path), directory, name) ||
	    !SDL_GetPathInfo(path, &info) || info.type != SDL_PATHTYPE_FILE ||
	    (userdata != NULL && !SDL_RemovePath(path)))
	{
		return SDL_ENUM_FAILURE;
	}
	return SDL_ENUM_CONTINUE;
}

int NativeTrackLibrary_Delete(const struct NativeTrackMetadata *track, const char *directory)
{
	char path[NATIVE_TRACK_PATH_MAX], root[NATIVE_TRACK_PATH_MAX];
	if (track == NULL || directory == NULL || !NativeTrackLibrary_TrackPath(track, "", path, sizeof(path))) { return 0; }
	/* SDL enumeration supplies a trailing separator, sometimes with a different
	   slash on Windows. Compare normalized paths, including older cached scans. */
	size_t out = 0;
	for (size_t i = 0; directory[i] != '\0' && out + 1 < sizeof(root); i++)
	{
		char c = directory[i] == '\\' ? '/' : directory[i];
		if (c != '/' || out == 0 || root[out - 1] != '/') { root[out++] = c; }
	}
	root[out] = '\0';
	while (out > 1 && root[out - 1] == '/') { root[--out] = '\0'; }
	size_t written = 0;
	for (size_t i = 0; path[i] != '\0'; i++)
	{
		char c = path[i] == '\\' ? '/' : path[i];
		if (c != '/' || written == 0 || path[written - 1] != '/') { path[written++] = c; }
	}
	path[written] = '\0';
	size_t rootLength = strlen(root);
	/* Only remove an immediate child of the supplied library directory. */
	if (rootLength == 0 || rootLength >= strlen(path) || strncmp(path, root, rootLength) != 0 || path[rootLength] != '/' ||
	    strchr(path + rootLength + 1, '/') != path + strlen(path) - 1 ||
	    strchr(path + rootLength + 1, '\\') != NULL || strcmp(path + rootLength + 1, "../") == 0 ||
	    strcmp(path + rootLength + 1, "./") == 0) { return 0; }
	path[strlen(path) - 1] = '\0';
	int removed = SDL_EnumerateDirectory(path, NativeTrackLibrary_DeleteEntry, NULL) &&
	              SDL_EnumerateDirectory(path, NativeTrackLibrary_DeleteEntry, path) && SDL_RemovePath(path);
	int refreshed = NativeTrackLibrary_Refresh(directory, NULL);
	return removed && refreshed;
}

void NativeTrackLibrary_Shutdown(void)
{
	NativeTrackRegistry_Destroy(s_nativeTrackLibrary);
	s_nativeTrackLibrary = NULL;
}
