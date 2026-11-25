#include "PSX_SDK_GpuInternal.h"

int LoadImage(RECT16 *rect, void *p)
{
	psxGpuCheckRect("LoadImage", rect);
	return psxGpuJtb->addque(psxGpuJtb->load, rect, 8, p);
}

int StoreImage(RECT16 *rect, u32 *p)
{
	psxGpuCheckRect("StoreImage", rect);
	return psxGpuJtb->addque(psxGpuJtb->store, rect, 8, p);
}

int MoveImage(RECT16 *rect, int x, int y)
{
	psxGpuCheckRect("MoveImage", rect);
	if ((rect->w == 0) || (rect->h == 0))
	{
		return -1;
	}
	psxGpuMovePrim.src = ((u32 *)rect)[0];
	psxGpuMovePrim.dst = ((u32)y << 16) | ((u32)x & 0xffffu);
	psxGpuMovePrim.wh = ((u32 *)rect)[1];
	return psxGpuJtb->addque(psxGpuJtb->otc, &psxGpuMovePrim.tag, 20, NULL);
}
