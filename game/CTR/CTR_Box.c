#include <common.h>

void CTR_Box_DrawWirePrims(s16 x1, s16 y1, s16 x2, s16 y2, s32 r, s32 g, s32 b, u32 *ot, struct PrimMem *primMem)
{
	LINE_F2 *next = primMem->cursor;
	LINE_F2 *p = NULL;
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Keep the native null-cursor guard; retail assumes an initialized arena.
	if (!next)
		return;
#endif
	if (next <= (LINE_F2 *)primMem->guardEnd)
	{
		p = next;
		primMem->cursor = p + 1;
	}
	if (p)
	{
		setLineF2(p);
		p->r0 = r;
		p->g0 = g;
		p->b0 = b;
		p->x0 = x1;
		p->y0 = y1;
		p->x1 = x2;
		p->y1 = y2;
		AddPrim(ot, p);
	}
}

void CTR_Box_DrawWireBox(RECT *r, const Color *color, void *ot, struct PrimMem *primMem)
{
	LINE_F3 *next = primMem->cursor;
	LINE_F3 *p = NULL;
	if (next <= (LINE_F3 *)primMem->guardEnd)
	{
		p = next;
		primMem->cursor = p + 1;
	}
	if (p)
	{
		setLineF3(p);
		// Two polylines form the outline; each allocation can fail independently.
		p->r0 = color->r;
		p->g0 = color->g;
		p->b0 = color->b;
		p->x0 = r->x;
		p->y0 = r->y;
		p->x1 = r->x + r->w;
		p->y1 = r->y;
		p->x2 = r->x + r->w;
		p->y2 = r->y + r->h;
		AddPrim(ot, p);

		next = primMem->cursor;
		p = NULL;
		if (next <= (LINE_F3 *)primMem->guardEnd)
		{
			p = next;
			primMem->cursor = p + 1;
		}
		if (p)
		{
			setLineF3(p);
			p->r0 = color->r;
			p->g0 = color->g;
			p->b0 = color->b;
			p->x0 = r->x;
			p->y0 = r->y;
			p->x1 = r->x;
			p->y1 = r->y + r->h;
			p->x2 = r->x + r->w;
			p->y2 = r->y + r->h;
			AddPrim(ot, p);
		}
	}
}

void CTR_Box_DrawClearBox(const RECT *r, const Color *color, s32 transparency, u32 *ot, struct PrimMem *primMem)
{
	typedef struct TPagePolyF4
	{
		DR_TPAGE page;
		POLY_F4 poly;
	} TPagePolyF4;
	TPagePolyF4 *next = primMem->cursor;
	TPagePolyF4 *p = NULL;
	if (next <= (TPagePolyF4 *)primMem->guardEnd)
	{
		p = next;
		primMem->cursor = p + 1;
	}
	if (p)
	{
		setlen(p, 7);
		// One packet carries the draw mode, a zero command, and the translucent quad.
		setcode(&p->poly, 0x2a);
		p->page.code[0] = ((u32)transparency << 5) | 0xe1000a00u;
		p->poly.tag = 0;
		p->poly.r0 = color->r;
		p->poly.g0 = color->g;
		p->poly.b0 = color->b;
		p->poly.x0 = r->x;
		p->poly.y0 = r->y;
		p->poly.x1 = r->x + r->w;
		p->poly.y1 = r->y;
		p->poly.x2 = r->x;
		p->poly.y2 = r->y + r->h;
		p->poly.x3 = r->x + r->w;
		p->poly.y3 = r->y + r->h;
#ifdef CTR_NATIVE
		// NOTE(aalhendi): PsyCross requires drawing into the display area to be enabled.
		p->page.code[0] |= 0x400;
#endif
		AddPrim(ot, p);
	}
}

void CTR_Box_DrawSolidBox(RECT *r, const Color *color, u32 *ot, struct PrimMem *primMem)
{
	POLY_F4 *next = primMem->cursor;
	POLY_F4 *p = NULL;
	if (next <= (POLY_F4 *)primMem->guardEnd)
	{
		p = next;
		primMem->cursor = p + 1;
	}
	if (p)
	{
		// NOTE(aalhendi): Retail packs the solid quad directly, masking each X halfword.
		CtrGpu_WriteColorCode(&p->r0, (ColorCode_GetPacked(color) & 0xffffffu) | 0x28000000u);
		CtrGpu_WritePackedXY(&p->x0, (u16)r->x | ((u32)r->y << 16));
		CtrGpu_WritePackedXY(&p->x1, ((r->x + r->w) & 0xffff) | ((u32)r->y << 16));
		CtrGpu_WritePackedXY(&p->x2, (u16)r->x | ((u32)(r->y + r->h) << 16));
		CtrGpu_WritePackedXY(&p->x3, ((r->x + r->w) & 0xffff) | ((u32)(r->y + r->h) << 16));
		p->tag = (*ot & 0xffffffu) | 0x05000000u;
		*ot = CtrGpu_PrimToOTLink24(p);
	}
}
