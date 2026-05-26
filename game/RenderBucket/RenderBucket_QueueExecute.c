#include <common.h>

#ifdef CTR_INTERNAL
volatile int gCtrDebugTires = 0;
volatile int gCtrDebugTireBudget = 0;
volatile int gCtrDebugTireLevel = -1;

enum
{
	CTR_TIREDBG_RENDERBUCKET = 1 << 0,
	CTR_TIREDBG_SOLID_STAGE = 1 << 1,
	CTR_TIREDBG_SOLID_PRIM = 1 << 2,
	CTR_TIREDBG_REFLECT_STAGE = 1 << 3,
	CTR_TIREDBG_REFLECT_PRIM = 1 << 4,
	CTR_TIREDBG_RENDERBUCKET_PRIM = 1 << 5,
	CTR_TIREDBG_RENDERBUCKET_REJECT = 1 << 6,
	CTR_TIREDBG_RENDERBUCKET_UNHANDLED = 1 << 7,
};

static int CtrTireDebug_ShouldLog(int mask)
{
	if ((gCtrDebugTires & mask) == 0)
		return 0;

	if (gCtrDebugTireBudget == 0)
		return 0;

	if (gCtrDebugTireLevel >= 0)
	{
		struct GameTracker *gGT = sdata->gGT;
		if ((gGT == 0) || (gGT->levelID != gCtrDebugTireLevel))
			return 0;
	}

	if (gCtrDebugTireBudget > 0)
		gCtrDebugTireBudget--;

	return 1;
}
#endif

struct RenderBucketEntry
{
	struct Instance *inst;
	struct Instance *instPlayerBase;
};

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

_Static_assert(sizeof(struct RenderBucketSplitVertex) == 0x18);
_Static_assert(offsetof(struct RenderBucketSplitVertex, xy) == 0x0);
_Static_assert(offsetof(struct RenderBucketSplitVertex, z) == 0x4);
_Static_assert(offsetof(struct RenderBucketSplitVertex, color) == 0x8);
_Static_assert(offsetof(struct RenderBucketSplitVertex, uv) == 0xc);
_Static_assert(offsetof(struct RenderBucketSplitVertex, splitDist) == 0xe);
_Static_assert(offsetof(struct RenderBucketSplitVertex, sxy) == 0x10);
_Static_assert(offsetof(struct RenderBucketSplitVertex, sz) == 0x14);

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

static int RenderBucket_SignExtendBits(u32 value, int bits)
{
	int shift = 32 - bits;
	return (s32)(value << shift) >> shift;
}

static int RenderBucket_GetSignedBits(const u32 *vertData, int *bitIndex, int bits)
{
	int const b = *bitIndex >> 5;
	int const e = 32 - bits;
	int const s = e - (*bitIndex & 31);
	u32 const ret = s < 0 ? (vertData[b] << -s) | (vertData[b + 1] >> (s & 31)) : vertData[b] >> s;

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
	u32 colorHelper = (u32)(uintptr_t)ctx->inst->funcPtr[3];

	// NOTE(aalhendi): ASM-verified helper 0x8006d4a4-0x8006d55c. This split
	// clips against model-space Y, not the depth plane used by 0x8006b4c8.
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
		// NOTE(aalhendi): ASM-verified helper 0x8006d404-0x8006d428.
		dst->color = 0x00ffffff;
	}
	else
	{
#ifdef CTR_INTERNAL
		if ((colorHelper != RB_RETAIL_INST_FUNC3_SPLIT_INTERP) && (CtrTireDebug_ShouldLog(CTR_TIREDBG_RENDERBUCKET_UNHANDLED) != 0))
		{
			fprintf(stderr, "[TIREDBG][rb-split-unhandled-color-helper] inst=%p func=%08x\n", (void *)ctx->inst, colorHelper);
		}
#endif
		// NOTE(aalhendi): ASM-verified helper 0x8006d428-0x8006d4a4 for the
		// retail table's non-white generated-vertex color path.
		dst->color = (u32)RenderBucket_InterpU8((u8)from->color, (u8)to->color, factor) |
		             ((u32)RenderBucket_InterpU8((u8)(from->color >> 8), (u8)(to->color >> 8), factor) << 8) |
		             ((u32)RenderBucket_InterpU8((u8)(from->color >> 16), (u8)(to->color >> 16), factor) << 16);
	}
	dst->splitDist = 0;
}

static struct ModelAnim *RenderBucket_GetAnim(struct Instance *inst, struct ModelHeader *mh)
{
	if (mh->ptrAnimations == 0)
		return 0;

	if (mh->numAnimations == 0)
		return 0;

	if (inst->animIndex >= mh->numAnimations)
		return 0;

	return mh->ptrAnimations[inst->animIndex];
}

static u32 RenderBucket_PackXY(int x, int y)
{
	return ((u32)(u16)x) | ((u32)(u16)y << 16);
}

static int RenderBucket_SignExtendByte(u8 value)
{
	return ((value & 0x80) != 0) ? (int)value - 0x100 : value;
}

static u32 RenderBucket_OTAddress(void *ptr)
{
	return (u32)ptr & 0xffffff;
}

static void RenderBucket_GteLoadLightMatrixWords(const MATRIX *m);

static void RenderBucket_LinkPrimRaw(u_long *otEntry, void *prim, u32 lenWord)
{
	u32 *tag = (u32 *)prim;

	// NOTE(aalhendi): Source-backs DrawInstPrim_Normal's retail OT tag write at
	// 0x8006ae50-0x8006ae64.
	*tag = *(u32 *)otEntry | lenWord;
	*(u32 *)otEntry = RenderBucket_OTAddress(prim);
}

static const u32 sRenderBucketDispatchTable8008a428[8] = {
    0x8006c948, 0x8006c974, 0x8006c928, 0x8006c928, 0x8006c948, 0x8006c948, 0x8006c984, 0x8006ad88,
};

static const u32 sRenderBucketDispatchTable8008a444[8] = {
    0x8006ad88, 0x8006b968, 0x8006ae90, 0x8006c778, 0x8006bad0, 0x8006ad6c, 0x8006d670, 0x8006d55c,
};

static const u32 sRenderBucketInstanceFunc2Table8008a460[4] = {
    0x8006d55c,
    0x8006d588,
    0x8006d5b8,
    0x8006d59c,
};

static const u32 sRenderBucketInstanceFunc3Table8008a470[4] = {
    0x8006d428,
    0x8006d428,
    0x8006d404,
    0x8006d428,
};

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
#define RB_INSTANCE_CUSTOM_MATRIX            0x800U
#define RB_INSTANCE_SPLIT_SPECIAL            0x1000U
#define RB_INSTANCE_SPLIT_STATE_MASK         0x7000U
#define RB_INSTANCE_DEPTH_FADE               0x1000000U
#define RB_INSTANCE_OWNER_PB_GATE            0x4000000U
#define RB_INSTANCE_REFLECTION_FUNC23        0x100000U
#define RB_MODEL_ALWAYS_POINT_NORTH          0x1U

static void RenderBucket_CopyDispatchTables(void)
{
	u32 *dst0 = CTR_SCRATCHPAD_PTR(u32, 0x94);
	u32 *dst1 = CTR_SCRATCHPAD_PTR(u32, 0xc4);

	// NOTE(aalhendi): ASM-verified helper 0x80071590 copies two 8-word table
	// windows from 0x8008a428/0x8008a444 into scratch 0x94/0xc4.
	for (int i = 0; i < 8; i++)
	{
		dst0[i] = sRenderBucketDispatchTable8008a428[i];
	}

	for (int i = 0; i < 8; i++)
	{
		dst1[i] = sRenderBucketDispatchTable8008a444[i];
	}
}

static void RenderBucket_WriteInstanceCallbackLabels(struct Instance *inst, u32 instFlags)
{
	u32 *setupTable = CTR_SCRATCHPAD_PTR(u32, 0x94);
	u32 *primTable = CTR_SCRATCHPAD_PTR(u32, 0xc4);
	int func01Index = ((instFlags >> 14) & 0x1c) >> 2;
	int func23Index = ((instFlags >> 18) & 0x0c) >> 2;

	// NOTE(aalhendi): Source-backs QueueDraw's retail Instance+0x5c/0x60 and
	// Instance+0x64/0x68 label stores at 0x800714b0-0x800714f8. These are retail
	// labels, not native host-callable pointers.
	inst->funcPtr[0] = (void *)(uintptr_t)setupTable[func01Index];
	inst->funcPtr[1] = (void *)(uintptr_t)primTable[func01Index];
	inst->funcPtr[2] = (void *)(uintptr_t)sRenderBucketInstanceFunc2Table8008a460[func23Index];
	inst->funcPtr[3] = (void *)(uintptr_t)sRenderBucketInstanceFunc3Table8008a470[func23Index];
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

static void RenderBucket_SelectRetailHandlers(struct InstDrawPerPlayer *idpp, u32 *instFlags, const struct RenderBucketSplitState *split)
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
					*instFlags |= RB_INSTANCE_REFLECTION_FUNC23;
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
					*instFlags |= RB_INSTANCE_REFLECTION_FUNC23;
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

	// NOTE(aalhendi): Source-backs QueueDraw's retail selector writes at
	// 0x800713b8-0x800714ac. Native now feeds the source-backed split/reflection
	// producer subset. Execute no-ops unsupported labels instead of masking them
	// through the normal draw path.
	idpp->unkEC = drawFunc;
	idpp->unkF0 = uncompressFunc;
}

static int RenderBucket_ClampOTByteOffset(int depthBin)
{
	int byteOffset = depthBin << 2;

	if (depthBin < 0)
		return 0;

	if (byteOffset > 0xffc)
		return 0xffc;

	return byteOffset;
}

static s32 RenderBucket_MipsSub(int lhs, int rhs)
{
	return (s32)((u32)lhs - (u32)rhs);
}

static int RenderBucket_WriteAlphaScale(struct Instance *inst, struct InstDrawPerPlayer *idpp, int viewDepth, u32 instFlags)
{
	int alpha = (u16)inst->alphaScale;

	// NOTE(aalhendi): Source-backs QueueDraw's 0x80070a5c depth-fade alpha gate.
	if ((instFlags & RB_INSTANCE_DEPTH_FADE) != 0)
	{
		int depthFade = RenderBucket_MipsSll(viewDepth, 1);

		if (RenderBucket_MipsSub(depthFade, 0x1000) > 0)
			return 0;

		alpha = (int)(u32)((u32)alpha + (u32)depthFade);
		if (RenderBucket_MipsSub(alpha, 0x1000) > 0)
			return 0;
	}

	idpp->alphaScale = (s16)alpha;
	return 1;
}

static void RenderBucket_ApplyOwnerPushBufferGate(struct Instance *inst, int playerIndex, u32 *instFlags)
{
	struct Thread *thread;
	struct Driver *driver;

	if ((*instFlags & RB_INSTANCE_OWNER_PB_GATE) == 0)
		return;

	thread = inst->thread;
	if (thread == 0 || thread->object == 0)
		return;

	driver = (struct Driver *)thread->object;

	// NOTE(aalhendi): Source-backs QueueDraw's 0x800709d4 owner PB clear gate.
	if ((s8)driver->driverID == playerIndex)
		*instFlags &= ~PUSHBUFFER_EXISTS;
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
	int x;
	int y;
	s32 diff;

	// NOTE(aalhendi): ASM-verified helper 0x80071524; native models retail's
	// t1/t2 -> t3/t4/t5/t6/t7/s0 register tuple as an explicit bounds struct.
	y = RenderBucket_PackedSxyY(sxy);
	diff = RenderBucket_MipsSub(y, bounds->minY);
	if (diff < 0)
		bounds->minY = y;
	diff = RenderBucket_MipsSub(y, bounds->maxY);
	if (diff > 0)
		bounds->maxY = y;

	x = RenderBucket_PackedSxyX(sxy);
	diff = RenderBucket_MipsSub(x, bounds->minX);
	if (diff < 0)
		bounds->minX = x;
	diff = RenderBucket_MipsSub(x, bounds->maxX);
	if (diff > 0)
		bounds->maxX = x;

	diff = RenderBucket_MipsSub(depth, bounds->minZ);
	if (diff < 0)
		bounds->minZ = depth;
	diff = RenderBucket_MipsSub(depth, bounds->maxZ);
	if (diff > 0)
		bounds->maxZ = depth;
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
		RenderBucket_BoundsUpdate_80071524(bounds, sxy0, depth0);
	RenderBucket_BoundsUpdate_80071524(bounds, sxy1, depth1);
	RenderBucket_BoundsUpdate_80071524(bounds, sxy2, depth2);
}

static u32 RenderBucket_PackedFrameXY(struct ModelFrame *frame, struct ModelFrame *nextFrame)
{
	u32 packedXY = *(u32 *)&frame->pos[0];
	int blendedY;
	int blendedX;

	if (nextFrame == 0)
		return packedXY;

	// NOTE(aalhendi): Retail QueueDraw averages current/next frame bounds at
	// 0x80070db4-0x80070df0 before the 0x3fc bbox projection.
	blendedY = (((int)nextFrame->pos[1] + ((int)packedXY >> 16)) >> 2) << 17;
	blendedX = (((int)nextFrame->pos[0] + (s16)packedXY) >> 1) & 0xffff;
	return blendedY | blendedX;
}

static int RenderBucket_FrameZ(struct ModelFrame *frame, struct ModelFrame *nextFrame)
{
	if (nextFrame == 0)
		return frame->pos[2];

	return (frame->pos[2] + nextFrame->pos[2]) >> 1;
}

static int RenderBucket_ProjectFrameBounds(struct ModelFrame *frame, struct ModelFrame *nextFrame, struct PushBuffer *pb, const MATRIX *projectionMvp,
                                           struct RenderBucketBounds *bounds)
{
	u32 minXY = (RenderBucket_PackedFrameXY(frame, nextFrame) << 2) & 0xfff8ffff;
	u32 minZ = ((u32)RenderBucket_FrameZ(frame, nextFrame)) << 2;
	u32 maxXMinY = (minXY + 0x3fc) & 0xfff8ffff;
	u32 maxXMaxY = maxXMinY + 0x03fc0000;
	u32 minXMaxY = minXY + 0x03fc0000;
	u32 maxZ = minZ + 0x3fc;
	int geomScreenHalf;

	// NOTE(aalhendi): Retail projects the bbox through packed GTE VXY/VZ
	// registers at 0x80071054-0x80071164 and folds min/max through ASM-verified
	// helper 0x80071524. Native keeps the register tuple as explicit bounds fields.
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
	geomScreenHalf = (int)CFC2(26) >> 1;
	RenderBucket_BoundsUpdate_80071524(bounds, MFC2(13), MFC2(18));

	if (bounds->maxX < 0)
		return 0;

	if (bounds->maxY < 0)
		return 0;

	if (bounds->maxZ - geomScreenHalf < 0)
		return 0;

	if (bounds->minX > pb->rect.w)
		return 0;

	if (bounds->minY > pb->rect.h)
		return 0;

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

	pos.vx = inst->matrix.t[0] - pb->matrix_Camera.t[0];
	pos.vy = inst->matrix.t[1] - pb->matrix_Camera.t[1];
	pos.vz = inst->matrix.t[2] - pb->matrix_Camera.t[2];

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

static struct ModelHeader *RenderBucket_SelectModelHeader(struct Instance *inst, struct PushBuffer *pb, int *lodIndexOut, VECTOR *viewPos, int viewDepth)
{
	struct ModelHeader *mh;
	int projectedDistance;

	*lodIndexOut = 0;

	if (inst->model->numHeaders <= 0)
		return 0;

	if (pb->distanceToScreen_PREV == 0)
		return 0;

	// NOTE(aalhendi): Retail keeps the low 32 bits of this product before dividing by GTE H.
	projectedDistance = (int)(u32)((s64)(pb->rect.w >> 1) * viewDepth) / pb->distanceToScreen_PREV;
	mh = inst->model->headers;

	// NOTE(aalhendi): PSX-backfeed blocker: retail LOD selection at
	// 0x80070ae4-0x80070b34 uses transformed GTE depth in scratchpad 0x8c,
	// divides by GTE H, and walks ModelHeader entries through s4/s5. Native C
	// preserves the projected-distance comparison explicitly until the register
	// walk is restored.
	for (int lodIndex = 0; lodIndex < inst->model->numHeaders; lodIndex++, mh++)
	{
		if ((projectedDistance - (u16)mh->maxDistanceLOD) < 0)
		{
			*lodIndexOut = lodIndex;
			RenderBucket_AdjustViewPositionForMvp(inst, viewPos);
			return mh;
		}
	}

