#ifndef NATIVE_CHAOS_IPC_H
#define NATIVE_CHAOS_IPC_H

#include <stddef.h>

enum
{
	NATIVE_CHAOS_INVALID_COMMAND = -1,
	NATIVE_CHAOS_NO_COMMAND = 0,
	NATIVE_CHAOS_COMMAND = 1,
	NATIVE_CHAOS_COMMAND_SIZE = 128,
	NATIVE_CHAOS_CATALOGUE_SIZE = 8192,
};

// Only returns COMMAND/INVALID after the file has been consumed successfully.
// COMMAND returns a NUL-terminated slug with surrounding whitespace removed.
int NativeChaosIPC_ReadCommand(char *command, size_t capacity);
void NativeChaosIPC_WriteEffectsCatalogue(const char *catalogue);

#endif
