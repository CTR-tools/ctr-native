#include <platform/native_track_patch.h>
#include <platform/native_track_import.h>
#include <platform/native_track_library.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#include "native_track_patch_disc.c"
#include "native_track_patch_audio.c"

enum { PATCH_PATH_BYTES = 1024, PATCH_NAME_BYTES = 200, PATCH_TRACK_COUNT = 18, PATCH_TRACK_LIMIT = 64,
       PATCH_TRACK_GROUPS = 25, PATCH_FILES_PER_GROUP = 8 };
enum { PATCH_AUTHOR_BYTES = 256, PATCH_AUTHOR_FILE_LIMIT = 16 * 1024 * 1024 };
enum PatchStage { PATCH_COPY, PATCH_APPLY, PATCH_EXTRACT, PATCH_COMPARE, PATCH_INSTALL, PATCH_CLEANUP };

struct TrackPatchMusicChoice { char track[256], source[256]; };

struct TrackPatchJob
{
	SDL_Thread *thread;
	SDL_AtomicInt done, cancel, stage;
	int succeeded;
	int review, reviewReady;
	int musicGenerated, musicSkipped;
	struct NativeTrackRegistry *replacement;
	char assets[PATCH_PATH_BYTES], patch[PATCH_PATH_BYTES], name[PATCH_NAME_BYTES];
	char work[PATCH_PATH_BYTES];
	char result[256];
	struct TrackPatchMusicChoice musicChoices[PATCH_TRACK_LIMIT];
	int musicChoiceCount;
	char excluded[PATCH_TRACK_LIMIT][256];
	int excludedCount;
};

static struct TrackPatchJob s_trackPatchJob;

static int TrackPatch_Included(const struct TrackPatchJob *job, const char *identity)
{
	for (int i = 0; identity != NULL && i < job->excludedCount; i++)
	{
		if (strcmp(job->excluded[i], identity) == 0) { return 0; }
	}
	return 1;
}

static const char *TrackPatch_MusicChoice(const struct TrackPatchJob *job, const char *identity)
{
	for (int i = 0; identity != NULL && i < job->musicChoiceCount; i++)
	{
		if (strcmp(job->musicChoices[i].track, identity) == 0) { return job->musicChoices[i].source; }
	}
	return NULL;
}

static int TrackPatch_Path(char *out, const char *directory, const char *name)
{
	int length = SDL_snprintf(out, PATCH_PATH_BYTES, "%s/%s", directory, name);
	return length >= 0 && length < PATCH_PATH_BYTES;
}

static SDL_EnumerationResult SDLCALL TrackPatch_RemoveEntry(void *userdata, const char *directory, const char *name);

static int TrackPatch_RemoveTree(const char *path)
{
	SDL_PathInfo info;
	if (!SDL_GetPathInfo(path, &info))
	{
		return 1;
	}
	if (info.type == SDL_PATHTYPE_DIRECTORY && !SDL_EnumerateDirectory(path, TrackPatch_RemoveEntry, NULL))
	{
		return 0;
	}
	return SDL_RemovePath(path);
}

static SDL_EnumerationResult SDLCALL TrackPatch_RemoveEntry(void *userdata, const char *directory, const char *name)
{
	(void)userdata;
	char path[PATCH_PATH_BYTES];
	return TrackPatch_Path(path, directory, name) && TrackPatch_RemoveTree(path) ? SDL_ENUM_CONTINUE : SDL_ENUM_FAILURE;
}

/* SDL passes an argv vector directly, including paths containing spaces. Never
 * invoke a command shell for user-selected patch paths. */
static int TrackPatch_Process(struct TrackPatchJob *job, const char *const *args)
{
	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, (void *)args);
	SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDIN_NUMBER, SDL_PROCESS_STDIO_NULL);
	SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_NULL);
	SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDERR_NUMBER, SDL_PROCESS_STDIO_NULL);
	SDL_Process *process = SDL_CreateProcessWithProperties(props);
	SDL_DestroyProperties(props);
	if (process == NULL)
	{
		return 0;
	}
	int code = -1;
	while (!SDL_WaitProcess(process, false, &code))
	{
		if (SDL_GetAtomicInt(&job->cancel))
		{
			SDL_KillProcess(process, true);
			SDL_WaitProcess(process, true, &code);
			break;
		}
		SDL_Delay(20);
	}
	SDL_DestroyProcess(process);
	return code == 0 && !SDL_GetAtomicInt(&job->cancel);
}

static int TrackPatch_Copy(struct TrackPatchJob *job, const char *source, const char *target)
{
	FILE *in = fopen(source, "rb");
	if (in == NULL)
	{
		return 0;
	}
	FILE *out = fopen(target, "wb");
	unsigned char buffer[64 * 1024];
	int ok = out != NULL;
	while (ok)
	{
		size_t bytes = fread(buffer, 1, sizeof(buffer), in);
		if (SDL_GetAtomicInt(&job->cancel) || ferror(in) || fwrite(buffer, 1, bytes, out) != bytes)
		{
			ok = 0;
			break;
		}
		if (bytes != sizeof(buffer))
		{
			break;
		}
	}
	if (out != NULL && fclose(out) != 0)
	{
		ok = 0;
	}
	fclose(in);
	return ok;
}

static const char *TrackPatch_Error(struct TrackPatchJob *job, const char *error)
{
	return SDL_GetAtomicInt(&job->cancel) ? "Import cancelled" : error;
}

/* Return -1 for unreadable files rather than misidentifying them as changes. */
static int TrackPatch_Different(const char *a, const char *b)
{
	FILE *left = fopen(a, "rb"), *right = fopen(b, "rb");
	int result = -1;
	if (left != NULL && right != NULL)
	{
		unsigned char x[8192], y[8192];
		do
		{
			size_t nx = fread(x, 1, sizeof(x), left), ny = fread(y, 1, sizeof(y), right);
			if (ferror(left) || ferror(right))
			{
				break;
			}
			if (nx != ny || memcmp(x, y, nx) != 0)
			{
				result = 1;
				break;
			}
			if (nx < sizeof(x))
			{
				result = 0;
				break;
			}
		} while (1);
	}
	if (left != NULL) { fclose(left); }
	if (right != NULL) { fclose(right); }
	return result;
}