	return 0;
}

static void RenderBucket_GteLoadRotMatrixWords(u32 m0, u32 m1, u32 m2, u32 m3, u32 m4)
{
	CTC2(m0, 0);
	CTC2(m1, 1);
	CTC2(m2, 2);
	CTC2(m3, 3);
	CTC2(m4, 4);
}

static void RenderBucket_StoreMatrixWords(MATRIX *m, u32 m0, u32 m1, u32 m2, u32 m3, u32 m4)
{
	*(u32 *)&m->m[0][0] = m0;
	*(u32 *)&m->m[0][2] = m1;
	*(u32 *)&m->m[1][1] = m2;
	*(u32 *)&m->m[2][0] = m3;
	*(u32 *)&m->m[2][2] = m4;
}

static void RenderBucket_LoadMatrixWords(const MATRIX *m, u32 *m0, u32 *m1, u32 *m2, u32 *m3, u32 *m4)
{
	*m0 = *(u32 *)&m->m[0][0];
	*m1 = *(u32 *)&m->m[0][2];
	*m2 = *(u32 *)&m->m[1][1];
	*m3 = *(u32 *)&m->m[2][0];
	*m4 = *(u32 *)&m->m[2][2];
}

static void RenderBucket_GteLoadLightMatrixWords(const MATRIX *m)
{
	CTC2(*(u32 *)&m->m[0][0], 8);
	CTC2(*(u32 *)&m->m[0][2], 9);
	CTC2(*(u32 *)&m->m[1][1], 10);
	CTC2(*(u32 *)&m->m[2][0], 11);
	CTC2((u16)m->m[2][2], 12);
}

static void RenderBucket_GteScaleMatrixColumns(u32 *m0, u32 *m1, u32 *m2, u32 *m3, u32 *m4)
{
	u32 v0;
	u32 v1;
	u32 v2;
	u32 t0;
	u32 t1;
	u32 t2;
	u32 t3;
	u32 t4;

	// NOTE(aalhendi): PSX-backfeed blocker: retail helper 0x8006c49c consumes
	// matrix words through t3/t4/t5/t6/t7 and live GTE color/rotation state.
	// Native mirrors the opcode sequence with explicit C parameters until that
	// register-entry ABI is restored.
	MTC2((*m0 & 0xffff) | (*m1 & 0xffff0000), 0);
	MTC2(*m3, 1);
	doCOP2(0x0486012);

	MTC2((*m0 >> 16) | (*m2 << 16), 2);
	MTC2(*m3 >> 16, 3);
	t0 = MFC2(9);
	t1 = MFC2(10);
	t3 = MFC2(11);
	doCOP2(0x048e012);

	MTC2((*m1 & 0xffff) | (*m2 & 0xffff0000), 4);
	MTC2(*m4, 5);
	t0 &= 0xffff;
	t1 <<= 16;
	t3 &= 0xffff;

	v0 = MFC2(9);
	v1 = MFC2(10);
	v2 = MFC2(11);
	doCOP2(0x0496012);

	v0 <<= 16;
	*m0 = t0 | v0;
	v1 &= 0xffff;
	v2 <<= 16;
	*m3 = t3 | v2;

	v0 = MFC2(9);
	t2 = MFC2(10);
	t4 = MFC2(11);
	v0 &= 0xffff;
	*m1 = t1 | v0;
	t2 <<= 16;
	*m2 = v1 | t2;
	*m4 = t4;

	RenderBucket_GteLoadRotMatrixWords(*m0, *m1, *m2, *m3, *m4);
}

static void RenderBucket_GteMulLightMatrixColumns(u32 *m0, u32 *m1, u32 *m2, u32 *m3, u32 *m4)
{
	u32 v0;
	u32 v1;
	u32 v2;
	u32 t0;
	u32 t1;
	u32 t2;
	u32 t3;
	u32 t4;

	// NOTE(aalhendi): Source-backs helper 0x8006c558, which QueueDraw uses to
	// compose the scaled model matrix through the current PushBuffer light matrix.
	MTC2((*m0 & 0xffff) | (*m1 & 0xffff0000), 0);
	MTC2(*m3, 1);
	doCOP2(0x04a6012);

	MTC2((*m0 >> 16) | (*m2 << 16), 2);
	MTC2(*m3 >> 16, 3);
	t0 = MFC2(9);
	t1 = MFC2(10);
	t3 = MFC2(11);
	doCOP2(0x04ae012);

	MTC2((*m1 & 0xffff) | (*m2 & 0xffff0000), 4);
	MTC2(*m4, 5);
	t0 &= 0xffff;
	t1 <<= 16;
	t3 &= 0xffff;

	v0 = MFC2(9);
	v1 = MFC2(10);
	v2 = MFC2(11);
	doCOP2(0x04b6012);

	v0 <<= 16;
	*m0 = t0 | v0;
	v1 &= 0xffff;
	v2 <<= 16;
	*m3 = t3 | v2;

	v0 = MFC2(9);
	t2 = MFC2(10);
	t4 = MFC2(11);
	v0 &= 0xffff;
	*m1 = t1 | v0;
	t2 <<= 16;
	*m2 = v1 | t2;
	*m4 = t4;
}

static int RenderBucket_GetScaledMatrixElem(struct Instance *inst, int scale[3], int row, int col)
{
	return (inst->matrix.m[row][col] * scale[col]) >> 8;
}

static void RenderBucket_BuildM3x3(struct Instance *inst, struct ModelHeader *mh, int viewDepth, struct InstDrawPerPlayer *idpp,
                                   struct RenderBucketMatrixState *matrixState)
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

	// NOTE(aalhendi): PSX-backfeed blocker: retail QueueDraw produces IDPP
	// m3x3 at 0x80070b38-0x80070c18 by loading the instance matrix through
	// t3/t4/t5/t6/t7, setting the GTE color matrix from ModelHeader scale, then
	// calling helper 0x8006c49c. Native preserves the same field and GTE helper
	// sequence through explicit C parameters until the exact register/scratchpad
	// boundary is restored.
	// NOTE(aalhendi): Retail treats MATRIX halfwords as packed GTE register words here.
	m0 = *(u32 *)&inst->matrix.m[0][0];
	m1 = *(u32 *)&inst->matrix.m[0][2];
	m2 = *(u32 *)&inst->matrix.m[1][1];
	m3 = *(u32 *)&inst->matrix.m[2][0];
	m4 = *(u32 *)&inst->matrix.m[2][2];
	RenderBucket_GteLoadRotMatrixWords(m0, m1, m2, m3, m4);

	depthShift = (viewDepth < 0x1000) ? 2 : 0;
	scaleXYShift = 0x12 - depthShift;
	scaleZShift = 2 - depthShift;
	packedScaleXY = *(u32 *)&mh->scale[0];

	CTC2((packedScaleXY << 16) >> scaleXYShift, 16);
	CTC2(0, 17);
	CTC2(packedScaleXY >> scaleXYShift, 18);
	CTC2(0, 19);
	CTC2((u16)mh->scale[2] >> scaleZShift, 20);

	scaleX = inst->scale[0];
	scaleY = inst->scale[1];
	scaleZ = inst->scale[2];
	if ((inst->flags & PIXEL_LOD) != 0)
	{
		int pixelScale = (viewDepth >> 1) + 0x1000;

		scaleX = (pixelScale * scaleX) >> 12;
		scaleY = (pixelScale * scaleY) >> 12;
		scaleZ = (pixelScale * scaleZ) >> 12;
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
	RenderBucket_GteScaleMatrixColumns(&m0, &m1, &m2, &m3, &m4);
	matrixState->m0 = m0;
	matrixState->m1 = m1;
	matrixState->m2 = m2;
	matrixState->m3 = m3;
	matrixState->m4 = m4;
	RenderBucket_StoreMatrixWords(&idpp->m3x3, m0, m1, m2, m3, m4);
}

static void RenderBucket_BuildMvp(struct PushBuffer *pb, struct InstDrawPerPlayer *idpp, const VECTOR *viewPos, MATRIX *projectionMvp)
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
	idpp->mvp.t[0] = viewPos->vx;
	idpp->mvp.t[1] = viewPos->vy;
	idpp->mvp.t[2] = viewPos->vz;
	*projectionMvp = idpp->mvp;
	RenderBucket_GteLoadRotMatrixWords(m0, m1, m2, m3, m4);
}

static void RenderBucket_StoreViewMatrixForSplit(struct InstDrawPerPlayer *idpp)
{
	RenderBucket_StoreMatrixWords(&idpp->mvp, CFC2(8), CFC2(9), CFC2(10), CFC2(11), CFC2(12));
}

static int RenderBucket_NeedsCustomMatrix(u32 instFlags, const struct ModelHeader *mh)
{
	return ((instFlags & RB_INSTANCE_CUSTOM_MATRIX) != 0) || ((mh->flags & RB_MODEL_ALWAYS_POINT_NORTH) != 0);
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
		viewX = -viewX;

	MTC2(viewX, 9);
	MTC2(viewZ, 10);
	doCOP2(0x0a00428);
	length = SquareRoot0_stub(MFC2(25) + MFC2(26));
	if (length == 0)
		return;

	m0 = (u16)(RenderBucket_MipsSll(viewZ, 12) / length);
	m1 = (u16)(RenderBucket_MipsSll(viewX, 12) / length);
	m2 = 0x1000;
	m3 = (u16)-m1;
	m4 = m0;

	RenderBucket_GteLoadRotMatrixWords(matrixState->m0, matrixState->m1, matrixState->m2, matrixState->m3, matrixState->m4);
	RenderBucket_GteScaleMatrixColumns(&m0, &m1, &m2, &m3, &m4);
	RenderBucket_GteLoadRotMatrixWords(0x1000, 0, 0xfffff600, 0, 0x1000);
	RenderBucket_GteScaleMatrixColumns(&m0, &m1, &m2, &m3, &m4);
	RenderBucket_StoreMatrixWords(&idpp->mvp, m0, m1, m2, m3, m4);
	RenderBucket_StoreMatrixWords(projectionMvp, m0, m1, m2, m3, m4);
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

	if ((*instFlags & RB_INSTANCE_SPLIT_STATE_MASK) == 0)
	{
		if (RenderBucket_NeedsCustomMatrix(*instFlags, mh) != 0)
			RenderBucket_BuildCustomMatrix(idpp, *instFlags, matrixState, projectionMvp);
		return split;
	}

	if ((*instFlags & RB_INSTANCE_SPLIT_SPECIAL) != 0)
	{
		int specialDepth = viewDepth - ((int)pb->distanceToScreen_PREV << 2) - ((matrixState->scratch78 * 1448) >> 12);

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
			RenderBucket_LoadMatrixWords(projectionMvp, &m0, &m1, &m2, &m3, &m4);
			RenderBucket_StoreMatrixWords(&idpp->m3x3, m0, m1, m2, m3, m4);
			RenderBucket_StoreMatrixWords(&idpp->mvp, 0x1000, 0, 0x1000, 0, 0x1000);
			split.scratch70 = *instFlags;
		}
		else if (RenderBucket_NeedsCustomMatrix(*instFlags, mh) != 0)
		{
			RenderBucket_BuildCustomMatrix(idpp, *instFlags, matrixState, projectionMvp);
		}

		return split;
	}

	if (matrixState->scratch76 == 0)
	{
		// TODO(aalhendi): Confirm retail divide-by-zero behavior for this path
		// before mirroring it on native.
		return split;
	}

	frameY = (s16)(RenderBucket_PackedFrameXY(frame, nextFrame) >> 16);
	baseY = inst->matrix.t[1] + ((frameY / matrixState->scratch76) >> 12);
	rawSplit = inst->vertSplit - baseY;
	matrixOr = matrixState->m0 | matrixState->m1 | matrixState->m2 | matrixState->m3 | matrixState->m4;

	if (matrixOr == 0x1000)
	{
		splitLine = (RenderBucket_MipsSll(rawSplit, 12) / matrixState->scratch76) << 2;
		idpp->splitLine = (s16)splitLine;
		split.scratch68 = splitLine;
		*instFlags &= ~PUSHBUFFER_EXISTS;
		if (RenderBucket_NeedsCustomMatrix(*instFlags, mh) != 0)
			RenderBucket_BuildCustomMatrix(idpp, *instFlags, matrixState, projectionMvp);
		return split;
	}

	splitLine = RenderBucket_MipsSll(rawSplit, 2);
	idpp->splitLine = (s16)splitLine;
	if (((*instFlags & REFLECTIVE) == 0) && ((splitLine + 362) < 0))
	{
		if (RenderBucket_NeedsCustomMatrix(*instFlags, mh) != 0)
			RenderBucket_BuildCustomMatrix(idpp, *instFlags, matrixState, projectionMvp);
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
		// NOTE(aalhendi): Source-backs 0x80070ea4-0x80070ed8. Retail stores
		// the PushBuffer view matrix in IDPP for the later split/reflection
		// Execute path while immediate bounds still use the composed projection
		// matrix.
		RenderBucket_StoreViewMatrixForSplit(idpp);
	}

	return split;
}

static u_long *RenderBucket_AllocateOTRange(struct OTMem *otMem, struct PushBuffer *pb, int minDepth, int maxDepth, int viewDepth, int depthBias,
                                            int usePushBuffer)
{
	u_long *rangeStart;
	u_long *rangeEnd;
	u_long *newCurr;
	u_long *otSlot;
	int range;
	int byteOffset;

	if (otMem == 0)
		return 0;

	if (pb->ptrOT == 0)
		return 0;

	range = maxDepth - minDepth;
	if (range < 0)
		return 0;

	rangeStart = otMem->curr;
	rangeEnd = rangeStart + range;
	newCurr = rangeEnd + 1;

	// NOTE(aalhendi): Retail rejects when `otMemEnd - newCurr <= 0` at
	// 0x80071230-0x80071234, so `end - 1 word` remains a valid final cursor.
	if (newCurr >= otMem->end)
		return 0;

	// NOTE(aalhendi): PSX-backfeed blocker: retail QueueDraw links per-instance
	// OT ranges by writing raw 24-bit primitive tags at 0x80071218-0x800712f4.
	// Native uses the same tag-chain shape with explicit C pointer masking.
	otMem->curr = newCurr;

	byteOffset = RenderBucket_ClampOTByteOffset((viewDepth >> 6) + depthBias);

	if (usePushBuffer != 0)
	{
		// NOTE(aalhendi): PSX-backfeed blocker: retail QueueDraw stores the
		// range start/end/byte-offset metadata at PushBuffer 0xf4/0xf8/0xfc for
		// PUSHBUFFER_EXISTS instances. Native keeps 0xf4 as ptrOT until
		// DrawInstPrim/PushBuffer ownership is restored, so the range start is
		// held by IDPP unkE4 and the extra metadata is stored in named fields.
		rangeStart[0] = 0;
		pb->renderBucketOTRangeEnd = rangeEnd;
		pb->renderBucketOTByteOffset = byteOffset;
	}
	else
	{
		otSlot = (u_long *)((char *)pb->ptrOT + byteOffset);
		rangeStart[0] = otSlot[0];
		otSlot[0] = RenderBucket_OTAddress(rangeEnd);
	}

	for (u_long *entry = rangeStart; entry != rangeEnd; entry++)
	{
		entry[1] = RenderBucket_OTAddress(entry);
	}

	return rangeStart - minDepth;
}

static void RenderBucket_UpdatePushBufferMetadata(struct PushBuffer *pb, const struct RenderBucketBounds *bounds, u32 *instFlags)
{
	int width;
	int height;
#ifdef CTR_INTERNAL
	u32 beforeFlags;
#endif

	if ((*instFlags & PUSHBUFFER_EXISTS) == 0)
		return;

#ifdef CTR_INTERNAL
	beforeFlags = *instFlags;
#endif

	// NOTE(aalhendi): PSX-backfeed blocker: retail QueueDraw writes projected
	// screen pos/size to PushBuffer 0x100/0x104 and clears PUSHBUFFER_EXISTS
	// when the projected bounds exceed the small-buffer constraints at
	// 0x80071164-0x800711c8. Native mirrors the dataflow explicitly until the
	// exact scratchpad/register path is restored.
	width = bounds->maxX - bounds->minX;
	height = bounds->maxY - bounds->minY;
	pb->renderBucketScreenPos = RenderBucket_PackXY(bounds->minX, bounds->minY);
	pb->renderBucketScreenSize = RenderBucket_PackXY(width, height);

	if (bounds->minX < 0 || bounds->minY < 0 || height > 0x40 || width > 0x60 || bounds->maxX >= pb->rect.w || bounds->maxY >= pb->rect.h)
	{
		*instFlags &= ~PUSHBUFFER_EXISTS;
	}

#ifdef CTR_INTERNAL
	if (CtrTireDebug_ShouldLog(CTR_TIREDBG_RENDERBUCKET) != 0)
	{
		fprintf(stderr, "[TIREDBG][rb-pb] flags=%08x->%08x min=(%d,%d) max=(%d,%d) size=%dx%d rect=%dx%d pb=%p\n", beforeFlags, *instFlags, bounds->minX,
		        bounds->minY, bounds->maxX, bounds->maxY, width, height, pb->rect.w, pb->rect.h, (void *)pb);
	}
#endif
}

static int RenderBucket_ShouldAllocateSecondaryRange(u32 instFlags, const struct RenderBucketSplitState *split)
{
	if ((instFlags & PUSHBUFFER_EXISTS) != 0)
		return 0;

	if ((instFlags & (SPLIT_LINE | REFLECTIVE)) == 0)
		return 0;

	if ((instFlags & REFLECTIVE) != 0)
		return 1;

	// NOTE(aalhendi): Source-backs QueueDraw's secondary-range gate at
	// 0x80071300-0x80071320 using the split/reflection producer state.
	return RenderBucket_HasSplitOutput(split);
}

