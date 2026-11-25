#include <macros.h>

// NOTE(aalhendi): Retail libetc dispatches callback registration through this
// resident table at 0x8008c020; this declaration names existing storage only.
struct PsxCallbackDispatch
{
	void *unused0;
	void *(*dmaCallback)(s32, void *);
	void *(*intrCallback)(s32, void *);
	s32 (*resetCallback)(void);
	void *unused4;
	void *(*vsyncCallback)(s32, void *);
};

extern struct PsxCallbackDispatch *psxCallbackDispatch;

s32 ResetCallback(void)
{
	return psxCallbackDispatch->resetCallback();
}

void *InterruptCallback(s32 irq, void *func)
{
	return psxCallbackDispatch->intrCallback(irq, func);
}

void *DMACallback(s32 dma, void *func)
{
	return psxCallbackDispatch->dmaCallback(dma, func);
}

void *VSyncCallback(void (*func)(void))
{
	return psxCallbackDispatch->vsyncCallback(4, (void *)func);
}
