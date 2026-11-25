#include <ctr_compiler.h>
#include <macros.h>

// NOTE(aalhendi): Retail libetc stores the DICR hardware register pointer at
// 0x8008cb04 and the 8-slot DMA callback table at 0x8008cb08; these
// declarations name existing resident storage only.
extern volatile u32 *psxDmaDicr;
extern void (*psxDmaCallbacks[8])(void);
extern void *InterruptCallback(s32 irq, void *func);
extern void _trapIntrDMA(void);
extern void *_setIntrDMA(s32 ch, void (*func)(void));
extern void _clearIntrDMATable(void **buf, s32 n);

void *startIntrDMA(void)
{
	void *setter;

	_clearIntrDMATable((void **)psxDmaCallbacks, 8);
	// NOTE(aalhendi): Non-volatile store matches the retail initialization write
	// placed in the InterruptCallback delay slot before IRQ 3 is enabled.
	*(u32 *)psxDmaDicr = 0;
	InterruptCallback(3, _trapIntrDMA);
	setter = _setIntrDMA;
	// NOTE(aalhendi): Keep the return address materialized before the framed
	// epilogue restores $ra.
	CTR_PSX_KEEP_VALUE(setter);
	return setter;
}