/* CTR-tools r14's NTSC-U BIGFILE names, in retail level order. Only the 1P
 * circuit pair belongs in the current Time Trial library, not relic, split
 * screen, arenas, hubs, overlays or modified executable code. */
static const char *const s_patchTrackSlots[PATCH_TRACK_COUNT] = {
	"proto8", "proto9", "desert2", "island1", "temple1", "temple2",
	"tube1", "blimp1", "sewer1", "cave1", "castle1", "labs1",
	"ice1", "space", "coco1", "arena2", "secret1", "secret2"
};

/* Only merge exact LEV/VRM pairs within this patch; an existing library entry
 * may have its own metadata, music, records or intended identity. */
static int TrackPatch_DuplicatePair(const char *patched, int slot, const int *accepted, int acceptedCount)
{
	char currentLev[PATCH_PATH_BYTES], currentVrm[PATCH_PATH_BYTES], relative[128];
	SDL_snprintf(relative, sizeof(relative), "%s/1P/data.lev", s_patchTrackSlots[slot]);
	if (!TrackPatch_Path(currentLev, patched, relative)) { return -1; }
	SDL_snprintf(relative, sizeof(relative), "%s/1P/data.vrm", s_patchTrackSlots[slot]);
	if (!TrackPatch_Path(currentVrm, patched, relative)) { return -1; }
	for (int i = 0; i < acceptedCount; i++)
	{
		char otherLev[PATCH_PATH_BYTES], otherVrm[PATCH_PATH_BYTES];
		SDL_snprintf(relative, sizeof(relative), "%s/1P/data.lev", s_patchTrackSlots[accepted[i]]);
		if (!TrackPatch_Path(otherLev, patched, relative)) { return -1; }
		SDL_snprintf(relative, sizeof(relative), "%s/1P/data.vrm", s_patchTrackSlots[accepted[i]]);
		if (!TrackPatch_Path(otherVrm, patched, relative)) { return -1; }
		int lev = TrackPatch_Different(currentLev, otherLev);
		int vrm = TrackPatch_Different(currentVrm, otherVrm);
		if (lev < 0 || vrm < 0) { return -1; }
		if (lev == 0 && vrm == 0) { return 1; }
	}
	return 0;
}

/* Menu credits are free-form strings, not structured track metadata. Only
 * accept a complete NUL-terminated credit and refuse conflicting names. */
static int TrackPatch_FindAuthor(const unsigned char *data, size_t size, char author[PATCH_AUTHOR_BYTES])
{
	static const char prefix[] = "created by";
	if (data == NULL) { return 1; }
	for (size_t at = 0; at + sizeof(prefix) < size; at++)
	{
		if ((at != 0 && data[at - 1] != 0) || SDL_strncasecmp((const char *)data + at, prefix, sizeof(prefix) - 1) != 0)
		{
			continue;
		}
		size_t start = at + sizeof(prefix) - 1;
		if (data[start] == ':') { start++; }
		if (start >= size || data[start] != ' ') { continue; }
		while (start < size && data[start] == ' ') { start++; }
		size_t end = start;
		while (end < size && data[end] >= 32 && data[end] <= 126) { end++; }
		if (end >= size || data[end] != 0) { continue; }
		while (end > start && data[end - 1] == ' ') { end--; }
		size_t length = end - start;
		if (length == 0 || length >= PATCH_AUTHOR_BYTES) { continue; }
		char value[PATCH_AUTHOR_BYTES];
		memcpy(value, data + start, length);
		value[length] = '\0';
		if (author[0] != '\0' && SDL_strcasecmp(author, value) != 0) { return 0; }
		SDL_strlcpy(author, value, PATCH_AUTHOR_BYTES);
	}
	return 1;
}

static int TrackPatch_ReadAuthorFile(const char *path, char author[PATCH_AUTHOR_BYTES])
{
	SDL_PathInfo info;
	if (!SDL_GetPathInfo(path, &info) || info.type != SDL_PATHTYPE_FILE || info.size > PATCH_AUTHOR_FILE_LIMIT) { return 0; }
	size_t size = 0;
	unsigned char *bytes = SDL_LoadFile(path, &size);
	if (bytes == NULL) { return 0; }
	int ok = TrackPatch_FindAuthor(bytes, size, author);
	SDL_free(bytes);
	return ok;
}

static const char *TrackPatch_WriteAuthor(const char *prepared, const char *name, const struct PatchMusic *music)
{
	char author[PATCH_AUTHOR_BYTES] = {0};
	char directory[PATCH_PATH_BYTES], path[PATCH_PATH_BYTES];
	if (!TrackPatch_Path(directory, prepared, name) || !TrackPatch_Path(path, directory, "track.lev")) { return "Path is too long"; }
	if (!TrackPatch_FindAuthor(music->menu.data, music->menu.size, author) ||
	    !TrackPatch_FindAuthor(music->exe.data, music->exe.size, author) || !TrackPatch_ReadAuthorFile(path, author) || author[0] == '\0')
	{
		return NULL;
	}
	struct NativeTrackRegistry *registry = NativeTrackRegistry_Create();
	if (registry == NULL || !NativeTrackLibrary_ReadDirectory(registry, directory, name))
	{
		NativeTrackRegistry_Destroy(registry);
		return "Cannot read track metadata";
	}
	const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE);
	int ok = track->author != NULL || NativeTrackLibrary_SetDetails(track, track->title, author);
	NativeTrackRegistry_Destroy(registry);
	return ok ? NULL : "Cannot write track metadata";
}

static const char *TrackPatch_WriteTitle(const char *prepared, const char *name, const char *title)
{
	char directory[PATCH_PATH_BYTES], manifest[PATCH_PATH_BYTES];
	if (!TrackPatch_Path(directory, prepared, name) || !TrackPatch_Path(manifest, directory, "track.ini")) { return "Path is too long"; }
	SDL_PathInfo existing;
	if (SDL_GetPathInfo(manifest, &existing)) { return NULL; }
	FILE *file = fopen(manifest, "wb");
	if (file == NULL) { return "Cannot write track metadata"; }
	int ok = fprintf(file, "title = %s\n", title) > 0;
	if (fclose(file) != 0) { ok = 0; }
	return ok ? NULL : "Cannot write track metadata";
}