static int RenderBucket_BuildDepthRange(struct Instance *inst, struct ModelFrame *frame, struct ModelFrame *nextFrame, struct PushBuffer *pb,
                                        struct InstDrawPerPlayer *idpp, struct OTMem *otMem, int viewDepth, u32 *instFlags,
                                        const struct RenderBucketSplitState *split, const MATRIX *projectionMvp)
{
	struct RenderBucketBounds bounds;
	u_long *secondaryRange;
	int minDepth;
	int maxDepth;

	if (RenderBucket_ProjectFrameBounds(frame, nextFrame, pb, projectionMvp, &bounds) == 0)
		return 0;

	RenderBucket_UpdatePushBufferMetadata(pb, &bounds, instFlags);
	minDepth = (bounds.minZ >> 5) - 2;
	maxDepth = (bounds.maxZ >> 5) + 1;

	idpp->depthOffset[0] = minDepth;
	idpp->depthOffset[1] = maxDepth;
	idpp->unkE4 = (int)RenderBucket_AllocateOTRange(otMem, pb, minDepth, maxDepth, viewDepth, RenderBucket_SignExtendByte(inst->unk50),
	                                                (*instFlags & PUSHBUFFER_EXISTS) != 0);
	idpp->unkE8 = idpp->unkE4;

	if (idpp->unkE4 == 0)
		return 0;

	if (RenderBucket_ShouldAllocateSecondaryRange(*instFlags, split) != 0)
	{
		// NOTE(aalhendi): Retail QueueDraw allocates this distinct
		// reflected/split OT range at 0x80071320-0x800713b4 with signed
		// Instance.unk51 as the second depth bias. The allocation predicate is
		// source-backed; the split-line scratch producer is still pending.
		secondaryRange = RenderBucket_AllocateOTRange(otMem, pb, minDepth, maxDepth, viewDepth, RenderBucket_SignExtendByte(inst->unk51), 0);
		if (secondaryRange == 0)
			return 0;

		idpp->unkE8 = (int)secondaryRange;
	}

	return 1;
}

static void RenderBucket_StoreInstanceAnimWord(struct Instance *inst, int frame)
{
	// NOTE(aalhendi): Retail QueueDraw uses `sw` at Instance+0x54, so this
	// intentionally writes the full animFrame/vertSplit word.
	*(u32 *)((char *)inst + offsetof(struct Instance, animFrame)) = (u32)frame;
}

static void RenderBucket_AdvanceInstanceAnimWord(struct Instance *inst, int gameMode1, int playerIndex, int lastFrame, u32 *queuedFlags)
{
	int frame;

	if (gameMode1 != 0)
		return;

	if (playerIndex != 0)
		return;

	if (lastFrame < 0)
		return;

	if ((*queuedFlags & 0x30) == 0)
		return;

	frame = (u16)inst->animFrame;

	if ((*queuedFlags & 0x20) == 0)
	{
		if ((lastFrame - frame) > 0)
			frame++;
		else
			frame = 0;
	}

	else if ((*queuedFlags & 0x10) != 0)
	{
		if ((lastFrame - frame) > 0)
			frame++;
		else
		{
			*queuedFlags &= ~0x10;
			frame = lastFrame - 1;
		}
	}

	else
	{
		if (frame > 0)
			frame--;
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
	char *firstFrame;

	*nextFrameOut = 0;
	*deltaArrayOut = 0;
	*lastFrameAdvanceOut = -1;

	if (mh->ptrFrameData != 0)
	{
		*deltaArrayOut = mh->unk3;
		return mh->ptrFrameData;
	}

	anim = RenderBucket_GetAnim(inst, mh);
	if (anim == 0)
		return 0;

	if (anim->numFrames == 0)
		return 0;

	// NOTE(aalhendi): PSX-backfeed blocker: retail frame selection at
	// 0x80070ca0-0x80070dfc uses stack fields and returns current/next frame
	// through s6/s1 plus ptrDeltaArray through IDPP 0xd4. Native keeps the same
	// frame-selection rules as explicit return values until the register ABI is
	// restored.
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

	if (frameIndex > lastFrame)
		frameIndex = lastFrame;

	firstFrame = (char *)MODELANIM_GETFRAME(anim);

	if (hasNextFrame != 0)
		*nextFrameOut = (struct ModelFrame *)(firstFrame + (anim->frameSize * (frameIndex + 1)));

	return (struct ModelFrame *)(firstFrame + (anim->frameSize * frameIndex));
}

static struct RenderBucketEntry *RenderBucket_QueueDraw(struct Instance *inst, struct RenderBucketEntry *rbi, int playerIndex, u32 lodMask, int gameMode1,
                                                        struct OTMem *otMem)
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
	int viewDepth;
	VECTOR viewPos;

	// NOTE(aalhendi): PSX-backfeed blocker: retail QueueDraw consumes implicit
	// scratchpad/register state from QueueLev/QueueNonLev. This native helper
	// spells that state out as C parameters until the full ASM audit is done.

	if (inst == 0)
		return rbi;

	if (inst->model == 0)
		return rbi;

	if (inst->model->headers == 0)
		return rbi;

	if ((inst->flags & lodMask) == 0)
		return rbi;

	instPlayerBase = (struct Instance *)((char *)inst + (playerIndex * sizeof(struct InstDrawPerPlayer)));
	idpp = INST_GETIDPP(instPlayerBase);
	pb = idpp->pushBuffer;

	if (pb == 0)
		return rbi;

	queuedFlags = inst->flags;
	RenderBucket_ApplyOwnerPushBufferGate(inst, playerIndex, &queuedFlags);
	idpp->instFlags = queuedFlags & ~DRAW_SUCCESSFUL;
	RenderBucket_GetViewPosition(inst, pb, &viewPos);
	viewDepth = viewPos.vz;
	if (RenderBucket_WriteAlphaScale(inst, idpp, viewDepth, queuedFlags) == 0)
		return rbi;

	mh = RenderBucket_SelectModelHeader(inst, pb, &lodIndex, &viewPos, viewDepth);
	if (mh == 0)
		return rbi;

	frame = RenderBucket_GetFrame(inst, mh, &nextFrame, &deltaArray, &lastFrameAdvance);
	if (frame == 0)
		return rbi;

	RenderBucket_AdvanceInstanceAnimWord(inst, gameMode1, playerIndex, lastFrameAdvance, &queuedFlags);
	idpp->instFlags = queuedFlags & ~DRAW_SUCCESSFUL;
	idpp->splitLine = 0;
	idpp->ptrCurrFrame = frame;
	idpp->ptrNextFrame = nextFrame;
	idpp->ptrCommandList = mh->ptrCommandList;
	idpp->ptrTexLayout = mh->ptrTexLayout;
	idpp->ptrColorLayout = (u32)mh->ptrColors;
	idpp->ptrDeltaArray = deltaArray;
	idpp->lodIndex = lodIndex;
	idpp->mh = mh;
	RenderBucket_BuildM3x3(inst, mh, viewDepth, idpp, &matrixState);
	RenderBucket_BuildMvp(pb, idpp, &viewPos, &projectionMvp);
	split = RenderBucket_BuildSplitState(inst, mh, frame, nextFrame, pb, idpp, viewDepth, &queuedFlags, &matrixState, &projectionMvp);
	idpp->unkE4 = 0;
	idpp->unkE8 = 0;
	idpp->unkEC = 0;
	idpp->unkF0 = 0;

	if (RenderBucket_BuildDepthRange(inst, frame, nextFrame, pb, idpp, otMem, viewDepth, &queuedFlags, &split, &projectionMvp) == 0)
		return rbi;

	RenderBucket_SelectRetailHandlers(idpp, &queuedFlags, &split);
	RenderBucket_WriteInstanceCallbackLabels(inst, queuedFlags);
	idpp->instFlags = queuedFlags | DRAW_SUCCESSFUL;
	rbi->inst = inst;
	rbi->instPlayerBase = instPlayerBase;
	return rbi + 1;
}

void *RenderBucket_QueueLevInstances(struct CameraDC *cDC, u_long *otMem, void *rbi, char *lod, char numPlyr, int gameMode1)
{
	struct RenderBucketEntry *entry = (struct RenderBucketEntry *)rbi;
	u32 lodMask = (u32)(u8)(u32)lod;
	int count = (int)(u8)numPlyr;

	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80070720-0x8007084c.
	// Retail enters QueueDraw through a scratch/register
	// ABI; native passes the same state as explicit C parameters.
	RenderBucket_CopyDispatchTables();

	for (int player = count - 1; player >= 0; player--)
	{
		struct Instance **visInstSrc = cDC[player].visInstSrc;

		if (visInstSrc == 0)
			continue;

		for (; *visInstSrc != 0; visInstSrc++)
		{
			entry = RenderBucket_QueueDraw(*visInstSrc, entry, player, lodMask, gameMode1, (struct OTMem *)otMem);
		}
	}

	return entry;
}

void *RenderBucket_QueueNonLevInstances(struct Item *item, u_long *otMem, void *rbi, char *lod, char numPlyr, int gameMode1)
{
	struct RenderBucketEntry *entry = (struct RenderBucketEntry *)rbi;
	u32 lodMask = (u32)(u8)(u32)lod;
	int count = (int)(u8)numPlyr;

	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8007084c-0x80070950.
	// Retail enters QueueDraw through a scratch/register
	// ABI; native passes the same state as explicit C parameters.
	RenderBucket_CopyDispatchTables();

	for (int player = count - 1; player >= 0; player--)
	{
		for (struct Item *curr = item; curr != 0; curr = curr->next)
		{
			entry = RenderBucket_QueueDraw((struct Instance *)curr, entry, player, lodMask, gameMode1, (struct OTMem *)otMem);
		}
	}

	return entry;
}

static u32 RenderBucket_PackModelVertexXY(struct RenderBucketDrawContext *ctx, const RenderBucketVertex *vertex)
{
	u32 frameOriginXY = (u16)(ctx->mf->pos[0] & 0x7fff) | ((u32)(u16)ctx->mf->pos[1] << 16);
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
	u32 frameOriginXY = (u16)(ctx->mf->pos[0] + nextFrame->pos[0]);
	u32 currXZ = ((u32)curr->x) | ((u32)curr->z << 16);
	u32 nextXZ = ((u32)next->x) | ((u32)next->z << 16);

	frameOriginXY |= (u32)(u16)(ctx->mf->pos[1] + nextFrame->pos[1]) << 16;

	// NOTE(aalhendi): Source-backs retail next-frame decoder 0x8006b464-
	// 0x8006b480: current and next vertex X/Z bytes are added with the summed
	// frame origin, then shifted by one instead of the non-interpolated << 2.
	return ((currXZ + nextXZ + frameOriginXY) << 1) & 0xfff8ffff;
}

static u32 RenderBucket_ModelVertexZ(struct RenderBucketDrawContext *ctx, const RenderBucketVertex *vertex)
{
	return ((u32)(ctx->mf->pos[2] + vertex->y)) << 2;
}

static u32 RenderBucket_InterpolatedModelVertexZ(struct RenderBucketDrawContext *ctx, const RenderBucketVertex *curr, const RenderBucketVertex *next)
{
	struct ModelFrame *nextFrame = ctx->idpp->ptrNextFrame;
	int z = (int)curr->y + (int)next->y + (int)ctx->mf->pos[2] + (int)nextFrame->pos[2];

	// NOTE(aalhendi): Source-backs retail 0x8006b480-0x8006b49c. The vertical
	// byte pair and summed frame Z are doubled, producing the halfway frame.
	return (u32)RenderBucket_MipsSll(z, 1);
}

static struct RenderBucketPackedVertex RenderBucket_CachePackedVertex(struct RenderBucketDrawContext *ctx, u16 stackIndex)
{
	u32 *scratchVertex = CTR_SCRATCHPAD_PTR(u32, 0x140 + (stackIndex * 8));
	struct RenderBucketPackedVertex packed;

	packed.xy = RenderBucket_PackModelVertexXY(ctx, &ctx->stack[stackIndex]);
	packed.z = RenderBucket_ModelVertexZ(ctx, &ctx->stack[stackIndex]);
	ctx->packedStack[stackIndex] = packed;
	scratchVertex[0] = packed.xy;
	scratchVertex[1] = packed.z;
	return packed;
}

static struct RenderBucketPackedVertex RenderBucket_CacheInterpolatedPackedVertex(struct RenderBucketDrawContext *ctx, u16 stackIndex,
                                                                                  const RenderBucketVertex *curr, const RenderBucketVertex *next)
{
	u32 *scratchVertex = CTR_SCRATCHPAD_PTR(u32, 0x140 + (stackIndex * 8));
	struct RenderBucketPackedVertex packed;

	packed.xy = RenderBucket_PackInterpolatedModelVertexXY(ctx, curr, next);
	packed.z = RenderBucket_InterpolatedModelVertexZ(ctx, curr, next);
	ctx->packedStack[stackIndex] = packed;
	scratchVertex[0] = packed.xy;
	scratchVertex[1] = packed.z;
	return packed;
}

static void RenderBucket_CopyScratchColorCache(struct RenderBucketDrawContext *ctx)
{
	u32 *scratchColor = CTR_SCRATCHPAD_PTR(u32, 0x140);
	u32 *commandList = (u32 *)ctx->idpp->ptrCommandList;
	u32 *colorLayout = (u32 *)ctx->idpp->ptrColorLayout;
	u32 count = commandList[0];

	// NOTE(aalhendi): Retail Execute copies ptrColorLayout to scratchpad 0x140
	// before DrawFunc_Normal so UncompressAnimationFrame can service command
	// bit 0x08000000 from the same cache.
	for (u32 i = 0; i < count; i++)
	{
		scratchColor[i] = colorLayout[i];
	}
}

static int RenderBucket_GetCommandColor(struct RenderBucketDrawContext *ctx, u32 command)
{
	u32 *colorLayout = (u32 *)ctx->idpp->ptrColorLayout;
	u32 colorOffset = (command >> 7) & 0x1fc;

	if ((s32)(command << 4) < 0)
		return *CTR_SCRATCHPAD_PTR(u32, 0x140 + colorOffset);

	return *(u32 *)((char *)colorLayout + colorOffset);
}

static int RenderBucket_GetIndexedColor(struct RenderBucketDrawContext *ctx, u32 colorOffset, int useScratch)
{
	if (useScratch != 0)
		return *CTR_SCRATCHPAD_PTR(u32, 0x140 + colorOffset);

	return *(u32 *)((char *)ctx->idpp->ptrColorLayout + colorOffset);
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

static int RenderBucket_ReadDeltaComponentFromStream(const u32 *vertData, int *bitIndex, u8 bits, int temporalBase, int *accum)
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
	return RenderBucket_ReadDeltaComponentFromStream((u32 *)ctx->vertData, &ctx->bitIndex, bits, temporalBase, accum);
}

static void RenderBucket_ReadNextFrameDeltaComponent(struct RenderBucketDrawContext *ctx, u8 bits, int temporalBase, int *currAccum, int *nextAccum)
{
	RenderBucket_ReadDeltaComponentFromStream((u32 *)ctx->vertData, &ctx->bitIndex, bits, temporalBase, currAccum);
	RenderBucket_ReadDeltaComponentFromStream((u32 *)ctx->nextVertData, &ctx->nextBitIndex, bits, temporalBase, nextAccum);
}

