#include "PSX_SDK_SpuInternal.h"

void _spu_FiDMA(void)
{
	u32 i;

	if (psxSpuDmaDir == 0)
	{
		psxSpuWaitReady();
	}
	psxSpuSdkRegs[0x1aa / sizeof(u16)] &= 0xffcfu;
	i = 0;
	while ((psxSpuSdkRegs[0x1aa / sizeof(u16)] & 0x30u) != 0)
	{
		if (++i > 0xf00u)
		{
			break;
		}
	}
	if (psxSpuTransferCallback != NULL)
	{
		psxSpuTransferCallback();
	}
	else
	{
		DeliverEvent(0xf0000009, 0x20);
	}
}
