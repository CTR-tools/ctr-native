#include <macros.h>
#include <psx/libspu.h>

SpuVolume psxSpuReverbDepth CTR_PSX_MATCH_SECTION(".data.psxSpuReverbDepth") = {0, 0};
volatile u16 *psxSpuSdkRegs CTR_PSX_MATCH_SECTION(".data.psxSpuSdkRegs") = (volatile u16 *)0x1f801c00u;

#ifndef CTR_NATIVE
// NOTE(aalhendi): Describe the genuine C objects to the old assembler; these annotations emit no bytes.
__asm__(".type psxSpuReverbDepth,@object\n.size psxSpuReverbDepth,4\n");
__asm__(".type psxSpuSdkRegs,@object\n.size psxSpuSdkRegs,4\n");
#endif
