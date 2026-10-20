#ifndef CTR_MEMCARD_TRANSFER_RETAIL_SYMBOLS_H
#define CTR_MEMCARD_TRANSFER_RETAIL_SYMBOLS_H
#include <common.h>

// NOTE(aalhendi): Typed aliases name canonical resident storage and define no data.
#define MEMCARD_TRANSFER_STAGE ctr_transferStage
extern __typeof__(sdata->memcard_stage) ctr_transferStage asm("1176($28)");
#define MEMCARD_TRANSFER_FD ctr_transferFd
extern __typeof__(sdata->memcard_fd) ctr_transferFd asm("1184($28)");
#define MEMCARD_TRANSFER_STATUS ctr_transferStatus
extern __typeof__(sdata->memcardStatusFlags) ctr_transferStatus asm("2400($28)");
#define MEMCARD_TRANSFER_ICON_SIZE ctr_transferIconSize
extern __typeof__(sdata->memcardIconSize) ctr_transferIconSize asm("2412($28)");
#define MEMCARD_TRANSFER_PATH ctr_transferPath
extern __typeof__(sdata->s_memcardFileCurr) ctr_transferPath asm("sdata_static+49464");
#define MEMCARD_TRANSFER_ICON_HAND ctr_transferIconHand
extern __typeof__(data.memcardIcon_PsyqHand) ctr_transferIconHand asm("data+19968");
#endif
