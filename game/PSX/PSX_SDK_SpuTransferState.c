#include "PSX_SDK_SpuTransferState.h"

s32 psxSpuTransferModeRaw CTR_PSX_MATCH_SECTION(".data.psxSpuTransferModeRaw") = 0;
s32 psxSpuTransferMode CTR_PSX_MATCH_SECTION(".data.psxSpuTransferMode") = 0;
SpuTransferCallbackProc psxSpuTransferCallback CTR_PSX_MATCH_SECTION(".data.psxSpuTransferCallback") = 0;

#ifndef CTR_NATIVE
// NOTE(aalhendi): Describe the genuine C objects for old GCC; these annotations emit no bytes.
__asm__(".type psxSpuTransferModeRaw,@object\n.size psxSpuTransferModeRaw,4\n"
        ".type psxSpuTransferMode,@object\n.size psxSpuTransferMode,4\n"
        ".type psxSpuTransferCallback,@object\n.size psxSpuTransferCallback,4\n");
#endif