struct RenderBucketUncompressResult RenderBucket_UncompressAnimationFrame(struct RenderBucketDrawContext *ctx, u32 command, u16 stackIndex)
{
	struct RenderBucketUncompressResult result;
	u32 *deltaArray = (u32 *)ctx->idpp->ptrDeltaArray;
	u8 flags = (command >> 24) & 0xff;

	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006a8e0-0x8006aaa8.
	if ((flags & 4) != 0)
	{
		u32 *scratchVertex = CTR_SCRATCHPAD_PTR(u32, 0x140 + (stackIndex * 8));

		// Command bit 0x04000000 returns the cached packed vertex.
		result.packed.xy = scratchVertex[0];
		result.packed.z = scratchVertex[1];
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
		int bx = RenderBucket_SignExtendBits(temporal >> 25, 7) << 1;
		int bz = RenderBucket_SignExtendBits(temporal >> 17, 8);
		int by = RenderBucket_SignExtendBits(temporal >> 9, 8);

		// NOTE(aalhendi): Retail delta decode keeps the reset rule: an 8-bit
		// field skips the temporal base add.
		RenderBucket_ReadDeltaComponent(ctx, xBits, bx, &ctx->x_alu);
		RenderBucket_ReadDeltaComponent(ctx, zBits, bz, &ctx->y_alu);
		RenderBucket_ReadDeltaComponent(ctx, yBits, by, &ctx->z_alu);

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

static struct RenderBucketUncompressResult RenderBucket_UncompressAnimationFrame_NextFrame(struct RenderBucketDrawContext *ctx, u32 command, u16 stackIndex)
{
	struct RenderBucketUncompressResult result;
	u32 *deltaArray = (u32 *)ctx->idpp->ptrDeltaArray;
	u8 flags = (command >> 24) & 0xff;
	RenderBucketVertex nextVertex;

	if ((ctx->idpp->ptrNextFrame == 0) || (ctx->nextVertData == 0))
		return RenderBucket_UncompressAnimationFrame(ctx, command, stackIndex);

	if ((flags & 4) != 0)
	{
		u32 *scratchVertex = CTR_SCRATCHPAD_PTR(u32, 0x140 + (stackIndex * 8));

		// NOTE(aalhendi): Retail next-frame entry shares the cached-vertex
		// command bit path with 0x8006a8e0 before returning color.
		result.packed.xy = scratchVertex[0];
		result.packed.z = scratchVertex[1];
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
		int bx = RenderBucket_SignExtendBits(temporal >> 25, 7) << 1;
		int bz = RenderBucket_SignExtendBits(temporal >> 17, 8);
		int by = RenderBucket_SignExtendBits(temporal >> 9, 8);

		// NOTE(aalhendi): ASM-verified retail 0x8006b294-0x8006b460. It consumes
		// current and next compressed streams with the same temporal word.
		RenderBucket_ReadNextFrameDeltaComponent(ctx, xBits, bx, &ctx->x_alu, &ctx->x_next_alu);
		RenderBucket_ReadNextFrameDeltaComponent(ctx, zBits, bz, &ctx->y_alu, &ctx->y_next_alu);
		RenderBucket_ReadNextFrameDeltaComponent(ctx, yBits, by, &ctx->z_alu, &ctx->z_next_alu);

		ctx->stack[stackIndex].x = ctx->x_alu;
		ctx->stack[stackIndex].y = ctx->z_alu;
		ctx->stack[stackIndex].z = ctx->y_alu;
		nextVertex.x = ctx->x_next_alu;
		nextVertex.y = ctx->z_next_alu;
		nextVertex.z = ctx->y_next_alu;
	}
	else
	{
		RenderBucketCompVertex *currVerts = (RenderBucketCompVertex *)ctx->vertData;
		RenderBucketCompVertex *nextVerts = (RenderBucketCompVertex *)ctx->nextVertData;

		// NOTE(aalhendi): ASM-verified retail 0x8006b26c-0x8006b290. Both frame
		// vertex streams advance together before the interpolated pack.
		ctx->stack[stackIndex].x = currVerts[ctx->vertexIndex].x;
		ctx->stack[stackIndex].y = currVerts[ctx->vertexIndex].y;
		ctx->stack[stackIndex].z = currVerts[ctx->vertexIndex].z;
		nextVertex.x = nextVerts[ctx->vertexIndex].x;
		nextVertex.y = nextVerts[ctx->vertexIndex].y;
		nextVertex.z = nextVerts[ctx->vertexIndex].z;
	}

	result.packed = RenderBucket_CacheInterpolatedPackedVertex(ctx, stackIndex, &ctx->stack[stackIndex], &nextVertex);
	result.color = RenderBucket_GetCommandColor(ctx, command);
	return result;
}

static struct RenderBucketUncompressResult RenderBucket_TransformSplitDecodedVertex(struct RenderBucketDrawContext *ctx, u32 command, u16 stackIndex,
                                                                                    struct RenderBucketUncompressResult result)
{
	u8 flags = (command >> 24) & 0xff;
	u32 *scratchVertex = CTR_SCRATCHPAD_PTR(u32, 0x140 + (stackIndex * 8));

	if ((flags & 4) != 0)
		return result;

	// NOTE(aalhendi): ASM-verified retail 0x8006bf30/0x8006cdec split
	// transform tail: packed VXY/VZ through the split light matrix with MVMVA.
	MTC2(result.packed.xy, 0);
	MTC2(result.packed.z, 1);
	doCOP2(0x04a6012);
	result.packed.xy = ((u32)MFC2(10) << 16) | ((u32)MFC2(9) & 0xffff);
	result.packed.z = MFC2(11);
	ctx->packedStack[stackIndex] = result.packed;
	scratchVertex[0] = result.packed.xy;
	scratchVertex[1] = result.packed.z;
	return result;
}

static struct RenderBucketUncompressResult RenderBucket_UncompressAnimationFrame_Split(struct RenderBucketDrawContext *ctx, u32 command, u16 stackIndex)
{
	return RenderBucket_TransformSplitDecodedVertex(ctx, command, stackIndex, RenderBucket_UncompressAnimationFrame(ctx, command, stackIndex));
}

static struct RenderBucketUncompressResult RenderBucket_UncompressAnimationFrame_ReflectNextFrame(struct RenderBucketDrawContext *ctx, u32 command,
                                                                                                  u16 stackIndex)
{
	// NOTE(aalhendi): ASM-verified next-frame counterpart to 0x8006bf30:
	// decode through 0x8006b24c semantics, then apply the split transform.
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

static u_long *RenderBucket_GetNormalOTEntry(int activeRange, int depthMac0)
{
	int depthBin = (int)((u32)depthMac0 >> 17);

	if (activeRange == 0)
		return 0;

	// NOTE(aalhendi): Source-backs DrawInstPrim_Normal's active-range +
	// (MAC0 >> 17) OT lookup at 0x8006ad88-0x8006ad98. Retail trusts QueueDraw's
	// range producer here; native intentionally does not clamp to depthOffset
	// because that would mask producer/consumer depth mismatches.
	return (u_long *)activeRange + depthBin;
}

static u_long *RenderBucket_GetClampedOTEntry(struct RenderBucketDrawContext *ctx, int activeRange, int depthMac0)
{
	int depthBin = (int)((u32)depthMac0 >> 17);

	if (activeRange == 0)
		return 0;

	if (depthBin < ctx->idpp->depthOffset[0])
		depthBin = ctx->idpp->depthOffset[0];
	else if (depthBin > ctx->idpp->depthOffset[1])
		depthBin = ctx->idpp->depthOffset[1];

	return (u_long *)activeRange + depthBin;
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
		return 0;

	if ((s32)(reject << 16) < 0)
		return 0;

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

static int RenderBucket_CheckProjectedPrim(struct RenderBucketDrawContext *ctx, u32 command, u32 gteFlag, u16 cullXorMask, int *depthMac0Out)
{
	int depthMac0;

	// NOTE(aalhendi): Source-backs the shared retail projection gate:
	// flag reject, optional NCLIP, AVSZ3, screen-window reject, then MAC0.
	if ((s32)(gteFlag << 13) < 0)
		return 0;

	if ((s32)(command << 3) < 0)
	{
		int opZ;
		int cullXor;
		s16 cullFlags = (s16)((u16)ctx->idpp->instFlags ^ cullXorMask);

		gte_nclip();
		gte_stopz(&opZ);
		if (opZ == 0)
			return 0;

		cullXor = (s32)cullFlags ^ (s32)(command << 2);
		if ((s32)((u32)opZ ^ (u32)cullXor) <= 0)
			return 0;
	}

	gte_avsz3();

	if (RenderBucket_TriangleInScreenWindow(ctx) == 0)
		return 0;

	gte_stopz(&depthMac0);
	*depthMac0Out = depthMac0;
	return 1;
}

static int RenderBucket_ProjectPrim_Normal(struct RenderBucketDrawContext *ctx, u32 command, int useRtps, int reuseFirstVertex, int *depthMac0Out)
{
	if (useRtps != 0)
		RenderBucket_LoadPrimRTPS(ctx, reuseFirstVertex);
	else
		RenderBucket_LoadPrimRTPT(ctx);

	return RenderBucket_CheckProjectedPrim(ctx, command, CFC2(31), 0, depthMac0Out);
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

static int RenderBucket_ProjectPrim_Split(struct RenderBucketDrawContext *ctx, u32 command, int useRtps, int reuseFirstVertex, int *depthMac0Out)
{
	int shouldDraw = RenderBucket_ProjectPrim_Normal(ctx, command, useRtps, reuseFirstVertex, depthMac0Out);

	if (shouldDraw != 0)
		RenderBucket_StoreSplitProjectedRegs(ctx);

	return shouldDraw;
}

static struct TextureLayout *RenderBucket_GetCommandTexture(struct RenderBucketDrawContext *ctx, u32 command, int *isValid)
{
	u16 texIndex = command & 0x1ff;

	*isValid = 1;

	if (texIndex == 0)
		return 0;

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

static int RenderBucket_OTEntryPassesDpctGate(const u_long *otEntry)
{
	u32 addr = RenderBucket_OTAddress((void *)otEntry);

	// NOTE(aalhendi): Retail tests the post-depth-bin OT entry register with
	// `sll t0,t3,7` before DPCT. Native models that with the 24-bit OT address
	// domain used by primitive tags, not host pointer high bits.
	return (s32)(addr << 7) >= 0;
}

static void RenderBucket_LoadPrimColors(struct RenderBucketDrawContext *ctx, const u_long *otEntry)
{
	s16 alpha = ctx->idpp->alphaScale;

	MTC2(ctx->tempColor[1], 20);
	MTC2(ctx->tempColor[2], 21);
	MTC2(ctx->tempColor[3], 22);
	MTC2(alpha, 8);

	if ((alpha != 0) && (RenderBucket_OTEntryPassesDpctGate(otEntry) != 0))
		gte_dpct();
}

static u8 RenderBucket_SaturateU8(int value)
{
	if (value > 0xff)
		return 0xff;

	if (value < 0)
		return 0;

	return (u8)value;
}

static int RenderBucket_DrawInstPrim_NormalAtOTEntry(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, u_long *otEntry)
{
	u16 texIndex = command & 0x1ff;

	if ((char *)ctx->primMem->curr + sizeof(POLY_GT3) >= (char *)ctx->primMem->endMin100)
		return -1;

	if (otEntry == 0)
		return 0;

	RenderBucket_LoadPrimColors(ctx, otEntry);

	if (tex == 0)
	{
		POLY_G3 *p = ctx->primMem->curr;

		*(u32 *)&p->r0 = 0x30000000 | (u32)MFC2(20);
		*(int *)&p->r1 = MFC2(21);
		*(int *)&p->r2 = MFC2(22);
		gte_stsxy3(&p->x0, &p->x1, &p->x2);
#ifdef CTR_INTERNAL
		if (CtrTireDebug_ShouldLog(CTR_TIREDBG_RENDERBUCKET_PRIM) != 0)
		{
			fprintf(stderr,
			        "[TIREDBG][rb-prim] kind=G3 frame=%d level=%d inst=%p flags=%08x cmd=%08x code=%02x rgb0=%02x,%02x,%02x "
			        "xy=(%d,%d)(%d,%d)(%d,%d) ot=%p\n",
			        sdata->gGT != 0 ? sdata->gGT->framesInThisLEV : -1, sdata->gGT != 0 ? sdata->gGT->levelID : -1, (void *)ctx->inst, ctx->idpp->instFlags,
			        command, p->code, p->r0, p->g0, p->b0, p->x0, p->y0, p->x1, p->y1, p->x2, p->y2, (void *)otEntry);
		}
#endif
		RenderBucket_LinkPrimRaw(otEntry, p, 0x06000000);
		ctx->primMem->curr = (char *)p + 0x1c;
	}
	else
	{
		POLY_GT3 *p;
		u32 texWord1;
		u32 codeWord;

		p = ctx->primMem->curr;
		texWord1 = *(u32 *)&tex->u1;
		codeWord = ((texWord1 & 0x00600000) == 0x00600000) ? 0x34000000 : 0x36000000;

		*(u32 *)&p->r0 = codeWord | (u32)MFC2(20);
		*(int *)&p->r1 = MFC2(21);
		*(int *)&p->r2 = MFC2(22);
		*(int *)&p->u0 = *(int *)&tex->u0;
		*(u32 *)&p->u1 = texWord1;
		*(u32 *)&p->u2 = *(u32 *)&tex->u2;
		gte_stsxy3(&p->x0, &p->x1, &p->x2);
#ifdef CTR_INTERNAL
		if (CtrTireDebug_ShouldLog(CTR_TIREDBG_RENDERBUCKET_PRIM) != 0)
		{
			fprintf(stderr,
			        "[TIREDBG][rb-prim] kind=GT3 frame=%d level=%d inst=%p flags=%08x cmd=%08x code=%02x rgb0=%02x,%02x,%02x rgb1=%02x,%02x,%02x "
			        "rgb2=%02x,%02x,%02x xy=(%d,%d)(%d,%d)(%d,%d) tpage=%04x blend=%d clut=%04x tex=%u ot=%p\n",
			        sdata->gGT != 0 ? sdata->gGT->framesInThisLEV : -1, sdata->gGT != 0 ? sdata->gGT->levelID : -1, (void *)ctx->inst, ctx->idpp->instFlags,
			        command, p->code, p->r0, p->g0, p->b0, p->r1, p->g1, p->b1, p->r2, p->g2, p->b2, p->x0, p->y0, p->x1, p->y1, p->x2, p->y2, p->tpage,
			        (p->tpage >> 5) & 3, p->clut, texIndex, (void *)otEntry);
		}
#endif
		RenderBucket_LinkPrimRaw(otEntry, p, 0x09000000);
		ctx->primMem->curr = (char *)p + 0x28;
	}

	return 0;
}

static int RenderBucket_DrawInstPrim_NormalAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange, int depthMac0)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006ad88-0x8006ae74 body; native passes
	// the retail scratch/register inputs as explicit context and depth state.
	return RenderBucket_DrawInstPrim_NormalAtOTEntry(ctx, command, tex, RenderBucket_GetNormalOTEntry(activeRange, depthMac0));
}

static int RenderBucket_DrawInstPrim_KeyRelicTokenAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                          int depthMac0)
{
	u_long *otEntry;
	int signedTest;
	u32 litColor;
	int add;
	u32 r;
	u32 g;
	u32 b;
	u32 texWord1;
	u32 codeWord;
	u32 tpageMask;

	if ((char *)ctx->primMem->curr + sizeof(POLY_FT3) >= (char *)ctx->primMem->endMin100)
		return -1;

	// NOTE(aalhendi): Retail only routes textured key/relic/token models here.
	// Native keeps the dispatch host-safe if malformed instance data reaches it.
	if (tex == 0)
		return 0;

	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
		return 0;

	// NOTE(aalhendi): ASM-verified against NTSC-U 926 0x8006ae90-0x8006b030.
	// Retail inputs live in scratch/GTE registers; native passes the same state
	// through RenderBucketDrawContext and the current projected GTE FIFO.
	gte_nclip();
	signedTest = MFC2_S(24) ^ ((s16)ctx->idpp->instFlags ^ (int)(command << 2));
	if (signedTest < 0)
		otEntry++;

	POLY_FT3 *p = ctx->primMem->curr;
	gte_stsxy3(&p->x0, &p->x1, &p->x2);

	u32 sourceColor = (u32)ctx->tempColor[1];
	MTC2((s32)(sourceColor << 24) >> 19, 9);
	MTC2((s32)((sourceColor >> 8) << 24) >> 19, 10);
	MTC2((s32)((sourceColor >> 16) << 24) >> 19, 11);
	gte_lcir();

	int brightness = MFC2_S(9) + 0x1000;
	int distance = MFC2_S(11) >> 3;
	brightness = (((brightness * 7) + 0x2000) >> 4);
	if (signedTest < 0)
		brightness >>= 1;

	MTC2(brightness, 8);
	gte_dpcs();

	if (distance < 0)
		distance = -distance;

	add = 0;
	distance -= 0x180;
	if (distance >= 0)
	{
		if (distance >= 0x80)
			distance = 0x7f;

		add = sRenderBucketKeyRelicBrightness8008a2c4[distance];
	}

	litColor = (u32)MFC2(22);
	if (signedTest < 0)
		add >>= 3;

	r = RenderBucket_SaturateU8((litColor & 0xff) + add);
	g = RenderBucket_SaturateU8(((litColor >> 8) & 0xff) + add);
	b = RenderBucket_SaturateU8(((litColor >> 16) & 0xff) + add);

	texWord1 = *(u32 *)&tex->u1;
	codeWord = 0x24000000;
	tpageMask = 0x00600000;
	if (((texWord1 & 0x00600000) == 0) && (signedTest >= 0))
	{
		codeWord = 0x26000000;
		tpageMask = 0x00200000;
	}

	*(u32 *)&p->r0 = codeWord | (b << 16) | (g << 8) | r;
	*(u32 *)&p->u0 = *(u32 *)&tex->u0;
	*(u32 *)&p->u1 = texWord1 | tpageMask;
	*(u32 *)&p->u2 = *(u32 *)&tex->u2;

	RenderBucket_LinkPrimRaw(otEntry, p, 0x07000000);
	ctx->primMem->curr = (char *)p + 0x20;
	return 0;
}

static int RenderBucket_DrawInstPrim_KeyRelicToken(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	return RenderBucket_DrawInstPrim_KeyRelicTokenAtRange(ctx, command, tex, ctx->idpp->unkE4, depthMac0);
}

int RenderBucket_DrawInstPrim_Normal(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	return RenderBucket_DrawInstPrim_NormalAtRange(ctx, command, tex, ctx->idpp->unkE4, depthMac0);
}

static int RenderBucket_DrawInstPrim_SelectRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006ad6c-0x8006ad88.
	int activeRange = ((s32)(command << 6) > 0) ? ctx->idpp->unkE4 : ctx->idpp->unkE8;

	return RenderBucket_DrawInstPrim_NormalAtRange(ctx, command, tex, activeRange, depthMac0);
}

static u32 RenderBucket_DepthFadeColor(u32 color, int sz)
{
	int fade;

	sz -= 0xc00;
	if (sz <= 0)
		return 0;

	fade = 0x800 - sz;
	if (fade < 0)
		return color;

	MTC2(color, 22);
	MTC2(color, 6);
	MTC2(fade << 1, 8);
	gte_dpcs();
	return (u32)MFC2(22);
}

static int RenderBucket_DrawInstPrim_DepthFadeAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                      int depthMac0)
{
	u_long *otEntry;
	u32 color0;
	u32 color1;
	u32 color2;
	POLY_GT3 *p;

	// NOTE(aalhendi): Retail only selects this primitive writer for textured
	// rows. Keep native host-safe if malformed instance data reaches it.
	if (tex == 0)
		return 0;

	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
		return 0;

	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006b968-0x8006bad0. It fades each
	// rolling vertex color by SZ1/SZ2/SZ3, skips fully black triangles, then
	// emits a textured GT3 with the normal 0x09 OT tag length.
	color0 = RenderBucket_DepthFadeColor((u32)ctx->tempColor[1], MFC2_S(17));
	color1 = RenderBucket_DepthFadeColor((u32)ctx->tempColor[2], MFC2_S(18));
	color2 = RenderBucket_DepthFadeColor((u32)ctx->tempColor[3], MFC2_S(19));

	if ((color0 | color1 | color2) == 0)
		return 0;

	if ((char *)ctx->primMem->curr + sizeof(POLY_GT3) >= (char *)ctx->primMem->endMin100)
		return -1;

	p = ctx->primMem->curr;
	*(u32 *)&p->r0 = 0x36000000 | (color0 & 0x00ffffff);
	*(u32 *)&p->r1 = color1;
	*(u32 *)&p->r2 = color2;
	gte_stsxy3(&p->x0, &p->x1, &p->x2);
	*(u32 *)&p->u0 = *(u32 *)&tex->u0;
	*(u32 *)&p->u1 = *(u32 *)&tex->u1;
	*(u32 *)&p->u2 = *(u32 *)&tex->u2;

	RenderBucket_LinkPrimRaw(otEntry, p, 0x09000000);
	ctx->primMem->curr = (char *)p + 0x28;
	return 0;
}

static int RenderBucket_DrawInstPrim_DepthFade(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	return RenderBucket_DrawInstPrim_DepthFadeAtRange(ctx, command, tex, ctx->idpp->unkE4, depthMac0);
}

static int RenderBucket_DrawInstPrim_ClampDepthAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                       int depthMac0)
{
	// NOTE(aalhendi): ASM-verified retail 0x8006bad0-0x8006bbc0. Native passes
	// the scratch active range and MAC0-derived depth as explicit arguments.
	return RenderBucket_DrawInstPrim_NormalAtOTEntry(ctx, command, tex, RenderBucket_GetClampedOTEntry(ctx, activeRange, depthMac0));
}