static int TrackPatch_WriteRetailMusic(const char *directory, const char *name, int level)
{
	struct NativeTrackRegistry *registry = NativeTrackRegistry_Create();
	int ok = registry != NULL && NativeTrackLibrary_ReadDirectory(registry, directory, name);
	if (ok)
	{
		const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(registry, NATIVE_TRACK_CUSTOM_ID_BASE);
		ok = NativeTrackLibrary_SetMusic(track, level, 0);
	}
	NativeTrackRegistry_Destroy(registry);
	return ok;
}

/* Generic strategy: compare bigtool's 18 named 1P pairs against retail. This
 * also handles single-track patches without knowing their custom loader. */
static const char *TrackPatch_Collect(struct TrackPatchJob *job, const char *work, int *count, struct PatchMusic *music)
{
	char original[PATCH_PATH_BYTES], patched[PATCH_PATH_BYTES], prepared[PATCH_PATH_BYTES];
	if (!TrackPatch_Path(original, work, "original/levels/tracks") ||
	    !TrackPatch_Path(patched, work, "patched/levels/tracks") ||
	    !TrackPatch_Path(prepared, work, "prepared"))
	{
		return "Path is too long";
	}
	*count = 0;
	unsigned char adventureVrm[27], adventureLev[27];
	int adventureLoader = music != NULL && PatchMusic_AdventureFiles(&music->exe, adventureVrm, adventureLev);
	int accepted[PATCH_TRACK_COUNT];
	char singleName[PATCH_NAME_BYTES] = {0};
	for (int i = 0; i < PATCH_TRACK_COUNT; i++)
	{
		if (SDL_GetAtomicInt(&job->cancel)) { return "Import cancelled"; }
		char relative[128], a[PATCH_PATH_BYTES], b[PATCH_PATH_BYTES];
		SDL_snprintf(relative, sizeof(relative), "%s/1P/data.lev", s_patchTrackSlots[i]);
		if (!TrackPatch_Path(a, original, relative) || !TrackPatch_Path(b, patched, relative))
		{
			return "Path is too long";
		}
		int changedLev = TrackPatch_Different(a, b);
		SDL_snprintf(relative, sizeof(relative), "%s/1P/data.vrm", s_patchTrackSlots[i]);
		if (!TrackPatch_Path(a, original, relative) || !TrackPatch_Path(b, patched, relative))
		{
			return "Path is too long";
		}
		int changedVrm = TrackPatch_Different(a, b);
		if (changedLev < 0 || changedVrm < 0)
		{
			/* A recognized adventure may not have a bigtool-compatible layout;
			 * its explicitly referenced pairs are checked separately below. */
			if (adventureLoader) { continue; }
			return "Missing circuit files - unsupported BIGFILE layout";
		}
		if (!changedLev && !changedVrm) { continue; }
		int duplicate = TrackPatch_DuplicatePair(patched, i, accepted, *count);
		if (duplicate < 0) { return "Cannot compare circuit files"; }
		if (duplicate) { continue; }
		char source[PATCH_PATH_BYTES], name[PATCH_NAME_BYTES];
		SDL_snprintf(relative, sizeof(relative), "%s/1P", s_patchTrackSlots[i]);
		if (!TrackPatch_Path(source, patched, relative)) { return "Path is too long"; }
		SDL_snprintf(name, sizeof(name), "%s-%s", job->name, s_patchTrackSlots[i]);
		SDL_strlcpy(singleName, name, sizeof(singleName));
		const char *error = NativeTrackImport_Directory(prepared, source, name);
		if (error != NULL)
		{
			if (adventureLoader && (strcmp(error, "Invalid LEV or VRM") == 0 ||
			                        strcmp(error, "Invalid track metadata or files") == 0)) { continue; }
			return error;
		}
		if (music != NULL)
		{
			char directory[PATCH_PATH_BYTES], sca[PATCH_PATH_BYTES];
			if (!TrackPatch_Path(directory, prepared, name) || !TrackPatch_Path(sca, directory, "track.sca"))
			{
				return "Path is too long";
			}
			char title[256];
			if (PatchMusic_AllBinTrack(&music->exe, &music->all, i, title, sizeof(title)) >= 0 && title[0] != '\0')
			{
				const char *titleError = TrackPatch_WriteTitle(prepared, name, title);
				if (titleError != NULL) { return titleError; }
			}
			int level = PatchMusic_SelectLevel(music, i);
			int retail = level >= 0 ? PatchMusic_RetailMatch(music, level) : -1;
			if (retail >= 0)
			{
				if (!TrackPatch_WriteRetailMusic(directory, name, retail)) { return "Cannot save retail music"; }
			}
			else if (level >= 0 && PatchMusic_WriteSca(music, level, sca)) { music->generated++; }
			else { music->skipped++; }
		}
		accepted[(*count)++] = i;
	}
	if (*count == 0)
	{
		if (!adventureLoader)
		{
			return "No modified single-player circuits found";
		}
	}
	/* A global menu credit cannot be attributed to one of several circuits. */
	return *count == 1 && music != NULL ? TrackPatch_WriteAuthor(prepared, singleName, music) : NULL;
}

