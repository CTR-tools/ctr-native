#ifndef CTR_PSX_HWREGS_H
#define CTR_PSX_HWREGS_H

#include <macros.h>

// NOTE(aalhendi): Retail SDK data holds these hardware register pointers.
// Native uses its CD/SPU providers, never this register interface.
extern volatile u8 *psxCdReg0;
extern volatile u8 *psxCdReg1;
extern volatile u8 *psxCdReg2;
extern volatile u8 *psxCdReg3;
extern volatile u16 *psxSpuRegs;

#endif
