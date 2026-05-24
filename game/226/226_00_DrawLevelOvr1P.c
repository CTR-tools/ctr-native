#include <common.h>

struct DrawLevelOvr1PRenderListSlot
{
	struct QuadBlock **ptrQuadBlocksRendered;
	struct VisMemBspListNode *bspListStart;
};

struct DrawLevelOvr1PRenderList
{
	struct DrawLevelOvr1PRenderListSlot list[5];
	struct VisMemBspListNode *bspListStart_FullDynamic;
	struct QuadBlock **ptrQuadBlocksRendered_FullDynamic;
};

enum DrawLevelOvr1PBucketKind
{
	DRAW_LEVEL_OVR1P_BUCKET_QUADBLOCKS_RENDERED,
	DRAW_LEVEL_OVR1P_BUCKET_BSP_LIST,
};

enum DrawLevelOvr1PBucketRole
{
	DRAW_LEVEL_OVR1P_BUCKET_4X4_RENDERED,
	DRAW_LEVEL_OVR1P_BUCKET_4X4_LIST,
	DRAW_LEVEL_OVR1P_BUCKET_DYNAMIC_RENDERED,
	DRAW_LEVEL_OVR1P_BUCKET_DYNAMIC_LIST,
	DRAW_LEVEL_OVR1P_BUCKET_4X2_RENDERED,
	DRAW_LEVEL_OVR1P_BUCKET_4X2_LIST,
	DRAW_LEVEL_OVR1P_BUCKET_4X1_RENDERED,
	DRAW_LEVEL_OVR1P_BUCKET_4X1_LIST,
	DRAW_LEVEL_OVR1P_BUCKET_WATER_RENDERED,
	DRAW_LEVEL_OVR1P_BUCKET_WATER_LIST,
	DRAW_LEVEL_OVR1P_BUCKET_FULL_DYNAMIC_LIST,
};

struct DrawLevelOvr1PBucket
{
	u8 renderListOffset;
	u8 kind;
	u8 role;
	u8 lodMode;
};

struct DrawLevelOvr1PFaceSelector
{
	u32 selector;
	u8 drawOrderShift;
};

struct DrawLevelOvr1PNearSubdivisionCase
{
	u32 listHandlerAddress;
	u32 renderedHandlerAddress;
	u8 subIndices[2][4];
	u32 directMasks[2];
	u32 slotWords[2];
};

enum DrawLevelOvr1PUvRot
{
	DRAW_LEVEL_OVR1P_UV_NONE = 0,
	DRAW_LEVEL_OVR1P_UV_ROT90 = 1,
	DRAW_LEVEL_OVR1P_UV_ROT180 = 2,
	DRAW_LEVEL_OVR1P_UV_ROT270 = 3,
	DRAW_LEVEL_OVR1P_UV_FLIP_ROT270 = 4,
	DRAW_LEVEL_OVR1P_UV_FLIP_ROT180 = 5,
	DRAW_LEVEL_OVR1P_UV_FLIP_ROT90 = 6,
	DRAW_LEVEL_OVR1P_UV_FLIP = 7,
};

enum DrawLevelOvr1PLod
{
	DRAW_LEVEL_OVR1P_LOD_HIGH,
	DRAW_LEVEL_OVR1P_LOD_LOW,
};

enum DrawLevelOvr1PGteFlag
{
	DRAW_LEVEL_OVR1P_GTE_RTPT_OVERFLOW = 0x20000,
};

enum DrawLevelOvr1PDepth
{
	DRAW_LEVEL_OVR1P_MAX_OT_INDEX = 1020,
	DRAW_LEVEL_OVR1P_RECURSION_FRAME_SIZE = 0xb8,
	DRAW_LEVEL_OVR1P_MAX_NEAR_SUBDIV_DEPTH = 2,
	DRAW_LEVEL_OVR1P_CLIP_RECORD_GT3_SIZE = 0x30,
	DRAW_LEVEL_OVR1P_CLIP_RECORD_GT4_SIZE = 0x3c,
	DRAW_LEVEL_OVR1P_CLIP_RECORD_PRIM_RESERVE = 0xd68,
	DRAW_LEVEL_OVR1P_BUCKET_RESERVE_4X1 = 0x1040,
	DRAW_LEVEL_OVR1P_BUCKET_RESERVE_4X2 = 0x1380,
	DRAW_LEVEL_OVR1P_BUCKET_RESERVE_DEFAULT = 0x1a00,
	DRAW_LEVEL_OVR1P_BUCKET_RESERVE_FULL_DYNAMIC = 0x2700,
};

enum DrawLevelOvr1PDirectMask
{
	DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY = 0x4,
	DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY = 0x8,
	DRAW_LEVEL_OVR1P_DIRECT_QUAD = 0xc,
};

enum DrawLevelOvr1PClipBytes
{
	DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST = 0,
	DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED = 1,
};

enum DrawLevelOvr1POtIndex
{
	DRAW_LEVEL_OVR1P_OT_INDEX_AUTO = -1,
};

static const u32 DRAW_LEVEL_OVR1P_SLOT_WORD_PRESERVE = 0xffffffff;

struct DrawLevelOvr1PScratchVertex
{
	s16 pos[3];
	u16 flags;
	u8 color_hi[4];
	s16 posScreen[2];
	u16 depth;
	u8 clipNear;
	u8 clipHalfNear;
};

struct DrawLevelOvr1PClipRecordVertex
{
	s16 pos[3];
	u16 flags;
	u8 color_hi[4];
};

struct DrawLevelOvr1PClipRecord
{
	u32 header;
	u32 otEntry;
	s16 tpage;
	s16 clut;
	struct DrawLevelOvr1PClipRecordVertex vertex[4];
};

_Static_assert(sizeof(struct DrawLevelOvr1PScratchVertex) == 0x14);
_Static_assert(sizeof(struct DrawLevelOvr1PClipRecordVertex) == 0xc);
_Static_assert(sizeof(struct DrawLevelOvr1PClipRecord) == 0x3c);

static const int sDrawLevelOvr1PHighLodIndices[16] = {
    5, 0, 6, 4, 6, 4, 7, 1, 2, 5, 8, 6, 8, 6, 3, 7,
};

static const int sDrawLevelOvr1PLowLodIndices[4] = {
    2,
    0,
    3,
    1,
};

static const int sDrawLevelOvr1PFullDynamicLowIndices[4] = {
    0,
    1,
    2,
    3,
};

static const int sDrawLevelOvr1PGridFaceIndices[4][4] = {
    {0, 4, 5, 6},
    {4, 1, 6, 7},
    {5, 6, 2, 8},
    {6, 7, 8, 3},
};

static const int sDrawLevelOvr1PGridExtraFaceIndices[4][4] = {
    {5, 0, 6, 1},
    {1, 3, 6, 8},
    {2, 0, 6, 4},
    {2, 6, 3, 7},
};

static const int sDrawLevelOvr1PGridMixedFaceIndices[4][4] = {
    {4, 1, 6, 3},
    {5, 6, 2, 3},
    {0, 1, 6, 7},
    {0, 6, 2, 8},
};

static const struct DrawLevelOvr1PFaceSelector sDrawLevelOvr1P4x1FaceSelectors[4] = {
    {0x00506478, 8},
    {0x5014788c, 13},
    {0x647828a0, 18},
    {0x788ca03c, 23},
};

// NOTE(aalhendi): These native cases are keyed by the copied retail recursive
// handler addresses at scratch 0x14c. List and rendered handlers share topology.
static const struct DrawLevelOvr1PNearSubdivisionCase sDrawLevelOvr1PNearSubdivisionCases[3] = {
    {0x800a39c4,
     0x800a44e0,
     {{0, 4, 2, 3}, {1, 3, 4, 2}},
     {DRAW_LEVEL_OVR1P_DIRECT_QUAD, DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY},
     {DRAW_LEVEL_OVR1P_SLOT_WORD_PRESERVE, DRAW_LEVEL_OVR1P_SLOT_WORD_PRESERVE}},
    {0x800a39fc,
     0x800a4518,
     {{1, 5, 0, 2}, {0, 1, 5, 3}},
     {DRAW_LEVEL_OVR1P_DIRECT_QUAD, DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY},
     {DRAW_LEVEL_OVR1P_SLOT_WORD_PRESERVE, DRAW_LEVEL_OVR1P_SLOT_WORD_PRESERVE}},
    {0x800a3a34, 0x800a4550, {{0, 4, 2, 5}, {4, 1, 5, 3}}, {DRAW_LEVEL_OVR1P_DIRECT_QUAD, DRAW_LEVEL_OVR1P_DIRECT_QUAD}, {0x0, 0xc}},
};

// NOTE(aalhendi): Deepest generic-grid helpers at 0x800a535c..0x800a557c /
// 0x800a6260..0x800a6480 fall through to a 3x3 topology, not the 4x1 compact
// topology above. The bottom/right cases use projected record 8.
static const struct DrawLevelOvr1PNearSubdivisionCase sDrawLevelOvr1PDeepestGridSubdivisionCases[3] = {
    {0,
     0,
     {{0, 4, 2, 3}, {1, 3, 4, 2}},
     {DRAW_LEVEL_OVR1P_DIRECT_QUAD, DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY},
     {DRAW_LEVEL_OVR1P_SLOT_WORD_PRESERVE, DRAW_LEVEL_OVR1P_SLOT_WORD_PRESERVE}},
    {0,
     0,
     {{1, 8, 0, 2}, {0, 1, 8, 3}},
     {DRAW_LEVEL_OVR1P_DIRECT_QUAD, DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY},
     {DRAW_LEVEL_OVR1P_SLOT_WORD_PRESERVE, DRAW_LEVEL_OVR1P_SLOT_WORD_PRESERVE}},
    {0, 0, {{0, 4, 2, 8}, {4, 1, 8, 3}}, {DRAW_LEVEL_OVR1P_DIRECT_QUAD, DRAW_LEVEL_OVR1P_DIRECT_QUAD}, {0x0, 0xc}},
};

// NOTE(aalhendi): Retail overlay 226 dispatches these offsets from 0x28 down
// to 0x00 through tables at 0x800ab40c/0x800ab438.
static const struct DrawLevelOvr1PBucket sDrawLevelOvr1PBuckets[] = {
    {0x00, DRAW_LEVEL_OVR1P_BUCKET_QUADBLOCKS_RENDERED, DRAW_LEVEL_OVR1P_BUCKET_4X4_RENDERED, DRAW_LEVEL_OVR1P_LOD_HIGH},
    {0x04, DRAW_LEVEL_OVR1P_BUCKET_BSP_LIST, DRAW_LEVEL_OVR1P_BUCKET_4X4_LIST, DRAW_LEVEL_OVR1P_LOD_HIGH},
    {0x08, DRAW_LEVEL_OVR1P_BUCKET_QUADBLOCKS_RENDERED, DRAW_LEVEL_OVR1P_BUCKET_DYNAMIC_RENDERED, DRAW_LEVEL_OVR1P_LOD_HIGH},
    {0x0c, DRAW_LEVEL_OVR1P_BUCKET_BSP_LIST, DRAW_LEVEL_OVR1P_BUCKET_DYNAMIC_LIST, DRAW_LEVEL_OVR1P_LOD_HIGH},
    {0x10, DRAW_LEVEL_OVR1P_BUCKET_QUADBLOCKS_RENDERED, DRAW_LEVEL_OVR1P_BUCKET_4X2_RENDERED, DRAW_LEVEL_OVR1P_LOD_HIGH},
    {0x14, DRAW_LEVEL_OVR1P_BUCKET_BSP_LIST, DRAW_LEVEL_OVR1P_BUCKET_4X2_LIST, DRAW_LEVEL_OVR1P_LOD_HIGH},
    {0x18, DRAW_LEVEL_OVR1P_BUCKET_QUADBLOCKS_RENDERED, DRAW_LEVEL_OVR1P_BUCKET_4X1_RENDERED, DRAW_LEVEL_OVR1P_LOD_LOW},
    {0x1c, DRAW_LEVEL_OVR1P_BUCKET_BSP_LIST, DRAW_LEVEL_OVR1P_BUCKET_4X1_LIST, DRAW_LEVEL_OVR1P_LOD_LOW},
    {0x20, DRAW_LEVEL_OVR1P_BUCKET_QUADBLOCKS_RENDERED, DRAW_LEVEL_OVR1P_BUCKET_WATER_RENDERED, DRAW_LEVEL_OVR1P_LOD_HIGH},
    {0x24, DRAW_LEVEL_OVR1P_BUCKET_BSP_LIST, DRAW_LEVEL_OVR1P_BUCKET_WATER_LIST, DRAW_LEVEL_OVR1P_LOD_HIGH},
    {0x28, DRAW_LEVEL_OVR1P_BUCKET_BSP_LIST, DRAW_LEVEL_OVR1P_BUCKET_FULL_DYNAMIC_LIST, DRAW_LEVEL_OVR1P_LOD_LOW},
};

_Static_assert(sizeof(sDrawLevelOvr1PBuckets) / sizeof(sDrawLevelOvr1PBuckets[0]) == OVR226_BUCKET_COUNT);
_Static_assert(sizeof(struct OverlayRDATA_226) == 0x564);
_Static_assert(sizeof(struct OverlayRDATA_226_BucketSetupRecord) == 0x64);
_Static_assert(sizeof(((struct OverlayRDATA_226 *)0)->scratchInitTable) == 0x60);
_Static_assert(sizeof(((struct OverlayRDATA_226 *)0)->clipRecordJumpTable) == 0x60);

static void DrawLevelOvr1P_SetGridFaceSlot(const struct DrawLevelOvr1PScratchVertex *projected, int faceIndex);

static void DrawLevelOvr1P_SetActiveDrawOrderLow(const struct QuadBlock *block)
{
	*CTR_SCRATCHPAD_PTR(u32, 0x7c) = block->draw_order_low;
}

static u32 DrawLevelOvr1P_GetActiveDrawOrderLow(void)
{
	return *CTR_SCRATCHPAD_PTR(u32, 0x7c);
}

static void DrawLevelOvr1P_CopyScratchWords(const u32 *source, const struct OverlayRDATA_226_BucketSetupCopy *copy)
{
	u32 *scratch = CTR_SCRATCHPAD_PTR(u32, copy->scratchOffset);

	for (u32 i = 0; i <= copy->loopCounter; i++)
		scratch[i] = source[i];
}

static void DrawLevelOvr1P_InitClipRecordJumpTable(void)
{
	u32 *clipRecordJumpTable = CTR_SCRATCHPAD_PTR(u32, 0x240);

	for (int i = 0; i < OVR226_CLIP_RECORD_JUMP_WORD_COUNT; i++)
		clipRecordJumpTable[i] = R226.clipRecordJumpTable[i];
}

static const struct OverlayRDATA_226_BucketSetupRecord *DrawLevelOvr1P_FindBucketSetupRecord(u32 setupAddress)
{
	for (int i = 0; i < OVR226_BUCKET_COUNT; i++)
	{
		u32 recordAddress = OVR226_RDATA_BUCKET_SETUP_BASE + (u32)(i * sizeof(R226.bucketSetups[0]));

		if (recordAddress == setupAddress)
			return &R226.bucketSetups[i];
	}

	return NULL;
}

static void DrawLevelOvr1P_InitScratchpadTables(void)
{
	u32 *scratch = CTR_SCRATCHPAD_PTR(u32, 0xec);

	for (int i = 0; i < OVR226_SCRATCH_INIT_WORD_COUNT; i++)
		scratch[i] = R226.scratchInitTable[i];

	DrawLevelOvr1P_InitClipRecordJumpTable();
}

static void DrawLevelOvr1P_ApplyBucketSetup(int bucketIndex)
{
	const struct OverlayRDATA_226_BucketSetupRecord *setup = DrawLevelOvr1P_FindBucketSetupRecord(R226.bucketSetupAddresses[bucketIndex]);

	if (setup == NULL)
		return;

	DrawLevelOvr1P_CopyScratchWords(setup->copy0, &setup->copies[0]);
	DrawLevelOvr1P_CopyScratchWords(setup->copy1, &setup->copies[1]);
}

static u32 DrawLevelOvr1P_Select4x1ProjectedTableWord(const struct QuadBlock *block, const struct DrawLevelOvr1PFaceSelector *selector)
{
	u32 tableIndex;

	(void)block;

	tableIndex = (DrawLevelOvr1P_GetActiveDrawOrderLow() >> selector->drawOrderShift) & 0x1f;

	return *CTR_SCRATCHPAD_PTR(u32, 0xec + (int)(tableIndex * sizeof(u32)));
}

static u32 DrawLevelOvr1P_Select4x1ProjectedIndices(const struct QuadBlock *block, const struct DrawLevelOvr1PFaceSelector *selector, int *indices)
{
	u32 tableWord = DrawLevelOvr1P_Select4x1ProjectedTableWord(block, selector);

	*CTR_SCRATCHPAD_PTR(u32, 0x194) = tableWord;

	for (int i = 0; i < 4; i++)
	{
		u32 tableShift = (tableWord >> ((3 - i) * 8)) & 0x1f;
		u32 recordOffset = (selector->selector >> tableShift) & 0xff;

		indices[i] = recordOffset / (int)sizeof(struct DrawLevelOvr1PScratchVertex);
	}

	return tableWord;
}

static u32 DrawLevelOvr1P_Select4x1ProjectedFace(const struct DrawLevelOvr1PScratchVertex *projected, const struct QuadBlock *block, int faceIndex,
                                                 int *indices)
{
	// NOTE(aalhendi): Retail selector helpers store the active face slot at
	// frame+0xb4 before decoding the 0xec table word.
	DrawLevelOvr1P_SetGridFaceSlot(projected, faceIndex);
	return DrawLevelOvr1P_Select4x1ProjectedIndices(block, &sDrawLevelOvr1P4x1FaceSelectors[faceIndex], indices);
}

static int DrawLevelOvr1P_IsFaceVisible(const int *visFaceList, const struct QuadBlock *block)
{
	u32 blockID = (u16)block->blockID;
	u32 wordIndex;
	u32 word;

	if (visFaceList == 0)
		return 1;

	// NOTE(aalhendi): Retail 0x800a36d4 forms a byte offset with
	// `(blockID >> 3) & 0x1fc`; scale it back to the native int array index.
	wordIndex = ((blockID >> 3) & 0x1fc) >> 2;
	word = (u32)visFaceList[wordIndex];
	return (s32)(word << (blockID & 0x1f)) < 0;
}

static u32 DrawLevelOvr1P_GetGridFaceSlotWord(const struct DrawLevelOvr1PScratchVertex *projected)
{
	return *(const u32 *)((const u8 *)projected + 0xb4);
}

static struct TextureLayout *DrawLevelOvr1P_ResolveTexturePointer(uintptr_t texturePtr)
{
	struct TextureLayout *texture;

	if (texturePtr == 0)
		return NULL;

	if ((texturePtr & 1) != 0)
		texture = *(struct TextureLayout **)(texturePtr - 1);
	else
		texture = (struct TextureLayout *)texturePtr;

	return texture;
}

static struct TextureLayout *DrawLevelOvr1P_ResolveMidTexture(const struct QuadBlock *block, int faceIndex)
{
	return DrawLevelOvr1P_ResolveTexturePointer((uintptr_t)block->ptr_texture_mid[faceIndex]);
}

static int DrawLevelOvr1P_IsPlausibleTextureLayout(const struct TextureLayout *texture)
{
	if (texture == NULL)
		return 0;

	return (texture->tpage & 0xfe00) == 0;
}

static int DrawLevelOvr1P_IsNativeLevelTexturePointer(u32 value)
{
#ifdef REBUILD_PC
	uintptr_t ptr = (uintptr_t)value;
	uintptr_t levelStart;
	uintptr_t levelEnd;

	if (sdata->PtrMempack == NULL || sdata->ptrLevelFile == NULL)
		return 0;

	levelStart = (uintptr_t)sdata->ptrLevelFile;
	levelEnd = (uintptr_t)sdata->PtrMempack->lastFreeByte;

	if (ptr < levelStart || ptr >= levelEnd || levelEnd - ptr < sizeof(struct TextureLayout))
		return 0;

	return DrawLevelOvr1P_IsPlausibleTextureLayout((const struct TextureLayout *)ptr);
#else
	(void)value;
	return 0;
#endif
}

static int DrawLevelOvr1P_TreatAsRetailNegativeTextureWord(u32 value)
{
	return (s32)value < 0 || DrawLevelOvr1P_IsNativeLevelTexturePointer(value);
}

static int DrawLevelOvr1P_IsRepresentableActiveSlotWord(u32 slotWord)
{
	return (slotWord & 3) == 0 && slotWord <= 0xc;
}

static u32 DrawLevelOvr1P_GetProjectedRepresentableSlotWord(const struct DrawLevelOvr1PScratchVertex *projected, int faceIndex)
{
	u32 slotWord = projected != NULL ? DrawLevelOvr1P_GetGridFaceSlotWord(projected) : (u32)(faceIndex * 4);

	// TODO(aalhendi): Retail 0x800a850c/0x800aa12c also uses wide active
	// slots from helpers 0x800a557c/0x800a6480/0x800a745c/0x800a810c/
	// 0x800a907c/0x800a9d2c for OT bias reads. Native QuadBlock only has
	// representable texture/draw-order data for slots 0/4/8/0xc right now.
	if (!DrawLevelOvr1P_IsRepresentableActiveSlotWord(slotWord))
		return (u32)(faceIndex * 4);

	return slotWord;
}

static struct TextureLayout *DrawLevelOvr1P_ResolveProjectedMidTexture(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected)
{
	u32 slotWord;

	if (projected == NULL)
		return NULL;

	slotWord = DrawLevelOvr1P_GetGridFaceSlotWord(projected);

	// TODO(aalhendi): Retail selector entries 0x800a8380/0x800a9fa0 pass
	// representable slots 0/4/8/0xc. Default wide helpers 0x800a745c,
	// 0x800a810c, 0x800a907c, and 0x800a9d2c can carry 0x30/0x3c into terminal
	// active-slot reads; keep those bounded until exact data/dispatch is ported.
	if (!DrawLevelOvr1P_IsRepresentableActiveSlotWord(slotWord))
		return NULL;

	return DrawLevelOvr1P_ResolveTexturePointer((uintptr_t)*(void *const *)((const u8 *)block + 0x1c + slotWord));
}

static struct TextureLayout *DrawLevelOvr1P_GetTexture(const struct QuadBlock *block, int faceIndex, int numFaces)
{
	struct TextureLayout *texture = block->ptr_texture_low;

	if (numFaces == 4)
	{
		texture = DrawLevelOvr1P_ResolveMidTexture(block, faceIndex);

		if (texture != 0)
			texture += 2;
	}

	return texture;
}

static struct TextureLayout *DrawLevelOvr1P_GetProjectedMidTexture(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected,
                                                                   int faceIndex, u32 maxDepth)
{
	struct TextureLayout *texture = DrawLevelOvr1P_ResolveProjectedMidTexture(block, projected);
	int hasWideActiveSlot;
	u32 mosaicWord;

	hasWideActiveSlot = projected != NULL && !DrawLevelOvr1P_IsRepresentableActiveSlotWord(DrawLevelOvr1P_GetGridFaceSlotWord(projected));

	// NOTE(aalhendi): Retail 0x800a8504/0x800aa124 uses the active slot, not
	// the caller face index. If the active slot is a still-unported wide slot,
	// preserve inherited UVs rather than force-mapping it to ptr_texture_mid[0..3].
	if (texture == NULL && !hasWideActiveSlot)
		texture = DrawLevelOvr1P_ResolveMidTexture(block, faceIndex);

	if (texture == NULL)
		return NULL;

	mosaicWord = *(u32 *)((u8 *)texture + 0x24);

	// NOTE(aalhendi): Retail stores texture+0x24 at scratch 0x84 for the
	// deepest-frame UV reload path.
	*CTR_SCRATCHPAD_PTR(u32, 0x84) = mosaicWord;

	if ((s32)maxDepth < *CTR_SCRATCHPAD_PTR(s32, 0x1c))
		texture++;

	if ((s32)maxDepth < *CTR_SCRATCHPAD_PTR(s32, 0x20))
		texture++;

	if ((s32)maxDepth < *CTR_SCRATCHPAD_PTR(s32, 0x24))
	{
		// NOTE(aalhendi): Retail tests this word as signed PS1 data. Pointer-map
		// rebasing turns valid 0x80xxxxxx hi-texture pointers into positive native
		// addresses, so keep those on the retail negative path.
		if (!DrawLevelOvr1P_TreatAsRetailNegativeTextureWord(mosaicWord))
			texture++;
	}

	return texture;
}

