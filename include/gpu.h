#ifndef CTR_NATIVE_GPU_H
#define CTR_NATIVE_GPU_H

#include <platform/native_gpu_links.h>
#include <platform/native_pgxp.h>

// PS1 primitive tags store the next OT link as 24 bits. Native routes this
// through a GPU link-token bridge so the packet layout stays retail-shaped.
static inline u32 CtrGpu_PrimToOTLink24(const void *prim)
{
	return NativeGpuLinks_FromHostPointer(prim);
}

static inline u32 CtrGpu_PackOTTag(u32 ot, u32 tag)
{
	return ((u32)ot & 0xffffffu) | tag;
}

static inline b32 CtrGpu_IsCurrentOTRange(const struct DB *db, const u32 *start, const u32 *end)
{
	u32 rangeStart;
	u32 rangeEnd;
	u32 otStart;
	u32 otCursor;

	if ((db == NULL) || (start == NULL) || (end == NULL))
	{
		return false;
	}

	rangeStart = (u32)start;
	rangeEnd = (u32)end;
	otStart = (u32)db->otMem.start;
	otCursor = (u32)db->otMem.cursor;

	if (rangeEnd < rangeStart)
	{
		return false;
	}

	return (rangeStart >= otStart) && (rangeEnd < otCursor);
}


static inline u32 CtrGpu_PackColorCode(u32 color, u32 code)
{
	return (color & 0xffffff) | (code << 24);
}

static inline void CtrGpu_WriteColorCode(u8 *r, u32 colorCode)
{
	r[0] = (u8)colorCode;
	r[1] = (u8)(colorCode >> 8);
	r[2] = (u8)(colorCode >> 16);
	r[3] = (u8)(colorCode >> 24);
}

// Storing a packed SXY word with no precision attached. Use
// CtrGpu_WritePackedXY for game code that projects through the GTE, and
// CtrGpu_CopyPackedXY when the value is read back out of another buffer.
static inline void CtrGpu_StorePackedXY(VERTTYPE *x, u32 xy)
{
	x[0] = (VERTTYPE)xy;
	x[1] = (VERTTYPE)(xy >> 16);
}

static inline void CtrGpu_WritePackedXY(VERTTYPE *x, u32 xy)
{
	CtrGpu_StorePackedXY(x, xy);
	// World effects write MFC2 results straight into packets; pair them with the
	// precise GTE vertex they came from (see native_pgxp.h). Outside the world
	// phase this clears the shadow instead, so HUD and menu quads never inherit
	// a world vertex that happens to share the same integer SXY.
	NativePgxp_BindWrittenXY(x, xy);
}

// Writes a packed SXY word read from `src`, carrying over the precise vertex that
// was recorded when the GTE stored it there.
static inline void CtrGpu_CopyPackedXY(VERTTYPE *x, const void *src, u32 xy)
{
	CtrGpu_StorePackedXY(x, xy);
	NativePgxp_CopyXY(x, src, xy);
}

static inline void CtrGpu_WritePackedUV(u8 *u, u16 uv)
{
	u[0] = (u8)uv;
	u[1] = (u8)(uv >> 8);
}

