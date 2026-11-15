#include <platform/chaos/native_chaos_ipc.h>
#include <platform/native_assets.h>
#include <platform/native_log.h>
#include <platform/native_replay_scheduler.h>
#include <SDL3/SDL.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

enum
{
	CHAOS_IPC_POLL_MS = 100,
	CHAOS_IPC_PATH_SIZE = 2048,
};

static bool NativeChaosIPC_Path(char *path, size_t size, const char *name)
{
	return (size_t)snprintf(path, size, "%s/chaos/%s", NativeAssets_GetBaseDir(), name) < size;
}

int NativeChaosIPC_ReadCommand(char *command, size_t capacity)
{
	static Uint64 nextPoll;
	if (NativeReplayScheduler_IsPlayback() || capacity < 2)
	{
		return NATIVE_CHAOS_NO_COMMAND;
	}
	const Uint64 now = SDL_GetTicks();
	if (now < nextPoll)
	{
		return NATIVE_CHAOS_NO_COMMAND;
	}
	nextPoll = now + CHAOS_IPC_POLL_MS;
	char path[CHAOS_IPC_PATH_SIZE];
	if (!NativeChaosIPC_Path(path, sizeof(path), "command"))
	{
		return NATIVE_CHAOS_NO_COMMAND;
	}
	FILE *input = fopen(path, "rb");
	if (input == NULL)
	{
		const int openError = errno;
		if (openError != ENOENT)
		{
			Platform_LogError("[Chaos] Could not open command; retaining it for retry: %s\n", strerror(openError));
		}
		return NATIVE_CHAOS_NO_COMMAND;
	}
	const size_t length = fread(command, 1, capacity, input);
	const bool readError = ferror(input) != 0;
	const bool closeError = fclose(input) != 0;
	if (readError || closeError)
	{
		Platform_LogError("[Chaos] Could not read command; retaining it for retry.\n");
		return NATIVE_CHAOS_NO_COMMAND;
	}
	// Release the slot before activation, so a consumed command cannot run twice.
	if (!SDL_RemovePath(path))
	{
		Platform_LogError("[Chaos] Could not consume command: %s\n", SDL_GetError());
		return NATIVE_CHAOS_NO_COMMAND;
	}
	if (length == capacity || memchr(command, '\0', length) != NULL)
	{
		Platform_LogError("[Chaos] Consumed invalid command: oversized input or embedded NUL.\n");
		return NATIVE_CHAOS_INVALID_COMMAND;
	}
	command[length] = '\0';
	char *slug = command;
	while (isspace((unsigned char)*slug))
	{
		slug++;
	}
	char *end = slug + strlen(slug);
	while (end > slug && isspace((unsigned char)end[-1]))
	{
		end--;
	}
	const size_t slugLength = (size_t)(end - slug);
	memmove(command, slug, slugLength);
	command[slugLength] = '\0';
	return NATIVE_CHAOS_COMMAND;
}

void NativeChaosIPC_WriteEffectsCatalogue(const char *catalogue)
{
	if (NativeReplayScheduler_IsPlayback())
	{
		return;
	}
	char directory[CHAOS_IPC_PATH_SIZE];
	char path[CHAOS_IPC_PATH_SIZE];
	char temporary[CHAOS_IPC_PATH_SIZE];
	if (strlen(catalogue) >= NATIVE_CHAOS_CATALOGUE_SIZE ||
	    !NativeChaosIPC_Path(directory, sizeof(directory), "") ||
	    !NativeChaosIPC_Path(path, sizeof(path), "effects") ||
	    !NativeChaosIPC_Path(temporary, sizeof(temporary), "effects.tmp"))
	{
		Platform_LogError("[Chaos] Effects catalogue or path is too long.\n");
		return;
	}
	if (!SDL_CreateDirectory(directory))
	{
		Platform_LogError("[Chaos] Could not create catalogue directory: %s\n", SDL_GetError());
		return;
	}
	FILE *output = fopen(temporary, "wb");
	if (output == NULL)
	{
		Platform_LogError("[Chaos] Could not write effects catalogue.\n");
		return;
	}
	const bool written = fputs(catalogue, output) >= 0;
	const bool closed = fclose(output) == 0;
	if (!written || !closed || !SDL_RenamePath(temporary, path))
	{
		Platform_LogError("[Chaos] Could not publish effects catalogue.\n");
		SDL_RemovePath(temporary);
		return;
	}
}