static void DrawLevelOvr1P_SetUvRotation(POLY_GT4 *prim, const struct TextureLayout *texture, u32 rotation)
{
	switch (rotation & 7)
	{
	case DRAW_LEVEL_OVR1P_UV_NONE:
		setUV4(prim, texture->u2, texture->v2, texture->u0, texture->v0, texture->u3, texture->v3, texture->u1, texture->v1);
		break;
	case DRAW_LEVEL_OVR1P_UV_ROT90:
		setUV4(prim, texture->u3, texture->v3, texture->u2, texture->v2, texture->u1, texture->v1, texture->u0, texture->v0);
		break;
	case DRAW_LEVEL_OVR1P_UV_ROT180:
		setUV4(prim, texture->u1, texture->v1, texture->u3, texture->v3, texture->u0, texture->v0, texture->u2, texture->v2);
		break;
	case DRAW_LEVEL_OVR1P_UV_ROT270:
		setUV4(prim, texture->u0, texture->v0, texture->u1, texture->v1, texture->u2, texture->v2, texture->u3, texture->v3);
		break;
	case DRAW_LEVEL_OVR1P_UV_FLIP_ROT270:
		setUV4(prim, texture->u1, texture->v1, texture->u0, texture->v0, texture->u3, texture->v3, texture->u2, texture->v2);
		break;
	case DRAW_LEVEL_OVR1P_UV_FLIP_ROT180:
		setUV4(prim, texture->u0, texture->v0, texture->u2, texture->v2, texture->u1, texture->v1, texture->u3, texture->v3);
		break;
	case DRAW_LEVEL_OVR1P_UV_FLIP_ROT90:
		setUV4(prim, texture->u2, texture->v2, texture->u3, texture->v3, texture->u0, texture->v0, texture->u1, texture->v1);
		break;
	case DRAW_LEVEL_OVR1P_UV_FLIP:
		setUV4(prim, texture->u3, texture->v3, texture->u1, texture->v1, texture->u2, texture->v2, texture->u0, texture->v0);
		break;
	}
}

static void DrawLevelOvr1P_ApplyFaceMode(POLY_GT4 *prim, u32 faceMode)
{
	u8 u[4] = {prim->u0, prim->u1, prim->u2, prim->u3};
	u8 v[4] = {prim->v0, prim->v1, prim->v2, prim->v3};

	if (faceMode == 1)
	{
		setUV4(prim, u[2], v[2], u[0], v[0], u[3], v[3], u[1], v[1]);
	}
	else if (faceMode == 2)
	{
		setUV4(prim, u[3], v[3], u[2], v[2], u[0], v[0], u[1], v[1]);
	}
}

static u16 DrawLevelOvr1P_PackUv(u8 u, u8 v)
{
	return (u16)u | ((u16)v << 8);
}

static void DrawLevelOvr1P_WriteProjectedUv(struct DrawLevelOvr1PScratchVertex *projected, const int *indices, const struct TextureLayout *texture,
                                            u32 tableWord)
{
	u16 uv[4];

	uv[0] = DrawLevelOvr1P_PackUv(texture->u0, texture->v0);
	uv[1] = DrawLevelOvr1P_PackUv(texture->u1, texture->v1);
	uv[2] = DrawLevelOvr1P_PackUv(texture->u2, texture->v2);
	uv[3] = DrawLevelOvr1P_PackUv(texture->u3, texture->v3);

	*CTR_SCRATCHPAD_PTR(u32, 0x1a0) = uv[0] | ((u32)texture->clut << 16);
	*CTR_SCRATCHPAD_PTR(u32, 0x1a4) = uv[1] | ((u32)texture->tpage << 16);
	*CTR_SCRATCHPAD_PTR(u32, 0x1ac) = uv[0] | ((u32)texture->clut << 16);
	*CTR_SCRATCHPAD_PTR(u32, 0x1b0) = uv[1] | ((u32)texture->tpage << 16);

	// NOTE(aalhendi): Retail 4x1 handlers store selected UV halfwords at scratch-record offset 0x6 before primitive emission.
	if ((s32)(tableWord << 8) < 0)
	{
		projected[indices[0]].flags = uv[1];
		projected[indices[1]].flags = uv[0];
		projected[indices[2]].flags = uv[3];

		if ((s32)(tableWord << 24) >= 0)
			projected[indices[3]].flags = uv[2];
	}
	else
	{
		projected[indices[0]].flags = uv[0];
		projected[indices[1]].flags = uv[1];
		projected[indices[2]].flags = uv[2];

		if ((s32)(tableWord << 24) >= 0)
			projected[indices[3]].flags = uv[3];
	}
}

static void DrawLevelOvr1P_StoreProjectedDirectUvScratch(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count)
{
	// NOTE(aalhendi): Retail terminal direct helpers update UV scratch before packet emission.
	*CTR_SCRATCHPAD_PTR(s16, 0x1a0) = (s16)projected[indices[0]].flags;
	*CTR_SCRATCHPAD_PTR(s16, 0x1a4) = (s16)projected[indices[1]].flags;
	*CTR_SCRATCHPAD_PTR(s16, 0x1a8) = (s16)projected[indices[2]].flags;

	if (count == 4)
		*CTR_SCRATCHPAD_PTR(s16, 0x1aa) = (s16)projected[indices[3]].flags;
}

static void DrawLevelOvr1P_RestoreProjectedUvScratch(void)
{
	*CTR_SCRATCHPAD_PTR(u32, 0x1a0) = *CTR_SCRATCHPAD_PTR(u32, 0x1ac);
	*CTR_SCRATCHPAD_PTR(u32, 0x1a4) = *CTR_SCRATCHPAD_PTR(u32, 0x1b0);
}

static int DrawLevelOvr1P_GetDeepestMosaicReloadGate(u32 directHandlerAddress, u32 previousHandlerAddress, u32 *reloadSpan)
{
	u32 expectedHandlerAddress;

	switch (directHandlerAddress)
	{
	case 0x800a4034:
	case 0x800a402c:
	case 0x800a40b8:
		expectedHandlerAddress = 0x800a3a34;
		*reloadSpan = 0x30;
		break;

	case 0x800a4c14:
	case 0x800a4c0c:
	case 0x800a4cc8:
		expectedHandlerAddress = 0x800a4550;
		*reloadSpan = 0x30;
		break;

	case 0x800a5d14:
	case 0x800a5d0c:
	case 0x800a5d98:
		expectedHandlerAddress = 0x800a557c;
		*reloadSpan = 0x60;
		break;

	case 0x800a6bb4:
	case 0x800a6bac:
	case 0x800a6c68:
		expectedHandlerAddress = 0x800a6480;
		*reloadSpan = 0x60;
		break;

	case 0x800a7a60:
	case 0x800a7a58:
	case 0x800a7ae4:
		expectedHandlerAddress = 0x800a745c;
		*reloadSpan = 0xc0;
		break;

	case 0x800a87d4:
	case 0x800a87cc:
	case 0x800a8888:
		expectedHandlerAddress = 0x800a810c;
		*reloadSpan = 0xc0;
		break;

	case 0x800a9680:
	case 0x800a9678:
	case 0x800a9704:
		expectedHandlerAddress = 0x800a907c;
		*reloadSpan = 0xc0;
		break;

	case 0x800aa3f4:
	case 0x800aa3ec:
	case 0x800aa4a8:
		expectedHandlerAddress = 0x800a9d2c;
		*reloadSpan = 0xc0;
		break;

	default:
		return 0;
	}

	return previousHandlerAddress == expectedHandlerAddress;
}

static void DrawLevelOvr1P_PrepareDeepestMosaicUv(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, u32 directHandlerAddress)
{
	u32 mosaicBase;
	u32 reloadSpan;
	u32 sourceOffset;
	const u8 *source;
	u32 uv0;
	u32 uv1;
	struct DrawLevelOvr1PScratchVertex *mutableProjected;

	if (projected != CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PScratchVertex, 0x324))
		return;

	mosaicBase = *CTR_SCRATCHPAD_PTR(u32, 0x84);
	// NOTE(aalhendi): Retail 0x800a8714/0x800aa334 restores saved UV scratch
	// for positive inline sentinels, but rebased native hi-texture pointers must
	// still follow PS1's negative-pointer reload path.
	if ((s32)mosaicBase > 0 && !DrawLevelOvr1P_IsNativeLevelTexturePointer(mosaicBase))
	{
		DrawLevelOvr1P_RestoreProjectedUvScratch();
		return;
	}

	if (!DrawLevelOvr1P_GetDeepestMosaicReloadGate(directHandlerAddress, *CTR_SCRATCHPAD_PTR(u32, 0x9c), &reloadSpan))
	{
		DrawLevelOvr1P_RestoreProjectedUvScratch();
		return;
	}

	sourceOffset = *CTR_SCRATCHPAD_PTR(u32, 0x320) << 1;
	if ((s32)(*CTR_SCRATCHPAD_PTR(u32, 0x194) << 8) < 0)
		sourceOffset += reloadSpan;
	sourceOffset += *CTR_SCRATCHPAD_PTR(u32, 0x3d8);

	source = (const u8 *)(uintptr_t)(mosaicBase + sourceOffset);
	uv0 = *(const u32 *)(const void *)(source + 0);
	uv1 = *(const u32 *)(const void *)(source + 4);

	// NOTE(aalhendi): Retail deepest fallthrough rewrites selected scratch-record
	// UV halfwords before jumping through the direct table.
	mutableProjected = (struct DrawLevelOvr1PScratchVertex *)projected;
	*CTR_SCRATCHPAD_PTR(u32, 0x1a0) = uv0;
	*CTR_SCRATCHPAD_PTR(u32, 0x1a4) = uv1;
	mutableProjected[indices[0]].flags = (u16)uv0;
	mutableProjected[indices[1]].flags = (u16)uv1;
	mutableProjected[indices[2]].flags = *(const u16 *)(const void *)(source + 8);
	mutableProjected[indices[3]].flags = *(const u16 *)(const void *)(source + 10);
}

static void DrawLevelOvr1P_SetGridFaceSlot(const struct DrawLevelOvr1PScratchVertex *projected, int faceIndex)
{
	// NOTE(aalhendi): Retail 3x3 helper frames use frame+0xb4 for face slot*4.
	*(u32 *)((u8 *)projected + 0xb4) = (u32)(faceIndex * 4);
}

static void DrawLevelOvr1P_SetGridFaceSlotWord(const struct DrawLevelOvr1PScratchVertex *projected, u32 slotWord)
{
	*(u32 *)((u8 *)projected + 0xb4) = slotWord;
}

static u32 DrawLevelOvr1P_GetDefaultGridFaceSlotWord(u32 handlerAddress, int faceIndex)
{
	static const u32 sNormalSlotWords[] = {0x0, 0x4, 0x8, 0xc};
	static const u32 sWide2SlotWords[] = {0x0, 0xc, 0x18, 0x24};
	static const u32 sWide4SlotWords[] = {0x0, 0xc, 0x30, 0x3c};

	// NOTE(aalhendi): Retail default composite helpers stride frame+0xb4 by the
	// copied helper family's deepest UV reload span, not always by faceIndex*4.
	// The wide4 helpers intentionally use QuadBlock-relative slots 0/0xc/0x30/0x3c.
	switch (handlerAddress)
	{
	case 0x800a557c:
	case 0x800a6480:
		return sWide2SlotWords[faceIndex];

	case 0x800a745c:
	case 0x800a810c:
	case 0x800a907c:
	case 0x800a9d2c:
		return sWide4SlotWords[faceIndex];

	default:
		return sNormalSlotWords[faceIndex];
	}
}

static int DrawLevelOvr1P_ProjectVertex(struct LevVertex *vertices, const struct QuadBlock *block, int vertexIndex, s16 *posScreen)
{
	long gteFlag;

	gte_ldv0(&vertices[block->index[vertexIndex]].pos[0]);
	gte_rtps();
	gte_stsxy(posScreen);
	gte_stflg(&gteFlag);

	return ((u32)gteFlag & DRAW_LEVEL_OVR1P_GTE_RTPT_OVERFLOW) != 0;
}

static struct DrawLevelOvr1PScratchVertex *DrawLevelOvr1P_GetScratchVertices(void)
{
	// NOTE(aalhendi): Retail 4x1 handlers build 20-byte projected vertex records at scratch 0x1f8001b4.
	return CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PScratchVertex, 0x1b4);
}

static s32 DrawLevelOvr1P_GetDepthClipThreshold(void)
{
	return *CTR_SCRATCHPAD_PTR(s32, 0x5c);
}

static u8 *DrawLevelOvr1P_GetClipRecordCursor(void)
{
	return (u8 *)(uintptr_t)*CTR_SCRATCHPAD_PTR(u32, 0x10);
}

static void DrawLevelOvr1P_SetClipRecordCursor(u8 *cursor)
{
	*CTR_SCRATCHPAD_PTR(u32, 0x10) = (u32)(uintptr_t)cursor;
}

static u8 *DrawLevelOvr1P_GetClipRecordStart(void)
{
	return data.PtrClipBuffer[0];
}

static u8 *DrawLevelOvr1P_GetClipRecordEnd(void)
{
	u8 *start = DrawLevelOvr1P_GetClipRecordStart();
	struct GameTracker *gGT = sdata->gGT;

	if (start == NULL || gGT == NULL)
		return start;

	return start + ((size_t)DECOMP_MainDB_GetClipSize(gGT->levelID, gGT->numPlyrCurrGame) << 2);
}

static size_t DrawLevelOvr1P_GetClipRecordSize(int count)
{
	return count == 4 ? DRAW_LEVEL_OVR1P_CLIP_RECORD_GT4_SIZE : DRAW_LEVEL_OVR1P_CLIP_RECORD_GT3_SIZE;
}

static int DrawLevelOvr1P_HasClipRecordSpace(size_t size)
{
	u8 *start = DrawLevelOvr1P_GetClipRecordStart();
	u8 *cursor = DrawLevelOvr1P_GetClipRecordCursor();
	u8 *end = DrawLevelOvr1P_GetClipRecordEnd();

	return start != NULL && cursor != NULL && end != NULL && cursor >= start && cursor <= end && size <= (size_t)(end - cursor);
}

static u8 DrawLevelOvr1P_MipsSubuSignBit(u32 lhs, u32 rhs)
{
	return (u8)((lhs - rhs) >> 31);
}

static void DrawLevelOvr1P_SetProjectedDepth(struct DrawLevelOvr1PScratchVertex *projected, u32 depth, int writeClipBytes)
{
	u32 threshold = (u32)DrawLevelOvr1P_GetDepthClipThreshold();

	projected->depth = (u16)depth;

	if (writeClipBytes)
	{
		// NOTE(aalhendi): Retail stores the `subu` sign-bit result as byte 0/1.
		projected->clipNear = DrawLevelOvr1P_MipsSubuSignBit(depth, threshold);
		projected->clipHalfNear = DrawLevelOvr1P_MipsSubuSignBit(depth << 1, threshold);
	}
	else
	{
		projected->clipNear = 0;
		projected->clipHalfNear = 0;
	}
}

static int DrawLevelOvr1P_ProjectVertexTriple(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected,
                                              int index0, int index1, int index2, int writeClipBytes)
{
	struct LevVertex *vertex0 = &vertices[block->index[index0]];
	struct LevVertex *vertex1 = &vertices[block->index[index1]];
	struct LevVertex *vertex2 = &vertices[block->index[index2]];
	u32 depth0;
	u32 depth1;
	u32 depth2;
	long gteFlag;

	*(u32 *)&projected[index0].pos[0] = *(u32 *)&vertex0->pos[0];
	*(u32 *)&projected[index0].pos[2] = *(u32 *)&vertex0->pos[2];
	*(u32 *)&projected[index0].color_hi[0] = *(u32 *)&vertex0->color_hi[0];
	*(u32 *)&projected[index1].pos[0] = *(u32 *)&vertex1->pos[0];
	*(u32 *)&projected[index1].pos[2] = *(u32 *)&vertex1->pos[2];
	*(u32 *)&projected[index1].color_hi[0] = *(u32 *)&vertex1->color_hi[0];
	*(u32 *)&projected[index2].pos[0] = *(u32 *)&vertex2->pos[0];
	*(u32 *)&projected[index2].pos[2] = *(u32 *)&vertex2->pos[2];
	*(u32 *)&projected[index2].color_hi[0] = *(u32 *)&vertex2->color_hi[0];

	gte_ldv3(&vertex0->pos[0], &vertex1->pos[0], &vertex2->pos[0]);
	gte_rtpt();
	gte_stsxy3(&projected[index0].posScreen[0], &projected[index1].posScreen[0], &projected[index2].posScreen[0]);
	gte_stsz3(&depth0, &depth1, &depth2);
	gte_stflg(&gteFlag);

	DrawLevelOvr1P_SetProjectedDepth(&projected[index0], depth0, writeClipBytes);
	DrawLevelOvr1P_SetProjectedDepth(&projected[index1], depth1, writeClipBytes);
	DrawLevelOvr1P_SetProjectedDepth(&projected[index2], depth2, writeClipBytes);

	return ((u32)gteFlag & DRAW_LEVEL_OVR1P_GTE_RTPT_OVERFLOW) != 0;
}

static void DrawLevelOvr1P_ProjectScratchVertex(struct DrawLevelOvr1PScratchVertex *projected, int writeClipBytes);

static void DrawLevelOvr1P_CopyVertexToProjected(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected,
                                                 int index)
{
	struct LevVertex *vertex = &vertices[block->index[index]];

	*(u32 *)&projected[index].pos[0] = *(u32 *)&vertex->pos[0];
	*(u32 *)&projected[index].pos[2] = *(u32 *)&vertex->pos[2];
	*(u32 *)&projected[index].color_hi[0] = *(u32 *)&vertex->color_hi[0];
}

static void DrawLevelOvr1P_ProjectFullDynamicLowQuad(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected)
{
	// NOTE(aalhendi): Retail 0x800a0ef4 projects the low four before deciding
	// whether the near path needs the full 3x3 transition grid.
	(void)DrawLevelOvr1P_ProjectVertexTriple(vertices, block, projected, 0, 1, 2, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST);
	DrawLevelOvr1P_CopyVertexToProjected(vertices, block, projected, 3);
	DrawLevelOvr1P_ProjectScratchVertex(&projected[3], DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST);
}

static void DrawLevelOvr1P_InitRuntimeScratch(struct PushBuffer *pb, struct PrimMem *primMem)
{
	s16 *data6 = CTR_SCRATCHPAD_PTR(s16, 0x198);
	u32 *projectedScratch = CTR_SCRATCHPAD_PTR(u32, 0x1b4);

	// NOTE(aalhendi): Retail seeds these entry scratch words before bucket dispatch; clipped-record handlers consume the cursor at 0x10.
	*CTR_SCRATCHPAD_PTR(u32, 0x10) = (u32)(uintptr_t)data.PtrClipBuffer[0];
	*CTR_SCRATCHPAD_PTR(u32, 0x30) = (u32)(uintptr_t)primMem->end;
	*CTR_SCRATCHPAD_PTR(u32, 0xdc) = (u32)(uintptr_t)pb;

	// NOTE(aalhendi): Retail stores `(distanceToScreen_PREV >> 1) + 1` at scratch 0x5c for 4x1 depth clip flags.
	*CTR_SCRATCHPAD_PTR(s32, 0x5c) = (pb->distanceToScreen_PREV >> 1) + 1;
	*CTR_SCRATCHPAD_PTR(u32, 0x6c) = (u16)pb->rect.w | ((u32)(u16)pb->rect.h << 16);

	for (int i = 0; i < 3; i++)
		data6[i] = (s16)((u16)(u8)pb->data6[i * 2] | ((u16)(u8)pb->data6[i * 2 + 1] << 8));

	for (int offset = 0; offset < 0x224; offset += (int)sizeof(u32))
		projectedScratch[offset / (int)sizeof(u32)] = 0;
}

static int DrawLevelOvr1P_ProjectQuadBlockGrid(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected,
                                               int writeClipBytes)
{
	int gteOverflow = 0;

	gteOverflow |= DrawLevelOvr1P_ProjectVertexTriple(vertices, block, projected, 0, 1, 2, writeClipBytes);
	gteOverflow |= DrawLevelOvr1P_ProjectVertexTriple(vertices, block, projected, 3, 4, 5, writeClipBytes);
	gteOverflow |= DrawLevelOvr1P_ProjectVertexTriple(vertices, block, projected, 6, 7, 8, writeClipBytes);

	return gteOverflow;
}

static u32 DrawLevelOvr1P_GetProjectedColorWord(const struct DrawLevelOvr1PScratchVertex *projected);

static u32 DrawLevelOvr1P_ApplyWaterRenderedColorFade(u32 color, s16 x, s16 z)
{
	const s16 *center = CTR_SCRATCHPAD_PTR(s16, 0x198);
	s32 deltaX = (s32)x - center[0];
	s32 deltaZ = (s32)z - center[2];
	s32 major;
	s32 minor;
	s32 falloff;
	s32 factor;

	if (deltaX < 0)
		deltaX = -deltaX;

	if (deltaZ < 0)
		deltaZ = -deltaZ;

	major = deltaX;
	minor = deltaZ;
	if (deltaX < deltaZ)
	{
		major = deltaZ;
		minor = deltaX;
	}

	falloff = major + (minor >> 2) - 0x1000;
	if (falloff >= 0)
		return 0;

	factor = (s32)((u32)falloff << 2) + 0x1000;
	// NOTE(aalhendi): Retail 0x800a2d30 fades water-rendered vertex color with DPCS.
	MTC2(color, 6);
	if (factor < 0)
		return color;

	MTC2(factor, 8);
	gte_dpcs();
	return MFC2(22);
}

static void DrawLevelOvr1P_ApplyWaterRenderedColorFades(struct DrawLevelOvr1PScratchVertex *projected)
{
	for (int i = 0; i < 9; i++)
		*(u32 *)&projected[i].color_hi[0] =
		    DrawLevelOvr1P_ApplyWaterRenderedColorFade(DrawLevelOvr1P_GetProjectedColorWord(&projected[i]), projected[i].pos[0], projected[i].pos[2]);
}

static int DrawLevelOvr1P_NclipProjected(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	const struct DrawLevelOvr1PScratchVertex *vertex0 = &projected[indices[0]];
	const struct DrawLevelOvr1PScratchVertex *vertex1 = &projected[indices[1]];
	const struct DrawLevelOvr1PScratchVertex *vertex2 = &projected[indices[2]];
	u32 sxy0;
	u32 sxy1;
	u32 sxy2;
	s32 nclip;

	sxy0 = (u16)vertex0->posScreen[0] | ((u32)(u16)vertex0->posScreen[1] << 16);
	sxy1 = (u16)vertex1->posScreen[0] | ((u32)(u16)vertex1->posScreen[1] << 16);
	sxy2 = (u16)vertex2->posScreen[0] | ((u32)(u16)vertex2->posScreen[1] << 16);

	// NOTE(aalhendi): Retail helper paths use GTE NCLIP/MAC0 for this sign
	// fold; using the GTE also avoids host signed-overflow behavior here.
	MTC2(sxy0, 12);
	MTC2(sxy1, 13);
	MTC2(sxy2, 14);
	gte_nclip();
	gte_stopz(&nclip);

	return nclip;
}

static u32 DrawLevelOvr1P_PackProjectedSxy(const struct DrawLevelOvr1PScratchVertex *projected)
{
	return (u16)projected->posScreen[0] | ((u32)(u16)projected->posScreen[1] << 16);
}

