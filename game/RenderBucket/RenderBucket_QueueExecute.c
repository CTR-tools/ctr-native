#include <common.h>
#include <ctr_gte_transfer.h>

#include "RenderBucket_ProjectedBounds.h"
#include "RenderBucket_ExecuteScratch.h"

struct RenderBucketEntry
{
	struct Instance *inst;
	struct Instance *instPlayerBase;
};

struct RenderBucketQueueState
{
	struct PushBuffer *lastPushBuffer;
	u32 *otBase;
	u32 *otCurr;
	u32 *otEndMinusOne;
};
CTR_STATIC_ASSERT(sizeof(struct RenderBucketEntry) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketEntry, inst) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketEntry, instPlayerBase) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct Item, next) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct Item, prev) == 0x4);
CTR_STATIC_ASSERT(sizeof(struct Item) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct CameraDC, visInstSrc) == 0x28);
CTR_STATIC_ASSERT(sizeof(struct CameraDC) == 0xdc);
CTR_STATIC_ASSERT(CTR_OFFSET_OF_2D_ARRAY(MATRIX, m, 0, 0) == 0x0);
CTR_STATIC_ASSERT(CTR_OFFSET_OF_2D_ARRAY(MATRIX, m, 0, 2) == 0x4);
CTR_STATIC_ASSERT(CTR_OFFSET_OF_2D_ARRAY(MATRIX, m, 1, 1) == 0x8);
CTR_STATIC_ASSERT(CTR_OFFSET_OF_2D_ARRAY(MATRIX, m, 2, 0) == 0xc);
CTR_STATIC_ASSERT(CTR_OFFSET_OF_2D_ARRAY(MATRIX, m, 2, 2) == 0x10);
CTR_STATIC_ASSERT(CTR_OFFSET_OF_ARRAY(MATRIX, t, 0) == 0x14);
CTR_STATIC_ASSERT(CTR_OFFSET_OF_ARRAY(MATRIX, t, 1) == 0x18);
CTR_STATIC_ASSERT(CTR_OFFSET_OF_ARRAY(MATRIX, t, 2) == 0x1c);
CTR_STATIC_ASSERT(sizeof(MATRIX) == 0x20);
CTR_STATIC_ASSERT(offsetof(struct ModelFrame, pos.x) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct ModelFrame, pos.y) == 0x2);
CTR_STATIC_ASSERT(offsetof(struct ModelFrame, pos.z) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct ModelFrame, vertexOffset) == 0x18);
CTR_STATIC_ASSERT(sizeof(struct ModelFrame) == 0x1c);
CTR_STATIC_ASSERT(offsetof(struct Thread, object) == 0x30);
CTR_STATIC_ASSERT(offsetof(struct Driver, driverID) == 0x4a);
CTR_STATIC_ASSERT(offsetof(struct Model, numHeaders) == 0x12);
CTR_STATIC_ASSERT(offsetof(struct Model, headers) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct ModelHeader, maxDistanceLOD) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct ModelHeader, flags) == 0x16);
CTR_STATIC_ASSERT(offsetof(struct ModelHeader, scale) == 0x18);
CTR_STATIC_ASSERT(offsetof(struct ModelHeader, ptrCommandList) == 0x20);
CTR_STATIC_ASSERT(offsetof(struct ModelHeader, ptrFrameData) == 0x24);
CTR_STATIC_ASSERT(offsetof(struct ModelHeader, ptrTexLayout) == 0x28);
CTR_STATIC_ASSERT(offsetof(struct ModelHeader, ptrColors) == 0x2c);
CTR_STATIC_ASSERT(offsetof(struct ModelHeader, unk3) == 0x30);
CTR_STATIC_ASSERT(offsetof(struct ModelHeader, numAnimations) == 0x34);
CTR_STATIC_ASSERT(offsetof(struct ModelHeader, ptrAnimations) == 0x38);
CTR_STATIC_ASSERT(sizeof(struct ModelHeader) == 0x40);
CTR_STATIC_ASSERT(offsetof(struct ModelAnim, numFrames) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct ModelAnim, frameSize) == 0x12);
CTR_STATIC_ASSERT(offsetof(struct ModelAnim, ptrDeltaArray) == 0x14);
CTR_STATIC_ASSERT(sizeof(struct ModelAnim) == 0x18);
CTR_STATIC_ASSERT(sizeof(u32) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, pos.x) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, pos.y) == 0x2);
CTR_STATIC_ASSERT(offsetof(struct PushBuffer, pos.z) == 0x4);

typedef struct
{
	u8 x;
	u8 y;
	u8 z;
} RenderBucketCompVertex;

typedef struct
{
	u8 x;
	u8 y;
	u8 z;
	u8 w;
} RenderBucketVertex;

struct RenderBucketPackedVertex
{
	u32 xy;
	u32 z;
};

enum
{
	RENDER_BUCKET_PAYLOAD_SCRATCH_OFFSET = 0x140,
};

CTR_STATIC_ASSERT(sizeof(struct RenderBucketPackedVertex) == 0x8);

struct RenderBucketUncompressResult
{
	struct RenderBucketPackedVertex packed;
	int color;
};

struct RenderBucketSplitVertex
{
	u32 xy;
	u32 z;
	u32 color;
	u16 uv;
	s16 splitDist;
	u32 sxy;
	u32 sz;
};

CTR_STATIC_ASSERT(sizeof(struct RenderBucketSplitVertex) == 0x18);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketSplitVertex, xy) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketSplitVertex, z) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketSplitVertex, color) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketSplitVertex, uv) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketSplitVertex, splitDist) == 0xe);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketSplitVertex, sxy) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketSplitVertex, sz) == 0x14);

struct RenderBucketGhostMaskPacket
{
	u32 tag;
	u32 drawMode;
	u32 pad;
	u32 colorAndCode;
	u32 xy0;
	u32 xy1;
	u32 xy2;
};

struct RenderBucketTaglessG3
{
	u32 color0AndCode;
	u32 xy0;
	u32 color1;
	u32 xy1;
	u32 color2;
	u32 xy2;
};

struct RenderBucketTaglessGT3
{
	u32 color0AndCode;
	u32 xy0;
	u32 uv0;
	u32 color1;
	u32 xy1;
	u32 uv1;
	u32 color2;
	u32 xy2;
	u32 uv2;
};

struct RenderBucketGhostFlatPacket
{
	struct RenderBucketGhostMaskPacket mask;
	u32 drawMode;
	u32 pad;
	struct RenderBucketTaglessG3 body;
};

struct RenderBucketGhostTexturedPacket
{
	struct RenderBucketGhostMaskPacket mask;
	struct RenderBucketTaglessGT3 body;
};

CTR_STATIC_ASSERT(sizeof(struct RenderBucketGhostMaskPacket) == 0x1C);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketGhostMaskPacket, tag) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketGhostMaskPacket, drawMode) == 0x04);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketGhostMaskPacket, pad) == 0x08);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketGhostMaskPacket, colorAndCode) == 0x0C);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketGhostMaskPacket, xy0) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketGhostMaskPacket, xy1) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketGhostMaskPacket, xy2) == 0x18);

CTR_STATIC_ASSERT(sizeof(struct RenderBucketTaglessG3) == 0x18);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketTaglessG3, color0AndCode) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketTaglessG3, xy0) == 0x04);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketTaglessG3, color1) == 0x08);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketTaglessG3, xy1) == 0x0C);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketTaglessG3, color2) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketTaglessG3, xy2) == 0x14);

CTR_STATIC_ASSERT(sizeof(struct RenderBucketTaglessGT3) == 0x24);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketTaglessGT3, color0AndCode) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketTaglessGT3, xy0) == 0x04);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketTaglessGT3, uv0) == 0x08);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketTaglessGT3, color1) == 0x0C);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketTaglessGT3, xy1) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketTaglessGT3, uv1) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketTaglessGT3, color2) == 0x18);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketTaglessGT3, xy2) == 0x1C);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketTaglessGT3, uv2) == 0x20);

CTR_STATIC_ASSERT(sizeof(struct RenderBucketGhostFlatPacket) == 0x3C);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketGhostFlatPacket, mask) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketGhostFlatPacket, drawMode) == 0x1C);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketGhostFlatPacket, pad) == 0x20);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketGhostFlatPacket, body) == 0x24);

CTR_STATIC_ASSERT(sizeof(struct RenderBucketGhostTexturedPacket) == 0x40);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketGhostTexturedPacket, mask) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct RenderBucketGhostTexturedPacket, body) == 0x1C);

#define RB_RETAIL_INST_FUNC3_SPLIT_WHITE  ((u32)0x8006d404U)
#define RB_RETAIL_INST_FUNC3_SPLIT_INTERP ((u32)0x8006d428U)

struct RenderBucketBounds
{
	int minX;
	int maxX;
	int minY;
	int maxY;
	int minZ;
	int maxZ;
};

struct RenderBucketSplitState
{
	int hasNextFrame;
	int scratch68;
	int scratch6c;
	int scratch70;
};

struct RenderBucketMatrixState
{
	int scratch74;
	int scratch76;
	int scratch78;
	u32 m0;
	u32 m1;
	u32 m2;
	u32 m3;
	u32 m4;
};

struct RenderBucketProjectedRegs
{
	u32 sxy0;
	u32 sz1;
	u32 sxy1;
	u32 sz2;
	u32 sxy2;
	u32 sz3;
};

struct RenderBucketDrawContext
{
	struct Instance *inst;
	struct InstDrawPerPlayer *idpp;
	struct PushBuffer *pb;
	struct PrimMem *primMem;
	struct ModelHeader *mh;
	struct ModelFrame *mf;
	struct ModelAnim *anim;
	char *vertData;
	char *nextVertData;
	RenderBucketVertex tempCoords[4];
	struct RenderBucketPackedVertex tempPacked[4];
	struct RenderBucketSplitVertex tempSplit[4];
	struct RenderBucketProjectedRegs specialMirrorRegs;
	int tempColor[4];
	RenderBucketVertex stack[256];
	struct RenderBucketPackedVertex packedStack[256];
	int bitIndex;
	int nextBitIndex;
	int x_alu;
	int y_alu;
	int z_alu;
	int x_next_alu;
	int y_next_alu;
	int z_next_alu;
	int stripLength;
	u32 primCommand;
	int vertexIndex;
	int splitPlane;
	int waterSplitSide;
};

enum
{
	RENDER_BUCKET_TEX_WORD0_OFFSET = 0,
	RENDER_BUCKET_TEX_WORD1_OFFSET = offsetof(struct TextureLayout, u1) - offsetof(struct TextureLayout, u0),
	RENDER_BUCKET_TEX_WORD2_OFFSET = offsetof(struct TextureLayout, u2) - offsetof(struct TextureLayout, u0),
};

static u32 RenderBucket_ReadPackedWord(const void *src)
{
	const u8 *bytes = (const u8 *)src;

	return (u32)bytes[0] | ((u32)bytes[1] << 8) | ((u32)bytes[2] << 16) | ((u32)bytes[3] << 24);
}

static void RenderBucket_WritePackedWord(void *dst, u32 value)
{
	u8 *bytes = (u8 *)dst;

	bytes[0] = (u8)value;
	bytes[1] = (u8)(value >> 8);
	bytes[2] = (u8)(value >> 16);
	bytes[3] = (u8)(value >> 24);
}

static u32 RenderBucket_ReadMatrixWord(const MATRIX *m, u32 offset)
{
	return RenderBucket_ReadPackedWord((const u8 *)m + offset);
}

static void RenderBucket_WriteMatrixWord(MATRIX *m, u32 offset, u32 value)
{
	RenderBucket_WritePackedWord((u8 *)m + offset, value);
}

static u32 RenderBucket_ReadTextureWord(const struct TextureLayout *tex, u32 offset)
{
	return RenderBucket_ReadPackedWord((const u8 *)&tex->u0 + offset);
}

static u32 *RenderBucket_OTSlotAtByteOffset(u32 *otBase, int byteOffset)
{
	return otBase + ((u32)byteOffset >> 2);
}

static u8 *RenderBucket_ModelAnimFirstFrameBytes(struct ModelAnim *anim)
{
	return (u8 *)anim + sizeof(struct ModelAnim);
}

static struct ModelFrame *RenderBucket_ModelFrameAtByteOffset(u8 *base, u32 byteOffset)
{
	return (struct ModelFrame *)(void *)(base + byteOffset);
}

static struct Instance *RenderBucket_InstancePlayerBase(struct Instance *inst, int playerIndex)
{
	return (struct Instance *)(void *)((u8 *)inst + (playerIndex * sizeof(struct InstDrawPerPlayer)));
}

static struct InstDrawPerPlayer *RenderBucket_InstancePlayerIdpp(struct Instance *instPlayerBase)
{
	return (struct InstDrawPerPlayer *)(void *)((u8 *)instPlayerBase + sizeof(struct Instance));
}

static int RenderBucket_SignExtendBits(u32 value, int bits)
{
	int shift = 32 - bits;
	return (s32)(value << shift) >> shift;
}

static int RenderBucket_GetSignedBits(const u8 *vertData, int *bitIndex, int bits)
{
	int const b = *bitIndex >> 5;
	int const e = 32 - bits;
	int const s = e - (*bitIndex & 31);
	u32 const word = RenderBucket_ReadPackedWord(vertData + (b * sizeof(u32)));
	u32 const ret = s < 0 ? (word << -s) | (RenderBucket_ReadPackedWord(vertData + ((b + 1) * sizeof(u32))) >> (s & 31)) : word >> s;

	// NOTE(aalhendi): Source-backs the retail s1/s3 MSB-first bitstream reader
	// at 0x8006a92c-0x8006aa30. The stream uses little-endian 32-bit loads, then
	// sign-extends each requested field through arithmetic shift.
	*bitIndex += bits;
	return RenderBucket_SignExtendBits(ret, bits);
}

static int RenderBucket_SignedByte(int value)
{
	return (s8)value;
}

static u32 RenderBucket_PackXY(int x, int y);

static inline int RenderBucket_MipsSllSigned(int value, int shift)
{
	return (s32)((u32)value << shift);
}

static inline int RenderBucket_MipsMulLoSra16(int lhs, int rhs)
{
	return (s32)(u32)((s64)lhs * (s64)rhs) >> 16;
}

static inline int RenderBucket_MipsMulLoSra12(int lhs, int rhs)
{
	return (s32)(u32)((s64)lhs * (s64)rhs) >> 12;
}

static inline u32 RenderBucket_MipsMultuLo(u32 lhs, u32 rhs)
{
	return (u32)((u64)lhs * (u64)rhs);
}

static inline u8 RenderBucket_InterpU8(u8 from, u8 to, int factor)
{
	int delta = (int)to - (int)from;
	return (u8)(RenderBucket_MipsMulLoSra16(delta, factor) + from);
}

static inline s16 RenderBucket_InterpS16(s16 from, s16 to, int factor)
{
	int delta = (int)to - (int)from;
	return (s16)(RenderBucket_MipsMulLoSra16(delta, factor) + from);
}

static inline void RenderBucket_SplitInterpolateVertex(struct RenderBucketSplitVertex *dst, const struct RenderBucketSplitVertex *from,
                                                       const struct RenderBucketSplitVertex *to, int hasTexture)
{
	int denom = (int)to->z - (int)from->z;
	int factor = RenderBucket_MipsSllSigned(from->splitDist, 16) / denom;

	// NOTE(aalhendi): Source-backs helper 0x8006b82c. Intersections live on the
	// split plane; callers prefill dst->z, matching retail scratchpad 0xdc/0xf4.
	dst->splitDist = 0;
	if (hasTexture != 0)
	{
		u8 u = RenderBucket_InterpU8((u8)from->uv, (u8)to->uv, factor);
		u8 v = RenderBucket_InterpU8((u8)(from->uv >> 8), (u8)(to->uv >> 8), factor);
		dst->uv = (u16)u | ((u16)v << 8);
	}

	dst->xy = RenderBucket_PackXY(RenderBucket_InterpS16((s16)from->xy, (s16)to->xy, factor),
	                              RenderBucket_InterpS16((s16)(from->xy >> 16), (s16)(to->xy >> 16), factor));
	dst->color = (u32)RenderBucket_InterpU8((u8)from->color, (u8)to->color, factor) |
	             ((u32)RenderBucket_InterpU8((u8)(from->color >> 8), (u8)(to->color >> 8), factor) << 8) |
	             ((u32)RenderBucket_InterpU8((u8)(from->color >> 16), (u8)(to->color >> 16), factor) << 16);
}

static inline void RenderBucket_WaterSplitInterpolateVertex(struct RenderBucketDrawContext *ctx, struct RenderBucketSplitVertex *dst,
                                                            const struct RenderBucketSplitVertex *from, const struct RenderBucketSplitVertex *to,
                                                            int hasTexture, s16 splitLine)
{
	int denom = (s16)(to->xy >> 16) - (s16)(from->xy >> 16);
	int factor = RenderBucket_MipsSllSigned(from->splitDist, 16) / denom;
	u32 colorHelper = (u32)(u32)ctx->inst->funcPtr[3];

	// This split clips against model-space Y, not the depth plane used by 0x8006b4c8.
	if (hasTexture != 0)
	{
		u8 u = RenderBucket_InterpU8((u8)from->uv, (u8)to->uv, factor);
		u8 v = RenderBucket_InterpU8((u8)(from->uv >> 8), (u8)(to->uv >> 8), factor);
		dst->uv = (u16)u | ((u16)v << 8);
	}

	dst->xy = RenderBucket_PackXY(RenderBucket_InterpS16((s16)from->xy, (s16)to->xy, factor), splitLine);
	dst->z = (u32)(RenderBucket_MipsMulLoSra16((s32)to->z - (s32)from->z, factor) + (s32)from->z);
	if (colorHelper == RB_RETAIL_INST_FUNC3_SPLIT_WHITE)
	{
		dst->color = 0x00ffffff;
	}
	else
	{
		// The retail table uses this non-white generated-vertex color path.
		dst->color = (u32)RenderBucket_InterpU8((u8)from->color, (u8)to->color, factor) |
		             ((u32)RenderBucket_InterpU8((u8)(from->color >> 8), (u8)(to->color >> 8), factor) << 8) |
		             ((u32)RenderBucket_InterpU8((u8)(from->color >> 16), (u8)(to->color >> 16), factor) << 16);
	}
	dst->splitDist = 0;
}

static struct ModelAnim *RenderBucket_GetAnim(struct Instance *inst, struct ModelHeader *mh)
{
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail indexes this table directly when ptrAnimations is
	// non-null; native keeps malformed/incomplete model data from trapping.
	if (mh->numAnimations == 0)
	{
		return 0;
	}

	if (inst->animIndex >= mh->numAnimations)
	{
		return 0;
	}
#endif

	return mh->ptrAnimations[inst->animIndex];
}

static u32 RenderBucket_PackXY(int x, int y)
{
	return ((u32)(u16)x) | ((u32)(u16)y << 16);
}

static struct RenderBucketExecuteScratch *RenderBucket_Scratch(void)
{
	return CTR_SCRATCHPAD_PTR(struct RenderBucketExecuteScratch, 0);
}

static struct RenderBucketPackedVertex *RenderBucket_PackedVertexScratch(u16 stackIndex)
{
	return CTR_SCRATCHPAD_PTR(struct RenderBucketPackedVertex, RENDER_BUCKET_PAYLOAD_SCRATCH_OFFSET + (stackIndex * sizeof(struct RenderBucketPackedVertex)));
}

static u32 *RenderBucket_ColorCacheScratch(void)
{
	return CTR_SCRATCHPAD_PTR(u32, RENDER_BUCKET_PAYLOAD_SCRATCH_OFFSET);
}

static int RenderBucket_SignExtendByte(u8 value)
{
	return ((value & 0x80) != 0) ? (int)value - 0x100 : value;
}

static void RenderBucket_StoreRawViewScratch(const VECTOR *rawViewPos)
{
	SVec3Slot *rawView = &RenderBucket_Scratch()->rawView;

	// NOTE(aalhendi): Retail 0x80070a40-0x80070a4c writes these as halfwords,
	// and the later compressed-normal test reads words from 0xb4/0xb8. The high
	// halfword at 0xba is intentionally left with scratchpad lifetime.
	rawView->x = rawViewPos->vx;
	rawView->y = rawViewPos->vy;
	rawView->z = rawViewPos->vz;
}

static void RenderBucket_AdjustDepthBiasForNormal(struct Instance *inst, int playerIndex, int *normalBias, int *reflectBias)
{
	SVec3Slot *rawView;
	u32 compressed = inst->compressedNormalAndDriverIndex;
	int driverIndex;
	int normalX;
	int normalY;
	int normalZ;

	if (compressed == 0)
	{
		return;
	}

	driverIndex = compressed >> 24;
	if ((playerIndex + 1) == driverIndex)
	{
		return;
	}

	normalX = (s8)compressed;
	normalY = (s8)(compressed >> 8);
	normalZ = (s8)(compressed >> 16);

	// NOTE(aalhendi): Source-backs QueueDraw 0x80070c2c-0x80070c9c. Retail
	// tests the compressed ground normal against the pre-view-transform vector
	// and biases both OT ranges by +10 when MAC2 is positive.
	rawView = &RenderBucket_Scratch()->rawView;
	CTC2((u16)normalX | ((u32)(u16)normalY << 16), 16);
	CTC2(normalZ, 17);
	MTC2(CTR_PackS16Pair(rawView->x, rawView->y), 0);
	MTC2(CTR_PackS16Pair(rawView->z, rawView->w), 1);
	doCOP2(0x04c6012);

	if ((s32)MFC2(25) > 0)
	{
		*normalBias += 10;
		*reflectBias += 10;
	}
}

static u32 RenderBucket_OTAddress(void *ptr)
{
	return CtrGpu_PrimToOTLink24(ptr);
}

static void RenderBucket_GteLoadLightMatrixWords(const MATRIX *m);
static void RenderBucket_StoreViewMatrixForSplit(struct InstDrawPerPlayer *idpp);

static void RenderBucket_LinkPrimRaw(u32 *otEntry, void *prim, u32 lenWord)
{
	// NOTE(aalhendi): Source-backs DrawInstPrim_Normal's retail OT tag write at
	// 0x8006ae50-0x8006ae64.
	RenderBucket_WritePackedWord(prim, CtrGpu_PackOTTag(*otEntry, lenWord));
	*otEntry = RenderBucket_OTAddress(prim);
}

static const u32 sRenderBucketDispatchTable8008a428[8] = {
    0x8006c948, 0x8006c974, 0x8006c928, 0x8006c928, 0x8006c948, 0x8006c948, 0x8006c984, 0x8006ad88,
};
CTR_STATIC_ASSERT(sizeof(sRenderBucketDispatchTable8008a428) == 0x20);

static const u32 sRenderBucketDispatchTable8008a444[8] = {
    0x8006ad88, 0x8006b968, 0x8006ae90, 0x8006c778, 0x8006bad0, 0x8006ad6c, 0x8006d670, 0x8006d55c,
};
CTR_STATIC_ASSERT(sizeof(sRenderBucketDispatchTable8008a444) == 0x20);

static const u32 sRenderBucketInstanceFunc2Table8008a460[4] = {
    0x8006d55c,
    0x8006d588,
    0x8006d5b8,
    0x8006d59c,
};
CTR_STATIC_ASSERT(sizeof(sRenderBucketInstanceFunc2Table8008a460) == 0x10);

static const u32 sRenderBucketInstanceFunc3Table8008a470[4] = {
    0x8006d428,
    0x8006d428,
    0x8006d404,
    0x8006d428,
};
CTR_STATIC_ASSERT(sizeof(sRenderBucketInstanceFunc3Table8008a470) == 0x10);

static const u8 sRenderBucketKeyRelicBrightness8008a2c4[0x80] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x08, 0x09, 0x09, 0x0a, 0x0b, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x16, 0x17, 0x19, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 0x27, 0x29, 0x2c, 0x2f, 0x32, 0x35, 0x38, 0x3c,
    0x40, 0x44, 0x48, 0x4c, 0x51, 0x56, 0x5c, 0x62, 0x68, 0x6e, 0x75, 0x7c, 0x84, 0x8c, 0x95, 0x9e, 0xa8, 0xb3, 0xbe, 0xc9, 0xd5, 0xe3, 0xf0, 0xff,
};

