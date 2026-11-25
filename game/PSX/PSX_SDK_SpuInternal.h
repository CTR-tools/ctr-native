#ifndef CTR_PSX_SDK_SPU_INTERNAL_H
#define CTR_PSX_SDK_SPU_INTERNAL_H

#include <ctr_compiler.h>
#include <macros.h>
#include <psx/hwregs.h>
#include <psx/libapi.h>
#include <psx/libetc.h>
#include <psx/libspu.h>

// NOTE(aalhendi): Retail libspu keeps these internal state slots across
// 0x8008c33c..0x8008cb6c; these declarations name existing storage only.
struct PsxSpuRevAttrState
{
	s32 mode;
	SpuVolume depth;
	s32 delay;
	s32 feedback;
};

extern u32 psxSpuEvDma;
extern u32 psxSpuKeyStat;
extern s32 psxSpuTransferModeRaw;
extern s32 psxSpuRevFlag;
extern s32 psxSpuRevReserveWa;
extern u32 psxSpuRevOffsetAddr;
extern struct PsxSpuRevAttrState psxSpuRevAttr;
extern u32 psxSpuRQVoice;
extern u32 psxSpuRQMask;
extern u16 psxSpuVoiceCenterNote[24];
extern u32 psxSpuEnv;
extern s32 psxSpuIsCalled;
extern volatile u16 psxSpuTsa;
extern s32 psxSpuTransferMode;
extern u32 psxSpuMemModePlus;
extern s32 psxSpuInTransfer;
extern volatile SpuTransferCallbackProc psxSpuTransferCallback;
extern SpuIRQCallbackProc psxSpuIrqCallback;
extern u32 psxSpuRevStartAddr[];
extern s32 psxSpuMemListSize;
extern s32 psxSpuMemListCount;
extern void *psxSpuMemList;
extern u8 psxSpuZeroBuf[];

extern void _SpuInit(s32 mode);
extern void _spu_init(s32 mode);
extern void _spu_FsetRXX(s32 l_c, u32 addr, s32 flag);
extern u32 _spu_FsetRXXa(s32 l_c, u32 addr);
extern u32 _spu_Fr(u8 *addr, u32 size);
extern void _spu_Fr_(void *addr, u16 offset, u16 size);
extern void _spu_FiDMA(void);
extern void _spu_t(s32 op, ...);
extern s32 _SpuIsInAllocateArea_(u32 addr);
extern u32 _SpuSetAnyVoice(s32 on_off, u32 voice_bit, s32 regLo, s32 regHi);
extern void _SpuCallback(SpuIRQCallbackProc func);
extern void _SpuDataCallback(void (*func)(void));
extern void *InterruptCallback(s32 irq, void *func);
extern void *DMACallback(s32 dma, void *func);

#endif
