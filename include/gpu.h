#pragma once

#ifdef CTR_NATIVE
#include <platform/native_gpu_links.h>
#endif

// PS1 primitive tags store the next OT link as 24 bits. Native routes this
// through a GPU link-token bridge so the packet layout stays retail-shaped.
force_inline u32 CtrGpu_PrimToOTLink24(const void *prim)
{
#ifdef CTR_NATIVE
	return NativeGpuLinks_FromHostPointer(prim);
#else
	return (u32)((uintptr_t)prim & 0xffffffu);
#endif
}

force_inline u32 CtrGpu_PackOTTag(uint32_t ot, u32 tag)
{
	return ((u32)ot & 0xffffffu) | tag;
}

#ifdef CTR_NATIVE
force_inline b32 CtrGpu_IsCurrentOTRange(const struct DB *db, const uint32_t *start, const uint32_t *end)
{
	uintptr_t rangeStart;
	uintptr_t rangeEnd;
	uintptr_t otStart;
	uintptr_t otCursor;

	if ((db == NULL) || (start == NULL) || (end == NULL))
	{
		return false;
	}

	rangeStart = (uintptr_t)start;
	rangeEnd = (uintptr_t)end;
	otStart = (uintptr_t)db->otMem.start;
	otCursor = (uintptr_t)db->otMem.cursor;

	if (rangeEnd < rangeStart)
	{
		return false;
	}

	return (rangeStart >= otStart) && (rangeEnd < otCursor);
}

#endif

force_inline u32 CtrGpu_PackColorCode(u32 color, u32 code)
{
	return (color & 0xffffff) | (code << 24);
}

force_inline void CtrGpu_WriteColorCode(uint8_t *r, u32 colorCode)
{
	r[0] = (uint8_t)colorCode;
	r[1] = (uint8_t)(colorCode >> 8);
	r[2] = (uint8_t)(colorCode >> 16);
	r[3] = (uint8_t)(colorCode >> 24);
}

force_inline void CtrGpu_WritePackedXY(VERTTYPE *x, u32 xy)
{
	x[0] = (VERTTYPE)xy;
	x[1] = (VERTTYPE)(xy >> 16);
}

force_inline void CtrGpu_WritePackedUV(uint8_t *u, u16 uv)
{
	u[0] = (uint8_t)uv;
	u[1] = (uint8_t)(uv >> 8);
}

force_inline void CtrGpu_WritePackedUVWord(uint8_t *u, u32 uvTpage)
{
	u[0] = (uint8_t)uvTpage;
	u[1] = (uint8_t)(uvTpage >> 8);
	u[2] = (uint8_t)(uvTpage >> 16);
	u[3] = (uint8_t)(uvTpage >> 24);
}

CTR_STATIC_ASSERT(sizeof(POLY_FT4) == 0x28);
CTR_STATIC_ASSERT(offsetof(POLY_FT4, tag) == 0x00);
CTR_STATIC_ASSERT(offsetof(POLY_FT4, r0) == 0x04);
CTR_STATIC_ASSERT(offsetof(POLY_FT4, code) == 0x07);
CTR_STATIC_ASSERT(offsetof(POLY_FT4, x0) == 0x08);
CTR_STATIC_ASSERT(offsetof(POLY_FT4, u0) == 0x0C);
CTR_STATIC_ASSERT(offsetof(POLY_FT4, x1) == 0x10);
CTR_STATIC_ASSERT(offsetof(POLY_FT4, u1) == 0x14);
CTR_STATIC_ASSERT(offsetof(POLY_FT4, x2) == 0x18);
CTR_STATIC_ASSERT(offsetof(POLY_FT4, u2) == 0x1C);
CTR_STATIC_ASSERT(offsetof(POLY_FT4, x3) == 0x20);
CTR_STATIC_ASSERT(offsetof(POLY_FT4, u3) == 0x24);

CTR_STATIC_ASSERT(sizeof(POLY_GT4) == 0x34);
CTR_STATIC_ASSERT(offsetof(POLY_GT4, tag) == 0x00);
CTR_STATIC_ASSERT(offsetof(POLY_GT4, r0) == 0x04);
CTR_STATIC_ASSERT(offsetof(POLY_GT4, code) == 0x07);
CTR_STATIC_ASSERT(offsetof(POLY_GT4, x0) == 0x08);
CTR_STATIC_ASSERT(offsetof(POLY_GT4, u0) == 0x0C);
CTR_STATIC_ASSERT(offsetof(POLY_GT4, r1) == 0x10);
CTR_STATIC_ASSERT(offsetof(POLY_GT4, x1) == 0x14);
CTR_STATIC_ASSERT(offsetof(POLY_GT4, u1) == 0x18);
CTR_STATIC_ASSERT(offsetof(POLY_GT4, r2) == 0x1C);
CTR_STATIC_ASSERT(offsetof(POLY_GT4, x2) == 0x20);
CTR_STATIC_ASSERT(offsetof(POLY_GT4, u2) == 0x24);
CTR_STATIC_ASSERT(offsetof(POLY_GT4, r3) == 0x28);
CTR_STATIC_ASSERT(offsetof(POLY_GT4, x3) == 0x2C);
CTR_STATIC_ASSERT(offsetof(POLY_GT4, u3) == 0x30);