/* Extract a BIGFILE entry independently of bigtool's LOD directory names. */
static int TrackPatch_BigEntry(const char *big, unsigned int index, const char *output)
{
	FILE *in = fopen(big, "rb");
	if (in == NULL) { return 0; }
	unsigned char header[8], entry[8], buffer[8192];
	int ok = fread(header, 1, sizeof(header), in) == sizeof(header) && index < PatchDisc_Read32(header + 4) &&
	         index < 4096 && fseek(in, 8 + (long)index * 8, SEEK_SET) == 0 && fread(entry, 1, sizeof(entry), in) == sizeof(entry);
	uint32_t sector = ok ? PatchDisc_Read32(entry) : 0, size = ok ? PatchDisc_Read32(entry + 4) : 0;
	uint64_t start = (uint64_t)sector * PATCH_DISC_DATA_BYTES;
	if (size == 0 || size > 64 * 1024 * 1024 || start > INT32_MAX ||
	    fseek(in, 0, SEEK_END) != 0) { ok = 0; }
	long length = ok ? ftell(in) : -1;
	if (length < 0 || start + size > (uint64_t)length || fseek(in, (long)start, SEEK_SET) != 0) { ok = 0; }
	FILE *out = ok ? fopen(output, "wb") : NULL;
	if (out == NULL) { ok = 0; }
	for (uint32_t left = size; ok && left != 0;)
	{
		size_t n = left < sizeof(buffer) ? left : sizeof(buffer);
		ok = fread(buffer, 1, n, in) == n && fwrite(buffer, 1, n, out) == n;
		left -= (uint32_t)n;
	}
	if (out != NULL && fclose(out) != 0) { ok = 0; }
	fclose(in);
	return ok;
}

struct TrackPatchPairCheck
{
	const char *lev, *vrm;
	int duplicate, failed;
};

static SDL_EnumerationResult SDLCALL TrackPatch_CheckPreparedPair(void *userdata, const char *directory, const char *name)
{
	struct TrackPatchPairCheck *check = userdata;
	char dir[PATCH_PATH_BYTES], lev[PATCH_PATH_BYTES], vrm[PATCH_PATH_BYTES];
	if (!TrackPatch_Path(dir, directory, name) || !TrackPatch_Path(lev, dir, "track.lev") ||
	    !TrackPatch_Path(vrm, dir, "track.vrm")) { check->failed = 1; return SDL_ENUM_FAILURE; }
	int a = TrackPatch_Different(check->lev, lev), b = TrackPatch_Different(check->vrm, vrm);
	if (a < 0 || b < 0) { check->failed = 1; return SDL_ENUM_FAILURE; }
	if (a == 0 && b == 0) { check->duplicate = 1; }
	return SDL_ENUM_CONTINUE;
}

/* Supplemental strategy: a recognized adventure loader names BIGFILE entries
 * directly. Different LODs of one retail slot can contain different tracks.
 * Unknown loaders return without adding anything, not a guessed LOD mapping. */
static const char *TrackPatch_CollectAdventure(struct TrackPatchJob *job, const char *work, int *count,
                                               struct PatchMusic *music)
{
	unsigned char vrm[27], lev[27];
	if (music == NULL || !PatchMusic_AdventureFiles(&music->exe, vrm, lev)) { return NULL; }
	char original[PATCH_PATH_BYTES], patched[PATCH_PATH_BYTES], prepared[PATCH_PATH_BYTES];
	char oldDir[PATCH_PATH_BYTES], newDir[PATCH_PATH_BYTES], oldLev[PATCH_PATH_BYTES], oldVrm[PATCH_PATH_BYTES];
	char newLev[PATCH_PATH_BYTES], newVrm[PATCH_PATH_BYTES];
	if (!TrackPatch_Path(original, work, "original.big") || !TrackPatch_Path(patched, work, "patched.big") ||
	    !TrackPatch_Path(prepared, work, "prepared") || !TrackPatch_Path(oldDir, work, "lod-original") ||
	    !TrackPatch_Path(newDir, work, "lod-patched") || !TrackPatch_Path(oldLev, oldDir, "data.lev") ||
	    !TrackPatch_Path(oldVrm, oldDir, "data.vrm") || !TrackPatch_Path(newLev, newDir, "data.lev") ||
	    !TrackPatch_Path(newVrm, newDir, "data.vrm") || !SDL_CreateDirectory(prepared) ||
	    !SDL_CreateDirectory(oldDir) || !SDL_CreateDirectory(newDir))
	{
		return "Cannot prepare additional track files";
	}
	for (int i = 0; i < 27; i++)
	{
		if (SDL_GetAtomicInt(&job->cancel)) { return "Import cancelled"; }
		unsigned int id = vrm[i];
		/* Adventure loaders also store tracks in the seven arena groups. Other
		 * BIGFILE groups are not necessarily LEV/VRM track pairs. */
		if (id >= PATCH_TRACK_GROUPS * PATCH_FILES_PER_GROUP) { continue; }
		int repeated = 0;
		for (int j = 0; j < i; j++) { if (vrm[j] == id && lev[j] == lev[i]) { repeated = 1; } }
		if (repeated) { continue; }
		if (!TrackPatch_BigEntry(original, id, oldVrm) || !TrackPatch_BigEntry(original, lev[i], oldLev) ||
		    !TrackPatch_BigEntry(patched, id, newVrm) || !TrackPatch_BigEntry(patched, lev[i], newLev))
		{
			return "Cannot extract additional track files";
		}
		int changedLev = TrackPatch_Different(oldLev, newLev), changedVrm = TrackPatch_Different(oldVrm, newVrm);
		if (changedLev < 0 || changedVrm < 0) { return "Cannot compare additional track files"; }
		if (!changedLev && !changedVrm) { continue; }
		/* The generic 1P scan or another loader entry may already have installed
		 * this exact pair in prepared; only merge duplicates within this patch. */
		struct TrackPatchPairCheck check = {newLev, newVrm, 0, 0};
		if (!SDL_EnumerateDirectory(prepared, TrackPatch_CheckPreparedPair, &check) || check.failed)
		{
			return "Cannot compare additional track files";
		}
		if (check.duplicate) { continue; }
		char name[PATCH_NAME_BYTES];
		unsigned int group = id / PATCH_FILES_PER_GROUP;
		if (group < PATCH_TRACK_COUNT)
		{
			SDL_snprintf(name, sizeof(name), "%s-%s-lod%u", job->name, s_patchTrackSlots[group], id & 7);
		}
		else
		{
			SDL_snprintf(name, sizeof(name), "%s-slot%u-lod%u", job->name, group, id & 7);
		}
		const char *error = NativeTrackImport_Directory(prepared, newDir, name);
		if (error != NULL)
		{
			if (strcmp(error, "Invalid LEV or VRM") == 0 || strcmp(error, "Invalid track metadata or files") == 0) { continue; }
			return error;
		}
		(*count)++;
		/* The file-index signature does not identify the runtime music level. */
		music->skipped++;
		char title[256];
		if (PatchMusic_AdventureTitle(&music->exe, &music->adventure, i, title, sizeof(title)))
		{
			const char *titleError = TrackPatch_WriteTitle(prepared, name, title);
			if (titleError != NULL) { return titleError; }
		}
	}
	return NULL;
}

