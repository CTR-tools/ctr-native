#include "PSX_SDK_GpuInternal.h"

u32 DrawSyncCallback(void (*func)(void))
{
	void (*old)(void);

	if (psxGpuEnv.debug >= 2)
	{
		psxGpuPrintf("DrawSyncCallback(%08x)...\n", func);
	}
	old = psxGpuEnv.drawSyncCb;
	psxGpuEnv.drawSyncCb = func;
	return (u32)old;
}

void SetDispMask(int mask)
{
	if (psxGpuEnv.debug >= 2)
	{
		psxGpuPrintf("SetDispMask(%d)...\n", mask);
	}
	if (mask == 0)
	{
		psxGpuFillBytes(&psxGpuEnv.dispEnv, -1, sizeof(psxGpuEnv.dispEnv));
	}
	psxGpuJtb->cwc((mask != 0) ? 0x03000000u : 0x03000001u);
}