static int DrawLevelOvr1P_IsProjectedPolyOffscreenPacked(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count)
{
	u32 packedWindow = *CTR_SCRATCHPAD_PTR(u32, 0x6c);
	u32 packedAnd = 0xffffffff;
	u32 packedSubOr = 0;
	u32 packedReject;

	for (int i = 0; i < count; i++)
	{
		u32 packedSxy = DrawLevelOvr1P_PackProjectedSxy(&projected[indices[i]]);

		packedAnd &= packedSxy;
		packedSubOr |= packedSxy - packedWindow;
	}

	// NOTE(aalhendi): Retail uses packed SXY sign bits to reject polys that are
	// wholly left/right/top/bottom of the pushbuffer rectangle.
	packedReject = ~packedSubOr | packedAnd;
	return ((s32)packedReject < 0) || ((s32)(packedReject << 16) < 0);
}

static int DrawLevelOvr1P_IsProjectedFaceOffscreen(struct PushBuffer *pb, const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	(void)pb;

	return DrawLevelOvr1P_IsProjectedPolyOffscreenPacked(projected, indices, 4);
}

static int DrawLevelOvr1P_IsProjectedTriOffscreen(struct PushBuffer *pb, const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	(void)pb;

	return DrawLevelOvr1P_IsProjectedPolyOffscreenPacked(projected, indices, 3);
}

static int DrawLevelOvr1P_IsProjectedFaceFullyNear(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	return projected[indices[0]].clipHalfNear && projected[indices[1]].clipHalfNear && projected[indices[2]].clipHalfNear && projected[indices[3]].clipHalfNear;
}

static u32 DrawLevelOvr1P_GetProjectedNearMaskAtScratchOffset(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, u32 scratchOffset)
{
	static const u32 bits[4] = {0x4, 0x8, 0x10, 0x20};
	u32 threshold = *CTR_SCRATCHPAD_PTR(u32, scratchOffset);
	u32 mask = 0;

	for (int i = 0; i < 4; i++)
	{
		if (DrawLevelOvr1P_MipsSubuSignBit(projected[indices[i]].depth, threshold))
			mask |= bits[i];
	}

	return mask;
}

static u32 DrawLevelOvr1P_GetProjectedNearMask(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	return DrawLevelOvr1P_GetProjectedNearMaskAtScratchOffset(projected, indices, 0x24);
}

static u32 DrawLevelOvr1P_GetProjectedRecursiveNearMask(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	// NOTE(aalhendi): Retail helper recursion switches from the top-level
	// 0x24 threshold to scratch 0x28 before selecting the next child handler.
	return DrawLevelOvr1P_GetProjectedNearMaskAtScratchOffset(projected, indices, 0x28);
}

static u32 DrawLevelOvr1P_GetProjectedNearMaskForMode(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int writeClipBytes)
{
	(void)writeClipBytes;

	return DrawLevelOvr1P_GetProjectedNearMask(projected, indices);
}

static u32 DrawLevelOvr1P_GetProjectedNearMaskForDepth(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int depth)
{
	return depth == 0 ? DrawLevelOvr1P_GetProjectedNearMask(projected, indices) : DrawLevelOvr1P_GetProjectedRecursiveNearMask(projected, indices);
}

static u32 DrawLevelOvr1P_GetProjectedCopiedGridNearMask(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	// NOTE(aalhendi): Retail copied grid helper bodies at 0x800a825c and
	// 0x800a9e7c load scratch 0x28 before selecting the next child helper.
	return DrawLevelOvr1P_GetProjectedRecursiveNearMask(projected, indices);
}

static void DrawLevelOvr1P_ProjectScratchVertex(struct DrawLevelOvr1PScratchVertex *projected, int writeClipBytes)
{
	u32 depth;

	gte_ldv0(&projected->pos[0]);
	gte_rtps();
	gte_stsxy(&projected->posScreen[0]);
	gte_stsz(&depth);
	DrawLevelOvr1P_SetProjectedDepth(projected, depth, writeClipBytes);
}

static void DrawLevelOvr1P_BuildMidpointPair(struct DrawLevelOvr1PScratchVertex *dstA, struct DrawLevelOvr1PScratchVertex *dstB,
                                             struct DrawLevelOvr1PScratchVertex *dstMid, const struct DrawLevelOvr1PScratchVertex *srcA,
                                             const struct DrawLevelOvr1PScratchVertex *srcB, int writeClipBytes)
{
	u8 *dstUv;
	const u8 *srcAUv;
	const u8 *srcBUv;

	*dstA = *srcA;
	*dstB = *srcB;
	dstA->color_hi[3] = 0;
	dstB->color_hi[3] = 0;

	for (int i = 0; i < 3; i++)
		dstMid->pos[i] = (s16)(((s32)srcA->pos[i] + (s32)srcB->pos[i]) >> 1);

	dstUv = (u8 *)&dstMid->flags;
	srcAUv = (const u8 *)&srcA->flags;
	srcBUv = (const u8 *)&srcB->flags;

	dstUv[0] = (u8)(((u32)srcAUv[0] + (u32)srcBUv[0]) >> 1);
	dstUv[1] = (u8)(((u32)srcAUv[1] + (u32)srcBUv[1]) >> 1);

	for (int i = 0; i < 3; i++)
		dstMid->color_hi[i] = (u8)(((u32)srcA->color_hi[i] + (u32)srcB->color_hi[i]) >> 1);

	// NOTE(aalhendi): Retail midpoint helpers only write RGB at 0x8..0xa; the
	// copied endpoint and generated midpoint code bytes stay pre-cleared.
	dstMid->color_hi[3] = 0;
	DrawLevelOvr1P_ProjectScratchVertex(dstMid, writeClipBytes);
}

static u32 DrawLevelOvr1P_GetProjectedMaxDepth(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	u32 maxDepth = projected[indices[0]].depth;

	for (int i = 1; i < 4; i++)
	{
		if (maxDepth < projected[indices[i]].depth)
			maxDepth = projected[indices[i]].depth;
	}

	return maxDepth;
}

static u32 DrawLevelOvr1P_GetProjectedTriMaxDepth(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	u32 maxDepth = projected[indices[0]].depth;

	for (int i = 1; i < 3; i++)
	{
		if (maxDepth < projected[indices[i]].depth)
			maxDepth = projected[indices[i]].depth;
	}

	return maxDepth;
}

static int DrawLevelOvr1P_GetProjectedOtIndex(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected, u32 maxDepth, int faceIndex)
{
	s32 otIndex;

	if (faceIndex < 0)
	{
		otIndex = (s32)(maxDepth >> 6) + (s8)(block->draw_order_low & 0xff);
	}
	else
	{
		u32 slotWord = DrawLevelOvr1P_GetProjectedRepresentableSlotWord(projected, faceIndex);
		const s8 *drawOrder = (const s8 *)&block->draw_order_high;

		// NOTE(aalhendi): Retail selector terminals read the OT bias byte from
		// `quad + 0x18 + (frame_slot >> 2)`. Copied recursive/default helpers keep
		// inherited OT state; wide slots stay bounded until that exact state is ported.
		otIndex = (s32)(maxDepth >> 6) + drawOrder[slotWord >> 2];
	}

	if (otIndex < 0)
		return 0;

	if (otIndex > DRAW_LEVEL_OVR1P_MAX_OT_INDEX)
		return DRAW_LEVEL_OVR1P_MAX_OT_INDEX;

	return otIndex;
}

static int DrawLevelOvr1P_ResolveProjectedOtIndex(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected, u32 maxDepth,
                                                  int faceIndex, int otIndexOverride)
{
	if (otIndexOverride >= 0)
		return otIndexOverride;

	return DrawLevelOvr1P_GetProjectedOtIndex(block, projected, maxDepth, faceIndex);
}

static int DrawLevelOvr1P_ShouldSetSemiTransFromTpage(u16 tpage)
{
	// NOTE(aalhendi): Retail 226 direct/clip emit helpers use code 0x36/0x3e
	// unless the tpage blend bits are exactly 0x60.
	return (tpage & 0x60) != 0x60;
}

static void DrawLevelOvr1P_AddRawPrimToOt(u32 *primWords, u32 wordCount, u_long *otEntry)
{
	primWords[0] = ((u32)*otEntry & 0x00ffffff) | (wordCount << 24);
	*otEntry = (u32)(uintptr_t)primWords & 0x00ffffff;
}

static u32 DrawLevelOvr1P_GetProjectedColorCode(const struct DrawLevelOvr1PScratchVertex *projected, u32 code)
{
	return *(const u32 *)&projected->color_hi[0] | (code << 24);
}

static u32 DrawLevelOvr1P_GetProjectedColorWord(const struct DrawLevelOvr1PScratchVertex *projected)
{
	return *(const u32 *)&projected->color_hi[0];
}

static u32 DrawLevelOvr1P_SelectRawPrimitiveCode(u32 uv1Word, u32 semiTransCode, u32 opaqueCode)
{
	return ((uv1Word & 0x00600000) != 0x00600000) ? semiTransCode : opaqueCode;
}

static int DrawLevelOvr1P_EmitPreparedProjectedQuadRawAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                           const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                           const struct TextureLayout *texture, u32 maxDepth, int otIndexOverride)
{
	u32 *prim;
	void *nextPrim;
	int otIndex;
	u32 code;
	u32 uv0 = 0;
	u32 uv1 = 0;
	u32 uv2 = 0;

	prim = primMem->curr;
	nextPrim = (u8 *)prim + sizeof(POLY_GT4);

	DrawLevelOvr1P_StoreProjectedDirectUvScratch(projected, indices, 4);
	uv0 = *CTR_SCRATCHPAD_PTR(u32, 0x1a0);
	uv1 = *CTR_SCRATCHPAD_PTR(u32, 0x1a4);
	uv2 = *CTR_SCRATCHPAD_PTR(u32, 0x1a8);

	otIndex = DrawLevelOvr1P_ResolveProjectedOtIndex(block, projected, maxDepth, faceIndex, otIndexOverride);
	code = texture != NULL ? DrawLevelOvr1P_SelectRawPrimitiveCode(uv1, 0x3e, 0x3c) : 0x3e;

	prim[1] = DrawLevelOvr1P_GetProjectedColorCode(&projected[indices[0]], code);
	prim[2] = DrawLevelOvr1P_PackProjectedSxy(&projected[indices[0]]);
	prim[3] = uv0;
	prim[4] = DrawLevelOvr1P_GetProjectedColorCode(&projected[indices[1]], 0);
	prim[5] = DrawLevelOvr1P_PackProjectedSxy(&projected[indices[1]]);
	prim[6] = uv1;
	prim[7] = DrawLevelOvr1P_GetProjectedColorCode(&projected[indices[2]], 0);
	prim[8] = DrawLevelOvr1P_PackProjectedSxy(&projected[indices[2]]);
	prim[9] = uv2;
	prim[10] = DrawLevelOvr1P_GetProjectedColorCode(&projected[indices[3]], 0);
	prim[11] = DrawLevelOvr1P_PackProjectedSxy(&projected[indices[3]]);
	prim[12] = uv2 >> 16;
	DrawLevelOvr1P_AddRawPrimToOt(prim, 12, &pb->ptrOT[otIndex]);
	primMem->curr = nextPrim;
	return 1;
}

static int DrawLevelOvr1P_EmitPreparedProjectedQuadRaw(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                       const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                       const struct TextureLayout *texture, u32 maxDepth)
{
	return DrawLevelOvr1P_EmitPreparedProjectedQuadRawAtOt(pb, primMem, block, projected, indices, faceIndex, texture, maxDepth,
	                                                       DRAW_LEVEL_OVR1P_OT_INDEX_AUTO);
}

static int DrawLevelOvr1P_EmitPreparedProjectedTriRawAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                          const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                          const struct TextureLayout *texture, u32 maxDepth, int otIndexOverride)
{
	u32 *prim;
	void *nextPrim;
	int otIndex;
	u32 code;
	u32 uv0 = 0;
	u32 uv1 = 0;
	u32 uv2 = 0;

	prim = primMem->curr;
	nextPrim = (u8 *)prim + sizeof(POLY_GT3);

	DrawLevelOvr1P_StoreProjectedDirectUvScratch(projected, indices, 3);
	uv0 = *CTR_SCRATCHPAD_PTR(u32, 0x1a0);
	uv1 = *CTR_SCRATCHPAD_PTR(u32, 0x1a4);
	uv2 = *CTR_SCRATCHPAD_PTR(u32, 0x1a8);

	otIndex = DrawLevelOvr1P_ResolveProjectedOtIndex(block, projected, maxDepth, faceIndex, otIndexOverride);
	code = texture != NULL ? DrawLevelOvr1P_SelectRawPrimitiveCode(uv1, 0x36, 0x34) : 0x36;

	prim[1] = DrawLevelOvr1P_GetProjectedColorCode(&projected[indices[0]], code);
	prim[2] = DrawLevelOvr1P_PackProjectedSxy(&projected[indices[0]]);
	prim[3] = uv0;
	prim[4] = DrawLevelOvr1P_GetProjectedColorCode(&projected[indices[1]], 0);
	prim[5] = DrawLevelOvr1P_PackProjectedSxy(&projected[indices[1]]);
	prim[6] = uv1;
	prim[7] = DrawLevelOvr1P_GetProjectedColorCode(&projected[indices[2]], 0);
	prim[8] = DrawLevelOvr1P_PackProjectedSxy(&projected[indices[2]]);
	prim[9] = uv2;
	DrawLevelOvr1P_AddRawPrimToOt(prim, 9, &pb->ptrOT[otIndex]);
	primMem->curr = nextPrim;
	return 1;
}

static int DrawLevelOvr1P_EmitPreparedProjectedTriRaw(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                      const struct TextureLayout *texture, u32 maxDepth)
{
	return DrawLevelOvr1P_EmitPreparedProjectedTriRawAtOt(pb, primMem, block, projected, indices, faceIndex, texture, maxDepth, DRAW_LEVEL_OVR1P_OT_INDEX_AUTO);
}

static int DrawLevelOvr1P_IsClipByteSet(u8 clipByte)
{
	return clipByte != 0;
}

static int DrawLevelOvr1P_AreProjectedVerticesHalfNear(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count)
{
	for (int i = 0; i < count; i++)
	{
		if (!DrawLevelOvr1P_IsClipByteSet(projected[indices[i]].clipHalfNear))
			return 0;
	}

	return 1;
}

static int DrawLevelOvr1P_HasProjectedVertexNear(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count)
{
	for (int i = 0; i < count; i++)
	{
		if (DrawLevelOvr1P_IsClipByteSet(projected[indices[i]].clipNear))
			return 1;
	}

	return 0;
}

static s32 DrawLevelOvr1P_MulShift16(s32 delta, s32 factor)
{
	u32 product = (u32)((s64)delta * factor);

	return ((s32)product) >> 16;
}

static s16 DrawLevelOvr1P_LerpS16_16(s16 a, s16 b, s32 factor)
{
	return (s16)((s32)a + DrawLevelOvr1P_MulShift16((s32)b - (s32)a, factor));
}

static u8 DrawLevelOvr1P_LerpU8_16(u8 a, u8 b, s32 factor)
{
	return (u8)((s32)a + DrawLevelOvr1P_MulShift16((s32)b - (s32)a, factor));
}

static s32 DrawLevelOvr1P_ShiftLeft16Wrap(s32 value)
{
	return (s32)((u32)value << 16);
}

static s16 DrawLevelOvr1P_ShiftLeft1S16Wrap(s16 value)
{
	return (s16)((u32)(s32)value << 1);
}

static u32 DrawLevelOvr1P_ReadWord(const void *base, u32 offset)
{
	return *(const u32 *)((const u8 *)base + offset);
}

static void DrawLevelOvr1P_AdjustFullDynamicMidVertex(struct DrawLevelOvr1PScratchVertex *projected, struct LevVertex *vertices, const struct QuadBlock *block,
                                                      int midIndex, int endpointAIndex, int endpointBIndex)
{
	struct DrawLevelOvr1PScratchVertex *mid = &projected[midIndex];
	const struct DrawLevelOvr1PScratchVertex *endpointA = &projected[endpointAIndex];
	const struct DrawLevelOvr1PScratchVertex *endpointB = &projected[endpointBIndex];
	const struct LevVertex *levMid = &vertices[block->index[midIndex]];
	s16 midpoint[3];
	s32 factor;
	u32 color;
	u32 depth;

	factor = (s32)(((u32)mid->depth - *CTR_SCRATCHPAD_PTR(u32, 0x2c)) << 2);
	if (factor < 0)
		return;

	for (int i = 0; i < 3; i++)
		midpoint[i] = (s16)(((s32)endpointA->pos[i] + (s32)endpointB->pos[i]) >> 1);

	color = DrawLevelOvr1P_ReadWord(levMid, 0xc);

	if (factor < 0x1000)
	{
		u32 farColor = color << 4;

		// NOTE(aalhendi): Retail full-dynamic helper 0x800a1408 uses DPCS/GPL12
		// to fade the projected midpoint toward the endpoint midpoint.
		MTC2(factor, 8);
		MTC2(DrawLevelOvr1P_ReadWord(mid, 0x8), 6);
		CTC2((farColor >> 16) & 0xff0, 23);
		CTC2((farColor >> 8) & 0xff0, 22);
		CTC2(farColor & 0xff0, 21);
		gte_dpcs();
		color = MFC2(22);

		MTC2((u32)(s32)mid->pos[0], 25);
		MTC2((u32)(s32)(midpoint[0] - mid->pos[0]), 9);
		MTC2((u32)(s32)mid->pos[1], 26);
		MTC2((u32)(s32)(midpoint[1] - mid->pos[1]), 10);
		MTC2((u32)(s32)mid->pos[2], 27);
		MTC2((u32)(s32)(midpoint[2] - mid->pos[2]), 11);
		gte_gpl12();

		midpoint[0] = (s16)MFC2_S(25);
		midpoint[1] = (s16)MFC2_S(26);
		midpoint[2] = (s16)MFC2_S(27);
	}

	mid->pos[0] = midpoint[0];
	mid->pos[1] = midpoint[1];
	mid->pos[2] = midpoint[2];
	*(u32 *)&mid->color_hi[0] = color;

	MTC2(DrawLevelOvr1P_ReadWord(&mid->pos[0], 0), 0);
	MTC2((u32)(s32)mid->pos[2], 1);
	gte_rtps();
	gte_stsxy(&mid->posScreen[0]);
	gte_stsz(&depth);
	mid->depth = (u16)depth;
	mid->clipNear = 0;
	mid->clipHalfNear = 0;
}

static void DrawLevelOvr1P_AdjustFullDynamicMidVertices(struct DrawLevelOvr1PScratchVertex *projected, struct LevVertex *vertices,
                                                        const struct QuadBlock *block)
{
	DrawLevelOvr1P_AdjustFullDynamicMidVertex(projected, vertices, block, 4, 0, 1);
	DrawLevelOvr1P_AdjustFullDynamicMidVertex(projected, vertices, block, 5, 0, 2);
	DrawLevelOvr1P_AdjustFullDynamicMidVertex(projected, vertices, block, 6, 1, 2);
	DrawLevelOvr1P_AdjustFullDynamicMidVertex(projected, vertices, block, 7, 1, 3);
	DrawLevelOvr1P_AdjustFullDynamicMidVertex(projected, vertices, block, 8, 2, 3);
}

static u32 DrawLevelOvr1P_GetPreparedProjectedMaxDepthCount(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count)
{
	u32 maxDepth = projected[indices[0]].depth;

	for (int i = 1; i < count; i++)
	{
		if (maxDepth < projected[indices[i]].depth)
			maxDepth = projected[indices[i]].depth;
	}

	return maxDepth;
}

static int DrawLevelOvr1P_SourceInsideClipRecordWindow(const struct DrawLevelOvr1PScratchVertex *src)
{
	const s16 *center = CTR_SCRATCHPAD_PTR(s16, 0x198);
	s32 x = (s32)src->pos[0] - center[0];
	s32 y = (s32)src->pos[1] - center[1];
	s32 z = (s32)src->pos[2] - center[2];
	u32 insideBits;

	if (x < 0)
		x = -x;

	if (y < 0)
		y = -y;

	if (z < 0)
		z = -z;

	// NOTE(aalhendi): Retail 0x800a4f38 uses the sign bit of these three
	// extent checks ANDed together; negative means the source point is inside.
	insideBits = (u32)(x - 0x100) & (u32)(y - 0x180) & (u32)(z - 0x100);
	return (s32)insideBits < 0;
}

static int DrawLevelOvr1P_ShouldWriteRenderedClippedRecord(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count)
{
	for (int i = 0; i < count; i++)
	{
		if (DrawLevelOvr1P_SourceInsideClipRecordWindow(&projected[indices[i]]))
			return 1;
	}

	return 0;
}

static void DrawLevelOvr1P_CopyClipRecordVertex(struct DrawLevelOvr1PClipRecordVertex *dst, const struct DrawLevelOvr1PScratchVertex *src)
{
	*(u32 *)&dst->pos[0] = *(const u32 *)&src->pos[0];
	*(u32 *)&dst->pos[2] = *(const u32 *)&src->pos[2];
	*(u32 *)&dst->color_hi[0] = *(const u32 *)&src->color_hi[0];

	// NOTE(aalhendi): Retail 0x800a4f78 ORs scratch byte 0x13 into the
	// record color/code word; the 0x800aa790 consumer uses it for near masks.
	dst->color_hi[3] |= src->clipHalfNear;
}

static void DrawLevelOvr1P_StoreRenderedClipRecordHeader(u32 tableWord)
{
	// NOTE(aalhendi): Retail rendered helpers 0x800a84dc/0x800aa0fc store
	// clipped-record polarity at scratch 0x80 before helper-local dispatch.
	*CTR_SCRATCHPAD_PTR(u32, 0x80) = (DrawLevelOvr1P_GetActiveDrawOrderLow() & 0x80000000u) | ((tableWord & 0x80000000u) >> 1);
}

static u32 DrawLevelOvr1P_GetRenderedClipRecordHeader(const struct QuadBlock *block, int count)
{
	u32 header = *CTR_SCRATCHPAD_PTR(u32, 0x80);

	(void)block;

	// NOTE(aalhendi): Retail 0x800a4dcc/0x800a4e70 ORs the GT4 count bit into
	// the stored scratch 0x80 polarity word when writing clipped records.
	if (count == 4)
		header |= 1;

	return header;
}

static int DrawLevelOvr1P_WriteRenderedClippedRecordAtOt(struct PushBuffer *pb, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count, int faceIndex,
                                                         const struct TextureLayout *texture, int otIndexOverride)
{
	struct DrawLevelOvr1PClipRecord *record;
	u8 *cursor = DrawLevelOvr1P_GetClipRecordCursor();
	size_t recordSize = DrawLevelOvr1P_GetClipRecordSize(count);
	u32 maxDepth;
	int otIndex;

	if (!DrawLevelOvr1P_ShouldWriteRenderedClippedRecord(projected, indices, count))
		return 1;

	if (!DrawLevelOvr1P_HasClipRecordSpace(recordSize))
		return 1;

	maxDepth = DrawLevelOvr1P_GetPreparedProjectedMaxDepthCount(projected, indices, count);
	otIndex = DrawLevelOvr1P_ResolveProjectedOtIndex(block, projected, maxDepth, faceIndex, otIndexOverride);

	record = (struct DrawLevelOvr1PClipRecord *)cursor;
	record->header = DrawLevelOvr1P_GetRenderedClipRecordHeader(block, count);
	record->otEntry = (u32)(uintptr_t)&pb->ptrOT[otIndex];
	record->tpage = texture != NULL ? texture->tpage : 0;
	record->clut = texture != NULL ? texture->clut : 0;

	for (int i = 0; i < count; i++)
		DrawLevelOvr1P_CopyClipRecordVertex(&record->vertex[i], &projected[indices[i]]);

	DrawLevelOvr1P_SetClipRecordCursor(cursor + recordSize);
	return 1;
}