struct TrackPatchPublish
{
	struct TrackPatchJob *job;
	struct NativeTrackRegistry *prepared;
	const char *library;
	const char *error;
	char installed[PATCH_TRACK_LIMIT][PATCH_PATH_BYTES];
	int count;
};

static SDL_EnumerationResult SDLCALL TrackPatch_Publish(void *userdata, const char *directory, const char *name)
{
	struct TrackPatchPublish *publish = userdata;
	char source[PATCH_PATH_BYTES], target[PATCH_PATH_BYTES];
	if (SDL_GetAtomicInt(&publish->job->cancel))
	{
		publish->error = "Import cancelled";
	}
	else if (publish->count == PATCH_TRACK_LIMIT || !TrackPatch_Path(source, directory, name) ||
	         !TrackPatch_Path(target, publish->library, name))
	{
		publish->error = "Path is too long";
	}
	else
	{
		char lev[PATCH_PATH_BYTES];
		const struct NativeTrackMetadata *preparedTrack = NULL;
		if (TrackPatch_Path(lev, source, "track.lev"))
		{
			for (size_t i = 0; i < NativeTrackRegistry_Count(publish->prepared); i++)
			{
				const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(publish->prepared, NATIVE_TRACK_CUSTOM_ID_BASE + (int)i);
				if (track != NULL && strcmp(track->levPath, lev) == 0) { preparedTrack = track; break; }
			}
		}
		if (preparedTrack == NULL) { publish->error = "Cannot read prepared track"; return SDL_ENUM_FAILURE; }
		if (!TrackPatch_Included(publish->job, preparedTrack->identity)) { return SDL_ENUM_CONTINUE; }
		publish->error = NativeTrackImport_Directory(publish->library, source, name);
		if (publish->error == NULL)
		{
			SDL_strlcpy(publish->installed[publish->count++], target, PATCH_PATH_BYTES);
			struct NativeTrackRegistry *candidate = NativeTrackRegistry_Create();
			if (candidate == NULL || !NativeTrackLibrary_ReadDirectory(candidate, target, name))
			{
				publish->error = "Cannot read imported track";
			}
			else
			{
				const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(candidate, NATIVE_TRACK_CUSTOM_ID_BASE);
				const char *selected = TrackPatch_MusicChoice(publish->job, track->identity);
				if (selected != NULL)
				{
					int level;
					const struct NativeTrackMetadata *music = NativeTrackRegistry_Find(publish->prepared, selected, &level) ?
					    NativeTrackRegistry_Get(publish->prepared, level) : NULL;
					char sca[PATCH_PATH_BYTES];
					if (music == NULL || music->scaPath == NULL || !TrackPatch_Path(sca, target, "track.sca") ||
					    !SDL_CopyFile(music->scaPath, sca)) { publish->error = "Cannot copy selected music"; }
				}
				if (publish->error == NULL && (selected != NULL || (track->music == NULL && track->scaPath != NULL)))
				{
					if (!NativeTrackLibrary_SetMusic(track, -1, 1)) { publish->error = "Cannot save selected music"; }
				}
			}
			NativeTrackRegistry_Destroy(candidate);
		}
	}
	return publish->error == NULL ? SDL_ENUM_CONTINUE : SDL_ENUM_FAILURE;
}

static const char *TrackPatch_InstallPrepared(struct TrackPatchJob *job, const char *prepared, const char *library)
{
	struct TrackPatchPublish publish = {.job = job, .library = library, .prepared = NativeTrackLibrary_Scan(prepared, NULL)};
	if (publish.prepared == NULL) { return "Cannot read prepared tracks"; }
	const char *error = NULL;
	if (!SDL_EnumerateDirectory(prepared, TrackPatch_Publish, &publish))
	{
		error = publish.error != NULL ? publish.error : "Cannot read prepared tracks";
	}
	if (SDL_GetAtomicInt(&job->cancel))
	{
		error = "Import cancelled";
	}
	if (error == NULL && publish.count == 0) { error = "No tracks selected"; }
	if (error != NULL)
	{
		for (int i = 0; i < publish.count; i++)
		{
			if (!TrackPatch_RemoveTree(publish.installed[i]))
			{
				error = "Import failed - cannot remove partial installation";
			}
		}
	}
	NativeTrackRegistry_Destroy(publish.prepared);
	return error;
}