#define RB_RETAIL_INST_SETUP_LIGHT_COLOR     ((u32)0x8006c928U)
#define RB_RETAIL_INST_SETUP_COLOR           ((u32)0x8006c948U)
#define RB_RETAIL_INST_SETUP_ZERO_COLOR      ((u32)0x8006c974U)
#define RB_RETAIL_INST_SETUP_FADE_COLOR      ((u32)0x8006c984U)
#define RB_RETAIL_INST_PRIM_SELECT_RANGE     ((u32)0x8006ad6cU)
#define RB_RETAIL_INST_PRIM_NORMAL           ((u32)0x8006ad88U)
#define RB_RETAIL_INST_PRIM_DEPTH_FADE       ((u32)0x8006b968U)
#define RB_RETAIL_INST_PRIM_KEY_TOKEN        ((u32)0x8006ae90U)
#define RB_RETAIL_INST_PRIM_CLAMP_DEPTH      ((u32)0x8006bad0U)
#define RB_RETAIL_INST_PRIM_LIT_TEXTURE      ((u32)0x8006c778U)
#define RB_RETAIL_INST_PRIM_GHOST            ((u32)0x8006d670U)
#define RB_RETAIL_INST_FUNC2_SPLIT_BOTH_MASK ((u32)0x8006d55cU)
#define RB_RETAIL_INST_FUNC2_SPLIT_NEGATIVE  ((u32)0x8006d588U)
#define RB_RETAIL_INST_FUNC2_SPLIT_XOR       ((u32)0x8006d59cU)
#define RB_RETAIL_INST_FUNC2_SPLIT_DIM_XOR   ((u32)0x8006d5b8U)

#define RB_RETAIL_DRAWFUNC_NORMAL            ((int)0x8006a52cU)
#define RB_RETAIL_DRAWFUNC_NORMAL_ALT        ((int)0x8006a6b8U)
#define RB_RETAIL_DRAWFUNC_SPLIT             ((int)0x8006b030U)
#define RB_RETAIL_DRAWFUNC_SPECIAL           ((int)0x8006bbc0U)
#define RB_RETAIL_DRAWFUNC_REFLECTION        ((int)0x8006c9c4U)
#define RB_RETAIL_UNCOMPRESS_NORMAL          ((int)0x8006a8e0U)
#define RB_RETAIL_UNCOMPRESS_NEXTFRAME       ((int)0x8006b24cU)
#define RB_RETAIL_UNCOMPRESS_SPLIT           ((int)0x8006bf30U)
#define RB_RETAIL_UNCOMPRESS_REFLECT         ((int)0x8006cdecU)
#define RB_INSTANCE_SKIP_OT_RANGE            0x2000000U
#define RB_MODEL_ALWAYS_POINT_NORTH          0x1U
CTR_STATIC_ASSERT(VISIBLE_DURING_GAMEPLAY == RB_INSTANCE_SKIP_OT_RANGE);

#include "RenderBucket_CopyDispatchTables.c"

#ifndef CTR_NATIVE
// NOTE(aalhendi): Keep the typed C provider apart from the scratch-register entry.
#define RenderBucket_CopyDispatchTables RenderBucket_CopyDispatchTables_Context
#endif

static void RenderBucket_CopyDispatchTables(void)
{
	struct RenderBucketExecuteScratch *scratch = RenderBucket_Scratch();

	RenderBucket_CopyDispatchWindow(sRenderBucketDispatchTable8008a428, scratch->setupDispatch);
	RenderBucket_CopyDispatchWindow(sRenderBucketDispatchTable8008a444, scratch->primitiveState.primDispatch);
}

static void RenderBucket_WriteInstanceCallbackLabels(struct Instance *inst, u32 instFlags)
{
	struct RenderBucketExecuteScratch *scratch = RenderBucket_Scratch();
	u32 *setupTable = scratch->setupDispatch;
	u32 *primTable = scratch->primitiveState.primDispatch;
	int func01Index = ((instFlags >> 14) & 0x1c) >> 2;
	int func23Index = ((instFlags >> 18) & 0x0c) >> 2;

	// NOTE(aalhendi): Source-backs QueueDraw's retail Instance+0x5c/0x60 and
	// Instance+0x64/0x68 label stores at 0x800714b0-0x800714f8. These are retail
	// labels, not native host-callable pointers.
	inst->funcPtr[2] = (void *)(u32)sRenderBucketInstanceFunc2Table8008a460[func23Index];
	inst->funcPtr[3] = (void *)(u32)sRenderBucketInstanceFunc3Table8008a470[func23Index];
	inst->funcPtr[0] = (void *)(u32)setupTable[func01Index];
	inst->funcPtr[1] = (void *)(u32)primTable[func01Index];
}

static struct RenderBucketSplitState RenderBucket_InitSplitState(const struct ModelFrame *nextFrame)
{
	struct RenderBucketSplitState state = {0};

	state.hasNextFrame = nextFrame != 0;
	return state;
}

static s32 RenderBucket_MipsSll(int value, int shift)
{
	return (s32)((u32)value << shift);
}

static s32 RenderBucket_MipsSub(int lhs, int rhs)
{
	return (s32)((u32)lhs - (u32)rhs);
}

static s32 RenderBucket_MipsAdd(int lhs, int rhs)
{
	return (s32)((u32)lhs + (u32)rhs);
}

static int RenderBucket_AddressSub(const void *lhs, const void *rhs)
{
	return RenderBucket_MipsSub((int)(u32)(u32)lhs, (int)(u32)(u32)rhs);
}

static int RenderBucket_AddressSubOffset(const void *lhs, int rhs)
{
	return RenderBucket_MipsSub((int)(u32)(u32)lhs, rhs);
}

static int RenderBucket_HasSplitOutput(const struct RenderBucketSplitState *split)
{
	return (split->scratch68 | split->scratch6c) != 0;
}

static void RenderBucket_SelectRetailDefaultHandlers(u32 instFlags, const struct RenderBucketSplitState *split, int *drawFunc, int *uncompressFunc)
{
	if (((instFlags & 0x1000) != 0) && (split->scratch70 != 0))
	{
		*drawFunc = RB_RETAIL_DRAWFUNC_NORMAL_ALT;
		*uncompressFunc = (split->hasNextFrame != 0) ? RB_RETAIL_UNCOMPRESS_REFLECT : RB_RETAIL_UNCOMPRESS_SPLIT;
		return;
	}

	*drawFunc = RB_RETAIL_DRAWFUNC_NORMAL;
	*uncompressFunc = (split->hasNextFrame != 0) ? RB_RETAIL_UNCOMPRESS_NEXTFRAME : RB_RETAIL_UNCOMPRESS_NORMAL;
}

static void RenderBucket_SelectRetailHandlers(u32 *instFlags, const struct RenderBucketSplitState *split, int *drawFuncOut, int *uncompressFuncOut)
{
	int drawFunc;
	int uncompressFunc;

	if ((*instFlags & (SPLIT_LINE | REFLECTIVE)) != 0)
	{
		if (RenderBucket_HasSplitOutput(split) != 0)
		{
			if (split->scratch6c != 0)
			{
				drawFunc = ((*instFlags & REFLECTIVE) != 0) ? RB_RETAIL_DRAWFUNC_REFLECTION : RB_RETAIL_DRAWFUNC_SPLIT;
				if ((*instFlags & REFLECTIVE) != 0)
				{
					*instFlags |= REFLECTION_FUNC23;
				}
				uncompressFunc = (split->hasNextFrame != 0) ? RB_RETAIL_UNCOMPRESS_REFLECT : RB_RETAIL_UNCOMPRESS_SPLIT;
			}
			else if (split->scratch68 < 0)
			{
				if ((*instFlags & REFLECTIVE) != 0)
				{
					drawFunc = RB_RETAIL_DRAWFUNC_SPECIAL;
					uncompressFunc = (split->hasNextFrame != 0) ? RB_RETAIL_UNCOMPRESS_NEXTFRAME : RB_RETAIL_UNCOMPRESS_NORMAL;
				}
				else
				{
					RenderBucket_SelectRetailDefaultHandlers(*instFlags, split, &drawFunc, &uncompressFunc);
				}
			}
			else
			{
				drawFunc = ((*instFlags & REFLECTIVE) != 0) ? RB_RETAIL_DRAWFUNC_REFLECTION : RB_RETAIL_DRAWFUNC_SPLIT;
				if ((*instFlags & REFLECTIVE) != 0)
				{
					*instFlags |= REFLECTION_FUNC23;
				}
				uncompressFunc = (split->hasNextFrame != 0) ? RB_RETAIL_UNCOMPRESS_NEXTFRAME : RB_RETAIL_UNCOMPRESS_NORMAL;
			}
		}
		else
		{
			drawFunc = RB_RETAIL_DRAWFUNC_SPECIAL;
			uncompressFunc = (split->hasNextFrame != 0) ? RB_RETAIL_UNCOMPRESS_NEXTFRAME : RB_RETAIL_UNCOMPRESS_NORMAL;
		}
	}
	else
	{
		RenderBucket_SelectRetailDefaultHandlers(*instFlags, split, &drawFunc, &uncompressFunc);
	}

	// NOTE(aalhendi): Source-backs QueueDraw's retail selector branch tree at
	// 0x800713b8-0x80071498. The successful epilogue owns the field stores after
	// the render-bucket entry is queued and DRAW_SUCCESSFUL is set.
	*drawFuncOut = drawFunc;
	*uncompressFuncOut = uncompressFunc;
}

static int RenderBucket_ClampOTByteOffset(int depthBin)
{
	int byteOffset = RenderBucket_MipsSll(depthBin, 2);

	if (depthBin < 0)
	{
		return 0;
	}

	if (RenderBucket_MipsSub(byteOffset, 0xffc) > 0)
	{
		return 0xffc;
	}

	return byteOffset;
}

static int RenderBucket_WriteAlphaScale(struct Instance *inst, struct InstDrawPerPlayer *idpp, int viewDepth, u32 instFlags)
{
	int alpha = (u16)inst->alphaScale;

	// NOTE(aalhendi): Source-backs QueueDraw's 0x80070a5c depth-fade alpha gate.
	if ((instFlags & DEPTH_FADE) != 0)
	{
		int depthFade = RenderBucket_MipsSll(viewDepth, 1);
		int firstReject = RenderBucket_MipsSub(depthFade, 0x1000);

#ifndef CTR_NATIVE
		// NOTE(aalhendi): Retail 0x80070a78 writes this in the first reject
		// branch delay slot; native cannot safely write absolute address 0x24.
		*(volatile u32 *)0x24 = 0;
#endif

		if (firstReject > 0)
		{
			return 0;
		}

		alpha = (int)(u32)((u32)alpha + (u32)depthFade);
		if (RenderBucket_MipsSub(alpha, 0x1000) > 0)
		{
			idpp->alphaScale = (s16)alpha;
			return 0;
		}
	}

	idpp->alphaScale = (s16)alpha;
	return 1;
}

static void RenderBucket_ApplyOwnerPushBufferGate(struct Instance *inst, int playerIndex, u32 *instFlags)
{
	struct Thread *thread;
	struct Driver *driver;

	if ((*instFlags & OWNER_PUSHBUFFER_GATE) == 0)
	{
		return;
	}

	thread = inst->thread;
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail dereferences this owner path directly; native keeps
	// the host stable for malformed/incomplete instance ownership.
	if (thread == 0 || thread->object == 0)
	{
		return;
	}
#endif

	driver = (struct Driver *)thread->object;

	// NOTE(aalhendi): Source-backs QueueDraw's 0x800709d4 owner PB clear gate.
	if ((s8)driver->driverID == playerIndex)
	{
		*instFlags &= ~PUSHBUFFER_EXISTS;
	}
}

static int RenderBucket_PackedSxyX(u32 sxy)
{
	return (s16)sxy;
}

static int RenderBucket_PackedSxyY(u32 sxy)
{
	return (s16)(sxy >> 16);
}

static void RenderBucket_BoundsInit(struct RenderBucketBounds *bounds, u32 sxy, int depth)
{
	int x = RenderBucket_PackedSxyX(sxy);
	int y = RenderBucket_PackedSxyY(sxy);

	bounds->minX = x;
	bounds->maxX = x;
	bounds->minY = y;
	bounds->maxY = y;
	bounds->minZ = depth;
	bounds->maxZ = depth;
}

static void RenderBucket_BoundsUpdate_80071524(struct RenderBucketBounds *bounds, u32 sxy, int depth)
{
	RENDER_BUCKET_EXPAND_PROJECTED_BOUNDS(sxy, depth, bounds->minX, bounds->maxX, bounds->minY, bounds->maxY, bounds->minZ, bounds->maxZ);
}

static void RenderBucket_ProjectBoundsUpdate3(struct RenderBucketBounds *bounds, int skipFirstPoint)
{
	int sxy0 = MFC2(12);
	int depth0 = MFC2(17);
	int sxy1 = MFC2(13);
	int depth1 = MFC2(18);
	int sxy2 = MFC2(14);
	int depth2 = MFC2(19);

	if (skipFirstPoint == 0)
	{
		RenderBucket_BoundsUpdate_80071524(bounds, sxy0, depth0);
	}
	RenderBucket_BoundsUpdate_80071524(bounds, sxy1, depth1);
	RenderBucket_BoundsUpdate_80071524(bounds, sxy2, depth2);
}

static u32 RenderBucket_PackedFrameXY(struct ModelFrame *frame, struct ModelFrame *nextFrame)
{
	u32 packedXY = RenderBucket_ReadPackedWord(&frame->pos.x);
	int blendedY;
	int blendedX;

	if (nextFrame == 0)
	{
		return packedXY;
	}

	// NOTE(aalhendi): Retail QueueDraw averages current/next frame bounds at
	// 0x80070db4-0x80070df0 before the 0x3fc bbox projection.
	blendedY = RenderBucket_MipsSll(RenderBucket_MipsAdd(nextFrame->pos.y, (s32)packedXY >> 16) >> 2, 17);
	blendedX = (RenderBucket_MipsAdd(nextFrame->pos.x, (s16)packedXY) >> 1) & 0xffff;
	return blendedY | blendedX;
}

static int RenderBucket_FrameZ(struct ModelFrame *frame, struct ModelFrame *nextFrame)
{
	if (nextFrame == 0)
	{
		return frame->pos.z;
	}

	return RenderBucket_MipsAdd(frame->pos.z, nextFrame->pos.z) >> 1;
}

static void RenderBucket_ProjectFrameBounds(struct ModelFrame *frame, struct ModelFrame *nextFrame, struct PushBuffer *pb, const MATRIX *projectionMvp,
                                            struct RenderBucketBounds *bounds, int *geomScreenHalf)
{
	u32 minXY = RenderBucket_MipsSll(RenderBucket_PackedFrameXY(frame, nextFrame), 2) & 0xfff8ffff;
	u32 minZ = RenderBucket_MipsSll(RenderBucket_FrameZ(frame, nextFrame), 2);
	u32 maxXMinY = RenderBucket_MipsAdd(minXY, 0x3fc) & 0xfff8ffff;
	u32 maxXMaxY = RenderBucket_MipsAdd(maxXMinY, 0x03fc0000);
	u32 minXMaxY = RenderBucket_MipsAdd(minXY, 0x03fc0000);
	u32 maxZ = RenderBucket_MipsAdd(minZ, 0x3fc);

	// NOTE(aalhendi): Retail projects the bbox through packed GTE VXY/VZ
	// registers at 0x80071054-0x80071164 and folds min/max through helper
	// 0x80071524. Native keeps the register tuple as explicit bounds fields.
	gte_SetRotMatrix(projectionMvp);
	gte_SetTransMatrix(projectionMvp);
	gte_SetGeomOffset(pb->rect.w >> 1, pb->rect.h >> 1);
	gte_SetGeomScreen(pb->distanceToScreen_PREV);

	MTC2(minXY, 0);
	MTC2(minZ, 1);
	MTC2(maxXMinY, 2);
	MTC2(minZ, 3);
	MTC2(maxXMaxY, 4);
	MTC2(minZ, 5);
	gte_rtpt();
	RenderBucket_BoundsInit(bounds, MFC2(12), MFC2(17));
	RenderBucket_ProjectBoundsUpdate3(bounds, 1);

	MTC2(maxZ, 1);
	MTC2(maxZ, 3);
	MTC2(maxZ, 5);
	gte_rtpt();
	RenderBucket_ProjectBoundsUpdate3(bounds, 0);

	MTC2(minXMaxY, 0);
	MTC2(minZ, 1);
	MTC2(minXMaxY, 2);
	MTC2(0, 4);
	MTC2(0, 5);
	gte_rtpt();
	RenderBucket_BoundsUpdate_80071524(bounds, MFC2(12), MFC2(17));
	*geomScreenHalf = (int)CFC2(26) >> 1;
	RenderBucket_BoundsUpdate_80071524(bounds, MFC2(13), MFC2(18));
}

static int RenderBucket_AcceptProjectedBounds(const struct RenderBucketBounds *bounds, struct PushBuffer *pb, int geomScreenHalf)
{
	// NOTE(aalhendi): Retail performs these viewport/depth rejects only after the
	// PUSHBUFFER_EXISTS metadata path at 0x80071164-0x800711c8.
	if (bounds->maxX < 0)
	{
		return 0;
	}

	if (bounds->maxY < 0)
	{
		return 0;
	}

	if (RenderBucket_MipsSub(bounds->maxZ, geomScreenHalf) < 0)
	{
		return 0;
	}

	if (RenderBucket_MipsSub(bounds->minX, (u16)pb->rect.w) > 0)
	{
		return 0;
	}

	if (RenderBucket_MipsSub(bounds->minY, (u16)pb->rect.h) > 0)
	{
		return 0;
	}

	return 1;
}

static void RenderBucket_GetViewPosition(struct Instance *inst, struct PushBuffer *pb, VECTOR *viewPos)
{
	VECTOR pos;

	if ((inst->flags & SCREENSPACE_INSTANCE) != 0)
	{
		viewPos->vx = inst->matrix.t[0];
		viewPos->vy = inst->matrix.t[1];
		viewPos->vz = inst->matrix.t[2];
		return;
	}

	pos.vx = RenderBucket_MipsSub(inst->matrix.t[0], pb->pos.x);
	pos.vy = RenderBucket_MipsSub(inst->matrix.t[1], pb->pos.y);
	pos.vz = RenderBucket_MipsSub(inst->matrix.t[2], pb->pos.z);
	RenderBucket_StoreRawViewScratch(&pos);

	// NOTE(aalhendi): Source-backs QueueDraw's view-space position transform at
	// 0x80070a1c-0x80070ae0. Retail routes the camera-relative vector through the
	// PushBuffer view matrix loaded as the GTE light matrix.
	RenderBucket_GteLoadLightMatrixWords(&pb->matrix_ViewProj);
	MTC2(pos.vx, 9);
	MTC2(pos.vy, 10);
	MTC2(pos.vz, 11);
	doCOP2(0x04be012);
	viewPos->vx = MFC2(9);
	viewPos->vy = MFC2(10);
	viewPos->vz = MFC2(11);
}

static void RenderBucket_AdjustViewPositionForMvp(struct Instance *inst, VECTOR *viewPos)
{
	// NOTE(aalhendi): Retail stores raw depth for LOD, then adjusts the MVP
	// translation at 0x80070a8c-0x80070ac8. Near instances are shifted left by 2
	// to match the full-scale model matrix; DRAW_HUGE shifts that translation
	// back down.
	if ((s32)((u32)viewPos->vz - 0x1000u) < 0)
	{
		viewPos->vx = RenderBucket_MipsSll(viewPos->vx, 2);
		viewPos->vy = RenderBucket_MipsSll(viewPos->vy, 2);
		viewPos->vz = RenderBucket_MipsSll(viewPos->vz, 2);
	}

	if ((inst->flags & DRAW_HUGE) != 0)
	{
		viewPos->vx >>= 2;
		viewPos->vy >>= 2;
		viewPos->vz >>= 2;
	}
}

static void RenderBucket_StoreMvpTranslation(struct InstDrawPerPlayer *idpp, const VECTOR *viewPos)
{
	// NOTE(aalhendi): Retail writes IDPP MVP translation and GTE TRX/TRY/TRZ
	// before LOD selection, so exhausted-LOD exits keep this side effect.
	idpp->mvp.t[0] = viewPos->vx;
	idpp->mvp.t[1] = viewPos->vy;
	idpp->mvp.t[2] = viewPos->vz;
	CTC2(viewPos->vx, 5);
	CTC2(viewPos->vy, 6);
	CTC2(viewPos->vz, 7);
}

static struct ModelHeader *RenderBucket_SelectModelHeader(struct Instance *inst, struct PushBuffer *pb, int *lodIndexOut, int *lodExhaustedOut, int viewDepth)
{
	struct ModelHeader *mh;
	int headersRemaining;
	int lodIndex;
	int projectedDistance;

	*lodIndexOut = 0;
	*lodExhaustedOut = 0;

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail trusts ModelHeader count and will walk raw model
	// data; native keeps malformed host-side models from trapping.
	if (inst->model->numHeaders <= 0)
	{
		return 0;
	}
#endif

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail reaches a raw div by GTE H here; native guards the
	// host divide-by-zero case.
	if (pb->distanceToScreen_PREV == 0)
	{
		return 0;
	}
#endif

	// NOTE(aalhendi): Retail keeps the low 32 bits of this product before dividing by GTE H.
	projectedDistance = (int)(u32)((s64)(pb->rect.w >> 1) * viewDepth) / pb->distanceToScreen_PREV;
	mh = inst->model->headers;
	headersRemaining = inst->model->numHeaders;
	lodIndex = 0;

	// NOTE(aalhendi): Retail 0x80070ae4-0x80070b34 uses transformed GTE depth,
	// GTE H, and a post-decrement s4/s5 header walk. Native keeps the same
	// comparison and walk as explicit C state.
	for (;;)
	{
		if (RenderBucket_MipsSub(projectedDistance, (u16)mh->maxDistanceLOD) < 0)
		{
			*lodIndexOut = lodIndex;
			return mh;
		}

		lodIndex++;
		headersRemaining--;
		mh++;
		if (headersRemaining == 0)
		{
			break;
		}
	}

	*lodExhaustedOut = 1;
	return 0;
}

static void RenderBucket_GteLoadRotMatrixWords(u32 m0, u32 m1, u32 m2, u32 m3, u32 m4)
{
	// NOTE(aalhendi): The retail loader consumes t3-t7, not C ABI arguments.
#ifdef CTR_NATIVE
	MATRIX_SET_r11r12r13r14r15(m0, m1, m2, m3, m4);
#else
	register u32 t3 CTR_PSX_REGISTER("$11") = m0;
	register u32 t4 CTR_PSX_REGISTER("$12") = m1;
	register u32 t5 CTR_PSX_REGISTER("$13") = m2;
	register u32 t6 CTR_PSX_REGISTER("$14") = m3;
	__asm__ volatile(".set noreorder\n\tjal MATRIX_SET_r11r12r13r14r15\n\tlw $15,%4\n\t.set reorder"
	                 :
	                 : "r"(t3), "r"(t4), "r"(t5), "r"(t6), "m"(m4)
	                 : "$15", "$31", "memory");
#endif
}

static void RenderBucket_StoreMatrixWords(MATRIX *m, u32 m0, u32 m1, u32 m2, u32 m3, u32 m4)
{
	RenderBucket_WriteMatrixWord(m, offsetof(MATRIX, m[0][0]), m0);
	RenderBucket_WriteMatrixWord(m, offsetof(MATRIX, m[0][2]), m1);
	RenderBucket_WriteMatrixWord(m, offsetof(MATRIX, m[1][1]), m2);
	RenderBucket_WriteMatrixWord(m, offsetof(MATRIX, m[2][0]), m3);
	RenderBucket_WriteMatrixWord(m, offsetof(MATRIX, m[2][2]), m4);
}

static void RenderBucket_LoadMatrixWords(const MATRIX *m, u32 *m0, u32 *m1, u32 *m2, u32 *m3, u32 *m4)
{
	*m0 = RenderBucket_ReadMatrixWord(m, offsetof(MATRIX, m[0][0]));
	*m1 = RenderBucket_ReadMatrixWord(m, offsetof(MATRIX, m[0][2]));
	*m2 = RenderBucket_ReadMatrixWord(m, offsetof(MATRIX, m[1][1]));
	*m3 = RenderBucket_ReadMatrixWord(m, offsetof(MATRIX, m[2][0]));
	*m4 = RenderBucket_ReadMatrixWord(m, offsetof(MATRIX, m[2][2]));
}