static int DrawLevelOvr1P_WriteRenderedClippedRecord(struct PushBuffer *pb, const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected,
                                                     const int *indices, int count, int faceIndex, const struct TextureLayout *texture)
{
	return DrawLevelOvr1P_WriteRenderedClippedRecordAtOt(pb, block, projected, indices, count, faceIndex, texture, DRAW_LEVEL_OVR1P_OT_INDEX_AUTO);
}

static int DrawLevelOvr1P_WriteWaterRenderedClippedRecord(struct PushBuffer *pb, const struct QuadBlock *block,
                                                          const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count, int faceIndex)
{
	struct DrawLevelOvr1PClipRecord *record;
	u8 *cursor = DrawLevelOvr1P_GetClipRecordCursor();
	size_t recordSize = DrawLevelOvr1P_GetClipRecordSize(count);
	u32 maxDepth;
	int otIndex;

	if (!DrawLevelOvr1P_ShouldWriteRenderedClippedRecord(projected, indices, count))
		return 1;

	if (!DrawLevelOvr1P_HasClipRecordSpace(recordSize))
		return 1;

	maxDepth = DrawLevelOvr1P_GetPreparedProjectedMaxDepthCount(projected, indices, count);
	otIndex = DrawLevelOvr1P_GetProjectedOtIndex(block, projected, maxDepth, faceIndex);

	record = (struct DrawLevelOvr1PClipRecord *)cursor;
	// NOTE(aalhendi): Retail water direct helpers 0x800a34d4/0x800a3578 set
	// bit 31 on clipped-record headers so the consumer keeps the NCLIP result.
	record->header = count == 4 ? 0x80000001u : 0x80000000u;
	record->otEntry = (u32)(uintptr_t)&pb->ptrOT[otIndex];
	record->tpage = *CTR_SCRATCHPAD_PTR(s16, 0x1a6);
	record->clut = *CTR_SCRATCHPAD_PTR(s16, 0x1a2);

	for (int i = 0; i < count; i++)
		DrawLevelOvr1P_CopyClipRecordVertex(&record->vertex[i], &projected[indices[i]]);

	DrawLevelOvr1P_SetClipRecordCursor(cursor + recordSize);
	return 1;
}

static int DrawLevelOvr1P_EmitRenderedClippedPolygonBridgeAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count,
                                                               int faceIndex, const struct TextureLayout *texture, int otIndexOverride)
{
	(void)primMem;

	// TODO(aalhendi): Replace the native consumer below with retail 0x800aa790
	// and its jump-table/GTE emit path. This side now writes retail-sized raw
	// records matching 0x800a4dcc/0x800a4e70.
	return DrawLevelOvr1P_WriteRenderedClippedRecordAtOt(pb, block, projected, indices, count, faceIndex, texture, otIndexOverride);
}

static int DrawLevelOvr1P_EmitRenderedClippedPolygonBridge(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                           const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count, int faceIndex,
                                                           const struct TextureLayout *texture)
{
	return DrawLevelOvr1P_EmitRenderedClippedPolygonBridgeAtOt(pb, primMem, block, projected, indices, count, faceIndex, texture,
	                                                           DRAW_LEVEL_OVR1P_OT_INDEX_AUTO);
}

static int DrawLevelOvr1P_EmitWaterRenderedClippedPolygonBridge(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count,
                                                                int faceIndex)
{
	(void)primMem;

	return DrawLevelOvr1P_WriteWaterRenderedClippedRecord(pb, block, projected, indices, count, faceIndex);
}

static int DrawLevelOvr1P_ShouldEmitClipRecordNclip(s32 nclip, const struct DrawLevelOvr1PClipRecord *record)
{
	s32 header = (s32)record->header;

	if (nclip == 0)
		return 0;

	if (header < 0)
		return 1;

	return (s32)((u32)nclip ^ ((u32)header << 1)) > 0;
}

static int DrawLevelOvr1P_ShouldSetClipRecordSemiTrans(const struct DrawLevelOvr1PClipRecord *record)
{
	return DrawLevelOvr1P_ShouldSetSemiTransFromTpage(record->tpage);
}

static u32 DrawLevelOvr1P_GetClipRecordColorCode(const struct DrawLevelOvr1PScratchVertex *projected, u32 code)
{
	return (*(const u32 *)&projected->color_hi[0] & 0x00ffffff) | (code << 24);
}

static u32 DrawLevelOvr1P_GetClipRecordUvWord(const struct DrawLevelOvr1PScratchVertex *projected, u16 page)
{
	return (u16)projected->flags | ((u32)page << 16);
}

static u32 DrawLevelOvr1P_GetClipRecordSignedUvWord(const struct DrawLevelOvr1PScratchVertex *projected)
{
	return (u32)(s32)(s16)projected->flags;
}

static void DrawLevelOvr1P_SetClipRecordPageScratch(const struct DrawLevelOvr1PClipRecord *record)
{
	*CTR_SCRATCHPAD_PTR(s16, 0x1a6) = record->tpage;
	*CTR_SCRATCHPAD_PTR(s16, 0x1a2) = record->clut;
}

static struct DrawLevelOvr1PScratchVertex *DrawLevelOvr1P_GetClipRecordWorkspace(void)
{
	return CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PScratchVertex, 0x1b4);
}

static u32 DrawLevelOvr1P_StoreClipRecordUvScratch(const struct DrawLevelOvr1PScratchVertex *projected, int scratchOffset)
{
	*CTR_SCRATCHPAD_PTR(s16, scratchOffset) = (s16)projected->flags;
	return *CTR_SCRATCHPAD_PTR(u32, scratchOffset);
}

static void DrawLevelOvr1P_SetClipRecordSourceDelta(struct DrawLevelOvr1PScratchVertex *projected, s16 delta)
{
	*(s16 *)&projected->clipNear = delta;
}

static s16 DrawLevelOvr1P_GetClipRecordSourceDelta(const struct DrawLevelOvr1PScratchVertex *projected)
{
	return *(const s16 *)&projected->clipNear;
}

static void DrawLevelOvr1P_GetClipRecordSourceVector(const struct DrawLevelOvr1PScratchVertex *projected, SVECTOR *source)
{
	if (DrawLevelOvr1P_GetClipRecordSourceDelta(projected) < 0)
	{
		source->vx = projected->posScreen[0];
		source->vy = projected->posScreen[1];
		source->vz = (s16)projected->depth;
	}
	else
	{
		source->vx = projected->pos[0];
		source->vy = projected->pos[1];
		source->vz = projected->pos[2];
	}

	source->pad = 0;
}

static s32 DrawLevelOvr1P_ProjectClipRecordEmitVertices(struct DrawLevelOvr1PScratchVertex *dst, const struct DrawLevelOvr1PScratchVertex *projected,
                                                        const int *indices, int count)
{
	SVECTOR source[4];
	s32 nclip;

	for (int i = 0; i < count; i++)
	{
		dst[i] = projected[indices[i]];
		DrawLevelOvr1P_GetClipRecordSourceVector(&projected[indices[i]], &source[i]);
	}

	gte_ldv3(&source[0], &source[1], &source[2]);
	gte_rtpt();
	gte_stsxy3(&dst[0].posScreen[0], &dst[1].posScreen[0], &dst[2].posScreen[0]);
	gte_nclip();
	gte_stopz(&nclip);

	if (count == 4)
	{
		gte_ldv0(&source[3]);
		gte_rtps();
		gte_stsxy(&dst[3].posScreen[0]);
	}

	return nclip;
}

static void DrawLevelOvr1P_ProjectClipRecordEmitVertex(struct DrawLevelOvr1PScratchVertex *dst, const struct DrawLevelOvr1PScratchVertex *projected)
{
	SVECTOR source;

	*dst = *projected;
	DrawLevelOvr1P_GetClipRecordSourceVector(projected, &source);

	gte_ldv0(&source);
	gte_rtps();
	gte_stsxy(&dst->posScreen[0]);
}

static int DrawLevelOvr1P_EmitClipRecordTri(struct PushBuffer *pb, struct PrimMem *primMem, u_long *otEntry,
                                            const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                            const struct DrawLevelOvr1PClipRecord *record);

static int DrawLevelOvr1P_EmitClipRecordQuad(struct PushBuffer *pb, struct PrimMem *primMem, u_long *otEntry,
                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                             const struct DrawLevelOvr1PClipRecord *record)
{
	struct DrawLevelOvr1PScratchVertex emit[4];
	int emitIndices[4] = {0, 1, 2, 3};
	u32 *prim;
	void *nextPrim;
	int fallbackIndices[3] = {indices[1], indices[3], indices[2]};
	s32 nclip;
	u32 code;
	u32 uv0;
	u32 uv1;
	u32 uv2;

	// NOTE(aalhendi): Retail 0x800aad44 projects the fourth vertex only after
	// the first triangle survives the NCLIP/header fold.
	nclip = DrawLevelOvr1P_ProjectClipRecordEmitVertices(emit, projected, indices, 3);

	// NOTE(aalhendi): Retail GT4 helper 0x800aad44 falls back to the GT3 helper
	// with vertices 1/3/2 when the first triangle fails its NCLIP fold.
	if (!DrawLevelOvr1P_ShouldEmitClipRecordNclip(nclip, record))
		return DrawLevelOvr1P_EmitClipRecordTri(pb, primMem, otEntry, projected, fallbackIndices, record);

	DrawLevelOvr1P_ProjectClipRecordEmitVertex(&emit[3], &projected[indices[3]]);

	if (DrawLevelOvr1P_IsProjectedFaceOffscreen(pb, emit, emitIndices))
		return 1;

	// NOTE(aalhendi): Retail relies on the 0x800aa848 per-record 0xd68 prim reserve.
	prim = primMem->curr;
	nextPrim = (u8 *)prim + sizeof(POLY_GT4);
	code = DrawLevelOvr1P_ShouldSetClipRecordSemiTrans(record) ? 0x3e : 0x3c;
	uv0 = DrawLevelOvr1P_StoreClipRecordUvScratch(&emit[0], 0x1a0);
	uv1 = DrawLevelOvr1P_StoreClipRecordUvScratch(&emit[1], 0x1a4);
	uv2 = DrawLevelOvr1P_StoreClipRecordUvScratch(&emit[2], 0x1a8);

	prim[1] = DrawLevelOvr1P_GetClipRecordColorCode(&emit[0], code);
	prim[2] = DrawLevelOvr1P_PackProjectedSxy(&emit[0]);
	prim[3] = uv0;
	prim[4] = DrawLevelOvr1P_GetClipRecordColorCode(&emit[1], 0);
	prim[5] = DrawLevelOvr1P_PackProjectedSxy(&emit[1]);
	prim[6] = uv1;
	prim[7] = DrawLevelOvr1P_GetClipRecordColorCode(&emit[2], 0);
	prim[8] = DrawLevelOvr1P_PackProjectedSxy(&emit[2]);
	prim[9] = uv2;
	prim[10] = DrawLevelOvr1P_GetClipRecordColorCode(&emit[3], 0);
	prim[11] = DrawLevelOvr1P_PackProjectedSxy(&emit[3]);
	prim[12] = DrawLevelOvr1P_GetClipRecordSignedUvWord(&emit[3]);
	DrawLevelOvr1P_AddRawPrimToOt(prim, 12, otEntry);
	primMem->curr = nextPrim;
	return 1;
}

static int DrawLevelOvr1P_EmitClipRecordTri(struct PushBuffer *pb, struct PrimMem *primMem, u_long *otEntry,
                                            const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                            const struct DrawLevelOvr1PClipRecord *record)
{
	struct DrawLevelOvr1PScratchVertex emit[3];
	int emitIndices[3] = {0, 1, 2};
	u32 *prim;
	void *nextPrim;
	s32 nclip;
	u32 code;
	u32 uv0;
	u32 uv1;
	u32 uv2;

	nclip = DrawLevelOvr1P_ProjectClipRecordEmitVertices(emit, projected, indices, 3);

	if (!DrawLevelOvr1P_ShouldEmitClipRecordNclip(nclip, record))
		return 1;

	if (DrawLevelOvr1P_IsProjectedTriOffscreen(pb, emit, emitIndices))
		return 1;

	// NOTE(aalhendi): Retail relies on the 0x800aa848 per-record 0xd68 prim reserve.
	prim = primMem->curr;
	nextPrim = (u8 *)prim + sizeof(POLY_GT3);
	code = DrawLevelOvr1P_ShouldSetClipRecordSemiTrans(record) ? 0x36 : 0x34;
	uv0 = DrawLevelOvr1P_StoreClipRecordUvScratch(&emit[0], 0x1a0);
	uv1 = DrawLevelOvr1P_StoreClipRecordUvScratch(&emit[1], 0x1a4);
	uv2 = DrawLevelOvr1P_StoreClipRecordUvScratch(&emit[2], 0x1a8);

	prim[1] = DrawLevelOvr1P_GetClipRecordColorCode(&emit[0], code);
	prim[2] = DrawLevelOvr1P_PackProjectedSxy(&emit[0]);
	prim[3] = uv0;
	prim[4] = DrawLevelOvr1P_GetClipRecordColorCode(&emit[1], 0);
	prim[5] = DrawLevelOvr1P_PackProjectedSxy(&emit[1]);
	prim[6] = uv1;
	prim[7] = DrawLevelOvr1P_GetClipRecordColorCode(&emit[2], 0);
	prim[8] = DrawLevelOvr1P_PackProjectedSxy(&emit[2]);
	prim[9] = uv2;
	DrawLevelOvr1P_AddRawPrimToOt(prim, 9, otEntry);
	primMem->curr = nextPrim;
	return 1;
}

static void DrawLevelOvr1P_StoreCurrentIrVector(s16 *out)
{
#ifdef REBUILD_PC
	out[0] = (s16)MFC2_S(9);
	out[1] = (s16)MFC2_S(10);
	out[2] = (s16)MFC2_S(11);
#else
	s32 ir0;
	s32 ir1;
	s32 ir2;

	__asm__ volatile("mfc2 %0,$9\n"
	                 "mfc2 %1,$10\n"
	                 "mfc2 %2,$11\n"
	                 : "=r"(ir0), "=r"(ir1), "=r"(ir2));

	out[0] = (s16)ir0;
	out[1] = (s16)ir1;
	out[2] = (s16)ir2;
#endif
}

static void DrawLevelOvr1P_ProjectClipRecordVertex(struct DrawLevelOvr1PScratchVertex *projected, const struct DrawLevelOvr1PClipRecordVertex *src)
{
	s16 ir[3];
	s32 threshold = DrawLevelOvr1P_GetDepthClipThreshold();

	*(u32 *)&projected->pos[0] = *(const u32 *)&src->pos[0];
	*(u32 *)&projected->pos[2] = *(const u32 *)&src->pos[2];
	*(u32 *)&projected->color_hi[0] = *(const u32 *)&src->color_hi[0];

	gte_ldv0(&projected->pos[0]);
	// NOTE(aalhendi): Retail 0x800aa86c/0x800aa8b0/0x800aa8e8/0x800aaee4 uses LLV0BK for this source-vector transform.
	gte_llv0bk_b();
	DrawLevelOvr1P_StoreCurrentIrVector(ir);

	projected->pos[0] = ir[0];
	projected->pos[1] = ir[1];
	projected->pos[2] = ir[2];
	// NOTE(aalhendi): Retail 0x800aaad0 uses MIPS `sll 1` for the doubled IR
	// scratch vector; keep wrap semantics instead of signed C shift behavior.
	projected->posScreen[0] = DrawLevelOvr1P_ShiftLeft1S16Wrap(ir[0]);
	projected->posScreen[1] = DrawLevelOvr1P_ShiftLeft1S16Wrap(ir[1]);
	projected->depth = (u16)DrawLevelOvr1P_ShiftLeft1S16Wrap(ir[2]);
	DrawLevelOvr1P_SetClipRecordSourceDelta(projected, (s16)((s32)ir[2] - threshold));
}

static u32 DrawLevelOvr1P_GetClipRecordProjectedNearMask(const struct DrawLevelOvr1PScratchVertex *projected, int count)
{
	static const u32 bits[4] = {0x4, 0x8, 0x10, 0x20};
	u32 mask = 0;

	for (int i = 0; i < count; i++)
	{
		if (DrawLevelOvr1P_IsClipByteSet(projected[i].color_hi[3]))
			mask |= bits[i];
	}

	return mask;
}

static void DrawLevelOvr1P_ClearClipRecordProjectedNearBytes(struct DrawLevelOvr1PScratchVertex *projected, int count)
{
	for (int i = 0; i < count; i++)
		projected[i].color_hi[3] = 0;
}

static void DrawLevelOvr1P_InterpolateClipRecordVertex(struct DrawLevelOvr1PScratchVertex *dst, const struct DrawLevelOvr1PScratchVertex *inside,
                                                       const struct DrawLevelOvr1PScratchVertex *outside)
{
	const u8 *insideUv = (const u8 *)&inside->flags;
	const u8 *outsideUv = (const u8 *)&outside->flags;
	u8 *dstUv = (u8 *)&dst->flags;
	s32 threshold = DrawLevelOvr1P_GetDepthClipThreshold();
	s32 denom = (s16)outside->depth - (s16)inside->depth;
	s32 factor = 0;

	if (denom != 0)
		factor = DrawLevelOvr1P_ShiftLeft16Wrap(threshold - (s32)(s16)inside->depth) / denom;

	// NOTE(aalhendi): Retail 0x800aab00 clips in doubled-IR space with an
	// unclamped 16.16 factor, then writes a new scratch source vector.
	dst->pos[0] = DrawLevelOvr1P_LerpS16_16(inside->posScreen[0], outside->posScreen[0], factor);
	dst->pos[1] = DrawLevelOvr1P_LerpS16_16(inside->posScreen[1], outside->posScreen[1], factor);
	dst->pos[2] = (s16)threshold;
	dst->posScreen[0] = dst->pos[0];
	dst->posScreen[1] = dst->pos[1];
	dst->depth = (u16)(s16)threshold;
	dstUv[0] = DrawLevelOvr1P_LerpU8_16(insideUv[0], outsideUv[0], factor);
	dstUv[1] = DrawLevelOvr1P_LerpU8_16(insideUv[1], outsideUv[1], factor);

	for (int i = 0; i < 3; i++)
		dst->color_hi[i] = DrawLevelOvr1P_LerpU8_16(inside->color_hi[i], outside->color_hi[i], factor);

	dst->color_hi[3] = 0;
	DrawLevelOvr1P_SetClipRecordSourceDelta(dst, 0);
}

static u32 DrawLevelOvr1P_GetClipRecordJumpAddress(int count, u32 nearMask)
{
	u32 tableOffset = count == 4 ? 0x260 : 0x240;

	return *CTR_SCRATCHPAD_PTR(u32, tableOffset + (int)nearMask);
}

static int DrawLevelOvr1P_EmitClipRecordGT3Table(struct PushBuffer *pb, struct PrimMem *primMem, u_long *otEntry,
                                                 const struct DrawLevelOvr1PScratchVertex *projected, const struct DrawLevelOvr1PClipRecord *record)
{
	struct DrawLevelOvr1PScratchVertex *work = DrawLevelOvr1P_GetClipRecordWorkspace();
	int indices[4];
	u32 handlerAddress;
	u32 nearMask;

	if (work != projected)
	{
		work[0] = projected[0];
		work[1] = projected[1];
		work[2] = projected[2];
	}

	// NOTE(aalhendi): Retail 0x800aa934..0x800aa968 reads the projected
	// scratch near bytes to select the GT3 table, then clears those bytes.
	nearMask = DrawLevelOvr1P_GetClipRecordProjectedNearMask(work, 3);
	DrawLevelOvr1P_ClearClipRecordProjectedNearBytes(work, 3);
	handlerAddress = DrawLevelOvr1P_GetClipRecordJumpAddress(3, nearMask);

	// NOTE(aalhendi): Retail dispatches through scratch 0x240; native keeps the handler bodies as C cases keyed by the copied retail addresses.
	//  Work records live at retail scratch 0x1b4/0x1c8/0x1dc/0x204/0x218.
	switch (handlerAddress)
	{
	case 0x800aa96c:
	{
		int tri[3] = {0, 1, 2};
		return DrawLevelOvr1P_EmitClipRecordTri(pb, primMem, otEntry, work, tri, record);
	}
	case 0x800aa984:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[1], &work[0]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[2], &work[0]);
		indices[0] = 1;
		indices[1] = 2;
		indices[2] = 4;
		indices[3] = 5;
		return DrawLevelOvr1P_EmitClipRecordQuad(pb, primMem, otEntry, work, indices, record);
	case 0x800aa9bc:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[2], &work[1]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[0], &work[1]);
		indices[0] = 2;
		indices[1] = 0;
		indices[2] = 4;
		indices[3] = 5;
		return DrawLevelOvr1P_EmitClipRecordQuad(pb, primMem, otEntry, work, indices, record);
	case 0x800aa9f4:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[2], &work[1]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[2], &work[0]);
		indices[0] = 2;
		indices[1] = 5;
		indices[2] = 4;
		return DrawLevelOvr1P_EmitClipRecordTri(pb, primMem, otEntry, work, indices, record);
	case 0x800aaa28:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[0], &work[2]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[1], &work[2]);
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 4;
		indices[3] = 5;
		return DrawLevelOvr1P_EmitClipRecordQuad(pb, primMem, otEntry, work, indices, record);
	case 0x800aaa60:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[1], &work[0]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[1], &work[2]);
		indices[0] = 1;
		indices[1] = 5;
		indices[2] = 4;
		return DrawLevelOvr1P_EmitClipRecordTri(pb, primMem, otEntry, work, indices, record);
	case 0x800aaa94:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[0], &work[2]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[0], &work[1]);
		indices[0] = 0;
		indices[1] = 5;
		indices[2] = 4;
		return DrawLevelOvr1P_EmitClipRecordTri(pb, primMem, otEntry, work, indices, record);
	case 0x800aaac8:
		return 1;
	default:
		return 1;
	}
}

static int DrawLevelOvr1P_EmitClipRecordTableTri(struct PushBuffer *pb, struct PrimMem *primMem, u_long *otEntry,
                                                 const struct DrawLevelOvr1PScratchVertex *work, const struct DrawLevelOvr1PClipRecord *record, int a, int b,
                                                 int c)
{
	int indices[3] = {a, b, c};

	return DrawLevelOvr1P_EmitClipRecordTri(pb, primMem, otEntry, work, indices, record);
}

static int DrawLevelOvr1P_EmitClipRecordTableQuad(struct PushBuffer *pb, struct PrimMem *primMem, u_long *otEntry,
                                                  const struct DrawLevelOvr1PScratchVertex *work, const struct DrawLevelOvr1PClipRecord *record, int a, int b,
                                                  int c, int d)
{
	int indices[4] = {a, b, c, d};

	return DrawLevelOvr1P_EmitClipRecordQuad(pb, primMem, otEntry, work, indices, record);
}