static const char *TrackPatch_Run(struct TrackPatchJob *job, const char *work, int *count)
{
	/* Work only on temporary images: decode, extract both BIGFILEs, discover
	 * and validate candidates in prepared, then publish the whole batch. */
	char base[PATCH_PATH_BYTES], copy[PATCH_PATH_BYTES], patched[PATCH_PATH_BYTES];
	char originalBig[PATCH_PATH_BYTES], patchedBig[PATCH_PATH_BYTES], tool[PATCH_PATH_BYTES];
	char prepared[PATCH_PATH_BYTES], library[PATCH_PATH_BYTES];
	if (!TrackPatch_Path(base, job->assets, "ctr-u.bin") || !TrackPatch_Path(copy, work, "source.bin") ||
	    !TrackPatch_Path(patched, work, "patched.bin") || !TrackPatch_Path(originalBig, work, "original.big") ||
	    !TrackPatch_Path(patchedBig, work, "patched.big") || !TrackPatch_Path(prepared, work, "prepared") ||
	    !TrackPatch_Path(library, job->assets, "tracks"))
	{
		return "Path is too long";
	}
#if defined(_WIN32)
	const char *xdelta = "tools/xdelta3.exe", *bigtool = "tools/bigtool.exe";
#else
	const char *xdelta = "tools/xdelta3", *bigtool = "tools/bigtool";
#endif
	SDL_PathInfo toolInfo;
	if (!TrackPatch_Path(tool, job->assets, bigtool)) { return "Path is too long"; }
	if (!SDL_GetPathInfo(tool, &toolInfo) || toolInfo.type != SDL_PATHTYPE_FILE)
	{
		return "Missing assets/tools/bigtool";
	}
	if (!TrackPatch_Path(tool, job->assets, xdelta)) { return "Path is too long"; }
	if (!SDL_GetPathInfo(tool, &toolInfo) || toolInfo.type != SDL_PATHTYPE_FILE)
	{
		return "Missing assets/tools/xdelta3";
	}
	SDL_SetAtomicInt(&job->stage, PATCH_COPY);
	if (!TrackPatch_Copy(job, base, copy)) { return TrackPatch_Error(job, "Cannot copy assets/ctr-u.bin"); }
	SDL_SetAtomicInt(&job->stage, PATCH_APPLY);
	const char *decode[] = {tool, "-d", "-s", copy, job->patch, patched, NULL};
	if (!TrackPatch_Process(job, decode)) { return TrackPatch_Error(job, "Xdelta failed - check patch and base ROM"); }
	SDL_SetAtomicInt(&job->stage, PATCH_EXTRACT);
	if (!PatchDisc_ExtractBig(copy, originalBig, &job->cancel) ||
	    !PatchDisc_ExtractBig(patched, patchedBig, &job->cancel))
	{
		return TrackPatch_Error(job, "Cannot extract BIGFILE.BIG from the ROM");
	}
	if (!TrackPatch_Path(tool, job->assets, bigtool)) { return "Path is too long"; }
	const char *extractOriginal[] = {tool, originalBig, NULL};
	const char *extractPatched[] = {tool, patchedBig, NULL};
	struct PatchMusic music;
	PatchMusic_Open(&music, copy, patched, &job->cancel);
	unsigned char vrm[27], lev[27];
	int adventureLoader = PatchMusic_AdventureFiles(&music.exe, vrm, lev);
	int extracted = TrackPatch_Process(job, extractOriginal) && TrackPatch_Process(job, extractPatched);
	/* bigtool supplies named 1P folders for the generic strategy. A recognized
	 * adventure can still use direct BIGFILE indices if that extraction fails. */
	if (!extracted && (!adventureLoader || SDL_GetAtomicInt(&job->cancel)))
	{
		PatchMusic_Close(&music);
		return TrackPatch_Error(job, "CTR-tools bigtool failed");
	}
	SDL_SetAtomicInt(&job->stage, PATCH_COMPARE);
	*count = 0;
	const char *error = extracted ? TrackPatch_Collect(job, work, count, &music) : NULL;
	/* Try the loader-specific strategy in addition to the generic scan. Both
	 * prepare validated pairs before publication; failures leave the library untouched. */
	if (error == NULL) { error = TrackPatch_CollectAdventure(job, work, count, &music); }
	if (error == NULL && *count == 0) { error = "No modified single-player circuits found"; }
	job->musicGenerated = music.generated;
	job->musicSkipped = music.skipped;
	PatchMusic_Close(&music);
	if (error != NULL) { return error; }
	if (job->review) { return NULL; }
	SDL_SetAtomicInt(&job->stage, PATCH_INSTALL);
	return TrackPatch_InstallPrepared(job, prepared, library);
}

static int SDLCALL TrackPatch_Worker(void *userdata)
{
	struct TrackPatchJob *job = userdata;
	char work[PATCH_PATH_BYTES];
	int length = SDL_snprintf(work, sizeof(work), "%s/.import-xdelta-%llu", job->assets,
	                          (unsigned long long)SDL_GetTicksNS());
	SDL_PathInfo info;
	const char *error = "Cannot create temporary directory";
	int count = 0;
	if (length >= 0 && length < (int)sizeof(work) && !SDL_GetPathInfo(work, &info) && SDL_CreateDirectory(work))
	{
		error = TrackPatch_Run(job, work, &count);
		if (error == NULL && job->review && !SDL_GetAtomicInt(&job->cancel))
		{
			SDL_strlcpy(job->work, work, sizeof(job->work));
			job->reviewReady = 1;
		}
		else
		{
			if (error == NULL && job->review) { error = "Import cancelled"; }
			SDL_SetAtomicInt(&job->stage, PATCH_CLEANUP);
			int removed = TrackPatch_RemoveTree(work);
			if (!removed) { error = "Import ended - temporary cleanup failed"; }
		}
	}
	job->succeeded = error == NULL;
	if (error != NULL) { SDL_strlcpy(job->result, error, sizeof(job->result)); }
	else if (job->reviewReady)
	{
		SDL_snprintf(job->result, sizeof(job->result), "%d track%s ready for review", count, count == 1 ? "" : "s");
	}
	else if (job->musicSkipped != 0)
	{
		SDL_snprintf(job->result, sizeof(job->result), "%d track%s imported; %d music skipped", count,
		             count == 1 ? "" : "s", job->musicSkipped);
	}
	else { SDL_snprintf(job->result, sizeof(job->result), "%d track%s imported with music", count, count == 1 ? "" : "s"); }
	SDL_SetAtomicInt(&job->done, 1);
	return 0;
}

static int TrackPatch_Start(const char *assets, const char *patch, int review)
{
	struct TrackPatchJob *job = &s_trackPatchJob;
	if (job->thread != NULL || job->reviewReady || assets == NULL || patch == NULL || strlen(assets) >= sizeof(job->assets) ||
	    strlen(patch) >= sizeof(job->patch))
	{
		return 0;
	}
	memset(job, 0, sizeof(*job));
	job->review = review;
	SDL_strlcpy(job->assets, assets, sizeof(job->assets));
	SDL_strlcpy(job->patch, patch, sizeof(job->patch));
	const char *name = patch;
	for (const char *p = patch; *p; p++)
	{
		if (*p == '/' || *p == '\\') { name = p + 1; }
	}
	size_t length = strlen(name);
	if (length <= 7 || SDL_strcasecmp(name + length - 7, ".xdelta") != 0 || length - 7 > sizeof(job->name) - 24)
	{
		return 0;
	}
	memcpy(job->name, name, length - 7);
	/* Keep display-independent directory/identity names valid for local import. */
	for (char *p = job->name; *p; p++)
	{
		if ((unsigned char)*p < 32 || strchr(":<>\"|?*", *p) != NULL || (p == job->name && *p == '.')) { *p = '_'; }
	}
	job->thread = SDL_CreateThread(TrackPatch_Worker, "track-xdelta", job);
	return job->thread != NULL;
}