static int RenderBucket_DrawInstPrim_ClampDepth(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	return RenderBucket_DrawInstPrim_ClampDepthAtRange(ctx, command, tex, ctx->idpp->unkE4, depthMac0);
}

static int RenderBucket_DrawInstPrim_LitTextureAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                       int depthMac0)
{
	u_long *otEntry;
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

	if ((char *)ctx->primMem->curr + sizeof(POLY_FT3) >= (char *)ctx->primMem->endMin100)
		return -1;

	if (tex == 0)
		return 0;

	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
		return 0;

	// NOTE(aalhendi): Source-backs retail primitive writer 0x8006c778-0x8006c920.
	// It derives side from NCLIP, lights the first rolling RGB word, then emits
	// a textured FT3 with side-dependent code/tpage bits.
	gte_nclip();
	signedTest = MFC2_S(24) ^ ((s16)ctx->idpp->instFlags ^ (int)(command << 2));
	if (signedTest < 0)
		otEntry++;

	sourceColor = (u32)ctx->tempColor[1];
	MTC2((s32)(sourceColor << 24) >> 19, 9);
	MTC2((s32)((sourceColor >> 8) << 24) >> 19, 10);
	MTC2((s32)((sourceColor >> 16) << 24) >> 19, 11);
	gte_lcir();

	brightness = MFC2_S(9) + 0x1000;
	distance = MFC2_S(11) >> 3;
	brightness = (((brightness * 7) + 0x2000) >> 4);
	if (signedTest < 0)
		brightness >>= 1;

	MTC2(brightness, 8);
	gte_dpcs();

	if (distance < 0)
		distance = -distance;

	add = 0;
	distance -= 0x180;
	if (distance >= 0)
	{
		if (distance >= 0x80)
			distance = 0x7f;

		add = sRenderBucketKeyRelicBrightness8008a2c4[distance];
	}

	litColor = (u32)MFC2(22);
	if (signedTest < 0)
		add >>= 3;

	r = RenderBucket_SaturateU8((litColor & 0xff) + add);
	g = RenderBucket_SaturateU8(((litColor >> 8) & 0xff) + add);
	b = RenderBucket_SaturateU8(((litColor >> 16) & 0xff) + add);

	texWord1 = *(u32 *)&tex->u1;
	if ((texWord1 & 0x00600000U) != 0)
	{
		u32 otSide = RenderBucket_OTAddress(otEntry) << 3;

		// NOTE(aalhendi): Retail tests raw OT pointer bits here. Native uses the
		// 24-bit OT/tag address domain so host pointer high bits cannot affect
		// primitive visibility.
		if ((s32)(otSide & (u32)signedTest) < 0)
			return 0;

		codeWord = 0x24000000;
		tpageMask = 0x00600000;
	}
	else if (signedTest < 0)
	{
		codeWord = 0x24000000;
		tpageMask = 0x00400000;
	}
	else
	{
		codeWord = 0x26000000;
		tpageMask = 0x00200000;
	}

	p = ctx->primMem->curr;
	*(u32 *)&p->r0 = codeWord | (b << 16) | (g << 8) | r;
	gte_stsxy3(&p->x0, &p->x1, &p->x2);
	*(u32 *)&p->u0 = *(u32 *)&tex->u0;
	*(u32 *)&p->u1 = texWord1 | tpageMask;
	*(u32 *)&p->u2 = *(u32 *)&tex->u2;

	RenderBucket_LinkPrimRaw(otEntry, p, 0x07000000);
	ctx->primMem->curr = (char *)p + 0x20;
	return 0;
}

static int RenderBucket_DrawInstPrim_LitTexture(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	return RenderBucket_DrawInstPrim_LitTextureAtRange(ctx, command, tex, ctx->idpp->unkE4, depthMac0);
}

static int RenderBucket_DrawInstPrim_GhostAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange, int depthMac0)
{
	u_long *otEntry;
	int alpha = ctx->idpp->alphaScale;
	u32 *p;

	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
		return 0;

	// NOTE(aalhendi): ASM-verified against NTSC-U 926 0x8006d670-0x8006d79c.
	// Alpha-zero ghosts tail-call the normal primitive writer at retail
	// 0x8006adc8, which is represented by the shared native normal writer.
	MTC2(ctx->tempColor[1], 20);
	MTC2(ctx->tempColor[2], 21);
	MTC2(ctx->tempColor[3], 22);
	MTC2(alpha, 8);

	if (alpha == 0)
		return RenderBucket_DrawInstPrim_NormalAtRange(ctx, command, tex, activeRange, depthMac0);

	if ((char *)ctx->primMem->curr + 0x40 >= (char *)ctx->primMem->endMin100)
		return -1;

	gte_dpct();

	p = (u32 *)ctx->primMem->curr;
	p[1] = 0xe1000a40;
	p[2] = 0;
	p[3] = *CTR_SCRATCHPAD_PTR(u32, 0x124);
	p[4] = (u32)MFC2(12);
	p[5] = (u32)MFC2(13);
	p[6] = (u32)MFC2(14);

	if (tex == 0)
	{
		p[7] = 0xe1000a20;
		p[8] = 0;
		p[9] = 0x32000000 | (u32)MFC2(20);
		p[10] = (u32)MFC2(12);
		p[11] = (u32)MFC2(21);
		p[12] = (u32)MFC2(13);
		p[13] = (u32)MFC2(22);
		p[14] = (u32)MFC2(14);

		RenderBucket_LinkPrimRaw(otEntry, p, 0x0e000000);
		ctx->primMem->curr = (char *)p + 0x3c;
	}
	else
	{
		u32 texWord1 = (*(u32 *)&tex->u1 & ~0x00600000U) | 0x00200000U;

		p[7] = 0x36000000 | (u32)MFC2(20);
		p[8] = (u32)MFC2(12);
		p[9] = *(u32 *)&tex->u0;
		p[10] = (u32)MFC2(21);
		p[11] = (u32)MFC2(13);
		p[12] = texWord1;
		p[13] = (u32)MFC2(22);
		p[14] = (u32)MFC2(14);
		p[15] = *(u32 *)&tex->u2;

		RenderBucket_LinkPrimRaw(otEntry, p, 0x0f000000);
		ctx->primMem->curr = (char *)p + 0x40;
	}

	return 0;
}

static int RenderBucket_DrawInstPrim_Ghost(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	return RenderBucket_DrawInstPrim_GhostAtRange(ctx, command, tex, ctx->idpp->unkE4, depthMac0);
}

static int RenderBucket_DispatchDrawInstPrimAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange, int depthMac0)
{
	switch ((u32)(uintptr_t)ctx->inst->funcPtr[1])
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
#ifdef CTR_INTERNAL
		if (CtrTireDebug_ShouldLog(CTR_TIREDBG_RENDERBUCKET_UNHANDLED) != 0)
		{
			fprintf(stderr, "[TIREDBG][rb-unhandled-prim-writer] inst=%p func=%p handler=%08x cmd=%08x depth=%d\n", (void *)ctx->inst, ctx->inst->funcPtr[1],
			        (u32)ctx->idpp->unkEC, command, depthMac0);
		}
#endif
		return 0;
	}
}

static int RenderBucket_DispatchDrawInstPrim(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	return RenderBucket_DispatchDrawInstPrimAtRange(ctx, command, tex, ctx->idpp->unkE4, depthMac0);
}

static int RenderBucket_SelectPrimitiveActiveRange(struct RenderBucketDrawContext *ctx, u32 command)
{
	if ((u32)(uintptr_t)ctx->inst->funcPtr[1] == RB_RETAIL_INST_PRIM_SELECT_RANGE)
		return ((s32)(command << 6) > 0) ? ctx->idpp->unkE4 : ctx->idpp->unkE8;

	return ctx->idpp->unkE4;
}

static void RenderBucket_AssignSplitUvs(struct RenderBucketDrawContext *ctx, const struct TextureLayout *tex)
{
	if (tex == 0)
		return;

	ctx->tempSplit[1].uv = (u16)tex->u0 | ((u16)tex->v0 << 8);
	ctx->tempSplit[2].uv = (u16)tex->u1 | ((u16)tex->v1 << 8);
	ctx->tempSplit[3].uv = (u16)tex->u2 | ((u16)tex->v2 << 8);
}

static void RenderBucket_LoadSplitPrimColors(struct RenderBucketDrawContext *ctx, const u_long *otEntry, u32 color0, u32 color1, u32 color2)
{
	s16 alpha = ctx->idpp->alphaScale;

	MTC2(color0, 20);
	MTC2(color1, 21);
	MTC2(color2, 22);
	MTC2(alpha, 8);

	if ((alpha != 0) && (RenderBucket_OTEntryPassesDpctGate(otEntry) != 0))
		gte_dpct();
}

static int RenderBucket_SplitPrimitiveWriterSupported(struct RenderBucketDrawContext *ctx)
{
	u32 prim = (u32)(uintptr_t)ctx->inst->funcPtr[1];

	return (prim == RB_RETAIL_INST_PRIM_NORMAL) || (prim == RB_RETAIL_INST_PRIM_SELECT_RANGE) || (prim == RB_RETAIL_INST_PRIM_DEPTH_FADE) ||
	       (prim == RB_RETAIL_INST_PRIM_KEY_TOKEN) || (prim == RB_RETAIL_INST_PRIM_CLAMP_DEPTH) || (prim == RB_RETAIL_INST_PRIM_LIT_TEXTURE) ||
	       (prim == RB_RETAIL_INST_PRIM_GHOST);
}

static int RenderBucket_DrawSplitPrimitiveNormalAtOTEntry(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, u_long *otEntry,
                                                          const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                          const struct RenderBucketSplitVertex *v2)
{
	u16 texIndex = command & 0x1ff;

	if ((char *)ctx->primMem->curr + sizeof(POLY_GT3) >= (char *)ctx->primMem->endMin100)
		return -1;

	if (otEntry == 0)
		return 0;

	RenderBucket_LoadSplitPrimColors(ctx, otEntry, v0->color, v1->color, v2->color);

	if (tex == 0)
	{
		POLY_G3 *p = ctx->primMem->curr;

		*(u32 *)&p->r0 = 0x30000000 | (u32)MFC2(20);
		*(int *)&p->r1 = MFC2(21);
		*(int *)&p->r2 = MFC2(22);
		p->x0 = (s16)v0->sxy;
		p->y0 = (s16)(v0->sxy >> 16);
		p->x1 = (s16)v1->sxy;
		p->y1 = (s16)(v1->sxy >> 16);
		p->x2 = (s16)v2->sxy;
		p->y2 = (s16)(v2->sxy >> 16);
		RenderBucket_LinkPrimRaw(otEntry, p, 0x06000000);
		ctx->primMem->curr = (char *)p + 0x1c;
	}
	else
	{
		POLY_GT3 *p = ctx->primMem->curr;
		u32 texWord1 = *(u32 *)&tex->u1;
		u32 codeWord = ((texWord1 & 0x00600000) == 0x00600000) ? 0x34000000 : 0x36000000;

		*(u32 *)&p->r0 = codeWord | (u32)MFC2(20);
		*(int *)&p->r1 = MFC2(21);
		*(int *)&p->r2 = MFC2(22);
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
		p->u2 = (u8)v2->uv;
		p->v2 = (u8)(v2->uv >> 8);
#ifdef CTR_INTERNAL
		if (CtrTireDebug_ShouldLog(CTR_TIREDBG_RENDERBUCKET_PRIM) != 0)
		{
			fprintf(stderr,
			        "[TIREDBG][rb-split-prim] frame=%d level=%d inst=%p flags=%08x cmd=%08x tex=%u ot=%p sxy=(%08x,%08x,%08x) "
			        "dist=(%d,%d,%d)\n",
			        sdata->gGT != 0 ? sdata->gGT->framesInThisLEV : -1, sdata->gGT != 0 ? sdata->gGT->levelID : -1, (void *)ctx->inst, ctx->idpp->instFlags,
			        command, texIndex, (void *)otEntry, v0->sxy, v1->sxy, v2->sxy, v0->splitDist, v1->splitDist, v2->splitDist);
		}
#endif
		RenderBucket_LinkPrimRaw(otEntry, p, 0x09000000);
		ctx->primMem->curr = (char *)p + 0x28;
	}

	return 0;
}

static u32 RenderBucket_TextureWordWithSplitUv(u32 texWord, u16 uv)
{
	return (texWord & 0xffff0000U) | uv;
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
		brightness >>= 1;

	MTC2(brightness, 8);
	gte_dpcs();

	if (distance < 0)
		distance = -distance;

	add = 0;
	distance -= 0x180;
	if (distance >= 0)
	{
		if (distance >= 0x80)
			distance = 0x7f;

		add = sRenderBucketKeyRelicBrightness8008a2c4[distance];
	}

	litColor = (u32)MFC2(22);
	if (signedTest < 0)
		add >>= 3;

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
	*(u32 *)&p->u0 = texWord0;
	p->x1 = (s16)v1->sxy;
	p->y1 = (s16)(v1->sxy >> 16);
	*(u32 *)&p->u1 = texWord1;
	p->x2 = (s16)v2->sxy;
	p->y2 = (s16)(v2->sxy >> 16);
	*(u32 *)&p->u2 = texWord2;
}

