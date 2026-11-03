#include <platform/native_track_patch.h>
#include <platform/native_track_import.h>
#include <platform/native_track_library.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#include "native_track_patch_disc.c"

enum { PATCH_PATH_BYTES = 1024, PATCH_NAME_BYTES = 200, PATCH_TRACK_COUNT = 18 };
enum PatchStage { PATCH_COPY, PATCH_APPLY, PATCH_EXTRACT, PATCH_COMPARE, PATCH_INSTALL, PATCH_CLEANUP };

struct TrackPatchJob
{
	SDL_Thread *thread;
	SDL_AtomicInt done, cancel, stage;
	int succeeded;
	char assets[PATCH_PATH_BYTES], patch[PATCH_PATH_BYTES], name[PATCH_NAME_BYTES];
	char result[256];
};

static struct TrackPatchJob s_trackPatchJob;

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

static const char *TrackPatch_Collect(struct TrackPatchJob *job, const char *work, int *count)
{
	char original[PATCH_PATH_BYTES], patched[PATCH_PATH_BYTES], prepared[PATCH_PATH_BYTES];
	if (!TrackPatch_Path(original, work, "original/levels/tracks") ||
	    !TrackPatch_Path(patched, work, "patched/levels/tracks") ||
	    !TrackPatch_Path(prepared, work, "prepared"))
	{
		return "Path is too long";
	}
	*count = 0;
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
			return "Missing circuit files - unsupported BIGFILE layout";
		}
		if (!changedLev && !changedVrm) { continue; }
		char source[PATCH_PATH_BYTES], name[PATCH_NAME_BYTES];
		SDL_snprintf(relative, sizeof(relative), "%s/1P", s_patchTrackSlots[i]);
		if (!TrackPatch_Path(source, patched, relative)) { return "Path is too long"; }
		SDL_snprintf(name, sizeof(name), "%s-%s", job->name, s_patchTrackSlots[i]);
		const char *error = NativeTrackImport_Directory(prepared, source, name);
		if (error != NULL) { return error; }
		(*count)++;
	}
	return *count != 0 ? NULL : "No modified single-player circuits found";
}

struct TrackPatchPublish
{
	struct TrackPatchJob *job;
	const char *library;
	const char *error;
	char installed[PATCH_TRACK_COUNT][PATCH_PATH_BYTES];
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
	else if (publish->count == PATCH_TRACK_COUNT || !TrackPatch_Path(source, directory, name) ||
	         !TrackPatch_Path(target, publish->library, name))
	{
		publish->error = "Path is too long";
	}
	else
	{
		publish->error = NativeTrackImport_Directory(publish->library, source, name);
		if (publish->error == NULL)
		{
			SDL_strlcpy(publish->installed[publish->count++], target, PATCH_PATH_BYTES);
		}
	}
	return publish->error == NULL ? SDL_ENUM_CONTINUE : SDL_ENUM_FAILURE;
}

static const char *TrackPatch_InstallPrepared(struct TrackPatchJob *job, const char *prepared, const char *library)
{
	struct TrackPatchPublish publish = {.job = job, .library = library};
	const char *error = NULL;
	if (!SDL_EnumerateDirectory(prepared, TrackPatch_Publish, &publish))
	{
		error = publish.error != NULL ? publish.error : "Cannot read prepared tracks";
	}
	if (SDL_GetAtomicInt(&job->cancel))
	{
		error = "Import cancelled";
	}
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
	return error;
}

static const char *TrackPatch_Run(struct TrackPatchJob *job, const char *work, int *count)
{
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
	if (!TrackPatch_Process(job, extractOriginal) || !TrackPatch_Process(job, extractPatched))
	{
		return TrackPatch_Error(job, "CTR-tools bigtool failed");
	}
	SDL_SetAtomicInt(&job->stage, PATCH_COMPARE);
	const char *error = TrackPatch_Collect(job, work, count);
	if (error != NULL) { return error; }
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
		SDL_SetAtomicInt(&job->stage, PATCH_CLEANUP);
		int removed = TrackPatch_RemoveTree(work);
		if (!removed) { error = "Import ended - temporary cleanup failed"; }
	}
	job->succeeded = error == NULL;
	if (error != NULL) { SDL_strlcpy(job->result, error, sizeof(job->result)); }
	else { SDL_snprintf(job->result, sizeof(job->result), "%d track%s imported", count, count == 1 ? "" : "s"); }
	SDL_SetAtomicInt(&job->done, 1);
	return 0;
}

int NativeTrackPatch_Start(const char *assets, const char *patch)
{
	struct TrackPatchJob *job = &s_trackPatchJob;
	if (job->thread != NULL || assets == NULL || patch == NULL || strlen(assets) >= sizeof(job->assets) ||
	    strlen(patch) >= sizeof(job->patch))
	{
		return 0;
	}
	memset(job, 0, sizeof(*job));
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
}
