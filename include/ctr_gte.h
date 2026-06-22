#ifndef CTR_GTE_H
#define CTR_GTE_H

#include <ctr_math.h>
#include <psx/inline_c.h>
#include <psx/libgte.h>

static inline void CTR_GteStoreU32(void *dst, u32 value)
{
	u8 *bytes = (u8 *)dst;

	bytes[0] = (u8)value;
	bytes[1] = (u8)(value >> 8);
	bytes[2] = (u8)(value >> 16);
	bytes[3] = (u8)(value >> 24);
}

static inline void CTR_GteLoadSV0(const SVECTOR *v)
{
	MTC2(CTR_PackS16Pair(v->vx, v->vy), 0);
	MTC2(CTR_PackS16Pair(v->vz, 0), 1);
}

static inline void CTR_GteLoadSV1(const SVECTOR *v)
{
	MTC2(CTR_PackS16Pair(v->vx, v->vy), 2);
	MTC2(CTR_PackS16Pair(v->vz, 0), 3);
}

static inline void CTR_GteLoadSV2(const SVECTOR *v)
{
	MTC2(CTR_PackS16Pair(v->vx, v->vy), 4);
	MTC2(CTR_PackS16Pair(v->vz, 0), 5);
}

static inline void CTR_GteLoadSV3(const SVECTOR *v0, const SVECTOR *v1, const SVECTOR *v2)
{
	CTR_GteLoadSV0(v0);
	CTR_GteLoadSV1(v1);
	CTR_GteLoadSV2(v2);
}

static inline void CTR_GteLoadSV3WithPad(const SVECTOR *v0, const SVECTOR *v1, const SVECTOR *v2)
{
	MTC2(CTR_PackS16Pair(v0->vx, v0->vy), 0);
	MTC2(CTR_PackS16Pair(v0->vz, v0->pad), 1);
	MTC2(CTR_PackS16Pair(v1->vx, v1->vy), 2);
	MTC2(CTR_PackS16Pair(v1->vz, v1->pad), 3);
	MTC2(CTR_PackS16Pair(v2->vx, v2->vy), 4);
	MTC2(CTR_PackS16Pair(v2->vz, v2->pad), 5);
}

static inline void CTR_GteLoadSVec3V0(const SVec3 *v)
{
	MTC2(CTR_PackS16Pair(v->x, v->y), 0);
	MTC2(CTR_PackS16Pair(v->z, 0), 1);
}

static inline void CTR_GteLoadSVec3V1(const SVec3 *v)
{
	MTC2(CTR_PackS16Pair(v->x, v->y), 2);
	MTC2(CTR_PackS16Pair(v->z, 0), 3);
}

static inline void CTR_GteLoadSVec3V2(const SVec3 *v)
{
	MTC2(CTR_PackS16Pair(v->x, v->y), 4);
	MTC2(CTR_PackS16Pair(v->z, 0), 5);
}

static inline void CTR_GteLoadSVec3V3(const SVec3 *v0, const SVec3 *v1, const SVec3 *v2)
{
	CTR_GteLoadSVec3V0(v0);
	CTR_GteLoadSVec3V1(v1);
	CTR_GteLoadSVec3V2(v2);
}

static inline void CTR_GteLoadRotRow0SVec3(const SVec3 *v)
{
	u32 r13r21 = (CFC2(1) & 0xffff0000U) | (u16)v->z;

	CTC2(CTR_PackS16Pair(v->x, v->y), 0);
	CTC2(r13r21, 1);
}

static inline void CTR_GteLoadSVec4V0(const SVec4 *v)
{
	MTC2(CTR_PackS16Pair(v->x, v->y), 0);
	MTC2(CTR_PackS16Pair(v->z, v->w), 1);
}

static inline void CTR_GteLoadS16TripletV0(const s16 *v)
{
	MTC2(CTR_PackS16Pair(v[0], v[1]), 0);
	MTC2(CTR_PackS16Pair(v[2], 0), 1);
}

static inline void CTR_GteLoadLVL(const s32 *v)
{
	MTC2((u32)v[0], 9);
	MTC2((u32)v[1], 10);
	MTC2((u32)v[2], 11);
}

static inline void CTR_GteStoreSXY(void *xy)
{
	CTR_GteStoreU32(xy, MFC2(14));
}

static inline void CTR_GteStoreSXY0(void *xy)
{
	CTR_GteStoreU32(xy, MFC2(12));
}

static inline void CTR_GteStoreSXY1(void *xy)
{
	CTR_GteStoreU32(xy, MFC2(13));
}

static inline void CTR_GteStoreSXY2(void *xy)
{
	CTR_GteStoreSXY(xy);
}

static inline void CTR_GteStoreSXY3(void *xy0, void *xy1, void *xy2)
{
	CTR_GteStoreU32(xy0, MFC2(12));
	CTR_GteStoreU32(xy1, MFC2(13));
	CTR_GteStoreU32(xy2, MFC2(14));
}

static inline s32 CTR_GteReadMAC1(void)
{
	return MFC2_S(25);
}

static inline void CTR_GteStoreMAC(s32 *out)
{
	out[0] = (s32)MFC2(25);
	out[1] = (s32)MFC2(26);
	out[2] = (s32)MFC2(27);
}

static inline void CTR_GteStoreIR(s32 *out)
{
	out[0] = (s32)MFC2(9);
	out[1] = (s32)MFC2(10);
	out[2] = (s32)MFC2(11);
}

static inline void CTR_GteStoreSV(SVECTOR *out)
{
	out->vx = (s16)MFC2(9);
	out->vy = (s16)MFC2(10);
	out->vz = (s16)MFC2(11);
}

static inline void CTR_GteStoreS16Triplet(s16 *out)
{
	out[0] = (s16)MFC2(9);
	out[1] = (s16)MFC2(10);
	out[2] = (s16)MFC2(11);
}

#endif