static int DrawLevelOvr1P_EmitClipRecordGT4Table(struct PushBuffer *pb, struct PrimMem *primMem, u_long *otEntry,
                                                 const struct DrawLevelOvr1PScratchVertex *projected, const struct DrawLevelOvr1PClipRecord *record)
{
	struct DrawLevelOvr1PScratchVertex *work = DrawLevelOvr1P_GetClipRecordWorkspace();
	u32 handlerAddress;
	u32 nearMask;

	if (work != projected)
	{
		work[0] = projected[0];
		work[1] = projected[1];
		work[2] = projected[2];
		work[3] = projected[3];
	}

	// NOTE(aalhendi): Retail 0x800aaf28..0x800aaf6c mirrors the GT3 path for
	// GT4 records, including the table-select clear of the source near bytes.
	nearMask = DrawLevelOvr1P_GetClipRecordProjectedNearMask(work, 4);
	DrawLevelOvr1P_ClearClipRecordProjectedNearBytes(work, 4);
	handlerAddress = DrawLevelOvr1P_GetClipRecordJumpAddress(4, nearMask);

	// NOTE(aalhendi): Retail dispatches through scratch 0x260; native keeps the handler bodies as C cases keyed by the copied retail addresses.
	//  Work records live at retail scratch 0x1b4..0x22c.
	switch (handlerAddress)
	{
	case 0x800aaf70:
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 0, 1, 2, 3);
	case 0x800aaf8c:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[1], &work[0]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[2], &work[0]);
		if (!DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 3, 2, 1))
			return 0;
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 1, 2, 4, 5);
	case 0x800aafd4:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[3], &work[1]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[2], &work[1]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[6], &work[0], &work[1]);
		if (!DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 3, 2, 4, 5))
			return 0;
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 2, 0, 5, 6);
	case 0x800ab02c:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[3], &work[1]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[2], &work[1]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[6], &work[2], &work[0]);
		if (!DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 3, 2, 4, 5))
			return 0;
		return DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 2, 6, 5);
	case 0x800ab080:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[0], &work[2]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[1], &work[2]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[6], &work[3], &work[2]);
		if (!DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 0, 1, 4, 5))
			return 0;
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 1, 3, 5, 6);
	case 0x800ab0d8:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[1], &work[0]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[1], &work[2]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[6], &work[3], &work[2]);
		if (!DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 1, 5, 4))
			return 0;
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 1, 3, 5, 6);
	case 0x800ab12c:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[0], &work[2]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[0], &work[1]);
		if (!DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 0, 5, 4))
			return 0;
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[3], &work[2]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[3], &work[1]);
		return DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 3, 5, 4);
	case 0x800ab18c:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[3], &work[1]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[3], &work[2]);
		return DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 3, 5, 4);
	case 0x800ab1c0:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[2], &work[3]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[1], &work[3]);
		if (!DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 0, 1, 2))
			return 0;
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 2, 1, 4, 5);
	case 0x800ab208:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[1], &work[0]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[2], &work[0]);
		if (!DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 1, 2, 4, 5))
			return 0;
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[1], &work[3]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[2], &work[3]);
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 1, 2, 4, 5);
	case 0x800ab270:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[2], &work[3]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[2], &work[1]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[6], &work[0], &work[1]);
		if (!DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 2, 5, 4))
			return 0;
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 2, 0, 5, 6);
	case 0x800ab2c4:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[2], &work[3]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[2], &work[1]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[6], &work[2], &work[0]);
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 4, 2, 5, 6);
	case 0x800ab308:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[0], &work[2]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[1], &work[2]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[6], &work[1], &work[3]);
		if (!DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 0, 1, 4, 5))
			return 0;
		return DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 1, 6, 5);
	case 0x800ab35c:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[1], &work[0]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[1], &work[2]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[6], &work[1], &work[3]);
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 4, 1, 5, 6);
	case 0x800ab3a0:
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[4], &work[0], &work[2]);
		DrawLevelOvr1P_InterpolateClipRecordVertex(&work[5], &work[0], &work[1]);
		return DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 0, 5, 4);
	case 0x800ab3d4:
		return 1;
	default:
		return 1;
	}
}

static int DrawLevelOvr1P_EmitClippedRecordBridge(struct PushBuffer *pb, struct PrimMem *primMem, const struct DrawLevelOvr1PScratchVertex *projected,
                                                  const int *indices, int count, const struct DrawLevelOvr1PClipRecord *record)
{
	u_long *otEntry = (u_long *)(uintptr_t)record->otEntry;

	if (otEntry == NULL)
		return 1;

	if (count == 3)
	{
		// NOTE(aalhendi): Native mirrors the GT3 jump table at 0x800aa96c..0x800aaac8 through the copied scratch table.
		return DrawLevelOvr1P_EmitClipRecordGT3Table(pb, primMem, otEntry, projected, record);
	}

	// NOTE(aalhendi): Native mirrors the GT4 jump table at 0x800aaf70..0x800ab3d4 through the copied scratch table.
	//  Shared emit-helper GTE side effects remain a separate audit gap.
	return DrawLevelOvr1P_EmitClipRecordGT4Table(pb, primMem, otEntry, projected, record);
}

static void DrawLevelOvr1P_SetClipRecordGteState(struct PushBuffer *pb)
{
	s16 threshold = (s16)(((u32)pb->distanceToScreen_PREV >> 1) + 1);

	for (int i = 0; i < 8; i++)
		CTC2(DrawLevelOvr1P_ReadWord(&pb->matrix_ViewProj, i * 4), 8 + i);

	CTC2((u32)(s32)pb->rect.w << 15, 24);
	CTC2((u32)(s32)pb->rect.h << 15, 25);
	CTC2((u32)pb->distanceToScreen_PREV, 26);

	// NOTE(aalhendi): Retail 0x800aa7f8 mirrors the clip threshold into the
	// temporary vertices used by the 0x800aab00 interpolation helper.
	*CTR_SCRATCHPAD_PTR(s16, 0x208) = threshold;
	*CTR_SCRATCHPAD_PTR(s16, 0x21c) = threshold;
	*CTR_SCRATCHPAD_PTR(s16, 0x230) = threshold;
	*CTR_SCRATCHPAD_PTR(s16, 0x216) = 0;
	*CTR_SCRATCHPAD_PTR(s16, 0x22a) = 0;
	*CTR_SCRATCHPAD_PTR(s16, 0x23e) = 0;
	*CTR_SCRATCHPAD_PTR(u32, 0x6c) = DrawLevelOvr1P_ReadWord(&pb->rect, 4);

	CTC2(0x1000, 0);
	CTC2(0, 1);
	CTC2(0x1000, 2);
	CTC2(0, 3);
	CTC2(0x1000, 4);
	CTC2(0, 5);
	CTC2(0, 6);
	CTC2(0, 7);
}

static int DrawLevelOvr1P_HasClipRecordConsumerPrimReserve(const struct PrimMem *primMem)
{
	return (u8 *)primMem->curr + DRAW_LEVEL_OVR1P_CLIP_RECORD_PRIM_RESERVE <= (u8 *)primMem->end;
}

// NOTE(aalhendi): Ghidra shows top-level 226 handlers preflight primMem by bucket family before each quadblock.
static u32 DrawLevelOvr1P_GetBucketPrimReserve(int role)
{
	switch (role)
	{
	case DRAW_LEVEL_OVR1P_BUCKET_4X1_LIST:
	case DRAW_LEVEL_OVR1P_BUCKET_4X1_RENDERED:
		return DRAW_LEVEL_OVR1P_BUCKET_RESERVE_4X1;
	case DRAW_LEVEL_OVR1P_BUCKET_4X2_LIST:
	case DRAW_LEVEL_OVR1P_BUCKET_4X2_RENDERED:
		return DRAW_LEVEL_OVR1P_BUCKET_RESERVE_4X2;
	case DRAW_LEVEL_OVR1P_BUCKET_FULL_DYNAMIC_LIST:
		return DRAW_LEVEL_OVR1P_BUCKET_RESERVE_FULL_DYNAMIC;
	default:
		return DRAW_LEVEL_OVR1P_BUCKET_RESERVE_DEFAULT;
	}
}

static int DrawLevelOvr1P_HasBucketPrimReserve(const struct PrimMem *primMem, u32 reserve)
{
	u8 *curr = primMem->curr;
	u8 *end = primMem->end;

	return curr <= end && (size_t)reserve <= (size_t)(end - curr);
}

static int DrawLevelOvr1P_ConsumeClipRecords(struct PushBuffer *pb, struct PrimMem *primMem)
{
	u8 *start = DrawLevelOvr1P_GetClipRecordStart();
	u8 *cursor = start;
	u8 *end = DrawLevelOvr1P_GetClipRecordCursor();
	u8 *bufferEnd = DrawLevelOvr1P_GetClipRecordEnd();

	if (start == NULL || end == NULL || bufferEnd == NULL)
		return 1;

	if (end < start || end > bufferEnd)
	{
		DrawLevelOvr1P_SetClipRecordCursor(start);
		return 1;
	}

	DrawLevelOvr1P_SetClipRecordGteState(pb);

	while (cursor < end)
	{
		struct DrawLevelOvr1PClipRecord *record = (struct DrawLevelOvr1PClipRecord *)cursor;
		struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetClipRecordWorkspace();
		int indices[4] = {0, 1, 2, 3};
		int count = (record->header & 1) != 0 ? 4 : 3;
		size_t recordSize = DrawLevelOvr1P_GetClipRecordSize(count);

		if (recordSize > (size_t)(end - cursor))
			break;

		// NOTE(aalhendi): Retail 0x800aa848 preflights primMem->end against curr+0xd68 before each clipped record.
		if (!DrawLevelOvr1P_HasClipRecordConsumerPrimReserve(primMem))
			return 0;

		DrawLevelOvr1P_SetClipRecordPageScratch(record);

		for (int i = 0; i < count; i++)
			DrawLevelOvr1P_ProjectClipRecordVertex(&projected[i], &record->vertex[i]);

		if (!DrawLevelOvr1P_EmitClippedRecordBridge(pb, primMem, projected, indices, count, record))
			return 0;

		cursor += recordSize;
	}

	DrawLevelOvr1P_SetClipRecordCursor(start);
	return 1;
}

static int DrawLevelOvr1P_SelectDirectBit(s32 nclipResult, u32 tableWord, u32 drawOrderLow, int negateResult, u32 directBit)
{
	u32 resultBits;

	if (nclipResult == 0)
		resultBits = 0;
	else if (negateResult)
		resultBits = (0u - (u32)nclipResult) ^ tableWord;
	else
		resultBits = (u32)nclipResult ^ tableWord;

	if ((s32)(resultBits | drawOrderLow) < 0)
		return directBit;

	return 0;
}

static u32 DrawLevelOvr1P_SelectDirectMask(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                           u32 tableWord, u32 allowedMask)
{
	int primaryIndices[3] = {indices[0], indices[1], indices[2]};
	int secondaryNclipIndices[3] = {indices[3], indices[1], indices[2]};
	u32 drawOrderLow = DrawLevelOvr1P_GetActiveDrawOrderLow();
	u32 directMask;

	// NOTE(aalhendi): Retail derives direct offsets 4/8 from the GTE NCLIP
	// sign folded with scratch 0x7c and the selected scratch table word.
	(void)block;
	directMask = DrawLevelOvr1P_SelectDirectBit(DrawLevelOvr1P_NclipProjected(projected, primaryIndices), tableWord, drawOrderLow, 1,
	                                            DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY);
	directMask |= DrawLevelOvr1P_SelectDirectBit(DrawLevelOvr1P_NclipProjected(projected, secondaryNclipIndices), tableWord, drawOrderLow, 0,
	                                             DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY);

	return directMask & allowedMask;
}

static u32 DrawLevelOvr1P_SelectAndStoreDirectMask(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                                   u32 tableWord, u32 allowedMask)
{
	u32 directMask = DrawLevelOvr1P_SelectDirectMask(block, projected, indices, tableWord, allowedMask);

	*CTR_SCRATCHPAD_PTR(u32, 0x70) = directMask;
	return directMask;
}

static u32 DrawLevelOvr1P_GetDirectHandlerAddress(u32 directMask)
{
	if (directMask == 0)
		return 0;

	return *CTR_SCRATCHPAD_PTR(u32, 0x184 + (int)directMask);
}

static int DrawLevelOvr1P_EmitPreparedProjectedTriDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                         const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect);
static int DrawLevelOvr1P_EmitPreparedProjectedTriDirectAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                             const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect,
                                                             int otIndexOverride);
static int DrawLevelOvr1P_EmitPreparedProjectedQuadDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                          const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                          const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect);
static int DrawLevelOvr1P_EmitPreparedProjectedQuadDirectAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                              const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                              const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect,
                                                              int otIndexOverride);
static int DrawLevelOvr1P_EmitPreparedProjectedDirectMaskRaw(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                             const struct TextureLayout *texture, u32 directMask);
static int DrawLevelOvr1P_EmitPreparedProjectedDirectMaskRawAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                                 const struct TextureLayout *texture, u32 directMask, int otIndexOverride);
static int DrawLevelOvr1P_EmitPreparedProjectedDirectMaskAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                              const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                              const struct TextureLayout *texture, u32 directMask, int writeClipBytes, int waterRenderedDirect,
                                                              int otIndexOverride);

static int DrawLevelOvr1P_IsRawListDirectHandlerAddress(u32 handlerAddress)
{
	switch (handlerAddress)
	{
	case 0x800a1ce8:
	case 0x800a1ce0:
	case 0x800a1d6c:
	case 0x800a27dc:
	case 0x800a27d4:
	case 0x800a2850:
	case 0x800a4034:
	case 0x800a402c:
	case 0x800a40b8:
	case 0x800a5d14:
	case 0x800a5d0c:
	case 0x800a5d98:
	case 0x800a7a60:
	case 0x800a7a58:
	case 0x800a7ae4:
	case 0x800a9680:
	case 0x800a9678:
	case 0x800a9704:
		return 1;
	default:
		return 0;
	}
}

static int DrawLevelOvr1P_EmitPreparedProjectedDirectMask(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                          const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                          const struct TextureLayout *texture, u32 directMask, int writeClipBytes, int waterRenderedDirect)
{
	return DrawLevelOvr1P_EmitPreparedProjectedDirectMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes,
	                                                          waterRenderedDirect, DRAW_LEVEL_OVR1P_OT_INDEX_AUTO);
}

static int DrawLevelOvr1P_EmitPreparedProjectedDirectMaskAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                              const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                              const struct TextureLayout *texture, u32 directMask, int writeClipBytes, int waterRenderedDirect,
                                                              int otIndexOverride)
{
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	if (handlerAddress == 0)
		return 1;

	// NOTE(aalhendi): Retail list-bucket direct handlers outside the target
	// 0x800a87d4/0x800a8888 family are raw packet writers after the caller's
	// cull/direct-mask checks. The target family keeps local terminal clip
	// checks even though list-projected clip bytes normally make them inert.
	if (writeClipBytes == DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST && !waterRenderedDirect && DrawLevelOvr1P_IsRawListDirectHandlerAddress(handlerAddress))
		return DrawLevelOvr1P_EmitPreparedProjectedDirectMaskRawAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, otIndexOverride);

	// NOTE(aalhendi): Retail direct table offsets 4/8/12 map to GT3 primary,
	// GT3 secondary, and GT4 handlers through scratch 0x188.
	switch (handlerAddress)
	{
	case 0x800a1ce8:
	case 0x800a27dc:
	case 0x800a333c:
	case 0x800a4034:
	case 0x800a4c14:
	case 0x800a5d14:
	case 0x800a6bb4:
	case 0x800a7a60:
	case 0x800a87d4:
	case 0x800a9680:
	case 0x800aa3f4:
	{
		int triIndices[3] = {indices[0], indices[1], indices[2]};

		return DrawLevelOvr1P_EmitPreparedProjectedTriDirectAtOt(pb, primMem, block, projected, triIndices, faceIndex, texture, writeClipBytes,
		                                                         waterRenderedDirect, otIndexOverride);
	}

	case 0x800a1ce0:
	case 0x800a27d4:
	case 0x800a3334:
	case 0x800a402c:
	case 0x800a4c0c:
	case 0x800a5d0c:
	case 0x800a6bac:
	case 0x800a7a58:
	case 0x800a87cc:
	case 0x800a9678:
	case 0x800aa3ec:
	{
		int triIndices[3] = {indices[1], indices[3], indices[2]};

		return DrawLevelOvr1P_EmitPreparedProjectedTriDirectAtOt(pb, primMem, block, projected, triIndices, faceIndex, texture, writeClipBytes,
		                                                         waterRenderedDirect, otIndexOverride);
	}

	case 0x800a1d6c:
	case 0x800a2850:
	case 0x800a33e0:
	case 0x800a40b8:
	case 0x800a4cc8:
	case 0x800a5d98:
	case 0x800a6c68:
	case 0x800a7ae4:
	case 0x800a8888:
	case 0x800a9704:
	case 0x800aa4a8:
		return DrawLevelOvr1P_EmitPreparedProjectedQuadDirectAtOt(pb, primMem, block, projected, indices, faceIndex, texture, writeClipBytes,
		                                                          waterRenderedDirect, otIndexOverride);

	default:
		return 1;
	}
}

static int DrawLevelOvr1P_EmitPreparedProjectedDirectMaskRaw(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                             const struct TextureLayout *texture, u32 directMask)
{
	return DrawLevelOvr1P_EmitPreparedProjectedDirectMaskRawAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask,
	                                                             DRAW_LEVEL_OVR1P_OT_INDEX_AUTO);
}

static int DrawLevelOvr1P_EmitPreparedProjectedDirectMaskRawAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                                 const struct TextureLayout *texture, u32 directMask, int otIndexOverride)
{
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	if (handlerAddress == 0)
		return 1;

	// NOTE(aalhendi): List direct helpers trust the caller's
	// offscreen/direct/near decisions and write raw packets.
	switch (handlerAddress)
	{
	case 0x800a1ce8:
	case 0x800a27dc:
	case 0x800a4034:
	case 0x800a5d14:
	case 0x800a7a60:
	case 0x800a87d4:
	case 0x800a9680:
	{
		int triIndices[3] = {indices[0], indices[1], indices[2]};

		return DrawLevelOvr1P_EmitPreparedProjectedTriRawAtOt(pb, primMem, block, projected, triIndices, faceIndex, texture,
		                                                      DrawLevelOvr1P_GetProjectedTriMaxDepth(projected, triIndices), otIndexOverride);
	}

	case 0x800a1ce0:
	case 0x800a27d4:
	case 0x800a402c:
	case 0x800a5d0c:
	case 0x800a7a58:
	case 0x800a87cc:
	case 0x800a9678:
	{
		int triIndices[3] = {indices[1], indices[3], indices[2]};

		return DrawLevelOvr1P_EmitPreparedProjectedTriRawAtOt(pb, primMem, block, projected, triIndices, faceIndex, texture,
		                                                      DrawLevelOvr1P_GetProjectedTriMaxDepth(projected, triIndices), otIndexOverride);
	}

	case 0x800a1d6c:
	case 0x800a2850:
	case 0x800a40b8:
	case 0x800a5d98:
	case 0x800a7ae4:
	case 0x800a8888:
	case 0x800a9704:
		return DrawLevelOvr1P_EmitPreparedProjectedQuadRawAtOt(pb, primMem, block, projected, indices, faceIndex, texture,
		                                                       DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices), otIndexOverride);

	default:
		return DrawLevelOvr1P_EmitPreparedProjectedDirectMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask,
		                                                          DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST, 0, otIndexOverride);
	}
}

static int DrawLevelOvr1P_EmitPreparedProjectedTriDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                         const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect)
{
	return DrawLevelOvr1P_EmitPreparedProjectedTriDirectAtOt(pb, primMem, block, projected, indices, faceIndex, texture, writeClipBytes, waterRenderedDirect,
	                                                         DRAW_LEVEL_OVR1P_OT_INDEX_AUTO);
}

static int DrawLevelOvr1P_EmitPreparedProjectedTriDirectAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                             const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect,
                                                             int otIndexOverride)
{
	(void)writeClipBytes;
	(void)waterRenderedDirect;

	// NOTE(aalhendi): Retail GT3 direct helpers use `bltz` on clip bytes that
	// the rendered/list projection paths store as 0/1, so GT3 stays direct here.
	// Terminal direct handlers write raw packets after the caller-side cull.

	return DrawLevelOvr1P_EmitPreparedProjectedTriRawAtOt(pb, primMem, block, projected, indices, faceIndex, texture,
	                                                      DrawLevelOvr1P_GetProjectedTriMaxDepth(projected, indices), otIndexOverride);
}

static int DrawLevelOvr1P_EmitPreparedProjectedQuadDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                          const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                          const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect)
{
	return DrawLevelOvr1P_EmitPreparedProjectedQuadDirectAtOt(pb, primMem, block, projected, indices, faceIndex, texture, writeClipBytes, waterRenderedDirect,
	                                                          DRAW_LEVEL_OVR1P_OT_INDEX_AUTO);
}

static int DrawLevelOvr1P_EmitPreparedProjectedQuadDirectAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                              const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                              const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect,
                                                              int otIndexOverride)
{
	// NOTE(aalhendi): Retail terminal GT4 helpers consume the current scratch
	// 0x80 clipped-record header; selector/rendered owners refresh it before
	// dispatch, while copied recursive helpers inherit it unchanged.
	if (writeClipBytes)
	{
		if (DrawLevelOvr1P_AreProjectedVerticesHalfNear(projected, indices, 4))
			return 1;

		if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
		{
			if (waterRenderedDirect)
				return DrawLevelOvr1P_EmitWaterRenderedClippedPolygonBridge(pb, primMem, block, projected, indices, 4, faceIndex);

			return DrawLevelOvr1P_EmitRenderedClippedPolygonBridgeAtOt(pb, primMem, block, projected, indices, 4, faceIndex, texture, otIndexOverride);
		}
	}

	// NOTE(aalhendi): Retail GT4 terminal direct handlers have no second
	// screen-space cull after the caller-side direct-mask reduction.
	return DrawLevelOvr1P_EmitPreparedProjectedQuadRawAtOt(pb, primMem, block, projected, indices, faceIndex, texture,
	                                                       DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices), otIndexOverride);
}

static int DrawLevelOvr1P_EmitPreparedProjectedDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                      const struct TextureLayout *texture, u32 tableWord, int writeClipBytes, u32 allowedMask)
{
	u32 directMask = DrawLevelOvr1P_SelectAndStoreDirectMask(block, projected, indices, tableWord, allowedMask);

	return DrawLevelOvr1P_EmitPreparedProjectedDirectMask(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes, 0);
}

static u32 DrawLevelOvr1P_GetNearSubdivisionHandlerAddress(u32 nearMask, int writeClipBytes)
{
	u32 tableIndex = (nearMask >> 2) - 1;

	if (tableIndex >= 15)
		return writeClipBytes ? 0x800a4550 : 0x800a3a34;

	return *CTR_SCRATCHPAD_PTR(u32, 0x14c + (int)(tableIndex * sizeof(u32)));
}

static void DrawLevelOvr1P_SetPreviousRecursiveHandler(u32 handlerAddress)
{
	// NOTE(aalhendi): Retail stores the active recursive handler at scratch
	// 0x9c immediately before jumping through the copied table at scratch 0x14c.
	*CTR_SCRATCHPAD_PTR(u32, 0x9c) = handlerAddress;
}

static const struct DrawLevelOvr1PNearSubdivisionCase *DrawLevelOvr1P_FindCompactNearSubdivisionCase(u32 handlerAddress)
{
	for (int i = 0; i < (int)(sizeof(sDrawLevelOvr1PNearSubdivisionCases) / sizeof(sDrawLevelOvr1PNearSubdivisionCases[0])); i++)
	{
		if (handlerAddress == sDrawLevelOvr1PNearSubdivisionCases[i].listHandlerAddress ||
		    handlerAddress == sDrawLevelOvr1PNearSubdivisionCases[i].renderedHandlerAddress)
			return &sDrawLevelOvr1PNearSubdivisionCases[i];
	}

	return NULL;
}

static struct DrawLevelOvr1PScratchVertex *DrawLevelOvr1P_GetSubdivisionFrame(int depth)
{
	return CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PScratchVertex, 0x1b4 + ((depth + 1) * DRAW_LEVEL_OVR1P_RECURSION_FRAME_SIZE));
}

static void DrawLevelOvr1P_BuildGridSubdivisionFrame(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                     const int *indices, int writeClipBytes);
static void DrawLevelOvr1P_BuildGridSubdivisionFrame4x4(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                        const int *indices, int writeClipBytes);

static int DrawLevelOvr1P_EmitDeepestProjectedDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                     const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                     const struct TextureLayout *texture, u32 tableWord, int writeClipBytes, u32 allowedMask);
static int DrawLevelOvr1P_EmitDeepestProjectedDirectAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                         const struct TextureLayout *texture, u32 tableWord, int writeClipBytes, u32 allowedMask,
                                                         int otIndexOverride)
{
	u32 directMask;
	u32 directHandlerAddress;
	int result;

	directMask = DrawLevelOvr1P_SelectAndStoreDirectMask(block, projected, indices, tableWord, allowedMask);
	directHandlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);
	if (texture != NULL && directHandlerAddress != 0)
		DrawLevelOvr1P_PrepareDeepestMosaicUv(projected, indices, directHandlerAddress);

	result = DrawLevelOvr1P_EmitPreparedProjectedDirectMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes, 0,
	                                                            otIndexOverride);

	// NOTE(aalhendi): Retail 0x800a4000/0x800a4be0 restores the saved UV words
	// after deepest-frame direct-handler dispatch.
	DrawLevelOvr1P_RestoreProjectedUvScratch();

	return result;
}

