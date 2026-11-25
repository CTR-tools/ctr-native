#include <macros.h>

extern void psxDctInHw(u32 *buf, u32 size);
extern void psxDctOutHw(u32 *buf, s32 size);

s32 DecDCTBufSize(u32 *bs)
{
	return (u16)*bs;
}

void DecDCTin(u32 *buf, s32 mode)
{
	if (mode & 1)
	{
		*buf &= ~0x08000000u;
	}
	else
	{
		*buf |= 0x08000000u;
	}
	if (mode & 2)
	{
		*buf |= 0x02000000u;
	}
	else
	{
		*buf &= ~0x02000000u;
	}
	psxDctInHw(buf, (u16)*buf);
}

void DecDCTout(u32 *buf, s32 size)
{
	psxDctOutHw(buf, size);
}