int NativeTrackPatch_Start(const char *assets, const char *patch)
{
	return TrackPatch_Start(assets, patch, 0);
}

int NativeTrackPatch_StartReview(const char *assets, const char *patch)
{
	return TrackPatch_Start(assets, patch, 1);
}

const char *NativeTrackPatch_PrepareFolder(const char *assets, const char *source)
{
	struct TrackPatchJob *job = &s_trackPatchJob;
	if (job->thread != NULL || job->reviewReady) { return "Another import is active"; }
	if (assets == NULL || source == NULL || strlen(assets) >= sizeof(job->assets) || strlen(source) >= PATCH_PATH_BYTES)
	{
		return "Invalid import path";
	}
	char trimmed[PATCH_PATH_BYTES];
	SDL_strlcpy(trimmed, source, sizeof(trimmed));
	size_t length = strlen(trimmed);
	while (length > 0 && (trimmed[length - 1] == '/' || trimmed[length - 1] == '\\')) { trimmed[--length] = '\0'; }
	const char *name = trimmed + length;
	while (name > trimmed && name[-1] != '/' && name[-1] != '\\') { name--; }
	if (*name == '\0') { return "Invalid source directory"; }
	memset(job, 0, sizeof(*job));
	SDL_strlcpy(job->assets, assets, sizeof(job->assets));
	int n = SDL_snprintf(job->work, sizeof(job->work), "%s/.import-folder-%llu", assets,
	                     (unsigned long long)SDL_GetTicksNS());
	char prepared[PATCH_PATH_BYTES];
	SDL_PathInfo info;
	if (n < 0 || (size_t)n >= sizeof(job->work) || !TrackPatch_Path(prepared, job->work, "prepared") ||
	    SDL_GetPathInfo(job->work, &info) || !SDL_CreateDirectory(job->work))
	{
		job->work[0] = '\0';
		return "Cannot create temporary directory";
	}
	const char *error = NativeTrackImport_Directory(prepared, trimmed, name);
	if (error != NULL)
	{
		if (!TrackPatch_RemoveTree(job->work)) { return "Import failed - temporary cleanup failed"; }
		job->work[0] = '\0';
		return error;
	}
	job->reviewReady = 1;
	return NULL;
}

const char *NativeTrackPatch_ReviewDirectory(void)
{
	static char prepared[PATCH_PATH_BYTES];
	struct TrackPatchJob *job = &s_trackPatchJob;
	return job->thread == NULL && job->reviewReady && TrackPatch_Path(prepared, job->work, "prepared") ? prepared : NULL;
}

const char *NativeTrackPatch_ReviewMusic(const char *identity)
{
	return TrackPatch_MusicChoice(&s_trackPatchJob, identity);
}

int NativeTrackPatch_ReviewIncluded(const char *identity)
{
	return TrackPatch_Included(&s_trackPatchJob, identity);
}

int NativeTrackPatch_ReviewSetIncluded(const char *identity, int included)
{
	struct TrackPatchJob *job = &s_trackPatchJob;
	if (!job->reviewReady || job->thread != NULL || identity == NULL || identity[0] == '\0' || strlen(identity) >= 256) { return 0; }
	int i = 0;
	while (i < job->excludedCount && strcmp(job->excluded[i], identity) != 0) { i++; }
	if (included)
	{
		if (i < job->excludedCount) { memcpy(job->excluded[i], job->excluded[--job->excludedCount], sizeof(job->excluded[i])); }
		return 1;
	}
	if (i == job->excludedCount)
	{
		if (i == PATCH_TRACK_LIMIT) { return 0; }
		SDL_strlcpy(job->excluded[i], identity, sizeof(job->excluded[i]));
		job->excludedCount++;
	}
	return 1;
}

int NativeTrackPatch_ReviewSetMusic(const char *identity, const char *source)
{
	struct TrackPatchJob *job = &s_trackPatchJob;
	if (!job->reviewReady || job->thread != NULL || identity == NULL || identity[0] == '\0' || strlen(identity) >= 256 ||
	    (source != NULL && (source[0] == '\0' || strlen(source) >= 256))) { return 0; }
	int i = 0;
	while (i < job->musicChoiceCount && strcmp(job->musicChoices[i].track, identity) != 0) { i++; }
	if (source == NULL)
	{
		if (i < job->musicChoiceCount) { job->musicChoices[i] = job->musicChoices[--job->musicChoiceCount]; }
		return 1;
	}
	if (i == job->musicChoiceCount)
	{
		if (i == PATCH_TRACK_LIMIT) { return 0; }
		job->musicChoiceCount++;
	}
	SDL_strlcpy(job->musicChoices[i].track, identity, sizeof(job->musicChoices[i].track));
	SDL_strlcpy(job->musicChoices[i].source, source, sizeof(job->musicChoices[i].source));
	return 1;
}

const char *NativeTrackPatch_ReviewUpdate(const char *identity, const char *title, const char *author)
{
	const char *prepared = NativeTrackPatch_ReviewDirectory();
	if (prepared == NULL) { return "No import to review"; }
	if (!NativeTrackLibrary_ValidText(identity, 0) || !NativeTrackLibrary_ValidText(title, 0) ||
	    !NativeTrackLibrary_ValidText(author, 1)) { return "Invalid track metadata"; }
	struct NativeTrackRegistry *registry = NativeTrackLibrary_Scan(prepared, NULL);
	int level;
	if (registry == NULL || !NativeTrackRegistry_Find(registry, identity, &level))
	{
		NativeTrackRegistry_Destroy(registry);
		return "Track not found";
	}
	const struct NativeTrackMetadata *track = NativeTrackRegistry_Get(registry, level);
	int retailMusic = NativeTrackLibrary_Music(track);
	int custom = retailMusic < 0 && track->scaPath != NULL;
	if (retailMusic < 0 && !custom && track->music != NULL) { retailMusic = NATIVE_TRACK_DEFAULT_RETAIL_MUSIC; }
	int ok = NativeTrackLibrary_WriteMetadata(track, title, author, retailMusic, custom);
	NativeTrackRegistry_Destroy(registry);
	return ok ? NULL : "Cannot write track metadata";
}