struct CtrGpuDrawModePacket
{
	u32 tag;
	u32 drawMode;
	u32 terminator;
};

CTR_STATIC_ASSERT(sizeof(struct CtrGpuDrawModePacket) == 0x0C);
CTR_STATIC_ASSERT(offsetof(struct CtrGpuDrawModePacket, tag) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct CtrGpuDrawModePacket, drawMode) == 0x04);
CTR_STATIC_ASSERT(offsetof(struct CtrGpuDrawModePacket, terminator) == 0x08);

force_inline void CtrGpu_LinkPacket24(uint32_t *ot, u32 *packetTag, const void *packet, u32 tag)
{
	*packetTag = CtrGpu_PackOTTag(*ot, tag);
	*ot = (uint32_t)CtrGpu_PrimToOTLink24(packet);
}

force_inline void CtrGpu_LinkPrimToOT(uint32_t *ot, const void *prim)
{
	*ot = (uint32_t)CtrGpu_PrimToOTLink24(prim);
}

force_inline void addPolyF3(uint32_t *ot, POLY_F3 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x4000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x20;
}

force_inline void addPolyFT3(uint32_t *ot, POLY_FT3 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x7000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x24;
}

force_inline void addPolyG3(uint32_t *ot, POLY_G3 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x6000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x30;
}

force_inline void addPolyGT3(uint32_t *ot, POLY_GT3 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x9000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x34;
}

force_inline void addPolyF4(uint32_t *ot, POLY_F4 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x5000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x28;
}

force_inline void addPolyFT4(uint32_t *ot, POLY_FT4 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x9000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x2c;
}

force_inline void addPolyG4(uint32_t *ot, POLY_G4 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x8000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x38;
}

force_inline void addPolyGT4(uint32_t *ot, POLY_GT4 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0xc000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x3c;
}

force_inline void addSprt8(uint32_t *ot, SPRT *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x3000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x74;
}

force_inline void addSprt16(uint32_t *ot, SPRT *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x3000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x7c;
}

force_inline void addSprt(uint32_t *ot, SPRT *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x4000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x64;
}

force_inline void addTile1(uint32_t *ot, TILE *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x2000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x68;
}

force_inline void addTile8(uint32_t *ot, TILE *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x2000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x70;
}

force_inline void addTile16(uint32_t *ot, TILE *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x2000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x78;
}

force_inline void addTile(uint32_t *ot, TILE *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x3000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x60;
}

force_inline void addLineF2(uint32_t *ot, LINE_F2 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x3000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x40;
}

force_inline void addLineG2(uint32_t *ot, LINE_G2 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x4000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x50;
}

force_inline void addLineF3(uint32_t *ot, LINE_F3 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x5000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x48;
	p->pad = 0x55555555;
}

force_inline void addLineG3(uint32_t *ot, LINE_G3 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x7000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x58;
	p->pad = 0x55555555;
	p->p1, p->p2 = 0;
}

force_inline void addLineF4(uint32_t *ot, LINE_F4 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x6000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x4c;
	p->pad = 0x55555555;
}

force_inline void addLineG4(uint32_t *ot, LINE_G4 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x9000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x5c;
	p->pad = 0x55555555;
	p->p1, p->p2 = 0;
}

#ifndef CTR_NATIVE
force_inline void addFill(uint32_t *ot, FILL *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x3000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 2;
}
#endif

// version of psn00bsdk's setXY4 macro that compiles to a smaller bytesize
// based on original compiled code for the game's primitive functions
// this produces bugs if any of the X values are negative and not cast to u16
// this is terrible code
// please avoid writing something like this, unless you really really need it
#define setXY4CompilerHack(p, s0, t0, s1, t1, s2, t2, s3, t3) \
	*(u32 *)&p->x0 = s0 | (t0 << 16), *(u32 *)&p->x1 = s1 | (t1 << 16), *(u32 *)&p->x2 = s2 | (t2 << 16), *(u32 *)&p->x3 = s3 | (t3 << 16)

// like psn00bsdk's setColor macros but with terrible compiler hacks
// as the color values are read and written as 32-bit ints these have to be used prior to setting code
#define setInt32RGB0(p, color0)                         *(u32 *)&p->r0 = color0

#define setInt32RGB4(p, color0, color1, color2, color3) *(u32 *)&p->r0 = color0, *(u32 *)&p->r1 = color1, *(u32 *)&p->r2 = color2, *(u32 *)&p->r3 = color3

// clear blending mode bits of the texpage using AND, then set them using OR
// then set image to use semi-transparent mode using the setSemiTrans macro
// (which enables the 2 bit on the primitive's code field)
#define setTransparency(p, transparency)                p->tpage = (p->tpage & 0xff9f) | ((transparency - 1) << 5), p->code |= 2

// version of psn00bsdk's setColor macro that simultaneously accepts 4 colors
#define setColor4(p, rgb0, rgb1, rgb2, rgb3)                                                                                         \
	(((P_COLOR *)&((p)->r0))->color = (rgb0)), (((P_COLOR *)&((p)->r1))->color = (rgb1)), (((P_COLOR *)&((p)->r2))->color = (rgb2)), \
	    (((P_COLOR *)&((p)->r3))->color = (rgb3))