static int RenderBucket_DrawSplitPrimitiveDepthFadeAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                           int depthMac0, const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                           const struct RenderBucketSplitVertex *v2)
{
	u_long *otEntry;
	u32 color0;
	u32 color1;
	u32 color2;
	POLY_GT3 *p;

	if (tex == 0)
		return 0;

	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
		return 0;

	color0 = RenderBucket_DepthFadeColor(v0->color, (int)v0->sz);
	color1 = RenderBucket_DepthFadeColor(v1->color, (int)v1->sz);
	color2 = RenderBucket_DepthFadeColor(v2->color, (int)v2->sz);
	if ((color0 | color1 | color2) == 0)
		return 0;

	if ((char *)ctx->primMem->curr + sizeof(POLY_GT3) >= (char *)ctx->primMem->endMin100)
		return -1;

	// NOTE(aalhendi): Source-backs generated-split use of retail 0x8006b968:
	// depth-fade still writes GT3, but generated vertices supply SXY/SZ and UV.
	p = ctx->primMem->curr;
	*(u32 *)&p->r0 = 0x36000000 | (color0 & 0x00ffffff);
	*(u32 *)&p->r1 = color1;
	*(u32 *)&p->r2 = color2;
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
	p->u2 = (u8)v2->uv;
	p->v2 = (u8)(v2->uv >> 8);
	RenderBucket_LinkPrimRaw(otEntry, p, 0x09000000);
	ctx->primMem->curr = (char *)p + 0x28;
	return 0;
}

static int RenderBucket_DrawSplitPrimitiveGhostAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                       int depthMac0, const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                       const struct RenderBucketSplitVertex *v2)
{
	u_long *otEntry;
	int alpha = ctx->idpp->alphaScale;
	u32 *p;

	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
		return 0;

	MTC2(v0->color, 20);
	MTC2(v1->color, 21);
	MTC2(v2->color, 22);
	MTC2(alpha, 8);

	if (alpha == 0)
		return RenderBucket_DrawSplitPrimitiveNormalAtOTEntry(ctx, command, tex, otEntry, v0, v1, v2);

	if ((char *)ctx->primMem->curr + 0x40 >= (char *)ctx->primMem->endMin100)
		return -1;

	gte_dpct();

	// NOTE(aalhendi): Source-backs generated-split use of retail 0x8006d670:
	// same ghost primitive packet, but generated vertices supply SXY and UV.
	p = (u32 *)ctx->primMem->curr;
	p[1] = 0xe1000a40;
	p[2] = 0;
	p[3] = *CTR_SCRATCHPAD_PTR(u32, 0x124);
	p[4] = v0->sxy;
	p[5] = v1->sxy;
	p[6] = v2->sxy;

	if (tex == 0)
	{
		p[7] = 0xe1000a20;
		p[8] = 0;
		p[9] = 0x32000000 | (u32)MFC2(20);
		p[10] = v0->sxy;
		p[11] = (u32)MFC2(21);
		p[12] = v1->sxy;
		p[13] = (u32)MFC2(22);
		p[14] = v2->sxy;

		RenderBucket_LinkPrimRaw(otEntry, p, 0x0e000000);
		ctx->primMem->curr = (char *)p + 0x3c;
	}
	else
	{
		u32 texWord0 = RenderBucket_TextureWordWithSplitUv(*(u32 *)&tex->u0, v0->uv);
		u32 texWord1 = RenderBucket_TextureWordWithSplitUv((*(u32 *)&tex->u1 & ~0x00600000U) | 0x00200000U, v1->uv);
		u32 texWord2 = RenderBucket_TextureWordWithSplitUv(*(u32 *)&tex->u2, v2->uv);

		p[7] = 0x36000000 | (u32)MFC2(20);
		p[8] = v0->sxy;
		p[9] = texWord0;
		p[10] = (u32)MFC2(21);
		p[11] = v1->sxy;
		p[12] = texWord1;
		p[13] = (u32)MFC2(22);
		p[14] = v2->sxy;
		p[15] = texWord2;

		RenderBucket_LinkPrimRaw(otEntry, p, 0x0f000000);
		ctx->primMem->curr = (char *)p + 0x40;
	}

	return 0;
}

static int RenderBucket_DrawSplitPrimitiveKeyRelicTokenAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                               int depthMac0, const struct RenderBucketSplitVertex *v0,
                                                               const struct RenderBucketSplitVertex *v1, const struct RenderBucketSplitVertex *v2)
{
	u_long *otEntry;
	int signedTest;
	u32 texWord0;
	u32 texWord1;
	u32 texWord2;
	u32 codeWord;
	u32 tpageMask;
	POLY_FT3 *p;

	if ((char *)ctx->primMem->curr + sizeof(POLY_FT3) >= (char *)ctx->primMem->endMin100)
		return -1;

	if (tex == 0)
		return 0;

	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
		return 0;

	RenderBucket_LoadSplitProjectedRegs(v0, v1, v2);
	gte_nclip();
	signedTest = MFC2_S(24) ^ ((s16)ctx->idpp->instFlags ^ (int)(command << 2));
	if (signedTest < 0)
		otEntry++;

	texWord0 = RenderBucket_TextureWordWithSplitUv(*(u32 *)&tex->u0, v0->uv);
	texWord1 = RenderBucket_TextureWordWithSplitUv(*(u32 *)&tex->u1, v1->uv);
	texWord2 = RenderBucket_TextureWordWithSplitUv(*(u32 *)&tex->u2, v2->uv);

	codeWord = 0x24000000;
	tpageMask = 0x00600000;
	if (((texWord1 & 0x00600000U) == 0) && (signedTest >= 0))
	{
		codeWord = 0x26000000;
		tpageMask = 0x00200000;
	}

	// NOTE(aalhendi): Source-backs generated-split use of retail 0x8006ae90:
	// same flat textured writer, with generated SXY/UV and generated color.
	p = ctx->primMem->curr;
	*(u32 *)&p->r0 = codeWord | RenderBucket_LightFlatTextureColor(v0->color, signedTest);
	RenderBucket_WriteSplitFT3(p, v0, v1, v2, texWord0, texWord1 | tpageMask, texWord2);

	RenderBucket_LinkPrimRaw(otEntry, p, 0x07000000);
	ctx->primMem->curr = (char *)p + 0x20;
	return 0;
}

static int RenderBucket_DrawSplitPrimitiveLitTextureAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange,
                                                            int depthMac0, const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                            const struct RenderBucketSplitVertex *v2)
{
	u_long *otEntry;
	int signedTest;
	u32 texWord0;
	u32 texWord1;
	u32 texWord2;
	u32 codeWord;
	u32 tpageMask;
	POLY_FT3 *p;

	if ((char *)ctx->primMem->curr + sizeof(POLY_FT3) >= (char *)ctx->primMem->endMin100)
		return -1;

	if (tex == 0)
		return 0;

	otEntry = RenderBucket_GetNormalOTEntry(activeRange, depthMac0);
	if (otEntry == 0)
		return 0;

	RenderBucket_LoadSplitProjectedRegs(v0, v1, v2);
	gte_nclip();
	signedTest = MFC2_S(24) ^ ((s16)ctx->idpp->instFlags ^ (int)(command << 2));
	if (signedTest < 0)
		otEntry++;

	texWord0 = RenderBucket_TextureWordWithSplitUv(*(u32 *)&tex->u0, v0->uv);
	texWord1 = RenderBucket_TextureWordWithSplitUv(*(u32 *)&tex->u1, v1->uv);
	texWord2 = RenderBucket_TextureWordWithSplitUv(*(u32 *)&tex->u2, v2->uv);

	if ((texWord1 & 0x00600000U) != 0)
	{
		u32 otSide = RenderBucket_OTAddress(otEntry) << 3;

		if ((s32)(otSide & (u32)signedTest) < 0)
			return 0;

		codeWord = 0x24000000;
		tpageMask = 0x00600000;
	}
	else if (signedTest < 0)
	{
		codeWord = 0x24000000;
		tpageMask = 0x00400000;
	}
	else
	{
		codeWord = 0x26000000;
		tpageMask = 0x00200000;
	}

	// NOTE(aalhendi): Source-backs generated-split use of retail 0x8006c778:
	// same side-dependent flat textured writer with generated SXY/UV/color.
	p = ctx->primMem->curr;
	*(u32 *)&p->r0 = codeWord | RenderBucket_LightFlatTextureColor(v0->color, signedTest);
	RenderBucket_WriteSplitFT3(p, v0, v1, v2, texWord0, texWord1 | tpageMask, texWord2);

	RenderBucket_LinkPrimRaw(otEntry, p, 0x07000000);
	ctx->primMem->curr = (char *)p + 0x20;
	return 0;
}

static int RenderBucket_DrawSplitPrimitiveAtRange(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange, int depthMac0,
                                                  const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                  const struct RenderBucketSplitVertex *v2)
{
	u32 prim = (u32)(uintptr_t)ctx->inst->funcPtr[1];

	// NOTE(aalhendi): Retail tail-calls Instance+0x60 from the generated split
	// helpers. Native only claims the labels whose generated-UV ABI is modeled.
	if (RenderBucket_SplitPrimitiveWriterSupported(ctx) == 0)
	{
#ifdef CTR_INTERNAL
		if (CtrTireDebug_ShouldLog(CTR_TIREDBG_RENDERBUCKET_UNHANDLED) != 0)
		{
			fprintf(stderr, "[TIREDBG][rb-split-unhandled-prim-writer] inst=%p func=%p handler=%08x cmd=%08x depth=%d\n", (void *)ctx->inst,
			        ctx->inst->funcPtr[1], (u32)ctx->idpp->unkEC, command, depthMac0);
		}
#endif
		return 0;
	}

	if (prim == RB_RETAIL_INST_PRIM_DEPTH_FADE)
		return RenderBucket_DrawSplitPrimitiveDepthFadeAtRange(ctx, command, tex, activeRange, depthMac0, v0, v1, v2);

	if (prim == RB_RETAIL_INST_PRIM_KEY_TOKEN)
		return RenderBucket_DrawSplitPrimitiveKeyRelicTokenAtRange(ctx, command, tex, activeRange, depthMac0, v0, v1, v2);

	if (prim == RB_RETAIL_INST_PRIM_CLAMP_DEPTH)
		return RenderBucket_DrawSplitPrimitiveNormalAtOTEntry(ctx, command, tex, RenderBucket_GetClampedOTEntry(ctx, activeRange, depthMac0), v0, v1, v2);

	if (prim == RB_RETAIL_INST_PRIM_LIT_TEXTURE)
		return RenderBucket_DrawSplitPrimitiveLitTextureAtRange(ctx, command, tex, activeRange, depthMac0, v0, v1, v2);

	if (prim == RB_RETAIL_INST_PRIM_GHOST)
		return RenderBucket_DrawSplitPrimitiveGhostAtRange(ctx, command, tex, activeRange, depthMac0, v0, v1, v2);

	return RenderBucket_DrawSplitPrimitiveNormalAtOTEntry(ctx, command, tex, RenderBucket_GetNormalOTEntry(activeRange, depthMac0), v0, v1, v2);
}

static void RenderBucket_ProjectSplitVertex(struct RenderBucketDrawContext *ctx, struct RenderBucketSplitVertex *v)
{
	// NOTE(aalhendi): Source-backs retail generated-vertex SXY/SZ stores.
	MTC2(v->xy, 0);
	MTC2(v->z, 1);
	gte_rtps();
	v->sxy = MFC2(12);
	v->sz = MFC2(17);
}

static void RenderBucket_BuildDepthSplitIntersection(struct RenderBucketDrawContext *ctx, struct RenderBucketSplitVertex *dst,
                                                     const struct RenderBucketSplitVertex *from, const struct RenderBucketSplitVertex *to, int hasTexture)
{
	dst->z = (u32)ctx->splitPlane;
	RenderBucket_SplitInterpolateVertex(dst, from, to, hasTexture);
	RenderBucket_ProjectSplitVertex(ctx, dst);
}

static int RenderBucket_SelectDepthSplitHelperRange(struct RenderBucketDrawContext *ctx, u32 command, int helperRange)
{
	if ((u32)(uintptr_t)ctx->inst->funcPtr[1] == RB_RETAIL_INST_PRIM_SELECT_RANGE)
		return RenderBucket_SelectPrimitiveActiveRange(ctx, command);

	return helperRange;
}

static int RenderBucket_DrawDepthSplitCandidate(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange, int depthMac0,
                                                const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                const struct RenderBucketSplitVertex *v2, int guardDist)
{
	if (guardDist >= 0)
		return 0;

	activeRange = RenderBucket_SelectDepthSplitHelperRange(ctx, command, activeRange);
	return RenderBucket_DrawSplitPrimitiveAtRange(ctx, command, tex, activeRange, depthMac0, v0, v1, v2);
}

static int RenderBucket_DrawSplitClipped(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	const struct RenderBucketSplitVertex *a;
	const struct RenderBucketSplitVertex *b;
	const struct RenderBucketSplitVertex *c;
	struct RenderBucketSplitVertex ab;
	struct RenderBucketSplitVertex ac;
	struct RenderBucketSplitVertex bc;
	struct RenderBucketSplitVertex cb;
	int hasTexture = tex != 0;
	int primaryRange;
	int secondaryRange;
	int signMask = 0;

	RenderBucket_AssignSplitUvs(ctx, tex);

	a = &ctx->tempSplit[1];
	b = &ctx->tempSplit[2];
	c = &ctx->tempSplit[3];

	if (a->splitDist < 0)
		signMask |= 1;
	if (b->splitDist < 0)
		signMask |= 2;
	if (c->splitDist < 0)
		signMask |= 4;

	// NOTE(aalhendi): ASM-verified retail 0x8006b4c8-0x8006b968 candidate
	// order and scratch[0x3c]/scratch[0x40] range switching. Native carries the
	// scratch vertices as explicit structs, then dispatches through the current
	// Instance+0x60 primitive writer.
	switch (signMask)
	{
	case 0:
		return 0;

	case 7:
		return RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, ctx->idpp->unkE4, depthMac0, a, b, c, a->splitDist);

	case 1:
		primaryRange = ctx->idpp->unkE8;
		secondaryRange = ctx->idpp->unkE4;
		RenderBucket_BuildDepthSplitIntersection(ctx, &ab, a, b, hasTexture);
		RenderBucket_BuildDepthSplitIntersection(ctx, &ac, a, c, hasTexture);
		if (RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, primaryRange, depthMac0, c, &ac, &ab, c->splitDist) < 0)
			return -1;
		if (RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, primaryRange, depthMac0, b, c, &ab, b->splitDist) < 0)
			return -1;
		return RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, &ac, a, &ab, a->splitDist);

	case 2:
		primaryRange = ctx->idpp->unkE8;
		secondaryRange = ctx->idpp->unkE4;
		RenderBucket_BuildDepthSplitIntersection(ctx, &ab, a, b, hasTexture);
		RenderBucket_BuildDepthSplitIntersection(ctx, &cb, c, b, hasTexture);
		if (RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, primaryRange, depthMac0, c, a, &ab, a->splitDist) < 0)
			return -1;
		if (RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, primaryRange, depthMac0, &cb, c, &ab, c->splitDist) < 0)
			return -1;
		return RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, b, &cb, &ab, b->splitDist);

	case 4:
		primaryRange = ctx->idpp->unkE8;
		secondaryRange = ctx->idpp->unkE4;
		RenderBucket_BuildDepthSplitIntersection(ctx, &ac, a, c, hasTexture);
		RenderBucket_BuildDepthSplitIntersection(ctx, &bc, b, c, hasTexture);
		if (RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, primaryRange, depthMac0, b, &bc, &ac, b->splitDist) < 0)
			return -1;
		if (RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, primaryRange, depthMac0, a, b, &ac, a->splitDist) < 0)
			return -1;
		return RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, &bc, c, &ac, c->splitDist);

	case 3:
		primaryRange = ctx->idpp->unkE4;
		secondaryRange = ctx->idpp->unkE8;
		RenderBucket_BuildDepthSplitIntersection(ctx, &ac, a, c, hasTexture);
		RenderBucket_BuildDepthSplitIntersection(ctx, &bc, b, c, hasTexture);
		if (RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, primaryRange, depthMac0, b, &bc, &ac, b->splitDist) < 0)
			return -1;
		if (RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, primaryRange, depthMac0, a, b, &ac, a->splitDist) < 0)
			return -1;
		return RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, &bc, c, &ac, c->splitDist);

	case 5:
		primaryRange = ctx->idpp->unkE4;
		secondaryRange = ctx->idpp->unkE8;
		RenderBucket_BuildDepthSplitIntersection(ctx, &ab, a, b, hasTexture);
		RenderBucket_BuildDepthSplitIntersection(ctx, &cb, c, b, hasTexture);
		if (RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, primaryRange, depthMac0, c, a, &ab, a->splitDist) < 0)
			return -1;
		if (RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, primaryRange, depthMac0, &cb, c, &ab, c->splitDist) < 0)
			return -1;
		return RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, b, &cb, &ab, b->splitDist);

	case 6:
		primaryRange = ctx->idpp->unkE4;
		secondaryRange = ctx->idpp->unkE8;
		RenderBucket_BuildDepthSplitIntersection(ctx, &ab, a, b, hasTexture);
		RenderBucket_BuildDepthSplitIntersection(ctx, &ac, a, c, hasTexture);
		if (RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, primaryRange, depthMac0, c, &ac, &ab, c->splitDist) < 0)
			return -1;
		if (RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, primaryRange, depthMac0, b, c, &ab, b->splitDist) < 0)
			return -1;
		return RenderBucket_DrawDepthSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, &ac, a, &ab, a->splitDist);
	}

	return 0;
}