static const char *TrackPatch_Commit(struct TrackPatchJob *job, int scan)
{
	char prepared[PATCH_PATH_BYTES], library[PATCH_PATH_BYTES];
	if (!TrackPatch_Path(prepared, job->work, "prepared") || !TrackPatch_Path(library, job->assets, "tracks")) { return "Path is too long"; }
	SDL_SetAtomicInt(&job->stage, PATCH_INSTALL);
	const char *error = TrackPatch_InstallPrepared(job, prepared, library);
	if (error != NULL) { return error; }
	job->reviewReady = 0;
	if (scan) { job->replacement = NativeTrackLibrary_Scan(library, NULL); }
	SDL_SetAtomicInt(&job->stage, PATCH_CLEANUP);
	if (!TrackPatch_RemoveTree(job->work)) { return "Imported - temporary cleanup failed"; }
	job->work[0] = '\0';
	if (scan && job->replacement == NULL) { return "Imported - restart game"; }
	return NULL;
}

const char *NativeTrackPatch_ReviewCommit(void)
{
	if (NativeTrackPatch_ReviewDirectory() == NULL) { return "No import to confirm"; }
	return TrackPatch_Commit(&s_trackPatchJob, 0);
}

static int SDLCALL TrackPatch_CommitWorker(void *userdata)
{
	struct TrackPatchJob *job = userdata;
	const char *error = TrackPatch_Commit(job, 1);
	job->succeeded = error == NULL || !job->reviewReady;
	SDL_strlcpy(job->result, error != NULL ? error : "Tracks imported", sizeof(job->result));
	SDL_SetAtomicInt(&job->done, 1);
	return 0;
}

int NativeTrackPatch_ReviewCommitStart(void)
{
	struct TrackPatchJob *job = &s_trackPatchJob;
	if (job->thread != NULL || !job->reviewReady) { return 0; }
	SDL_SetAtomicInt(&job->done, 0);
	SDL_SetAtomicInt(&job->cancel, 0);
	SDL_SetAtomicInt(&job->stage, PATCH_INSTALL);
	job->thread = SDL_CreateThread(TrackPatch_CommitWorker, "track-install", job);
	return job->thread != NULL;
}

struct NativeTrackRegistry *NativeTrackPatch_TakeRegistry(void)
{
	if (s_trackPatchJob.thread != NULL) { return NULL; }
	struct NativeTrackRegistry *registry = s_trackPatchJob.replacement;
	s_trackPatchJob.replacement = NULL;
	return registry;
}

int NativeTrackPatch_ReviewDiscard(void)
{
	struct TrackPatchJob *job = &s_trackPatchJob;
	if (job->thread != NULL || !job->reviewReady) { return 0; }
	if (!TrackPatch_RemoveTree(job->work)) { return 0; }
	job->reviewReady = 0;
	job->work[0] = '\0';
	return 1;
}

static int SDLCALL TrackPatch_DiscardWorker(void *userdata)
{
	struct TrackPatchJob *job = userdata;
	job->succeeded = TrackPatch_RemoveTree(job->work);
	if (job->succeeded)
	{
		job->reviewReady = 0;
		job->work[0] = '\0';
	}
	SDL_strlcpy(job->result, job->succeeded ? "Import cancelled" : "Cannot discard import", sizeof(job->result));
	SDL_SetAtomicInt(&job->done, 1);
	return 0;
}

int NativeTrackPatch_ReviewDiscardStart(void)
{
	struct TrackPatchJob *job = &s_trackPatchJob;
	if (job->thread != NULL || !job->reviewReady) { return 0; }
	SDL_SetAtomicInt(&job->done, 0);
	SDL_SetAtomicInt(&job->cancel, 0);
	SDL_SetAtomicInt(&job->stage, PATCH_CLEANUP);
	job->thread = SDL_CreateThread(TrackPatch_DiscardWorker, "track-discard", job);
	return job->thread != NULL;
}

int NativeTrackPatch_Poll(void)
{
	if (s_trackPatchJob.thread == NULL) { return s_trackPatchJob.succeeded ? 1 : -1; }
	if (!SDL_GetAtomicInt(&s_trackPatchJob.done)) { return 0; }
	SDL_WaitThread(s_trackPatchJob.thread, NULL);
	s_trackPatchJob.thread = NULL;
	return s_trackPatchJob.succeeded ? 1 : -1;
}

const char *NativeTrackPatch_Message(void)
{
	static const char *const stages[] = {"Copying ROM", "Applying xdelta patch", "Extracting ROM with CTR-tools",
	                                     "Finding custom circuits", "Installing custom circuits", "Removing temporary files"};
	if (SDL_GetAtomicInt(&s_trackPatchJob.done)) { return s_trackPatchJob.result; }
	if (SDL_GetAtomicInt(&s_trackPatchJob.cancel)) { return "Cancelling import"; }
	return stages[SDL_GetAtomicInt(&s_trackPatchJob.stage)];
}

void NativeTrackPatch_Cancel(void)
{
	SDL_SetAtomicInt(&s_trackPatchJob.cancel, 1);
}

void NativeTrackPatch_Shutdown(void)
{
	NativeTrackPatch_Cancel();
	if (s_trackPatchJob.thread != NULL)
	{
		SDL_WaitThread(s_trackPatchJob.thread, NULL);
		s_trackPatchJob.thread = NULL;
	}
	if (s_trackPatchJob.reviewReady) { NativeTrackPatch_ReviewDiscard(); }
	NativeTrackRegistry_Destroy(s_trackPatchJob.replacement);
	s_trackPatchJob.replacement = NULL;
}