static void RenderBucket_GteLoadLightMatrixWords(const MATRIX *m)
{
	// NOTE(aalhendi): The retail loader consumes t3-t7, not C ABI arguments.
#ifdef CTR_NATIVE
	Unknown_8006c600(RenderBucket_ReadMatrixWord(m, offsetof(MATRIX, m[0][0])), RenderBucket_ReadMatrixWord(m, offsetof(MATRIX, m[0][2])),
	                 RenderBucket_ReadMatrixWord(m, offsetof(MATRIX, m[1][1])), RenderBucket_ReadMatrixWord(m, offsetof(MATRIX, m[2][0])),
	                 RenderBucket_ReadMatrixWord(m, offsetof(MATRIX, m[2][2])));
#else
	register u32 t3 CTR_PSX_REGISTER("$11") = RenderBucket_ReadMatrixWord(m, offsetof(MATRIX, m[0][0]));
	register u32 t4 CTR_PSX_REGISTER("$12") = RenderBucket_ReadMatrixWord(m, offsetof(MATRIX, m[0][2]));
	register u32 t5 CTR_PSX_REGISTER("$13") = RenderBucket_ReadMatrixWord(m, offsetof(MATRIX, m[1][1]));
	register u32 t6 CTR_PSX_REGISTER("$14") = RenderBucket_ReadMatrixWord(m, offsetof(MATRIX, m[2][0]));
	__asm__ volatile(".set noreorder\n\tjal Unknown_8006c600\n\tlw $15,%5(%4)\n\t.set reorder"
	                 :
	                 : "r"(t3), "r"(t4), "r"(t5), "r"(t6), "r"(m), "I"(offsetof(MATRIX, m[2][2]))
	                 : "$15", "$31", "memory");
#endif
}

#include "RenderBucket_LightMatrix.c"

static void RenderBucket_BuildM3x3(struct Instance *inst, struct ModelHeader *mh, int viewDepth, struct RenderBucketMatrixState *matrixState)
{
	u32 m0;
	u32 m1;
	u32 m2;
	u32 m3;
	u32 m4;
	u32 packedScaleXY;
	int depthShift;
	int scaleXYShift;
	int scaleZShift;
	int scaleX;
	int scaleY;
	int scaleZ;
	u32 scaledX;
	u32 scaledY;
	u32 scaledZ;

	// NOTE(aalhendi): Retail 0x80070b38-0x80070c18 computes the m3x3 tuple
	// through t3/t4/t5/t6/t7, ModelHeader scale, and helper 0x8006c49c. The
	// later IDPP store happens after frame selection at 0x80070dfc.
	// NOTE(aalhendi): Retail treats MATRIX halfwords as packed GTE register words here.
	m0 = RenderBucket_ReadMatrixWord(&inst->matrix, offsetof(MATRIX, m[0][0]));
	m1 = RenderBucket_ReadMatrixWord(&inst->matrix, offsetof(MATRIX, m[0][2]));
	m2 = RenderBucket_ReadMatrixWord(&inst->matrix, offsetof(MATRIX, m[1][1]));
	m3 = RenderBucket_ReadMatrixWord(&inst->matrix, offsetof(MATRIX, m[2][0]));
	m4 = RenderBucket_ReadMatrixWord(&inst->matrix, offsetof(MATRIX, m[2][2]));
	RenderBucket_GteLoadRotMatrixWords(m0, m1, m2, m3, m4);

	depthShift = (RenderBucket_MipsSub(viewDepth, 0x1000) < 0) ? 2 : 0;
	scaleXYShift = 0x12 - depthShift;
	scaleZShift = 2 - depthShift;
	packedScaleXY = RenderBucket_ReadPackedWord(&mh->scale.x);

	CTC2((packedScaleXY << 16) >> scaleXYShift, 16);
	CTC2(0, 17);
	CTC2(packedScaleXY >> scaleXYShift, 18);
	CTC2(0, 19);
	CTC2((u16)mh->scale.z >> scaleZShift, 20);

	scaleX = inst->scale.x;
	scaleY = inst->scale.y;
	scaleZ = inst->scale.z;
	if ((inst->flags & PIXEL_LOD) != 0)
	{
		int pixelScale = RenderBucket_MipsAdd(viewDepth >> 1, 0x1000);

		scaleX = RenderBucket_MipsMulLoSra12(pixelScale, scaleX);
		scaleY = RenderBucket_MipsMulLoSra12(pixelScale, scaleY);
		scaleZ = RenderBucket_MipsMulLoSra12(pixelScale, scaleZ);
	}

	MTC2(RenderBucket_PackXY(scaleX, scaleY), 0);
	MTC2(scaleZ, 1);
	doCOP2(0x04c6012);

	scaledX = MFC2(9);
	scaledY = MFC2(10);
	scaledZ = MFC2(11);
	matrixState->scratch74 = (s16)scaledX;
	matrixState->scratch76 = (s16)scaledY;
	matrixState->scratch78 = (int)scaledZ;
	// NOTE(aalhendi): Retail passes the lcv0 result as a diagonal matrix into helper 0x8006c49c.
	m0 = scaledX & 0xffff;
	m1 = 0;
	m2 = scaledY & 0xffff;
	m3 = 0;
	m4 = scaledZ & 0xffff;
	CTR_GteComposeRotationWords(&m0, &m1, &m2, &m3, &m4);
	matrixState->m0 = m0;
	matrixState->m1 = m1;
	matrixState->m2 = m2;
	matrixState->m3 = m3;
	matrixState->m4 = m4;
}

static void RenderBucket_BuildMvp(struct PushBuffer *pb, struct InstDrawPerPlayer *idpp, MATRIX *projectionMvp)
{
	u32 m0;
	u32 m1;
	u32 m2;
	u32 m3;
	u32 m4;

	// NOTE(aalhendi): Source-backs QueueDraw's normal MVP rotation path through
	// helper 0x8006c558. Split/reflection variants keep a separate projection
	// matrix because retail projects bounds through live GTE state while storing
	// a different matrix in IDPP for Execute.
	RenderBucket_LoadMatrixWords(&idpp->m3x3, &m0, &m1, &m2, &m3, &m4);
	RenderBucket_GteLoadLightMatrixWords(&pb->matrix_ViewProj);
	RenderBucket_GteMulLightMatrixColumns(&m0, &m1, &m2, &m3, &m4);
	RenderBucket_StoreMatrixWords(&idpp->mvp, m0, m1, m2, m3, m4);
	*projectionMvp = idpp->mvp;
	RenderBucket_GteLoadRotMatrixWords(m0, m1, m2, m3, m4);
}

static void RenderBucket_BuildSplitViewMvp(struct PushBuffer *pb, struct InstDrawPerPlayer *idpp, MATRIX *projectionMvp)
{
	u32 m0;
	u32 m1;
	u32 m2;
	u32 m3;
	u32 m4;

	// NOTE(aalhendi): Source-backs QueueDraw 0x80070ea4-0x80070ed8. Retail
	// composes the projection matrix, stores the PushBuffer view matrix in IDPP,
	// then reloads the composed tuple for immediate bbox projection.
	RenderBucket_LoadMatrixWords(&idpp->m3x3, &m0, &m1, &m2, &m3, &m4);
	RenderBucket_GteLoadLightMatrixWords(&pb->matrix_ViewProj);
	RenderBucket_GteMulLightMatrixColumns(&m0, &m1, &m2, &m3, &m4);
	*projectionMvp = idpp->mvp;
	RenderBucket_StoreMatrixWords(projectionMvp, m0, m1, m2, m3, m4);
	RenderBucket_StoreViewMatrixForSplit(idpp);
	RenderBucket_GteLoadRotMatrixWords(m0, m1, m2, m3, m4);
}

static void RenderBucket_StoreViewMatrixForSplit(struct InstDrawPerPlayer *idpp)
{
	RenderBucket_StoreMatrixWords(&idpp->mvp, CFC2(8), CFC2(9), CFC2(10), CFC2(11), CFC2(12));
}

static int RenderBucket_NeedsCustomMatrix(u32 instFlags, const struct ModelHeader *mh)
{
	return ((instFlags & CUSTOM_MATRIX) != 0) || ((mh->flags & RB_MODEL_ALWAYS_POINT_NORTH) != 0);
}

static void RenderBucket_BuildCustomMatrix(struct InstDrawPerPlayer *idpp, u32 instFlags, const struct RenderBucketMatrixState *matrixState,
                                           MATRIX *projectionMvp)
{
	u32 m0;
	u32 m1;
	u32 m2;
	u32 m3;
	u32 m4;
	int viewX = idpp->mvp.t[0];
	int viewZ = idpp->mvp.t[2];
	int length;

	// NOTE(aalhendi): Source-backs QueueDraw's 0x80070f84 custom/always-north
	// matrix producer. Retail builds a view-facing basis from TRX/TRZ, scales it
	// through helper 0x8006c49c, then applies the fixed tilt matrix before bbox
	// projection.
	if ((instFlags & REVERSE_CULL_DIRECTION) != 0)
	{
		viewX = RenderBucket_MipsSub(0, viewX);
	}

	MTC2(viewX, 9);
	MTC2(viewZ, 10);
	doCOP2(0x0a00428);
	length = SquareRoot0_stub(RenderBucket_MipsAdd(MFC2(25), MFC2(26)));
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail falls through to raw divs by this value; native
	// keeps the host from trapping on impossible/degenerate camera-facing input.
	if (length == 0)
	{
		return;
	}
#endif

	m0 = (u16)(RenderBucket_MipsSll(viewZ, 12) / length);
	m1 = (u16)(RenderBucket_MipsSll(viewX, 12) / length);
	m2 = 0x1000;
	m3 = (u16)RenderBucket_MipsSub(0, m1);
	m4 = m0;

	RenderBucket_GteLoadRotMatrixWords(matrixState->m0, matrixState->m1, matrixState->m2, matrixState->m3, matrixState->m4);
	CTR_GteComposeRotationWords(&m0, &m1, &m2, &m3, &m4);
	RenderBucket_GteLoadRotMatrixWords(0x1000, 0, 0xfffff600, 0, 0x1000);
	CTR_GteComposeRotationWords(&m0, &m1, &m2, &m3, &m4);
	RenderBucket_StoreMatrixWords(&idpp->mvp, m0, m1, m2, m3, m4);
	*projectionMvp = idpp->mvp;
}

static struct RenderBucketSplitState RenderBucket_BuildSplitState(struct Instance *inst, struct ModelHeader *mh, struct ModelFrame *frame,
                                                                  struct ModelFrame *nextFrame, struct PushBuffer *pb, struct InstDrawPerPlayer *idpp,
                                                                  int viewDepth, u32 *instFlags, const struct RenderBucketMatrixState *matrixState,
                                                                  MATRIX *projectionMvp)
{
	struct RenderBucketSplitState split = RenderBucket_InitSplitState(nextFrame);
	u32 matrixOr;
	int frameY;
	int baseY;
	int rawSplit;
	int splitLine;

	if ((*instFlags & SPLIT_STATE_MASK) == 0)
	{
		if (RenderBucket_NeedsCustomMatrix(*instFlags, mh) != 0)
		{
			RenderBucket_BuildCustomMatrix(idpp, *instFlags, matrixState, projectionMvp);
		}
		else
		{
			RenderBucket_BuildMvp(pb, idpp, projectionMvp);
		}
		return split;
	}

	if ((*instFlags & SPLIT_SPECIAL) != 0)
	{
		int specialDepth = RenderBucket_MipsSub(RenderBucket_MipsSub(viewDepth, RenderBucket_MipsSll(pb->distanceToScreen_PREV, 2)),
		                                        RenderBucket_MipsMulLoSra12(matrixState->scratch78, 1448));

		if (specialDepth < 0)
		{
			u32 m0;
			u32 m1;
			u32 m2;
			u32 m3;
			u32 m4;

			// NOTE(aalhendi): Source-backs the 0x80070edc-0x80070f48 special
			// split producer: retail stores the live composed matrix in IDPP
			// m3x3, but keeps bbox projection on the live GTE matrix.
			RenderBucket_BuildMvp(pb, idpp, projectionMvp);
			RenderBucket_LoadMatrixWords(projectionMvp, &m0, &m1, &m2, &m3, &m4);
			RenderBucket_StoreMatrixWords(&idpp->m3x3, m0, m1, m2, m3, m4);
			RenderBucket_StoreMatrixWords(&idpp->mvp, 0x1000, 0, 0x1000, 0, 0x1000);
			split.scratch70 = *instFlags;
		}
		else if (RenderBucket_NeedsCustomMatrix(*instFlags, mh) != 0)
		{
			RenderBucket_BuildCustomMatrix(idpp, *instFlags, matrixState, projectionMvp);
		}
		else
		{
			RenderBucket_BuildMvp(pb, idpp, projectionMvp);
		}

		return split;
	}

#ifdef CTR_NATIVE
	if (matrixState->scratch76 == 0)
	{
		// NOTE(aalhendi): Retail reaches raw MIPS divs by this value. PS1 div
		// by zero writes HI/LO instead of trapping; see PSX-SPX CPU
		// Specifications:
		// https://psx-spx.consoledev.net/cpuspecifications/#cpu-alu-opcodes
		// Native cannot execute host C division by zero, so preserve the retail
		// reflective zero-scale path instead of selecting the special mirrored
		// handler with stale MVP state.
		if ((*instFlags & REFLECTIVE) != 0)
		{
			*instFlags &= ~PUSHBUFFER_EXISTS;
			split.scratch6c = -257;
		}

		if (RenderBucket_NeedsCustomMatrix(*instFlags, mh) != 0)
		{
			RenderBucket_BuildCustomMatrix(idpp, *instFlags, matrixState, projectionMvp);
		}
		else if ((*instFlags & REFLECTIVE) != 0)
		{
			RenderBucket_BuildSplitViewMvp(pb, idpp, projectionMvp);
		}
		else
		{
			RenderBucket_BuildMvp(pb, idpp, projectionMvp);
		}
		return split;
	}
#endif

	frameY = (s16)(RenderBucket_PackedFrameXY(frame, nextFrame) >> 16);
	baseY = RenderBucket_MipsAdd(inst->matrix.t[1], (frameY / matrixState->scratch76) >> 12);
	rawSplit = RenderBucket_MipsSub(inst->vertSplit, baseY);
	matrixOr = matrixState->m0 | matrixState->m1 | matrixState->m2 | matrixState->m3 | matrixState->m4;

	if (matrixOr == 0x1000)
	{
		splitLine = RenderBucket_MipsSll(RenderBucket_MipsSll(rawSplit, 12) / matrixState->scratch76, 2);
		idpp->splitLine = (s16)splitLine;
		split.scratch68 = splitLine;
		*instFlags &= ~PUSHBUFFER_EXISTS;
		if (RenderBucket_NeedsCustomMatrix(*instFlags, mh) != 0)
		{
			RenderBucket_BuildCustomMatrix(idpp, *instFlags, matrixState, projectionMvp);
		}
		else
		{
			RenderBucket_BuildMvp(pb, idpp, projectionMvp);
		}
		return split;
	}

	splitLine = RenderBucket_MipsSll(rawSplit, 2);
	idpp->splitLine = (s16)splitLine;
	if (((*instFlags & REFLECTIVE) == 0) && (RenderBucket_MipsAdd(splitLine, 362) < 0))
	{
		if (RenderBucket_NeedsCustomMatrix(*instFlags, mh) != 0)
		{
			RenderBucket_BuildCustomMatrix(idpp, *instFlags, matrixState, projectionMvp);
		}
		else
		{
			RenderBucket_BuildMvp(pb, idpp, projectionMvp);
		}
		return split;
	}

	*instFlags &= ~PUSHBUFFER_EXISTS;
	split.scratch6c = -257;

	if (RenderBucket_NeedsCustomMatrix(*instFlags, mh) != 0)
	{
		RenderBucket_BuildCustomMatrix(idpp, *instFlags, matrixState, projectionMvp);
	}
	else
	{
		RenderBucket_BuildSplitViewMvp(pb, idpp, projectionMvp);
	}

	return split;
}

static int RenderBucket_AllocateOTRange(struct RenderBucketQueueState *queueState, struct PushBuffer *pb, int minDepth, int maxDepth, int viewDepth,
                                        int depthBias, int usePushBuffer)
{
	u32 *rangeStart;
	u32 *rangeEnd;
	u32 *newCurr;
	u32 *otSlot;
	u32 prevOtSlotHead;
	int range;
	int byteOffset;

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail consumes the OT allocator scratch pointers
	// directly; native keeps malformed host-side render state from trapping.
	if (queueState->otCurr == 0)
	{
		return 0;
	}

	if (queueState->otEndMinusOne == 0)
	{
		return 0;
	}

	if ((usePushBuffer == 0) && (queueState->otBase == 0))
	{
		return 0;
	}
#endif

	range = RenderBucket_MipsSub(maxDepth, minDepth);
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail reaches allocation with cull-ordered depths and
	// does not guard this separately.
	if (range < 0)
	{
		return 0;
	}
#endif

	rangeStart = queueState->otCurr;
	rangeEnd = rangeStart + range;
	newCurr = rangeEnd + 1;

	// NOTE(aalhendi): Retail callers seed scratch 0x38 with `otMem->end - 4`;
	// QueueDraw rejects when `(end - 4) - newCurr <= 0` at 0x80071230-0x80071234.
	if (RenderBucket_AddressSub(queueState->otEndMinusOne, newCurr) <= 0)
	{
		return 0;
	}

	// NOTE(aalhendi): Retail 0x80071218-0x800712f4 links per-instance OT ranges
	// by writing raw 24-bit primitive tags. Native uses the same tag-chain shape
	// with explicit C pointer masking.
	queueState->otCurr = newCurr;

	byteOffset = RenderBucket_ClampOTByteOffset(RenderBucket_MipsAdd(viewDepth >> 6, depthBias));

	if (usePushBuffer != 0)
	{
		// NOTE(aalhendi): Source-backs QueueDraw 0x800712bc-0x800712c8.
		// PUSHBUFFER_EXISTS reuses PushBuffer 0xf4/0xf8/0xfc as range
		// start/end/byte-offset metadata after DecalMP seeds ptrOT.
		pb->ptrOT = rangeStart;
		pb->renderBucketOTRangeEnd = rangeEnd;
		pb->renderBucketOTByteOffset = byteOffset;
		rangeStart[0] = 0;
	}
	else
	{
		otSlot = RenderBucket_OTSlotAtByteOffset(queueState->otBase, byteOffset);
		prevOtSlotHead = otSlot[0];
		// NOTE(aalhendi): Retail stores the OT slot before rangeStart[0].
		otSlot[0] = RenderBucket_OTAddress(rangeEnd);
		rangeStart[0] = prevOtSlotHead;
	}

	{
		u32 *entry = rangeStart;
		for (; entry != rangeEnd; entry++)
		{
			entry[1] = RenderBucket_OTAddress(entry);
		}
	}

	return RenderBucket_AddressSubOffset(rangeStart, RenderBucket_MipsSll(minDepth, 2));
}

static void RenderBucket_UpdatePushBufferMetadata(struct PushBuffer *pb, const struct RenderBucketBounds *bounds, u32 *instFlags)
{
	int width;
	int height;
	int packedWidth;

	if ((*instFlags & PUSHBUFFER_EXISTS) == 0)
	{
		return;
	}

	// NOTE(aalhendi): Retail 0x80071164-0x800711c8 writes projected screen
	// pos/size to PushBuffer 0x100/0x104 and clears PUSHBUFFER_EXISTS when the
	// projected bounds exceed the small-buffer constraints.
	width = RenderBucket_MipsSub(bounds->maxX, bounds->minX);
	height = RenderBucket_MipsSub(bounds->maxY, bounds->minY);
	packedWidth = (u16)width;
	pb->renderBucketScreenPos.packed = RenderBucket_PackXY(bounds->minX, bounds->minY);
	pb->renderBucketScreenSize.packed = RenderBucket_PackXY(width, height);

	if ((bounds->minX < 0) || (bounds->minY < 0) || (RenderBucket_MipsSub(height, 0x40) > 0) || (RenderBucket_MipsSub(packedWidth, 0x60) > 0) ||
	    (RenderBucket_MipsSub(bounds->maxX, (s16)pb->rect.w) >= 0) || (RenderBucket_MipsSub(bounds->maxY, (s16)pb->rect.h) >= 0))
	{
		*instFlags &= ~PUSHBUFFER_EXISTS;
	}
}

static int RenderBucket_ShouldAllocateSecondaryRange(u32 instFlags, const struct RenderBucketSplitState *split)
{
	if ((instFlags & PUSHBUFFER_EXISTS) != 0)
	{
		return 0;
	}

	if ((instFlags & (SPLIT_LINE | REFLECTIVE)) == 0)
	{
		return 0;
	}

	if ((instFlags & REFLECTIVE) != 0)
	{
		return 1;
	}

	// NOTE(aalhendi): Source-backs QueueDraw's secondary-range gate at
	// 0x80071300-0x80071320 using the split/reflection producer state.
	return RenderBucket_HasSplitOutput(split);
}

static int RenderBucket_BuildDepthRange(struct Instance *inst, struct ModelFrame *frame, struct ModelFrame *nextFrame, struct PushBuffer *pb,
                                        struct InstDrawPerPlayer *idpp, struct RenderBucketQueueState *queueState, int viewDepth, int normalDepthBias,
                                        int secondaryDepthBias, u32 *instFlags, const struct RenderBucketSplitState *split, const MATRIX *projectionMvp)
{
	struct RenderBucketBounds bounds;
	int primaryRange;
	int secondaryRange;
	int geomScreenHalf;
	int minDepth;
	int maxDepth;

	(void)inst;
	RenderBucket_ProjectFrameBounds(frame, nextFrame, pb, projectionMvp, &bounds, &geomScreenHalf);
	RenderBucket_UpdatePushBufferMetadata(pb, &bounds, instFlags);
	if (RenderBucket_AcceptProjectedBounds(&bounds, pb, geomScreenHalf) == 0)
	{
		return 0;
	}

	if ((*instFlags & RB_INSTANCE_SKIP_OT_RANGE) != 0)
	{
		// NOTE(aalhendi): Source-backs QueueDraw 0x800711f4-0x80071200. Retail
		// still performs bbox projection/cull, then skips OT range allocation.
		// If split/reflection selection runs, 0x800713c8 stores the stale t1
		// value from the cull path, which is still the unsigned viewport height.
		if ((*instFlags & (SPLIT_LINE | REFLECTIVE)) != 0)
		{
			idpp->otRangeSecondary = (u16)pb->rect.h;
		}
		return 1;
	}

	minDepth = RenderBucket_MipsAdd(bounds.minZ >> 5, -2);
	maxDepth = RenderBucket_MipsAdd(bounds.maxZ >> 5, 1);

	idpp->depthOffset[0] = minDepth;
	idpp->depthOffset[1] = maxDepth;

	primaryRange = RenderBucket_AllocateOTRange(queueState, pb, minDepth, maxDepth, viewDepth, normalDepthBias, (*instFlags & PUSHBUFFER_EXISTS) != 0);
	if (primaryRange == 0)
	{
		return 0;
	}

	idpp->otRangeNormal = primaryRange;
	idpp->otRangeSecondary = primaryRange;

	if (RenderBucket_ShouldAllocateSecondaryRange(*instFlags, split) != 0)
	{
		// NOTE(aalhendi): Retail QueueDraw allocates this distinct
		// reflected/split OT range at 0x80071320-0x800713b4 with signed
		// Instance.depthBiasSecondary as the second depth bias. The allocation predicate is
		// source-backed through RenderBucketSplitState.
		secondaryRange = RenderBucket_AllocateOTRange(queueState, pb, minDepth, maxDepth, viewDepth, secondaryDepthBias, 0);
		if (secondaryRange == 0)
		{
			return 0;
		}

		idpp->otRangeSecondary = secondaryRange;
	}

	return 1;
}

static void RenderBucket_StoreInstanceAnimWord(struct Instance *inst, int frame)
{
	// NOTE(aalhendi): Retail QueueDraw uses `sw` at Instance+0x54, so this
	// intentionally writes the full animFrame/vertSplit word.
	RenderBucket_WritePackedWord((u8 *)inst + offsetof(struct Instance, animFrame), (u32)frame);
}