static void RenderBucket_BuildWaterSplitIntersection(struct RenderBucketDrawContext *ctx, struct RenderBucketSplitVertex *dst,
                                                     const struct RenderBucketSplitVertex *from, const struct RenderBucketSplitVertex *to, int hasTexture)
{
	RenderBucket_WaterSplitInterpolateVertex(ctx, dst, from, to, hasTexture, ctx->idpp->splitLine);
	RenderBucket_ProjectSplitVertex(ctx, dst);
}

static int RenderBucket_SelectWaterSplitHelperRange(struct RenderBucketDrawContext *ctx, u32 command, int helperRange)
{
	if ((u32)(uintptr_t)ctx->inst->funcPtr[1] == RB_RETAIL_INST_PRIM_SELECT_RANGE)
		return RenderBucket_SelectPrimitiveActiveRange(ctx, command);

	return helperRange;
}

static u32 RenderBucket_WaterSplitShiftMaskColor(struct RenderBucketDrawContext *ctx, u32 color)
{
	return (color >> (((s8)ctx->inst->unk53) & 31)) & ctx->inst->reflectionRGBA;
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
	switch ((u32)(uintptr_t)ctx->inst->funcPtr[2])
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
			return 0;

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
		return (guardDist ^ (s8)ctx->inst->unk53) >= 0;

	case RB_RETAIL_INST_FUNC2_SPLIT_DIM_XOR:
		selector = guardDist ^ (s8)ctx->inst->unk53;
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

static int RenderBucket_DrawWaterSplitCandidate(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int activeRange, int depthMac0,
                                                const struct RenderBucketSplitVertex *v0, const struct RenderBucketSplitVertex *v1,
                                                const struct RenderBucketSplitVertex *v2, int guardDist)
{
	struct RenderBucketSplitVertex out0 = *v0;
	struct RenderBucketSplitVertex out1 = *v1;
	struct RenderBucketSplitVertex out2 = *v2;

	if (RenderBucket_ApplyWaterSplitSideSelector(ctx, guardDist, &out0, &out1, &out2) == 0)
		return 0;

	activeRange = RenderBucket_SelectWaterSplitHelperRange(ctx, command, activeRange);
	return RenderBucket_DrawSplitPrimitiveAtRange(ctx, command, tex, activeRange, depthMac0, &out0, &out1, &out2);
}

static int RenderBucket_DrawWaterSplitClipped(struct RenderBucketDrawContext *ctx, u32 command, struct TextureLayout *tex, int depthMac0)
{
	const struct RenderBucketSplitVertex *a;
	const struct RenderBucketSplitVertex *b;
	const struct RenderBucketSplitVertex *c;
	struct RenderBucketSplitVertex ab;
	struct RenderBucketSplitVertex ac;
	struct RenderBucketSplitVertex bc;
	struct RenderBucketSplitVertex cb;
	int hasTexture = tex != 0;
	int primaryRange;
	int secondaryRange;
	int signMask = 0;

	RenderBucket_AssignSplitUvs(ctx, tex);

	a = &ctx->tempSplit[1];
	b = &ctx->tempSplit[2];
	c = &ctx->tempSplit[3];

	if (a->splitDist < 0)
		signMask |= 1;
	if (b->splitDist < 0)
		signMask |= 2;
	if (c->splitDist < 0)
		signMask |= 4;

	// NOTE(aalhendi): Source-backs retail helper 0x8006d094's candidate
	// topology. Retail builds fixed split-plane intersections, emits up to
	// three candidate triangles, and lets Instance+0x64 decide draw/color side.
	// This preserves those branch-specific edge directions instead of generic
	// polygon clipping; final scratch/register choreography remains pending.
	switch (signMask)
	{
	case 0:
		return RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, ctx->idpp->unkE8, depthMac0, a, b, c, a->splitDist);

	case 7:
		return RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, ctx->idpp->unkE4, depthMac0, a, b, c, a->splitDist);

	case 1:
		primaryRange = ctx->idpp->unkE8;
		secondaryRange = ctx->idpp->unkE4;
		RenderBucket_BuildWaterSplitIntersection(ctx, &ab, a, b, hasTexture);
		RenderBucket_BuildWaterSplitIntersection(ctx, &ac, a, c, hasTexture);
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, c, &ac, &ab, c->splitDist) < 0)
			return -1;
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, b, c, &ab, b->splitDist) < 0)
			return -1;
		return RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, &ac, a, &ab, a->splitDist);

	case 6:
		primaryRange = ctx->idpp->unkE4;
		secondaryRange = ctx->idpp->unkE8;
		RenderBucket_BuildWaterSplitIntersection(ctx, &ab, a, b, hasTexture);
		RenderBucket_BuildWaterSplitIntersection(ctx, &ac, a, c, hasTexture);
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, c, &ac, &ab, c->splitDist) < 0)
			return -1;
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, b, c, &ab, b->splitDist) < 0)
			return -1;
		return RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, &ac, a, &ab, a->splitDist);

	case 2:
		primaryRange = ctx->idpp->unkE8;
		secondaryRange = ctx->idpp->unkE4;
		RenderBucket_BuildWaterSplitIntersection(ctx, &ab, a, b, hasTexture);
		RenderBucket_BuildWaterSplitIntersection(ctx, &cb, c, b, hasTexture);
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, c, a, &ab, a->splitDist) < 0)
			return -1;
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, &cb, c, &ab, c->splitDist) < 0)
			return -1;
		return RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, b, &cb, &ab, b->splitDist);

	case 5:
		primaryRange = ctx->idpp->unkE4;
		secondaryRange = ctx->idpp->unkE8;
		RenderBucket_BuildWaterSplitIntersection(ctx, &ab, a, b, hasTexture);
		RenderBucket_BuildWaterSplitIntersection(ctx, &cb, c, b, hasTexture);
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, c, a, &ab, a->splitDist) < 0)
			return -1;
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, &cb, c, &ab, c->splitDist) < 0)
			return -1;
		return RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, b, &cb, &ab, b->splitDist);

	case 3:
		primaryRange = ctx->idpp->unkE4;
		secondaryRange = ctx->idpp->unkE8;
		RenderBucket_BuildWaterSplitIntersection(ctx, &ac, a, c, hasTexture);
		RenderBucket_BuildWaterSplitIntersection(ctx, &bc, b, c, hasTexture);
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, b, &bc, &ac, b->splitDist) < 0)
			return -1;
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, a, b, &ac, a->splitDist) < 0)
			return -1;
		return RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, &bc, c, &ac, c->splitDist);

	case 4:
		primaryRange = ctx->idpp->unkE8;
		secondaryRange = ctx->idpp->unkE4;
		RenderBucket_BuildWaterSplitIntersection(ctx, &ac, a, c, hasTexture);
		RenderBucket_BuildWaterSplitIntersection(ctx, &bc, b, c, hasTexture);
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, b, &bc, &ac, b->splitDist) < 0)
			return -1;
		if (RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, primaryRange, depthMac0, a, b, &ac, a->splitDist) < 0)
			return -1;
		return RenderBucket_DrawWaterSplitCandidate(ctx, command, tex, secondaryRange, depthMac0, &bc, c, &ac, c->splitDist);
	}

	return 0;
}

void RenderBucket_DrawFunc_Normal(struct RenderBucketDrawContext *ctx)
{
	u32 *pCmd;

	// NOTE(aalhendi): Native C command-list traversal for the normal
	// RenderBucket draw function. It has the named retail boundary, but the
	// exact branch/register choreography remains a pending ASM audit.
	pCmd = (u32 *)ctx->idpp->ptrCommandList;
	RenderBucket_CopyScratchColorCache(ctx);
	pCmd++;

	while (*pCmd != 0xffffffff)
	{
		u32 command = *pCmd++;
		u16 flags = (command >> 24) & 0xff;
		u16 stackIndex = (command >> 16) & 0xff;
		int startsNewStrip;
		int useRtps;
		int reuseFirstVertex;
		u32 drawCommand;
		int color;
		struct RenderBucketUncompressResult decoded;

		if ((command >> 16) == 0)
		{
			RenderBucket_ApplyColorOnlyCommand(ctx, command);
			continue;
		}

		decoded = RenderBucket_DispatchUncompressAnimationFrame(ctx, command, stackIndex);
		color = decoded.color;
		if ((flags & 4) == 0)
			ctx->vertexIndex++;

		ctx->tempCoords[0] = ctx->tempCoords[1];
		ctx->tempCoords[1] = ctx->tempCoords[2];
		ctx->tempCoords[2] = ctx->tempCoords[3];
		ctx->tempCoords[3] = ctx->stack[stackIndex];
		ctx->tempPacked[0] = ctx->tempPacked[1];
		ctx->tempPacked[1] = ctx->tempPacked[2];
		ctx->tempPacked[2] = ctx->tempPacked[3];
		ctx->tempPacked[3] = decoded.packed;

		ctx->tempColor[0] = ctx->tempColor[1];
		ctx->tempColor[1] = ctx->tempColor[2];
		ctx->tempColor[2] = ctx->tempColor[3];
		ctx->tempColor[3] = color;

		startsNewStrip = (flags & 0x80) != 0;
		if ((startsNewStrip != 0) || (ctx->stripLength == 0))
			ctx->primCommand = command;

		if (startsNewStrip != 0)
			ctx->stripLength = 0;

		useRtps = ctx->stripLength > 2;
		reuseFirstVertex = (useRtps != 0) && ((flags & 0x40) != 0);

		if (reuseFirstVertex != 0)
		{
			ctx->tempCoords[1] = ctx->tempCoords[0];
			ctx->tempPacked[1] = ctx->tempPacked[0];
			ctx->tempColor[1] = ctx->tempColor[0];
		}

		if (ctx->stripLength < 2)
		{
			ctx->stripLength++;
			continue;
		}

		// NOTE(aalhendi): Retail stores the restart command at scratchpad
		// `0x10c` and restores it after the initial RTPT, so the first
		// primitive in a strip is keyed by the first command, not the third.
		drawCommand = (ctx->stripLength == 2) ? ctx->primCommand : command;

		{
			struct TextureLayout *tex;
			int depthMac0;
			int isValidTexture;
			int shouldDraw;

			shouldDraw = RenderBucket_ProjectPrim_Normal(ctx, drawCommand, useRtps, reuseFirstVertex, &depthMac0);
			if (shouldDraw == 0)
			{
				ctx->stripLength++;
				continue;
			}

			tex = RenderBucket_GetCommandTexture(ctx, drawCommand, &isValidTexture);
			if (isValidTexture == 0)
			{
				ctx->stripLength++;
				continue;
			}

			if (RenderBucket_DispatchDrawInstPrim(ctx, drawCommand, tex, depthMac0) < 0)
				return;
		}

		ctx->stripLength++;
	}
}

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
	int shift = ((s8)ctx->inst->unk53) & 31;
	int ret;

	// NOTE(aalhendi): Source-backs the 0x8006bbc0 mirrored-side color path:
	// retail shifts each rolling RGB word by Instance+0x53 and masks by
	// Instance+0x58 before calling the primitive writer on the secondary range.
	ctx->tempColor[1] = (int)(((u32)savedColor1 >> shift) & mask);
	ctx->tempColor[2] = (int)(((u32)savedColor2 >> shift) & mask);
	ctx->tempColor[3] = (int)(((u32)savedColor3 >> shift) & mask);
	ret = RenderBucket_DispatchDrawInstPrimAtRange(ctx, command, tex, ctx->idpp->unkE8, depthMac0);
	ctx->tempColor[1] = savedColor1;
	ctx->tempColor[2] = savedColor2;
	ctx->tempColor[3] = savedColor3;
	return ret;
}

static int RenderBucket_DrawSpecialPrimitive(struct RenderBucketDrawContext *ctx, u32 command, int useRtps, int reuseFirstVertex, struct TextureLayout *tex)
{
	struct RenderBucketProjectedRegs originalRegs;
	u32 mirrorFlag;
	u32 originalFlag;
	int depthMac0;

	if (useRtps != 0)
		RenderBucket_LoadPrimRTPS(ctx, reuseFirstVertex);
	else
		RenderBucket_LoadPrimRTPT(ctx);

	RenderBucket_StoreProjectedRegs(&originalRegs);
	if (useRtps != 0)
		RenderBucket_LoadSpecialMirroredRTPS(ctx, reuseFirstVertex);
	else
		RenderBucket_LoadSpecialMirroredRTPT(ctx);
	mirrorFlag = CFC2(31);
	RenderBucket_StoreProjectedRegs(&ctx->specialMirrorRegs);

	if (RenderBucket_CheckProjectedPrim(ctx, command, mirrorFlag, 0x8000, &depthMac0) != 0)
	{
		if (RenderBucket_DrawSpecialMirroredPass(ctx, command, tex, depthMac0) < 0)
			return -1;
	}

	RenderBucket_LoadProjectedRegs(&originalRegs);
	// NOTE(aalhendi): Retail reads CFC2(31) again after restoring the
	// original FIFO at 0x8006bda8.
	originalFlag = CFC2(31);
	if (RenderBucket_CheckProjectedPrim(ctx, command, originalFlag, 0, &depthMac0) != 0)
	{
		if (RenderBucket_DispatchDrawInstPrimAtRange(ctx, command, tex, ctx->idpp->unkE4, depthMac0) < 0)
			return -1;
	}

	return 0;
}

static void RenderBucket_SwapActiveRanges(struct RenderBucketDrawContext *ctx)
{
	int range = ctx->idpp->unkE4;

	ctx->idpp->unkE4 = ctx->idpp->unkE8;
	ctx->idpp->unkE8 = range;
}

