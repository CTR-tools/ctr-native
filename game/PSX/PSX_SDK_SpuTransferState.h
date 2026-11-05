#ifndef CTR_SDK_SPU_TRANSFER_STATE_H
#define CTR_SDK_SPU_TRANSFER_STATE_H

#include <macros.h>
#include <psx/libspu.h>

extern s32 psxSpuTransferModeRaw;
extern s32 psxSpuTransferMode;
extern SpuTransferCallbackProc psxSpuTransferCallback;

#ifdef CTR_NATIVE
#define SPU_TRANSFER_STATE(type, owner, page, offset) (&(owner))
#else
#define SPU_TRANSFER_STATE(type, owner, page, offset) ((type *)((page) - (offset)))
#endif

#endif