static void RenderBucket_AdvanceInstanceAnimWord(struct Instance *inst, int gameMode1, int playerIndex, int lastFrame, u32 *queuedFlags)
{
	int frame;

	if (gameMode1 != 0)
	{
		return;
	}

	// NOTE(aalhendi): Retail tests `sp == s8`; caller `s8 = inst + player*0x88`.
	if (playerIndex != 0)
	{
		return;
	}

	if (lastFrame < 0)
	{
		return;
	}

	if ((*queuedFlags & 0x30) == 0)
	{
		return;
	}

	frame = (u16)inst->animFrame;

	if ((*queuedFlags & 0x20) == 0)
	{
		if (RenderBucket_MipsSub(lastFrame, frame) > 0)
		{
			frame = RenderBucket_MipsAdd(frame, 1);
		}
		else
		{
			frame = 0;
		}
	}

	else if ((*queuedFlags & 0x10) != 0)
	{
		if (RenderBucket_MipsSub(lastFrame, frame) > 0)
		{
			frame = RenderBucket_MipsAdd(frame, 1);
		}
		else
		{
			*queuedFlags &= ~0x10;
			frame = RenderBucket_MipsAdd(lastFrame, -1);
		}
	}

	else
	{
		if (frame > 0)
		{
			frame = RenderBucket_MipsAdd(frame, -1);
		}
		else
		{
			*queuedFlags |= 0x10;
			frame = 1;
		}
	}

	RenderBucket_StoreInstanceAnimWord(inst, frame);
}

static struct ModelFrame *RenderBucket_GetFrame(struct Instance *inst, struct ModelHeader *mh, struct ModelFrame **nextFrameOut, int *deltaArrayOut,
                                                int *lastFrameAdvanceOut)
{
	struct ModelAnim *anim;
	int frameIndex;
	int lastFrame;
	int hasNextFrame;
	u16 frameSize;
	u8 *firstFrame;
	struct ModelFrame *currentFrame;

	*nextFrameOut = 0;
	*deltaArrayOut = 0;
	*lastFrameAdvanceOut = -1;

	if (mh->ptrAnimations == 0)
	{
		*deltaArrayOut = mh->unk3;
		return mh->ptrFrameData;
	}

	anim = RenderBucket_GetAnim(inst, mh);
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail uses the selected animation pointer directly;
	// native tolerates sparse host-side model data while keeping PSX behavior
	// as the default path.
	if (anim == 0)
	{
		return 0;
	}
	if (anim->numFrames == 0)
	{
		return 0;
	}
#endif

	// NOTE(aalhendi): Retail 0x80070ca0-0x80070dfc checks ptrAnimations first,
	// then carries current/next frame through s6/s1 plus ptrDeltaArray through
	// IDPP 0xd4. Native keeps those values as explicit return values.
	*deltaArrayOut = (int)anim->ptrDeltaArray;
	frameIndex = (u16)inst->animFrame;
	lastFrame = (anim->numFrames & 0x7fff) - 1;
	*lastFrameAdvanceOut = lastFrame;
	hasNextFrame = 0;

	if ((s16)anim->numFrames < 0)
	{
		lastFrame >>= 1;
		hasNextFrame = frameIndex & 1;
		frameIndex >>= 1;
	}

	if (RenderBucket_MipsSub(lastFrame, frameIndex) < 0)
	{
		frameIndex = lastFrame;
	}

	firstFrame = RenderBucket_ModelAnimFirstFrameBytes(anim);
	frameSize = (u16)anim->frameSize;
	currentFrame = RenderBucket_ModelFrameAtByteOffset(firstFrame, RenderBucket_MipsMultuLo((u32)frameIndex, frameSize));

	if (hasNextFrame != 0)
	{
		*nextFrameOut = RenderBucket_ModelFrameAtByteOffset((u8 *)currentFrame, frameSize);
	}

	return currentFrame;
}

#ifndef CTR_NATIVE
// NOTE(aalhendi): Keep C context providers apart from the renderer register ABI.
#define RenderBucket_QueueDraw            RenderBucket_QueueDraw_Context
#define RenderBucket_QueueLevInstances    RenderBucket_QueueLevInstances_Context
#define RenderBucket_QueueNonLevInstances RenderBucket_QueueNonLevInstances_Context
#endif

static struct RenderBucketEntry *RenderBucket_QueueDraw(struct Instance *inst, struct RenderBucketEntry *rbi, int playerIndex, u32 lodMask, int gameMode1,
                                                        struct RenderBucketQueueState *queueState)
{
	struct ModelHeader *mh;
	struct ModelFrame *frame;
	struct ModelFrame *nextFrame;
	struct InstDrawPerPlayer *idpp;
	struct Instance *instPlayerBase;
	struct PushBuffer *pb;
	struct RenderBucketMatrixState matrixState;
	struct RenderBucketSplitState split;
	MATRIX projectionMvp;
	u32 queuedFlags;
	int deltaArray;
	int lastFrameAdvance;
	int lodIndex;
	int lodExhausted;
	int normalDepthBias;
	int secondaryDepthBias;
	int viewDepth;
	int drawFunc;
	int uncompressFunc;
	VECTOR viewPos;

	// NOTE(aalhendi): Retail QueueDraw consumes implicit scratchpad/register
	// state from QueueLev/QueueNonLev. Native spells the verified same state out
	// as C parameters and RenderBucketQueueState.

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail callers provide a valid visible-list instance and
	// model; native keeps malformed host-side lists from trapping.
	if (inst == 0)
	{
		return rbi;
	}

	if (inst->model == 0)
	{
		return rbi;
	}

	if (inst->model->headers == 0)
	{
		return rbi;
	}
#endif

	queuedFlags = inst->flags;
	instPlayerBase = RenderBucket_InstancePlayerBase(inst, playerIndex);
	idpp = RenderBucket_InstancePlayerIdpp(instPlayerBase);
	pb = idpp->pushBuffer;

	if ((queuedFlags & lodMask) == 0)
	{
		// NOTE(aalhendi): Retail 0x80070950-0x80070964 exits through the common
		// epilogue and still stores the current gp flags to IDPP+0xb8.
		idpp->instFlags = queuedFlags;
		return rbi;
	}

	if (pb == 0)
	{
		// NOTE(aalhendi): Retail 0x80070964 reaches the same epilogue store.
		idpp->instFlags = queuedFlags;
		return rbi;
	}

	if (queueState->lastPushBuffer != pb)
	{
		// NOTE(aalhendi): Retail 0x80070968-0x800709d0 refreshes scratch
		// `at+0x30` from PushBuffer+0xf4 only when the PushBuffer changes.
		queueState->lastPushBuffer = pb;
		queueState->otBase = pb->ptrOT;
	}

	RenderBucket_ApplyOwnerPushBufferGate(inst, playerIndex, &queuedFlags);
	RenderBucket_GetViewPosition(inst, pb, &viewPos);
	viewDepth = viewPos.vz;
	if (RenderBucket_WriteAlphaScale(inst, idpp, viewDepth, queuedFlags) == 0)
	{
		idpp->instFlags = queuedFlags;
		return rbi;
	}

	RenderBucket_AdjustViewPositionForMvp(inst, &viewPos);
	RenderBucket_StoreMvpTranslation(idpp, &viewPos);
	mh = RenderBucket_SelectModelHeader(inst, pb, &lodIndex, &lodExhausted, viewDepth);
	if (mh == 0)
	{
		if (lodExhausted != 0)
		{
			// NOTE(aalhendi): Source-backs the no-LOD branch at 0x80070b28.
			idpp->mh = 0;
		}
		idpp->instFlags = queuedFlags;
		return rbi;
	}

	idpp->mh = mh;
	idpp->lodIndex = lodIndex;
	normalDepthBias = RenderBucket_SignExtendByte(inst->depthBiasNormal);
	secondaryDepthBias = RenderBucket_SignExtendByte(inst->depthBiasSecondary);
	RenderBucket_BuildM3x3(inst, mh, viewDepth, &matrixState);
	RenderBucket_AdjustDepthBiasForNormal(inst, playerIndex, &normalDepthBias, &secondaryDepthBias);

	frame = RenderBucket_GetFrame(inst, mh, &nextFrame, &deltaArray, &lastFrameAdvance);
	idpp->ptrDeltaArray = deltaArray;
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail uses the selected frame pointer directly; native
	// tolerates incomplete host-side model data.
	if (frame == 0)
	{
		idpp->instFlags = queuedFlags;
		return rbi;
	}
#endif

	RenderBucket_AdvanceInstanceAnimWord(inst, gameMode1, playerIndex, lastFrameAdvance, &queuedFlags);
	idpp->ptrCurrFrame = frame;
	idpp->ptrNextFrame = nextFrame;
	RenderBucket_StoreMatrixWords(&idpp->m3x3, matrixState.m0, matrixState.m1, matrixState.m2, matrixState.m3, matrixState.m4);
	split = RenderBucket_BuildSplitState(inst, mh, frame, nextFrame, pb, idpp, viewDepth, &queuedFlags, &matrixState, &projectionMvp);

	if (RenderBucket_BuildDepthRange(inst, frame, nextFrame, pb, idpp, queueState, viewDepth, normalDepthBias, secondaryDepthBias, &queuedFlags, &split,
	                                 &projectionMvp) == 0)
	{
		idpp->instFlags = queuedFlags;
		return rbi;
	}

	RenderBucket_SelectRetailHandlers(&queuedFlags, &split, &drawFunc, &uncompressFunc);
	rbi->inst = inst;
	rbi->instPlayerBase = instPlayerBase;
	queuedFlags |= DRAW_SUCCESSFUL;
	idpp->unkEC = drawFunc;
	idpp->unkF0 = uncompressFunc;
	RenderBucket_WriteInstanceCallbackLabels(inst, queuedFlags);
	idpp->ptrCommandList = mh->ptrCommandList;
	idpp->ptrTexLayout = mh->ptrTexLayout;
	idpp->ptrColorLayout = (u32)mh->ptrColors;
	idpp->instFlags = queuedFlags;
	return rbi + 1;
}

void *RenderBucket_QueueLevInstances(struct CameraDC *cDC, struct OTMem *otState, void *rbi, u32 lodMask, u8 numPlyr, int gameMode1)
{
	struct RenderBucketEntry *entry = (struct RenderBucketEntry *)rbi;
	struct RenderBucketQueueState queueState = {0};
	int count = (int)(u8)numPlyr;

	// Retail enters QueueDraw through a scratch/register
	// ABI; native passes the same state as explicit C parameters.
	RenderBucket_CopyDispatchTables();

#ifdef CTR_NATIVE
	if (otState != 0)
#endif
	{
		queueState.otCurr = otState->cursor;
		queueState.otEndMinusOne = otState->end - 1;
	}

	{
		int player = count - 1;
		for (; player >= 0; player--)
		{
			struct Instance **visInstSrc = cDC[player].visInstSrc;

			if (visInstSrc == 0)
			{
				continue;
			}

			for (; *visInstSrc != 0; visInstSrc++)
			{
				entry = RenderBucket_QueueDraw(*visInstSrc, entry, player, lodMask, gameMode1, &queueState);
			}
		}
	}

#ifdef CTR_NATIVE
	if (otState != 0)
#endif
	{
		otState->cursor = queueState.otCurr;
	}

	return entry;
}

void *RenderBucket_QueueNonLevInstances(struct Item *item, struct OTMem *otState, void *rbi, u32 lodMask, u8 numPlyr, int gameMode1)
{
	struct RenderBucketEntry *entry = (struct RenderBucketEntry *)rbi;
	struct RenderBucketQueueState queueState = {0};
	int count = (int)(u8)numPlyr;

	// Retail enters QueueDraw through a scratch/register
	// ABI; native passes the same state as explicit C parameters.
	RenderBucket_CopyDispatchTables();

#ifdef CTR_NATIVE
	if (otState != 0)
#endif
	{
		queueState.otCurr = otState->cursor;
		queueState.otEndMinusOne = otState->end - 1;
	}

	{
		int player = count - 1;
		for (; player >= 0; player--)
		{
			struct Item *curr = item;
			for (; curr != 0; curr = curr->next)
			{
				entry = RenderBucket_QueueDraw((struct Instance *)curr, entry, player, lodMask, gameMode1, &queueState);
			}
		}
	}

#ifdef CTR_NATIVE
	if (otState != 0)
#endif
	{
		otState->cursor = queueState.otCurr;
	}

	return entry;
}

#ifndef CTR_NATIVE
#undef RenderBucket_QueueNonLevInstances
#undef RenderBucket_QueueLevInstances
#undef RenderBucket_QueueDraw
#undef RenderBucket_CopyDispatchTables
#endif

static u32 RenderBucket_PackModelVertexXY(struct RenderBucketDrawContext *ctx, const RenderBucketVertex *vertex)
{
	u32 frameOriginXY = (u16)(ctx->mf->pos.x & 0x7fff) | ((u32)(u16)ctx->mf->pos.y << 16);
	u32 vertexXZ = ((u32)vertex->x) | ((u32)vertex->z << 16);

	// NOTE(aalhendi): Retail does a single packed 32-bit add:
	// `((vertex & 0x00ff00ff) + frameOriginXY) << 2`, so preserve possible
	// carry from the low X half into the packed Z half. Execute masks frame X
	// with 0x7fff before writing the non-interpolated origin scratch word.
	return ((vertexXZ + frameOriginXY) << 2) & 0xfff8ffff;
}

static u32 RenderBucket_PackInterpolatedModelVertexXY(struct RenderBucketDrawContext *ctx, const RenderBucketVertex *curr, const RenderBucketVertex *next)
{
	struct ModelFrame *nextFrame = ctx->idpp->ptrNextFrame;
	u32 frameOriginXY = (u16)(ctx->mf->pos.x + nextFrame->pos.x);
	u32 currXZ = ((u32)curr->x) | ((u32)curr->z << 16);
	u32 nextXZ = ((u32)next->x) | ((u32)next->z << 16);

	frameOriginXY |= (u32)(u16)(ctx->mf->pos.y + nextFrame->pos.y) << 16;

	// NOTE(aalhendi): Source-backs retail next-frame decoder 0x8006b464-
	// 0x8006b480: current and next vertex X/Z bytes are added with the summed
	// frame origin, then shifted by one instead of the non-interpolated << 2.
	return ((currXZ + nextXZ + frameOriginXY) << 1) & 0xfff8ffff;
}

static u32 RenderBucket_ModelVertexZ(struct RenderBucketDrawContext *ctx, const RenderBucketVertex *vertex)
{
	return ((u32)(ctx->mf->pos.z + vertex->y)) << 2;
}

static u32 RenderBucket_InterpolatedModelVertexZ(struct RenderBucketDrawContext *ctx, const RenderBucketVertex *curr, const RenderBucketVertex *next)
{
	struct ModelFrame *nextFrame = ctx->idpp->ptrNextFrame;
	int z = (int)curr->y + (int)next->y + (int)ctx->mf->pos.z + (int)nextFrame->pos.z;

	// NOTE(aalhendi): Source-backs retail 0x8006b480-0x8006b49c. The vertical
	// byte pair and summed frame Z are doubled, producing the halfway frame.
	return (u32)RenderBucket_MipsSll(z, 1);
}

static struct RenderBucketPackedVertex RenderBucket_CachePackedVertex(struct RenderBucketDrawContext *ctx, u16 stackIndex)
{
	struct RenderBucketPackedVertex *scratchVertex = RenderBucket_PackedVertexScratch(stackIndex);
	struct RenderBucketPackedVertex packed;

	packed.xy = RenderBucket_PackModelVertexXY(ctx, &ctx->stack[stackIndex]);
	packed.z = RenderBucket_ModelVertexZ(ctx, &ctx->stack[stackIndex]);
	ctx->packedStack[stackIndex] = packed;
	*scratchVertex = packed;
	return packed;
}

static struct RenderBucketPackedVertex RenderBucket_CacheInterpolatedPackedVertex(struct RenderBucketDrawContext *ctx, u16 stackIndex,
                                                                                  const RenderBucketVertex *curr, const RenderBucketVertex *next)
{
	struct RenderBucketPackedVertex *scratchVertex = RenderBucket_PackedVertexScratch(stackIndex);
	struct RenderBucketPackedVertex packed;

	packed.xy = RenderBucket_PackInterpolatedModelVertexXY(ctx, curr, next);
	packed.z = RenderBucket_InterpolatedModelVertexZ(ctx, curr, next);
	ctx->packedStack[stackIndex] = packed;
	*scratchVertex = packed;
	return packed;
}

static void RenderBucket_CopyScratchColorCache(struct RenderBucketDrawContext *ctx)
{
	u32 *scratchColor = RenderBucket_ColorCacheScratch();
	u32 *commandList = (u32 *)ctx->idpp->ptrCommandList;
	u32 *colorLayout = (u32 *)ctx->idpp->ptrColorLayout;
	u32 count = commandList[0];

	// NOTE(aalhendi): Retail Execute copies ptrColorLayout to scratchpad 0x140
	// before DrawFunc_Normal so UncompressAnimationFrame can service command
	// bit 0x08000000 from the same cache.
	{
		u32 i = 0;
		for (; i < count; i++)
		{
			scratchColor[i] = colorLayout[i];
		}
	}
}

static int RenderBucket_GetCommandColor(struct RenderBucketDrawContext *ctx, u32 command)
{
	u32 *colorLayout = (u32 *)ctx->idpp->ptrColorLayout;
	u32 colorOffset = (command >> 7) & 0x1fc;

	if ((s32)(command << 4) < 0)
	{
		return RenderBucket_ColorCacheScratch()[colorOffset / sizeof(u32)];
	}

	return RenderBucket_ReadPackedWord((const u8 *)colorLayout + colorOffset);
}

static int RenderBucket_GetIndexedColor(struct RenderBucketDrawContext *ctx, u32 colorOffset, int useScratch)
{
	if (useScratch != 0)
	{
		return RenderBucket_ColorCacheScratch()[colorOffset / sizeof(u32)];
	}

	return RenderBucket_ReadPackedWord((const u8 *)ctx->idpp->ptrColorLayout + colorOffset);
}

static void RenderBucket_ApplyColorOnlyCommand(struct RenderBucketDrawContext *ctx, u32 command)
{
	int colorA = RenderBucket_GetIndexedColor(ctx, (command >> 7) & 0x1fc, command & 1);
	int colorB = RenderBucket_GetIndexedColor(ctx, command & 0x1fc, command & 2);

	// NOTE(aalhendi): Source-backs DrawFunc_Normal's high-16-zero color-only
	// command path at 0x8006a620-0x8006a674.
	ctx->tempColor[1] = colorA;
	ctx->tempColor[2] = colorA;
	ctx->tempColor[3] = colorB;
}

static int RenderBucket_ReadDeltaComponentFromStream(const u8 *vertData, int *bitIndex, u8 bits, int temporalBase, int *accum)
{
	int value = RenderBucket_GetSignedBits(vertData, bitIndex, bits + 1);

	if (bits == 7)
	{
		*accum = RenderBucket_SignedByte(value);
	}
	else
	{
		*accum = RenderBucket_SignedByte(*accum + value + temporalBase);
	}

	return *accum;
}

static int RenderBucket_ReadDeltaComponent(struct RenderBucketDrawContext *ctx, u8 bits, int temporalBase, int *accum)
{
	return RenderBucket_ReadDeltaComponentFromStream((const u8 *)ctx->vertData, &ctx->bitIndex, bits, temporalBase, accum);
}

static void RenderBucket_ReadNextFrameDeltaComponent(struct RenderBucketDrawContext *ctx, u8 bits, int temporalBase, int *currAccum, int *nextAccum)
{
	RenderBucket_ReadDeltaComponentFromStream((const u8 *)ctx->vertData, &ctx->bitIndex, bits, temporalBase, currAccum);
	RenderBucket_ReadDeltaComponentFromStream((const u8 *)ctx->nextVertData, &ctx->nextBitIndex, bits, temporalBase, nextAccum);
}

#ifndef CTR_NATIVE
// TODO(aalhendi): Keep the C context decoder until the remaining renderer paths
// are ported. The public PSX entry uses registers, not this struct-return ABI.
#define RenderBucket_UncompressAnimationFrame RenderBucket_UncompressAnimationFrame_Context
static
#endif
    struct RenderBucketUncompressResult RenderBucket_UncompressAnimationFrame(struct RenderBucketDrawContext *ctx, u32 command, u16 stackIndex)
{
	struct RenderBucketUncompressResult result;
	u32 *deltaArray = (u32 *)ctx->idpp->ptrDeltaArray;
	u8 flags = (command >> 24) & 0xff;

	if ((flags & 4) != 0)
	{
		struct RenderBucketPackedVertex *scratchVertex = RenderBucket_PackedVertexScratch(stackIndex);

		// Command bit 0x04000000 returns the cached packed vertex.
		result.packed = *scratchVertex;
		result.color = RenderBucket_GetCommandColor(ctx, command);
		ctx->packedStack[stackIndex] = result.packed;
		return result;
	}

	if (deltaArray != 0)
	{
		u32 temporal = deltaArray[ctx->vertexIndex];
		u8 xBits = (temporal >> 6) & 7;
		u8 zBits = (temporal >> 3) & 7;
		u8 yBits = temporal & 7;
		int bx = RenderBucket_SignExtendBits(temporal >> 25, 7) * 2;
		int bz = RenderBucket_SignExtendBits(temporal >> 17, 8);
		int by = RenderBucket_SignExtendBits(temporal >> 9, 8);

		// NOTE(aalhendi): Retail publishes X/Z/Y byte accumulators in scratch.
		// An 8-bit field resets its accumulator without adding the temporal base.
		RenderBucket_ReadDeltaComponent(ctx, xBits, bx, &ctx->x_alu);
		*CTR_SCRATCHPAD_PTR(u8, 0x58) = (u8)ctx->x_alu;
		RenderBucket_ReadDeltaComponent(ctx, zBits, bz, &ctx->y_alu);
		*CTR_SCRATCHPAD_PTR(u8, 0x5a) = (u8)ctx->y_alu;
		RenderBucket_ReadDeltaComponent(ctx, yBits, by, &ctx->z_alu);
		*CTR_SCRATCHPAD_PTR(u8, 0x59) = (u8)ctx->z_alu;

		ctx->stack[stackIndex].x = ctx->x_alu;
		ctx->stack[stackIndex].y = ctx->z_alu;
		ctx->stack[stackIndex].z = ctx->y_alu;
	}
	else
	{
		RenderBucketCompVertex *ptrVerts = (RenderBucketCompVertex *)ctx->vertData;

		ctx->stack[stackIndex].x = ptrVerts[ctx->vertexIndex].x;
		ctx->stack[stackIndex].y = ptrVerts[ctx->vertexIndex].y;
		ctx->stack[stackIndex].z = ptrVerts[ctx->vertexIndex].z;
	}

	result.packed = RenderBucket_CachePackedVertex(ctx, stackIndex);
	result.color = RenderBucket_GetCommandColor(ctx, command);
	return result;
}

#ifdef CTR_NATIVE
#include "RenderBucket_UncompressAnimationFrame_NextFrame.c"

static struct RenderBucketUncompressResult RenderBucket_TransformSplitDecodedVertex(struct RenderBucketDrawContext *ctx, u32 command, u16 stackIndex,
                                                                                    struct RenderBucketUncompressResult result)
{
	u8 flags = (command >> 24) & 0xff;
	struct RenderBucketPackedVertex *scratchVertex = RenderBucket_PackedVertexScratch(stackIndex);

	if ((flags & 4) != 0)
	{
		return result;
	}

	// The split transform tail at 0x8006bf30-0x8006c124 and
	// 0x8006cdec-0x8006d094 sends packed VXY/VZ through MVMVA.
	MTC2(result.packed.xy, 0);
	MTC2(result.packed.z, 1);
	doCOP2(0x04a6012);
	result.packed.xy = ((u32)MFC2(10) << 16) | ((u32)MFC2(9) & 0xffff);
	result.packed.z = MFC2(11);
	ctx->packedStack[stackIndex] = result.packed;
	*scratchVertex = result.packed;
	// NOTE(aalhendi): Scratch colors can alias packed coordinates. Retail
	// selects color after storing the transformed vertex.
	result.color = RenderBucket_GetCommandColor(ctx, command);
	return result;
}