static int DrawLevelOvr1P_EmitDeepestProjectedDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                     const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                     const struct TextureLayout *texture, u32 tableWord, int writeClipBytes, u32 allowedMask)
{
	return DrawLevelOvr1P_EmitDeepestProjectedDirectAtOt(pb, primMem, block, projected, indices, faceIndex, texture, tableWord, writeClipBytes, allowedMask,
	                                                     DRAW_LEVEL_OVR1P_OT_INDEX_AUTO);
}

static int DrawLevelOvr1P_EmitDeepestProjectedDirectStoredMask(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                               const struct TextureLayout *texture, u32 directMask, int writeClipBytes);
static int DrawLevelOvr1P_EmitDeepestProjectedDirectStoredMaskAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                   const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                                   const struct TextureLayout *texture, u32 directMask, int writeClipBytes, int otIndexOverride)
{
	u32 directHandlerAddress;
	int result;

	*CTR_SCRATCHPAD_PTR(u32, 0x70) = directMask;
	directHandlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);
	if (texture != NULL && directHandlerAddress != 0)
		DrawLevelOvr1P_PrepareDeepestMosaicUv(projected, indices, directHandlerAddress);

	result = DrawLevelOvr1P_EmitPreparedProjectedDirectMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes, 0,
	                                                            otIndexOverride);

	// NOTE(aalhendi): Retail deepest-frame dispatch restores saved UV scratch
	// after jumping through the current direct table mask.
	DrawLevelOvr1P_RestoreProjectedUvScratch();

	return result;
}

static int DrawLevelOvr1P_EmitDeepestProjectedDirectStoredMask(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                               const struct TextureLayout *texture, u32 directMask, int writeClipBytes)
{
	return DrawLevelOvr1P_EmitDeepestProjectedDirectStoredMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes,
	                                                               DRAW_LEVEL_OVR1P_OT_INDEX_AUTO);
}

static int DrawLevelOvr1P_EmitDeepestProjectedDirectMask(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                         const struct TextureLayout *texture, u32 directMask, int writeClipBytes);
static int DrawLevelOvr1P_EmitDeepestProjectedDirectMaskAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                             const struct TextureLayout *texture, u32 directMask, int writeClipBytes, int otIndexOverride)
{
	u32 directHandlerAddress;
	int result;

	(void)writeClipBytes;

	*CTR_SCRATCHPAD_PTR(u32, 0x70) = directMask;
	directHandlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);
	if (texture != NULL && directHandlerAddress != 0)
		DrawLevelOvr1P_PrepareDeepestMosaicUv(projected, indices, directHandlerAddress);

	result = DrawLevelOvr1P_EmitPreparedProjectedDirectMaskRawAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, otIndexOverride);

	// NOTE(aalhendi): Deepest-frame direct dispatch preserves the caller's saved UV words.
	DrawLevelOvr1P_RestoreProjectedUvScratch();

	return result;
}

static int DrawLevelOvr1P_EmitDeepestProjectedDirectMask(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                         const struct TextureLayout *texture, u32 directMask, int writeClipBytes)
{
	return DrawLevelOvr1P_EmitDeepestProjectedDirectMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes,
	                                                         DRAW_LEVEL_OVR1P_OT_INDEX_AUTO);
}

static int DrawLevelOvr1P_GetCopiedRecursiveHandlerSlot(u32 handlerAddress)
{
	u32 *table = CTR_SCRATCHPAD_PTR(u32, 0x14c);

	for (int i = 0; i < 15; i++)
	{
		if (table[i] == handlerAddress)
			return i;
	}

	return 5;
}

static int DrawLevelOvr1P_EmitProjectedGridNearClippedBridge(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                             const struct TextureLayout *texture, u32 nearMask, int depth, int writeClipBytes, u32 allowedMask,
                                                             int inheritedOtIndex);
static int DrawLevelOvr1P_IsDeepestSubdivisionFrame(const struct DrawLevelOvr1PScratchVertex *projected);

static int DrawLevelOvr1P_EmitProjectedGridFaceNearBranch(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                          const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                          const struct TextureLayout *texture, int depth, int writeClipBytes, int inheritedOtIndex)
{
	const u32 directMask = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
	u32 nearMask;

	*CTR_SCRATCHPAD_PTR(u32, 0x70) = directMask;

	// NOTE(aalhendi): Retail 0x800a8360/0x800a9f80 enters the deepest
	// handler from frame 0x324 before selecting another near child.
	if (DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
		return DrawLevelOvr1P_EmitDeepestProjectedDirectStoredMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes,
		                                                               inheritedOtIndex);

	nearMask = DrawLevelOvr1P_GetProjectedCopiedGridNearMask(projected, indices);
	if (nearMask != 0)
	{
		return DrawLevelOvr1P_EmitProjectedGridNearClippedBridge(pb, primMem, block, projected, indices, faceIndex, texture, nearMask, depth, writeClipBytes,
		                                                         directMask, inheritedOtIndex);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedDirectMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes, 0,
	                                                          inheritedOtIndex);
}

static int DrawLevelOvr1P_EmitProjectedGridFaceDirectBranch(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                            const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                            const struct TextureLayout *texture, int depth, int writeClipBytes, u32 allowedMask,
                                                            int inheritedOtIndex)
{
	u32 directMask;
	u32 nearMask;

	// NOTE(aalhendi): Retail 0x800a82a0/0x800a9ec0 stores caller t2 before
	// packed-SXY/offscreen and NCLIP reduce it to the surviving direct mask.
	*CTR_SCRATCHPAD_PTR(u32, 0x70) = allowedMask;

	if (DrawLevelOvr1P_IsProjectedFaceOffscreen(pb, projected, indices))
		return 1;

	// NOTE(aalhendi): Retail projected-grid helpers reduce scratch 0x70 before
	// near recursion, so child dispatch only sees the surviving direct cases.
	directMask = DrawLevelOvr1P_SelectAndStoreDirectMask(block, projected, indices, *CTR_SCRATCHPAD_PTR(u32, 0x194), allowedMask);
	if (directMask == 0)
		return 1;

	// NOTE(aalhendi): The shared retail helper uses the deepest-frame direct
	// path for frame 0x324 even when no further near child is selected.
	if (DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
		return DrawLevelOvr1P_EmitDeepestProjectedDirectStoredMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes,
		                                                               inheritedOtIndex);

	nearMask = DrawLevelOvr1P_GetProjectedCopiedGridNearMask(projected, indices);
	if (nearMask != 0)
	{
		return DrawLevelOvr1P_EmitProjectedGridNearClippedBridge(pb, primMem, block, projected, indices, faceIndex, texture, nearMask, depth, writeClipBytes,
		                                                         directMask, inheritedOtIndex);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedDirectMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes, 0,
	                                                          inheritedOtIndex);
}

static int DrawLevelOvr1P_EmitProjectedGridFaceCommon(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                      const struct TextureLayout *texture, int depth, int writeClipBytes, u32 allowedMask, int inheritedOtIndex)
{
	// TODO(aalhendi): Replace shared grid bridge with exact retail terminal
	// handlers around 0x800a87c8 and 0x800aa3e8.
	// NOTE(aalhendi): Copied recursive/default grid helpers preserve inherited
	// UV/texture state. Texture-slot selection belongs to the selector entries
	// before this path, such as retail 0x800a8380 and 0x800a9fa0.
	if (writeClipBytes && DrawLevelOvr1P_IsProjectedFaceFullyNear(projected, indices))
		return 1;

	// NOTE(aalhendi): Rendered helpers branch on clip bytes before direct-mask
	// reduction. The clipped-record header scratch word is inherited here.
	if (writeClipBytes && DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
		return DrawLevelOvr1P_EmitProjectedGridFaceNearBranch(pb, primMem, block, projected, indices, faceIndex, texture, depth, writeClipBytes,
		                                                      inheritedOtIndex);

	return DrawLevelOvr1P_EmitProjectedGridFaceDirectBranch(pb, primMem, block, projected, indices, faceIndex, texture, depth, writeClipBytes, allowedMask,
	                                                        inheritedOtIndex);
}

static int DrawLevelOvr1P_EmitProjectedGridFace(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                const struct TextureLayout *texture, int depth, int writeClipBytes, u32 allowedMask, int inheritedOtIndex)
{
	return DrawLevelOvr1P_EmitProjectedGridFaceCommon(pb, primMem, block, projected, indices, faceIndex, texture, depth, writeClipBytes, allowedMask,
	                                                  inheritedOtIndex);
}

static int DrawLevelOvr1P_IsDeepestSubdivisionFrame(const struct DrawLevelOvr1PScratchVertex *projected)
{
	return projected == CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PScratchVertex, 0x324);
}

static int DrawLevelOvr1P_HandlerUsesDeepestCompactGrid(u32 handlerAddress)
{
	// NOTE(aalhendi): Only the 0x60 generic-grid helper families branch from
	// scratch frame 0x324 back to the compact topology labels.
	switch (handlerAddress)
	{
	case 0x800a535c:
	case 0x800a53a4:
	case 0x800a5404:
	case 0x800a5464:
	case 0x800a54ac:
	case 0x800a54e0:
	case 0x800a5514:
	case 0x800a5548:
	case 0x800a557c:
	case 0x800a6260:
	case 0x800a62a8:
	case 0x800a6308:
	case 0x800a6368:
	case 0x800a63b0:
	case 0x800a63e4:
	case 0x800a6418:
	case 0x800a644c:
	case 0x800a6480:
		return 1;

	default:
		return 0;
	}
}

static int DrawLevelOvr1P_HandlerUses4x4GridFrame(u32 handlerAddress)
{
	// NOTE(aalhendi): Dynamic-rendered 0x800a7fac..0x800a810c keeps the
	// normal 3x3 builder despite wide slot strides.
	switch (handlerAddress)
	{
	case 0x800a8f1c:
	case 0x800a8f5c:
	case 0x800a8fdc:
	case 0x800a8f7c:
	case 0x800a9004:
	case 0x800a907c:
	case 0x800a8f9c:
	case 0x800a902c:
	case 0x800a9054:
	case 0x800a9bcc:
	case 0x800a9c0c:
	case 0x800a9c8c:
	case 0x800a9c2c:
	case 0x800a9cb4:
	case 0x800a9d2c:
	case 0x800a9c4c:
	case 0x800a9cdc:
	case 0x800a9d04:
		return 1;

	default:
		return 0;
	}
}

static const struct DrawLevelOvr1PNearSubdivisionCase *DrawLevelOvr1P_GetDeepestGridCompactCase(int slot)
{
	switch (slot)
	{
	case 0:
	case 1:
	case 2:
		return &sDrawLevelOvr1PDeepestGridSubdivisionCases[0];

	case 3:
	case 7:
	case 11:
		return &sDrawLevelOvr1PDeepestGridSubdivisionCases[1];

	default:
		return &sDrawLevelOvr1PDeepestGridSubdivisionCases[2];
	}
}

static int DrawLevelOvr1P_DispatchProjectedGridDeepestCompact(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                              const struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, int depth,
                                                              const struct TextureLayout *texture, int writeClipBytes, u32 allowedMask, int slot,
                                                              int inheritedOtIndex)
{
	const struct DrawLevelOvr1PNearSubdivisionCase *subdivisionCase = DrawLevelOvr1P_GetDeepestGridCompactCase(slot);

	for (int i = 0; i < 2; i++)
	{
		int subIndices[4];

		for (int j = 0; j < 4; j++)
			subIndices[j] = subdivisionCase->subIndices[i][j];

		// NOTE(aalhendi): Retail deepest generic-grid handlers branch back to
		// the compact topology labels, including the special +0xb4 writes.
		if (subdivisionCase->slotWords[i] != DRAW_LEVEL_OVR1P_SLOT_WORD_PRESERVE)
			DrawLevelOvr1P_SetGridFaceSlotWord(projected, subdivisionCase->slotWords[i]);

		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, subIndices, faceIndex, texture, depth, writeClipBytes,
		                                          subdivisionCase->directMasks[i] & allowedMask, inheritedOtIndex))
			return 0;
	}

	return 1;
}

static int DrawLevelOvr1P_DispatchProjectedGridHelper(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, int depth,
                                                      const struct TextureLayout *texture, u32 handlerAddress, int writeClipBytes, u32 allowedMask,
                                                      int inheritedOtIndex)
{
	int slot = DrawLevelOvr1P_GetCopiedRecursiveHandlerSlot(handlerAddress);
	const u32 allowedQuad = DRAW_LEVEL_OVR1P_DIRECT_QUAD & allowedMask;
	const u32 allowedTriPrimary = DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY & allowedMask;
	const u32 allowedTriSecondary = DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY & allowedMask;

	if (DrawLevelOvr1P_HandlerUsesDeepestCompactGrid(handlerAddress) && DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
		return DrawLevelOvr1P_DispatchProjectedGridDeepestCompact(pb, primMem, block, projected, faceIndex, depth, texture, writeClipBytes, allowedMask, slot,
		                                                          inheritedOtIndex);

	// NOTE(aalhendi): Non-4x1 ground helper tables share the 3x3 subdivision
	// topology used by water-rendered, but emit through ground direct handlers.
	// Retail carries the reduced scratch 0x70 mask into these child helpers.
	switch (slot)
	{
	case 0:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[0], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
			return 0;
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[1], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
			return 0;
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, writeClipBytes,
		                                            allowedQuad, inheritedOtIndex);
	case 1:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, writeClipBytes,
		                                          allowedQuad, inheritedOtIndex))
			return 0;
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
			return 0;
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                            writeClipBytes, allowedQuad, inheritedOtIndex);
	case 2:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
			return 0;
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, writeClipBytes,
		                                          allowedTriPrimary, inheritedOtIndex))
			return 0;
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, writeClipBytes,
		                                          allowedQuad, inheritedOtIndex))
			return 0;
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, writeClipBytes,
		                                            allowedQuad, inheritedOtIndex);
	case 3:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
			return 0;
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, writeClipBytes,
		                                          allowedQuad, inheritedOtIndex))
			return 0;
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                            writeClipBytes, allowedQuad, inheritedOtIndex);
	case 4:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
			return 0;
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, writeClipBytes,
		                                          allowedTriPrimary, inheritedOtIndex))
			return 0;
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, writeClipBytes,
		                                          allowedQuad, inheritedOtIndex))
			return 0;
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, writeClipBytes,
		                                            allowedQuad, inheritedOtIndex);
	case 7:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[2], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
			return 0;
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[3], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
			return 0;
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth, writeClipBytes,
		                                            allowedQuad, inheritedOtIndex);
	case 9:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
			return 0;
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, writeClipBytes,
		                                          allowedTriSecondary, inheritedOtIndex))
			return 0;
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, writeClipBytes,
		                                          allowedQuad, inheritedOtIndex))
			return 0;
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth, writeClipBytes,
		                                            allowedQuad, inheritedOtIndex);
	case 11:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
			return 0;
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, writeClipBytes,
		                                          allowedTriSecondary, inheritedOtIndex))
			return 0;
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, writeClipBytes,
		                                          allowedQuad, inheritedOtIndex))
			return 0;
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth, writeClipBytes,
		                                            allowedQuad, inheritedOtIndex);
	default:
		break;
	}

	DrawLevelOvr1P_SetGridFaceSlotWord(projected, DrawLevelOvr1P_GetDefaultGridFaceSlotWord(handlerAddress, 0));
	if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, writeClipBytes,
	                                          allowedQuad, inheritedOtIndex))
		return 0;
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, DrawLevelOvr1P_GetDefaultGridFaceSlotWord(handlerAddress, 1));
	if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, writeClipBytes,
	                                          allowedQuad, inheritedOtIndex))
		return 0;
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, DrawLevelOvr1P_GetDefaultGridFaceSlotWord(handlerAddress, 2));
	if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, writeClipBytes,
	                                          allowedQuad, inheritedOtIndex))
		return 0;
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, DrawLevelOvr1P_GetDefaultGridFaceSlotWord(handlerAddress, 3));
	return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth, writeClipBytes,
	                                            allowedQuad, inheritedOtIndex);
}

static int DrawLevelOvr1P_EmitProjectedGridNearClippedBridge(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                             const struct TextureLayout *texture, u32 nearMask, int depth, int writeClipBytes, u32 allowedMask,
                                                             int inheritedOtIndex)
{
	struct DrawLevelOvr1PScratchVertex *sub;
	u32 handlerAddress;

	if (depth >= DRAW_LEVEL_OVR1P_MAX_NEAR_SUBDIV_DEPTH)
	{
		if (writeClipBytes)
			return DrawLevelOvr1P_EmitDeepestProjectedDirectStoredMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, allowedMask,
			                                                               writeClipBytes, inheritedOtIndex);

		return DrawLevelOvr1P_EmitDeepestProjectedDirectAtOt(pb, primMem, block, projected, indices, faceIndex, texture, *CTR_SCRATCHPAD_PTR(u32, 0x194),
		                                                     writeClipBytes, allowedMask, inheritedOtIndex);
	}

	handlerAddress = DrawLevelOvr1P_GetNearSubdivisionHandlerAddress(nearMask, writeClipBytes);
	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	if (DrawLevelOvr1P_HandlerUses4x4GridFrame(handlerAddress))
		DrawLevelOvr1P_BuildGridSubdivisionFrame4x4(sub, projected, indices, writeClipBytes);
	else
		DrawLevelOvr1P_BuildGridSubdivisionFrame(sub, projected, indices, writeClipBytes);

	DrawLevelOvr1P_SetPreviousRecursiveHandler(handlerAddress);
	return DrawLevelOvr1P_DispatchProjectedGridHelper(pb, primMem, block, sub, faceIndex, depth + 1, texture, handlerAddress, writeClipBytes, allowedMask,
	                                                  inheritedOtIndex);
}

static int DrawLevelOvr1P_EmitProjectedNearClippedBridge(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                         const struct TextureLayout *texture, u32 nearMask, int depth, int writeClipBytes, u32 allowedMask,
                                                         int inheritedOtIndex)
{
	struct DrawLevelOvr1PScratchVertex *sub;
	const struct DrawLevelOvr1PNearSubdivisionCase *subdivisionCase;
	u32 handlerAddress;

	if (writeClipBytes && DrawLevelOvr1P_IsProjectedFaceFullyNear(projected, indices))
		return 1;

	if (depth >= DRAW_LEVEL_OVR1P_MAX_NEAR_SUBDIV_DEPTH)
	{
		if (writeClipBytes)
			return DrawLevelOvr1P_EmitDeepestProjectedDirectStoredMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, allowedMask,
			                                                               writeClipBytes, inheritedOtIndex);

		// NOTE(aalhendi): At the deepest retail scratch frame, 0x800a3f74 /
		// 0x800a4b54 stop recursing and fall through to the direct table.
		return DrawLevelOvr1P_EmitDeepestProjectedDirectAtOt(pb, primMem, block, projected, indices, faceIndex, texture, *CTR_SCRATCHPAD_PTR(u32, 0x194),
		                                                     writeClipBytes, allowedMask, inheritedOtIndex);
	}

	handlerAddress = DrawLevelOvr1P_GetNearSubdivisionHandlerAddress(nearMask, writeClipBytes);
	subdivisionCase = DrawLevelOvr1P_FindCompactNearSubdivisionCase(handlerAddress);
	if (subdivisionCase == NULL)
		return DrawLevelOvr1P_EmitProjectedGridNearClippedBridge(pb, primMem, block, projected, indices, faceIndex, texture, nearMask, depth, writeClipBytes,
		                                                         allowedMask, inheritedOtIndex);

	DrawLevelOvr1P_SetPreviousRecursiveHandler(handlerAddress);
	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);

	// TODO(aalhendi): Replace the shared native body with retail's exact
	// recursive handlers before ASM-stamping 0x800a3ef0/0x800a4ad0.
	// NOTE(aalhendi): Retail only writes clip bytes in the rendered helper
	// 0x800a4594; list helper 0x800a3a78 leaves them unused.
	DrawLevelOvr1P_BuildMidpointPair(&sub[0], &sub[1], &sub[4], &projected[indices[0]], &projected[indices[1]], writeClipBytes);
	DrawLevelOvr1P_BuildMidpointPair(&sub[2], &sub[3], &sub[5], &projected[indices[2]], &projected[indices[3]], writeClipBytes);

	for (int i = 0; i < 2; i++)
	{
		int subIndices[4];
		u32 subNearMask;

		for (int j = 0; j < 4; j++)
			subIndices[j] = subdivisionCase->subIndices[i][j];

		// NOTE(aalhendi): Retail 0x800a3a34/0x800a4550 retarget frame+0xb4
		// between the two compact quads; the other compact cases preserve it.
		if (subdivisionCase->slotWords[i] != DRAW_LEVEL_OVR1P_SLOT_WORD_PRESERVE)
			DrawLevelOvr1P_SetGridFaceSlotWord(sub, subdivisionCase->slotWords[i]);

		if (writeClipBytes && DrawLevelOvr1P_IsProjectedFaceFullyNear(sub, subIndices))
			continue;

		subNearMask = DrawLevelOvr1P_GetProjectedNearMaskForDepth(sub, subIndices, depth + 1);
		if (writeClipBytes && DrawLevelOvr1P_HasProjectedVertexNear(sub, subIndices, 4))
		{
			u32 renderedDirectMask = DRAW_LEVEL_OVR1P_DIRECT_QUAD;

			*CTR_SCRATCHPAD_PTR(u32, 0x70) = renderedDirectMask;
			if (subNearMask != 0)
			{
				if (!DrawLevelOvr1P_EmitProjectedNearClippedBridge(pb, primMem, block, sub, subIndices, faceIndex, texture, subNearMask, depth + 1,
				                                                   writeClipBytes, renderedDirectMask, inheritedOtIndex))
					return 0;
			}
			else if (!DrawLevelOvr1P_EmitPreparedProjectedDirectMaskAtOt(pb, primMem, block, sub, subIndices, faceIndex, texture, renderedDirectMask,
			                                                             writeClipBytes, 0, inheritedOtIndex))
			{
				return 0;
			}

			continue;
		}

		{
			u32 directMaskAllowed = subdivisionCase->directMasks[i] & allowedMask;
			u32 directMask;

			// NOTE(aalhendi): Retail compact helpers 0x800a3b90/0x800a46d0
			// store the subface allowance, cull offscreen, then select 0x70.
			*CTR_SCRATCHPAD_PTR(u32, 0x70) = directMaskAllowed;

			if (DrawLevelOvr1P_IsProjectedFaceOffscreen(pb, sub, subIndices))
				continue;

			directMask = DrawLevelOvr1P_SelectAndStoreDirectMask(block, sub, subIndices, *CTR_SCRATCHPAD_PTR(u32, 0x194), directMaskAllowed);
			if (directMask == 0)
				continue;

			if (subNearMask != 0)
			{
				if (!DrawLevelOvr1P_EmitProjectedNearClippedBridge(pb, primMem, block, sub, subIndices, faceIndex, texture, subNearMask, depth + 1,
				                                                   writeClipBytes, directMask, inheritedOtIndex))
					return 0;
			}
			else if (!DrawLevelOvr1P_EmitPreparedProjectedDirectMaskAtOt(pb, primMem, block, sub, subIndices, faceIndex, texture, directMask, writeClipBytes, 0,
			                                                             inheritedOtIndex))
			{
				return 0;
			}
		}
	}

	return 1;
}

