/*
 * Derived from REDRIVER2/PsyCross MIT source:
 * externals/PsyCross/src/psx/LIBGPU.C
 * See THIRD_PARTY_NOTICES.md for copyright and license details.
 */

#include <common.h>

void SetDrawMove(DR_MOVE *p, RECT16 *rect, s32 x, s32 y)
{
	s32 length = 5;
	u32 sourceXY;

	if ((rect->w == 0) || (rect->h == 0))
	{
		length = 0;
	}

	p->code[0] = 0x01000000u;
	p->code[1] = 0x80000000u;
	setlen(p, length);
	sourceXY = CTR_ReadU32AlignedLE(&rect->x);
	// NOTE(aalhendi): Unsigned packing retains retail's wrap for negative coordinates.
	p->code[3] = ((u32)y << 16) | (u16)x;
	p->code[2] = sourceXY;
	p->code[4] = CTR_ReadU32AlignedLE(&rect->w);
}