static struct RenderBucketUncompressResult RenderBucket_UncompressAnimationFrame_Split(struct RenderBucketDrawContext *ctx, u32 command, u16 stackIndex)
{
	return RenderBucket_TransformSplitDecodedVertex(ctx, command, stackIndex, RenderBucket_UncompressAnimationFrame(ctx, command, stackIndex));
}

static struct RenderBucketUncompressResult RenderBucket_UncompressAnimationFrame_ReflectNextFrame(struct RenderBucketDrawContext *ctx, u32 command,
                                                                                                  u16 stackIndex)
{
	// Decode through 0x8006b24c semantics, then apply the split transform.
	return RenderBucket_TransformSplitDecodedVertex(ctx, command, stackIndex, RenderBucket_UncompressAnimationFrame_NextFrame(ctx, command, stackIndex));
}

static struct RenderBucketUncompressResult RenderBucket_DispatchUncompressAnimationFrame(struct RenderBucketDrawContext *ctx, u32 command, u16 stackIndex)
{
	switch (ctx->idpp->unkF0)
	{
	case RB_RETAIL_UNCOMPRESS_NEXTFRAME:
		return RenderBucket_UncompressAnimationFrame_NextFrame(ctx, command, stackIndex);

	case RB_RETAIL_UNCOMPRESS_SPLIT:
		return RenderBucket_UncompressAnimationFrame_Split(ctx, command, stackIndex);

	case RB_RETAIL_UNCOMPRESS_REFLECT:
		return RenderBucket_UncompressAnimationFrame_ReflectNextFrame(ctx, command, stackIndex);

	default:
		return RenderBucket_UncompressAnimationFrame(ctx, command, stackIndex);
	}
}
#endif

static u32 *RenderBucket_GetNormalOTEntry(int activeRange, int depthMac0)
{
	int depthBin = (int)((u32)depthMac0 >> 17);

	if (activeRange == 0)
	{
		return 0;
	}

	// NOTE(aalhendi): Source-backs DrawInstPrim_Normal's active-range +
	// (MAC0 >> 17) OT lookup at 0x8006ad88-0x8006ad98. Retail trusts QueueDraw's
	// range producer here; native intentionally does not clamp to depthOffset
	// because that would mask producer/consumer depth mismatches.
	return (u32 *)activeRange + depthBin;
}

static u32 *RenderBucket_GetClampedOTEntry(struct RenderBucketDrawContext *ctx, int activeRange, int depthMac0)
{
	struct RenderBucketExecuteScratch *normalScratch = RenderBucket_Scratch();
	u32 normalOtAddress;
	u32 word;
	u32 normalWork0;
	u32 otBase;
	s32 work2;
	(void)ctx;

	if (activeRange == 0)
	{
		return 0;
	}

	normalOtAddress = (u32)depthMac0;
	word = (u32)(s32)normalScratch->minDepthByteOffset;
	normalWork0 = (u32)(s32)normalScratch->maxDepthByteOffset;
	otBase = (u32)activeRange;
#include "RenderBucket_ClampOT.inc"

	// NOTE(aalhendi): Execute supplies signed low16 byte offsets at 0x134/136;
	// the retail writer clamps after scaling, not against the unscaled IDPP.
	return (u32 *)normalOtAddress;
}

static void RenderBucket_StoreWriterInputs(struct RenderBucketDrawContext *ctx, struct TextureLayout *tex, int activeRange, int depthMac0)
{
	struct RenderBucketExecuteScratch *scratch = RenderBucket_Scratch();

	// NOTE(aalhendi): These are the live caller-owned scratch/register inputs
	// at the writer boundary, not additional packet output or a second view.
	scratch->depthMac0 = (u32)depthMac0;
	scratch->activeOTPtr32 = (u32)activeRange;
	scratch->primaryOTPtr32 = (u32)ctx->idpp->otRangeNormal;
	scratch->secondaryOTPtr32 = (u32)ctx->idpp->otRangeSecondary;
	scratch->texturePtr32 = (u32)tex;
	scratch->primitiveState.split.alphaScale = ctx->idpp->alphaScale;
}

static int RenderBucket_TriangleInScreenWindow(struct RenderBucketDrawContext *ctx)
{
	u32 screen = RenderBucket_PackXY(ctx->pb->rect.w, ctx->pb->rect.h);
	u32 sxy0 = MFC2(12);
	u32 sxy1 = MFC2(13);
	u32 sxy2 = MFC2(14);
	u32 allNegative = sxy0 & sxy1 & sxy2;
	u32 allPastScreen = ~(((sxy0 - screen) | (sxy1 - screen) | (sxy2 - screen)));
	u32 reject = allNegative | allPastScreen;

	if ((s32)reject < 0)
	{
		return 0;
	}

	if ((s32)(reject << 16) < 0)
	{
		return 0;
	}

	return 1;
}

static void RenderBucket_LoadPrimRTPT(struct RenderBucketDrawContext *ctx)
{
	// NOTE(aalhendi): Retail UncompressAnimationFrame returns packed GTE-ready
	// coordinates after adding the frame origin and shifting left by 2.
	MTC2(ctx->tempPacked[1].xy, 0);
	MTC2(ctx->tempPacked[1].z, 1);
	MTC2(ctx->tempPacked[2].xy, 2);
	MTC2(ctx->tempPacked[2].z, 3);
	MTC2(ctx->tempPacked[3].xy, 4);
	MTC2(ctx->tempPacked[3].z, 5);

	gte_rtpt();
}

static void RenderBucket_LoadPrimRTPS(struct RenderBucketDrawContext *ctx, int reuseFirstVertex)
{
	if (reuseFirstVertex != 0)
	{
		u32 sxy0 = MFC2(12);
		u32 sz1 = MFC2(17);

		// NOTE(aalhendi): Source-backs DrawFunc_Normal's bit30 continuation
		// sequence at 0x8006a680-0x8006a690: copy SXY0/SZ1 into the RTPS FIFO
		// before projecting the new vertex.
		MTC2(sxy0, 13);
		MTC2(sz1, 18);
	}

	MTC2(ctx->tempPacked[3].xy, 0);
	MTC2(ctx->tempPacked[3].z, 1);

	gte_rtps();
}

static void RenderBucket_StoreProjectedRegs(struct RenderBucketProjectedRegs *regs)
{
	regs->sxy0 = MFC2(12);
	regs->sz1 = MFC2(17);
	regs->sxy1 = MFC2(13);
	regs->sz2 = MFC2(18);
	regs->sxy2 = MFC2(14);
	regs->sz3 = MFC2(19);
}

static void RenderBucket_LoadProjectedRegs(const struct RenderBucketProjectedRegs *regs)
{
	MTC2(regs->sxy0, 12);
	MTC2(regs->sz1, 17);
	MTC2(regs->sxy1, 13);
	MTC2(regs->sz2, 18);
	MTC2(regs->sxy2, 14);
	MTC2(regs->sz3, 19);
}

static int RenderBucket_RestoreProjectedRegsAndReturn(const struct RenderBucketProjectedRegs *regs, int ret)
{
	RenderBucket_LoadProjectedRegs(regs);
	return ret;
}

static int RenderBucket_CheckProjectedPrim(struct RenderBucketDrawContext *ctx, u32 command, u32 gteFlag, u16 cullXorMask, int *depthMac0Out)
{
	int depthMac0;

	// NOTE(aalhendi): Source-backs the shared retail projection gate:
	// flag reject, optional NCLIP, AVSZ3, screen-window reject, then MAC0.
	if ((s32)(gteFlag << 13) < 0)
	{
		return 0;
	}

	if ((s32)(command << 3) < 0)
	{
		int opZ;
		int cullXor;
		s16 cullFlags = (s16)((u16)ctx->idpp->instFlags ^ cullXorMask);

		gte_nclip();
		gte_stopz(&opZ);
		if (opZ == 0)
		{
			return 0;
		}

		cullXor = (s32)cullFlags ^ (s32)(command << 2);
		if ((s32)((u32)opZ ^ (u32)cullXor) <= 0)
		{
			return 0;
		}
	}

	gte_avsz3();

	if (RenderBucket_TriangleInScreenWindow(ctx) == 0)
	{
		return 0;
	}

	gte_stopz(&depthMac0);
	*depthMac0Out = depthMac0;
	return 1;
}

static void RenderBucket_InitSplitVertex(struct RenderBucketDrawContext *ctx, int slot, u32 packedXY, u32 packedZ, int color)
{
	struct RenderBucketSplitVertex *split = &ctx->tempSplit[slot];
	int splitDist = (s16)ctx->splitPlane - (s32)packedZ;

	split->xy = packedXY;
	split->z = packedZ;
	split->color = (u32)color;
	split->uv = 0;
	split->splitDist = (s16)splitDist;
	split->sxy = 0;
	split->sz = 0;
}

static void RenderBucket_InitWaterSplitVertex(struct RenderBucketDrawContext *ctx, int slot, u32 packedXY, u32 packedZ, int color)
{
	struct RenderBucketSplitVertex *split = &ctx->tempSplit[slot];
	int splitDist = (s16)ctx->idpp->splitLine - (s16)(packedXY >> 16);

	split->xy = packedXY;
	split->z = packedZ;
	split->color = (u32)color;
	split->uv = 0;
	split->splitDist = (s16)splitDist;
	split->sxy = 0;
	split->sz = 0;
}

static void RenderBucket_StoreSplitProjectedRegs(struct RenderBucketDrawContext *ctx)
{
	ctx->tempSplit[1].sxy = MFC2(12);
	ctx->tempSplit[1].sz = MFC2(17);
	ctx->tempSplit[2].sxy = MFC2(13);
	ctx->tempSplit[2].sz = MFC2(18);
	ctx->tempSplit[3].sxy = MFC2(14);
	ctx->tempSplit[3].sz = MFC2(19);
}

static struct TextureLayout *RenderBucket_GetCommandTexture(struct RenderBucketDrawContext *ctx, u32 command, int *isValid)
{
	u16 texIndex = command & 0x1ff;

	*isValid = 1;

	if (texIndex == 0)
	{
		return 0;
	}

	if (ctx->idpp->ptrTexLayout == 0)
	{
		*isValid = 0;
		return 0;
	}

	// NOTE(aalhendi): Retail only uses texture index zero as the explicit G3
	// path, but a null texture-table entry also reaches DrawInstPrim_Normal as
	// `a2 == 0` and emits G3. Do not reject that case here.
	return ctx->idpp->ptrTexLayout[texIndex - 1];
}

static int RenderBucket_OTEntryPassesDpctGate(const u32 *otEntry)
{
	u32 addr = RenderBucket_OTAddress((void *)otEntry);

	// NOTE(aalhendi): Retail tests the post-depth-bin OT entry register with
	// `sll t0,t3,7` before DPCT. Native models that with the 24-bit OT address
	// domain used by primitive tags, not host pointer high bits.
	return (s32)(addr << 7) >= 0;
}

static u8 RenderBucket_SaturateU8(int value)
{
	if (value > 0xff)
	{
		return 0xff;
	}

	if (value < 0)
	{
		return 0;
	}

	return (u8)value;
}

#ifdef CTR_NATIVE
#include "RenderBucket_DrawInstPrim_Normal.c"
#include "RenderBucket_DrawInstPrim_DepthFade.c"
#endif

#include "RenderBucket_Writers.c"

#ifndef CTR_NATIVE
static int RenderBucket_RunContextWriter(struct RenderBucketDrawContext *ctx, u32 command, const u8 *entry)
{
	if ((char *)ctx->primMem->cursor + sizeof(POLY_GT3) >= (char *)ctx->primMem->guardEnd)
	{
		return -1;
	}
	ctx->primMem->cursor =
	    (void *)RenderBucket_RunWriter((u32)ctx->primMem->cursor, command, (u32)ctx->tempColor[1], (u32)ctx->tempColor[2], (u32)ctx->tempColor[3], entry);
	return 0;
}
#endif

static int RenderBucket_DrawInstPrim_NormalAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange, int depthMac0)
{
	RenderBucket_StoreWriterInputs(ctx, tex, activeRange, depthMac0);
#ifdef CTR_NATIVE
	(void)command;
	return RenderBucket_DrawInstPrim_NormalAtOTEntry(ctx, tex, RenderBucket_GetNormalOTEntry(activeRange, depthMac0));
#else
	return RenderBucket_RunContextWriter(ctx, command, RenderBucket_NormalInner);
#endif
}

static int RenderBucket_DrawInstPrim_KeyRelicTokenAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                          int depthMac0)
{
	u32 *otEntry;
	int signedTest;
	u32 litColor;
	int add;
	u32 r;
	u32 g;
	u32 b;
	u32 texWord1;
	u32 codeWord;
	u32 tpageMask;

	if ((char *)ctx->primMem->cursor + sizeof(POLY_FT3) >= (char *)ctx->primMem->guardEnd)
	{
		return -1;
	}

	// NOTE(aalhendi): Retail only routes textured key/relic/token models here.
	// Native keeps the dispatch host-safe if malformed instance data reaches it.
	if (tex == 0)
	{
		return 0;
	}

	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
	{
		return 0;
	}

	// Retail inputs live in scratch/GTE registers; native passes the same state
	// through RenderBucketDrawContext and the current projected GTE FIFO.
	gte_nclip();
	signedTest = MFC2_S(24) ^ ((s16)ctx->idpp->instFlags ^ (int)(command << 2));
	if (signedTest < 0)
	{
		otEntry++;
	}

	{
		POLY_FT3 *p = ctx->primMem->cursor;
		CTR_GteStoreSXY3(&p->x0, &p->x1, &p->x2);

		{
			u32 sourceColor = (u32)ctx->tempColor[1];
			MTC2((s32)(sourceColor << 24) >> 19, 9);
			MTC2((s32)((sourceColor >> 8) << 24) >> 19, 10);
			MTC2((s32)((sourceColor >> 16) << 24) >> 19, 11);
			gte_lcir();

			{
				int brightness = MFC2_S(9) + 0x1000;
				int distance = MFC2_S(11) >> 3;
				brightness = (((brightness * 7) + 0x2000) >> 4);
				if (signedTest < 0)
				{
					brightness >>= 1;
				}

				MTC2(brightness, 8);
				gte_dpcs();

				if (distance < 0)
				{
					distance = -distance;
				}

				add = 0;
				distance -= 0x180;
				if (distance >= 0)
				{
					if (distance >= 0x80)
					{
						distance = 0x7f;
					}

					add = sRenderBucketKeyRelicBrightness8008a2c4[distance];
				}

				litColor = (u32)MFC2(22);
				if (signedTest < 0)
				{
					add >>= 3;
				}

				r = RenderBucket_SaturateU8((litColor & 0xff) + add);
				g = RenderBucket_SaturateU8(((litColor >> 8) & 0xff) + add);
				b = RenderBucket_SaturateU8(((litColor >> 16) & 0xff) + add);

				texWord1 = RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD1_OFFSET);
				codeWord = 0x24000000;
				tpageMask = 0x00600000;
				if (((texWord1 & 0x00600000) == 0) && (signedTest >= 0))
				{
					codeWord = 0x26000000;
					tpageMask = 0x00200000;
				}

				CtrGpu_WriteColorCode(&p->r0, codeWord | (b << 16) | (g << 8) | r);
				CtrGpu_WritePackedUVWord(&p->u0, RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD0_OFFSET));
				CtrGpu_WritePackedUVWord(&p->u1, texWord1 | tpageMask);
				CtrGpu_WritePackedUVWord(&p->u2, RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD2_OFFSET));

				RenderBucket_LinkPrimRaw(otEntry, p, 0x07000000);
				ctx->primMem->cursor = (char *)p + 0x20;
				return 0;
			}
		}
	}
}

static int RenderBucket_DrawInstPrim_KeyRelicToken(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	return RenderBucket_DrawInstPrim_KeyRelicTokenAtRange(ctx, command, tex, ctx->idpp->otRangeNormal, depthMac0);
}

#ifdef CTR_NATIVE
int RenderBucket_DrawInstPrim_Normal(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	return RenderBucket_DrawInstPrim_NormalAtRange(ctx, command, tex, ctx->idpp->otRangeNormal, depthMac0);
}
#endif

static int RenderBucket_DrawInstPrim_SelectRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	int activeRange = ((s32)(command << 6) > 0) ? ctx->idpp->otRangeNormal : ctx->idpp->otRangeSecondary;

#ifdef CTR_NATIVE
	return RenderBucket_DrawInstPrim_NormalAtRange(ctx, command, tex, activeRange, depthMac0);
#else
	RenderBucket_StoreWriterInputs(ctx, tex, activeRange, depthMac0);
	return RenderBucket_RunContextWriter(ctx, command, RenderBucket_DrawInstPrim_Normal);
#endif
}

static int RenderBucket_DrawInstPrim_DepthFadeAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                      int depthMac0)
{
#ifdef CTR_NATIVE
	u32 *otEntry;
#endif

	RenderBucket_StoreWriterInputs(ctx, tex, activeRange, depthMac0);
#ifdef CTR_NATIVE
	(void)command;
	// NOTE(aalhendi): Keep native host-safe for missing texture/OT resources,
	// after publishing the same scratch inputs as retail.
	if (tex == 0)
		return 0;
	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
		return 0;
	return RenderBucket_DrawInstPrim_DepthFadeAtOTEntry(ctx, tex, otEntry);
#else
	if (tex == 0 || activeRange == 0)
		return 0;
	return RenderBucket_RunContextWriter(ctx, command, RenderBucket_DrawInstPrim_DepthFade);
#endif
}

static int RenderBucket_DrawInstPrim_ClampDepthAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                       int depthMac0)
{
	RenderBucket_StoreWriterInputs(ctx, tex, activeRange, depthMac0);
#ifdef CTR_NATIVE
	(void)command;
	return RenderBucket_DrawInstPrim_NormalAtOTEntry(ctx, tex, RenderBucket_GetClampedOTEntry(ctx, activeRange, depthMac0));
#else
	return RenderBucket_RunContextWriter(ctx, command, RenderBucket_DrawInstPrim_ClampDepth);
#endif
}

#ifdef CTR_NATIVE
static int RenderBucket_DrawInstPrim_ClampDepth(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	return RenderBucket_DrawInstPrim_ClampDepthAtRange(ctx, command, tex, ctx->idpp->otRangeNormal, depthMac0);
}
#endif

static int RenderBucket_DrawInstPrim_LitTextureAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                       int depthMac0)
{
	u32 *otEntry;
	int signedTest;
	u32 sourceColor;
	u32 litColor;
	int add;
	int brightness;
	int distance;
	u32 r;
	u32 g;
	u32 b;
	u32 texWord1;
	u32 codeWord;
	u32 tpageMask;
	POLY_FT3 *p;

	if ((char *)ctx->primMem->cursor + sizeof(POLY_FT3) >= (char *)ctx->primMem->guardEnd)
	{
		return -1;
	}

	if (tex == 0)
	{
		return 0;
	}

	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
	{
		return 0;
	}

	// NOTE(aalhendi): Source-backs retail primitive writer 0x8006c778-0x8006c920.
	// It derives side from NCLIP, lights the first rolling RGB word, then emits
	// a textured FT3 with side-dependent code/tpage bits.
	gte_nclip();
	signedTest = MFC2_S(24) ^ ((s16)ctx->idpp->instFlags ^ (int)(command << 2));
	if (signedTest < 0)
	{
		otEntry++;
	}

	sourceColor = (u32)ctx->tempColor[1];
	MTC2((s32)(sourceColor << 24) >> 19, 9);
	MTC2((s32)((sourceColor >> 8) << 24) >> 19, 10);
	MTC2((s32)((sourceColor >> 16) << 24) >> 19, 11);
	gte_lcir();

	brightness = MFC2_S(9) + 0x1000;
	distance = MFC2_S(11) >> 3;
	brightness = (((brightness * 7) + 0x2000) >> 4);
	if (signedTest < 0)
	{
		brightness >>= 1;
	}

	MTC2(brightness, 8);
	gte_dpcs();

	if (distance < 0)
	{
		distance = -distance;
	}

	add = 0;
	distance -= 0x180;
	if (distance >= 0)
	{
		if (distance >= 0x80)
		{
			distance = 0x7f;
		}

		add = sRenderBucketKeyRelicBrightness8008a2c4[distance];
	}

	litColor = (u32)MFC2(22);
	if (signedTest < 0)
	{
		add >>= 3;
	}

	r = RenderBucket_SaturateU8((litColor & 0xff) + add);
	g = RenderBucket_SaturateU8(((litColor >> 8) & 0xff) + add);
	b = RenderBucket_SaturateU8(((litColor >> 16) & 0xff) + add);

	texWord1 = RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD1_OFFSET);
	if ((texWord1 & 0x00600000U) != 0)
	{
		u32 otSide = RenderBucket_OTAddress(otEntry) << 3;

		// NOTE(aalhendi): Retail tests raw OT pointer bits here. Native uses the
		// 24-bit OT/tag address domain so host pointer high bits cannot affect
		// primitive visibility.
		if ((s32)(otSide & (u32)signedTest) < 0)
		{
			return 0;
		}

		codeWord = 0x24000000;
		tpageMask = 0x00600000;
	}
	else if (signedTest < 0)
	{
		codeWord = 0x26000000;
		tpageMask = 0x00400000;
	}
	else
	{
		codeWord = 0x26000000;
		tpageMask = 0x00200000;
	}

	p = ctx->primMem->cursor;
	CtrGpu_WriteColorCode(&p->r0, codeWord | (b << 16) | (g << 8) | r);
	CTR_GteStoreSXY3(&p->x0, &p->x1, &p->x2);
	CtrGpu_WritePackedUVWord(&p->u0, RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD0_OFFSET));
	CtrGpu_WritePackedUVWord(&p->u1, texWord1 | tpageMask);
	CtrGpu_WritePackedUVWord(&p->u2, RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD2_OFFSET));

	RenderBucket_LinkPrimRaw(otEntry, p, 0x07000000);
	ctx->primMem->cursor = (char *)p + 0x20;
	return 0;
}

static int RenderBucket_DrawInstPrim_LitTexture(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	return RenderBucket_DrawInstPrim_LitTextureAtRange(ctx, command, tex, ctx->idpp->otRangeNormal, depthMac0);
}

static int RenderBucket_DrawInstPrim_GhostAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange, int depthMac0)
{
	u32 *otEntry;
	int alpha = ctx->idpp->alphaScale;
	struct RenderBucketGhostMaskPacket *mask;

	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
	{
		return 0;
	}

	// Alpha-zero ghosts tail-call the normal primitive writer at retail
	// 0x8006adc8, which is represented by the shared native normal writer.
	MTC2(ctx->tempColor[1], 20);
	MTC2(ctx->tempColor[2], 21);
	MTC2(ctx->tempColor[3], 22);
	MTC2(alpha, 8);

	if (alpha == 0)
	{
		return RenderBucket_DrawInstPrim_NormalAtRange(ctx, command, tex, activeRange, depthMac0);
	}

	if ((char *)ctx->primMem->cursor + 0x40 >= (char *)ctx->primMem->guardEnd)
	{
		return -1;
	}

	gte_dpct();

	mask = (struct RenderBucketGhostMaskPacket *)ctx->primMem->cursor;
	mask->drawMode = 0xe1000a40;
	mask->pad = 0;
	mask->colorAndCode = RenderBucket_Scratch()->primitiveState.split.fadeColor.word;
	mask->xy0 = (u32)MFC2(12);
	mask->xy1 = (u32)MFC2(13);
	mask->xy2 = (u32)MFC2(14);

	if (tex == 0)
	{
		struct RenderBucketGhostFlatPacket *packet = (struct RenderBucketGhostFlatPacket *)mask;

		packet->drawMode = 0xe1000a20;
		packet->pad = 0;
		packet->body.color0AndCode = 0x32000000 | (u32)MFC2(20);
		packet->body.xy0 = (u32)MFC2(12);
		packet->body.color1 = (u32)MFC2(21);
		packet->body.xy1 = (u32)MFC2(13);
		packet->body.color2 = (u32)MFC2(22);
		packet->body.xy2 = (u32)MFC2(14);

		RenderBucket_LinkPrimRaw(otEntry, packet, 0x0e000000);
		ctx->primMem->cursor = packet + 1;
	}
	else
	{
		struct RenderBucketGhostTexturedPacket *packet = (struct RenderBucketGhostTexturedPacket *)mask;
		u32 texWord1 = (RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD1_OFFSET) & ~0x00600000U) | 0x00200000U;

		packet->body.color0AndCode = 0x36000000 | (u32)MFC2(20);
		packet->body.xy0 = (u32)MFC2(12);
		packet->body.uv0 = RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD0_OFFSET);
		packet->body.color1 = (u32)MFC2(21);
		packet->body.xy1 = (u32)MFC2(13);
		packet->body.uv1 = texWord1;
		packet->body.color2 = (u32)MFC2(22);
		packet->body.xy2 = (u32)MFC2(14);
		packet->body.uv2 = RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD2_OFFSET);

		RenderBucket_LinkPrimRaw(otEntry, packet, 0x0f000000);
		ctx->primMem->cursor = packet + 1;
	}

	return 0;
}

