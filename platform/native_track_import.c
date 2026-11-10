#include <platform/native_track_import.h>
#include <platform/native_track_library.h>
#include <platform/native_track_file.h>
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>

enum { TRACK_IMPORT_PATH_MAX = 1024, TRACK_IMPORT_NAME_MAX = 200 };

static int TrackImport_Path(char *out, size_t capacity, const char *dir, const char *name)
{
	int length = SDL_snprintf(out, capacity, "%s/%s", dir, name);
	return length >= 0 && (size_t)length < capacity;
}

static int TrackImport_ValidateFile(const struct NativeTrackRegistry *registry, enum NativeTrackFileRole role)
{
	struct NativeTrackFile *file = NativeTrackFile_Open(registry, NATIVE_TRACK_CUSTOM_ID_BASE, role);
	if (file == NULL)
	{
		return 0;
	}
	size_t size = NativeTrackFile_SectorBytes(file);
	void *buffer = malloc(size);
	int valid = buffer != NULL && NativeTrackFile_Read(file, buffer, size);
	free(buffer);
	NativeTrackFile_Close(file);
	return valid;
}

static enum ExpandedLngString TrackImport_Install(const char *library, const char *name, const char *sources[5])
{
	static const char *files[] = {"track.lev", "track.vrm", "track.sca", "preview.jpg", "track.ini"};
	char target[TRACK_IMPORT_PATH_MAX], staging[TRACK_IMPORT_PATH_MAX];
	char to[TRACK_IMPORT_PATH_MAX];
	SDL_PathInfo info;
	if (library == NULL || name == NULL || name[0] == '\0' ||
	    strlen(name) > TRACK_IMPORT_NAME_MAX || strcmp(name, ".") == 0 || strcmp(name, "..") == 0 ||
	    strpbrk(name, "/\\:<>\"|?*") != NULL || name[0] == '.')
	{
		return LNG_EXPANDED_IMPORT_INVALID_DESTINATION;
	}
	if (!TrackImport_Path(target, sizeof(target), library, name) ||
	    SDL_snprintf(staging, sizeof(staging), "%s/.import-%llu", library,
	                 (unsigned long long)SDL_GetTicksNS()) >= (int)sizeof(staging))
	{
		return LNG_EXPANDED_IMPORT_PATH_TOO_LONG;
	}
	if (!SDL_CreateDirectory(library))
	{
		return LNG_EXPANDED_IMPORT_CREATE_LIBRARY_FAILED;
	}
	if (SDL_GetPathInfo(target, &info))
	{
		return LNG_EXPANDED_IMPORT_DESTINATION_EXISTS;
	}
	if (SDL_GetPathInfo(staging, &info) || !SDL_CreateDirectory(staging))
	{
		return LNG_EXPANDED_IMPORT_CREATE_TEMP_FAILED;
	}
	enum ExpandedLngString error = LNG_TRACK_RESULT_OK;
	for (size_t i = 0; i < SDL_arraysize(files); i++)
	{
		if (!TrackImport_Path(to, sizeof(to), staging, files[i]))
		{
			error = LNG_EXPANDED_IMPORT_PATH_TOO_LONG;
			break;
		}
		if (sources[i] == NULL || sources[i][0] == '\0')
		{
			if (i < 2)
			{
				error = LNG_EXPANDED_IMPORT_MISSING_FILES;
				break;
			}
			continue;
		}
		if (!SDL_GetPathInfo(sources[i], &info) || info.type != SDL_PATHTYPE_FILE || !SDL_CopyFile(sources[i], to))
		{
			error = LNG_EXPANDED_IMPORT_COPY_FAILED;
			break;
		}
	}
	struct NativeTrackRegistry *candidate = NativeTrackRegistry_Create();
	if (error == LNG_TRACK_RESULT_OK && (candidate == NULL || !NativeTrackLibrary_ReadDirectory(candidate, staging, name)))
	{
		error = LNG_EXPANDED_IMPORT_INVALID_METADATA_FILES;
	}
	if (error == LNG_TRACK_RESULT_OK && (!TrackImport_ValidateFile(candidate, NATIVE_TRACK_FILE_LEV) ||
	                      !TrackImport_ValidateFile(candidate, NATIVE_TRACK_FILE_VRM)))
	{
		error = LNG_EXPANDED_IMPORT_INVALID_FILES;
	}
	if (error == LNG_TRACK_RESULT_OK)
	{
		struct NativeTrackRegistry *installed = NativeTrackLibrary_Scan(library, NULL);
		const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(candidate, NATIVE_TRACK_CUSTOM_ID_BASE);
		if (installed == NULL)
		{
			error = LNG_EXPANDED_IMPORT_SCAN_FAILED;
		}
		else if (NativeTrackRegistry_Find(installed, track->identity, NULL))
		{
			error = LNG_EXPANDED_IMPORT_DUPLICATE;
		}
		NativeTrackRegistry_Destroy(installed);
	}
	NativeTrackRegistry_Destroy(candidate);
	if (error == LNG_TRACK_RESULT_OK && (SDL_GetPathInfo(target, &info) || !SDL_RenamePath(staging, target)))
	{
		error = LNG_EXPANDED_IMPORT_PUBLISH_FAILED;
	}
	if (error != LNG_TRACK_RESULT_OK)
	{
		for (size_t i = 0; i < SDL_arraysize(files); i++)
		{
			if (TrackImport_Path(to, sizeof(to), staging, files[i]))
			{
				SDL_RemovePath(to);
			}
		}
		SDL_RemovePath(staging);
	}
	return error;
}