static int DrawLevelOvr1P_EmitProjectedGroundQuadAllowed(struct PushBuffer *pb, struct PrimMem *primMem, struct DrawLevelOvr1PScratchVertex *projected,
                                                         struct QuadBlock *block, const int *indices, int faceIndex, u32 tableWord, int writeClipBytes,
                                                         u32 allowedMask)
{
	struct TextureLayout *texture;
	u32 directMask;
	u32 maxDepth;
	u32 nearMask;
	int inheritedOtIndex;

	if (writeClipBytes && DrawLevelOvr1P_IsProjectedFaceFullyNear(projected, indices))
		return 1;

	// NOTE(aalhendi): Retail rendered helpers carry mask 0xc into near recursion here.
	if (writeClipBytes && DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		directMask = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
		*CTR_SCRATCHPAD_PTR(u32, 0x70) = directMask;
		// NOTE(aalhendi): Selector-style helpers refresh the clipped-record
		// header only after the surviving direct mask is committed.
		DrawLevelOvr1P_StoreRenderedClipRecordHeader(tableWord);

		maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices);
		inheritedOtIndex = DrawLevelOvr1P_GetProjectedOtIndex(block, projected, maxDepth, faceIndex);
		texture = DrawLevelOvr1P_GetProjectedMidTexture(block, projected, faceIndex, maxDepth);

		if (texture != 0)
			DrawLevelOvr1P_WriteProjectedUv(projected, indices, texture, tableWord);

		nearMask = DrawLevelOvr1P_GetProjectedNearMaskForMode(projected, indices, writeClipBytes);
		if (nearMask != 0)
			return DrawLevelOvr1P_EmitProjectedNearClippedBridge(pb, primMem, block, projected, indices, faceIndex, texture, nearMask, 0, writeClipBytes,
			                                                     directMask, inheritedOtIndex);

		return DrawLevelOvr1P_EmitPreparedProjectedDirectMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes, 0,
		                                                          inheritedOtIndex);
	}

	if (DrawLevelOvr1P_IsProjectedFaceOffscreen(pb, projected, indices))
		return 1;

	// NOTE(aalhendi): Retail 4x1 projected helpers reject zero direct masks
	// before selecting UVs or recursing into the near-subdivision table.
	directMask = DrawLevelOvr1P_SelectAndStoreDirectMask(block, projected, indices, tableWord, allowedMask);
	if (directMask == 0)
		return 1;

	if (writeClipBytes)
		DrawLevelOvr1P_StoreRenderedClipRecordHeader(tableWord);

	maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices);
	inheritedOtIndex = DrawLevelOvr1P_GetProjectedOtIndex(block, projected, maxDepth, faceIndex);
	texture = DrawLevelOvr1P_GetProjectedMidTexture(block, projected, faceIndex, maxDepth);

	if (texture != 0)
		DrawLevelOvr1P_WriteProjectedUv(projected, indices, texture, tableWord);

	nearMask = DrawLevelOvr1P_GetProjectedNearMaskForMode(projected, indices, writeClipBytes);
	if (nearMask != 0)
		return DrawLevelOvr1P_EmitProjectedNearClippedBridge(pb, primMem, block, projected, indices, faceIndex, texture, nearMask, 0, writeClipBytes,
		                                                     directMask, inheritedOtIndex);

	return DrawLevelOvr1P_EmitPreparedProjectedDirectMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes, 0,
	                                                          inheritedOtIndex);
}

static int DrawLevelOvr1P_EmitProjectedGroundQuad(struct PushBuffer *pb, struct PrimMem *primMem, struct DrawLevelOvr1PScratchVertex *projected,
                                                  struct QuadBlock *block, const int *indices, int faceIndex, int numFaces, u32 tableWord, int writeClipBytes)
{
	POLY_GT4 *prim;
	POLY_GT4 *primCurr;
	void *nextPrim;
	struct TextureLayout *texture;
	u32 drawOrderBits;
	u32 rotation;
	u32 faceMode;
	u32 maxDepth;
	u32 nearMask;
	int boolPassCull;
	int otIndex;

	if (numFaces == 4)
	{
		// NOTE(aalhendi): Retail 4x1/projected helpers select the mid texture
		// only after the direct-mask/offscreen preflight, because selection
		// writes scratch 0x84 for deepest UV reloads.
		return DrawLevelOvr1P_EmitProjectedGroundQuadAllowed(pb, primMem, projected, block, indices, faceIndex, tableWord, writeClipBytes,
		                                                     DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	}

	if (DrawLevelOvr1P_IsProjectedFaceOffscreen(pb, projected, indices))
		return 1;

	maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices);
	texture = DrawLevelOvr1P_GetTexture(block, faceIndex, numFaces);

	prim = primMem->curr;
	nextPrim = prim + 1;
	primCurr = prim;

	*(u32 *)&prim->r0 = *(u32 *)&projected[indices[0]].color_hi[0];
	*(u32 *)&prim->r1 = *(u32 *)&projected[indices[1]].color_hi[0];
	*(u32 *)&prim->r2 = *(u32 *)&projected[indices[2]].color_hi[0];
	*(u32 *)&prim->r3 = *(u32 *)&projected[indices[3]].color_hi[0];
	setPolyGT4(prim);
	setXY4(prim, projected[indices[0]].posScreen[0], projected[indices[0]].posScreen[1], projected[indices[1]].posScreen[0], projected[indices[1]].posScreen[1],
	       projected[indices[2]].posScreen[0], projected[indices[2]].posScreen[1], projected[indices[3]].posScreen[0], projected[indices[3]].posScreen[1]);

	prim->clut = 0;
	prim->tpage = 0;
	if (texture != 0)
	{
		drawOrderBits = (block->draw_order_low >> (8 + faceIndex * 5)) & 0x1f;
		rotation = drawOrderBits & 7;
		faceMode = (drawOrderBits >> 3) & 3;
		DrawLevelOvr1P_SetUvRotation(prim, texture, rotation);
		DrawLevelOvr1P_ApplyFaceMode(prim, faceMode);

		prim->clut = texture->clut;
		prim->tpage = texture->tpage;
	}

	boolPassCull = (block->draw_order_low & 0x80000000) != 0;
	if (!boolPassCull)
		boolPassCull = DrawLevelOvr1P_NclipProjected(projected, indices) >= 0;

	if (!boolPassCull)
		return 1;

	otIndex = DrawLevelOvr1P_GetProjectedOtIndex(block, projected, maxDepth, faceIndex);

	if (DrawLevelOvr1P_ShouldSetSemiTransFromTpage(prim->tpage))
		setSemiTrans(prim, true);

	AddPrim(&pb->ptrOT[otIndex], primCurr);
	primMem->curr = nextPrim;
	return 1;
}

static void DrawLevelOvr1P_SetRenderedListCursor(struct QuadBlock **renderedList)
{
	*CTR_SCRATCHPAD_PTR(u32, 0x64) = (u32)(uintptr_t)renderedList;
}

static struct QuadBlock **DrawLevelOvr1P_GetRenderedListCursor(void)
{
	return (struct QuadBlock **)(uintptr_t)*CTR_SCRATCHPAD_PTR(u32, 0x64);
}

static void DrawLevelOvr1P_AppendRenderedQuadBlock(struct QuadBlock *block)
{
	struct QuadBlock **renderedList = DrawLevelOvr1P_GetRenderedListCursor();

	if (renderedList == NULL)
		return;

	*renderedList = block;
	DrawLevelOvr1P_SetRenderedListCursor(renderedList + 1);
}

static void DrawLevelOvr1P_TerminateRenderedListCursor(void)
{
	struct QuadBlock **renderedList = DrawLevelOvr1P_GetRenderedListCursor();

	if (renderedList != NULL)
		*renderedList = NULL;
}

static int DrawLevelOvr1P_DrawWaterListQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh, struct QuadBlock *block,
                                                 int captureRenderedOverflow);

static void DrawLevelOvr1P_ClearProjectedUv(struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	*CTR_SCRATCHPAD_PTR(u32, 0x1a0) = 0;
	*CTR_SCRATCHPAD_PTR(u32, 0x1a4) = 0;
	*CTR_SCRATCHPAD_PTR(u32, 0x1a8) = 0;
	*CTR_SCRATCHPAD_PTR(u32, 0x1ac) = 0;
	*CTR_SCRATCHPAD_PTR(u32, 0x1b0) = 0;

	for (int i = 0; i < 4; i++)
		projected[indices[i]].flags = 0;
}

static struct TextureLayout *DrawLevelOvr1P_PrepareFullDynamicLowUv(struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected)
{
	const int *indices = sDrawLevelOvr1PFullDynamicLowIndices;
	struct TextureLayout *texture = block->ptr_texture_low;

	// NOTE(aalhendi): Retail full-dynamic 0x800a0ef4 seeds low-LOD UVs before
	// choosing either the direct low quad or the near/transition helper table.
	*CTR_SCRATCHPAD_PTR(u32, 0x194) = 0;
	DrawLevelOvr1P_SetActiveDrawOrderLow(block);
	DrawLevelOvr1P_SetGridFaceSlot(projected, 0);

	if (texture != NULL)
		DrawLevelOvr1P_WriteProjectedUv(projected, indices, texture, 0);
	else
		DrawLevelOvr1P_ClearProjectedUv(projected, indices);

	return texture;
}

static int DrawLevelOvr1P_EmitFullDynamicLowQuad(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                 struct DrawLevelOvr1PScratchVertex *projected)
{
	const int *indices = sDrawLevelOvr1PFullDynamicLowIndices;
	struct TextureLayout *texture = DrawLevelOvr1P_PrepareFullDynamicLowUv(block, projected);

	return DrawLevelOvr1P_EmitPreparedProjectedDirect(pb, primMem, block, projected, indices, -1, texture, 0, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST,
	                                                  DRAW_LEVEL_OVR1P_DIRECT_QUAD);
}

static u16 DrawLevelOvr1P_AverageFullDynamicUv(u16 uvA, u16 uvB)
{
	u8 u = (u8)((((u32)uvA & 0xff) + ((u32)uvB & 0xff)) >> 1);
	u8 v = (u8)((((u32)uvA >> 8) + ((u32)uvB >> 8)) >> 1);

	return DrawLevelOvr1P_PackUv(u, v);
}

static void DrawLevelOvr1P_AdjustFullDynamicMidUvs(struct DrawLevelOvr1PScratchVertex *projected)
{
	projected[4].flags = DrawLevelOvr1P_AverageFullDynamicUv(projected[0].flags, projected[1].flags);
	projected[5].flags = DrawLevelOvr1P_AverageFullDynamicUv(projected[0].flags, projected[2].flags);
	projected[6].flags = DrawLevelOvr1P_AverageFullDynamicUv(projected[1].flags, projected[2].flags);
	projected[7].flags = DrawLevelOvr1P_AverageFullDynamicUv(projected[1].flags, projected[3].flags);
	projected[8].flags = DrawLevelOvr1P_AverageFullDynamicUv(projected[2].flags, projected[3].flags);
}

static int DrawLevelOvr1P_EmitFullDynamicHelperSequence(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                        struct DrawLevelOvr1PScratchVertex *projected, u32 nearMask, struct TextureLayout *texture, int depth);

static int DrawLevelOvr1P_EmitFullDynamicTerminalNearBridge(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                            struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                            struct TextureLayout *texture, u32 nearMask, int depth, u32 directMask)
{
	struct DrawLevelOvr1PScratchVertex *sub;
	u32 handlerAddress;

	if (depth >= DRAW_LEVEL_OVR1P_MAX_NEAR_SUBDIV_DEPTH)
		return DrawLevelOvr1P_EmitDeepestProjectedDirectMask(pb, primMem, block, projected, indices, faceIndex, texture, directMask,
		                                                     DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST);

	handlerAddress = DrawLevelOvr1P_GetNearSubdivisionHandlerAddress(nearMask, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST);
	DrawLevelOvr1P_SetPreviousRecursiveHandler(handlerAddress);

	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	DrawLevelOvr1P_BuildGridSubdivisionFrame(sub, projected, indices, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST);

	return DrawLevelOvr1P_EmitFullDynamicHelperSequence(pb, primMem, block, sub, nearMask, texture, depth + 1);
}

static int DrawLevelOvr1P_EmitFullDynamicTerminalFaceSlotMode(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                              struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                              struct TextureLayout *texture, int depth, u32 allowedMask, int updateSlot)
{
	u32 directMask;
	u32 nearMask;

	if (updateSlot)
		DrawLevelOvr1P_SetGridFaceSlot(projected, faceIndex);
	*CTR_SCRATCHPAD_PTR(u32, 0x70) = allowedMask;

	if (DrawLevelOvr1P_IsProjectedFaceOffscreen(pb, projected, indices))
		return 1;

	// NOTE(aalhendi): Retail 0x800a18c0 rejects empty direct masks before near recursion.
	directMask = DrawLevelOvr1P_SelectAndStoreDirectMask(block, projected, indices, *CTR_SCRATCHPAD_PTR(u32, 0x194), allowedMask);
	if (directMask == 0)
		return 1;

	nearMask = DrawLevelOvr1P_GetProjectedNearMaskForDepth(projected, indices, depth);
	if (nearMask != 0)
		return DrawLevelOvr1P_EmitFullDynamicTerminalNearBridge(pb, primMem, block, projected, indices, faceIndex, texture, nearMask, depth, directMask);

	return DrawLevelOvr1P_EmitPreparedProjectedDirectMaskRaw(pb, primMem, block, projected, indices, faceIndex, texture, directMask);
}

static int DrawLevelOvr1P_EmitFullDynamicTerminalFace(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                      struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                      struct TextureLayout *texture, int depth, u32 allowedMask)
{
	return DrawLevelOvr1P_EmitFullDynamicTerminalFaceSlotMode(pb, primMem, block, projected, indices, faceIndex, texture, depth, allowedMask, 1);
}

static int DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                                  struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                                  struct TextureLayout *texture, int depth, u32 allowedMask)
{
	return DrawLevelOvr1P_EmitFullDynamicTerminalFaceSlotMode(pb, primMem, block, projected, indices, faceIndex, texture, depth, allowedMask, 0);
}

static int DrawLevelOvr1P_EmitFullDynamicSelectorFace(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                      struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, int depth, u32 allowedMask)
{
	struct TextureLayout *texture;
	int indices[4];
	u32 maxDepth;
	u32 tableWord;

	tableWord = DrawLevelOvr1P_Select4x1ProjectedFace(projected, block, faceIndex, indices);

	if (DrawLevelOvr1P_IsProjectedFaceOffscreen(pb, projected, indices))
		return 1;

	maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices);
	texture = DrawLevelOvr1P_GetProjectedMidTexture(block, projected, faceIndex, maxDepth);

	if (texture != NULL)
		DrawLevelOvr1P_WriteProjectedUv(projected, indices, texture, tableWord);

	return DrawLevelOvr1P_EmitFullDynamicTerminalFace(pb, primMem, block, projected, indices, faceIndex, texture, depth, allowedMask);
}

static int DrawLevelOvr1P_EmitFullDynamicAllFacesBridge(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                        struct DrawLevelOvr1PScratchVertex *projected, int depth, struct TextureLayout *texture)
{
	for (int faceIndex = 0; faceIndex < 4; faceIndex++)
	{
		if (depth == 0)
		{
			if (!DrawLevelOvr1P_EmitFullDynamicSelectorFace(pb, primMem, block, projected, faceIndex, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
				return 0;
		}
		else if (!DrawLevelOvr1P_EmitFullDynamicTerminalFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[faceIndex], faceIndex, texture,
		                                                     depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
	}

	return 1;
}

static int DrawLevelOvr1P_EmitFullDynamicHelperSequence(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                        struct DrawLevelOvr1PScratchVertex *projected, u32 nearMask, struct TextureLayout *texture, int depth)
{
	u32 handlerAddress = DrawLevelOvr1P_GetNearSubdivisionHandlerAddress(nearMask, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST);
	int slot = DrawLevelOvr1P_GetCopiedRecursiveHandlerSlot(handlerAddress);

	// NOTE(aalhendi): Non-default full-dynamic helpers preserve frame+0xb4; default/base helpers refresh it, matching 0x800a1534..0x800a1734.
	switch (slot)
	{
	case 0:
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], 1, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], 2, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], 0, texture, depth,
		                                                              DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 1:
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], 1, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], 2, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], 3, texture, depth,
		                                                              DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 2:
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], 3, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], 2, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY))
			return 0;
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], 0, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], 1, texture, depth,
		                                                              DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 3:
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], 0, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], 2, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], 1, texture, depth,
		                                                              DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 4:
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], 1, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], 1, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY))
			return 0;
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], 0, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], 2, texture, depth,
		                                                              DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 7:
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[2], 0, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[3], 0, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], 3, texture, depth,
		                                                              DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 9:
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], 2, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], 2, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY))
			return 0;
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], 1, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], 3, texture, depth,
		                                                              DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 11:
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], 0, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], 1, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY))
			return 0;
		if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], 2, texture, depth,
		                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], 3, texture, depth,
		                                                              DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	default:
		return DrawLevelOvr1P_EmitFullDynamicAllFacesBridge(pb, primMem, block, projected, depth, texture);
	}
}

static int DrawLevelOvr1P_EmitFullDynamicTransitionQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                             struct LevVertex *vertices, struct DrawLevelOvr1PScratchVertex *projected, u32 nearMask)
{
	struct TextureLayout *texture = DrawLevelOvr1P_PrepareFullDynamicLowUv(block, projected);

	DrawLevelOvr1P_AdjustFullDynamicMidVertices(projected, vertices, block);
	DrawLevelOvr1P_AdjustFullDynamicMidUvs(projected);

	// TODO(aalhendi): Replace this bridge with retail's full-dynamic
	// near/transition helper bodies; terminal direct packets now use a raw
	// full-dynamic dispatch bridge, but exact OT/register side effects remain.
	return DrawLevelOvr1P_EmitFullDynamicHelperSequence(pb, primMem, block, projected, nearMask, texture, 0);
}

static int DrawLevelOvr1P_EmitFullDynamicQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block, struct LevVertex *vertices,
                                                   struct DrawLevelOvr1PScratchVertex *projected)
{
	u32 nearMask = DrawLevelOvr1P_GetProjectedNearMask(projected, sDrawLevelOvr1PFullDynamicLowIndices);

	if (nearMask != 0)
	{
		// NOTE(aalhendi): The high/mid transition records are only needed once
		// the low quad takes the full-dynamic near path.
		(void)DrawLevelOvr1P_ProjectQuadBlockGrid(vertices, block, projected, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST);
		return DrawLevelOvr1P_EmitFullDynamicTransitionQuadBlock(pb, primMem, block, vertices, projected, nearMask);
	}

	return DrawLevelOvr1P_EmitFullDynamicLowQuad(pb, primMem, block, projected);
}

static int DrawLevelOvr1P_EmitQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh, struct BSP *bsp, struct QuadBlock *block,
                                        const int *visFaceList, int role, int lodMode, int captureRenderedOverflow)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();
	int numFaces = 4;
	const int *indices = sDrawLevelOvr1PHighLodIndices;

	if ((block->quadFlags & 0x8000) != 0)
		return 1;

	if (!DrawLevelOvr1P_IsFaceVisible(visFaceList, block))
		return 1;

	if ((bsp->flag & 2) != 0)
		return DrawLevelOvr1P_DrawWaterListQuadBlock(pb, primMem, mesh, block, captureRenderedOverflow);

	DrawLevelOvr1P_SetActiveDrawOrderLow(block);

	if (role == DRAW_LEVEL_OVR1P_BUCKET_FULL_DYNAMIC_LIST)
	{
		DrawLevelOvr1P_ProjectFullDynamicLowQuad(vertices, block, projected);
		return DrawLevelOvr1P_EmitFullDynamicQuadBlock(pb, primMem, block, vertices, projected);
	}

	if (DrawLevelOvr1P_ProjectQuadBlockGrid(vertices, block, projected, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST))
	{
		if (captureRenderedOverflow)
		{
			DrawLevelOvr1P_AppendRenderedQuadBlock(block);
			return 1;
		}
	}

	if (lodMode == DRAW_LEVEL_OVR1P_LOD_LOW)
	{
		numFaces = 1;
		indices = sDrawLevelOvr1PLowLodIndices;
	}

	for (int faceIndex = 0; faceIndex < numFaces; faceIndex++)
	{
		int selected[4];
		u32 tableWord;

		if (numFaces == 4)
		{
			tableWord = DrawLevelOvr1P_Select4x1ProjectedFace(projected, block, faceIndex, selected);
		}
		else
		{
			for (int i = 0; i < 4; i++)
				selected[i] = indices[i];

			tableWord = 0;
		}

		// TODO(aalhendi): ASM-port the exact retail 4x2 and dynamic-subdiv
		// helper bodies. This bridge now uses the active bucket's copied direct
		// and recursive tables instead of bypassing them with immediate GT4s.
		if (!DrawLevelOvr1P_EmitProjectedGroundQuad(pb, primMem, projected, block, selected, faceIndex, numFaces, tableWord, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST))
			return 0;
	}

	return 1;
}

static int DrawLevelOvr1P_DrawBspChain(struct VisMemBspListNode *slot, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                       const int *visFaceList, int role, int lodMode, int captureRenderedOverflow, u32 primReserve)
{
	while (slot != 0)
	{
		struct BSP *bsp = slot->bsp;

		if (bsp != 0)
		{
			for (int quadIndex = 0; quadIndex < bsp->data.leaf.numQuads; quadIndex++)
			{
				if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, primReserve))
					return 0;

				if (!DrawLevelOvr1P_EmitQuadBlock(pb, primMem, mesh, bsp, &bsp->data.leaf.ptrQuadBlockArray[quadIndex], visFaceList, role, lodMode,
				                                  captureRenderedOverflow))
					return 0;
			}
		}

		slot = slot->next;
	}

	return 1;
}

static int DrawLevelOvr1P_Emit4x1ListQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh, struct BSP *bsp, struct QuadBlock *block,
                                               const int *visFaceList)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	if ((block->quadFlags & 0x8000) != 0)
		return 1;

	if (!DrawLevelOvr1P_IsFaceVisible(visFaceList, block))
		return 1;

	if ((bsp->flag & 2) != 0)
		return DrawLevelOvr1P_DrawWaterListQuadBlock(pb, primMem, mesh, block, 1);

	DrawLevelOvr1P_SetActiveDrawOrderLow(block);

	if (DrawLevelOvr1P_ProjectQuadBlockGrid(vertices, block, projected, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST))
	{
		DrawLevelOvr1P_AppendRenderedQuadBlock(block);
		return 1;
	}

	for (int faceIndex = 0; faceIndex < 4; faceIndex++)
	{
		int indices[4];
		u32 tableWord;

		// TODO(aalhendi): Replace this bridge with the retail 0x800a36a8..0x800a4178 clip/subdivision helper path.
		tableWord = DrawLevelOvr1P_Select4x1ProjectedFace(projected, block, faceIndex, indices);
		if (!DrawLevelOvr1P_EmitProjectedGroundQuad(pb, primMem, projected, block, indices, faceIndex, 4, tableWord, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST))
			return 0;
	}

	return 1;
}

static int DrawLevelOvr1P_Draw4x1BspChain(struct VisMemBspListNode *slot, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                          const int *visFaceList)
{
	while (slot != 0)
	{
		struct BSP *bsp = slot->bsp;

		if (bsp != 0)
		{
			for (int quadIndex = 0; quadIndex < bsp->data.leaf.numQuads; quadIndex++)
			{
				if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, DRAW_LEVEL_OVR1P_BUCKET_RESERVE_4X1))
					return 0;

				if (!DrawLevelOvr1P_Emit4x1ListQuadBlock(pb, primMem, mesh, bsp, &bsp->data.leaf.ptrQuadBlockArray[quadIndex], visFaceList))
					return 0;
			}
		}

		slot = slot->next;
	}

	return 1;
}

