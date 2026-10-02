#include <common.h>

// NOTE(aalhendi): Matching bindings name canonical resident fields; native uses the live sdata pointer.
#ifndef MEMCARD_FILEIO_STAGE
#define MEMCARD_FILEIO_STAGE   sdata->memcard_stage
#define MEMCARD_FILEIO_BUFFER  sdata->memcard_ptrStart
#define MEMCARD_FILEIO_FD      sdata->memcard_fd
#define MEMCARD_FILEIO_RETRIES sdata->memcard_remainingAttempts
#define MEMCARD_FILEIO_SIZE    sdata->memcardFileSize
#define MEMCARD_FILEIO_SLOT    sdata->memcardSlot
#define MEMCARD_FILEIO_PATH    sdata->s_memcardFileCurr
#endif

int MEMCARD_NewTask(int slotIdx, char *name, u8 *ptrMemcard, int memcardFileSize)
{
	MEMCARD_FILEIO_SLOT = slotIdx;

	MEMCARD_StringSet(MEMCARD_FILEIO_PATH, slotIdx, name);

	MEMCARD_FILEIO_BUFFER = ptrMemcard;
	MEMCARD_FILEIO_RETRIES = 8;
	MEMCARD_FILEIO_SIZE = memcardFileSize;

	return 0;
}

void MEMCARD_CloseFile(void)
{
	int fd = MEMCARD_FILEIO_FD;

	if (fd != -1)
	{
		close(fd);
		MEMCARD_FILEIO_FD = -1;
	}

	MEMCARD_FILEIO_STAGE = MC_STAGE_IDLE;
}

int MEMCARD_ReadFile(int start_offset, int size)
{
	if (lseek(MEMCARD_FILEIO_FD, start_offset, 0) != -1 && read(MEMCARD_FILEIO_FD, MEMCARD_FILEIO_BUFFER, size) != -1)
	{
		// The read has started, the result will be found
		// the next time we wait for an event result
		return MC_RETURN_PENDING;
	}

	MEMCARD_CloseFile();
	return MC_RETURN_TIMEOUT;
}

u8 MEMCARD_WriteFile(int start_offset, const u8 *data, int size)
{
	if (lseek(MEMCARD_FILEIO_FD, start_offset, 0) != -1 && write(MEMCARD_FILEIO_FD, (void *)data, size) != -1)
	{
		// The write has started, the result will be found
		// the next time we wait for an event result
		return MC_RETURN_PENDING;
	}

	MEMCARD_CloseFile();
	return MC_RETURN_TIMEOUT;
}