static inline void CtrGpu_WritePackedUVWord(u8 *u, u32 uvTpage)
{
	u[0] = (u8)uvTpage;
	u[1] = (u8)(uvTpage >> 8);
	u[2] = (u8)(uvTpage >> 16);
	u[3] = (u8)(uvTpage >> 24);
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

static inline void CtrGpu_LinkPacket24(u32 *ot, u32 *packetTag, const void *packet, u32 tag)
{
	*packetTag = CtrGpu_PackOTTag(*ot, tag);
	*ot = (u32)CtrGpu_PrimToOTLink24(packet);
}

static inline void CtrGpu_LinkPrimToOT(u32 *ot, const void *prim)
{
	*ot = (u32)CtrGpu_PrimToOTLink24(prim);
}

static inline void addPolyF3(u32 *ot, POLY_F3 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x4000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x20;
}

static inline void addPolyFT3(u32 *ot, POLY_FT3 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x7000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x24;
}

static inline void addPolyG3(u32 *ot, POLY_G3 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x6000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x30;
}

static inline void addPolyGT3(u32 *ot, POLY_GT3 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x9000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x34;
}

static inline void addPolyF4(u32 *ot, POLY_F4 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x5000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x28;
}

static inline void addPolyFT4(u32 *ot, POLY_FT4 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x9000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x2c;
}

static inline void addPolyG4(u32 *ot, POLY_G4 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x8000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x38;
}

static inline void addPolyGT4(u32 *ot, POLY_GT4 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0xc000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x3c;
}

static inline void addSprt8(u32 *ot, SPRT *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x3000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x74;
}

static inline void addSprt16(u32 *ot, SPRT *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x3000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x7c;
}

static inline void addSprt(u32 *ot, SPRT *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x4000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x64;
}

static inline void addTile1(u32 *ot, TILE *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x2000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x68;
}

static inline void addTile8(u32 *ot, TILE *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x2000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x70;
}

static inline void addTile16(u32 *ot, TILE *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x2000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x78;
}

static inline void addTile(u32 *ot, TILE *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x3000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x60;
}

static inline void addLineF2(u32 *ot, LINE_F2 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x3000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x40;
}

static inline void addLineG2(u32 *ot, LINE_G2 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x4000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x50;
}

static inline void addLineF3(u32 *ot, LINE_F3 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x5000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x48;
	p->pad = 0x55555555;
}

static inline void addLineG3(u32 *ot, LINE_G3 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x7000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x58;
	p->pad = 0x55555555;
	p->p1 = 0;
	p->p2 = 0;
}

static inline void addLineF4(u32 *ot, LINE_F4 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x6000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x4c;
	p->pad = 0x55555555;
}

static inline void addLineG4(u32 *ot, LINE_G4 *p)
{
	p->tag = CtrGpu_PackOTTag(*ot, 0x9000000);
	CtrGpu_LinkPrimToOT(ot, p);
	p->code = 0x5c;
	p->pad = 0x55555555;
	p->p1 = 0;
	p->p2 = 0;
}

// version of psn00bsdk's setXY4 macro that compiles to a smaller bytesize
// based on original compiled code for the game's primitive functions
// this produces bugs if any of the X values are negative and not cast to u16
// this is terrible code
// please avoid writing something like this, unless you really really need it
#define setXY4CompilerHack(p, s0, t0, s1, t1, s2, t2, s3, t3)                                                           \
	CtrGpu_WritePackedXY(&(p)->x0, (s0) | ((u32)(t0) << 16)), CtrGpu_WritePackedXY(&(p)->x1, (s1) | ((u32)(t1) << 16)), \
	    CtrGpu_WritePackedXY(&(p)->x2, (s2) | ((u32)(t2) << 16)), CtrGpu_WritePackedXY(&(p)->x3, (s3) | ((u32)(t3) << 16))

// like psn00bsdk's setColor macros but with terrible compiler hacks
// as the color values are read and written as 32-bit ints these have to be used prior to setting code
#define setInt32RGB0(p, color0) CtrGpu_WriteColorCode(&(p)->r0, (color0))

#define setInt32RGB4(p, color0, color1, color2, color3)                                                                              \
	CtrGpu_WriteColorCode(&(p)->r0, (color0)), CtrGpu_WriteColorCode(&(p)->r1, (color1)), CtrGpu_WriteColorCode(&(p)->r2, (color2)), \
	    CtrGpu_WriteColorCode(&(p)->r3, (color3))

// clear blending mode bits of the texpage using AND, then set them using OR
// then set image to use semi-transparent mode using the setSemiTrans macro
// (which enables the 2 bit on the primitive's code field)
#define setTransparency(p, transparency) p->tpage = (p->tpage & 0xff9f) | ((transparency - 1) << 5), p->code |= 2

// version of psn00bsdk's setColor macro that simultaneously accepts 4 colors
#define setColor4(p, rgb0, rgb1, rgb2, rgb3)                                                                                         \
	(((P_COLOR *)&((p)->r0))->color = (rgb0)), (((P_COLOR *)&((p)->r1))->color = (rgb1)), (((P_COLOR *)&((p)->r2))->color = (rgb2)), \
	    (((P_COLOR *)&((p)->r3))->color = (rgb3))

#endif
