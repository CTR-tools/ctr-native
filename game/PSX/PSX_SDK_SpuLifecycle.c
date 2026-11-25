#include "PSX_SDK_SpuInternal.h"

u32 SpuSetIRQAddr(u32 addr)
{
	if (addr > 0x7fff8u)
	{
		return 0;
	}
	return _spu_FsetRXXa(210, addr);
}

u32 SpuSetTransferStartAddr(u32 addr)
{
	if ((addr < 0x1010u) || (addr > 0x7fff8u))
	{
		return 0;
	}
	psxSpuTsa = (u16)_spu_FsetRXXa(-1, addr);
	// NOTE(aalhendi): Match retail reload of psxSpuTsa before shifting by
	// psxSpuMemModePlus.
	CTR_PSX_MEMORY_BARRIER();
	return (u32)psxSpuTsa << psxSpuMemModePlus;
}

void SpuInit(void)
{
	_SpuInit(0);
}