static int DrawLevelOvr1P_Draw4x1Rendered(struct QuadBlock **renderedList, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	if (renderedList == NULL)
		return 1;

	while (*renderedList != NULL)
	{
		if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, DRAW_LEVEL_OVR1P_BUCKET_RESERVE_4X1))
			return 0;

		DrawLevelOvr1P_SetActiveDrawOrderLow(*renderedList);

		(void)DrawLevelOvr1P_ProjectQuadBlockGrid(vertices, *renderedList, projected, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);

		for (int faceIndex = 0; faceIndex < 4; faceIndex++)
		{
			int indices[4];
			u32 tableWord;

			// TODO(aalhendi): Replace this bridge with the retail scratch subdivision helper at 0x800a417c..0x800a4cc8.
			tableWord = DrawLevelOvr1P_Select4x1ProjectedFace(projected, *renderedList, faceIndex, indices);
			if (DrawLevelOvr1P_IsProjectedFaceFullyNear(projected, indices))
				continue;

			if (!DrawLevelOvr1P_EmitProjectedGroundQuad(pb, primMem, projected, *renderedList, indices, faceIndex, 4, tableWord,
			                                            DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED))
				return 0;
		}

		renderedList++;
	}

	return 1;
}

static struct QuadBlock **DrawLevelOvr1P_GetRenderedListForRole(struct DrawLevelOvr1PRenderList *renderList, int role)
{
	switch (role)
	{
	case DRAW_LEVEL_OVR1P_BUCKET_4X4_LIST:
	case DRAW_LEVEL_OVR1P_BUCKET_4X4_RENDERED:
		return renderList->list[0].ptrQuadBlocksRendered;
	case DRAW_LEVEL_OVR1P_BUCKET_DYNAMIC_LIST:
	case DRAW_LEVEL_OVR1P_BUCKET_DYNAMIC_RENDERED:
		return renderList->list[1].ptrQuadBlocksRendered;
	case DRAW_LEVEL_OVR1P_BUCKET_4X2_LIST:
	case DRAW_LEVEL_OVR1P_BUCKET_4X2_RENDERED:
		return renderList->list[2].ptrQuadBlocksRendered;
	case DRAW_LEVEL_OVR1P_BUCKET_4X1_LIST:
	case DRAW_LEVEL_OVR1P_BUCKET_4X1_RENDERED:
		return renderList->list[3].ptrQuadBlocksRendered;
	case DRAW_LEVEL_OVR1P_BUCKET_WATER_LIST:
	case DRAW_LEVEL_OVR1P_BUCKET_WATER_RENDERED:
		return renderList->list[4].ptrQuadBlocksRendered;
	default:
		return NULL;
	}
}

static u32 DrawLevelOvr1P_SelectWaterDirectMask(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, u32 allowedMask)
{
	u32 directMask = 0;

	if ((DrawLevelOvr1P_GetProjectedColorWord(&projected[indices[0]]) | DrawLevelOvr1P_GetProjectedColorWord(&projected[indices[1]]) |
	     DrawLevelOvr1P_GetProjectedColorWord(&projected[indices[2]])) != 0)
		directMask |= DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY;

	if ((DrawLevelOvr1P_GetProjectedColorWord(&projected[indices[1]]) | DrawLevelOvr1P_GetProjectedColorWord(&projected[indices[2]]) |
	     DrawLevelOvr1P_GetProjectedColorWord(&projected[indices[3]])) != 0)
		directMask |= DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY;

	return directMask & allowedMask;
}

static u32 DrawLevelOvr1P_SelectAndStoreWaterDirectMask(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, u32 allowedMask)
{
	u32 directMask = DrawLevelOvr1P_SelectWaterDirectMask(projected, indices, allowedMask);

	*CTR_SCRATCHPAD_PTR(u32, 0x70) = directMask;
	return directMask;
}

static void DrawLevelOvr1P_BuildGridSubdivisionFrame(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                     const int *indices, int writeClipBytes)
{
	DrawLevelOvr1P_BuildMidpointPair(&sub[0], &sub[1], &sub[4], &projected[indices[0]], &projected[indices[1]], writeClipBytes);
	DrawLevelOvr1P_BuildMidpointPair(&sub[1], &sub[3], &sub[7], &projected[indices[1]], &projected[indices[3]], writeClipBytes);
	DrawLevelOvr1P_BuildMidpointPair(&sub[2], &sub[0], &sub[5], &projected[indices[2]], &projected[indices[0]], writeClipBytes);
	DrawLevelOvr1P_BuildMidpointPair(&sub[3], &sub[2], &sub[8], &projected[indices[3]], &projected[indices[2]], writeClipBytes);
	DrawLevelOvr1P_BuildMidpointPair(&sub[1], &sub[2], &sub[6], &projected[indices[1]], &projected[indices[2]], writeClipBytes);
}

static void DrawLevelOvr1P_BuildGridSubdivisionFrame4x4(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                        const int *indices, int writeClipBytes)
{
	DrawLevelOvr1P_BuildMidpointPair(&sub[0], &sub[1], &sub[4], &projected[indices[0]], &projected[indices[1]], writeClipBytes);
	DrawLevelOvr1P_BuildMidpointPair(&sub[1], &sub[3], &sub[7], &projected[indices[1]], &projected[indices[3]], writeClipBytes);
	DrawLevelOvr1P_BuildMidpointPair(&sub[2], &sub[0], &sub[5], &projected[indices[2]], &projected[indices[0]], writeClipBytes);
	DrawLevelOvr1P_BuildMidpointPair(&sub[3], &sub[2], &sub[8], &projected[indices[3]], &projected[indices[2]], writeClipBytes);
	// NOTE(aalhendi): Retail 4x4 helpers build the center from edge midpoints.
	DrawLevelOvr1P_BuildMidpointPair(&sub[4], &sub[8], &sub[6], &sub[4], &sub[8], writeClipBytes);
}

static int DrawLevelOvr1P_EmitWaterListProjectedFace(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                     const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex, int depth,
                                                     u32 allowedMask);

static int DrawLevelOvr1P_DispatchWaterListHelper(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                  const struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, int depth, u32 handlerAddress,
                                                  u32 allowedMask)
{
	int slot = DrawLevelOvr1P_GetCopiedRecursiveHandlerSlot(handlerAddress);

	// NOTE(aalhendi): Water-list uses the same 3x3 helper topology as
	// water-rendered, but emits through the 0x800a27dc/0x800a27d4/0x800a2850
	// direct family without rendered clip-byte record diversion.
	switch (slot)
	{
	case 0:
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[0], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[1], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, depth,
		                                                 DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 1:
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, depth,
		                                                 DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 2:
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY))
			return 0;
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, depth,
		                                                 DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 3:
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, depth,
		                                                 DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 4:
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY))
			return 0;
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, depth,
		                                                 DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 7:
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[2], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[3], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, depth,
		                                                 DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 9:
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY))
			return 0;
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, depth,
		                                                 DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 11:
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY))
			return 0;
		if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, depth,
		                                               DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, depth,
		                                                 DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	default:
		break;
	}

	DrawLevelOvr1P_SetGridFaceSlot(projected, 0);
	if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], 0, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
		return 0;
	DrawLevelOvr1P_SetGridFaceSlot(projected, 1);
	if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], 1, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
		return 0;
	DrawLevelOvr1P_SetGridFaceSlot(projected, 2);
	if (!DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], 2, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
		return 0;
	DrawLevelOvr1P_SetGridFaceSlot(projected, 3);
	return DrawLevelOvr1P_EmitWaterListProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], 3, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
}

static int DrawLevelOvr1P_EmitWaterListNearBridge(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                  const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex, int depth,
                                                  u32 handlerAddress, u32 allowedMask)
{
	struct DrawLevelOvr1PScratchVertex *sub;

	if (depth >= DRAW_LEVEL_OVR1P_MAX_NEAR_SUBDIV_DEPTH)
	{
		u32 directMask = DrawLevelOvr1P_SelectAndStoreWaterDirectMask(projected, indices, allowedMask);

		return DrawLevelOvr1P_EmitPreparedProjectedDirectMask(pb, primMem, block, projected, indices, faceIndex, NULL, directMask,
		                                                      DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST, 0);
	}

	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	// NOTE(aalhendi): Retail 0x800a24e8 builds the water-list subdivision
	// frame without rendered clip bytes, then jumps through scratch 0x14c.
	DrawLevelOvr1P_BuildGridSubdivisionFrame(sub, projected, indices, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST);

	return DrawLevelOvr1P_DispatchWaterListHelper(pb, primMem, block, sub, faceIndex, depth + 1, handlerAddress, allowedMask);
}

static int DrawLevelOvr1P_EmitWaterListProjectedFace(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                     const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex, int depth,
                                                     u32 allowedMask)
{
	u32 directMask;
	u32 nearMask;

	if (DrawLevelOvr1P_IsProjectedFaceOffscreen(pb, projected, indices))
		return 1;

	// NOTE(aalhendi): Retail water-list helper 0x800a25d0 stores the color-derived direct mask before near recursion.
	directMask = DrawLevelOvr1P_SelectAndStoreWaterDirectMask(projected, indices, allowedMask);
	if (directMask == 0)
		return 1;

	nearMask = DrawLevelOvr1P_GetProjectedNearMaskForDepth(projected, indices, depth);
	if (nearMask != 0)
	{
		u32 handlerAddress = DrawLevelOvr1P_GetNearSubdivisionHandlerAddress(nearMask, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST);

		DrawLevelOvr1P_SetPreviousRecursiveHandler(handlerAddress);
		return DrawLevelOvr1P_EmitWaterListNearBridge(pb, primMem, block, projected, indices, faceIndex, depth, handlerAddress, directMask);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedDirectMask(pb, primMem, block, projected, indices, faceIndex, NULL, directMask, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST,
	                                                      0);
}

static int DrawLevelOvr1P_DrawWaterListQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh, struct QuadBlock *block,
                                                 int captureRenderedOverflow)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	if ((block->quadFlags & 0x8000) != 0)
		return 1;

	if (DrawLevelOvr1P_ProjectQuadBlockGrid(vertices, block, projected, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST))
	{
		if (captureRenderedOverflow)
			DrawLevelOvr1P_AppendRenderedQuadBlock(block);

		return 1;
	}

	DrawLevelOvr1P_ApplyWaterRenderedColorFades(projected);

	// TODO(aalhendi): Replace this bridge with the exact
	// 0x800a2344..0x800a2850 water-list recursive/direct helper bodies.
	return DrawLevelOvr1P_DispatchWaterListHelper(pb, primMem, block, projected, 0, 0, 0x800a24a4, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
}

static int DrawLevelOvr1P_EmitWaterRenderedProjectedFace(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex, int depth,
                                                         u32 allowedMask);

static int DrawLevelOvr1P_DispatchWaterRenderedHelper(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, int depth, u32 handlerAddress,
                                                      u32 allowedMask)
{
	// NOTE(aalhendi): Retail water-rendered recursion jumps through the copied
	// 0x14c table. Native keeps the helper bodies as address-keyed C cases.
	switch (handlerAddress)
	{
	case 0x800a2da0:
		return DrawLevelOvr1P_EmitWaterRenderedProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, depth, allowedMask);
	case 0x800a2db4:
		return DrawLevelOvr1P_EmitWaterRenderedProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, depth, allowedMask);
	case 0x800a2dc8:
		return DrawLevelOvr1P_EmitWaterRenderedProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, depth, allowedMask);
	case 0x800a2ddc:
		return DrawLevelOvr1P_EmitWaterRenderedProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, depth, allowedMask);
	case 0x800a2df0:
		return DrawLevelOvr1P_EmitWaterRenderedProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, depth,
		                                                     allowedMask);
	case 0x800a2e04:
		return DrawLevelOvr1P_EmitWaterRenderedProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, depth,
		                                                     allowedMask);
	case 0x800a2e18:
		return DrawLevelOvr1P_EmitWaterRenderedProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, depth,
		                                                     allowedMask);
	case 0x800a2e2c:
		return DrawLevelOvr1P_EmitWaterRenderedProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, depth,
		                                                     allowedMask);
	case 0x800a2e40:
		if (!DrawLevelOvr1P_EmitWaterRenderedProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[0], faceIndex, depth,
		                                                   DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitWaterRenderedProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[1], faceIndex, depth,
		                                                   DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2da0, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 0x800a2e80:
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2db4, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2e18, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2e2c, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 0x800a2ea0:
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2df0, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2dc8, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2e04, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 0x800a2ec0:
		if (!DrawLevelOvr1P_EmitWaterRenderedProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[2], faceIndex, depth,
		                                                   DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_EmitWaterRenderedProjectedFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[3], faceIndex, depth,
		                                                   DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2ddc, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 0x800a2f00:
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2e2c, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2dc8, DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY))
			return 0;
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2da0, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2db4, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 0x800a2f28:
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2e04, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2db4, DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY))
			return 0;
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2da0, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2dc8, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 0x800a2f50:
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2e18, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2dc8, DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY))
			return 0;
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2db4, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2ddc, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 0x800a2f78:
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2df0, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2db4, DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY))
			return 0;
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2dc8, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		return DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, faceIndex, depth, 0x800a2ddc, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	case 0x800a2fa0:
		DrawLevelOvr1P_SetGridFaceSlot(projected, 0);
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, 0, depth, 0x800a2da0, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		DrawLevelOvr1P_SetGridFaceSlot(projected, 1);
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, 1, depth, 0x800a2db4, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		DrawLevelOvr1P_SetGridFaceSlot(projected, 2);
		if (!DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, 2, depth, 0x800a2dc8, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
			return 0;
		DrawLevelOvr1P_SetGridFaceSlot(projected, 3);
		return DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, 3, depth, 0x800a2ddc, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	default:
		return 1;
	}
}

static int DrawLevelOvr1P_EmitWaterRenderedNearBridge(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex, int depth,
                                                      u32 handlerAddress, u32 allowedMask)
{
	struct DrawLevelOvr1PScratchVertex *sub;

	if (depth >= DRAW_LEVEL_OVR1P_MAX_NEAR_SUBDIV_DEPTH)
	{
		u32 directMask = DrawLevelOvr1P_SelectAndStoreWaterDirectMask(projected, indices, allowedMask);

		return DrawLevelOvr1P_EmitPreparedProjectedDirectMask(pb, primMem, block, projected, indices, faceIndex, NULL, directMask,
		                                                      DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED, 1);
	}

	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	// NOTE(aalhendi): Retail 0x800a31c0 builds the water 3x3 subdivision
	// frame before jumping through the copied recursive-handler table.
	DrawLevelOvr1P_BuildGridSubdivisionFrame(sub, projected, indices, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);

	return DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, sub, faceIndex, depth + 1, handlerAddress, allowedMask);
}

static int DrawLevelOvr1P_EmitWaterRenderedProjectedFace(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex, int depth,
                                                         u32 allowedMask)
{
	u32 directMask;
	u32 nearMask;

	if (DrawLevelOvr1P_AreProjectedVerticesHalfNear(projected, indices, 4))
		return 1;

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		nearMask = DrawLevelOvr1P_GetProjectedNearMaskForDepth(projected, indices, depth);

		*CTR_SCRATCHPAD_PTR(u32, 0x70) = allowedMask;
		if (nearMask != 0)
		{
			u32 handlerAddress = DrawLevelOvr1P_GetNearSubdivisionHandlerAddress(nearMask, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);

			// NOTE(aalhendi): Retail water-rendered 0x800a30f0 preserves the current direct mask in scratch 0x70 before near recursion.
			DrawLevelOvr1P_SetPreviousRecursiveHandler(handlerAddress);
			return DrawLevelOvr1P_EmitWaterRenderedNearBridge(pb, primMem, block, projected, indices, faceIndex, depth, handlerAddress, allowedMask);
		}

		return DrawLevelOvr1P_EmitPreparedProjectedDirectMask(pb, primMem, block, projected, indices, faceIndex, NULL, allowedMask,
		                                                      DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED, 1);
	}

	if (DrawLevelOvr1P_IsProjectedFaceOffscreen(pb, projected, indices))
		return 1;

	directMask = DrawLevelOvr1P_SelectAndStoreWaterDirectMask(projected, indices, allowedMask);
	if (directMask == 0)
		return 1;

	return DrawLevelOvr1P_EmitPreparedProjectedDirectMask(pb, primMem, block, projected, indices, faceIndex, NULL, directMask,
	                                                      DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED, 1);
}

static int DrawLevelOvr1P_DrawRenderedWaterQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh, struct QuadBlock *block)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	if ((block->quadFlags & 0x8000) != 0)
		return 1;

	DrawLevelOvr1P_SetActiveDrawOrderLow(block);

	(void)DrawLevelOvr1P_ProjectQuadBlockGrid(vertices, block, projected, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
	DrawLevelOvr1P_ApplyWaterRenderedColorFades(projected);

	// TODO(aalhendi): Replace this bridge with the exact
	// 0x800a2da0..0x800a33e0 recursive/direct helper body.
	return DrawLevelOvr1P_DispatchWaterRenderedHelper(pb, primMem, block, projected, 0, 0, 0x800a2fa0, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
}

static int DrawLevelOvr1P_DrawRenderedGroundQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh, struct QuadBlock *block,
                                                      int lodMode)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	(void)lodMode;

	if ((block->quadFlags & 0x8000) != 0)
		return 1;

	DrawLevelOvr1P_SetActiveDrawOrderLow(block);

	(void)DrawLevelOvr1P_ProjectQuadBlockGrid(vertices, block, projected, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);

	for (int faceIndex = 0; faceIndex < 4; faceIndex++)
	{
		int indices[4];
		u32 tableWord;

		// TODO(aalhendi): Replace this shared projected bridge with the exact
		// retail helper bodies for 0x800a2904/0x800a5e5c/0x800a7ba8/0x800a97c8.
		tableWord = DrawLevelOvr1P_Select4x1ProjectedFace(projected, block, faceIndex, indices);
		if (!DrawLevelOvr1P_EmitProjectedGroundQuad(pb, primMem, projected, block, indices, faceIndex, 4, tableWord, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED))
			return 0;
	}

	return 1;
}

static int DrawLevelOvr1P_DrawRenderedQuadBlocks(struct QuadBlock **renderedList, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                                 int role, int lodMode)
{
	u32 primReserve = DrawLevelOvr1P_GetBucketPrimReserve(role);

	if (renderedList == NULL)
		return 1;

	if (role == DRAW_LEVEL_OVR1P_BUCKET_4X1_RENDERED)
		return DrawLevelOvr1P_Draw4x1Rendered(renderedList, pb, mesh, primMem);

	while (*renderedList != NULL)
	{
		if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, primReserve))
			return 0;

		if (role == DRAW_LEVEL_OVR1P_BUCKET_WATER_RENDERED)
		{
			if (!DrawLevelOvr1P_DrawRenderedWaterQuadBlock(pb, primMem, mesh, *renderedList))
				return 0;
		}
		else if (!DrawLevelOvr1P_DrawRenderedGroundQuadBlock(pb, primMem, mesh, *renderedList, lodMode))
			return 0;

		renderedList++;
	}

	return 1;
}

static void *DrawLevelOvr1P_GetRenderListField(struct DrawLevelOvr1PRenderList *renderList, int offset)
{
	return *(void **)((u8 *)renderList + offset);
}

static int DrawLevelOvr1P_DrawBspList(struct DrawLevelOvr1PRenderList *renderList, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                      const int *visFaceList)
{
	DrawLevelOvr1P_InitScratchpadTables();

	for (int bucketIndex = (int)(sizeof(sDrawLevelOvr1PBuckets) / sizeof(sDrawLevelOvr1PBuckets[0])) - 1; bucketIndex >= 0; bucketIndex--)
	{
		const struct DrawLevelOvr1PBucket *bucket = &sDrawLevelOvr1PBuckets[bucketIndex];
		void *bucketValue = DrawLevelOvr1P_GetRenderListField(renderList, bucket->renderListOffset);
		u32 primReserve = DrawLevelOvr1P_GetBucketPrimReserve(bucket->role);

		if (bucketValue == NULL)
			continue;

		DrawLevelOvr1P_ApplyBucketSetup(bucketIndex);

		if (bucket->kind == DRAW_LEVEL_OVR1P_BUCKET_QUADBLOCKS_RENDERED)
		{
			if (!DrawLevelOvr1P_DrawRenderedQuadBlocks(bucketValue, pb, mesh, primMem, bucket->role, bucket->lodMode))
				return 0;
			continue;
		}

		// NOTE(aalhendi): Overlay 226 consumes VisMem BSP-list nodes: word zero is
		// next, word one is the BSP pointer preserved by VisMem initialization.
		if (bucket->role == DRAW_LEVEL_OVR1P_BUCKET_4X1_LIST)
		{
			struct QuadBlock **renderedList = DrawLevelOvr1P_GetRenderedListForRole(renderList, bucket->role);

			DrawLevelOvr1P_SetRenderedListCursor(renderedList);

			if (!DrawLevelOvr1P_Draw4x1BspChain(bucketValue, pb, mesh, primMem, visFaceList))
				return 0;

			// NOTE(aalhendi): Retail terminates the GTE-overflow retry list before
			// the following rendered bucket consumes it.
			DrawLevelOvr1P_TerminateRenderedListCursor();
			continue;
		}

		{
			struct QuadBlock **renderedList = DrawLevelOvr1P_GetRenderedListForRole(renderList, bucket->role);

			if (renderedList != NULL)
				DrawLevelOvr1P_SetRenderedListCursor(renderedList);

			if (!DrawLevelOvr1P_DrawBspChain(bucketValue, pb, mesh, primMem, visFaceList, bucket->role, bucket->lodMode, renderedList != NULL, primReserve))
				return 0;

			if (renderedList != NULL)
				DrawLevelOvr1P_TerminateRenderedListCursor();
		}
	}

	return 1;
}

void DrawLevelOvr1P(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *waterEnvMap)
{
	struct DrawLevelOvr1PRenderList *renderList = LevRenderList;
	struct mesh_info *mesh = (struct mesh_info *)bspList;
	const int *visFaceList = VisMem10;

	(void)waterEnvMap;

	if (renderList == 0)
		return;

	if (pb == 0)
		return;

	if (mesh == 0 || mesh->ptrQuadBlockArray == 0 || mesh->ptrVertexArray == 0)
		return;

	if (primMem == 0 || primMem->curr == 0 || primMem->end == 0)
		return;

	gte_SetRotMatrix(&pb->matrix_ViewProj);
	gte_SetTransMatrix(&pb->matrix_ViewProj);
	gte_SetGeomOffset(pb->rect.w >> 1, pb->rect.h >> 1);
	gte_SetGeomScreen(pb->distanceToScreen_PREV);

	DrawLevelOvr1P_InitRuntimeScratch(pb, primMem);
	if (!DrawLevelOvr1P_DrawBspList(renderList, pb, mesh, primMem, visFaceList))
		return;

	// NOTE(aalhendi): Retail 0x800a0e98 reloads 0x800ab910 into scratch
	// 0x240 immediately before 0x800aa790 consumes clipped records.
	DrawLevelOvr1P_InitClipRecordJumpTable();
	if (!DrawLevelOvr1P_ConsumeClipRecords(pb, primMem))
		return;
}

void DrawLevelOvr2P(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *VisMem14, void *waterEnvMap)
{
	// TODO(aalhendi): Port overlay 227. Native keeps the legacy fallback until
	// the 2P renderer is audited against retail.
	(void)VisMem14;
	TEST_226((struct RenderList *)LevRenderList, pb, (struct mesh_info *)bspList, primMem, VisMem10, (int)waterEnvMap);
}

void DrawLevelOvr3P(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *VisMem14, void *VisMem18,
                    void *waterEnvMap)
{
	// TODO(aalhendi): Port overlay 228. Native keeps the legacy fallback until
	// the 3P renderer is audited against retail.
	(void)VisMem14;
	(void)VisMem18;
	TEST_226((struct RenderList *)LevRenderList, pb, (struct mesh_info *)bspList, primMem, VisMem10, (int)waterEnvMap);
}

void DrawLevelOvr4P(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *VisMem14, void *VisMem18,
                    void *VisMem1C, void *waterEnvMap)
{
	// TODO(aalhendi): Port overlay 229. Native keeps the legacy fallback until
	// the 4P renderer is audited against retail.
	(void)VisMem14;
	(void)VisMem18;
	(void)VisMem1C;
	TEST_226((struct RenderList *)LevRenderList, pb, (struct mesh_info *)bspList, primMem, VisMem10, (int)waterEnvMap);
}