static int RenderBucket_DrawInstPrim_Ghost(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	return RenderBucket_DrawInstPrim_GhostAtRange(ctx, command, tex, ctx->idpp->otRangeNormal, depthMac0);
}

static int RenderBucket_DispatchDrawInstPrimAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange, int depthMac0)
{
	switch ((u32)(u32)ctx->inst->funcPtr[1])
	{
	case RB_RETAIL_INST_PRIM_SELECT_RANGE:
		return RenderBucket_DrawInstPrim_SelectRange(ctx, command, tex, depthMac0);

	case RB_RETAIL_INST_PRIM_NORMAL:
		return RenderBucket_DrawInstPrim_NormalAtRange(ctx, command, tex, activeRange, depthMac0);

	case RB_RETAIL_INST_PRIM_DEPTH_FADE:
		return RenderBucket_DrawInstPrim_DepthFadeAtRange(ctx, command, tex, activeRange, depthMac0);

	case RB_RETAIL_INST_PRIM_KEY_TOKEN:
		return RenderBucket_DrawInstPrim_KeyRelicTokenAtRange(ctx, command, tex, activeRange, depthMac0);

	case RB_RETAIL_INST_PRIM_CLAMP_DEPTH:
		return RenderBucket_DrawInstPrim_ClampDepthAtRange(ctx, command, tex, activeRange, depthMac0);

	case RB_RETAIL_INST_PRIM_LIT_TEXTURE:
		return RenderBucket_DrawInstPrim_LitTextureAtRange(ctx, command, tex, activeRange, depthMac0);

	case RB_RETAIL_INST_PRIM_GHOST:
		return RenderBucket_DrawInstPrim_GhostAtRange(ctx, command, tex, activeRange, depthMac0);

	default:
		// TODO(aalhendi): Port any newly observed Instance+0x60 primitive writers.
		// Do not draw through the normal writer here; that masks live retail rows.
		return 0;
	}
}

static int RenderBucket_DispatchDrawInstPrim(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	switch ((u32)(u32)ctx->inst->funcPtr[1])
	{
	case RB_RETAIL_INST_PRIM_SELECT_RANGE:
		return RenderBucket_DrawInstPrim_SelectRange(ctx, command, tex, depthMac0);

	case RB_RETAIL_INST_PRIM_NORMAL:
#ifdef CTR_NATIVE
		return RenderBucket_DrawInstPrim_Normal(ctx, command, tex, depthMac0);
#else
		return RenderBucket_DrawInstPrim_NormalAtRange(ctx, command, tex, ctx->idpp->otRangeNormal, depthMac0);
#endif

	case RB_RETAIL_INST_PRIM_DEPTH_FADE:
		return RenderBucket_DrawInstPrim_DepthFadeAtRange(ctx, command, tex, ctx->idpp->otRangeNormal, depthMac0);

	case RB_RETAIL_INST_PRIM_KEY_TOKEN:
		return RenderBucket_DrawInstPrim_KeyRelicToken(ctx, command, tex, depthMac0);

	case RB_RETAIL_INST_PRIM_CLAMP_DEPTH:
#ifdef CTR_NATIVE
		return RenderBucket_DrawInstPrim_ClampDepth(ctx, command, tex, depthMac0);
#else
		return RenderBucket_DrawInstPrim_ClampDepthAtRange(ctx, command, tex, ctx->idpp->otRangeNormal, depthMac0);
#endif

	case RB_RETAIL_INST_PRIM_LIT_TEXTURE:
		return RenderBucket_DrawInstPrim_LitTexture(ctx, command, tex, depthMac0);

	case RB_RETAIL_INST_PRIM_GHOST:
		return RenderBucket_DrawInstPrim_Ghost(ctx, command, tex, depthMac0);

	default:
		// TODO(aalhendi): Port any newly observed Instance+0x60 primitive writers.
		// Do not draw through the normal writer here; that masks live retail rows.
		return 0;
	}
}

static int RenderBucket_SelectPrimitiveActiveRange(struct RenderBucketDrawContext *ctx, u32 command)
{
	if ((u32)(u32)ctx->inst->funcPtr[1] == RB_RETAIL_INST_PRIM_SELECT_RANGE)
	{
		return ((s32)(command << 6) > 0) ? ctx->idpp->otRangeNormal : ctx->idpp->otRangeSecondary;
	}

	return ctx->idpp->otRangeNormal;
}

static void RenderBucket_AssignSplitUvs(struct RenderBucketDrawContext *ctx, const struct TextureLayout *tex)
{
	if (tex == 0)
	{
		return;
	}

	ctx->tempSplit[1].uv = (u16)tex->u0 | ((u16)tex->v0 << 8);
	ctx->tempSplit[2].uv = (u16)tex->u1 | ((u16)tex->v1 << 8);
	ctx->tempSplit[3].uv = (u16)tex->u2 | ((u16)tex->v2 << 8);
}

static void RenderBucket_LoadSplitPrimColors(struct RenderBucketDrawContext *ctx, const u32 *otEntry, u32 color0, u32 color1, u32 color2)
{
	s16 alpha = ctx->idpp->alphaScale;

	MTC2(color0, 20);
	MTC2(color1, 21);
	MTC2(color2, 22);
	MTC2(alpha, 8);

	if ((alpha != 0) && (RenderBucket_OTEntryPassesDpctGate(otEntry) != 0))
	{
		gte_dpct();
	}
}

static int RenderBucket_SplitPrimitiveWriterSupported(struct RenderBucketDrawContext *ctx)
{
	u32 prim = (u32)(u32)ctx->inst->funcPtr[1];

	return (prim == RB_RETAIL_INST_PRIM_NORMAL) || (prim == RB_RETAIL_INST_PRIM_SELECT_RANGE) || (prim == RB_RETAIL_INST_PRIM_DEPTH_FADE) ||
	       (prim == RB_RETAIL_INST_PRIM_KEY_TOKEN) || (prim == RB_RETAIL_INST_PRIM_CLAMP_DEPTH) || (prim == RB_RETAIL_INST_PRIM_LIT_TEXTURE) ||
	       (prim == RB_RETAIL_INST_PRIM_GHOST);
}

static u32 RenderBucket_TextureWordWithSplitUv(u32 texWord, u16 uv)
{
	return (texWord & 0xffff0000U) | uv;
}

static int RenderBucket_DrawSplitPrimitiveNormalAtOTEntry(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, u32 *otEntry,
                                                          const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                          const struct RenderBucketSplitVertex *v2)
{
	(void)command;
	if ((char *)ctx->primMem->cursor + sizeof(POLY_GT3) >= (char *)ctx->primMem->guardEnd)
	{
		return -1;
	}

	if (otEntry == 0)
	{
		return 0;
	}

	RenderBucket_LoadSplitPrimColors(ctx, otEntry, v0->color, v1->color, v2->color);

	if (tex == 0)
	{
		POLY_G3 *p = ctx->primMem->cursor;

		CtrGpu_WriteColorCode(&p->r0, 0x30000000 | (u32)MFC2(20));
		CtrGpu_WriteColorCode(&p->r1, (u32)MFC2(21));
		CtrGpu_WriteColorCode(&p->r2, (u32)MFC2(22));
		p->x0 = (s16)v0->sxy;
		p->y0 = (s16)(v0->sxy >> 16);
		p->x1 = (s16)v1->sxy;
		p->y1 = (s16)(v1->sxy >> 16);
		p->x2 = (s16)v2->sxy;
		p->y2 = (s16)(v2->sxy >> 16);
		RenderBucket_LinkPrimRaw(otEntry, p, 0x06000000);
		ctx->primMem->cursor = (char *)p + 0x1c;
	}
	else
	{
		POLY_GT3 *p = ctx->primMem->cursor;
		u32 texWord1 = RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD1_OFFSET);
		u32 codeWord = ((texWord1 & 0x00600000) == 0x00600000) ? 0x34000000 : 0x36000000;

		CtrGpu_WriteColorCode(&p->r0, codeWord | (u32)MFC2(20));
		CtrGpu_WriteColorCode(&p->r1, (u32)MFC2(21));
		CtrGpu_WriteColorCode(&p->r2, (u32)MFC2(22));
		p->x0 = (s16)v0->sxy;
		p->y0 = (s16)(v0->sxy >> 16);
		p->u0 = (u8)v0->uv;
		p->v0 = (u8)(v0->uv >> 8);
		p->clut = tex->clut;
		p->x1 = (s16)v1->sxy;
		p->y1 = (s16)(v1->sxy >> 16);
		p->u1 = (u8)v1->uv;
		p->v1 = (u8)(v1->uv >> 8);
		p->tpage = tex->tpage;
		p->x2 = (s16)v2->sxy;
		p->y2 = (s16)(v2->sxy >> 16);
		// NOTE(aalhendi): Generated retail textures retain both high lanes of
		// the third word; u3/v3 are real texture bytes, not disposable padding.
		CtrGpu_WritePackedUVWord(&p->u2, RenderBucket_TextureWordWithSplitUv(RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD2_OFFSET), v2->uv));
		RenderBucket_LinkPrimRaw(otEntry, p, 0x09000000);
		ctx->primMem->cursor = (char *)p + 0x28;
	}

	return 0;
}

static void RenderBucket_LoadSplitProjectedRegs(const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                const struct RenderBucketSplitVertex *v2)
{
	MTC2(v0->sxy, 12);
	MTC2(v0->sz, 17);
	MTC2(v1->sxy, 13);
	MTC2(v1->sz, 18);
	MTC2(v2->sxy, 14);
	MTC2(v2->sz, 19);
}

static u32 RenderBucket_LightFlatTextureColor(u32 sourceColor, int signedTest)
{
	u32 litColor;
	int add;
	int brightness;
	int distance;
	u32 r;
	u32 g;
	u32 b;

	MTC2((s32)(sourceColor << 24) >> 19, 9);
	MTC2((s32)((sourceColor >> 8) << 24) >> 19, 10);
	MTC2((s32)((sourceColor >> 16) << 24) >> 19, 11);
	gte_lcir();

	brightness = MFC2_S(9) + 0x1000;
	distance = MFC2_S(11) >> 3;
	brightness = (((brightness * 7) + 0x2000) >> 4);
	if (signedTest < 0)
	{
		brightness >>= 1;
	}

	MTC2(brightness, 8);
	gte_dpcs();

	if (distance < 0)
	{
		distance = -distance;
	}

	add = 0;
	distance -= 0x180;
	if (distance >= 0)
	{
		if (distance >= 0x80)
		{
			distance = 0x7f;
		}

		add = sRenderBucketKeyRelicBrightness8008a2c4[distance];
	}

	litColor = (u32)MFC2(22);
	if (signedTest < 0)
	{
		add >>= 3;
	}

	r = RenderBucket_SaturateU8((litColor & 0xff) + add);
	g = RenderBucket_SaturateU8(((litColor >> 8) & 0xff) + add);
	b = RenderBucket_SaturateU8(((litColor >> 16) & 0xff) + add);
	return (b << 16) | (g << 8) | r;
}

static void RenderBucket_WriteSplitFT3(POLY_FT3 *p, const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                       const struct RenderBucketSplitVertex *v2, u32 texWord0, u32 texWord1, u32 texWord2)
{
	p->x0 = (s16)v0->sxy;
	p->y0 = (s16)(v0->sxy >> 16);
	CtrGpu_WritePackedUVWord(&p->u0, texWord0);
	p->x1 = (s16)v1->sxy;
	p->y1 = (s16)(v1->sxy >> 16);
	CtrGpu_WritePackedUVWord(&p->u1, texWord1);
	p->x2 = (s16)v2->sxy;
	p->y2 = (s16)(v2->sxy >> 16);
	CtrGpu_WritePackedUVWord(&p->u2, texWord2);
}

static int RenderBucket_DrawSplitPrimitiveDepthFadeAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                           int depthMac0, const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                           const struct RenderBucketSplitVertex *v2)
{
	u32 *otEntry;
	u32 color0;
	u32 color1;
	u32 color2;
	POLY_GT3 *p;
	struct RenderBucketExecuteScratch *scratch = RenderBucket_Scratch();
	(void)command;
	RenderBucket_StoreWriterInputs(ctx, tex, activeRange, depthMac0);

	if (tex == 0)
	{
		return 0;
	}

	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
	{
		return 0;
	}

	RenderBucket_DepthFadeColors(v0->color, v1->color, v2->color, (int)v0->sz, (int)v1->sz, (int)v2->sz);
	color0 = scratch->primitiveState.split.depthFadeColors[0];
	color1 = scratch->primitiveState.split.depthFadeColors[1];
	color2 = scratch->primitiveState.split.depthFadeColors[2];
	if ((color0 | color1 | color2) == 0)
	{
		return 0;
	}

	if ((char *)ctx->primMem->cursor + sizeof(POLY_GT3) >= (char *)ctx->primMem->guardEnd)
	{
		return -1;
	}

	// NOTE(aalhendi): Source-backs generated-split use of retail 0x8006b968:
	// depth-fade still writes GT3, but generated vertices supply SXY/SZ and UV.
	p = ctx->primMem->cursor;
	CtrGpu_WriteColorCode(&p->r0, 0x36000000 | (color0 & 0x00ffffff));
	CtrGpu_WriteColorCode(&p->r1, color1);
	CtrGpu_WriteColorCode(&p->r2, color2);
	p->x0 = (s16)v0->sxy;
	p->y0 = (s16)(v0->sxy >> 16);
	p->u0 = (u8)v0->uv;
	p->v0 = (u8)(v0->uv >> 8);
	p->clut = tex->clut;
	p->x1 = (s16)v1->sxy;
	p->y1 = (s16)(v1->sxy >> 16);
	p->u1 = (u8)v1->uv;
	p->v1 = (u8)(v1->uv >> 8);
	p->tpage = tex->tpage;
	p->x2 = (s16)v2->sxy;
	p->y2 = (s16)(v2->sxy >> 16);
	CtrGpu_WritePackedUVWord(&p->u2, RenderBucket_TextureWordWithSplitUv(RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD2_OFFSET), v2->uv));
	RenderBucket_LinkPrimRaw(otEntry, p, 0x09000000);
	ctx->primMem->cursor = (char *)p + 0x28;
	return 0;
}

static int RenderBucket_DrawSplitPrimitiveGhostAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                       int depthMac0, const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                       const struct RenderBucketSplitVertex *v2)
{
	u32 *otEntry;
	int alpha = ctx->idpp->alphaScale;
	struct RenderBucketGhostMaskPacket *mask;

	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
	{
		return 0;
	}

	MTC2(v0->color, 20);
	MTC2(v1->color, 21);
	MTC2(v2->color, 22);
	MTC2(alpha, 8);

	if (alpha == 0)
	{
		return RenderBucket_DrawSplitPrimitiveNormalAtOTEntry(ctx, command, tex, otEntry, v0, v1, v2);
	}

	if ((char *)ctx->primMem->cursor + 0x40 >= (char *)ctx->primMem->guardEnd)
	{
		return -1;
	}

	gte_dpct();

	// NOTE(aalhendi): Source-backs generated-split use of retail 0x8006d670:
	// same ghost primitive packet, but generated vertices supply SXY and UV.
	mask = (struct RenderBucketGhostMaskPacket *)ctx->primMem->cursor;
	mask->drawMode = 0xe1000a40;
	mask->pad = 0;
	mask->colorAndCode = RenderBucket_Scratch()->primitiveState.split.fadeColor.word;
	mask->xy0 = v0->sxy;
	mask->xy1 = v1->sxy;
	mask->xy2 = v2->sxy;

	if (tex == 0)
	{
		struct RenderBucketGhostFlatPacket *packet = (struct RenderBucketGhostFlatPacket *)mask;

		packet->drawMode = 0xe1000a20;
		packet->pad = 0;
		packet->body.color0AndCode = 0x32000000 | (u32)MFC2(20);
		packet->body.xy0 = v0->sxy;
		packet->body.color1 = (u32)MFC2(21);
		packet->body.xy1 = v1->sxy;
		packet->body.color2 = (u32)MFC2(22);
		packet->body.xy2 = v2->sxy;

		RenderBucket_LinkPrimRaw(otEntry, packet, 0x0e000000);
		ctx->primMem->cursor = packet + 1;
	}
	else
	{
		struct RenderBucketGhostTexturedPacket *packet = (struct RenderBucketGhostTexturedPacket *)mask;
		u32 texWord0 = RenderBucket_TextureWordWithSplitUv(RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD0_OFFSET), v0->uv);
		u32 texWord1 =
		    RenderBucket_TextureWordWithSplitUv((RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD1_OFFSET) & ~0x00600000U) | 0x00200000U, v1->uv);
		u32 texWord2 = RenderBucket_TextureWordWithSplitUv(RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD2_OFFSET), v2->uv);

		packet->body.color0AndCode = 0x36000000 | (u32)MFC2(20);
		packet->body.xy0 = v0->sxy;
		packet->body.uv0 = texWord0;
		packet->body.color1 = (u32)MFC2(21);
		packet->body.xy1 = v1->sxy;
		packet->body.uv1 = texWord1;
		packet->body.color2 = (u32)MFC2(22);
		packet->body.xy2 = v2->sxy;
		packet->body.uv2 = texWord2;

		RenderBucket_LinkPrimRaw(otEntry, packet, 0x0f000000);
		ctx->primMem->cursor = packet + 1;
	}

	return 0;
}

static int RenderBucket_DrawSplitPrimitiveKeyRelicTokenAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                               int depthMac0, const struct RenderBucketSplitVertex *v0,
                                                               const struct RenderBucketSplitVertex *v1, const struct RenderBucketSplitVertex *v2)
{
	u32 *otEntry;
	int signedTest;
	u32 texWord0;
	u32 texWord1;
	u32 texWord2;
	u32 codeWord;
	u32 tpageMask;
	POLY_FT3 *p;

	if ((char *)ctx->primMem->cursor + sizeof(POLY_FT3) >= (char *)ctx->primMem->guardEnd)
	{
		return -1;
	}

	if (tex == 0)
	{
		return 0;
	}

	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
	{
		return 0;
	}

	RenderBucket_LoadSplitProjectedRegs(v0, v1, v2);
	gte_nclip();
	signedTest = MFC2_S(24) ^ ((s16)ctx->idpp->instFlags ^ (int)(command << 2));
	if (signedTest < 0)
	{
		otEntry++;
	}

	texWord0 = RenderBucket_TextureWordWithSplitUv(RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD0_OFFSET), v0->uv);
	texWord1 = RenderBucket_TextureWordWithSplitUv(RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD1_OFFSET), v1->uv);
	texWord2 = RenderBucket_TextureWordWithSplitUv(RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD2_OFFSET), v2->uv);

	codeWord = 0x24000000;
	tpageMask = 0x00600000;
	if (((texWord1 & 0x00600000U) == 0) && (signedTest >= 0))
	{
		codeWord = 0x26000000;
		tpageMask = 0x00200000;
	}

	// NOTE(aalhendi): Source-backs generated-split use of retail 0x8006ae90:
	// same flat textured writer, with generated SXY/UV and generated color.
	p = ctx->primMem->cursor;
	CtrGpu_WriteColorCode(&p->r0, codeWord | RenderBucket_LightFlatTextureColor(v0->color, signedTest));
	RenderBucket_WriteSplitFT3(p, v0, v1, v2, texWord0, texWord1 | tpageMask, texWord2);

	RenderBucket_LinkPrimRaw(otEntry, p, 0x07000000);
	ctx->primMem->cursor = (char *)p + 0x20;
	return 0;
}

static int RenderBucket_DrawSplitPrimitiveLitTextureAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                            int depthMac0, const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                            const struct RenderBucketSplitVertex *v2)
{
	u32 *otEntry;
	int signedTest;
	u32 texWord0;
	u32 texWord1;
	u32 texWord2;
	u32 codeWord;
	u32 tpageMask;
	POLY_FT3 *p;

	if ((char *)ctx->primMem->cursor + sizeof(POLY_FT3) >= (char *)ctx->primMem->guardEnd)
	{
		return -1;
	}

	if (tex == 0)
	{
		return 0;
	}

	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
	{
		return 0;
	}

	RenderBucket_LoadSplitProjectedRegs(v0, v1, v2);
	gte_nclip();
	signedTest = MFC2_S(24) ^ ((s16)ctx->idpp->instFlags ^ (int)(command << 2));
	if (signedTest < 0)
	{
		otEntry++;
	}

	texWord0 = RenderBucket_TextureWordWithSplitUv(RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD0_OFFSET), v0->uv);
	texWord1 = RenderBucket_TextureWordWithSplitUv(RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD1_OFFSET), v1->uv);
	texWord2 = RenderBucket_TextureWordWithSplitUv(RenderBucket_ReadTextureWord(tex, RENDER_BUCKET_TEX_WORD2_OFFSET), v2->uv);

	if ((texWord1 & 0x00600000U) != 0)
	{
		u32 otSide = RenderBucket_OTAddress(otEntry) << 3;

		if ((s32)(otSide & (u32)signedTest) < 0)
		{
			return 0;
		}

		codeWord = 0x24000000;
		tpageMask = 0x00600000;
	}
	else if (signedTest < 0)
	{
		codeWord = 0x26000000;
		tpageMask = 0x00400000;
	}
	else
	{
		codeWord = 0x26000000;
		tpageMask = 0x00200000;
	}

	// NOTE(aalhendi): Source-backs generated-split use of retail 0x8006c778:
	// same side-dependent flat textured writer with generated SXY/UV/color.
	p = ctx->primMem->cursor;
	CtrGpu_WriteColorCode(&p->r0, codeWord | RenderBucket_LightFlatTextureColor(v0->color, signedTest));
	RenderBucket_WriteSplitFT3(p, v0, v1, v2, texWord0, texWord1 | tpageMask, texWord2);

	RenderBucket_LinkPrimRaw(otEntry, p, 0x07000000);
	ctx->primMem->cursor = (char *)p + 0x20;
	return 0;
}

static int RenderBucket_DrawSplitPrimitiveAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange, int depthMac0,
                                                  const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                  const struct RenderBucketSplitVertex *v2)
{
	u32 prim = (u32)(u32)ctx->inst->funcPtr[1];

	if (prim == RB_RETAIL_INST_PRIM_NORMAL || prim == RB_RETAIL_INST_PRIM_SELECT_RANGE || prim == RB_RETAIL_INST_PRIM_DEPTH_FADE ||
	    prim == RB_RETAIL_INST_PRIM_CLAMP_DEPTH)
	{
		RenderBucket_StoreWriterInputs(ctx, tex, activeRange, depthMac0);
	}

	// NOTE(aalhendi): Retail tail-calls Instance+0x60 from the generated split
	// helpers. Native only claims the labels whose generated-UV ABI is modeled.
	if (RenderBucket_SplitPrimitiveWriterSupported(ctx) == 0)
	{
		return 0;
	}

	if (prim == RB_RETAIL_INST_PRIM_DEPTH_FADE)
	{
		return RenderBucket_DrawSplitPrimitiveDepthFadeAtRange(ctx, command, tex, activeRange, depthMac0, v0, v1, v2);
	}

	if (prim == RB_RETAIL_INST_PRIM_KEY_TOKEN)
	{
		return RenderBucket_DrawSplitPrimitiveKeyRelicTokenAtRange(ctx, command, tex, activeRange, depthMac0, v0, v1, v2);
	}

	if (prim == RB_RETAIL_INST_PRIM_CLAMP_DEPTH)
	{
		return RenderBucket_DrawSplitPrimitiveNormalAtOTEntry(ctx, command, tex, RenderBucket_GetClampedOTEntry(ctx, activeRange, depthMac0), v0, v1, v2);
	}

	if (prim == RB_RETAIL_INST_PRIM_LIT_TEXTURE)
	{
		return RenderBucket_DrawSplitPrimitiveLitTextureAtRange(ctx, command, tex, activeRange, depthMac0, v0, v1, v2);
	}

	if (prim == RB_RETAIL_INST_PRIM_GHOST)
	{
		return RenderBucket_DrawSplitPrimitiveGhostAtRange(ctx, command, tex, activeRange, depthMac0, v0, v1, v2);
	}

	return RenderBucket_DrawSplitPrimitiveNormalAtOTEntry(ctx, command, tex, RenderBucket_GetNormalOTEntry(activeRange, depthMac0), v0, v1, v2);
}