static int RenderBucket_DrawReflectionPrimitive(struct RenderBucketDrawContext *ctx, u32 command, int useRtps, int reuseFirstVertex, struct TextureLayout *tex)
{
	struct RenderBucketProjectedRegs originalRegs;
	u32 mirrorFlag;
	u32 originalFlag;
	int depthMac0;
	int savedWaterSplitSide = ctx->waterSplitSide;

	if (useRtps != 0)
		RenderBucket_LoadPrimRTPS(ctx, reuseFirstVertex);
	else
		RenderBucket_LoadPrimRTPT(ctx);

	RenderBucket_StoreProjectedRegs(&originalRegs);
	if (useRtps != 0)
		RenderBucket_LoadSpecialMirroredRTPS(ctx, reuseFirstVertex);
	else
		RenderBucket_LoadSpecialMirroredRTPT(ctx);

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

static void RenderBucket_DrawFunc_Special(struct RenderBucketDrawContext *ctx)
{
	u32 *pCmd = (u32 *)ctx->idpp->ptrCommandList;

	// NOTE(aalhendi): Source-backs the visible two-pass shape of the retail
	// 0x8006bbc0 special split handler. The exact RTPS FIFO/scratchpad
	// continuation choreography is still pending a direct ASM pass.
	RenderBucket_CopyScratchColorCache(ctx);
	pCmd++;

	while (*pCmd != 0xffffffff)
	{
		u32 command = *pCmd++;
		u16 flags = (command >> 24) & 0xff;
		u16 stackIndex = (command >> 16) & 0xff;
		int startsNewStrip;
		int useRtps;
		int reuseFirstVertex;
		u32 drawCommand;
		int color;
		struct RenderBucketUncompressResult decoded;

		if ((command >> 16) == 0)
		{
			RenderBucket_ApplyColorOnlyCommand(ctx, command);
			continue;
		}

		decoded = RenderBucket_DispatchUncompressAnimationFrame(ctx, command, stackIndex);
		color = decoded.color;
		if ((flags & 4) == 0)
			ctx->vertexIndex++;

		ctx->tempCoords[0] = ctx->tempCoords[1];
		ctx->tempCoords[1] = ctx->tempCoords[2];
		ctx->tempCoords[2] = ctx->tempCoords[3];
		ctx->tempCoords[3] = ctx->stack[stackIndex];
		ctx->tempPacked[0] = ctx->tempPacked[1];
		ctx->tempPacked[1] = ctx->tempPacked[2];
		ctx->tempPacked[2] = ctx->tempPacked[3];
		ctx->tempPacked[3] = decoded.packed;

		ctx->tempColor[0] = ctx->tempColor[1];
		ctx->tempColor[1] = ctx->tempColor[2];
		ctx->tempColor[2] = ctx->tempColor[3];
		ctx->tempColor[3] = color;

		startsNewStrip = (flags & 0x80) != 0;
		if ((startsNewStrip != 0) || (ctx->stripLength == 0))
			ctx->primCommand = command;

		if (startsNewStrip != 0)
			ctx->stripLength = 0;

		useRtps = ctx->stripLength > 2;
		reuseFirstVertex = (useRtps != 0) && ((flags & 0x40) != 0);

		if (reuseFirstVertex != 0)
		{
			ctx->tempCoords[1] = ctx->tempCoords[0];
			ctx->tempPacked[1] = ctx->tempPacked[0];
			ctx->tempColor[1] = ctx->tempColor[0];
		}

		if (ctx->stripLength < 2)
		{
			ctx->stripLength++;
			continue;
		}

		drawCommand = (ctx->stripLength == 2) ? ctx->primCommand : command;

		{
			struct TextureLayout *tex;
			int isValidTexture;

			tex = RenderBucket_GetCommandTexture(ctx, drawCommand, &isValidTexture);
			if (isValidTexture == 0)
			{
				ctx->stripLength++;
				continue;
			}

			if (RenderBucket_DrawSpecialPrimitive(ctx, drawCommand, useRtps, reuseFirstVertex, tex) < 0)
				return;
		}

		ctx->stripLength++;
	}
}

static void RenderBucket_DrawFunc_Reflection(struct RenderBucketDrawContext *ctx)
{
	u32 *pCmd = (u32 *)ctx->idpp->ptrCommandList;

	// NOTE(aalhendi): Source-backs retail 0x8006c9c4's reflected split draw
	// shape. The exact scratchpad/FIFO instruction choreography is still pending
	// a direct ASM pass.
	RenderBucket_CopyScratchColorCache(ctx);
	pCmd++;

	while (*pCmd != 0xffffffff)
	{
		u32 command = *pCmd++;
		u16 flags = (command >> 24) & 0xff;
		u16 stackIndex = (command >> 16) & 0xff;
		int startsNewStrip;
		int useRtps;
		int reuseFirstVertex;
		u32 drawCommand;
		int color;
		struct RenderBucketUncompressResult decoded;

		if ((command >> 16) == 0)
		{
			RenderBucket_ApplyColorOnlyCommand(ctx, command);
			continue;
		}

		decoded = RenderBucket_DispatchUncompressAnimationFrame(ctx, command, stackIndex);
		color = decoded.color;
		if ((flags & 4) == 0)
			ctx->vertexIndex++;

		ctx->tempCoords[0] = ctx->tempCoords[1];
		ctx->tempCoords[1] = ctx->tempCoords[2];
		ctx->tempCoords[2] = ctx->tempCoords[3];
		ctx->tempCoords[3] = ctx->stack[stackIndex];
		ctx->tempPacked[0] = ctx->tempPacked[1];
		ctx->tempPacked[1] = ctx->tempPacked[2];
		ctx->tempPacked[2] = ctx->tempPacked[3];
		ctx->tempPacked[3] = decoded.packed;
		ctx->tempSplit[0] = ctx->tempSplit[1];
		ctx->tempSplit[1] = ctx->tempSplit[2];
		ctx->tempSplit[2] = ctx->tempSplit[3];
		RenderBucket_InitWaterSplitVertex(ctx, 3, ctx->tempPacked[3].xy, ctx->tempPacked[3].z, color);

		ctx->tempColor[0] = ctx->tempColor[1];
		ctx->tempColor[1] = ctx->tempColor[2];
		ctx->tempColor[2] = ctx->tempColor[3];
		ctx->tempColor[3] = color;

		startsNewStrip = (flags & 0x80) != 0;
		if ((startsNewStrip != 0) || (ctx->stripLength == 0))
			ctx->primCommand = command;

		if (startsNewStrip != 0)
			ctx->stripLength = 0;

		useRtps = ctx->stripLength > 2;
		reuseFirstVertex = (useRtps != 0) && ((flags & 0x40) != 0);

		if (reuseFirstVertex != 0)
		{
			ctx->tempCoords[1] = ctx->tempCoords[0];
			ctx->tempPacked[1] = ctx->tempPacked[0];
			ctx->tempSplit[1] = ctx->tempSplit[0];
			ctx->tempColor[1] = ctx->tempColor[0];
		}

		if (ctx->stripLength < 2)
		{
			ctx->stripLength++;
			continue;
		}

		drawCommand = (ctx->stripLength == 2) ? ctx->primCommand : command;

		{
			struct TextureLayout *tex;
			int isValidTexture;

			tex = RenderBucket_GetCommandTexture(ctx, drawCommand, &isValidTexture);
			if (isValidTexture == 0)
			{
				ctx->stripLength++;
				continue;
			}

			if (RenderBucket_DrawReflectionPrimitive(ctx, drawCommand, useRtps, reuseFirstVertex, tex) < 0)
				return;
		}

		ctx->stripLength++;
	}
}

static void RenderBucket_DrawFunc_Split(struct RenderBucketDrawContext *ctx)
{
	u32 *pCmd = (u32 *)ctx->idpp->ptrCommandList;

	// NOTE(aalhendi): ASM-verified against NTSC-U 926 0x8006b030-0x8006b24c.
	// The called water split helper at 0x8006d094 is audited separately.
	RenderBucket_CopyScratchColorCache(ctx);
	pCmd++;

	while (*pCmd != 0xffffffff)
	{
		u32 command = *pCmd++;
		u16 flags = (command >> 24) & 0xff;
		u16 stackIndex = (command >> 16) & 0xff;
		int startsNewStrip;
		int useRtps;
		int reuseFirstVertex;
		u32 drawCommand;
		int color;
		struct RenderBucketUncompressResult decoded;

		if ((command >> 16) == 0)
		{
			RenderBucket_ApplyColorOnlyCommand(ctx, command);
			continue;
		}

		decoded = RenderBucket_DispatchUncompressAnimationFrame(ctx, command, stackIndex);
		color = decoded.color;
		if ((flags & 4) == 0)
			ctx->vertexIndex++;

		ctx->tempCoords[0] = ctx->tempCoords[1];
		ctx->tempCoords[1] = ctx->tempCoords[2];
		ctx->tempCoords[2] = ctx->tempCoords[3];
		ctx->tempCoords[3] = ctx->stack[stackIndex];
		ctx->tempPacked[0] = ctx->tempPacked[1];
		ctx->tempPacked[1] = ctx->tempPacked[2];
		ctx->tempPacked[2] = ctx->tempPacked[3];
		ctx->tempPacked[3] = decoded.packed;
		ctx->tempSplit[0] = ctx->tempSplit[1];
		ctx->tempSplit[1] = ctx->tempSplit[2];
		ctx->tempSplit[2] = ctx->tempSplit[3];
		RenderBucket_InitWaterSplitVertex(ctx, 3, ctx->tempPacked[3].xy, ctx->tempPacked[3].z, color);

		ctx->tempColor[0] = ctx->tempColor[1];
		ctx->tempColor[1] = ctx->tempColor[2];
		ctx->tempColor[2] = ctx->tempColor[3];
		ctx->tempColor[3] = color;

		startsNewStrip = (flags & 0x80) != 0;
		if ((startsNewStrip != 0) || (ctx->stripLength == 0))
			ctx->primCommand = command;

		if (startsNewStrip != 0)
			ctx->stripLength = 0;

		useRtps = ctx->stripLength > 2;
		reuseFirstVertex = (useRtps != 0) && ((flags & 0x40) != 0);

		if (reuseFirstVertex != 0)
		{
			ctx->tempCoords[1] = ctx->tempCoords[0];
			ctx->tempPacked[1] = ctx->tempPacked[0];
			ctx->tempSplit[1] = ctx->tempSplit[0];
			ctx->tempColor[1] = ctx->tempColor[0];
		}

		if (ctx->stripLength < 2)
		{
			ctx->stripLength++;
			continue;
		}

		drawCommand = (ctx->stripLength == 2) ? ctx->primCommand : command;

		{
			struct TextureLayout *tex;
			int depthMac0;
			int isValidTexture;
			int shouldDraw;

			shouldDraw = RenderBucket_ProjectPrim_Split(ctx, drawCommand, useRtps, reuseFirstVertex, &depthMac0);
			if (shouldDraw == 0)
			{
				ctx->stripLength++;
				continue;
			}

			tex = RenderBucket_GetCommandTexture(ctx, drawCommand, &isValidTexture);
			if (isValidTexture == 0)
			{
				ctx->stripLength++;
				continue;
			}

			if (RenderBucket_DrawWaterSplitClipped(ctx, drawCommand, tex, depthMac0) < 0)
				return;
		}

		ctx->stripLength++;
	}
}

static void RenderBucket_DrawFunc_NormalAlt(struct RenderBucketDrawContext *ctx)
{
	u32 *pCmd = (u32 *)ctx->idpp->ptrCommandList;

	// NOTE(aalhendi): Source-backs the 0x8006a6b8 alternate normal entry's
	// split plane setup. Retail stores the full `H * 2 - TRZ` word at
	// scratchpad 0xdc/0xf4 for generated vertex Z, and separately consumes its
	// low halfword from scratchpad 0x44 for split-distance tests.
	ctx->splitPlane = ((s32)CFC2(26) << 1) - (s32)CFC2(7);
	RenderBucket_CopyScratchColorCache(ctx);
	pCmd++;

	while (*pCmd != 0xffffffff)
	{
		u32 command = *pCmd++;
		u16 flags = (command >> 24) & 0xff;
		u16 stackIndex = (command >> 16) & 0xff;
		int startsNewStrip;
		int useRtps;
		int reuseFirstVertex;
		u32 drawCommand;
		int color;
		struct RenderBucketUncompressResult decoded;

		if ((command >> 16) == 0)
		{
			RenderBucket_ApplyColorOnlyCommand(ctx, command);
			continue;
		}

		decoded = RenderBucket_DispatchUncompressAnimationFrame(ctx, command, stackIndex);
		color = decoded.color;
		if ((flags & 4) == 0)
			ctx->vertexIndex++;

		ctx->tempCoords[0] = ctx->tempCoords[1];
		ctx->tempCoords[1] = ctx->tempCoords[2];
		ctx->tempCoords[2] = ctx->tempCoords[3];
		ctx->tempCoords[3] = ctx->stack[stackIndex];
		ctx->tempPacked[0] = ctx->tempPacked[1];
		ctx->tempPacked[1] = ctx->tempPacked[2];
		ctx->tempPacked[2] = ctx->tempPacked[3];
		ctx->tempPacked[3] = decoded.packed;
		ctx->tempSplit[0] = ctx->tempSplit[1];
		ctx->tempSplit[1] = ctx->tempSplit[2];
		ctx->tempSplit[2] = ctx->tempSplit[3];
		RenderBucket_InitSplitVertex(ctx, 3, ctx->tempPacked[3].xy, ctx->tempPacked[3].z, color);

		ctx->tempColor[0] = ctx->tempColor[1];
		ctx->tempColor[1] = ctx->tempColor[2];
		ctx->tempColor[2] = ctx->tempColor[3];
		ctx->tempColor[3] = color;

		startsNewStrip = (flags & 0x80) != 0;
		if ((startsNewStrip != 0) || (ctx->stripLength == 0))
			ctx->primCommand = command;

		if (startsNewStrip != 0)
			ctx->stripLength = 0;

		useRtps = ctx->stripLength > 2;
		reuseFirstVertex = (useRtps != 0) && ((flags & 0x40) != 0);

		if (reuseFirstVertex != 0)
		{
			ctx->tempCoords[1] = ctx->tempCoords[0];
			ctx->tempPacked[1] = ctx->tempPacked[0];
			ctx->tempSplit[1] = ctx->tempSplit[0];
			ctx->tempColor[1] = ctx->tempColor[0];
		}

		if (ctx->stripLength < 2)
		{
			ctx->stripLength++;
			continue;
		}

		drawCommand = (ctx->stripLength == 2) ? ctx->primCommand : command;

		{
			struct TextureLayout *tex;
			int depthMac0;
			int isValidTexture;
			int shouldDraw;

			shouldDraw = RenderBucket_ProjectPrim_Split(ctx, drawCommand, useRtps, reuseFirstVertex, &depthMac0);
			if (shouldDraw == 0)
			{
				ctx->stripLength++;
				continue;
			}

			tex = RenderBucket_GetCommandTexture(ctx, drawCommand, &isValidTexture);
			if (isValidTexture == 0)
			{
				ctx->stripLength++;
				continue;
			}

			if (RenderBucket_DrawSplitClipped(ctx, drawCommand, tex, depthMac0) < 0)
				return;
		}

		ctx->stripLength++;
	}
}

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
	switch ((u32)(uintptr_t)ctx->inst->funcPtr[0])
	{
	case RB_RETAIL_INST_SETUP_LIGHT_COLOR:
		CTC2((s8)ctx->inst->unk53, 16);
		CTC2((u16)ctx->inst->reflectionRGBA, 17);
		CTC2(*(u32 *)&ctx->idpp->specLight[0], 19);
		CTC2((u16)ctx->idpp->specLight[2], 20);
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
		// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006c984-0x8006c9c4.
		// The syms label calls this Draw_KartGhost, but this is the
		// Instance+0x5c setup/fade callback.
		if (ctx->inst->colorRGBA != 0)
		{
			RenderBucket_SetFarColorFromInstance(ctx->inst);
			return 1;
		}

		{
			u8 *scratchFade = CTR_SCRATCHPAD_PTR(u8, 0x124);
			int fade = (int)((u32)(0x1000 - (s32)ctx->inst->alphaScale) >> 5);

			scratchFade[0] = (u8)fade;
			scratchFade[1] = (u8)fade;
			scratchFade[2] = (u8)fade;
			scratchFade[3] = 0x22;
			if (ctx->inst->alphaScale == 0x1000)
				return 0;
		}

		RenderBucket_SetFarColorFromInstance(ctx->inst);
		return 1;

	default:
		// TODO(aalhendi): Port the remaining Instance+0x5c setup callbacks when
		// their selector rows become live. Do not substitute the common-color path;
		// that hides missing retail setup callbacks.
#ifdef CTR_INTERNAL
		if (CtrTireDebug_ShouldLog(CTR_TIREDBG_RENDERBUCKET_UNHANDLED) != 0)
		{
			fprintf(stderr, "[TIREDBG][rb-unhandled-setup] inst=%p func=%p handler=%08x prim=%p\n", (void *)ctx->inst, ctx->inst->funcPtr[0],
			        (u32)ctx->idpp->unkEC, ctx->inst->funcPtr[1]);
		}
#endif
		return 0;
	}
}

static void RenderBucket_DispatchDrawFunc(struct RenderBucketDrawContext *ctx)
{
	if (RenderBucket_RunInstanceSetupCallback(ctx) == 0)
		return;

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
#ifdef CTR_INTERNAL
		if (CtrTireDebug_ShouldLog(CTR_TIREDBG_RENDERBUCKET_UNHANDLED) != 0)
		{
			fprintf(stderr, "[TIREDBG][rb-unhandled-drawfunc] inst=%p handler=%08x uncompress=%08x flags=%08x prim=%p\n", (void *)ctx->inst,
			        (u32)ctx->idpp->unkEC, (u32)ctx->idpp->unkF0, ctx->idpp->instFlags, ctx->inst->funcPtr[1]);
		}
#endif
		return;
	}
}

static int RenderBucket_PrepareDrawContext(struct RenderBucketDrawContext *ctx, struct Instance *inst, struct Instance *instPlayerBase, struct PrimMem *primMem)
{
	struct InstDrawPerPlayer *idpp;
	struct PushBuffer *pb;
	struct ModelHeader *mh;
	struct ModelFrame *mf;
	struct ModelAnim *anim;

	if (inst == 0)
		return 0;

	if (inst->model == 0)
		return 0;

	idpp = INST_GETIDPP(instPlayerBase);
	pb = idpp->pushBuffer;
	if (pb == 0)
		return 0;

	if ((idpp->instFlags & 0x40) == 0)
		return 0;

	if ((idpp->instFlags & 0x80) != 0)
		return 0;

	mh = idpp->mh;
	if (mh == 0)
		return 0;

	if ((mh->ptrCommandList == 0) || (mh->ptrColors == 0))
		return 0;

	mf = idpp->ptrCurrFrame;
	if (mf == 0)
		return 0;

	anim = RenderBucket_GetAnim(inst, mh);

	gte_SetRotMatrix(&idpp->mvp);
	gte_SetTransMatrix(&idpp->mvp);
	gte_SetGeomOffset(pb->rect.w >> 1, pb->rect.h >> 1);
	gte_SetGeomScreen(pb->distanceToScreen_PREV);
	if ((idpp->instFlags & RB_INSTANCE_SPLIT_STATE_MASK) != 0)
		RenderBucket_GteLoadLightMatrixWords(&idpp->m3x3);

	ctx->inst = inst;
	ctx->idpp = idpp;
	ctx->pb = pb;
	ctx->primMem = primMem;
	ctx->mh = mh;
	ctx->mf = mf;
	ctx->anim = anim;
	ctx->vertData = MODELFRAME_GETVERT(mf);
	ctx->waterSplitSide = -1;
	if (idpp->ptrNextFrame != 0)
		ctx->nextVertData = (char *)idpp->ptrNextFrame + mf->vertexOffset;
	return 1;
}

void RenderBucket_Execute(void *param_1, struct PrimMem *param_2)
{
	struct RenderBucketEntry *entry = (struct RenderBucketEntry *)param_1;

	// NOTE(aalhendi): Native C implementation of the RenderBucket execution
	// contract. Full scratchpad/register ASM audit is still pending.
	for (; entry->inst != 0; entry++)
	{
		struct RenderBucketDrawContext ctx = {0};

		if (RenderBucket_PrepareDrawContext(&ctx, entry->inst, entry->instPlayerBase, param_2) == 0)
			continue;

		RenderBucket_DispatchDrawFunc(&ctx);
	}
}