struct TrackImportDirectory
{
	char paths[5][TRACK_IMPORT_PATH_MAX];
	int ambiguous;
	int tooLong;
};

static SDL_EnumerationResult SDLCALL TrackImport_Visit(void *userdata, const char *directory, const char *name)
{
	struct TrackImportDirectory *scan = userdata;
	const char *ext = strrchr(name, '.');
	int role = -1;
	if (ext != NULL && SDL_strcasecmp(ext, ".lev") == 0) { role = 0; }
	else if (ext != NULL && SDL_strcasecmp(ext, ".vrm") == 0) { role = 1; }
	else if (ext != NULL && SDL_strcasecmp(ext, ".sca") == 0) { role = 2; }
	else if (ext != NULL && (SDL_strcasecmp(ext, ".jpg") == 0 || SDL_strcasecmp(ext, ".jpeg") == 0)) { role = 3; }
	else if (SDL_strcasecmp(name, "track.ini") == 0) { role = 4; }
	if (role < 0)
	{
		return SDL_ENUM_CONTINUE;
	}
	char path[TRACK_IMPORT_PATH_MAX];
	SDL_PathInfo info;
	if (!TrackImport_Path(path, sizeof(path), directory, name))
	{
		scan->tooLong = 1;
		return SDL_ENUM_FAILURE;
	}
	if (!SDL_GetPathInfo(path, &info) || info.type != SDL_PATHTYPE_FILE)
	{
		return SDL_ENUM_CONTINUE;
	}
	if (scan->paths[role][0] != '\0')
	{
		scan->ambiguous = 1;
		return SDL_ENUM_FAILURE;
	}
	strcpy(scan->paths[role], path);
	return SDL_ENUM_CONTINUE;
}

enum ExpandedLngString NativeTrackImport_Directory(const char *library, const char *source, const char *name)
{
	if (source == NULL)
	{
		return LNG_EXPANDED_IMPORT_INVALID_SOURCE;
	}
	struct TrackImportDirectory scan = {0};
	if (!SDL_EnumerateDirectory(source, TrackImport_Visit, &scan))
	{
		return scan.ambiguous ? LNG_EXPANDED_IMPORT_AMBIGUOUS_FILES : scan.tooLong ? LNG_EXPANDED_IMPORT_PATH_TOO_LONG : LNG_EXPANDED_IMPORT_READ_FOLDER_FAILED;
	}
	const char *sources[5];
	for (size_t i = 0; i < SDL_arraysize(sources); i++)
	{
		sources[i] = scan.paths[i][0] != '\0' ? scan.paths[i] : NULL;
	}
	return TrackImport_Install(library, name, sources);
}