#ifndef CTR_NATIVE
static void RenderBucket_ProjectSplitVertex(struct RenderBucketDrawContext *ctx, struct RenderBucketSplitVertex *v)
{
	(void)ctx;
	// NOTE(aalhendi): Source-backs retail generated-vertex SXY/SZ stores.
	MTC2(v->xy, 0);
	MTC2(v->z, 1);
	gte_rtps();
	v->sxy = MFC2(14);
	v->sz = MFC2(19);
}
#endif

static void RenderBucket_BuildDepthSplitIntersection(struct RenderBucketDrawContext *ctx, struct RenderBucketSplitVertex *dst,
                                                     const struct RenderBucketSplitVertex *from, const struct RenderBucketSplitVertex *to, int hasTexture)
{
	dst->z = (u32)ctx->splitPlane;
	RenderBucket_SplitInterpolateVertex(dst, from, to, hasTexture);
#ifndef CTR_NATIVE
	RenderBucket_ProjectSplitVertex(ctx, dst);
#else
	// NOTE(aalhendi): Depth intersections use the shared depth-only RTPT tail.
#endif
}

static int RenderBucket_SelectDepthSplitHelperRange(struct RenderBucketDrawContext *ctx, u32 command, int helperRange)
{
	if ((u32)(u32)ctx->inst->funcPtr[1] == RB_RETAIL_INST_PRIM_SELECT_RANGE)
	{
		return RenderBucket_SelectPrimitiveActiveRange(ctx, command);
	}

	return helperRange;
}

#ifdef CTR_NATIVE
// NOTE(aalhendi): Depth and water records overlap Execute scratch. Preserve
// byte/halfword stores and unrelated high bytes, not complete structs.
static void RenderBucket_ScratchStoreHalf(int offset, u16 value)
{
	memcpy((u8 *)RenderBucket_Scratch() + offset, &value, sizeof(value));
}

static void RenderBucket_ScratchStoreWord(int offset, u32 value)
{
	RenderBucket_WritePackedWord((u8 *)RenderBucket_Scratch() + offset, value);
}

static void RenderBucket_DepthStoreIntersection(int offset, const struct RenderBucketSplitVertex *v, int hasTexture)
{
	u8 *raw = (u8 *)RenderBucket_Scratch() + offset;
	RenderBucket_WritePackedWord(raw, v->xy);
	raw[8] = (u8)v->color;
	raw[9] = (u8)(v->color >> 8);
	raw[10] = (u8)(v->color >> 16);
	if (hasTexture != 0)
	{
		RenderBucket_ScratchStoreHalf(offset + 12, v->uv);
	}
}

static void RenderBucket_DepthProjectIntersections(struct RenderBucketSplitVertex *first, struct RenderBucketSplitVertex *second)
{
	// Retail retains V0 from the original RTPT and loads F0 into V1, D8
	// into V2. Separate RTPS calls leave a different V/IR/MAC state.
	MTC2(second->xy, 2);
	MTC2(second->z, 3);
	MTC2(first->xy, 4);
	MTC2(first->z, 5);
	gte_rtpt();
	second->sxy = MFC2(13);
	second->sz = MFC2(18);
	first->sxy = MFC2(14);
	first->sz = MFC2(19);
	RenderBucket_ScratchStoreWord(0x100, second->sxy);
	RenderBucket_ScratchStoreWord(0x104, second->sz);
	RenderBucket_ScratchStoreWord(0xe8, first->sxy);
	RenderBucket_ScratchStoreWord(0xec, first->sz);
}

static int RenderBucket_DrawDepthSplitCandidate(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange, int depthMac0,
                                                const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                const struct RenderBucketSplitVertex *v2, int guardDist)
{
	// Both reuse tails update only the first two FIFO/UV slots. The third
	// slot is the first intersection projected by the preceding RTPT.
	RenderBucket_ScratchStoreWord(0x38, (u32)activeRange);
	MTC2(v0->sxy, 12);
	MTC2(v0->sz, 17);
	MTC2(v1->sxy, 13);
	MTC2(v1->sz, 18);
	if (tex != 0 && (u32)tex == (u32)((u8 *)RenderBucket_Scratch() + 0x60))
	{
		RenderBucket_ScratchStoreHalf(0x60, v0->uv);
		RenderBucket_ScratchStoreHalf(0x64, v1->uv);
	}
	if (guardDist >= 0)
	{
		return 0;
	}

	activeRange = RenderBucket_SelectDepthSplitHelperRange(ctx, command, activeRange);
	return RenderBucket_DrawSplitPrimitiveAtRange(ctx, command, tex, activeRange, depthMac0, v0, v1, v2);
}

static int RenderBucket_DrawSplitClipped(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	const struct RenderBucketSplitVertex *a;
	const struct RenderBucketSplitVertex *b;
	const struct RenderBucketSplitVertex *c;
	const struct RenderBucketSplitVertex *fromFirst;
	const struct RenderBucketSplitVertex *toFirst;
	const struct RenderBucketSplitVertex *fromSecond;
	const struct RenderBucketSplitVertex *toSecond;
	const struct RenderBucketSplitVertex *v0[3];
	const struct RenderBucketSplitVertex *v1[3];
	int guards[3];
	struct RenderBucketProjectedRegs savedRegs;
	struct RenderBucketSplitVertex originals[3];
	struct RenderBucketSplitVertex first;
	struct RenderBucketSplitVertex second;
	struct TextureLayout *generatedTex = 0;
	int primaryRange;
	int secondaryRange;
	int signMask = 0;
	int result = 0;
	int i;

	RenderBucket_AssignSplitUvs(ctx, tex);
	RenderBucket_StoreProjectedRegs(&savedRegs);
	// NormalAlt's fixed continuation is the native saved-return ABI input.
	RenderBucket_ScratchStoreWord(0x84, 0x8006a7f4U);
	// Color-only commands change live rolling colors without rewriting the
	// packed split vertices. The clipping family consumes t6/t7/s0.
	originals[0] = ctx->tempSplit[1];
	originals[1] = ctx->tempSplit[2];
	originals[2] = ctx->tempSplit[3];
	originals[0].color = (u32)ctx->tempColor[1];
	originals[1].color = (u32)ctx->tempColor[2];
	originals[2].color = (u32)ctx->tempColor[3];
	a = &originals[0];
	b = &originals[1];
	c = &originals[2];
	if (a->splitDist < 0)
		signMask |= 1;
	if (b->splitDist < 0)
		signMask |= 2;
	if (c->splitDist < 0)
		signMask |= 4;
	if (signMask == 0)
	{
		return 0;
	}
	RenderBucket_ScratchStoreWord(0x98, a->color);
	RenderBucket_ScratchStoreWord(0xb0, b->color);
	RenderBucket_ScratchStoreWord(0xc8, c->color);
	if (signMask == 7)
	{
		RenderBucket_ScratchStoreWord(0x38, (u32)ctx->idpp->otRangeNormal);
		return RenderBucket_DrawSplitPrimitiveAtRange(ctx, command, tex, ctx->idpp->otRangeNormal, depthMac0, a, b, c);
	}

	RenderBucket_ScratchStoreWord(0xa0, a->sxy);
	RenderBucket_ScratchStoreWord(0xa4, a->sz);
	RenderBucket_ScratchStoreWord(0xb8, b->sxy);
	RenderBucket_ScratchStoreWord(0xbc, b->sz);
	RenderBucket_ScratchStoreWord(0xd0, c->sxy);
	RenderBucket_ScratchStoreWord(0xd4, c->sz);
	RenderBucket_ScratchStoreWord(0x54, (u32)tex);
	if (tex != 0)
	{
		RenderBucket_ScratchStoreWord(0x60, RenderBucket_ReadTextureWord(tex, 0));
		RenderBucket_ScratchStoreWord(0x64, RenderBucket_ReadTextureWord(tex, 4));
		RenderBucket_ScratchStoreWord(0x68, RenderBucket_ReadTextureWord(tex, 8));
		RenderBucket_ScratchStoreHalf(0x9c, a->uv);
		RenderBucket_ScratchStoreHalf(0xb4, b->uv);
		RenderBucket_ScratchStoreHalf(0xcc, c->uv);
		generatedTex = (struct TextureLayout *)((u8 *)RenderBucket_Scratch() + 0x60);
	}
	RenderBucket_ScratchStoreWord(0x50, (u32)generatedTex);
	memcpy(&first, (u8 *)RenderBucket_Scratch() + 0xd8, sizeof(first));
	memcpy(&second, (u8 *)RenderBucket_Scratch() + 0xf0, sizeof(second));
	if (signMask == 1 || signMask == 6)
	{
		fromFirst = a;
		toFirst = b;
		fromSecond = a;
		toSecond = c;
		v0[0] = c;
		v1[0] = &second;
		guards[0] = c->splitDist;
		v0[1] = b;
		v1[1] = c;
		guards[1] = b->splitDist;
		v0[2] = &second;
		v1[2] = a;
		guards[2] = a->splitDist;
		primaryRange = signMask == 6 ? ctx->idpp->otRangeNormal : ctx->idpp->otRangeSecondary;
	}
	else if (signMask == 2 || signMask == 5)
	{
		fromFirst = a;
		toFirst = b;
		fromSecond = c;
		toSecond = b;
		v0[0] = c;
		v1[0] = a;
		guards[0] = a->splitDist;
		v0[1] = &second;
		v1[1] = c;
		guards[1] = c->splitDist;
		v0[2] = b;
		v1[2] = &second;
		guards[2] = b->splitDist;
		primaryRange = signMask == 5 ? ctx->idpp->otRangeNormal : ctx->idpp->otRangeSecondary;
	}
	else
	{
		fromFirst = a;
		toFirst = c;
		fromSecond = b;
		toSecond = c;
		v0[0] = b;
		v1[0] = &second;
		guards[0] = b->splitDist;
		v0[1] = a;
		v1[1] = b;
		guards[1] = a->splitDist;
		v0[2] = &second;
		v1[2] = c;
		guards[2] = c->splitDist;
		primaryRange = signMask == 3 ? ctx->idpp->otRangeNormal : ctx->idpp->otRangeSecondary;
	}
	secondaryRange = primaryRange == ctx->idpp->otRangeNormal ? ctx->idpp->otRangeSecondary : ctx->idpp->otRangeNormal;
	RenderBucket_BuildDepthSplitIntersection(ctx, &first, fromFirst, toFirst, tex != 0);
	RenderBucket_BuildDepthSplitIntersection(ctx, &second, fromSecond, toSecond, tex != 0);
	RenderBucket_DepthStoreIntersection(0xd8, &first, tex != 0);
	RenderBucket_DepthStoreIntersection(0xf0, &second, tex != 0);
	RenderBucket_DepthProjectIntersections(&first, &second);
	if (generatedTex != 0)
	{
		RenderBucket_ScratchStoreHalf(0x68, first.uv);
	}
	for (i = 0; i < 3; i++)
	{
		result = RenderBucket_DrawDepthSplitCandidate(ctx, command, generatedTex, i == 2 ? secondaryRange : primaryRange, depthMac0, v0[i], v1[i], &first,
		                                              guards[i]);
		if (result < 0)
			break;
	}
	RenderBucket_ScratchStoreWord(0x50, (u32)tex);
	return RenderBucket_RestoreProjectedRegsAndReturn(&savedRegs, result);
}

#endif

static void RenderBucket_BuildWaterSplitIntersection(struct RenderBucketDrawContext *ctx, struct RenderBucketSplitVertex *dst,
                                                     const struct RenderBucketSplitVertex *from, const struct RenderBucketSplitVertex *to, int hasTexture)
{
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Water projects both generated records in one RTPT tail.
	RenderBucket_WaterSplitInterpolateVertex(ctx, dst, from, to, hasTexture, ctx->idpp->splitLine);
#else
	RenderBucket_WaterSplitInterpolateVertex(ctx, dst, from, to, hasTexture, ctx->idpp->splitLine);
	RenderBucket_ProjectSplitVertex(ctx, dst);
#endif
}

static int RenderBucket_SelectWaterSplitHelperRange(struct RenderBucketDrawContext *ctx, u32 command, int helperRange)
{
	if ((u32)(u32)ctx->inst->funcPtr[1] == RB_RETAIL_INST_PRIM_SELECT_RANGE)
	{
		return RenderBucket_SelectPrimitiveActiveRange(ctx, command);
	}

	return helperRange;
}

static u32 RenderBucket_WaterSplitShiftMaskColor(struct RenderBucketDrawContext *ctx, u32 color)
{
	return (color >> (ctx->inst->specLightX & 31)) & ctx->inst->reflectionRGBA;
}

static u32 RenderBucket_WaterSplitDimColor(u32 color)
{
	u32 half = (color >> 1) & 0x007f7f7f;
	u32 quarter = (half >> 1) & 0x007f7f7f;

	return half + quarter;
}

static u32 RenderBucket_WaterSplitOffsetSxyX(u32 sxy)
{
	return (sxy & 0xffff0000) | ((u16)((sxy & 0xffff) + 3));
}

static int RenderBucket_ApplyWaterSplitSideSelector(struct RenderBucketDrawContext *ctx, int guardDist, struct RenderBucketSplitVertex *v0,
                                                    struct RenderBucketSplitVertex *v1, struct RenderBucketSplitVertex *v2)
{
	int selector;

	// NOTE(aalhendi): Maps retail side-selector labels 0x8006d55c-0x8006d5b8.
	switch ((u32)(u32)ctx->inst->funcPtr[2])
	{
	case RB_RETAIL_INST_FUNC2_SPLIT_BOTH_MASK:
		if (guardDist >= 0)
		{
			v0->color = RenderBucket_WaterSplitShiftMaskColor(ctx, v0->color);
			v1->color = RenderBucket_WaterSplitShiftMaskColor(ctx, v1->color);
			v2->color = RenderBucket_WaterSplitShiftMaskColor(ctx, v2->color);
		}
		return 1;

	case RB_RETAIL_INST_FUNC2_SPLIT_NEGATIVE:
		if (guardDist >= 0)
		{
			return 0;
		}

		// NOTE(aalhendi): Retail 0x8006d588 also checks scratch[0x108],
		// which reflection toggles per pass.
		if (ctx->waterSplitSide >= 0)
		{
			v0->color = RenderBucket_WaterSplitShiftMaskColor(ctx, v0->color);
			v1->color = RenderBucket_WaterSplitShiftMaskColor(ctx, v1->color);
			v2->color = RenderBucket_WaterSplitShiftMaskColor(ctx, v2->color);
		}
		return 1;

	case RB_RETAIL_INST_FUNC2_SPLIT_XOR:
		return (guardDist ^ ctx->inst->specLightX) >= 0;

	case RB_RETAIL_INST_FUNC2_SPLIT_DIM_XOR:
		selector = guardDist ^ ctx->inst->specLightX;
		if (selector >= 0)
		{
			v0->color = RenderBucket_WaterSplitDimColor(v0->color);
			v1->color = RenderBucket_WaterSplitDimColor(v1->color);
			v2->color = RenderBucket_WaterSplitDimColor(v2->color);
			v0->sxy = RenderBucket_WaterSplitOffsetSxyX(v0->sxy);
			v1->sxy = RenderBucket_WaterSplitOffsetSxyX(v1->sxy);
			v2->sxy = RenderBucket_WaterSplitOffsetSxyX(v2->sxy);
		}
		return 1;
	}

	return guardDist < 0;
}

#ifdef CTR_NATIVE
static void RenderBucket_WaterStoreIntersection(int offset, struct RenderBucketSplitVertex *v, int hasTexture)
{
	u8 *raw = (u8 *)RenderBucket_Scratch() + offset;
	RenderBucket_ScratchStoreHalf(offset, (u16)v->xy);
	RenderBucket_ScratchStoreWord(offset + 4, v->z);
	raw[8] = (u8)v->color;
	raw[9] = (u8)(v->color >> 8);
	raw[10] = (u8)(v->color >> 16);
	if (hasTexture != 0)
		RenderBucket_ScratchStoreHalf(offset + 12, v->uv);
	v->xy = RenderBucket_ReadPackedWord(raw);
	v->color = RenderBucket_ReadPackedWord(raw + 8);
}

static void RenderBucket_WaterProjectIntersections(struct RenderBucketSplitVertex *first, struct RenderBucketSplitVertex *second)
{
	// NOTE(aalhendi): Retail retains original V0, loads F0 into V1 and
	// D8 into V2, then executes RTPT. Separate RTPS changes V/IR/MAC/SZ0.
	MTC2(second->xy, 2);
	MTC2(second->z, 3);
	MTC2(first->xy, 4);
	MTC2(first->z, 5);
	gte_rtpt();
	second->sxy = MFC2(13);
	second->sz = MFC2(18);
	first->sxy = MFC2(14);
	first->sz = MFC2(19);
	RenderBucket_ScratchStoreWord(0xe8, first->sxy);
	RenderBucket_ScratchStoreWord(0xec, first->sz);
	RenderBucket_ScratchStoreWord(0x100, second->sxy);
	RenderBucket_ScratchStoreWord(0x104, second->sz);
}

static int RenderBucket_DrawWaterSplitCandidate(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange, int depthMac0,
                                                const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                const struct RenderBucketSplitVertex *v2, int guardDist)
{
	// NOTE(aalhendi): Candidate tails reload only the first two FIFO slots.
	// A DIM selector can shift SXY2; the next candidate inherits that live lane.
	MTC2(v0->sxy, 12);
	MTC2(v0->sz, 17);
	MTC2(v1->sxy, 13);
	MTC2(v1->sz, 18);
	RenderBucket_ScratchStoreWord(0x38, (u32)activeRange);
	if (tex != 0 && (u32)tex == (u32)((u8 *)RenderBucket_Scratch() + 0x60))
	{
		RenderBucket_ScratchStoreHalf(0x60, v0->uv);
		RenderBucket_ScratchStoreHalf(0x64, v1->uv);
	}
	struct RenderBucketSplitVertex out0 = *v0;
	struct RenderBucketSplitVertex out1 = *v1;
	struct RenderBucketSplitVertex out2 = *v2;
	out2.sxy = MFC2(14);
	out2.sz = MFC2(19);

	if (RenderBucket_ApplyWaterSplitSideSelector(ctx, guardDist, &out0, &out1, &out2) == 0)
	{
		return 0;
	}

	if ((u32)ctx->inst->funcPtr[2] == RB_RETAIL_INST_FUNC2_SPLIT_DIM_XOR && (guardDist ^ ctx->inst->specLightX) >= 0)
	{
		MTC2(out0.sxy, 12);
		MTC2(out1.sxy, 13);
		MTC2(out2.sxy, 14);
	}

	activeRange = RenderBucket_SelectWaterSplitHelperRange(ctx, command, activeRange);
	return RenderBucket_DrawSplitPrimitiveAtRange(ctx, command, tex, activeRange, depthMac0, &out0, &out1, &out2);
}

static int RenderBucket_DrawWaterSplitClipped(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	const struct RenderBucketSplitVertex *a;
	const struct RenderBucketSplitVertex *b;
	const struct RenderBucketSplitVertex *c;
	const struct RenderBucketSplitVertex *fromFirst;
	const struct RenderBucketSplitVertex *toFirst;
	const struct RenderBucketSplitVertex *fromSecond;
	const struct RenderBucketSplitVertex *toSecond;
	const struct RenderBucketSplitVertex *v0[3];
	const struct RenderBucketSplitVertex *v1[3];
	int guards[3];
	struct RenderBucketProjectedRegs savedRegs;
	struct RenderBucketSplitVertex originals[3];
	struct RenderBucketSplitVertex first;
	struct RenderBucketSplitVertex second;
	struct TextureLayout *generatedTex = 0;
	int primaryRange;
	int secondaryRange;
	int signMask = 0;
	int result = 0;
	int i;

	RenderBucket_AssignSplitUvs(ctx, tex);
	RenderBucket_StoreProjectedRegs(&savedRegs);
	// NOTE(aalhendi): Split's application return is the native saved-return ABI input.
	RenderBucket_ScratchStoreWord(0x84, 0x8006b15cU);
	// Color-only commands change live rolling colors without rewriting the
	// packed split vertices. The clipping family consumes t6/t7/s0.
	originals[0] = ctx->tempSplit[1];
	originals[1] = ctx->tempSplit[2];
	originals[2] = ctx->tempSplit[3];
	originals[0].color = (u32)ctx->tempColor[1];
	originals[1].color = (u32)ctx->tempColor[2];
	originals[2].color = (u32)ctx->tempColor[3];
	a = &originals[0];
	b = &originals[1];
	c = &originals[2];
	if (a->splitDist < 0)
		signMask |= 1;
	if (b->splitDist < 0)
		signMask |= 2;
	if (c->splitDist < 0)
		signMask |= 4;

	RenderBucket_ScratchStoreWord(0x98, a->color);
	RenderBucket_ScratchStoreWord(0xb0, b->color);
	RenderBucket_ScratchStoreWord(0xc8, c->color);
	if (signMask == 0 || signMask == 7)
	{
		return RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, signMask == 7 ? ctx->idpp->otRangeNormal : ctx->idpp->otRangeSecondary, depthMac0, a, b,
		                                            c, a->splitDist);
	}

	RenderBucket_ScratchStoreWord(0xa0, a->sxy);
	RenderBucket_ScratchStoreWord(0xa4, a->sz);
	RenderBucket_ScratchStoreWord(0xb8, b->sxy);
	RenderBucket_ScratchStoreWord(0xbc, b->sz);
	RenderBucket_ScratchStoreWord(0xd0, c->sxy);
	RenderBucket_ScratchStoreWord(0xd4, c->sz);
	RenderBucket_ScratchStoreWord(0x54, (u32)tex);
	if (tex != 0)
	{
		RenderBucket_ScratchStoreWord(0x60, RenderBucket_ReadTextureWord(tex, 0));
		RenderBucket_ScratchStoreWord(0x64, RenderBucket_ReadTextureWord(tex, 4));
		RenderBucket_ScratchStoreWord(0x68, RenderBucket_ReadTextureWord(tex, 8));
		RenderBucket_ScratchStoreHalf(0x9c, a->uv);
		RenderBucket_ScratchStoreHalf(0xb4, b->uv);
		RenderBucket_ScratchStoreHalf(0xcc, c->uv);
		generatedTex = (struct TextureLayout *)((u8 *)RenderBucket_Scratch() + 0x60);
	}
	RenderBucket_ScratchStoreWord(0x50, (u32)generatedTex);
	memcpy(&first, (u8 *)RenderBucket_Scratch() + 0xd8, sizeof(first));
	memcpy(&second, (u8 *)RenderBucket_Scratch() + 0xf0, sizeof(second));
	if (signMask == 1 || signMask == 6)
	{
		fromFirst = a;
		toFirst = b;
		fromSecond = a;
		toSecond = c;
		v0[0] = c;
		v1[0] = &second;
		guards[0] = c->splitDist;
		v0[1] = b;
		v1[1] = c;
		guards[1] = b->splitDist;
		v0[2] = &second;
		v1[2] = a;
		guards[2] = a->splitDist;
		primaryRange = signMask == 6 ? ctx->idpp->otRangeNormal : ctx->idpp->otRangeSecondary;
	}
	else if (signMask == 2 || signMask == 5)
	{
		fromFirst = a;
		toFirst = b;
		fromSecond = c;
		toSecond = b;
		v0[0] = c;
		v1[0] = a;
		guards[0] = a->splitDist;
		v0[1] = &second;
		v1[1] = c;
		guards[1] = c->splitDist;
		v0[2] = b;
		v1[2] = &second;
		guards[2] = b->splitDist;
		primaryRange = signMask == 5 ? ctx->idpp->otRangeNormal : ctx->idpp->otRangeSecondary;
	}
	else
	{
		fromFirst = a;
		toFirst = c;
		fromSecond = b;
		toSecond = c;
		v0[0] = b;
		v1[0] = &second;
		guards[0] = b->splitDist;
		v0[1] = a;
		v1[1] = b;
		guards[1] = a->splitDist;
		v0[2] = &second;
		v1[2] = c;
		guards[2] = c->splitDist;
		primaryRange = signMask == 3 ? ctx->idpp->otRangeNormal : ctx->idpp->otRangeSecondary;
	}
	secondaryRange = primaryRange == ctx->idpp->otRangeNormal ? ctx->idpp->otRangeSecondary : ctx->idpp->otRangeNormal;
	RenderBucket_BuildWaterSplitIntersection(ctx, &first, fromFirst, toFirst, tex != 0);
	RenderBucket_BuildWaterSplitIntersection(ctx, &second, fromSecond, toSecond, tex != 0);
	RenderBucket_WaterStoreIntersection(0xd8, &first, tex != 0);
	RenderBucket_WaterStoreIntersection(0xf0, &second, tex != 0);
	RenderBucket_ScratchStoreWord(0x88, signMask == 1 || signMask == 6 ? 0x8006d12cU : signMask == 2 || signMask == 5 ? 0x8006d17cU : 0x8006d1ccU);
	RenderBucket_WaterProjectIntersections(&first, &second);
	if (generatedTex != 0)
	{
		RenderBucket_ScratchStoreHalf(0x68, first.uv);
	}
	for (i = 0; i < 3; i++)
	{
		result = RenderBucket_DrawWaterSplitCandidate(ctx, command, generatedTex, i == 2 ? secondaryRange : primaryRange, depthMac0, v0[i], v1[i], &first,
		                                              guards[i]);
		if (result < 0)
			break;
	}
	RenderBucket_ScratchStoreWord(0x50, (u32)tex);
	return RenderBucket_RestoreProjectedRegsAndReturn(&savedRegs, result);
}

