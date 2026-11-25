#include "PSX_SDK_SpuInternal.h"

#ifdef CTR_NATIVE
#include <stdarg.h>
#else
// NOTE(aalhendi): Standalone PSX matching builds do not provide a libc
// <stdarg.h>; match PsyQ MIPS O32 variadic stack stepping against &op.
typedef char *va_list;
#define va_start(ap, last) ((ap) = (char *)&(last) + ((sizeof(last) + 3u) & ~3u))
#define va_arg(ap, type)   (*(type *)(((ap) += ((sizeof(type) + 3u) & ~3u)) - ((sizeof(type) + 3u) & ~3u)))
#define va_end(ap)         ((void)0)
#endif

void _spu_Fr_(void *addr, u16 offset, u16 size)
{
	psxSpuSdkRegs[0x1a6 / sizeof(u16)] = offset;
	psxSpuWaitReady();
	psxSpuSdkRegs[0x1aa / sizeof(u16)] |= 0x30u;
	psxSpuWaitReady();
	psxSpuSetDmaReadMode();
	*psxSpuDmaMadr = (u32)addr;
	*psxSpuDmaBcr = ((u32)size << 16) | 0x10u;
	psxSpuDmaDir = 1;
	*psxSpuDmaChcr = 0x01000200u;
}

s32 _spu_t(s32 op, ...)
{
	va_list ap;
	u32 i;
	u16 expected;
	u32 arg;
	u32 chcr;

	va_start(ap, op);
	switch (op)
	{
	case 2:
		arg = va_arg(ap, u32);
		psxSpuTsa = (u16)(arg >> psxSpuMemModePlus);
		psxSpuSdkRegs[0x1a6 / sizeof(u16)] = psxSpuTsa;
		break;
	case 1:
		psxSpuDmaDir = 0;
		i = 0;
		while (psxSpuSdkRegs[0x1a6 / sizeof(u16)] != psxSpuTsa)
		{
			if (++i > 0xf00u)
			{
				return -2;
			}
		}
		psxSpuSdkRegs[0x1aa / sizeof(u16)] = (u16)((psxSpuSdkRegs[0x1aa / sizeof(u16)] & 0xffcfu) | 0x20u);
		break;
	case 0:
		psxSpuDmaDir = 1;
		i = 0;
		while (psxSpuSdkRegs[0x1a6 / sizeof(u16)] != psxSpuTsa)
		{
			if (++i > 0xf00u)
			{
				return -2;
			}
		}
		psxSpuSdkRegs[0x1aa / sizeof(u16)] |= 0x30u;
		break;
	case 3:
		expected = (psxSpuDmaDir == 1) ? 0x30u : 0x20u;
		i = 0;
		while ((psxSpuSdkRegs[0x1aa / sizeof(u16)] & 0x30u) != expected)
		{
			if (++i > 0xf00u)
			{
				return -2;
			}
		}
		if (psxSpuDmaDir == 1)
		{
			psxSpuSetDmaReadMode();
		}
		else
		{
			psxSpuSetDmaWriteMode();
		}
		arg = va_arg(ap, u32);
		psxSpuDmaAddr = arg;
		arg = va_arg(ap, u32);
		psxSpuDmaBlocks = (arg >> 6) + ((arg & 0x3fu) ? 1u : 0u);
		*psxSpuDmaMadr = psxSpuDmaAddr;
		*psxSpuDmaBcr = (psxSpuDmaBlocks << 16) | 0x10u;
		chcr = (psxSpuDmaDir == 1) ? 0x01000200u : 0x01000201u;
		*psxSpuDmaChcr = chcr;
		break;
	}
	va_end(ap);
	return 0;
}

u32 _spu_Fr(u8 *addr, u32 size)
{
	if (psxSpuTransferMode == 0)
	{
		_spu_t(2, (u32)psxSpuTsa << psxSpuMemModePlus);
		_spu_t(1);
		_spu_t(3, addr, size);
	}
	else
	{
		psxSpuWriteByIO(addr, size);
	}
	return size;
}

void _spu_FsetRXX(s32 l_c, u32 addr, s32 flag)
{
	if (flag == 0)
	{
		((u16 *)psxSpuSdkRegs)[l_c] = (u16)addr;
	}
	else
	{
		((u16 *)psxSpuSdkRegs)[l_c] = (u16)(addr >> psxSpuMemModePlus);
	}
	// NOTE(aalhendi): Keep both branches converging on a single leaf return so
	// GCC fills the branch delay slot with the halfword store instead of
	// duplicating jr $ra.
	CTR_PSX_MEMORY_BARRIER();
}
