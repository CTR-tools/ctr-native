#ifndef CTR_MEMCARD_FILEIO_RETAIL_SYMBOLS_H
#define CTR_MEMCARD_FILEIO_RETAIL_SYMBOLS_H
#include "../../retail_bindings.h"

// NOTE(aalhendi): Typed aliases name canonical resident storage and define no data.
#define MEMCARD_FILEIO_STAGE ctr_fileioStage
extern __typeof__(sdata->memcard_stage) ctr_fileioStage asm("1176($28)");
#define MEMCARD_FILEIO_BUFFER ctr_fileioBuffer
extern __typeof__(sdata->memcard_ptrStart) ctr_fileioBuffer asm("1180($28)");
#define MEMCARD_FILEIO_FD ctr_fileioFd
extern __typeof__(sdata->memcard_fd) ctr_fileioFd asm("1184($28)");
#define MEMCARD_FILEIO_RETRIES ctr_fileioRetries
extern __typeof__(sdata->memcard_remainingAttempts) ctr_fileioRetries asm("1188($28)");
#define MEMCARD_FILEIO_SIZE ctr_fileioSize
extern __typeof__(sdata->memcardFileSize) ctr_fileioSize asm("2364($28)");
#define MEMCARD_FILEIO_SLOT ctr_fileioSlot
extern __typeof__(sdata->memcardSlot) ctr_fileioSlot asm("2380($28)");
#define MEMCARD_FILEIO_PATH ctr_fileioPath
extern __typeof__(sdata->s_memcardFileCurr) ctr_fileioPath asm("sdata_static+49464");
#endif