#else
static int RenderBucket_DrawWaterSplitCandidate(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange, int depthMac0,
                                                const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                const struct RenderBucketSplitVertex *v2, int guardDist)
{
	struct RenderBucketSplitVertex out0 = *v0;
	struct RenderBucketSplitVertex out1 = *v1;
	struct RenderBucketSplitVertex out2 = *v2;

	if (RenderBucket_ApplyWaterSplitSideSelector(ctx, guardDist, &out0, &out1, &out2) == 0)
	{
		return 0;
	}

	activeRange = RenderBucket_SelectWaterSplitHelperRange(ctx, command, activeRange);
	return RenderBucket_DrawSplitPrimitiveAtRange(ctx, command, tex, activeRange, depthMac0, &out0, &out1, &out2);
}

static int RenderBucket_DrawWaterSplitClipped(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	const struct RenderBucketSplitVertex *a;
	const struct RenderBucketSplitVertex *b;
	const struct RenderBucketSplitVertex *c;
	struct RenderBucketProjectedRegs savedRegs;
	struct RenderBucketSplitVertex ab;
	struct RenderBucketSplitVertex ac;
	struct RenderBucketSplitVertex bc;
	struct RenderBucketSplitVertex cb;
	int hasTexture = tex != 0;
	int primaryRange;
	int secondaryRange;
	int signMask = 0;

	RenderBucket_AssignSplitUvs(ctx, tex);
	RenderBucket_StoreProjectedRegs(&savedRegs);

	a = &ctx->tempSplit[1];
	b = &ctx->tempSplit[2];
	c = &ctx->tempSplit[3];

	if (a->splitDist < 0)
	{
		signMask |= 1;
	}
	if (b->splitDist < 0)
	{
		signMask |= 2;
	}
	if (c->splitDist < 0)
	{
		signMask |= 4;
	}

	// NOTE(aalhendi): Retain the default C fallback's candidate topology;
	// native uses the packed scratch/FIFO implementation above.
	switch (signMask)
	{
	case 0:
		return RenderBucket_RestoreProjectedRegsAndReturn(
		    &savedRegs, RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, ctx->idpp->otRangeSecondary, depthMac0, a, b, c, a->splitDist));

	case 7:
		return RenderBucket_RestoreProjectedRegsAndReturn(
		    &savedRegs, RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, ctx->idpp->otRangeNormal, depthMac0, a, b, c, a->splitDist));

	case 1:
		primaryRange = ctx->idpp->otRangeSecondary;
		secondaryRange = ctx->idpp->otRangeNormal;
		RenderBucket_BuildWaterSplitIntersection(ctx, &ab, a, b, hasTexture);
		RenderBucket_BuildWaterSplitIntersection(ctx, &ac, a, c, hasTexture);
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, c, &ac, &ab, c->splitDist) < 0)
		{
			return RenderBucket_RestoreProjectedRegsAndReturn(&savedRegs, -1);
		}
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, b, c, &ab, b->splitDist) < 0)
		{
			return RenderBucket_RestoreProjectedRegsAndReturn(&savedRegs, -1);
		}
		return RenderBucket_RestoreProjectedRegsAndReturn(
		    &savedRegs, RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, &ac, a, &ab, a->splitDist));

	case 6:
		primaryRange = ctx->idpp->otRangeNormal;
		secondaryRange = ctx->idpp->otRangeSecondary;
		RenderBucket_BuildWaterSplitIntersection(ctx, &ab, a, b, hasTexture);
		RenderBucket_BuildWaterSplitIntersection(ctx, &ac, a, c, hasTexture);
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, c, &ac, &ab, c->splitDist) < 0)
		{
			return RenderBucket_RestoreProjectedRegsAndReturn(&savedRegs, -1);
		}
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, b, c, &ab, b->splitDist) < 0)
		{
			return RenderBucket_RestoreProjectedRegsAndReturn(&savedRegs, -1);
		}
		return RenderBucket_RestoreProjectedRegsAndReturn(
		    &savedRegs, RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, &ac, a, &ab, a->splitDist));

	case 2:
		primaryRange = ctx->idpp->otRangeSecondary;
		secondaryRange = ctx->idpp->otRangeNormal;
		RenderBucket_BuildWaterSplitIntersection(ctx, &ab, a, b, hasTexture);
		RenderBucket_BuildWaterSplitIntersection(ctx, &cb, c, b, hasTexture);
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, c, a, &ab, a->splitDist) < 0)
		{
			return RenderBucket_RestoreProjectedRegsAndReturn(&savedRegs, -1);
		}
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, &cb, c, &ab, c->splitDist) < 0)
		{
			return RenderBucket_RestoreProjectedRegsAndReturn(&savedRegs, -1);
		}
		return RenderBucket_RestoreProjectedRegsAndReturn(
		    &savedRegs, RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, b, &cb, &ab, b->splitDist));

	case 5:
		primaryRange = ctx->idpp->otRangeNormal;
		secondaryRange = ctx->idpp->otRangeSecondary;
		RenderBucket_BuildWaterSplitIntersection(ctx, &ab, a, b, hasTexture);
		RenderBucket_BuildWaterSplitIntersection(ctx, &cb, c, b, hasTexture);
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, c, a, &ab, a->splitDist) < 0)
		{
			return RenderBucket_RestoreProjectedRegsAndReturn(&savedRegs, -1);
		}
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, &cb, c, &ab, c->splitDist) < 0)
		{
			return RenderBucket_RestoreProjectedRegsAndReturn(&savedRegs, -1);
		}
		return RenderBucket_RestoreProjectedRegsAndReturn(
		    &savedRegs, RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, b, &cb, &ab, b->splitDist));

	case 3:
		primaryRange = ctx->idpp->otRangeNormal;
		secondaryRange = ctx->idpp->otRangeSecondary;
		RenderBucket_BuildWaterSplitIntersection(ctx, &ac, a, c, hasTexture);
		RenderBucket_BuildWaterSplitIntersection(ctx, &bc, b, c, hasTexture);
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, b, &bc, &ac, b->splitDist) < 0)
		{
			return RenderBucket_RestoreProjectedRegsAndReturn(&savedRegs, -1);
		}
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, a, b, &ac, a->splitDist) < 0)
		{
			return RenderBucket_RestoreProjectedRegsAndReturn(&savedRegs, -1);
		}
		return RenderBucket_RestoreProjectedRegsAndReturn(
		    &savedRegs, RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, &bc, c, &ac, c->splitDist));

	case 4:
		primaryRange = ctx->idpp->otRangeSecondary;
		secondaryRange = ctx->idpp->otRangeNormal;
		RenderBucket_BuildWaterSplitIntersection(ctx, &ac, a, c, hasTexture);
		RenderBucket_BuildWaterSplitIntersection(ctx, &bc, b, c, hasTexture);
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, b, &bc, &ac, b->splitDist) < 0)
		{
			return RenderBucket_RestoreProjectedRegsAndReturn(&savedRegs, -1);
		}
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, a, b, &ac, a->splitDist) < 0)
		{
			return RenderBucket_RestoreProjectedRegsAndReturn(&savedRegs, -1);
		}
		return RenderBucket_RestoreProjectedRegsAndReturn(
		    &savedRegs, RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, &bc, c, &ac, c->splitDist));
	}

	return RenderBucket_RestoreProjectedRegsAndReturn(&savedRegs, 0);
}

#endif

#include "RenderBucket_Normal.c"

static u32 RenderBucket_MirrorSpecialPackedXY(struct RenderBucketDrawContext *ctx, u32 packedXY)
{
	u32 mirrorBase = (u32)(s32)(s16)ctx->idpp->splitLine;

	mirrorBase <<= 17;
	return (mirrorBase - (packedXY & 0xffff0000)) | (packedXY & 0xffff);
}

static void RenderBucket_LoadSpecialMirroredRTPT(struct RenderBucketDrawContext *ctx)
{
	MTC2(RenderBucket_MirrorSpecialPackedXY(ctx, ctx->tempPacked[1].xy), 0);
	MTC2(ctx->tempPacked[1].z, 1);
	MTC2(RenderBucket_MirrorSpecialPackedXY(ctx, ctx->tempPacked[2].xy), 2);
	MTC2(ctx->tempPacked[2].z, 3);
	MTC2(RenderBucket_MirrorSpecialPackedXY(ctx, ctx->tempPacked[3].xy), 4);
	MTC2(ctx->tempPacked[3].z, 5);
	gte_rtpt();
}

static void RenderBucket_LoadSpecialMirroredRTPS(struct RenderBucketDrawContext *ctx, int reuseFirstVertex)
{
	if (reuseFirstVertex != 0)
	{
		// NOTE(aalhendi): Source-backs 0x8006bea8-0x8006beb4: when retail
		// reuses the first original FIFO vertex, it also copies the previous
		// mirrored SXY0/SZ1 scratch pair into the mirrored SXY1/SZ2 slot.
		// Reflection repeats this at 0x8006ccf4-0x8006cd00.
		ctx->specialMirrorRegs.sxy1 = ctx->specialMirrorRegs.sxy0;
		ctx->specialMirrorRegs.sz2 = ctx->specialMirrorRegs.sz1;
	}

	MTC2(RenderBucket_MirrorSpecialPackedXY(ctx, ctx->tempPacked[3].xy), 0);
	MTC2(ctx->tempPacked[3].z, 1);
	MTC2(ctx->specialMirrorRegs.sxy1, 13);
	MTC2(ctx->specialMirrorRegs.sz2, 18);
	MTC2(ctx->specialMirrorRegs.sxy2, 14);
	MTC2(ctx->specialMirrorRegs.sz3, 19);
	gte_rtps();
}

static int RenderBucket_DrawSpecialMirroredPass(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	int savedColor1 = ctx->tempColor[1];
	int savedColor2 = ctx->tempColor[2];
	int savedColor3 = ctx->tempColor[3];
	u32 mask = ctx->inst->reflectionRGBA;
	int shift = ctx->inst->specLightX & 31;
	int ret;

	// NOTE(aalhendi): Source-backs the 0x8006bbc0 mirrored-side color path:
	// retail shifts each rolling RGB word by Instance+0x53 and masks by
	// Instance+0x58 before calling the primitive writer on the secondary range.
	ctx->tempColor[1] = (int)(((u32)savedColor1 >> shift) & mask);
	ctx->tempColor[2] = (int)(((u32)savedColor2 >> shift) & mask);
	ctx->tempColor[3] = (int)(((u32)savedColor3 >> shift) & mask);
	ret = RenderBucket_DispatchDrawInstPrimAtRange(ctx, command, tex, ctx->idpp->otRangeSecondary, depthMac0);
	ctx->tempColor[1] = savedColor1;
	ctx->tempColor[2] = savedColor2;
	ctx->tempColor[3] = savedColor3;
	return ret;
}

static void RenderBucket_SwapActiveRanges(struct RenderBucketDrawContext *ctx)
{
	int range = ctx->idpp->otRangeNormal;

	ctx->idpp->otRangeNormal = ctx->idpp->otRangeSecondary;
	ctx->idpp->otRangeSecondary = range;
}

static int RenderBucket_DrawReflectionPrimitive(struct RenderBucketDrawContext *ctx, u32 command, int useRtps, int reuseFirstVertex, struct TextureLayout *tex)
{
	struct RenderBucketProjectedRegs originalRegs;
	u32 mirrorFlag;
	u32 originalFlag;
	int depthMac0;
	int savedWaterSplitSide = ctx->waterSplitSide;

	if (useRtps != 0)
	{
		RenderBucket_LoadPrimRTPS(ctx, reuseFirstVertex);
	}
	else
	{
		RenderBucket_LoadPrimRTPT(ctx);
	}

	RenderBucket_StoreProjectedRegs(&originalRegs);
	if (useRtps != 0)
	{
		RenderBucket_LoadSpecialMirroredRTPS(ctx, reuseFirstVertex);
	}
	else
	{
		RenderBucket_LoadSpecialMirroredRTPT(ctx);
	}

	mirrorFlag = CFC2(31);
	RenderBucket_StoreProjectedRegs(&ctx->specialMirrorRegs);
	RenderBucket_StoreSplitProjectedRegs(ctx);
	RenderBucket_SwapActiveRanges(ctx);
	ctx->waterSplitSide = 0;
	if (RenderBucket_CheckProjectedPrim(ctx, command, mirrorFlag, 0x8000, &depthMac0) != 0)
	{
		if (RenderBucket_DrawWaterSplitClipped(ctx, command, tex, depthMac0) < 0)
		{
			RenderBucket_SwapActiveRanges(ctx);
			ctx->waterSplitSide = savedWaterSplitSide;
			return -1;
		}
	}
	RenderBucket_SwapActiveRanges(ctx);

	RenderBucket_LoadProjectedRegs(&originalRegs);
	RenderBucket_StoreSplitProjectedRegs(ctx);
	ctx->waterSplitSide = -1;
	originalFlag = CFC2(31);
	if (RenderBucket_CheckProjectedPrim(ctx, command, originalFlag, 0, &depthMac0) != 0)
	{
		if (RenderBucket_DrawWaterSplitClipped(ctx, command, tex, depthMac0) < 0)
		{
			ctx->waterSplitSide = savedWaterSplitSide;
			return -1;
		}
	}

	ctx->waterSplitSide = savedWaterSplitSide;
	return 0;
}

#include "RenderBucket_Special.c"

#ifdef CTR_NATIVE
#include "RenderBucket_Reflection.c"

#include "RenderBucket_Split.c"

static void RenderBucket_SetFarColorFromInstance(struct Instance *inst)
{
	u32 color = inst->colorRGBA;

	MTC2(0, 6);
	CTC2((color >> 16) & 0xff0, 21);
	CTC2((color >> 8) & 0xff0, 22);
	CTC2(color & 0xff0, 23);
}

static int RenderBucket_RunInstanceSetupCallback(struct RenderBucketDrawContext *ctx)
{
	switch ((u32)(u32)ctx->inst->funcPtr[0])
	{
	case RB_RETAIL_INST_SETUP_LIGHT_COLOR:
		CTC2(ctx->inst->specLightX, 16);
		CTC2((u16)ctx->inst->reflectionRGBA, 17);
		CTC2(RenderBucket_ReadPackedWord(&ctx->idpp->halfVector.x), 19);
		CTC2((u16)ctx->idpp->halfVector.z, 20);
		RenderBucket_SetFarColorFromInstance(ctx->inst);
		return 1;

	case RB_RETAIL_INST_SETUP_COLOR:
		RenderBucket_SetFarColorFromInstance(ctx->inst);
		return 1;

	case RB_RETAIL_INST_SETUP_ZERO_COLOR:
		CTC2(0, 21);
		CTC2(0, 22);
		CTC2(0, 23);
		return 1;

	case RB_RETAIL_INST_SETUP_FADE_COLOR:
		// NOTE(aalhendi): Zero-color instances use alphaScale for fading;
		// at 0x1000, skip primitive dispatch.
		if (ctx->inst->colorRGBA != 0)
		{
			RenderBucket_SetFarColorFromInstance(ctx->inst);
			return 1;
		}

		{
			union RenderBucketScratchColor *scratchFade = &RenderBucket_Scratch()->primitiveState.split.fadeColor;
			int fade = (int)((u32)(0x1000 - (s32)ctx->inst->alphaScale) >> 5);

			scratchFade->colorCode.r = (u8)fade;
			scratchFade->colorCode.g = (u8)fade;
			scratchFade->colorCode.b = (u8)fade;
			scratchFade->colorCode.code = 0x22;
			if (ctx->inst->alphaScale == 0x1000)
			{
				return 0;
			}
		}

		RenderBucket_SetFarColorFromInstance(ctx->inst);
		return 1;

	default:
		// TODO(aalhendi): Port the remaining Instance+0x5c setup callbacks when
		// their selector rows become live. Do not substitute the common-color path;
		// that hides missing retail setup callbacks.
		return 0;
	}
}

static void RenderBucket_DispatchDrawFunc(struct RenderBucketDrawContext *ctx)
{
	struct RenderBucketExecuteScratch *scratch = RenderBucket_Scratch();

	// NOTE(aalhendi): Retail RenderBucket_Execute copies the per-model color
	// cache to scratchpad 0x140 before Instance+0x5c setup callback dispatch.
	RenderBucket_CopyScratchColorCache(ctx);

	if (RenderBucket_RunInstanceSetupCallback(ctx) == 0)
	{
		return;
	}

	// NOTE(aalhendi): Execute's SLL/SH stores retain only low16 bits. Unsigned
	// multiplication preserves that contract without shifting a negative C value.
	scratch->minDepthByteOffset = (s16)(u16)((u32)(s32)ctx->idpp->depthOffset[0] * 4U);
	scratch->maxDepthByteOffset = (s16)(u16)((u32)(s32)ctx->idpp->depthOffset[1] * 4U);
	scratch->activeOTPtr32 = (u32)ctx->idpp->otRangeNormal;
	scratch->primaryOTPtr32 = (u32)ctx->idpp->otRangeNormal;
	scratch->secondaryOTPtr32 = (u32)ctx->idpp->otRangeSecondary;

	switch ((u32)ctx->idpp->unkEC)
	{
	case RB_RETAIL_DRAWFUNC_NORMAL:
		RenderBucket_DrawFunc_Normal(ctx);
		return;

	case RB_RETAIL_DRAWFUNC_NORMAL_ALT:
		RenderBucket_DrawFunc_NormalAlt(ctx);
		return;

	case RB_RETAIL_DRAWFUNC_SPLIT:
		RenderBucket_DrawFunc_Split(ctx);
		return;

	case RB_RETAIL_DRAWFUNC_SPECIAL:
		RenderBucket_DrawFunc_Special(ctx);
		return;

	case RB_RETAIL_DRAWFUNC_REFLECTION:
		RenderBucket_DrawFunc_Reflection(ctx);
		return;

	default:
		// TODO(aalhendi): Port any newly observed draw handlers.
		// Do not fall through to normal draw; that masks live retail handler rows.
		return;
	}
}
#else
#include "RenderBucket_Split.c"
#endif

static int RenderBucket_PrepareDrawContext(struct RenderBucketDrawContext *ctx, struct Instance *inst, struct Instance *instPlayerBase, struct PrimMem *primMem)
{
	struct RenderBucketExecuteScratch *scratch = RenderBucket_Scratch();
	struct InstDrawPerPlayer *idpp;
	struct PushBuffer *pb;
	struct ModelHeader *mh;
	struct ModelFrame *mf;
	struct ModelFrame *nextFrame;
	struct ModelAnim *anim;

	if (inst == 0)
	{
		return 0;
	}

	if (inst->model == 0)
	{
		return 0;
	}

	idpp = RenderBucket_InstancePlayerIdpp(instPlayerBase);
	pb = idpp->pushBuffer;
	if (pb == 0)
	{
		return 0;
	}

	scratch->instPtr32 = (u32)(u32)inst;
	if (scratch->pushBufferPtr32 != (u32)(u32)pb)
	{
		scratch->pushBufferPtr32 = (u32)(u32)pb;
		scratch->geomW = pb->rect.w;
		scratch->geomH = pb->rect.h;
		gte_SetGeomOffset(pb->rect.w >> 1, pb->rect.h >> 1);
		gte_SetGeomScreen(pb->distanceToScreen_PREV);
	}

	if ((idpp->instFlags & 0x40) == 0)
	{
		return 0;
	}

	if ((idpp->instFlags & 0x80) != 0)
	{
		return 0;
	}

	mh = idpp->mh;
	if (mh == 0)
	{
		return 0;
	}

	if ((mh->ptrCommandList == 0) || (mh->ptrColors == 0))
	{
		return 0;
	}

	mf = idpp->ptrCurrFrame;
	if (mf == 0)
	{
		return 0;
	}
	nextFrame = idpp->ptrNextFrame;

	anim = RenderBucket_GetAnim(inst, mh);

	if (nextFrame != 0)
	{
		scratch->frameOrigin.xy.x = mf->pos.x + nextFrame->pos.x;
		scratch->frameOrigin.xy.y = mf->pos.y + nextFrame->pos.y;
		scratch->frameOrigin.z = (s32)(mf->pos.z + nextFrame->pos.z) << 1;
	}
	else
	{
		scratch->frameOrigin.xy.x = mf->pos.x & 0x7fff;
		scratch->frameOrigin.xy.y = mf->pos.y;
		scratch->frameOrigin.z = mf->pos.z;
	}

	gte_SetRotMatrix(&idpp->mvp);
	gte_SetTransMatrix(&idpp->mvp);
	scratch->instFlags = idpp->instFlags;
	RenderBucket_WritePackedWord((u8 *)scratch + offsetof(struct RenderBucketExecuteScratch, primitiveState.split.alphaScale), (u32)(s32)idpp->alphaScale);
	if ((idpp->instFlags & SPLIT_STATE_MASK) != 0)
	{
		RenderBucket_GteLoadLightMatrixWords(&idpp->m3x3);
		// NOTE(aalhendi): Retail Execute owns these split scratch words before
		// callback dispatch.
		scratch->splitLinePrimary = idpp->splitLine;
		scratch->primitiveState.split.splitLineSecondary = idpp->splitLine;
		scratch->primitiveState.split.splitLineTertiary = idpp->splitLine;
		scratch->splitInstanceSpecLightX = (u8)inst->specLightX;
		scratch->splitReflectionRGBA = inst->reflectionRGBA;
		scratch->primitiveState.split.splitFixedLine = ((u32)(s32)idpp->splitLine) << 17;
		scratch->primitiveState.split.splitCounterPrimary = 0;
		scratch->primitiveState.split.splitCounterSecondary = 0;
		ctx->splitPlane = ((s32)CFC2(26) << 1) - (s32)CFC2(7);
	}

	// NOTE(aalhendi): Retail 0x8006ac94 clears scratch 0x58 before setup/draw.
	scratch->setupDrawClear = 0;

	ctx->inst = inst;
	ctx->idpp = idpp;
	ctx->pb = pb;
	ctx->primMem = primMem;
	ctx->mh = mh;
	ctx->mf = mf;
	ctx->anim = anim;
	ctx->vertData = MODELFRAME_GETVERT(mf);
	ctx->waterSplitSide = -1;
	if (nextFrame != 0)
	{
		ctx->nextVertData = (char *)nextFrame + mf->vertexOffset;
	}
	return 1;
}

#include "RenderBucket_Execute.c"

#ifndef CTR_NATIVE
#undef RenderBucket_UncompressAnimationFrame
#include "RenderBucket_UncompressAnimationFrame.c"
#include "RenderBucket_UncompressAnimationFrame_NextFrame.c"
#include "RenderBucket_QueueInstances.c"
#include "RenderBucket_QueueDraw.c"
#include "RenderBucket_InstSetupCallbacks.c"
#include "RenderBucket_UncompressAnimationFrame_Transformed.c"
#include "RenderBucket_Reflection.c"
#endif
