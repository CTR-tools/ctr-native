#include <common.h>

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
	DRAW_LEVEL_OVR1P_SPLIT_GROUND_MOSAIC_RELOAD_SPAN = 0xc0,
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

enum DrawLevelOvr1PPrimCode
{
	DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO = -1,
};

static const u32 DRAW_LEVEL_OVR1P_SLOT_WORD_PRESERVE = 0xffffffff;
static int sOvr226_800a1cc4_InheritedOtIndex;
static struct QuadBlock **sDrawLevelOvr1P_RenderedOverflowBase;
static u8 *sDrawLevelOvr1P_ClipRecordStart;
static u32 sDrawLevelOvr1P_PrimReserveBias;
static u32 sDrawLevelOvr1P_MosaicReloadSpanOverride;
static int sDrawLevelOvr1P_ListHandlersSeedRenderedCursor;

static u32 DrawLevelOvr1P_ReadPackedWord(const void *src)
{
	const u8 *bytes = (const u8 *)src;

	return (u32)bytes[0] | ((u32)bytes[1] << 8) | ((u32)bytes[2] << 16) | ((u32)bytes[3] << 24);
}

static u16 DrawLevelOvr1P_ReadPackedHalf(const void *src)
{
	const u8 *bytes = (const u8 *)src;

	return (u16)((u16)bytes[0] | ((u16)bytes[1] << 8));
}

static void DrawLevelOvr1P_WritePackedWord(void *dst, u32 value)
{
	u8 *bytes = (u8 *)dst;

	bytes[0] = (u8)value;
	bytes[1] = (u8)(value >> 8);
	bytes[2] = (u8)(value >> 16);
	bytes[3] = (u8)(value >> 24);
}

static void DrawLevelOvr1P_WritePackedHalf(void *dst, u16 value)
{
	u8 *bytes = (u8 *)dst;

	bytes[0] = (u8)value;
	bytes[1] = (u8)(value >> 8);
}

struct DrawLevelOvr1PFullDynamicRecursiveGate
{
	u32 directMask;
	int forceDirect;
};

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

CTR_STATIC_ASSERT(sizeof(sDrawLevelOvr1PBuckets) / sizeof(sDrawLevelOvr1PBuckets[0]) == OVR226_BUCKET_COUNT);
CTR_STATIC_ASSERT(sizeof(struct OverlayRDATA_226) == 0x564);
CTR_STATIC_ASSERT(sizeof(struct OverlayRDATA_226_BucketSetupRecord) == 0x64);
CTR_STATIC_ASSERT(sizeof(((struct OverlayRDATA_226 *)0)->scratchInitTable) == 0x60);
CTR_STATIC_ASSERT(sizeof(((struct OverlayRDATA_226 *)0)->clipRecordJumpTable) == 0x60);

static void DrawLevelOvr1P_SetGridFaceSlot(const struct DrawLevelOvr1PScratchVertex *projected, int faceIndex);
static int DrawLevelOvr1P_IsDeepestSubdivisionFrame(const struct DrawLevelOvr1PScratchVertex *projected);

static struct DrawLevelOvr1PStableScratch *DrawLevelOvr1P_Scratch(void)
{
	return CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PStableScratch, 0);
}

static struct MainRenderLevelGeometryScratch *DrawLevelOvr1P_RenderScratch(void)
{
	return &DrawLevelOvr1P_Scratch()->render;
}

static u32 *DrawLevelOvr1P_TerminalReturnPcScratch(void)
{
	return CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_TERMINAL_RETURN_PC_OFFSET);
}

static struct DrawLevelOvr1PScratchVertex *DrawLevelOvr1P_TerminalClipVertex(int index)
{
	return CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PScratchVertex,
	                          DRAW_LEVEL_OVR1P_TERMINAL_CLIP_VERTEX_OFFSET + (index * (int)sizeof(struct DrawLevelOvr1PScratchVertex)));
}

static void DrawLevelOvr1P_SetActiveDrawOrderLow(const struct QuadBlock *block)
{
	DrawLevelOvr1P_Scratch()->drawOrderOrHeader = block->draw_order_low;
}

static u32 DrawLevelOvr1P_GetActiveDrawOrderLow(void)
{
	return DrawLevelOvr1P_Scratch()->drawOrderOrHeader;
}

static void DrawLevelOvr1P_CopyScratchWords(const u32 *source, const struct OverlayRDATA_226_BucketSetupCopy *copy)
{
	u32 *scratch = CTR_SCRATCHPAD_PTR(u32, copy->scratchOffset);

	for (u32 i = 0; i <= copy->loopCounter; i++)
	{
		scratch[i] = source[i];
	}
}

static void Ovr226_800ab3dc_CopyClipRecordJumpTable(void)
{
	u32 *clipRecordJumpTable = CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_GT3_CLIP_RECORD_JUMP_TABLE_OFFSET);

	for (int i = 0; i < OVR226_CLIP_RECORD_JUMP_WORD_COUNT; i++)
	{
		clipRecordJumpTable[i] = R226.clipRecordJumpTable[i];
	}
}

static int Ovr226_800ab3d4_EmptyGT4ClipRecordLabel(void)
{
	return 1;
}

static const struct OverlayRDATA_226_BucketSetupRecord *DrawLevelOvr1P_FindBucketSetupRecord(u32 setupAddress)
{
	for (int i = 0; i < OVR226_BUCKET_COUNT; i++)
	{
		u32 recordAddress = OVR226_RDATA_BUCKET_SETUP_BASE + (u32)(i * sizeof(R226.bucketSetups[0]));

		if (recordAddress == setupAddress)
		{
			return &R226.bucketSetups[i];
		}
	}

	return NULL;
}

static void Ovr226_800a0ddc_CopyScratchInitTable(void)
{
	u32 *scratch = CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_SCRATCH_INIT_TABLE_OFFSET);

	for (int i = 0; i < OVR226_SCRATCH_INIT_WORD_COUNT; i++)
	{
		scratch[i] = R226.scratchInitTable[i];
	}
}

static void Ovr226_800a0e44_ApplyBucketSetup(u32 setupAddress)
{
	const struct OverlayRDATA_226_BucketSetupRecord *setup = DrawLevelOvr1P_FindBucketSetupRecord(setupAddress);

	if (setup == NULL)
	{
		return;
	}

	DrawLevelOvr1P_CopyScratchWords(setup->copy0, &setup->copies[0]);
	DrawLevelOvr1P_CopyScratchWords(setup->copy1, &setup->copies[1]);
}

static u32 DrawLevelOvr1P_Select4x1ProjectedTableWord(const struct QuadBlock *block, const struct DrawLevelOvr1PFaceSelector *selector)
{
	u32 tableIndex;

	(void)block;

	tableIndex = (DrawLevelOvr1P_GetActiveDrawOrderLow() >> selector->drawOrderShift) & 0x1f;

	return *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_SCRATCH_INIT_TABLE_OFFSET + (int)(tableIndex * sizeof(u32)));
}

static u32 DrawLevelOvr1P_Select4x1ProjectedIndices(const struct QuadBlock *block, const struct DrawLevelOvr1PFaceSelector *selector, int *indices)
{
	u32 tableWord = DrawLevelOvr1P_Select4x1ProjectedTableWord(block, selector);

	DrawLevelOvr1P_Scratch()->selected4x1TableWord = tableWord;

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
	{
		return 1;
	}

	// NOTE(aalhendi): Retail 0x800a36d4 forms a byte offset with
	// `(blockID >> 3) & 0x1fc`; scale it back to the native int array index.
	wordIndex = ((blockID >> 3) & 0x1fc) >> 2;
	word = (u32)visFaceList[wordIndex];
	return (s32)(word << (blockID & 0x1f)) < 0;
}

static u32 DrawLevelOvr1P_GetGridFaceSlotWord(const struct DrawLevelOvr1PScratchVertex *projected)
{
	return DrawLevelOvr1P_ReadPackedWord((const u8 *)projected + 0xb4);
}

static struct TextureLayout *DrawLevelOvr1P_ResolveTexturePointer(uintptr_t texturePtr)
{
	struct TextureLayout *texture;

	if (texturePtr == 0)
	{
		return NULL;
	}

	if ((texturePtr & 1) != 0)
	{
		texture = *(struct TextureLayout **)(texturePtr - 1);
	}
	else
	{
		texture = (struct TextureLayout *)texturePtr;
	}

	return texture;
}

static struct TextureLayout *DrawLevelOvr1P_ResolveMidTexture(const struct QuadBlock *block, int faceIndex)
{
	return DrawLevelOvr1P_ResolveTexturePointer((uintptr_t)block->ptr_texture_mid[faceIndex]);
}

static int DrawLevelOvr1P_IsPlausibleTextureLayout(const struct TextureLayout *texture)
{
	if (texture == NULL)
	{
		return 0;
	}

	return (texture->tpage & 0xfe00) == 0;
}

#ifdef CTR_NATIVE
// NOTE(aalhendi): Native data-boundary shim. Retail ptrmap leaves PSX address
// words in level data; native level loads store host-rebased pointers instead.
static int DrawLevelOvr1P_MempackContains(const struct Mempack *pack, uintptr_t ptr, uintptr_t *span)
{
	uintptr_t start = (uintptr_t)pack->start;
	uintptr_t end = (uintptr_t)pack->endOfAllocator;

	if ((start == 0) || (start >= end) || (ptr < start) || (ptr >= end))
	{
		return 0;
	}

	if (span != NULL)
	{
		*span = end - start;
	}

	return 1;
}

static const struct Mempack *DrawLevelOvr1P_FindMempackContaining(uintptr_t ptr)
{
	const struct Mempack *bestPack = NULL;
	uintptr_t bestSpan = UINTPTR_MAX;
	struct GameTracker *gGT = sdata->gGT;

	// PtrMempack can point at inactive mask/podium loads while level1 renders.
	if ((gGT == NULL) || ((gGT->gameMode2 & LEV_SWAP) == 0))
	{
		const struct Mempack *mainPack = &sdata->mempack[0];

		return DrawLevelOvr1P_MempackContains(mainPack, ptr, NULL) ? mainPack : NULL;
	}

	for (int i = 0; i < 4; i++)
	{
		const struct Mempack *pack = &sdata->mempack[i];
		uintptr_t span;

		if (DrawLevelOvr1P_MempackContains(pack, ptr, &span))
		{
			if (span < bestSpan)
			{
				bestPack = pack;
				bestSpan = span;
			}
		}
	}

	return bestPack;
}

static int DrawLevelOvr1P_IsNativeLevelSpan(uintptr_t ptr, uintptr_t size)
{
	const struct Mempack *pack;
	uintptr_t end;

	if (size == 0)
	{
		return 0;
	}

	pack = DrawLevelOvr1P_FindMempackContaining(ptr);
	if (pack == NULL)
	{
		return 0;
	}

	end = ptr + size;
	if (end < ptr)
	{
		return 0;
	}

	return end <= (uintptr_t)pack->endOfAllocator;
}

static const struct Mempack *DrawLevelOvr1P_GetLevelMempack(void)
{
	struct GameTracker *gGT = sdata->gGT;
	uintptr_t levelPtr = gGT != NULL ? (uintptr_t)gGT->level1 : 0;

	if (levelPtr == 0)
	{
		levelPtr = (uintptr_t)sdata->ptrLevelFile;
	}

	return DrawLevelOvr1P_FindMempackContaining(levelPtr);
}
#endif

static int DrawLevelOvr1P_IsNativeLevelTexturePointer(u32 value)
{
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native classifies host-rebased level texture pointers at
	// the data boundary; renderer control flow still follows retail sign tests.
	uintptr_t ptr = (uintptr_t)value;

	if (!DrawLevelOvr1P_IsNativeLevelSpan(ptr, sizeof(struct TextureLayout)))
	{
		return 0;
	}

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

static int DrawLevelOvr1P_IsRetailOtActiveSlotWord(u32 slotWord)
{
	return (slotWord & 3) == 0 && slotWord <= 0x3c;
}

static u32 DrawLevelOvr1P_GetProjectedOtSlotWord(const struct DrawLevelOvr1PScratchVertex *projected, int faceIndex)
{
	u32 slotWord = projected != NULL ? DrawLevelOvr1P_GetGridFaceSlotWord(projected) : (u32)(faceIndex * 4);

	if (!DrawLevelOvr1P_IsRetailOtActiveSlotWord(slotWord))
	{
		return (u32)(faceIndex * 4);
	}

	return slotWord;
}

#ifdef CTR_NATIVE
static int DrawLevelOvr1P_TryConvertNativeMempackPointerToPsxWord(u32 hostWord, u32 *psxWord)
{
	const u32 psxRamBase = 0x80000000u;
	const uintptr_t psxRamSize = 0x200000u;
	const struct Mempack *pack;
	uintptr_t hostEnd;
	uintptr_t hostBase;
	uintptr_t hostPtr;

	hostPtr = (uintptr_t)hostWord;
	pack = DrawLevelOvr1P_FindMempackContaining(hostPtr);
	if (pack == NULL || pack->endOfMemory == NULL)
	{
		return 0;
	}

	hostEnd = (uintptr_t)pack->endOfMemory;
	if (hostEnd < psxRamSize)
	{
		return 0;
	}

	hostBase = hostEnd - psxRamSize;
	if (hostPtr < hostBase || hostPtr >= hostEnd)
	{
		return 0;
	}

	*psxWord = psxRamBase + (u32)(hostPtr - hostBase);
	return 1;
}
#endif

static struct TextureLayout *DrawLevelOvr1P_ResolveTexturePointerChecked(uintptr_t texturePtr)
{
#ifdef CTR_NATIVE
	struct TextureLayout *texture;

	if (texturePtr == 0)
	{
		return NULL;
	}

	if ((texturePtr & 1) != 0)
	{
		uintptr_t activePtrSlot = texturePtr - 1;

		if (!DrawLevelOvr1P_IsNativeLevelSpan(activePtrSlot, sizeof(texture)))
		{
			return NULL;
		}

		texture = *(struct TextureLayout **)activePtrSlot;
	}
	else
	{
		texture = (struct TextureLayout *)texturePtr;
	}

	if (!DrawLevelOvr1P_IsNativeLevelSpan((uintptr_t)texture, sizeof(*texture)))
	{
		return NULL;
	}

	return DrawLevelOvr1P_IsPlausibleTextureLayout(texture) ? texture : NULL;
#else
	return DrawLevelOvr1P_ResolveTexturePointer(texturePtr);
#endif
}

static s8 DrawLevelOvr1P_ReadRetailQuadBlockByte(const struct QuadBlock *block, u32 byteOffset)
{
#ifdef CTR_NATIVE
	if (byteOffset >= 0x1c && byteOffset < 0x2c)
	{
		u32 pointerWordOffset = byteOffset & ~3u;
		u32 hostWord = DrawLevelOvr1P_ReadPackedWord((const u8 *)block + pointerWordOffset);
		u32 psxWord;

		// NOTE(aalhendi): Retail reads raw post-ptrmap PSX pointer bytes here;
		// native level pointer words are host-rebased, so reconstruct that byte.
		if (DrawLevelOvr1P_TryConvertNativeMempackPointerToPsxWord(hostWord, &psxWord))
		{
			return (s8)((psxWord >> ((byteOffset & 3u) * 8)) & 0xff);
		}
	}
#endif

	return *(const s8 *)((const u8 *)block + byteOffset);
}

static struct TextureLayout *DrawLevelOvr1P_ResolveProjectedMidTexture(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected)
{
	u32 slotWord;

	if (projected == NULL)
	{
		return NULL;
	}

	slotWord = DrawLevelOvr1P_GetGridFaceSlotWord(projected);
	if (!DrawLevelOvr1P_IsRetailOtActiveSlotWord(slotWord))
	{
		return NULL;
	}

	// NOTE(aalhendi): Retail selector bodies load raw `quad+0x1c+slot`.
	// Native validates the host-rebased word before following it.
	return DrawLevelOvr1P_ResolveTexturePointerChecked((uintptr_t)*(void *const *)((const u8 *)block + 0x1c + slotWord));
}

static struct TextureLayout *DrawLevelOvr1P_GetTexture(const struct QuadBlock *block, int faceIndex, int numFaces)
{
	struct TextureLayout *texture = block->ptr_texture_low;

	if (numFaces == 4)
	{
		texture = DrawLevelOvr1P_ResolveMidTexture(block, faceIndex);

		if (texture != 0)
		{
			texture += 2;
		}
	}

	return texture;
}

static struct TextureLayout *DrawLevelOvr1P_GetProjectedMidTexture(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected,
                                                                   int faceIndex, u32 maxDepth)
{
	struct TextureLayout *texture = DrawLevelOvr1P_ResolveProjectedMidTexture(block, projected);
	u32 mosaicWord;

	// NOTE(aalhendi): Retail 0x800a8504/0x800aa124 uses the active slot, not
	// the caller face index. Fall back only when the active slot itself is not
	// a retail slot, matching the caller's face-index fallback contract.
	if (texture == NULL && (projected == NULL || !DrawLevelOvr1P_IsRetailOtActiveSlotWord(DrawLevelOvr1P_GetGridFaceSlotWord(projected))))
	{
		texture = DrawLevelOvr1P_ResolveMidTexture(block, faceIndex);
	}

	if (texture == NULL)
	{
		return NULL;
	}

	mosaicWord = DrawLevelOvr1P_ReadPackedWord((const u8 *)texture + 0x24);

	// NOTE(aalhendi): Retail stores texture+0x24 at scratch 0x84 for the
	// deepest-frame UV reload path.
	DrawLevelOvr1P_Scratch()->mosaicTextureWord = mosaicWord;

	if ((s32)maxDepth < DrawLevelOvr1P_RenderScratch()->textureLodDepthThreshold0)
	{
		texture++;
	}

	if ((s32)maxDepth < DrawLevelOvr1P_RenderScratch()->textureLodDepthThreshold1)
	{
		texture++;
	}

	if ((s32)maxDepth < DrawLevelOvr1P_RenderScratch()->topLevelNearDepthThreshold)
	{
		// NOTE(aalhendi): Retail tests this word as signed PS1 data. Pointer-map
		// rebasing turns valid 0x80xxxxxx hi-texture pointers into positive native
		// addresses, so keep those on the retail negative path.
		if (!DrawLevelOvr1P_TreatAsRetailNegativeTextureWord(mosaicWord))
		{
			texture++;
		}
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

	DrawLevelOvr1P_Scratch()->uv.uv0 = uv[0] | ((u32)texture->clut << 16);
	DrawLevelOvr1P_Scratch()->uv.uv1 = uv[1] | ((u32)texture->tpage << 16);
	DrawLevelOvr1P_Scratch()->uv.savedUv0 = uv[0] | ((u32)texture->clut << 16);
	DrawLevelOvr1P_Scratch()->uv.savedUv1 = uv[1] | ((u32)texture->tpage << 16);

	// NOTE(aalhendi): Retail 4x1 handlers store selected UV halfwords at scratch-record offset 0x6 before primitive emission.
	if ((s32)(tableWord << 8) < 0)
	{
		projected[indices[0]].flags = uv[1];
		projected[indices[1]].flags = uv[0];
		projected[indices[2]].flags = uv[3];

		if ((s32)(tableWord << 24) >= 0)
		{
			projected[indices[3]].flags = uv[2];
		}
	}
	else
	{
		projected[indices[0]].flags = uv[0];
		projected[indices[1]].flags = uv[1];
		projected[indices[2]].flags = uv[2];

		if ((s32)(tableWord << 24) >= 0)
		{
			projected[indices[3]].flags = uv[3];
		}
	}
}

static void DrawLevelOvr1P_StoreProjectedDirectUvScratch(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count)
{
	// NOTE(aalhendi): Retail terminal direct helpers update UV scratch before packet emission.
	DrawLevelOvr1P_Scratch()->uv.flag0 = (s16)projected[indices[0]].flags;
	DrawLevelOvr1P_Scratch()->uv.flag1 = (s16)projected[indices[1]].flags;
	DrawLevelOvr1P_Scratch()->uv.flag2 = (s16)projected[indices[2]].flags;

	if (count == 4)
	{
		DrawLevelOvr1P_Scratch()->uv.flag3 = (s16)projected[indices[3]].flags;
	}
}

static void DrawLevelOvr1P_RestoreProjectedUvScratch(void)
{
	DrawLevelOvr1P_Scratch()->uv.uv0 = DrawLevelOvr1P_Scratch()->uv.savedUv0;
	DrawLevelOvr1P_Scratch()->uv.uv1 = DrawLevelOvr1P_Scratch()->uv.savedUv1;
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

	case 0x800a3670:
	case 0x800a3668:
	case 0x800a36f4:
		expectedHandlerAddress = 0x800a3014;
		*reloadSpan = 0xc0;
		break;

	case 0x800a3724:
	case 0x800a371c:
	case 0x800a37a8:
		expectedHandlerAddress = 0x800a30c8;
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

	if (previousHandlerAddress != expectedHandlerAddress)
	{
		return 0;
	}

	if (sDrawLevelOvr1P_MosaicReloadSpanOverride != 0)
	{
		*reloadSpan = sDrawLevelOvr1P_MosaicReloadSpanOverride;
	}

	return 1;
}

static void DrawLevelOvr1P_SetMosaicReloadSpanOverride(u32 reloadSpan)
{
	sDrawLevelOvr1P_MosaicReloadSpanOverride = reloadSpan;
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

	if (projected != CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PScratchVertex, DRAW_LEVEL_OVR1P_DEEPEST_PROJECTED_FRAME_OFFSET))
	{
		return;
	}

	mosaicBase = DrawLevelOvr1P_Scratch()->mosaicTextureWord;
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail uses scratch 0x84 directly. Native can inherit
	// host-rebased pointer words when a wide slot preserves texture state, so
	// only dereference values that are valid level texture data.
	if (mosaicBase == 0 || ((s32)mosaicBase < 0 && !DrawLevelOvr1P_IsNativeLevelTexturePointer(mosaicBase)))
	{
		DrawLevelOvr1P_RestoreProjectedUvScratch();
		return;
	}
#endif
	// NOTE(aalhendi): Retail 0x800a8714/0x800aa334 restores saved UV scratch
	// for positive inline sentinels, but rebased native hi-texture pointers must
	// still follow PS1's negative-pointer reload path.
	if ((s32)mosaicBase > 0 && !DrawLevelOvr1P_IsNativeLevelTexturePointer(mosaicBase))
	{
		DrawLevelOvr1P_RestoreProjectedUvScratch();
		return;
	}

	if (!DrawLevelOvr1P_GetDeepestMosaicReloadGate(directHandlerAddress, DrawLevelOvr1P_Scratch()->previousDirectHandlerAddress, &reloadSpan))
	{
		DrawLevelOvr1P_RestoreProjectedUvScratch();
		return;
	}

	sourceOffset = *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_MOSAIC_SOURCE_INDEX_OFFSET) << 1;
	if ((s32)(DrawLevelOvr1P_Scratch()->selected4x1TableWord << 8) < 0)
	{
		sourceOffset += reloadSpan;
	}
	sourceOffset += *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_MOSAIC_SOURCE_BIAS_OFFSET);

	source = (const u8 *)(uintptr_t)(mosaicBase + sourceOffset);
	uv0 = DrawLevelOvr1P_ReadPackedWord(source + 0);
	uv1 = DrawLevelOvr1P_ReadPackedWord(source + 4);

	// NOTE(aalhendi): Retail deepest fallthrough rewrites selected scratch-record
	// UV halfwords before jumping through the direct table.
	mutableProjected = (struct DrawLevelOvr1PScratchVertex *)projected;
	DrawLevelOvr1P_Scratch()->uv.uv0 = uv0;
	DrawLevelOvr1P_Scratch()->uv.uv1 = uv1;
	mutableProjected[indices[0]].flags = (u16)uv0;
	mutableProjected[indices[1]].flags = (u16)uv1;
	mutableProjected[indices[2]].flags = DrawLevelOvr1P_ReadPackedHalf(source + 8);
	mutableProjected[indices[3]].flags = DrawLevelOvr1P_ReadPackedHalf(source + 10);
}

static void DrawLevelOvr1P_SetGridFaceSlot(const struct DrawLevelOvr1PScratchVertex *projected, int faceIndex)
{
	// NOTE(aalhendi): Retail 3x3 helper frames use frame+0xb4 for face slot*4.
	DrawLevelOvr1P_WritePackedWord((u8 *)projected + 0xb4, (u32)(faceIndex * 4));
}

static void DrawLevelOvr1P_SetGridFaceSlotWord(const struct DrawLevelOvr1PScratchVertex *projected, u32 slotWord)
{
	DrawLevelOvr1P_WritePackedWord((u8 *)projected + 0xb4, slotWord);
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
	s32 gteFlag;

	CTR_GteLoadSVec3V0(&vertices[block->index[vertexIndex]].pos);
	gte_rtps();
	CTR_GteStoreSXY(posScreen);
	gte_stflg(&gteFlag);

	return ((u32)gteFlag & DRAW_LEVEL_OVR1P_GTE_RTPT_OVERFLOW) != 0;
}

static struct DrawLevelOvr1PScratchVertex *DrawLevelOvr1P_GetScratchVertices(void)
{
	// NOTE(aalhendi): Retail 4x1 handlers build 20-byte projected vertex records at scratch 0x1f8001b4.
	return CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PScratchVertex, DRAW_LEVEL_OVR1P_PROJECTED_FRAME0_OFFSET);
}

static s32 DrawLevelOvr1P_GetDepthClipThreshold(void)
{
	return DrawLevelOvr1P_Scratch()->depthClipThreshold;
}

static u8 *DrawLevelOvr1P_GetClipRecordCursor(void)
{
	return (u8 *)(uintptr_t)DrawLevelOvr1P_Scratch()->clipCursorPtr32;
}

static void DrawLevelOvr1P_SetClipRecordCursor(u8 *cursor)
{
	DrawLevelOvr1P_Scratch()->clipCursorPtr32 = (u32)(uintptr_t)cursor;
}

static u8 *DrawLevelOvr1P_GetClipRecordStart(void)
{
	if (sDrawLevelOvr1P_ClipRecordStart != NULL)
	{
		return sDrawLevelOvr1P_ClipRecordStart;
	}

	return data.PtrClipBuffer[0];
}

static void DrawLevelOvr1P_SetClipRecordStart(u8 *start)
{
	sDrawLevelOvr1P_ClipRecordStart = start;
}

static u8 *DrawLevelOvr1P_GetClipRecordEnd(void)
{
	u8 *start = DrawLevelOvr1P_GetClipRecordStart();
	struct GameTracker *gGT = sdata->gGT;

	if (start == NULL || gGT == NULL)
	{
		return start;
	}

	return start + ((size_t)MainDB_GetClipSize(gGT->levelID, gGT->numPlyrCurrGame) << 2);
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

static void DrawLevelOvr1P_CopyColorWord(u8 *dst, const u8 *src)
{
	DrawLevelOvr1P_WritePackedWord(dst, DrawLevelOvr1P_ReadPackedWord(src));
}

static void DrawLevelOvr1P_CopySourcePosFlags(struct DrawLevelOvr1PScratchVertex *projected, const struct LevVertex *vertex)
{
	projected->posVec = vertex->pos;
	projected->flags = vertex->flags;
}

static void DrawLevelOvr1P_CopySourceVertex(struct DrawLevelOvr1PScratchVertex *projected, const struct LevVertex *vertex)
{
	DrawLevelOvr1P_CopySourcePosFlags(projected, vertex);
	DrawLevelOvr1P_CopyColorWord(projected->color_hi, vertex->color_hi);
}

static void DrawLevelOvr1P_CopyProjectedScreenDepth(struct DrawLevelOvr1PScratchVertex *dst, const struct DrawLevelOvr1PScratchVertex *src)
{
	dst->posScreenVec = src->posScreenVec;
	dst->depth = src->depth;
	dst->clipNear = src->clipNear;
	dst->clipHalfNear = src->clipHalfNear;
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
	s32 gteFlag;

	DrawLevelOvr1P_CopySourceVertex(&projected[index0], vertex0);
	DrawLevelOvr1P_CopySourceVertex(&projected[index1], vertex1);
	DrawLevelOvr1P_CopySourceVertex(&projected[index2], vertex2);

	CTR_GteLoadSVec3V3(&vertex0->pos, &vertex1->pos, &vertex2->pos);
	gte_rtpt();
	CTR_GteStoreSXY3(&projected[index0].posScreen[0], &projected[index1].posScreen[0], &projected[index2].posScreen[0]);
	gte_stsz3(&depth0, &depth1, &depth2);
	gte_stflg(&gteFlag);

	DrawLevelOvr1P_SetProjectedDepth(&projected[index0], depth0, writeClipBytes);
	DrawLevelOvr1P_SetProjectedDepth(&projected[index1], depth1, writeClipBytes);
	DrawLevelOvr1P_SetProjectedDepth(&projected[index2], depth2, writeClipBytes);

	return ((u32)gteFlag & DRAW_LEVEL_OVR1P_GTE_RTPT_OVERFLOW) != 0;
}

static void DrawLevelOvr1P_ProjectScratchVertex(struct DrawLevelOvr1PScratchVertex *projected, int writeClipBytes);
static void DrawLevelOvr1P_StoreProjectedDepthWord(struct DrawLevelOvr1PScratchVertex *projected, u32 depth);

static void DrawLevelOvr1P_CopyVertexToProjected(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected,
                                                 int index)
{
	struct LevVertex *vertex = &vertices[block->index[index]];

	DrawLevelOvr1P_CopySourceVertex(&projected[index], vertex);
}

static void DrawLevelOvr1P_ProjectFullDynamicLowQuad(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected)
{
	// NOTE(aalhendi): Retail 0x800a0ef4 projects the low four before deciding
	// whether the near path needs the full 3x3 transition grid.
	(void)DrawLevelOvr1P_ProjectVertexTriple(vertices, block, projected, 0, 1, 2, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST);
	DrawLevelOvr1P_CopyVertexToProjected(vertices, block, projected, 3);
	DrawLevelOvr1P_ProjectScratchVertex(&projected[3], DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST);
}

static void Ovr226_800a0f78_ProjectVertexTripleFullDepth(struct LevVertex *vertices, const struct QuadBlock *block,
                                                         struct DrawLevelOvr1PScratchVertex *projected, int index0, int index1, int index2)
{
	struct LevVertex *vertex0 = &vertices[block->index[index0]];
	struct LevVertex *vertex1 = &vertices[block->index[index1]];
	struct LevVertex *vertex2 = &vertices[block->index[index2]];
	u32 depth0;
	u32 depth1;
	u32 depth2;

	DrawLevelOvr1P_CopySourceVertex(&projected[index0], vertex0);
	DrawLevelOvr1P_CopySourceVertex(&projected[index1], vertex1);
	DrawLevelOvr1P_CopySourceVertex(&projected[index2], vertex2);

	CTR_GteLoadSVec3V3(&vertex0->pos, &vertex1->pos, &vertex2->pos);
	gte_rtpt();
	CTR_GteStoreSXY3(&projected[index0].posScreen[0], &projected[index1].posScreen[0], &projected[index2].posScreen[0]);
	gte_stsz3(&depth0, &depth1, &depth2);

	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index0], depth0);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index1], depth1);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index2], depth2);
}

static void Ovr226_800a1024_ProjectFullDynamicLowFourth(struct LevVertex *vertices, const struct QuadBlock *block,
                                                        struct DrawLevelOvr1PScratchVertex *projected)
{
	struct LevVertex *vertex = &vertices[block->index[3]];
	u32 depth;

	DrawLevelOvr1P_CopySourceVertex(&projected[3], vertex);

	CTR_GteLoadSVec3V0(&vertex->pos);
	gte_rtps();
	CTR_GteStoreSXY(&projected[3].posScreen[0]);
	gte_stsz(&depth);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[3], depth);
}

static void Ovr226_800a0f78_ProjectFullDynamicLowQuad(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_SetGridFaceSlot(projected, 0);
	Ovr226_800a0f78_ProjectVertexTripleFullDepth(vertices, block, projected, 0, 1, 2);
	Ovr226_800a1024_ProjectFullDynamicLowFourth(vertices, block, projected);
	DrawLevelOvr1P_SetActiveDrawOrderLow(block);
}

static void Ovr226_800a0d20_SeedEntryScratchPointers(struct DrawLevelOvr1PRenderList *renderList, struct PushBuffer *pb)
{
	DrawLevelOvr1P_Scratch()->clipCursorPtr32 = (u32)(uintptr_t)data.PtrClipBuffer[0];
	DrawLevelOvr1P_Scratch()->pushBufferPtr32[0] = (u32)(uintptr_t)pb;
	DrawLevelOvr1P_Scratch()->renderListPtr32 = (u32)(uintptr_t)renderList;
}

static void Ovr226_800a0dc4_ClearProjectedScratch(void)
{
	u32 *projectedScratch = CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_PROJECTED_FRAME0_OFFSET);

	// NOTE(aalhendi): Retail 0x800a0dc4 clears through scratch word 0x3d8.
	for (int offset = 0; offset < 0x228; offset += (int)sizeof(u32))
	{
		projectedScratch[offset / (int)sizeof(u32)] = 0;
	}
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

static void Ovr226_800a1ee0_StoreWaterListProjectedVertex(struct LevVertex *vertex, struct DrawLevelOvr1PScratchVertex *projected)
{
	projected->posVec = vertex->pos;
	projected->flags = (u16)vertex->color_lo[0] | ((u16)vertex->color_lo[1] << 8);
}

static int Ovr226_800a1ee0_ProjectWaterListVertexTriple(struct LevVertex *vertices, const struct QuadBlock *block,
                                                        struct DrawLevelOvr1PScratchVertex *projected, int index0, int index1, int index2)
{
	struct LevVertex *vertex0 = &vertices[block->index[index0]];
	struct LevVertex *vertex1 = &vertices[block->index[index1]];
	struct LevVertex *vertex2 = &vertices[block->index[index2]];
	u32 depth0;
	u32 depth1;
	u32 depth2;
	s32 gteFlag;

	CTR_GteLoadSVec3V3(&vertex0->pos, &vertex1->pos, &vertex2->pos);
	gte_rtpt();

	DrawLevelOvr1P_CopyColorWord(projected[index0].color_hi, vertex0->color_hi);
	DrawLevelOvr1P_CopyColorWord(projected[index1].color_hi, vertex1->color_hi);
	DrawLevelOvr1P_CopyColorWord(projected[index2].color_hi, vertex2->color_hi);
	CTR_GteStoreSXY0(&projected[index0].posScreen[0]);
	gte_stflg(&gteFlag);
	depth0 = MFC2(17);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index0], depth0);
	if (((u32)gteFlag & DRAW_LEVEL_OVR1P_GTE_RTPT_OVERFLOW) != 0)
	{
		return 1;
	}

	CTR_GteStoreSXY1(&projected[index1].posScreen[0]);
	depth1 = MFC2(18);
	CTR_GteStoreSXY2(&projected[index2].posScreen[0]);
	depth2 = MFC2(19);
	Ovr226_800a1ee0_StoreWaterListProjectedVertex(vertex0, &projected[index0]);
	Ovr226_800a1ee0_StoreWaterListProjectedVertex(vertex1, &projected[index1]);
	Ovr226_800a1ee0_StoreWaterListProjectedVertex(vertex2, &projected[index2]);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index1], depth1);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index2], depth2);

	return 0;
}

static int Ovr226_800a1ee0_ProjectWaterListGrid(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_SetGridFaceSlot(projected, 0);
	if (Ovr226_800a1ee0_ProjectWaterListVertexTriple(vertices, block, projected, 0, 1, 2))
	{
		return 1;
	}

	if (Ovr226_800a1ee0_ProjectWaterListVertexTriple(vertices, block, projected, 3, 4, 5))
	{
		return 1;
	}

	DrawLevelOvr1P_SetActiveDrawOrderLow(block);
	return Ovr226_800a1ee0_ProjectWaterListVertexTriple(vertices, block, projected, 6, 7, 8);
}

static void Ovr226_800a3738_CopyGround4x1ListProjectedSource(struct LevVertex *vertex, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_CopySourcePosFlags(projected, vertex);
}

static int Ovr226_800a3738_ProjectGround4x1ListVertexTriple(struct LevVertex *vertices, const struct QuadBlock *block,
                                                            struct DrawLevelOvr1PScratchVertex *projected, int index0, int index1, int index2)
{
	struct LevVertex *vertex0 = &vertices[block->index[index0]];
	struct LevVertex *vertex1 = &vertices[block->index[index1]];
	struct LevVertex *vertex2 = &vertices[block->index[index2]];
	u32 depth0;
	u32 depth1;
	u32 depth2;
	s32 gteFlag;

	CTR_GteLoadSVec3V3(&vertex0->pos, &vertex1->pos, &vertex2->pos);
	gte_rtpt();

	DrawLevelOvr1P_CopyColorWord(projected[index0].color_hi, vertex0->color_hi);
	DrawLevelOvr1P_CopyColorWord(projected[index1].color_hi, vertex1->color_hi);
	DrawLevelOvr1P_CopyColorWord(projected[index2].color_hi, vertex2->color_hi);
	CTR_GteStoreSXY0(&projected[index0].posScreen[0]);
	gte_stflg(&gteFlag);
	depth0 = MFC2(17);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index0], depth0);
	if (((u32)gteFlag & DRAW_LEVEL_OVR1P_GTE_RTPT_OVERFLOW) != 0)
	{
		return 1;
	}

	CTR_GteStoreSXY1(&projected[index1].posScreen[0]);
	depth1 = MFC2(18);
	CTR_GteStoreSXY2(&projected[index2].posScreen[0]);
	depth2 = MFC2(19);
	Ovr226_800a3738_CopyGround4x1ListProjectedSource(vertex0, &projected[index0]);
	Ovr226_800a3738_CopyGround4x1ListProjectedSource(vertex1, &projected[index1]);
	Ovr226_800a3738_CopyGround4x1ListProjectedSource(vertex2, &projected[index2]);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index1], depth1);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index2], depth2);

	return 0;
}

static int Ovr226_800a3738_ProjectGround4x1ListGrid(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_SetGridFaceSlot(projected, 0);
	if (Ovr226_800a3738_ProjectGround4x1ListVertexTriple(vertices, block, projected, 0, 1, 2))
	{
		return 1;
	}

	if (Ovr226_800a3738_ProjectGround4x1ListVertexTriple(vertices, block, projected, 3, 4, 5))
	{
		return 1;
	}

	DrawLevelOvr1P_SetActiveDrawOrderLow(block);
	return Ovr226_800a3738_ProjectGround4x1ListVertexTriple(vertices, block, projected, 6, 7, 8);
}

static void Ovr226_800a5030_CopyGround4x2ListProjectedSource(struct LevVertex *vertex, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_CopySourcePosFlags(projected, vertex);
}

static int Ovr226_800a5030_ProjectGround4x2ListVertexTriple(struct LevVertex *vertices, const struct QuadBlock *block,
                                                            struct DrawLevelOvr1PScratchVertex *projected, int index0, int index1, int index2)
{
	struct LevVertex *vertex0 = &vertices[block->index[index0]];
	struct LevVertex *vertex1 = &vertices[block->index[index1]];
	struct LevVertex *vertex2 = &vertices[block->index[index2]];
	u32 depth0;
	u32 depth1;
	u32 depth2;
	s32 gteFlag;

	CTR_GteLoadSVec3V3(&vertex0->pos, &vertex1->pos, &vertex2->pos);
	gte_rtpt();

	DrawLevelOvr1P_CopyColorWord(projected[index0].color_hi, vertex0->color_hi);
	DrawLevelOvr1P_CopyColorWord(projected[index1].color_hi, vertex1->color_hi);
	DrawLevelOvr1P_CopyColorWord(projected[index2].color_hi, vertex2->color_hi);
	CTR_GteStoreSXY0(&projected[index0].posScreen[0]);
	gte_stflg(&gteFlag);
	depth0 = MFC2(17);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index0], depth0);
	if (((u32)gteFlag & DRAW_LEVEL_OVR1P_GTE_RTPT_OVERFLOW) != 0)
	{
		return 1;
	}

	CTR_GteStoreSXY1(&projected[index1].posScreen[0]);
	depth1 = MFC2(18);
	CTR_GteStoreSXY2(&projected[index2].posScreen[0]);
	depth2 = MFC2(19);
	Ovr226_800a5030_CopyGround4x2ListProjectedSource(vertex0, &projected[index0]);
	Ovr226_800a5030_CopyGround4x2ListProjectedSource(vertex1, &projected[index1]);
	Ovr226_800a5030_CopyGround4x2ListProjectedSource(vertex2, &projected[index2]);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index1], depth1);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index2], depth2);

	return 0;
}

static int Ovr226_800a5030_ProjectGround4x2ListGrid(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_SetGridFaceSlot(projected, 0);
	if (Ovr226_800a5030_ProjectGround4x2ListVertexTriple(vertices, block, projected, 0, 1, 2))
	{
		return 1;
	}

	if (Ovr226_800a5030_ProjectGround4x2ListVertexTriple(vertices, block, projected, 3, 4, 5))
	{
		return 1;
	}

	DrawLevelOvr1P_SetActiveDrawOrderLow(block);
	return Ovr226_800a5030_ProjectGround4x2ListVertexTriple(vertices, block, projected, 6, 7, 8);
}

static void Ovr226_800a6fd0_CopyDynamicListProjectedSource(struct LevVertex *vertex, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_CopySourcePosFlags(projected, vertex);
}

static int Ovr226_800a6fd0_ProjectDynamicListVertexTriple(struct LevVertex *vertices, const struct QuadBlock *block,
                                                          struct DrawLevelOvr1PScratchVertex *projected, int index0, int index1, int index2)
{
	struct LevVertex *vertex0 = &vertices[block->index[index0]];
	struct LevVertex *vertex1 = &vertices[block->index[index1]];
	struct LevVertex *vertex2 = &vertices[block->index[index2]];
	u32 depth0;
	u32 depth1;
	u32 depth2;
	s32 gteFlag;

	CTR_GteLoadSVec3V3(&vertex0->pos, &vertex1->pos, &vertex2->pos);
	gte_rtpt();

	DrawLevelOvr1P_CopyColorWord(projected[index0].color_hi, vertex0->color_hi);
	DrawLevelOvr1P_CopyColorWord(projected[index1].color_hi, vertex1->color_hi);
	DrawLevelOvr1P_CopyColorWord(projected[index2].color_hi, vertex2->color_hi);
	CTR_GteStoreSXY0(&projected[index0].posScreen[0]);
	gte_stflg(&gteFlag);
	depth0 = MFC2(17);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index0], depth0);
	if (((u32)gteFlag & DRAW_LEVEL_OVR1P_GTE_RTPT_OVERFLOW) != 0)
	{
		return 1;
	}

	CTR_GteStoreSXY1(&projected[index1].posScreen[0]);
	depth1 = MFC2(18);
	CTR_GteStoreSXY2(&projected[index2].posScreen[0]);
	depth2 = MFC2(19);
	Ovr226_800a6fd0_CopyDynamicListProjectedSource(vertex0, &projected[index0]);
	Ovr226_800a6fd0_CopyDynamicListProjectedSource(vertex1, &projected[index1]);
	Ovr226_800a6fd0_CopyDynamicListProjectedSource(vertex2, &projected[index2]);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index1], depth1);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index2], depth2);

	return 0;
}

static int Ovr226_800a6fd0_ProjectDynamicListGrid(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0);
	if (Ovr226_800a6fd0_ProjectDynamicListVertexTriple(vertices, block, projected, 0, 1, 2))
	{
		return 1;
	}

	if (Ovr226_800a6fd0_ProjectDynamicListVertexTriple(vertices, block, projected, 3, 4, 5))
	{
		return 1;
	}

	DrawLevelOvr1P_SetActiveDrawOrderLow(block);
	return Ovr226_800a6fd0_ProjectDynamicListVertexTriple(vertices, block, projected, 6, 7, 8);
}

static void Ovr226_800a8bf0_CopyWideDynamicProjectedSource(struct LevVertex *vertex, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_CopySourcePosFlags(projected, vertex);
}

static int Ovr226_800a8bf0_ProjectWideDynamicVertexTriple(struct LevVertex *vertices, const struct QuadBlock *block,
                                                          struct DrawLevelOvr1PScratchVertex *projected, int index0, int index1, int index2)
{
	struct LevVertex *vertex0 = &vertices[block->index[index0]];
	struct LevVertex *vertex1 = &vertices[block->index[index1]];
	struct LevVertex *vertex2 = &vertices[block->index[index2]];
	u32 depth0;
	u32 depth1;
	u32 depth2;
	s32 gteFlag;

	CTR_GteLoadSVec3V3(&vertex0->pos, &vertex1->pos, &vertex2->pos);
	gte_rtpt();

	DrawLevelOvr1P_CopyColorWord(projected[index0].color_hi, vertex0->color_hi);
	DrawLevelOvr1P_CopyColorWord(projected[index1].color_hi, vertex1->color_hi);
	DrawLevelOvr1P_CopyColorWord(projected[index2].color_hi, vertex2->color_hi);
	CTR_GteStoreSXY0(&projected[index0].posScreen[0]);
	gte_stflg(&gteFlag);
	depth0 = MFC2(17);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index0], depth0);
	if (((u32)gteFlag & DRAW_LEVEL_OVR1P_GTE_RTPT_OVERFLOW) != 0)
	{
		return 1;
	}

	CTR_GteStoreSXY1(&projected[index1].posScreen[0]);
	depth1 = MFC2(18);
	CTR_GteStoreSXY2(&projected[index2].posScreen[0]);
	depth2 = MFC2(19);
	Ovr226_800a8bf0_CopyWideDynamicProjectedSource(vertex0, &projected[index0]);
	Ovr226_800a8bf0_CopyWideDynamicProjectedSource(vertex1, &projected[index1]);
	Ovr226_800a8bf0_CopyWideDynamicProjectedSource(vertex2, &projected[index2]);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index1], depth1);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[index2], depth2);

	return 0;
}

static int Ovr226_800a8bf0_ProjectWideDynamicGrid(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0);
	if (Ovr226_800a8bf0_ProjectWideDynamicVertexTriple(vertices, block, projected, 0, 1, 2))
	{
		return 1;
	}

	if (Ovr226_800a8bf0_ProjectWideDynamicVertexTriple(vertices, block, projected, 3, 4, 5))
	{
		return 1;
	}

	DrawLevelOvr1P_SetActiveDrawOrderLow(block);
	return Ovr226_800a8bf0_ProjectWideDynamicVertexTriple(vertices, block, projected, 6, 7, 8);
}

static void Ovr226_800a5e5c_StoreGround4x2RenderedProjectedVertex(struct LevVertex *vertex, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_CopySourceVertex(projected, vertex);
}

static void Ovr226_800a5e5c_ProjectGround4x2RenderedVertexTriple(struct LevVertex *vertices, const struct QuadBlock *block,
                                                                 struct DrawLevelOvr1PScratchVertex *projected, int index0, int index1, int index2)
{
	struct LevVertex *vertex0 = &vertices[block->index[index0]];
	struct LevVertex *vertex1 = &vertices[block->index[index1]];
	struct LevVertex *vertex2 = &vertices[block->index[index2]];
	u32 depth0;
	u32 depth1;
	u32 depth2;

	Ovr226_800a5e5c_StoreGround4x2RenderedProjectedVertex(vertex0, &projected[index0]);
	Ovr226_800a5e5c_StoreGround4x2RenderedProjectedVertex(vertex1, &projected[index1]);
	Ovr226_800a5e5c_StoreGround4x2RenderedProjectedVertex(vertex2, &projected[index2]);

	CTR_GteLoadSVec3V3(&vertex0->pos, &vertex1->pos, &vertex2->pos);
	gte_rtpt();
	CTR_GteStoreSXY3(&projected[index0].posScreen[0], &projected[index1].posScreen[0], &projected[index2].posScreen[0]);
	gte_stsz3(&depth0, &depth1, &depth2);

	DrawLevelOvr1P_SetProjectedDepth(&projected[index0], depth0, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
	DrawLevelOvr1P_SetProjectedDepth(&projected[index1], depth1, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
	DrawLevelOvr1P_SetProjectedDepth(&projected[index2], depth2, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
}

static void Ovr226_800a5e5c_ProjectGround4x2RenderedGrid(struct LevVertex *vertices, const struct QuadBlock *block,
                                                         struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_SetGridFaceSlot(projected, 0);
	Ovr226_800a5e5c_ProjectGround4x2RenderedVertexTriple(vertices, block, projected, 0, 1, 2);
	Ovr226_800a5e5c_ProjectGround4x2RenderedVertexTriple(vertices, block, projected, 3, 4, 5);
	DrawLevelOvr1P_SetActiveDrawOrderLow(block);
	Ovr226_800a5e5c_ProjectGround4x2RenderedVertexTriple(vertices, block, projected, 6, 7, 8);
}

static void Ovr226_800a7ba8_StoreDynamicRenderedProjectedVertex(struct LevVertex *vertex, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_CopySourceVertex(projected, vertex);
}

static void Ovr226_800a7ba8_ProjectDynamicRenderedVertexTriple(struct LevVertex *vertices, const struct QuadBlock *block,
                                                               struct DrawLevelOvr1PScratchVertex *projected, int index0, int index1, int index2)
{
	struct LevVertex *vertex0 = &vertices[block->index[index0]];
	struct LevVertex *vertex1 = &vertices[block->index[index1]];
	struct LevVertex *vertex2 = &vertices[block->index[index2]];
	u32 depth0;
	u32 depth1;
	u32 depth2;

	Ovr226_800a7ba8_StoreDynamicRenderedProjectedVertex(vertex0, &projected[index0]);
	Ovr226_800a7ba8_StoreDynamicRenderedProjectedVertex(vertex1, &projected[index1]);
	Ovr226_800a7ba8_StoreDynamicRenderedProjectedVertex(vertex2, &projected[index2]);

	CTR_GteLoadSVec3V3(&vertex0->pos, &vertex1->pos, &vertex2->pos);
	gte_rtpt();
	CTR_GteStoreSXY3(&projected[index0].posScreen[0], &projected[index1].posScreen[0], &projected[index2].posScreen[0]);
	gte_stsz3(&depth0, &depth1, &depth2);

	DrawLevelOvr1P_SetProjectedDepth(&projected[index0], depth0, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
	DrawLevelOvr1P_SetProjectedDepth(&projected[index1], depth1, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
	DrawLevelOvr1P_SetProjectedDepth(&projected[index2], depth2, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
}

static void Ovr226_800a7ba8_ProjectDynamicRenderedGrid(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0);
	Ovr226_800a7ba8_ProjectDynamicRenderedVertexTriple(vertices, block, projected, 0, 1, 2);
	Ovr226_800a7ba8_ProjectDynamicRenderedVertexTriple(vertices, block, projected, 3, 4, 5);
	DrawLevelOvr1P_SetActiveDrawOrderLow(block);
	Ovr226_800a7ba8_ProjectDynamicRenderedVertexTriple(vertices, block, projected, 6, 7, 8);
}

static void Ovr226_800a97c8_StoreQuad4x4RenderedProjectedVertex(struct LevVertex *vertex, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_CopySourceVertex(projected, vertex);
}

static void Ovr226_800a97c8_ProjectQuad4x4RenderedVertexTriple(struct LevVertex *vertices, const struct QuadBlock *block,
                                                               struct DrawLevelOvr1PScratchVertex *projected, int index0, int index1, int index2)
{
	struct LevVertex *vertex0 = &vertices[block->index[index0]];
	struct LevVertex *vertex1 = &vertices[block->index[index1]];
	struct LevVertex *vertex2 = &vertices[block->index[index2]];
	u32 depth0;
	u32 depth1;
	u32 depth2;

	Ovr226_800a97c8_StoreQuad4x4RenderedProjectedVertex(vertex0, &projected[index0]);
	Ovr226_800a97c8_StoreQuad4x4RenderedProjectedVertex(vertex1, &projected[index1]);
	Ovr226_800a97c8_StoreQuad4x4RenderedProjectedVertex(vertex2, &projected[index2]);

	CTR_GteLoadSVec3V3(&vertex0->pos, &vertex1->pos, &vertex2->pos);
	gte_rtpt();
	CTR_GteStoreSXY3(&projected[index0].posScreen[0], &projected[index1].posScreen[0], &projected[index2].posScreen[0]);
	gte_stsz3(&depth0, &depth1, &depth2);

	DrawLevelOvr1P_SetProjectedDepth(&projected[index0], depth0, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
	DrawLevelOvr1P_SetProjectedDepth(&projected[index1], depth1, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
	DrawLevelOvr1P_SetProjectedDepth(&projected[index2], depth2, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
}

static void Ovr226_800a97c8_ProjectQuad4x4RenderedGrid(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0);
	Ovr226_800a97c8_ProjectQuad4x4RenderedVertexTriple(vertices, block, projected, 0, 1, 2);
	Ovr226_800a97c8_ProjectQuad4x4RenderedVertexTriple(vertices, block, projected, 3, 4, 5);
	DrawLevelOvr1P_SetActiveDrawOrderLow(block);
	Ovr226_800a97c8_ProjectQuad4x4RenderedVertexTriple(vertices, block, projected, 6, 7, 8);
}

static void Ovr226_800a417c_StoreGround4x1RenderedProjectedVertex(struct LevVertex *vertex, struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_CopySourceVertex(projected, vertex);
}

static void Ovr226_800a417c_ProjectGround4x1RenderedVertexTriple(struct LevVertex *vertices, const struct QuadBlock *block,
                                                                 struct DrawLevelOvr1PScratchVertex *projected, int index0, int index1, int index2)
{
	struct LevVertex *vertex0 = &vertices[block->index[index0]];
	struct LevVertex *vertex1 = &vertices[block->index[index1]];
	struct LevVertex *vertex2 = &vertices[block->index[index2]];
	u32 depth0;
	u32 depth1;
	u32 depth2;

	Ovr226_800a417c_StoreGround4x1RenderedProjectedVertex(vertex0, &projected[index0]);
	Ovr226_800a417c_StoreGround4x1RenderedProjectedVertex(vertex1, &projected[index1]);
	Ovr226_800a417c_StoreGround4x1RenderedProjectedVertex(vertex2, &projected[index2]);

	CTR_GteLoadSVec3V3(&vertex0->pos, &vertex1->pos, &vertex2->pos);
	gte_rtpt();
	CTR_GteStoreSXY3(&projected[index0].posScreen[0], &projected[index1].posScreen[0], &projected[index2].posScreen[0]);
	gte_stsz3(&depth0, &depth1, &depth2);

	DrawLevelOvr1P_SetProjectedDepth(&projected[index0], depth0, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
	DrawLevelOvr1P_SetProjectedDepth(&projected[index1], depth1, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
	DrawLevelOvr1P_SetProjectedDepth(&projected[index2], depth2, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
}

static void Ovr226_800a417c_ProjectGround4x1RenderedGrid(struct LevVertex *vertices, const struct QuadBlock *block,
                                                         struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_SetGridFaceSlot(projected, 0);
	Ovr226_800a417c_ProjectGround4x1RenderedVertexTriple(vertices, block, projected, 0, 1, 2);
	Ovr226_800a417c_ProjectGround4x1RenderedVertexTriple(vertices, block, projected, 3, 4, 5);
	DrawLevelOvr1P_SetActiveDrawOrderLow(block);
	Ovr226_800a417c_ProjectGround4x1RenderedVertexTriple(vertices, block, projected, 6, 7, 8);
}

static void Ovr226_800a2924_StoreWaterRenderedProjectedVertex(struct LevVertex *vertex, struct DrawLevelOvr1PScratchVertex *projected)
{
	projected->posVec = vertex->pos;
	projected->flags = (u16)vertex->color_lo[0] | ((u16)vertex->color_lo[1] << 8);
	DrawLevelOvr1P_CopyColorWord(projected->color_hi, vertex->color_hi);
}

static void Ovr226_800a2924_ProjectWaterRenderedVertexTriple(struct LevVertex *vertices, const struct QuadBlock *block,
                                                             struct DrawLevelOvr1PScratchVertex *projected, int index0, int index1, int index2)
{
	struct LevVertex *vertex0 = &vertices[block->index[index0]];
	struct LevVertex *vertex1 = &vertices[block->index[index1]];
	struct LevVertex *vertex2 = &vertices[block->index[index2]];
	u32 depth0;
	u32 depth1;
	u32 depth2;

	Ovr226_800a2924_StoreWaterRenderedProjectedVertex(vertex0, &projected[index0]);
	Ovr226_800a2924_StoreWaterRenderedProjectedVertex(vertex1, &projected[index1]);
	Ovr226_800a2924_StoreWaterRenderedProjectedVertex(vertex2, &projected[index2]);

	CTR_GteLoadSVec3V3(&vertex0->pos, &vertex1->pos, &vertex2->pos);
	gte_rtpt();
	CTR_GteStoreSXY3(&projected[index0].posScreen[0], &projected[index1].posScreen[0], &projected[index2].posScreen[0]);
	gte_stsz3(&depth0, &depth1, &depth2);

	DrawLevelOvr1P_SetProjectedDepth(&projected[index0], depth0, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
	DrawLevelOvr1P_SetProjectedDepth(&projected[index1], depth1, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
	DrawLevelOvr1P_SetProjectedDepth(&projected[index2], depth2, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
}

static void Ovr226_800a2924_ProjectWaterRenderedGrid(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected)
{
	Ovr226_800a2924_ProjectWaterRenderedVertexTriple(vertices, block, projected, 0, 1, 2);
	Ovr226_800a2924_ProjectWaterRenderedVertexTriple(vertices, block, projected, 3, 4, 5);
	DrawLevelOvr1P_SetActiveDrawOrderLow(block);
	Ovr226_800a2924_ProjectWaterRenderedVertexTriple(vertices, block, projected, 6, 7, 8);
}

static u32 DrawLevelOvr1P_GetProjectedColorWord(const struct DrawLevelOvr1PScratchVertex *projected);

static u32 Ovr226_800a2234_ApplyWaterListColorFade(u32 color, s16 x, s16 z)
{
	const s16 *center = DrawLevelOvr1P_Scratch()->projectedCenter.v;
	s32 deltaX = (s32)x - center[0];
	s32 deltaZ = (s32)z - center[2];
	s32 major;
	s32 minor;
	s32 falloff;
	s32 factor;

	if (deltaX < 0)
	{
		deltaX = -deltaX;
	}

	if (deltaZ < 0)
	{
		deltaZ = -deltaZ;
	}

	major = deltaX;
	minor = deltaZ;
	if (deltaX < deltaZ)
	{
		major = deltaZ;
		minor = deltaX;
	}

	falloff = major + (minor >> 2) - 0x1000;
	if (falloff >= 0)
	{
		return 0;
	}

	factor = (s32)((u32)falloff << 2) + 0x1000;
	// NOTE(aalhendi): Retail 0x800a2234 fades water-list vertex color with DPCS.
	MTC2(color, 6);
	if (factor < 0)
	{
		return color;
	}

	MTC2(factor, 8);
	gte_dpcs();
	return MFC2(22);
}

static u32 Ovr226_800a2d30_ApplyWaterRenderedColorFade(u32 color, s16 x, s16 z)
{
	const s16 *center = DrawLevelOvr1P_Scratch()->projectedCenter.v;
	s32 deltaX = (s32)x - center[0];
	s32 deltaZ = (s32)z - center[2];
	s32 major;
	s32 minor;
	s32 falloff;
	s32 factor;

	if (deltaX < 0)
	{
		deltaX = -deltaX;
	}

	if (deltaZ < 0)
	{
		deltaZ = -deltaZ;
	}

	major = deltaX;
	minor = deltaZ;
	if (deltaX < deltaZ)
	{
		major = deltaZ;
		minor = deltaX;
	}

	falloff = major + (minor >> 2) - 0x1000;
	if (falloff >= 0)
	{
		return 0;
	}

	factor = (s32)((u32)falloff << 2) + 0x1000;
	// NOTE(aalhendi): Retail 0x800a2d30 fades water-rendered vertex color with DPCS.
	MTC2(color, 6);
	if (factor < 0)
	{
		return color;
	}

	MTC2(factor, 8);
	gte_dpcs();
	return MFC2(22);
}

static void Ovr226_800a211c_ApplyWaterListColorFades(struct DrawLevelOvr1PScratchVertex *projected)
{
	for (int i = 0; i < 9; i++)
	{
		DrawLevelOvr1P_WritePackedWord(projected[i].color_hi, Ovr226_800a2234_ApplyWaterListColorFade(DrawLevelOvr1P_GetProjectedColorWord(&projected[i]),
		                                                                                              projected[i].pos[0], projected[i].pos[2]));
	}
}

static void Ovr226_800a2c4c_ApplyWaterRenderedColorFades(struct DrawLevelOvr1PScratchVertex *projected)
{
	for (int i = 0; i < 9; i++)
	{
		DrawLevelOvr1P_WritePackedWord(projected[i].color_hi, Ovr226_800a2d30_ApplyWaterRenderedColorFade(DrawLevelOvr1P_GetProjectedColorWord(&projected[i]),
		                                                                                                  projected[i].pos[0], projected[i].pos[2]));
	}
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
	u32 packedWindow = DrawLevelOvr1P_Scratch()->clipWindowPacked;
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
		{
			mask |= bits[i];
		}
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

static u32 DrawLevelOvr1P_GetProjectedWaterNearMask(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	// NOTE(aalhendi): Retail water shared bodies test s8 against scratch 0x1b4;
	// recursive frames use scratch 0x28 before selecting the next child.
	if (projected == DrawLevelOvr1P_GetScratchVertices())
	{
		return DrawLevelOvr1P_GetProjectedNearMask(projected, indices);
	}

	return DrawLevelOvr1P_GetProjectedRecursiveNearMask(projected, indices);
}

static u32 DrawLevelOvr1P_GetProjectedCopiedGridNearMask(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	// NOTE(aalhendi): Copied/default body entries use scratch 0x28 here.
	// Selector labels inside the same spans, like 0x800a7668/0x800a8380,
	// reselect texture first and then compute the near mask from 0x24.
	return DrawLevelOvr1P_GetProjectedRecursiveNearMask(projected, indices);
}

static void DrawLevelOvr1P_StoreProjectedDepthWord(struct DrawLevelOvr1PScratchVertex *projected, u32 depth)
{
	projected->depth = (u16)depth;
	projected->clipNear = (u8)(depth >> 16);
	projected->clipHalfNear = (u8)(depth >> 24);
}

static void DrawLevelOvr1P_ProjectScratchVertex(struct DrawLevelOvr1PScratchVertex *projected, int writeClipBytes)
{
	u32 depth;

	CTR_GteLoadS16TripletV0(&projected->pos[0]);
	gte_rtps();
	CTR_GteStoreSXY(&projected->posScreen[0]);
	gte_stsz(&depth);
	DrawLevelOvr1P_SetProjectedDepth(projected, depth, writeClipBytes);
}

static void DrawLevelOvr1P_ProjectCopiedGridListMidpoint_800a74a0_800a90c0(struct DrawLevelOvr1PScratchVertex *projected)
{
	u32 depth;

	CTR_GteLoadS16TripletV0(&projected->pos[0]);
	gte_rtps();
	CTR_GteStoreSXY(&projected->posScreen[0]);
	gte_stsz(&depth);

	// NOTE(aalhendi): Retail copied/default list helpers store full SZ3 at +0x10.
	DrawLevelOvr1P_StoreProjectedDepthWord(projected, depth);
}

static void DrawLevelOvr1P_ProjectCopiedGridRenderedMidpoint_800a8150_800a9d70(struct DrawLevelOvr1PScratchVertex *projected)
{
	u32 depth;
	u32 threshold = (u32)DrawLevelOvr1P_GetDepthClipThreshold();

	CTR_GteLoadS16TripletV0(&projected->pos[0]);
	gte_rtps();
	CTR_GteStoreSXY(&projected->posScreen[0]);
	gte_stsz(&depth);

	DrawLevelOvr1P_StoreProjectedDepthWord(projected, depth);
	projected->clipNear = DrawLevelOvr1P_MipsSubuSignBit(depth, threshold);
	projected->clipHalfNear = DrawLevelOvr1P_MipsSubuSignBit(depth << 1, threshold);
}

static void DrawLevelOvr1P_ProjectCompact4x1ListMidpoint_800a3a78(struct DrawLevelOvr1PScratchVertex *projected)
{
	u32 depth;

	CTR_GteLoadS16TripletV0(&projected->pos[0]);
	gte_rtps();
	CTR_GteStoreSXY(&projected->posScreen[0]);
	gte_stsz(&depth);

	// NOTE(aalhendi): Retail 0x800a3a78 stores SZ3 as a word at scratch +0x10.
	DrawLevelOvr1P_StoreProjectedDepthWord(projected, depth);
}

static void DrawLevelOvr1P_ProjectCompact4x1RenderedMidpoint_800a4594(struct DrawLevelOvr1PScratchVertex *projected)
{
	u32 depth;
	u32 threshold = (u32)DrawLevelOvr1P_GetDepthClipThreshold();

	CTR_GteLoadS16TripletV0(&projected->pos[0]);
	gte_rtps();
	CTR_GteStoreSXY(&projected->posScreen[0]);
	gte_stsz(&depth);

	DrawLevelOvr1P_StoreProjectedDepthWord(projected, depth);
	// NOTE(aalhendi): Retail 0x800a4594 overwrites bytes 0x12/0x13 after SZ3.
	projected->clipNear = DrawLevelOvr1P_MipsSubuSignBit(depth, threshold);
	projected->clipHalfNear = DrawLevelOvr1P_MipsSubuSignBit(depth << 1, threshold);
}

static void DrawLevelOvr1P_BuildMidpointValue(struct DrawLevelOvr1PScratchVertex *dstMid, const struct DrawLevelOvr1PScratchVertex *srcA,
                                              const struct DrawLevelOvr1PScratchVertex *srcB, int writeClipBytes)
{
	u8 *dstUv;
	const u8 *srcAUv;
	const u8 *srcBUv;

	for (int i = 0; i < 3; i++)
	{
		dstMid->pos[i] = (s16)(((s32)srcA->pos[i] + (s32)srcB->pos[i]) >> 1);
	}

	dstUv = (u8 *)&dstMid->flags;
	srcAUv = (const u8 *)&srcA->flags;
	srcBUv = (const u8 *)&srcB->flags;

	dstUv[0] = (u8)(((u32)srcAUv[0] + (u32)srcBUv[0]) >> 1);
	dstUv[1] = (u8)(((u32)srcAUv[1] + (u32)srcBUv[1]) >> 1);

	for (int i = 0; i < 3; i++)
	{
		dstMid->color_hi[i] = (u8)(((u32)srcA->color_hi[i] + (u32)srcB->color_hi[i]) >> 1);
	}

	// NOTE(aalhendi): Retail midpoint helpers only write RGB at 0x8..0xa; model
	// the pre-cleared code byte explicitly in native scratch.
	dstMid->color_hi[3] = 0;
	if (writeClipBytes)
	{
		DrawLevelOvr1P_ProjectCopiedGridRenderedMidpoint_800a8150_800a9d70(dstMid);
	}
	else
	{
		DrawLevelOvr1P_ProjectCopiedGridListMidpoint_800a74a0_800a90c0(dstMid);
	}
}

static void DrawLevelOvr1P_BuildCompact4x1MidpointValue(struct DrawLevelOvr1PScratchVertex *dstMid, const struct DrawLevelOvr1PScratchVertex *srcA,
                                                        const struct DrawLevelOvr1PScratchVertex *srcB, int writeClipBytes)
{
	u8 *dstUv;
	const u8 *srcAUv;
	const u8 *srcBUv;

	for (int i = 0; i < 3; i++)
	{
		dstMid->pos[i] = (s16)(((s32)srcA->pos[i] + (s32)srcB->pos[i]) >> 1);
	}

	dstUv = (u8 *)&dstMid->flags;
	srcAUv = (const u8 *)&srcA->flags;
	srcBUv = (const u8 *)&srcB->flags;

	dstUv[0] = (u8)(((u32)srcAUv[0] + (u32)srcBUv[0]) >> 1);
	dstUv[1] = (u8)(((u32)srcAUv[1] + (u32)srcBUv[1]) >> 1);

	for (int i = 0; i < 3; i++)
	{
		dstMid->color_hi[i] = (u8)(((u32)srcA->color_hi[i] + (u32)srcB->color_hi[i]) >> 1);
	}

	dstMid->color_hi[3] = 0;
	if (writeClipBytes)
	{
		DrawLevelOvr1P_ProjectCompact4x1RenderedMidpoint_800a4594(dstMid);
	}
	else
	{
		DrawLevelOvr1P_ProjectCompact4x1ListMidpoint_800a3a78(dstMid);
	}
}

static void DrawLevelOvr1P_BuildCompact4x1MidpointPair_800a3a78_800a4594(struct DrawLevelOvr1PScratchVertex *dstA, struct DrawLevelOvr1PScratchVertex *dstB,
                                                                         struct DrawLevelOvr1PScratchVertex *dstMid,
                                                                         const struct DrawLevelOvr1PScratchVertex *srcA,
                                                                         const struct DrawLevelOvr1PScratchVertex *srcB, int writeClipBytes)
{
	*dstA = *srcA;
	*dstB = *srcB;
	dstA->color_hi[3] = 0;
	dstB->color_hi[3] = 0;
	DrawLevelOvr1P_BuildCompact4x1MidpointValue(dstMid, srcA, srcB, writeClipBytes);
}

static void Ovr226_800a3a78_BuildGround4x1ListMidpointPair(struct DrawLevelOvr1PScratchVertex *dstA, struct DrawLevelOvr1PScratchVertex *dstB,
                                                           struct DrawLevelOvr1PScratchVertex *dstMid, const struct DrawLevelOvr1PScratchVertex *srcA,
                                                           const struct DrawLevelOvr1PScratchVertex *srcB)
{
	u8 *dstABytes = (u8 *)dstA;
	u8 *dstBBytes = (u8 *)dstB;
	u8 *dstMidBytes = (u8 *)dstMid;
	const u8 *srcABytes = (const u8 *)srcA;
	const u8 *srcBBytes = (const u8 *)srcB;
	u32 depth;

	for (int i = 0; i < 3; i++)
	{
		dstA->pos[i] = srcA->pos[i];
		dstB->pos[i] = srcB->pos[i];
		dstMid->pos[i] = (s16)(((s32)srcA->pos[i] + (s32)srcB->pos[i]) >> 1);
	}

	CTR_GteLoadS16TripletV0(&dstMid->pos[0]);
	gte_rtps();

	for (int offset = 8; offset <= 10; offset++)
	{
		dstABytes[offset] = srcABytes[offset];
		dstBBytes[offset] = srcBBytes[offset];
		dstMidBytes[offset] = (u8)(((u32)srcABytes[offset] + (u32)srcBBytes[offset]) >> 1);
	}

	for (int offset = 6; offset <= 7; offset++)
	{
		dstABytes[offset] = srcABytes[offset];
		dstBBytes[offset] = srcBBytes[offset];
		dstMidBytes[offset] = (u8)(((u32)srcABytes[offset] + (u32)srcBBytes[offset]) >> 1);
	}

	DrawLevelOvr1P_CopyProjectedScreenDepth(dstA, srcA);
	DrawLevelOvr1P_CopyProjectedScreenDepth(dstB, srcB);
	CTR_GteStoreSXY(&dstMid->posScreen[0]);
	gte_stsz(&depth);
	DrawLevelOvr1P_StoreProjectedDepthWord(dstMid, depth);
}

static void Ovr226_800a3a78_BuildGround4x1ListSubdivisionFrame(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                               const int *indices)
{
	Ovr226_800a3a78_BuildGround4x1ListMidpointPair(&sub[0], &sub[1], &sub[4], &projected[indices[0]], &projected[indices[1]]);
	Ovr226_800a3a78_BuildGround4x1ListMidpointPair(&sub[2], &sub[3], &sub[8], &projected[indices[2]], &projected[indices[3]]);
}

static void Ovr226_800a560c_BuildGround4x2ListEdgeMidpoint(struct DrawLevelOvr1PScratchVertex *dstA, struct DrawLevelOvr1PScratchVertex *dstMid,
                                                           const struct DrawLevelOvr1PScratchVertex *srcA, const struct DrawLevelOvr1PScratchVertex *srcB)
{
	u8 *dstABytes = (u8 *)dstA;
	u8 *dstMidBytes = (u8 *)dstMid;
	const u8 *srcABytes = (const u8 *)srcA;
	const u8 *srcBBytes = (const u8 *)srcB;
	u32 depth;

	for (int i = 0; i < 3; i++)
	{
		dstA->pos[i] = srcA->pos[i];
		dstMid->pos[i] = (s16)(((s32)srcA->pos[i] + (s32)srcB->pos[i]) >> 1);
	}

	CTR_GteLoadS16TripletV0(&dstMid->pos[0]);
	gte_rtps();

	for (int offset = 8; offset <= 10; offset++)
	{
		dstABytes[offset] = srcABytes[offset];
		dstMidBytes[offset] = (u8)(((u32)srcABytes[offset] + (u32)srcBBytes[offset]) >> 1);
	}

	for (int offset = 6; offset <= 7; offset++)
	{
		dstABytes[offset] = srcABytes[offset];
		dstMidBytes[offset] = (u8)(((u32)srcABytes[offset] + (u32)srcBBytes[offset]) >> 1);
	}

	DrawLevelOvr1P_CopyProjectedScreenDepth(dstA, srcA);
	CTR_GteStoreSXY(&dstMid->posScreen[0]);
	gte_stsz(&depth);
	DrawLevelOvr1P_StoreProjectedDepthWord(dstMid, depth);
}

static void Ovr226_800a56f4_BuildGround4x2ListPairMidpoint(struct DrawLevelOvr1PScratchVertex *dstA, struct DrawLevelOvr1PScratchVertex *dstB,
                                                           struct DrawLevelOvr1PScratchVertex *dstMid, const struct DrawLevelOvr1PScratchVertex *srcA,
                                                           const struct DrawLevelOvr1PScratchVertex *srcB)
{
	u8 *dstABytes = (u8 *)dstA;
	u8 *dstBBytes = (u8 *)dstB;
	u8 *dstMidBytes = (u8 *)dstMid;
	const u8 *srcABytes = (const u8 *)srcA;
	const u8 *srcBBytes = (const u8 *)srcB;
	u32 depth;

	for (int i = 0; i < 3; i++)
	{
		dstA->pos[i] = srcA->pos[i];
		dstB->pos[i] = srcB->pos[i];
		dstMid->pos[i] = (s16)(((s32)srcA->pos[i] + (s32)srcB->pos[i]) >> 1);
	}

	CTR_GteLoadS16TripletV0(&dstMid->pos[0]);
	gte_rtps();

	for (int offset = 8; offset <= 10; offset++)
	{
		dstABytes[offset] = srcABytes[offset];
		dstBBytes[offset] = srcBBytes[offset];
		dstMidBytes[offset] = (u8)(((u32)srcABytes[offset] + (u32)srcBBytes[offset]) >> 1);
	}

	for (int offset = 6; offset <= 7; offset++)
	{
		dstABytes[offset] = srcABytes[offset];
		dstBBytes[offset] = srcBBytes[offset];
		dstMidBytes[offset] = (u8)(((u32)srcABytes[offset] + (u32)srcBBytes[offset]) >> 1);
	}

	DrawLevelOvr1P_CopyProjectedScreenDepth(dstA, srcA);
	DrawLevelOvr1P_CopyProjectedScreenDepth(dstB, srcB);
	CTR_GteStoreSXY(&dstMid->posScreen[0]);
	gte_stsz(&depth);
	DrawLevelOvr1P_StoreProjectedDepthWord(dstMid, depth);
}

static void Ovr226_800a56f4_BuildGround4x2ListSubdivisionFrame(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                               const int *indices)
{
	Ovr226_800a56f4_BuildGround4x2ListPairMidpoint(&sub[0], &sub[1], &sub[4], &projected[indices[0]], &projected[indices[1]]);
	Ovr226_800a56f4_BuildGround4x2ListPairMidpoint(&sub[2], &sub[3], &sub[8], &projected[indices[2]], &projected[indices[3]]);

	if (DrawLevelOvr1P_IsDeepestSubdivisionFrame(sub))
	{
		return;
	}

	Ovr226_800a560c_BuildGround4x2ListEdgeMidpoint(&sub[2], &sub[5], &projected[indices[2]], &projected[indices[0]]);
	Ovr226_800a560c_BuildGround4x2ListEdgeMidpoint(&sub[1], &sub[6], &projected[indices[1]], &projected[indices[2]]);
	Ovr226_800a560c_BuildGround4x2ListEdgeMidpoint(&sub[3], &sub[7], &projected[indices[3]], &projected[indices[1]]);
}

static void Ovr226_800a6510_BuildGround4x2RenderedEdgeMidpoint(struct DrawLevelOvr1PScratchVertex *dstA, struct DrawLevelOvr1PScratchVertex *dstMid,
                                                               const struct DrawLevelOvr1PScratchVertex *srcA, const struct DrawLevelOvr1PScratchVertex *srcB)
{
	u8 *dstABytes = (u8 *)dstA;
	u8 *dstMidBytes = (u8 *)dstMid;
	const u8 *srcABytes = (const u8 *)srcA;
	const u8 *srcBBytes = (const u8 *)srcB;
	u32 depth;

	for (int i = 0; i < 3; i++)
	{
		dstA->pos[i] = srcA->pos[i];
		dstMid->pos[i] = (s16)(((s32)srcA->pos[i] + (s32)srcB->pos[i]) >> 1);
	}

	CTR_GteLoadS16TripletV0(&dstMid->pos[0]);
	gte_rtps();

	for (int offset = 8; offset <= 10; offset++)
	{
		dstABytes[offset] = srcABytes[offset];
		dstMidBytes[offset] = (u8)(((u32)srcABytes[offset] + (u32)srcBBytes[offset]) >> 1);
	}

	for (int offset = 6; offset <= 7; offset++)
	{
		dstABytes[offset] = srcABytes[offset];
		dstMidBytes[offset] = (u8)(((u32)srcABytes[offset] + (u32)srcBBytes[offset]) >> 1);
	}

	DrawLevelOvr1P_CopyProjectedScreenDepth(dstA, srcA);
	CTR_GteStoreSXY(&dstMid->posScreen[0]);
	gte_stsz(&depth);
	DrawLevelOvr1P_SetProjectedDepth(dstMid, depth, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
}

static void Ovr226_800a6510_BuildGround4x2RenderedSubdivisionFrame(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                                   const int *indices)
{
	Ovr226_800a6510_BuildGround4x2RenderedEdgeMidpoint(&sub[0], &sub[4], &projected[indices[0]], &projected[indices[1]]);
	Ovr226_800a6510_BuildGround4x2RenderedEdgeMidpoint(&sub[2], &sub[8], &projected[indices[2]], &projected[indices[3]]);
	Ovr226_800a6510_BuildGround4x2RenderedEdgeMidpoint(&sub[2], &sub[5], &projected[indices[2]], &projected[indices[0]]);
	Ovr226_800a6510_BuildGround4x2RenderedEdgeMidpoint(&sub[1], &sub[6], &projected[indices[1]], &projected[indices[2]]);
	Ovr226_800a6510_BuildGround4x2RenderedEdgeMidpoint(&sub[3], &sub[7], &projected[indices[3]], &projected[indices[1]]);
}

static void Ovr226_800a74a0_BuildDynamicListSubdivideMidpoint(struct DrawLevelOvr1PScratchVertex *dstA, struct DrawLevelOvr1PScratchVertex *dstMid,
                                                              const struct DrawLevelOvr1PScratchVertex *srcA, const struct DrawLevelOvr1PScratchVertex *srcB)
{
	const u8 *srcAUv = (const u8 *)&srcA->flags;
	const u8 *srcBUv = (const u8 *)&srcB->flags;
	u8 *dstAUv = (u8 *)&dstA->flags;
	u8 *dstMidUv = (u8 *)&dstMid->flags;
	u32 depth;

	for (int i = 0; i < 3; i++)
	{
		dstA->pos[i] = srcA->pos[i];
		dstMid->pos[i] = (s16)(((s32)srcA->pos[i] + (s32)srcB->pos[i]) >> 1);
	}

	CTR_GteLoadS16TripletV0(&dstMid->pos[0]);
	gte_rtps();

	for (int i = 0; i < 3; i++)
	{
		dstA->color_hi[i] = srcA->color_hi[i];
		dstMid->color_hi[i] = (u8)(((u32)srcA->color_hi[i] + (u32)srcB->color_hi[i]) >> 1);
	}

	for (int i = 0; i < 2; i++)
	{
		dstAUv[i] = srcAUv[i];
		dstMidUv[i] = (u8)(((u32)srcAUv[i] + (u32)srcBUv[i]) >> 1);
	}

	DrawLevelOvr1P_CopyProjectedScreenDepth(dstA, srcA);
	CTR_GteStoreSXY(&dstMid->posScreen[0]);
	gte_stsz(&depth);
	DrawLevelOvr1P_StoreProjectedDepthWord(dstMid, depth);
}

static void Ovr226_800a74a0_BuildDynamicListSubdivisionFrame(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                             const int *indices)
{
	Ovr226_800a74a0_BuildDynamicListSubdivideMidpoint(&sub[0], &sub[4], &projected[indices[0]], &projected[indices[1]]);
	Ovr226_800a74a0_BuildDynamicListSubdivideMidpoint(&sub[1], &sub[7], &projected[indices[1]], &projected[indices[3]]);
	Ovr226_800a74a0_BuildDynamicListSubdivideMidpoint(&sub[2], &sub[5], &projected[indices[2]], &projected[indices[0]]);
	Ovr226_800a74a0_BuildDynamicListSubdivideMidpoint(&sub[3], &sub[8], &projected[indices[3]], &projected[indices[2]]);
	Ovr226_800a74a0_BuildDynamicListSubdivideMidpoint(&sub[1], &sub[6], &projected[indices[1]], &projected[indices[2]]);
}

static void Ovr226_800a90c0_BuildWideDynamicSubdivideMidpoint(struct DrawLevelOvr1PScratchVertex *dstA, struct DrawLevelOvr1PScratchVertex *dstMid,
                                                              const struct DrawLevelOvr1PScratchVertex *srcA, const struct DrawLevelOvr1PScratchVertex *srcB)
{
	Ovr226_800a74a0_BuildDynamicListSubdivideMidpoint(dstA, dstMid, srcA, srcB);
}

static void Ovr226_800a90c0_BuildWideDynamicSubdivisionFrame(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                             const int *indices)
{
	Ovr226_800a90c0_BuildWideDynamicSubdivideMidpoint(&sub[0], &sub[4], &projected[indices[0]], &projected[indices[1]]);
	Ovr226_800a90c0_BuildWideDynamicSubdivideMidpoint(&sub[1], &sub[7], &projected[indices[1]], &projected[indices[3]]);
	Ovr226_800a90c0_BuildWideDynamicSubdivideMidpoint(&sub[2], &sub[5], &projected[indices[2]], &projected[indices[0]]);
	Ovr226_800a90c0_BuildWideDynamicSubdivideMidpoint(&sub[3], &sub[8], &projected[indices[3]], &projected[indices[2]]);
	Ovr226_800a90c0_BuildWideDynamicSubdivideMidpoint(&sub[4], &sub[6], &sub[4], &sub[8]);
}

static void Ovr226_800a8150_BuildDynamicRenderedSubdivideMidpoint(struct DrawLevelOvr1PScratchVertex *dstA, struct DrawLevelOvr1PScratchVertex *dstMid,
                                                                  const struct DrawLevelOvr1PScratchVertex *srcA,
                                                                  const struct DrawLevelOvr1PScratchVertex *srcB)
{
	const u8 *srcAUv = (const u8 *)&srcA->flags;
	const u8 *srcBUv = (const u8 *)&srcB->flags;
	u8 *dstAUv = (u8 *)&dstA->flags;
	u8 *dstMidUv = (u8 *)&dstMid->flags;
	u32 depth;

	for (int i = 0; i < 3; i++)
	{
		dstA->pos[i] = srcA->pos[i];
		dstMid->pos[i] = (s16)(((s32)srcA->pos[i] + (s32)srcB->pos[i]) >> 1);
	}

	CTR_GteLoadS16TripletV0(&dstMid->pos[0]);
	gte_rtps();

	for (int i = 0; i < 3; i++)
	{
		dstA->color_hi[i] = srcA->color_hi[i];
		dstMid->color_hi[i] = (u8)(((u32)srcA->color_hi[i] + (u32)srcB->color_hi[i]) >> 1);
	}

	for (int i = 0; i < 2; i++)
	{
		dstAUv[i] = srcAUv[i];
		dstMidUv[i] = (u8)(((u32)srcAUv[i] + (u32)srcBUv[i]) >> 1);
	}

	DrawLevelOvr1P_CopyProjectedScreenDepth(dstA, srcA);
	CTR_GteStoreSXY(&dstMid->posScreen[0]);
	gte_stsz(&depth);
	DrawLevelOvr1P_SetProjectedDepth(dstMid, depth, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
}

static void Ovr226_800a8150_BuildDynamicRenderedSubdivisionFrame(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                                 const int *indices)
{
	Ovr226_800a8150_BuildDynamicRenderedSubdivideMidpoint(&sub[0], &sub[4], &projected[indices[0]], &projected[indices[1]]);
	Ovr226_800a8150_BuildDynamicRenderedSubdivideMidpoint(&sub[1], &sub[7], &projected[indices[1]], &projected[indices[3]]);
	Ovr226_800a8150_BuildDynamicRenderedSubdivideMidpoint(&sub[2], &sub[5], &projected[indices[2]], &projected[indices[0]]);
	Ovr226_800a8150_BuildDynamicRenderedSubdivideMidpoint(&sub[3], &sub[8], &projected[indices[3]], &projected[indices[2]]);
	Ovr226_800a8150_BuildDynamicRenderedSubdivideMidpoint(&sub[1], &sub[6], &projected[indices[1]], &projected[indices[2]]);
}

static void Ovr226_800a9d70_BuildQuad4x4RenderedSubdivideMidpoint(struct DrawLevelOvr1PScratchVertex *dstA, struct DrawLevelOvr1PScratchVertex *dstMid,
                                                                  const struct DrawLevelOvr1PScratchVertex *srcA,
                                                                  const struct DrawLevelOvr1PScratchVertex *srcB)
{
	Ovr226_800a8150_BuildDynamicRenderedSubdivideMidpoint(dstA, dstMid, srcA, srcB);
}

static void Ovr226_800a9d70_BuildQuad4x4RenderedSubdivisionFrame(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                                 const int *indices)
{
	Ovr226_800a9d70_BuildQuad4x4RenderedSubdivideMidpoint(&sub[0], &sub[4], &projected[indices[0]], &projected[indices[1]]);
	Ovr226_800a9d70_BuildQuad4x4RenderedSubdivideMidpoint(&sub[1], &sub[7], &projected[indices[1]], &projected[indices[3]]);
	Ovr226_800a9d70_BuildQuad4x4RenderedSubdivideMidpoint(&sub[2], &sub[5], &projected[indices[2]], &projected[indices[0]]);
	Ovr226_800a9d70_BuildQuad4x4RenderedSubdivideMidpoint(&sub[3], &sub[8], &projected[indices[3]], &projected[indices[2]]);
	Ovr226_800a9d70_BuildQuad4x4RenderedSubdivideMidpoint(&sub[4], &sub[6], &sub[4], &sub[8]);
}

static void Ovr226_800a4594_BuildGround4x1RenderedMidpointPair(struct DrawLevelOvr1PScratchVertex *dstA, struct DrawLevelOvr1PScratchVertex *dstB,
                                                               struct DrawLevelOvr1PScratchVertex *dstMid, const struct DrawLevelOvr1PScratchVertex *srcA,
                                                               const struct DrawLevelOvr1PScratchVertex *srcB)
{
	u8 *dstABytes = (u8 *)dstA;
	u8 *dstBBytes = (u8 *)dstB;
	u8 *dstMidBytes = (u8 *)dstMid;
	const u8 *srcABytes = (const u8 *)srcA;
	const u8 *srcBBytes = (const u8 *)srcB;
	u32 depth;

	for (int i = 0; i < 3; i++)
	{
		dstA->pos[i] = srcA->pos[i];
		dstB->pos[i] = srcB->pos[i];
		dstMid->pos[i] = (s16)(((s32)srcA->pos[i] + (s32)srcB->pos[i]) >> 1);
	}

	CTR_GteLoadS16TripletV0(&dstMid->pos[0]);
	gte_rtps();

	for (int offset = 8; offset <= 10; offset++)
	{
		dstABytes[offset] = srcABytes[offset];
		dstBBytes[offset] = srcBBytes[offset];
		dstMidBytes[offset] = (u8)(((u32)srcABytes[offset] + (u32)srcBBytes[offset]) >> 1);
	}

	for (int offset = 6; offset <= 7; offset++)
	{
		dstABytes[offset] = srcABytes[offset];
		dstBBytes[offset] = srcBBytes[offset];
		dstMidBytes[offset] = (u8)(((u32)srcABytes[offset] + (u32)srcBBytes[offset]) >> 1);
	}

	DrawLevelOvr1P_CopyProjectedScreenDepth(dstA, srcA);
	DrawLevelOvr1P_CopyProjectedScreenDepth(dstB, srcB);
	CTR_GteStoreSXY(&dstMid->posScreen[0]);
	gte_stsz(&depth);
	DrawLevelOvr1P_SetProjectedDepth(dstMid, depth, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
}

static void Ovr226_800a4594_BuildGround4x1RenderedSubdivisionFrame(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                                   const int *indices)
{
	Ovr226_800a4594_BuildGround4x1RenderedMidpointPair(&sub[0], &sub[1], &sub[4], &projected[indices[0]], &projected[indices[1]]);
	Ovr226_800a4594_BuildGround4x1RenderedMidpointPair(&sub[2], &sub[3], &sub[8], &projected[indices[2]], &projected[indices[3]]);
}

static void DrawLevelOvr1P_BuildMidpointFromFirstEndpoint(struct DrawLevelOvr1PScratchVertex *dstA, struct DrawLevelOvr1PScratchVertex *dstMid,
                                                          const struct DrawLevelOvr1PScratchVertex *srcA, const struct DrawLevelOvr1PScratchVertex *srcB,
                                                          int writeClipBytes)
{
	*dstA = *srcA;
	dstA->color_hi[3] = 0;
	DrawLevelOvr1P_BuildMidpointValue(dstMid, srcA, srcB, writeClipBytes);
}

static void Ovr226_800a17d8_BuildFullDynamicSubdivideMidpoint(struct DrawLevelOvr1PScratchVertex *dstA, struct DrawLevelOvr1PScratchVertex *dstMid,
                                                              const struct DrawLevelOvr1PScratchVertex *srcA, const struct DrawLevelOvr1PScratchVertex *srcB)
{
	const u8 *srcABytes = (const u8 *)&srcA->flags;
	const u8 *srcBBytes = (const u8 *)&srcB->flags;
	u8 *dstABytes = (u8 *)&dstA->flags;
	u8 *dstMidBytes = (u8 *)&dstMid->flags;
	u32 depth;

	for (int i = 0; i < 3; i++)
	{
		dstA->pos[i] = srcA->pos[i];
		dstMid->pos[i] = (s16)(((s32)srcA->pos[i] + (s32)srcB->pos[i]) >> 1);
	}

	CTR_GteLoadS16TripletV0(&dstMid->pos[0]);
	gte_rtps();

	for (int i = 0; i < 3; i++)
	{
		dstA->color_hi[i] = srcA->color_hi[i];
		dstMid->color_hi[i] = (u8)(((u32)srcA->color_hi[i] + (u32)srcB->color_hi[i]) >> 1);
	}

	for (int i = 0; i < 2; i++)
	{
		dstABytes[i] = srcABytes[i];
		dstMidBytes[i] = (u8)(((u32)srcABytes[i] + (u32)srcBBytes[i]) >> 1);
	}

	DrawLevelOvr1P_CopyProjectedScreenDepth(dstA, srcA);
	CTR_GteStoreSXY(&dstMid->posScreen[0]);
	gte_stsz(&depth);
	DrawLevelOvr1P_StoreProjectedDepthWord(dstMid, depth);
}

static void Ovr226_800a17d8_BuildFullDynamicSubdivisionFrame(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                             const int *indices)
{
	Ovr226_800a17d8_BuildFullDynamicSubdivideMidpoint(&sub[0], &sub[4], &projected[indices[0]], &projected[indices[1]]);
	Ovr226_800a17d8_BuildFullDynamicSubdivideMidpoint(&sub[1], &sub[7], &projected[indices[1]], &projected[indices[3]]);
	Ovr226_800a17d8_BuildFullDynamicSubdivideMidpoint(&sub[2], &sub[5], &projected[indices[2]], &projected[indices[0]]);
	Ovr226_800a17d8_BuildFullDynamicSubdivideMidpoint(&sub[3], &sub[8], &projected[indices[3]], &projected[indices[2]]);
	Ovr226_800a17d8_BuildFullDynamicSubdivideMidpoint(&sub[1], &sub[6], &projected[indices[1]], &projected[indices[2]]);
}

static u32 DrawLevelOvr1P_GetProjectedMaxDepth(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	u32 maxDepth = projected[indices[0]].depth;

	for (int i = 1; i < 4; i++)
	{
		if (maxDepth < projected[indices[i]].depth)
		{
			maxDepth = projected[indices[i]].depth;
		}
	}

	return maxDepth;
}

static u32 DrawLevelOvr1P_GetProjectedTriMaxDepth(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	u32 maxDepth = projected[indices[0]].depth;

	for (int i = 1; i < 3; i++)
	{
		if (maxDepth < projected[indices[i]].depth)
		{
			maxDepth = projected[indices[i]].depth;
		}
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
		u32 slotWord = DrawLevelOvr1P_GetProjectedOtSlotWord(projected, faceIndex);
		s8 drawOrder = DrawLevelOvr1P_ReadRetailQuadBlockByte(block, 0x18 + (slotWord >> 2));

		otIndex = (s32)(maxDepth >> 6) + drawOrder;
	}

	if (otIndex < 0)
	{
		return 0;
	}

	if (otIndex > DRAW_LEVEL_OVR1P_MAX_OT_INDEX)
	{
		return DRAW_LEVEL_OVR1P_MAX_OT_INDEX;
	}

	return otIndex;
}

static void Ovr226_800a108c_800a1b10_SetFullDynamicInheritedOtIndex(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected,
                                                                    u32 maxDepth, int faceIndex)
{
	// NOTE(aalhendi): Retail full-dynamic paths keep the selected OT entry in `gp`
	// until 0x800a1cc4 links the direct primitive.
	sOvr226_800a1cc4_InheritedOtIndex = DrawLevelOvr1P_GetProjectedOtIndex(block, projected, maxDepth, faceIndex);
}

static int DrawLevelOvr1P_ResolveProjectedOtIndex(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected, u32 maxDepth,
                                                  int faceIndex, int otIndexOverride)
{
	if (otIndexOverride >= 0)
	{
		return otIndexOverride;
	}

	return DrawLevelOvr1P_GetProjectedOtIndex(block, projected, maxDepth, faceIndex);
}

static u32 DrawLevelOvr1P_GetWaterTopFrameOtDepth_800a2690_800a31f0(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	s32 depth0 = projected[indices[0]].depth;
	s32 depth1 = projected[indices[1]].depth;
	s32 depth2 = projected[indices[2]].depth;
	s32 depth3 = projected[indices[3]].depth;
	s32 delta;
	s32 deltaToDepth3;
	u32 selectedDepth;

	selectedDepth = depth0;
	delta = depth0 - depth1;
	if (delta <= 0)
	{
		selectedDepth = depth1;
	}

	delta = (s32)selectedDepth - depth2;
	deltaToDepth3 = (s32)selectedDepth - depth3;
	if (delta <= 0)
	{
		selectedDepth = depth2;
	}

	// NOTE(aalhendi): Retail water top-frame OT setup compares vertex 3 from the
	// pre-depth2 selected value in the branch delay slot.
	if (deltaToDepth3 <= 0)
	{
		selectedDepth = depth3;
	}

	return selectedDepth;
}

static int DrawLevelOvr1P_ResolveWaterInheritedOtIndex(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                                       int faceIndex, int inheritedOtIndex)
{
	if (inheritedOtIndex >= 0)
	{
		return inheritedOtIndex;
	}

	// NOTE(aalhendi): Retail water paths compute GP/OT only at the top scratch
	// frame, then recursive child helpers inherit it.
	if (projected == DrawLevelOvr1P_GetScratchVertices())
	{
		return DrawLevelOvr1P_GetProjectedOtIndex(block, projected, DrawLevelOvr1P_GetWaterTopFrameOtDepth_800a2690_800a31f0(projected, indices), faceIndex);
	}

	return DRAW_LEVEL_OVR1P_OT_INDEX_AUTO;
}

static uint32_t *Ovr226_800a2690_ResolveWaterListInheritedOtEntry(struct PushBuffer *pb, const struct QuadBlock *block,
                                                                  const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                                  uint32_t *inheritedOtEntry)
{
	u32 slotWord;
	s8 drawOrder;
	u32 selectedDepth;
	s32 otIndex;

	if (inheritedOtEntry != NULL)
	{
		return inheritedOtEntry;
	}

	// NOTE(aalhendi): Retail 0x800a2690..0x800a26d8 computes GP only on the
	// top water-list scratch frame and does not clamp the signed OT index.
	if (projected != DrawLevelOvr1P_GetScratchVertices())
	{
		return NULL;
	}

	selectedDepth = DrawLevelOvr1P_GetWaterTopFrameOtDepth_800a2690_800a31f0(projected, indices);
	slotWord = DrawLevelOvr1P_GetProjectedOtSlotWord(projected, faceIndex);
	drawOrder = DrawLevelOvr1P_ReadRetailQuadBlockByte(block, 0x18 + (slotWord >> 2));
	otIndex = (s32)(selectedDepth >> 6) + drawOrder;

	return &pb->ptrOT[otIndex];
}

static int DrawLevelOvr1P_ShouldSetSemiTransFromTpage(u16 tpage)
{
	// NOTE(aalhendi): Retail 226 direct/clip emit helpers use code 0x36/0x3e
	// unless the tpage blend bits are exactly 0x60.
	return (tpage & 0x60) != 0x60;
}

static void DrawLevelOvr1P_AddRawPrimToOt(struct PrimMem *primMem, void *packet, u32 wordCount, uint32_t *otEntry)
{
	u32 *packetTag = packet;

	*packetTag = CtrGpu_PackOTTag(*otEntry, wordCount << 24);
	*otEntry = CtrGpu_PrimToOTLink24(packet);
	// NOTE(aalhendi): Retail keeps the overlay primitive count in `sp`, seeded
	// from PrimMem+0x14 and stored back at the epilogue.
	primMem->primitiveCount++;
}

static u32 DrawLevelOvr1P_GetProjectedColorCode(const struct DrawLevelOvr1PScratchVertex *projected, u32 code)
{
	return DrawLevelOvr1P_ReadPackedWord(projected->color_hi) | (code << 24);
}

static u32 DrawLevelOvr1P_GetProjectedColorWord(const struct DrawLevelOvr1PScratchVertex *projected)
{
	return DrawLevelOvr1P_ReadPackedWord(projected->color_hi);
}

static void DrawLevelOvr1P_WriteProjectedGT3(POLY_GT3 *poly, const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, u32 code, u32 uv0,
                                             u32 uv1, u32 uv2)
{
	CtrGpu_WriteColorCode(&poly->r0, DrawLevelOvr1P_GetProjectedColorCode(&projected[indices[0]], code));
	CtrGpu_WritePackedXY(&poly->x0, DrawLevelOvr1P_PackProjectedSxy(&projected[indices[0]]));
	CtrGpu_WritePackedUVWord(&poly->u0, uv0);
	CtrGpu_WriteColorCode(&poly->r1, DrawLevelOvr1P_GetProjectedColorCode(&projected[indices[1]], 0));
	CtrGpu_WritePackedXY(&poly->x1, DrawLevelOvr1P_PackProjectedSxy(&projected[indices[1]]));
	CtrGpu_WritePackedUVWord(&poly->u1, uv1);
	CtrGpu_WriteColorCode(&poly->r2, DrawLevelOvr1P_GetProjectedColorCode(&projected[indices[2]], 0));
	CtrGpu_WritePackedXY(&poly->x2, DrawLevelOvr1P_PackProjectedSxy(&projected[indices[2]]));
	CtrGpu_WritePackedUVWord(&poly->u2, uv2);
}

static void DrawLevelOvr1P_WriteProjectedGT4(POLY_GT4 *poly, const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, u32 code, u32 uv0,
                                             u32 uv1, u32 uv2)
{
	DrawLevelOvr1P_WriteProjectedGT3((POLY_GT3 *)poly, projected, indices, code, uv0, uv1, uv2);
	CtrGpu_WriteColorCode(&poly->r3, DrawLevelOvr1P_GetProjectedColorCode(&projected[indices[3]], 0));
	CtrGpu_WritePackedXY(&poly->x3, DrawLevelOvr1P_PackProjectedSxy(&projected[indices[3]]));
	CtrGpu_WritePackedUVWord(&poly->u3, uv2 >> 16);
}

static u32 DrawLevelOvr1P_SelectRawPrimitiveCode(u32 uv1Word, u32 semiTransCode, u32 opaqueCode)
{
	return ((uv1Word & 0x00600000) != 0x00600000) ? semiTransCode : opaqueCode;
}

static int DrawLevelOvr1P_EmitPreparedProjectedQuadRawCodeAtOtEntry(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                    const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                                                    const struct TextureLayout *texture, uint32_t *otEntry, int primCodeOverride)
{
	POLY_GT4 *prim;
	POLY_GT4 *nextPrim;
	u32 code;
	u32 uv0 = 0;
	u32 uv1 = 0;
	u32 uv2 = 0;

	(void)pb;
	(void)block;

	prim = primMem->cursor;
	nextPrim = prim + 1;

	DrawLevelOvr1P_StoreProjectedDirectUvScratch(projected, indices, 4);
	uv0 = DrawLevelOvr1P_Scratch()->uv.uv0;
	uv1 = DrawLevelOvr1P_Scratch()->uv.uv1;
	uv2 = DrawLevelOvr1P_Scratch()->uv.uv2;

	// NOTE(aalhendi): Generic retail raw GT4 writers select from scratch
	// UV1/tpage; water terminal labels pass a fixed-code override.
	(void)texture;
	code = primCodeOverride >= 0 ? (u32)primCodeOverride : DrawLevelOvr1P_SelectRawPrimitiveCode(uv1, 0x3e, 0x3c);

	DrawLevelOvr1P_WriteProjectedGT4(prim, projected, indices, code, uv0, uv1, uv2);
	DrawLevelOvr1P_AddRawPrimToOt(primMem, prim, 12, otEntry);
	primMem->cursor = nextPrim;
	return 1;
}

static int DrawLevelOvr1P_EmitPreparedProjectedQuadRawCodeAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                               const struct TextureLayout *texture, u32 maxDepth, int otIndexOverride, int primCodeOverride)
{
	int otIndex = DrawLevelOvr1P_ResolveProjectedOtIndex(block, projected, maxDepth, faceIndex, otIndexOverride);

	return DrawLevelOvr1P_EmitPreparedProjectedQuadRawCodeAtOtEntry(pb, primMem, block, projected, indices, texture, &pb->ptrOT[otIndex], primCodeOverride);
}

static int DrawLevelOvr1P_EmitPreparedProjectedQuadRawAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                           const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                           const struct TextureLayout *texture, u32 maxDepth, int otIndexOverride)
{
	return DrawLevelOvr1P_EmitPreparedProjectedQuadRawCodeAtOt(pb, primMem, block, projected, indices, faceIndex, texture, maxDepth, otIndexOverride,
	                                                           DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
}

static int DrawLevelOvr1P_EmitPreparedProjectedQuadRaw(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                       const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                       const struct TextureLayout *texture, u32 maxDepth)
{
	return DrawLevelOvr1P_EmitPreparedProjectedQuadRawAtOt(pb, primMem, block, projected, indices, faceIndex, texture, maxDepth,
	                                                       DRAW_LEVEL_OVR1P_OT_INDEX_AUTO);
}

static int DrawLevelOvr1P_EmitPreparedProjectedTriRawCodeAtOtEntry(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                   const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                                                   const struct TextureLayout *texture, uint32_t *otEntry, int primCodeOverride)
{
	POLY_GT3 *prim;
	POLY_GT3 *nextPrim;
	u32 code;
	u32 uv0 = 0;
	u32 uv1 = 0;
	u32 uv2 = 0;

	(void)pb;
	(void)block;

	prim = primMem->cursor;
	nextPrim = prim + 1;

	DrawLevelOvr1P_StoreProjectedDirectUvScratch(projected, indices, 3);
	uv0 = DrawLevelOvr1P_Scratch()->uv.uv0;
	uv1 = DrawLevelOvr1P_Scratch()->uv.uv1;
	uv2 = DrawLevelOvr1P_Scratch()->uv.uv2;

	// NOTE(aalhendi): Generic retail raw GT3 writers select from scratch
	// UV1/tpage; water terminal labels pass a fixed-code override.
	(void)texture;
	code = primCodeOverride >= 0 ? (u32)primCodeOverride : DrawLevelOvr1P_SelectRawPrimitiveCode(uv1, 0x36, 0x34);

	DrawLevelOvr1P_WriteProjectedGT3(prim, projected, indices, code, uv0, uv1, uv2);
	DrawLevelOvr1P_AddRawPrimToOt(primMem, prim, 9, otEntry);
	primMem->cursor = nextPrim;
	return 1;
}

static int DrawLevelOvr1P_EmitPreparedProjectedTriRawCodeAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                              const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                              const struct TextureLayout *texture, u32 maxDepth, int otIndexOverride, int primCodeOverride)
{
	int otIndex = DrawLevelOvr1P_ResolveProjectedOtIndex(block, projected, maxDepth, faceIndex, otIndexOverride);

	return DrawLevelOvr1P_EmitPreparedProjectedTriRawCodeAtOtEntry(pb, primMem, block, projected, indices, texture, &pb->ptrOT[otIndex], primCodeOverride);
}

static int DrawLevelOvr1P_EmitPreparedProjectedTriRawAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                          const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                          const struct TextureLayout *texture, u32 maxDepth, int otIndexOverride)
{
	return DrawLevelOvr1P_EmitPreparedProjectedTriRawCodeAtOt(pb, primMem, block, projected, indices, faceIndex, texture, maxDepth, otIndexOverride,
	                                                          DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
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
		{
			return 0;
		}
	}

	return 1;
}

static int DrawLevelOvr1P_HasProjectedVertexNear(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count)
{
	for (int i = 0; i < count; i++)
	{
		if (DrawLevelOvr1P_IsClipByteSet(projected[indices[i]].clipNear))
		{
			return 1;
		}
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
	return DrawLevelOvr1P_ReadPackedWord((const u8 *)base + offset);
}

static void Ovr226_800a1408_AdjustFullDynamicMidVertex(struct DrawLevelOvr1PScratchVertex *projected, struct LevVertex *vertices, const struct QuadBlock *block,
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

	factor = (s32)((DrawLevelOvr1P_ReadWord(mid, 0x10) - DrawLevelOvr1P_RenderScratch()->fullDynamicFadeDepthStart) << 2);
	if (factor < 0)
	{
		return;
	}

	for (int i = 0; i < 3; i++)
	{
		midpoint[i] = (s16)(((s32)endpointA->pos[i] + (s32)endpointB->pos[i]) >> 1);
	}

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
	DrawLevelOvr1P_WritePackedWord(mid->color_hi, color);

	MTC2(DrawLevelOvr1P_ReadWord(&mid->pos[0], 0), 0);
	MTC2((u32)(s32)mid->pos[2], 1);
	gte_rtps();
	CTR_GteStoreSXY(&mid->posScreen[0]);
	gte_stsz(&depth);
	DrawLevelOvr1P_StoreProjectedDepthWord(mid, depth);
}

static void DrawLevelOvr1P_AdjustFullDynamicMidVertices(struct DrawLevelOvr1PScratchVertex *projected, struct LevVertex *vertices,
                                                        const struct QuadBlock *block)
{
	Ovr226_800a1408_AdjustFullDynamicMidVertex(projected, vertices, block, 4, 0, 1);
	Ovr226_800a1408_AdjustFullDynamicMidVertex(projected, vertices, block, 5, 0, 2);
	Ovr226_800a1408_AdjustFullDynamicMidVertex(projected, vertices, block, 6, 1, 2);
	Ovr226_800a1408_AdjustFullDynamicMidVertex(projected, vertices, block, 7, 1, 3);
	Ovr226_800a1408_AdjustFullDynamicMidVertex(projected, vertices, block, 8, 2, 3);
}

static u32 DrawLevelOvr1P_GetPreparedProjectedMaxDepthCount(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count)
{
	u32 maxDepth = projected[indices[0]].depth;

	for (int i = 1; i < count; i++)
	{
		if (maxDepth < projected[indices[i]].depth)
		{
			maxDepth = projected[indices[i]].depth;
		}
	}

	return maxDepth;
}

static int DrawLevelOvr1P_SourceInsideClipRecordWindow(const struct DrawLevelOvr1PScratchVertex *src)
{
	const s16 *center = DrawLevelOvr1P_Scratch()->projectedCenter.v;
	s32 x = (s32)src->pos[0] - center[0];
	s32 y = (s32)src->pos[1] - center[1];
	s32 z = (s32)src->pos[2] - center[2];
	u32 insideBits;

	if (x < 0)
	{
		x = -x;
	}

	if (y < 0)
	{
		y = -y;
	}

	if (z < 0)
	{
		z = -z;
	}

	// NOTE(aalhendi): Retail 0x800a4f38/0x800a3640 use the sign bit of these
	// extent checks ANDed together; negative means the source point is inside.
	insideBits = (u32)(x - 0x100) & (u32)(y - 0x180) & (u32)(z - 0x100);
	return (s32)insideBits < 0;
}

static int DrawLevelOvr1P_ShouldWriteRenderedClippedRecord(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count)
{
	for (int i = 0; i < count; i++)
	{
		if (DrawLevelOvr1P_SourceInsideClipRecordWindow(&projected[indices[i]]))
		{
			return 1;
		}
	}

	return 0;
}

static void DrawLevelOvr1P_CopyClipRecordVertex(struct DrawLevelOvr1PClipRecordVertex *dst, const struct DrawLevelOvr1PScratchVertex *src)
{
	dst->posVec = src->posVec;
	dst->flags = src->flags;
	DrawLevelOvr1P_CopyColorWord(dst->color_hi, src->color_hi);

	// NOTE(aalhendi): Retail 0x800a4f78/0x800a3680 OR scratch byte 0x13 into
	// the record color/code word; the 0x800aa790 consumer uses it for near masks.
	dst->color_hi[3] |= src->clipHalfNear;
}

static void DrawLevelOvr1P_StoreRenderedClipRecordHeader(u32 tableWord)
{
	// NOTE(aalhendi): Retail rendered helpers 0x800a84dc/0x800aa0fc store
	// clipped-record polarity at scratch 0x80 before helper-local dispatch.
	DrawLevelOvr1P_Scratch()->clipRecordHeader = (DrawLevelOvr1P_GetActiveDrawOrderLow() & 0x80000000u) | ((tableWord & 0x80000000u) >> 1);
}

static u32 DrawLevelOvr1P_GetRenderedClipRecordHeader(const struct QuadBlock *block, int count)
{
	u32 header = DrawLevelOvr1P_Scratch()->clipRecordHeader;

	(void)block;

	// NOTE(aalhendi): Retail 0x800a4dcc/0x800a4e70 ORs the GT4 count bit into
	// the stored scratch 0x80 polarity word when writing clipped records.
	if (count == 4)
	{
		header |= 1;
	}

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

	(void)texture;

	if (!DrawLevelOvr1P_ShouldWriteRenderedClippedRecord(projected, indices, count))
	{
		return 1;
	}

	if (!DrawLevelOvr1P_HasClipRecordSpace(recordSize))
	{
		return 1;
	}

	maxDepth = DrawLevelOvr1P_GetPreparedProjectedMaxDepthCount(projected, indices, count);
	otIndex = DrawLevelOvr1P_ResolveProjectedOtIndex(block, projected, maxDepth, faceIndex, otIndexOverride);

	record = (struct DrawLevelOvr1PClipRecord *)cursor;
	record->header = DrawLevelOvr1P_GetRenderedClipRecordHeader(block, count);
	record->otEntry = (u32)(uintptr_t)&pb->ptrOT[otIndex];
	// NOTE(aalhendi): Retail terminal near writers 0x800a89dc/0x800aa5fc
	// store the freshly selected scratch UV metadata, not the caller texture.
	record->tpage = DrawLevelOvr1P_Scratch()->uv.tpage;
	record->clut = DrawLevelOvr1P_Scratch()->uv.clut;

	for (int i = 0; i < count; i++)
	{
		DrawLevelOvr1P_CopyClipRecordVertex(&record->vertex[i], &projected[indices[i]]);
	}

	DrawLevelOvr1P_SetClipRecordCursor(cursor + recordSize);
	return 1;
}

static int DrawLevelOvr1P_WriteRenderedClippedRecord(struct PushBuffer *pb, const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected,
                                                     const int *indices, int count, int faceIndex, const struct TextureLayout *texture)
{
	return DrawLevelOvr1P_WriteRenderedClippedRecordAtOt(pb, block, projected, indices, count, faceIndex, texture, DRAW_LEVEL_OVR1P_OT_INDEX_AUTO);
}

static int DrawLevelOvr1P_WriteWaterRenderedClippedRecordAtOt(struct PushBuffer *pb, const struct QuadBlock *block,
                                                              const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count, int faceIndex,
                                                              int otIndexOverride)
{
	struct DrawLevelOvr1PClipRecord *record;
	u8 *cursor = DrawLevelOvr1P_GetClipRecordCursor();
	size_t recordSize = DrawLevelOvr1P_GetClipRecordSize(count);
	u32 maxDepth;
	int otIndex;

	if (!DrawLevelOvr1P_ShouldWriteRenderedClippedRecord(projected, indices, count))
	{
		return 1;
	}

	if (!DrawLevelOvr1P_HasClipRecordSpace(recordSize))
	{
		return 1;
	}

	maxDepth = DrawLevelOvr1P_GetPreparedProjectedMaxDepthCount(projected, indices, count);
	otIndex = DrawLevelOvr1P_ResolveProjectedOtIndex(block, projected, maxDepth, faceIndex, otIndexOverride);

	record = (struct DrawLevelOvr1PClipRecord *)cursor;
	// NOTE(aalhendi): Retail water direct helpers 0x800a34d4/0x800a3578 set
	// bit 31 on clipped-record headers so the consumer keeps the NCLIP result.
	record->header = count == 4 ? 0x80000001u : 0x80000000u;
	record->otEntry = (u32)(uintptr_t)&pb->ptrOT[otIndex];
	record->tpage = DrawLevelOvr1P_Scratch()->uv.tpage;
	record->clut = DrawLevelOvr1P_Scratch()->uv.clut;

	for (int i = 0; i < count; i++)
	{
		DrawLevelOvr1P_CopyClipRecordVertex(&record->vertex[i], &projected[indices[i]]);
	}

	DrawLevelOvr1P_SetClipRecordCursor(cursor + recordSize);
	return 1;
}

static int Ovr226_800a34d4_WriteWaterRenderedClippedRecordAtOtEntry(struct PushBuffer *pb, const struct DrawLevelOvr1PScratchVertex *projected,
                                                                    const int *indices, int count, uint32_t *otEntry)
{
	struct DrawLevelOvr1PClipRecord *record;
	u8 *cursor = DrawLevelOvr1P_GetClipRecordCursor();
	size_t recordSize = DrawLevelOvr1P_GetClipRecordSize(count);

	(void)pb;

	if (!DrawLevelOvr1P_ShouldWriteRenderedClippedRecord(projected, indices, count))
	{
		return 1;
	}

	record = (struct DrawLevelOvr1PClipRecord *)cursor;
	// NOTE(aalhendi): Retail water-rendered clipped-record writers
	// 0x800a34d4/0x800a3578 store the inherited GP/OT pointer directly.
	record->header = count == 4 ? 0x80000001u : 0x80000000u;
	record->otEntry = (u32)(uintptr_t)otEntry;
	record->tpage = DrawLevelOvr1P_Scratch()->uv.tpage;
	record->clut = DrawLevelOvr1P_Scratch()->uv.clut;

	for (int i = 0; i < count; i++)
	{
		DrawLevelOvr1P_CopyClipRecordVertex(&record->vertex[i], &projected[indices[i]]);
	}

	DrawLevelOvr1P_SetClipRecordCursor(cursor + recordSize);
	return 1;
}

static int DrawLevelOvr1P_WriteWaterRenderedClippedRecord(struct PushBuffer *pb, const struct QuadBlock *block,
                                                          const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count, int faceIndex)
{
	return DrawLevelOvr1P_WriteWaterRenderedClippedRecordAtOt(pb, block, projected, indices, count, faceIndex, DRAW_LEVEL_OVR1P_OT_INDEX_AUTO);
}

static int DrawLevelOvr1P_EmitRenderedClippedRecordAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                        const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count, int faceIndex,
                                                        const struct TextureLayout *texture, int otIndexOverride)
{
	(void)primMem;

	// NOTE(aalhendi): Retail 0x800a4dcc/0x800a4e70 writes this clipped-record
	// shape after the local source-window probe; 0x800aa790 consumes it later.
	return DrawLevelOvr1P_WriteRenderedClippedRecordAtOt(pb, block, projected, indices, count, faceIndex, texture, otIndexOverride);
}

static int DrawLevelOvr1P_EmitWaterRenderedClippedRecordAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count, int faceIndex,
                                                             int otIndexOverride)
{
	(void)primMem;

	return DrawLevelOvr1P_WriteWaterRenderedClippedRecordAtOt(pb, block, projected, indices, count, faceIndex, otIndexOverride);
}

static int DrawLevelOvr1P_ShouldEmitClipRecordNclip(s32 nclip, const struct DrawLevelOvr1PClipRecord *record)
{
	s32 header = (s32)record->header;

	if (nclip == 0)
	{
		return 0;
	}

	if (header < 0)
	{
		return 1;
	}

	return (s32)((u32)nclip ^ ((u32)header << 1)) > 0;
}

static u32 DrawLevelOvr1P_GetClipRecordColorCode(const struct DrawLevelOvr1PScratchVertex *projected, u32 code)
{
	return (DrawLevelOvr1P_ReadPackedWord(projected->color_hi) & 0x00ffffff) | (code << 24);
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
	DrawLevelOvr1P_Scratch()->uv.tpage = record->tpage;
	DrawLevelOvr1P_Scratch()->uv.clut = record->clut;
}

static struct DrawLevelOvr1PScratchVertex *DrawLevelOvr1P_GetClipRecordWorkspace(void)
{
	return CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PScratchVertex, DRAW_LEVEL_OVR1P_PROJECTED_FRAME0_OFFSET);
}

static u32 DrawLevelOvr1P_StoreClipRecordUvScratch(const struct DrawLevelOvr1PScratchVertex *projected, enum DrawLevelOvr1PUvScratchSlot slot)
{
	struct DrawLevelOvr1PUvScratch *scratch = &DrawLevelOvr1P_Scratch()->uv;

	switch (slot)
	{
	case DRAW_LEVEL_OVR1P_UV_SCRATCH_SLOT_0:
		scratch->flag0 = (s16)projected->flags;
		return scratch->uv0;

	case DRAW_LEVEL_OVR1P_UV_SCRATCH_SLOT_1:
		scratch->flag1 = (s16)projected->flags;
		return scratch->uv1;

	case DRAW_LEVEL_OVR1P_UV_SCRATCH_SLOT_2:
	default:
		scratch->flag2 = (s16)projected->flags;
		return scratch->uv2;
	}
}

static void DrawLevelOvr1P_WriteClipRecordGT3(POLY_GT3 *poly, const struct DrawLevelOvr1PScratchVertex *emit, u32 code, u32 uv0, u32 uv1, u32 uv2)
{
	CtrGpu_WriteColorCode(&poly->r0, DrawLevelOvr1P_GetClipRecordColorCode(&emit[0], code));
	CtrGpu_WritePackedXY(&poly->x0, DrawLevelOvr1P_PackProjectedSxy(&emit[0]));
	CtrGpu_WritePackedUVWord(&poly->u0, uv0);
	CtrGpu_WriteColorCode(&poly->r1, DrawLevelOvr1P_GetClipRecordColorCode(&emit[1], 0));
	CtrGpu_WritePackedXY(&poly->x1, DrawLevelOvr1P_PackProjectedSxy(&emit[1]));
	CtrGpu_WritePackedUVWord(&poly->u1, uv1);
	CtrGpu_WriteColorCode(&poly->r2, DrawLevelOvr1P_GetClipRecordColorCode(&emit[2], 0));
	CtrGpu_WritePackedXY(&poly->x2, DrawLevelOvr1P_PackProjectedSxy(&emit[2]));
	CtrGpu_WritePackedUVWord(&poly->u2, uv2);
}

static void DrawLevelOvr1P_WriteClipRecordGT4(POLY_GT4 *poly, const struct DrawLevelOvr1PScratchVertex *emit, u32 code, u32 uv0, u32 uv1, u32 uv2)
{
	DrawLevelOvr1P_WriteClipRecordGT3((POLY_GT3 *)poly, emit, code, uv0, uv1, uv2);
	CtrGpu_WriteColorCode(&poly->r3, DrawLevelOvr1P_GetClipRecordColorCode(&emit[3], 0));
	CtrGpu_WritePackedXY(&poly->x3, DrawLevelOvr1P_PackProjectedSxy(&emit[3]));
	CtrGpu_WritePackedUVWord(&poly->u3, DrawLevelOvr1P_GetClipRecordSignedUvWord(&emit[3]));
}

static void DrawLevelOvr1P_SetClipRecordSourceDelta(struct DrawLevelOvr1PScratchVertex *projected, s16 delta)
{
	DrawLevelOvr1P_WritePackedHalf(&projected->clipNear, (u16)delta);
}

static s16 DrawLevelOvr1P_GetClipRecordSourceDelta(const struct DrawLevelOvr1PScratchVertex *projected)
{
	return (s16)DrawLevelOvr1P_ReadPackedHalf(&projected->clipNear);
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

	CTR_GteLoadSV3(&source[0], &source[1], &source[2]);
	gte_rtpt();
	CTR_GteStoreSXY3(&dst[0].posScreen[0], &dst[1].posScreen[0], &dst[2].posScreen[0]);
	gte_nclip();
	gte_stopz(&nclip);

	if (count == 4)
	{
		CTR_GteLoadSV0(&source[3]);
		gte_rtps();
		CTR_GteStoreSXY(&dst[3].posScreen[0]);
	}

	return nclip;
}

static void DrawLevelOvr1P_ProjectClipRecordEmitVertex(struct DrawLevelOvr1PScratchVertex *dst, const struct DrawLevelOvr1PScratchVertex *projected)
{
	SVECTOR source;

	*dst = *projected;
	DrawLevelOvr1P_GetClipRecordSourceVector(projected, &source);

	CTR_GteLoadSV0(&source);
	gte_rtps();
	CTR_GteStoreSXY(&dst->posScreen[0]);
}

static int Ovr226_800aac00_EmitClipRecordGT3(struct PushBuffer *pb, struct PrimMem *primMem, uint32_t *otEntry,
                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                             const struct DrawLevelOvr1PClipRecord *record);

static int Ovr226_800aad44_EmitClipRecordGT4(struct PushBuffer *pb, struct PrimMem *primMem, uint32_t *otEntry,
                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                             const struct DrawLevelOvr1PClipRecord *record)
{
	struct DrawLevelOvr1PScratchVertex emit[4];
	int emitIndices[4] = {0, 1, 2, 3};
	POLY_GT4 *prim;
	POLY_GT4 *nextPrim;
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
	{
		return Ovr226_800aac00_EmitClipRecordGT3(pb, primMem, otEntry, projected, fallbackIndices, record);
	}

	DrawLevelOvr1P_ProjectClipRecordEmitVertex(&emit[3], &projected[indices[3]]);

	if (DrawLevelOvr1P_IsProjectedFaceOffscreen(pb, emit, emitIndices))
	{
		return 1;
	}

	// NOTE(aalhendi): Retail relies on the 0x800aa848 per-record 0xd68 prim reserve.
	prim = primMem->cursor;
	nextPrim = prim + 1;
	uv0 = DrawLevelOvr1P_StoreClipRecordUvScratch(&emit[0], DRAW_LEVEL_OVR1P_UV_SCRATCH_SLOT_0);
	uv1 = DrawLevelOvr1P_StoreClipRecordUvScratch(&emit[1], DRAW_LEVEL_OVR1P_UV_SCRATCH_SLOT_1);
	uv2 = DrawLevelOvr1P_StoreClipRecordUvScratch(&emit[2], DRAW_LEVEL_OVR1P_UV_SCRATCH_SLOT_2);
	code = DrawLevelOvr1P_SelectRawPrimitiveCode(uv1, 0x3e, 0x3c);

	DrawLevelOvr1P_WriteClipRecordGT4(prim, emit, code, uv0, uv1, uv2);
	DrawLevelOvr1P_AddRawPrimToOt(primMem, prim, 12, otEntry);
	primMem->cursor = nextPrim;
	return 1;
}

static int Ovr226_800aac00_EmitClipRecordGT3(struct PushBuffer *pb, struct PrimMem *primMem, uint32_t *otEntry,
                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                             const struct DrawLevelOvr1PClipRecord *record)
{
	struct DrawLevelOvr1PScratchVertex emit[3];
	int emitIndices[3] = {0, 1, 2};
	POLY_GT3 *prim;
	POLY_GT3 *nextPrim;
	s32 nclip;
	u32 code;
	u32 uv0;
	u32 uv1;
	u32 uv2;

	nclip = DrawLevelOvr1P_ProjectClipRecordEmitVertices(emit, projected, indices, 3);

	if (!DrawLevelOvr1P_ShouldEmitClipRecordNclip(nclip, record))
	{
		return 1;
	}

	if (DrawLevelOvr1P_IsProjectedTriOffscreen(pb, emit, emitIndices))
	{
		return 1;
	}

	// NOTE(aalhendi): Retail relies on the 0x800aa848 per-record 0xd68 prim reserve.
	prim = primMem->cursor;
	nextPrim = prim + 1;
	uv0 = DrawLevelOvr1P_StoreClipRecordUvScratch(&emit[0], DRAW_LEVEL_OVR1P_UV_SCRATCH_SLOT_0);
	uv1 = DrawLevelOvr1P_StoreClipRecordUvScratch(&emit[1], DRAW_LEVEL_OVR1P_UV_SCRATCH_SLOT_1);
	uv2 = DrawLevelOvr1P_StoreClipRecordUvScratch(&emit[2], DRAW_LEVEL_OVR1P_UV_SCRATCH_SLOT_2);
	code = DrawLevelOvr1P_SelectRawPrimitiveCode(uv1, 0x36, 0x34);

	DrawLevelOvr1P_WriteClipRecordGT3(prim, emit, code, uv0, uv1, uv2);
	DrawLevelOvr1P_AddRawPrimToOt(primMem, prim, 9, otEntry);
	primMem->cursor = nextPrim;
	return 1;
}

static void DrawLevelOvr1P_StoreCurrentIrVector(s16 *out)
{
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native reads PsyCross-backed GTE IR registers directly.
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

static void Ovr226_800aaad0_PrepareClipRecordDepthScratch(struct DrawLevelOvr1PScratchVertex *projected, s32 threshold)
{
	// NOTE(aalhendi): Retail 0x800aaad0..0x800aab00 doubles the transformed
	// IR vector with MIPS `sll 1` wrap semantics and stores sourceZ-threshold.
	projected->posScreen[0] = DrawLevelOvr1P_ShiftLeft1S16Wrap(projected->pos[0]);
	projected->posScreen[1] = DrawLevelOvr1P_ShiftLeft1S16Wrap(projected->pos[1]);
	projected->depth = (u16)DrawLevelOvr1P_ShiftLeft1S16Wrap(projected->pos[2]);
	DrawLevelOvr1P_SetClipRecordSourceDelta(projected, (s16)((s32)projected->pos[2] - threshold));
}

static void Ovr226_800aa858_ProjectClipRecordRawVertex(struct DrawLevelOvr1PScratchVertex *projected, const struct DrawLevelOvr1PClipRecordVertex *src)
{
	s16 ir[3];

	projected->posVec = src->posVec;
	projected->flags = src->flags;
	DrawLevelOvr1P_CopyColorWord(projected->color_hi, src->color_hi);

	CTR_GteLoadS16TripletV0(&projected->pos[0]);
	// NOTE(aalhendi): Retail 0x800aa86c/0x800aa8b0/0x800aa8e8/0x800aaee4
	// uses LLV0BK for this source-vector transform.
	gte_llv0bk_b();
	DrawLevelOvr1P_StoreCurrentIrVector(ir);

	projected->pos[0] = ir[0];
	projected->pos[1] = ir[1];
	projected->pos[2] = ir[2];
}

static void DrawLevelOvr1P_PrepareClipRecordDepthScratchRange(struct DrawLevelOvr1PScratchVertex *projected, int count)
{
	s32 threshold = DrawLevelOvr1P_GetDepthClipThreshold();

	for (int i = 0; i < count; i++)
	{
		Ovr226_800aaad0_PrepareClipRecordDepthScratch(&projected[i], threshold);
	}
}

static u32 DrawLevelOvr1P_GetClipRecordProjectedNearMask(const struct DrawLevelOvr1PScratchVertex *projected, int count)
{
	static const u32 bits[4] = {0x4, 0x8, 0x10, 0x20};
	u32 mask = 0;

	for (int i = 0; i < count; i++)
	{
		if (DrawLevelOvr1P_IsClipByteSet(projected[i].color_hi[3]))
		{
			mask |= bits[i];
		}
	}

	return mask;
}

static void DrawLevelOvr1P_ClearClipRecordProjectedNearBytes(struct DrawLevelOvr1PScratchVertex *projected, int count)
{
	for (int i = 0; i < count; i++)
	{
		projected[i].color_hi[3] = 0;
	}
}

static void Ovr226_800aab00_InterpolateClipRecordVertex(struct DrawLevelOvr1PScratchVertex *dst, const struct DrawLevelOvr1PScratchVertex *inside,
                                                        const struct DrawLevelOvr1PScratchVertex *outside)
{
	const u8 *insideUv = (const u8 *)&inside->flags;
	const u8 *outsideUv = (const u8 *)&outside->flags;
	u8 *dstUv = (u8 *)&dst->flags;
	s32 threshold = DrawLevelOvr1P_GetDepthClipThreshold();
	s32 denom = (s16)outside->depth - (s16)inside->depth;
	s32 factor = DrawLevelOvr1P_ShiftLeft16Wrap(threshold - (s32)(s16)inside->depth) / denom;

	// NOTE(aalhendi): Retail 0x800aab00..0x800aac00 writes only offsets
	// 0x00, 0x02, and bytes 0x06..0x0a. Offset 0x04 and source delta 0x12
	// are preseeded by 0x800aa7f8.
	dst->pos[0] = DrawLevelOvr1P_LerpS16_16(inside->posScreen[0], outside->posScreen[0], factor);
	dst->pos[1] = DrawLevelOvr1P_LerpS16_16(inside->posScreen[1], outside->posScreen[1], factor);
	dstUv[0] = DrawLevelOvr1P_LerpU8_16(insideUv[0], outsideUv[0], factor);
	dstUv[1] = DrawLevelOvr1P_LerpU8_16(insideUv[1], outsideUv[1], factor);

	for (int i = 0; i < 3; i++)
	{
		dst->color_hi[i] = DrawLevelOvr1P_LerpU8_16(inside->color_hi[i], outside->color_hi[i], factor);
	}
}

static u32 DrawLevelOvr1P_GetClipRecordJumpAddress(int count, u32 nearMask)
{
	u32 tableOffset = count == 4 ? DRAW_LEVEL_OVR1P_GT4_CLIP_RECORD_JUMP_TABLE_OFFSET : DRAW_LEVEL_OVR1P_GT3_CLIP_RECORD_JUMP_TABLE_OFFSET;

	return *CTR_SCRATCHPAD_PTR(u32, tableOffset + (int)nearMask);
}

static int Ovr226_800aa96c_DispatchGT3ClipRecordLabel(struct PushBuffer *pb, struct PrimMem *primMem, uint32_t *otEntry,
                                                      struct DrawLevelOvr1PScratchVertex *work, const struct DrawLevelOvr1PClipRecord *record,
                                                      u32 handlerAddress)
{
	int indices[4];

	// NOTE(aalhendi): Retail 0x800aa96c..0x800aaad0 jump labels branch back to
	// 0x800aa848 after advancing the clipped-record cursor by 0x30.
	//  Work records live at retail scratch 0x1b4/0x1c8/0x1dc/0x204/0x218.
	switch (handlerAddress)
	{
	case 0x800aa96c:
	{
		int tri[3] = {0, 1, 2};
		return Ovr226_800aac00_EmitClipRecordGT3(pb, primMem, otEntry, work, tri, record);
	}
	case 0x800aa984:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[1], &work[0]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[2], &work[0]);
		indices[0] = 1;
		indices[1] = 2;
		indices[2] = 4;
		indices[3] = 5;
		return Ovr226_800aad44_EmitClipRecordGT4(pb, primMem, otEntry, work, indices, record);
	case 0x800aa9bc:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[2], &work[1]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[0], &work[1]);
		indices[0] = 2;
		indices[1] = 0;
		indices[2] = 4;
		indices[3] = 5;
		return Ovr226_800aad44_EmitClipRecordGT4(pb, primMem, otEntry, work, indices, record);
	case 0x800aa9f4:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[2], &work[1]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[2], &work[0]);
		indices[0] = 2;
		indices[1] = 5;
		indices[2] = 4;
		return Ovr226_800aac00_EmitClipRecordGT3(pb, primMem, otEntry, work, indices, record);
	case 0x800aaa28:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[0], &work[2]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[1], &work[2]);
		indices[0] = 0;
		indices[1] = 1;
		indices[2] = 4;
		indices[3] = 5;
		return Ovr226_800aad44_EmitClipRecordGT4(pb, primMem, otEntry, work, indices, record);
	case 0x800aaa60:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[1], &work[0]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[1], &work[2]);
		indices[0] = 1;
		indices[1] = 5;
		indices[2] = 4;
		return Ovr226_800aac00_EmitClipRecordGT3(pb, primMem, otEntry, work, indices, record);
	case 0x800aaa94:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[0], &work[2]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[0], &work[1]);
		indices[0] = 0;
		indices[1] = 5;
		indices[2] = 4;
		return Ovr226_800aac00_EmitClipRecordGT3(pb, primMem, otEntry, work, indices, record);
	case 0x800aaac8:
		return 1;
	default:
		return 1;
	}
}

static int DrawLevelOvr1P_EmitClipRecordGT3Table(struct PushBuffer *pb, struct PrimMem *primMem, uint32_t *otEntry,
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
	}

	// NOTE(aalhendi): Retail 0x800aa934..0x800aa968 reads the projected
	// scratch near bytes to select the GT3 table, then clears those bytes.
	nearMask = DrawLevelOvr1P_GetClipRecordProjectedNearMask(work, 3);
	DrawLevelOvr1P_ClearClipRecordProjectedNearBytes(work, 3);
	handlerAddress = DrawLevelOvr1P_GetClipRecordJumpAddress(3, nearMask);

	return Ovr226_800aa96c_DispatchGT3ClipRecordLabel(pb, primMem, otEntry, work, record, handlerAddress);
}

static int DrawLevelOvr1P_EmitClipRecordTableTri(struct PushBuffer *pb, struct PrimMem *primMem, uint32_t *otEntry,
                                                 const struct DrawLevelOvr1PScratchVertex *work, const struct DrawLevelOvr1PClipRecord *record, int a, int b,
                                                 int c)
{
	int indices[3] = {a, b, c};

	return Ovr226_800aac00_EmitClipRecordGT3(pb, primMem, otEntry, work, indices, record);
}

static int DrawLevelOvr1P_EmitClipRecordTableQuad(struct PushBuffer *pb, struct PrimMem *primMem, uint32_t *otEntry,
                                                  const struct DrawLevelOvr1PScratchVertex *work, const struct DrawLevelOvr1PClipRecord *record, int a, int b,
                                                  int c, int d)
{
	int indices[4] = {a, b, c, d};

	return Ovr226_800aad44_EmitClipRecordGT4(pb, primMem, otEntry, work, indices, record);
}

static int Ovr226_800aaf70_DispatchGT4ClipRecordLabel(struct PushBuffer *pb, struct PrimMem *primMem, uint32_t *otEntry,
                                                      struct DrawLevelOvr1PScratchVertex *work, const struct DrawLevelOvr1PClipRecord *record,
                                                      u32 handlerAddress)
{
	// NOTE(aalhendi): Retail 0x800aaf70..0x800ab3d4 jump labels branch back to
	// 0x800aa848 after advancing the clipped-record cursor by 0x3c.
	//  Work records live at retail scratch 0x1b4..0x22c.
	switch (handlerAddress)
	{
	case 0x800aaf70:
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 0, 1, 2, 3);
	case 0x800aaf8c:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[1], &work[0]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[2], &work[0]);
		if (!DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 3, 2, 1))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 1, 2, 4, 5);
	case 0x800aafd4:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[3], &work[1]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[2], &work[1]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[6], &work[0], &work[1]);
		if (!DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 3, 2, 4, 5))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 2, 0, 5, 6);
	case 0x800ab02c:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[3], &work[1]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[2], &work[1]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[6], &work[2], &work[0]);
		if (!DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 3, 2, 4, 5))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 2, 6, 5);
	case 0x800ab080:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[0], &work[2]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[1], &work[2]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[6], &work[3], &work[2]);
		if (!DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 0, 1, 4, 5))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 1, 3, 5, 6);
	case 0x800ab0d8:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[1], &work[0]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[1], &work[2]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[6], &work[3], &work[2]);
		if (!DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 1, 5, 4))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 1, 3, 5, 6);
	case 0x800ab12c:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[0], &work[2]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[0], &work[1]);
		if (!DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 0, 5, 4))
		{
			return 0;
		}
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[3], &work[2]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[3], &work[1]);
		return DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 3, 5, 4);
	case 0x800ab18c:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[3], &work[1]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[3], &work[2]);
		return DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 3, 5, 4);
	case 0x800ab1c0:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[2], &work[3]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[1], &work[3]);
		if (!DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 0, 1, 2))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 2, 1, 4, 5);
	case 0x800ab208:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[1], &work[0]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[2], &work[0]);
		if (!DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 1, 2, 4, 5))
		{
			return 0;
		}
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[1], &work[3]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[2], &work[3]);
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 1, 2, 4, 5);
	case 0x800ab270:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[2], &work[3]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[2], &work[1]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[6], &work[0], &work[1]);
		if (!DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 2, 5, 4))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 2, 0, 5, 6);
	case 0x800ab2c4:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[2], &work[3]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[2], &work[1]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[6], &work[2], &work[0]);
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 4, 2, 5, 6);
	case 0x800ab308:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[0], &work[2]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[1], &work[2]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[6], &work[1], &work[3]);
		if (!DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 0, 1, 4, 5))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 1, 6, 5);
	case 0x800ab35c:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[1], &work[0]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[1], &work[2]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[6], &work[1], &work[3]);
		return DrawLevelOvr1P_EmitClipRecordTableQuad(pb, primMem, otEntry, work, record, 4, 1, 5, 6);
	case 0x800ab3a0:
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[4], &work[0], &work[2]);
		Ovr226_800aab00_InterpolateClipRecordVertex(&work[5], &work[0], &work[1]);
		return DrawLevelOvr1P_EmitClipRecordTableTri(pb, primMem, otEntry, work, record, 0, 5, 4);
	case 0x800ab3d4:
		return Ovr226_800ab3d4_EmptyGT4ClipRecordLabel();
	default:
		return 1;
	}
}

static int DrawLevelOvr1P_EmitClipRecordGT4Table(struct PushBuffer *pb, struct PrimMem *primMem, uint32_t *otEntry,
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

	return Ovr226_800aaf70_DispatchGT4ClipRecordLabel(pb, primMem, otEntry, work, record, handlerAddress);
}

static int Ovr226_800aaed4_ProjectFourthClipRecordAndDispatchGT4(struct PushBuffer *pb, struct PrimMem *primMem, struct DrawLevelOvr1PScratchVertex *projected,
                                                                 const struct DrawLevelOvr1PClipRecord *record)
{
	uint32_t *otEntry = (uint32_t *)(uintptr_t)record->otEntry;

	Ovr226_800aa858_ProjectClipRecordRawVertex(&projected[3], &record->vertex[3]);
	DrawLevelOvr1P_PrepareClipRecordDepthScratchRange(projected, 4);

	// NOTE(aalhendi): Retail 0x800aaf28..0x800aaf6c dispatches through scratch
	// 0x260; native keeps the handler bodies as C cases keyed by the copied
	// retail addresses.
	return DrawLevelOvr1P_EmitClipRecordGT4Table(pb, primMem, otEntry, projected, record);
}

static int DrawLevelOvr1P_HasClipRecordConsumerPrimReserve(const struct PrimMem *primMem);

static int Ovr226_800aa848_ProjectFirstThreeClipRecordsAndDispatch(struct PushBuffer *pb, struct PrimMem *primMem,
                                                                   const struct DrawLevelOvr1PClipRecord *record)
{
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetClipRecordWorkspace();
	u32 header = record->header;

	// NOTE(aalhendi): Retail 0x800aa848 preflights primMem->end against
	// curr+0xd68 before each clipped record.
	if (!DrawLevelOvr1P_HasClipRecordConsumerPrimReserve(primMem))
	{
		return 0;
	}

	DrawLevelOvr1P_SetClipRecordPageScratch(record);

	for (int i = 0; i < 3; i++)
	{
		Ovr226_800aa858_ProjectClipRecordRawVertex(&projected[i], &record->vertex[i]);
	}

	// NOTE(aalhendi): Retail delay-slot stores the clipped-record header at
	// scratch 0x7c before selecting the GT3/GT4 consumer path.
	DrawLevelOvr1P_Scratch()->drawOrderOrHeader = header;

	if ((header & 1) != 0)
	{
		return Ovr226_800aaed4_ProjectFourthClipRecordAndDispatchGT4(pb, primMem, projected, record);
	}

	DrawLevelOvr1P_PrepareClipRecordDepthScratchRange(projected, 3);
	// NOTE(aalhendi): Retail 0x800aa934..0x800aa968 dispatches through scratch
	// 0x240; native keeps the handler bodies as C cases keyed by copied addresses.
	return DrawLevelOvr1P_EmitClipRecordGT3Table(pb, primMem, (uint32_t *)(uintptr_t)record->otEntry, projected, record);
}

static int Ovr226_800aa790_TerminalPreamble(struct PushBuffer *pb, const u8 *cursor, const u8 *end)
{
	s16 threshold = (s16)(((u32)pb->distanceToScreen_PREV >> 1) + 1);

	// NOTE(aalhendi): Retail saves the terminal call return address at scratch
	// 0x2a0 before the empty-queue branch. Native models the scratch side
	// effect with the retail return PC; the host ABI owns the real return.
	*DrawLevelOvr1P_TerminalReturnPcScratch() = 0x800a0eb4;

	if (cursor == end)
	{
		return 0;
	}

	for (int i = 0; i < 8; i++)
	{
		CTC2(DrawLevelOvr1P_ReadWord(&pb->matrix_ViewProj, i * 4), 8 + i);
	}

	CTC2((u32)(s32)pb->rect.w << 15, 24);
	CTC2((u32)(s32)pb->rect.h << 15, 25);
	CTC2((u32)pb->distanceToScreen_PREV, 26);

	// NOTE(aalhendi): Retail 0x800aa7f8 mirrors the clip threshold into the
	// temporary vertices used by the 0x800aab00 interpolation helper.
	for (int i = 0; i < 3; i++)
	{
		struct DrawLevelOvr1PScratchVertex *vertex = DrawLevelOvr1P_TerminalClipVertex(i);

		vertex->pos[2] = threshold;
		vertex->clipNear = 0;
	}
	DrawLevelOvr1P_Scratch()->clipWindowPacked = DrawLevelOvr1P_ReadWord(&pb->rect, 4);

	CTC2(0x1000, 0);
	CTC2(0, 1);
	CTC2(0x1000, 2);
	CTC2(0, 3);
	CTC2(0x1000, 4);
	CTC2(0, 5);
	CTC2(0, 6);
	CTC2(0, 7);

	return 1;
}

static int DrawLevelOvr1P_HasClipRecordConsumerPrimReserve(const struct PrimMem *primMem)
{
	return (u8 *)primMem->cursor + DRAW_LEVEL_OVR1P_CLIP_RECORD_PRIM_RESERVE + sDrawLevelOvr1P_PrimReserveBias <= (u8 *)primMem->end;
}

static int DrawLevelOvr1P_HasBucketPrimReserve(const struct PrimMem *primMem, u32 reserve)
{
	u8 *curr = primMem->cursor;
	u8 *end = primMem->end;

	return curr <= end && (size_t)(reserve + sDrawLevelOvr1P_PrimReserveBias) <= (size_t)(end - curr);
}

static void DrawLevelOvr1P_SetPrimReserveBias(u32 bias)
{
	sDrawLevelOvr1P_PrimReserveBias = bias;
}

static void DrawLevelOvr1P_SetListHandlersSeedRenderedCursor(int enabled)
{
	sDrawLevelOvr1P_ListHandlersSeedRenderedCursor = enabled;
}

static int DrawLevelOvr1P_ConsumeClipRecords(struct PushBuffer *pb, struct PrimMem *primMem)
{
	u8 *start = DrawLevelOvr1P_GetClipRecordStart();
	u8 *cursor = start;
	u8 *end = DrawLevelOvr1P_GetClipRecordCursor();
	u8 *bufferEnd = DrawLevelOvr1P_GetClipRecordEnd();

	if (start == NULL || end == NULL || bufferEnd == NULL)
	{
		return 1;
	}

	if (end < start || end > bufferEnd)
	{
		DrawLevelOvr1P_SetClipRecordCursor(start);
		return 1;
	}

	if (!Ovr226_800aa790_TerminalPreamble(pb, cursor, end))
	{
		return 1;
	}

	while (cursor < end)
	{
		struct DrawLevelOvr1PClipRecord *record = (struct DrawLevelOvr1PClipRecord *)cursor;
		int count = (record->header & 1) != 0 ? 4 : 3;
		size_t recordSize = DrawLevelOvr1P_GetClipRecordSize(count);

		if (recordSize > (size_t)(end - cursor))
		{
			break;
		}

		if (!Ovr226_800aa848_ProjectFirstThreeClipRecordsAndDispatch(pb, primMem, record))
		{
			return 0;
		}

		cursor += recordSize;
	}

	DrawLevelOvr1P_SetClipRecordCursor(start);
	return 1;
}

static int DrawLevelOvr1P_SelectDirectBit(s32 nclipResult, u32 tableWord, u32 drawOrderLow, int negateResult, u32 directBit)
{
	u32 resultBits;

	if (nclipResult == 0)
	{
		resultBits = 0;
	}
	else if (negateResult)
	{
		resultBits = (0u - (u32)nclipResult) ^ tableWord;
	}
	else
	{
		resultBits = (u32)nclipResult ^ tableWord;
	}

	if ((s32)(resultBits | drawOrderLow) < 0)
	{
		return directBit;
	}

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

	DrawLevelOvr1P_Scratch()->directMask = directMask;
	return directMask;
}

static int Ovr226_800a18c0_FullDynamicRecursiveGate(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, u32 allowedMask,
                                                    struct DrawLevelOvr1PFullDynamicRecursiveGate *gate)
{
	u32 tableWord = DrawLevelOvr1P_Scratch()->selected4x1TableWord;

	gate->directMask = 0;
	gate->forceDirect = 0;
	DrawLevelOvr1P_Scratch()->directMask = allowedMask;

	if (DrawLevelOvr1P_IsProjectedPolyOffscreenPacked(projected, indices, 4))
	{
		return 0;
	}

	gate->directMask = DrawLevelOvr1P_SelectDirectMask(NULL, projected, indices, tableWord, allowedMask);
	DrawLevelOvr1P_Scratch()->directMask = gate->directMask;
	if (gate->directMask == 0)
	{
		return 0;
	}

	gate->forceDirect = projected == CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PScratchVertex, DRAW_LEVEL_OVR1P_DEEPEST_PROJECTED_FRAME_OFFSET);

	return 1;
}

static u32 DrawLevelOvr1P_GetDirectHandlerAddress(u32 directMask)
{
	if (directMask == 0)
	{
		return 0;
	}

	return *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_DIRECT_HANDLER_TABLE_OFFSET + (int)directMask);
}

static int DrawLevelOvr1P_EmitPreparedProjectedTriDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                         const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect);
static int DrawLevelOvr1P_EmitPreparedProjectedTriDirectAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                             const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect,
                                                             int otIndexOverride);
static int DrawLevelOvr1P_EmitPreparedProjectedTriDirectCodeAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                                 const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect,
                                                                 int otIndexOverride, int primCodeOverride);
static int DrawLevelOvr1P_EmitSignedClipProjectedTriDirectAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                               const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect,
                                                               int otIndexOverride, int primCodeOverride);
static int DrawLevelOvr1P_EmitNonzeroClipProjectedQuadDirectAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                                 const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect,
                                                                 int otIndexOverride, int primCodeOverride);
static int DrawLevelOvr1P_EmitPreparedProjectedQuadDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                          const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                          const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect);
static int DrawLevelOvr1P_EmitPreparedProjectedQuadDirectAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                              const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                              const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect,
                                                              int otIndexOverride);
static int DrawLevelOvr1P_EmitPreparedProjectedQuadDirectCodeAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                  const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                                  const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect,
                                                                  int otIndexOverride, int primCodeOverride);
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

static int DrawLevelOvr1P_IsSignedClipTerminalTriHandlerAddress(u32 handlerAddress)
{
	switch (handlerAddress)
	{
	case 0x800a333c:
	case 0x800a3334:
	case 0x800a4c14:
	case 0x800a4c0c:
	case 0x800a6bb4:
	case 0x800a6bac:
	case 0x800a87d4:
	case 0x800a87cc:
	case 0x800aa3f4:
	case 0x800aa3ec:
		return 1;
	default:
		return 0;
	}
}

static int DrawLevelOvr1P_IsNonzeroClipTerminalQuadHandlerAddress(u32 handlerAddress)
{
	switch (handlerAddress)
	{
	case 0x800a33e0:
	case 0x800a4cc8:
	case 0x800a6c68:
	case 0x800a8888:
	case 0x800aa4a8:
		return 1;
	default:
		return 0;
	}
}

static int DrawLevelOvr1P_IsTargetGridTerminalHandlerAddress(u32 handlerAddress)
{
	switch (handlerAddress)
	{
	case 0x800a87d4:
	case 0x800a87cc:
	case 0x800a8888:
	case 0x800aa3f4:
	case 0x800aa3ec:
	case 0x800aa4a8:
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
	{
		return 1;
	}

	// NOTE(aalhendi): Retail list-bucket direct handlers outside the target
	// 0x800a87d4/0x800a8888 family are raw packet writers after the caller's
	// cull/direct-mask checks. The target family keeps local terminal clip
	// checks even though list-projected clip bytes normally make them inert.
	if (writeClipBytes == DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST && !waterRenderedDirect && DrawLevelOvr1P_IsRawListDirectHandlerAddress(handlerAddress) &&
	    !DrawLevelOvr1P_IsTargetGridTerminalHandlerAddress(handlerAddress))
	{
		return DrawLevelOvr1P_EmitPreparedProjectedDirectMaskRawAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, otIndexOverride);
	}

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
		int primCodeOverride = (handlerAddress == 0x800a27dc || handlerAddress == 0x800a333c) ? 0x36 : DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO;

		if (DrawLevelOvr1P_IsSignedClipTerminalTriHandlerAddress(handlerAddress))
		{
			return DrawLevelOvr1P_EmitSignedClipProjectedTriDirectAtOt(pb, primMem, block, projected, triIndices, faceIndex, texture, writeClipBytes,
			                                                           waterRenderedDirect, otIndexOverride, primCodeOverride);
		}

		return DrawLevelOvr1P_EmitPreparedProjectedTriDirectCodeAtOt(pb, primMem, block, projected, triIndices, faceIndex, texture, writeClipBytes,
		                                                             waterRenderedDirect, otIndexOverride, primCodeOverride);
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
		int primCodeOverride = (handlerAddress == 0x800a27d4 || handlerAddress == 0x800a3334) ? 0x36 : DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO;

		if (DrawLevelOvr1P_IsSignedClipTerminalTriHandlerAddress(handlerAddress))
		{
			return DrawLevelOvr1P_EmitSignedClipProjectedTriDirectAtOt(pb, primMem, block, projected, triIndices, faceIndex, texture, writeClipBytes,
			                                                           waterRenderedDirect, otIndexOverride, primCodeOverride);
		}

		return DrawLevelOvr1P_EmitPreparedProjectedTriDirectCodeAtOt(pb, primMem, block, projected, triIndices, faceIndex, texture, writeClipBytes,
		                                                             waterRenderedDirect, otIndexOverride, primCodeOverride);
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
	{
		int primCodeOverride = (handlerAddress == 0x800a2850 || handlerAddress == 0x800a33e0) ? 0x3e : DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO;

		if (DrawLevelOvr1P_IsNonzeroClipTerminalQuadHandlerAddress(handlerAddress))
		{
			return DrawLevelOvr1P_EmitNonzeroClipProjectedQuadDirectAtOt(pb, primMem, block, projected, indices, faceIndex, texture, writeClipBytes,
			                                                             waterRenderedDirect, otIndexOverride, primCodeOverride);
		}

		return DrawLevelOvr1P_EmitPreparedProjectedQuadDirectCodeAtOt(pb, primMem, block, projected, indices, faceIndex, texture, writeClipBytes,
		                                                              waterRenderedDirect, otIndexOverride, primCodeOverride);
	}

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
	{
		return 1;
	}

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

		return DrawLevelOvr1P_EmitPreparedProjectedTriRawCodeAtOt(pb, primMem, block, projected, triIndices, faceIndex, texture,
		                                                          DrawLevelOvr1P_GetProjectedTriMaxDepth(projected, triIndices), otIndexOverride,
		                                                          handlerAddress == 0x800a27dc ? 0x36 : DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
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

		return DrawLevelOvr1P_EmitPreparedProjectedTriRawCodeAtOt(pb, primMem, block, projected, triIndices, faceIndex, texture,
		                                                          DrawLevelOvr1P_GetProjectedTriMaxDepth(projected, triIndices), otIndexOverride,
		                                                          handlerAddress == 0x800a27d4 ? 0x36 : DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
	}

	case 0x800a1d6c:
	case 0x800a2850:
	case 0x800a40b8:
	case 0x800a5d98:
	case 0x800a7ae4:
	case 0x800a8888:
	case 0x800a9704:
		return DrawLevelOvr1P_EmitPreparedProjectedQuadRawCodeAtOt(pb, primMem, block, projected, indices, faceIndex, texture,
		                                                           DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices), otIndexOverride,
		                                                           handlerAddress == 0x800a2850 ? 0x3e : DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);

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
	return DrawLevelOvr1P_EmitPreparedProjectedTriDirectCodeAtOt(pb, primMem, block, projected, indices, faceIndex, texture, writeClipBytes,
	                                                             waterRenderedDirect, otIndexOverride, DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
}

static int DrawLevelOvr1P_EmitPreparedProjectedTriDirectCodeAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                                 const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect,
                                                                 int otIndexOverride, int primCodeOverride)
{
	(void)writeClipBytes;
	(void)waterRenderedDirect;

	// NOTE(aalhendi): Retail GT3 direct helpers use `bltz` on clip bytes that
	// the rendered/list projection paths store as 0/1, so GT3 stays direct here.
	// Terminal direct handlers write raw packets after the caller-side cull.

	return DrawLevelOvr1P_EmitPreparedProjectedTriRawCodeAtOt(pb, primMem, block, projected, indices, faceIndex, texture,
	                                                          DrawLevelOvr1P_GetProjectedTriMaxDepth(projected, indices), otIndexOverride, primCodeOverride);
}

static int DrawLevelOvr1P_AreProjectedVerticesHalfNearSigned(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count)
{
	int value = (s8)projected[indices[0]].clipHalfNear;

	for (int i = 1; i < count; i++)
	{
		value &= (s8)projected[indices[i]].clipHalfNear;
	}

	return value < 0;
}

static int DrawLevelOvr1P_HasProjectedVertexNearSigned(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count)
{
	int value = 0;

	for (int i = 0; i < count; i++)
	{
		value |= (s8)projected[indices[i]].clipNear;
	}

	return value < 0;
}

static int DrawLevelOvr1P_EmitSignedClipProjectedTriDirectAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                               const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect,
                                                               int otIndexOverride, int primCodeOverride)
{
	(void)writeClipBytes;

	// NOTE(aalhendi): Retail rendered/water GT3 terminals use signed `bltz`
	// gates; current projection bytes are 0/1, so these are normally inert.
	if (DrawLevelOvr1P_AreProjectedVerticesHalfNearSigned(projected, indices, 3))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNearSigned(projected, indices, 3))
	{
		if (waterRenderedDirect)
		{
			return DrawLevelOvr1P_EmitWaterRenderedClippedRecordAtOt(pb, primMem, block, projected, indices, 3, faceIndex, otIndexOverride);
		}

		return DrawLevelOvr1P_EmitRenderedClippedRecordAtOt(pb, primMem, block, projected, indices, 3, faceIndex, texture, otIndexOverride);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedTriRawCodeAtOt(pb, primMem, block, projected, indices, faceIndex, texture,
	                                                          DrawLevelOvr1P_GetProjectedTriMaxDepth(projected, indices), otIndexOverride, primCodeOverride);
}

static int DrawLevelOvr1P_EmitNonzeroClipProjectedQuadDirectAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                                 const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect,
                                                                 int otIndexOverride, int primCodeOverride)
{
	(void)writeClipBytes;

	// NOTE(aalhendi): Retail rendered/water GT4 terminals use `bnez` gates at
	// the terminal labels before raw packet or clipped-record emission.
	if (DrawLevelOvr1P_AreProjectedVerticesHalfNear(projected, indices, 4))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		if (waterRenderedDirect)
		{
			return DrawLevelOvr1P_EmitWaterRenderedClippedRecordAtOt(pb, primMem, block, projected, indices, 4, faceIndex, otIndexOverride);
		}

		return DrawLevelOvr1P_EmitRenderedClippedRecordAtOt(pb, primMem, block, projected, indices, 4, faceIndex, texture, otIndexOverride);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedQuadRawCodeAtOt(pb, primMem, block, projected, indices, faceIndex, texture,
	                                                           DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices), otIndexOverride, primCodeOverride);
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
	return DrawLevelOvr1P_EmitPreparedProjectedQuadDirectCodeAtOt(pb, primMem, block, projected, indices, faceIndex, texture, writeClipBytes,
	                                                              waterRenderedDirect, otIndexOverride, DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
}

static int DrawLevelOvr1P_EmitPreparedProjectedQuadDirectCodeAtOt(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                  const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                                  const struct TextureLayout *texture, int writeClipBytes, int waterRenderedDirect,
                                                                  int otIndexOverride, int primCodeOverride)
{
	// NOTE(aalhendi): Retail terminal GT4 helpers consume the current scratch
	// 0x80 clipped-record header; selector/rendered owners refresh it before
	// dispatch, while copied recursive helpers inherit it unchanged.
	if (writeClipBytes)
	{
		if (DrawLevelOvr1P_AreProjectedVerticesHalfNear(projected, indices, 4))
		{
			return 1;
		}

		if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
		{
			if (waterRenderedDirect)
			{
				return DrawLevelOvr1P_EmitWaterRenderedClippedRecordAtOt(pb, primMem, block, projected, indices, 4, faceIndex, otIndexOverride);
			}

			return DrawLevelOvr1P_EmitRenderedClippedRecordAtOt(pb, primMem, block, projected, indices, 4, faceIndex, texture, otIndexOverride);
		}
	}

	// NOTE(aalhendi): Retail GT4 terminal direct handlers have no second
	// screen-space cull after the caller-side direct-mask reduction.
	return DrawLevelOvr1P_EmitPreparedProjectedQuadRawCodeAtOt(pb, primMem, block, projected, indices, faceIndex, texture,
	                                                           DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices), otIndexOverride, primCodeOverride);
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
	{
		return writeClipBytes ? 0x800a4550 : 0x800a3a34;
	}

	return *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_NEAR_SUBDIVISION_HANDLER_TABLE_OFFSET + (int)(tableIndex * sizeof(u32)));
}

static int DrawLevelOvr1P_GetNearSubdivisionTableSlot(u32 nearMask)
{
	u32 tableIndex = (nearMask >> 2) - 1;

	if (tableIndex >= 15)
	{
		return 5;
	}

	return (int)tableIndex;
}

static void DrawLevelOvr1P_SetPreviousRecursiveHandler(u32 handlerAddress)
{
	// NOTE(aalhendi): Retail stores the active recursive handler at scratch
	// 0x9c immediately before jumping through the copied table at scratch 0x14c.
	DrawLevelOvr1P_Scratch()->previousDirectHandlerAddress = handlerAddress;
}

static struct DrawLevelOvr1PScratchVertex *DrawLevelOvr1P_GetSubdivisionFrame(int depth)
{
	return CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PScratchVertex,
	                          DRAW_LEVEL_OVR1P_PROJECTED_FRAME0_OFFSET + ((depth + 1) * DRAW_LEVEL_OVR1P_RECURSION_FRAME_SIZE));
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
	if (directHandlerAddress != 0)
	{
		DrawLevelOvr1P_PrepareDeepestMosaicUv(projected, indices, directHandlerAddress);
	}

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

	DrawLevelOvr1P_Scratch()->directMask = directMask;
	directHandlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);
	if (directHandlerAddress != 0)
	{
		DrawLevelOvr1P_PrepareDeepestMosaicUv(projected, indices, directHandlerAddress);
	}

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

	DrawLevelOvr1P_Scratch()->directMask = directMask;
	directHandlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);
	if (directHandlerAddress != 0)
	{
		DrawLevelOvr1P_PrepareDeepestMosaicUv(projected, indices, directHandlerAddress);
	}

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

static int Ovr226_800a78a8_800a861c_800a94c8_800aa23c_DispatchCopiedGridNear(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                                                             int faceIndex, const struct TextureLayout *texture, u32 nearMask, int depth,
                                                                             int writeClipBytes, u32 allowedMask, int inheritedOtIndex);
static int DrawLevelOvr1P_IsDeepestSubdivisionFrame(const struct DrawLevelOvr1PScratchVertex *projected);

static int Ovr226_800a8360_800a9f80_CopiedGridRenderedNearBranch(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                                 const struct TextureLayout *texture, int depth, int writeClipBytes, int inheritedOtIndex)
{
	const u32 directMask = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
	u32 nearMask;

	DrawLevelOvr1P_Scratch()->directMask = directMask;

	// NOTE(aalhendi): Retail 0x800a8360/0x800a9f80 enters the deepest
	// handler from frame 0x324 before selecting another near child.
	if (DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
	{
		return DrawLevelOvr1P_EmitDeepestProjectedDirectStoredMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes,
		                                                               inheritedOtIndex);
	}

	nearMask = DrawLevelOvr1P_GetProjectedCopiedGridNearMask(projected, indices);
	if (nearMask != 0)
	{
		return Ovr226_800a78a8_800a861c_800a94c8_800aa23c_DispatchCopiedGridNear(pb, primMem, block, projected, indices, faceIndex, texture, nearMask, depth,
		                                                                         writeClipBytes, directMask, inheritedOtIndex);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedDirectMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes, 0,
	                                                          inheritedOtIndex);
}

static int Ovr226_800a7588_800a91a8_CopiedGridDirectPreflight(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                              const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                              const struct TextureLayout *texture, int depth, int writeClipBytes, u32 allowedMask,
                                                              int inheritedOtIndex)
{
	u32 directMask;
	u32 nearMask;

	// NOTE(aalhendi): Retail 0x800a82a0/0x800a9ec0 stores caller t2 before
	// packed-SXY/offscreen and NCLIP reduce it to the surviving direct mask.
	// List/default 0x800a7588/0x800a91a8 use this same branch shape.
	DrawLevelOvr1P_Scratch()->directMask = allowedMask;

	if (DrawLevelOvr1P_IsProjectedFaceOffscreen(pb, projected, indices))
	{
		return 1;
	}

	// NOTE(aalhendi): Retail projected-grid helpers reduce scratch 0x70 before
	// near recursion, so child dispatch only sees the surviving direct cases.
	directMask = DrawLevelOvr1P_SelectAndStoreDirectMask(block, projected, indices, DrawLevelOvr1P_Scratch()->selected4x1TableWord, allowedMask);
	if (directMask == 0)
	{
		return 1;
	}

	// NOTE(aalhendi): The shared retail helper uses the deepest-frame direct
	// path for frame 0x324 even when no further near child is selected.
	if (DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
	{
		return DrawLevelOvr1P_EmitDeepestProjectedDirectStoredMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes,
		                                                               inheritedOtIndex);
	}

	nearMask = DrawLevelOvr1P_GetProjectedCopiedGridNearMask(projected, indices);
	if (nearMask != 0)
	{
		return Ovr226_800a78a8_800a861c_800a94c8_800aa23c_DispatchCopiedGridNear(pb, primMem, block, projected, indices, faceIndex, texture, nearMask, depth,
		                                                                         writeClipBytes, directMask, inheritedOtIndex);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedDirectMaskAtOt(pb, primMem, block, projected, indices, faceIndex, texture, directMask, writeClipBytes, 0,
	                                                          inheritedOtIndex);
}

static int Ovr226_800a7588_800a825c_800a91a8_800a9e7c_CopiedGridFace(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                     const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                                     const struct TextureLayout *texture, int depth, int writeClipBytes, u32 allowedMask,
                                                                     int inheritedOtIndex)
{
	// NOTE(aalhendi): The branch-level retail split is modeled here:
	// 0x800a7588/0x800a91a8 skip clip bytes, while 0x800a825c/0x800a9e7c
	// test bytes 0x13/0x12 before the shared direct/near path.
	// Refreshed objdump 2026-06-01 checked the deepest branch handoff and the
	// non-deepest near continuations at 0x800a78a8/0x800a861c/0x800a94c8/0x800aa23c.
	// NOTE(aalhendi): Copied recursive/default grid helpers preserve inherited
	// UV/texture state. Texture-slot selection belongs to the selector entries
	// before this path, such as retail 0x800a7668, 0x800a8380, and 0x800a9fa0.
	if (writeClipBytes && DrawLevelOvr1P_IsProjectedFaceFullyNear(projected, indices))
	{
		return 1;
	}

	// NOTE(aalhendi): Rendered helpers branch on clip bytes before direct-mask
	// reduction. The clipped-record header scratch word is inherited here.
	if (writeClipBytes && DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		return Ovr226_800a8360_800a9f80_CopiedGridRenderedNearBranch(pb, primMem, block, projected, indices, faceIndex, texture, depth, writeClipBytes,
		                                                             inheritedOtIndex);
	}

	return Ovr226_800a7588_800a91a8_CopiedGridDirectPreflight(pb, primMem, block, projected, indices, faceIndex, texture, depth, writeClipBytes, allowedMask,
	                                                          inheritedOtIndex);
}

static int DrawLevelOvr1P_EmitProjectedGridFace(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                const struct TextureLayout *texture, int depth, int writeClipBytes, u32 allowedMask, int inheritedOtIndex)
{
	return Ovr226_800a7588_800a825c_800a91a8_800a9e7c_CopiedGridFace(pb, primMem, block, projected, indices, faceIndex, texture, depth, writeClipBytes,
	                                                                 allowedMask, inheritedOtIndex);
}

static int DrawLevelOvr1P_IsDeepestSubdivisionFrame(const struct DrawLevelOvr1PScratchVertex *projected)
{
	return projected == CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PScratchVertex, DRAW_LEVEL_OVR1P_DEEPEST_PROJECTED_FRAME_OFFSET);
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

	(void)allowedMask;

	for (int i = 0; i < 2; i++)
	{
		int subIndices[4];

		for (int j = 0; j < 4; j++)
		{
			subIndices[j] = subdivisionCase->subIndices[i][j];
		}

		// NOTE(aalhendi): Retail deepest generic-grid handlers branch back to
		// the compact topology labels, including the special +0xb4 writes.
		if (subdivisionCase->slotWords[i] != DRAW_LEVEL_OVR1P_SLOT_WORD_PRESERVE)
		{
			DrawLevelOvr1P_SetGridFaceSlotWord(projected, subdivisionCase->slotWords[i]);
		}

		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, subIndices, faceIndex, texture, depth, writeClipBytes,
		                                          subdivisionCase->directMasks[i], inheritedOtIndex))
		{
			return 0;
		}
	}

	return 1;
}

static int DrawLevelOvr1P_DispatchProjectedGridHelper(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, int depth,
                                                      const struct TextureLayout *texture, u32 handlerAddress, int handlerSlot, int writeClipBytes,
                                                      u32 allowedMask, int inheritedOtIndex)
{
	int slot = handlerSlot;
	const u32 allowedQuad = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
	const u32 allowedTriPrimary = DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY;
	const u32 allowedTriSecondary = DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY;

	if (DrawLevelOvr1P_HandlerUsesDeepestCompactGrid(handlerAddress) && DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
	{
		return DrawLevelOvr1P_DispatchProjectedGridDeepestCompact(pb, primMem, block, projected, faceIndex, depth, texture, writeClipBytes, allowedMask, slot,
		                                                          inheritedOtIndex);
	}

	// NOTE(aalhendi): Non-4x1 ground helper tables share the 3x3 subdivision
	// topology used by water-rendered, but emit through ground direct handlers.
	// Retail child labels pass their own t2 immediates into the shared helper;
	// the parent's reduced scratch 0x70 is not intersected into those labels.
	switch (slot)
	{
	case 0:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[0], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[1], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, writeClipBytes,
		                                            allowedQuad, inheritedOtIndex);
	case 1:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, writeClipBytes,
		                                          allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                            writeClipBytes, allowedQuad, inheritedOtIndex);
	case 2:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, writeClipBytes,
		                                          allowedTriPrimary, inheritedOtIndex))
		{
			return 0;
		}
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, writeClipBytes,
		                                          allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, writeClipBytes,
		                                            allowedQuad, inheritedOtIndex);
	case 3:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, writeClipBytes,
		                                          allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                            writeClipBytes, allowedQuad, inheritedOtIndex);
	case 4:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, writeClipBytes,
		                                          allowedTriPrimary, inheritedOtIndex))
		{
			return 0;
		}
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, writeClipBytes,
		                                          allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, writeClipBytes,
		                                            allowedQuad, inheritedOtIndex);
	case 7:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[2], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[3], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth, writeClipBytes,
		                                            allowedQuad, inheritedOtIndex);
	case 9:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, writeClipBytes,
		                                          allowedTriSecondary, inheritedOtIndex))
		{
			return 0;
		}
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, writeClipBytes,
		                                          allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth, writeClipBytes,
		                                            allowedQuad, inheritedOtIndex);
	case 11:
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                          writeClipBytes, allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, writeClipBytes,
		                                          allowedTriSecondary, inheritedOtIndex))
		{
			return 0;
		}
		if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, writeClipBytes,
		                                          allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth, writeClipBytes,
		                                            allowedQuad, inheritedOtIndex);
	default:
		break;
	}

	DrawLevelOvr1P_SetGridFaceSlotWord(projected, DrawLevelOvr1P_GetDefaultGridFaceSlotWord(handlerAddress, 0));
	if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, writeClipBytes,
	                                          allowedQuad, inheritedOtIndex))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, DrawLevelOvr1P_GetDefaultGridFaceSlotWord(handlerAddress, 1));
	if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, writeClipBytes,
	                                          allowedQuad, inheritedOtIndex))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, DrawLevelOvr1P_GetDefaultGridFaceSlotWord(handlerAddress, 2));
	if (!DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, writeClipBytes,
	                                          allowedQuad, inheritedOtIndex))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, DrawLevelOvr1P_GetDefaultGridFaceSlotWord(handlerAddress, 3));
	return DrawLevelOvr1P_EmitProjectedGridFace(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth, writeClipBytes,
	                                            allowedQuad, inheritedOtIndex);
}

static int Ovr226_800a78a8_800a861c_800a94c8_800aa23c_DispatchCopiedGridNear(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                                                             int faceIndex, const struct TextureLayout *texture, u32 nearMask, int depth,
                                                                             int writeClipBytes, u32 allowedMask, int inheritedOtIndex)
{
	struct DrawLevelOvr1PScratchVertex *sub;
	u32 handlerAddress;
	int handlerSlot;

	// NOTE(aalhendi): Retail reaches these continuations only after the caller's
	// deepest-frame branch has failed, then builds the next frame and jumps
	// through the copied near-handler table at scratch 0x148 + nearMask.
	handlerAddress = DrawLevelOvr1P_GetNearSubdivisionHandlerAddress(nearMask, writeClipBytes);
	handlerSlot = DrawLevelOvr1P_GetNearSubdivisionTableSlot(nearMask);
	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	if (DrawLevelOvr1P_HandlerUses4x4GridFrame(handlerAddress))
	{
		DrawLevelOvr1P_BuildGridSubdivisionFrame4x4(sub, projected, indices, writeClipBytes);
	}
	else
	{
		DrawLevelOvr1P_BuildGridSubdivisionFrame(sub, projected, indices, writeClipBytes);
	}

	DrawLevelOvr1P_SetPreviousRecursiveHandler(handlerAddress);
	return DrawLevelOvr1P_DispatchProjectedGridHelper(pb, primMem, block, sub, faceIndex, depth + 1, texture, handlerAddress, handlerSlot, writeClipBytes,
	                                                  allowedMask, inheritedOtIndex);
}

static int Ovr226_800a3eb0_Ground4x1NearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                 const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset, int inheritedOtIndex);
static int Ovr226_800a3f74_DispatchGround4x1Deepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                    const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                    const struct TextureLayout *texture, int inheritedOtIndex);
static int Ovr226_800a402c_DispatchGround4x1DirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                       const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                       const struct TextureLayout *texture, int inheritedOtIndex);
static int Ovr226_800a46d0_Ground4x1RenderedPreflight(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                      const struct TextureLayout *texture, int depth, u32 allowedMask, uint32_t *inheritedOtEntry);
static int Ovr226_800a44e0_DispatchGround4x1RenderedHelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                   struct DrawLevelOvr1PScratchVertex *projected, int faceIndex,
                                                                   const struct TextureLayout *texture, int depth, u32 handlerAddress,
                                                                   uint32_t *inheritedOtEntry);
static int Ovr226_800a4ad0_Ground4x1RenderedNearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                         const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset,
                                                         uint32_t *inheritedOtEntry);
static int Ovr226_800a4b54_DispatchGround4x1RenderedDeepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                            const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                            const struct TextureLayout *texture, uint32_t *inheritedOtEntry);
static int Ovr226_800a4c0c_DispatchGround4x1RenderedDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                               const struct TextureLayout *texture, uint32_t *inheritedOtEntry);
static int DrawLevelOvr1P_AreProjectedVerticesHalfNearSigned(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count);
static int DrawLevelOvr1P_HasProjectedVertexNearSigned(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count);

static int Ovr226_800a3b90_SelectAndStoreGround4x1DirectMask(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, u32 allowedMask,
                                                             u32 *directMask)
{
	int primaryIndices[3] = {indices[0], indices[1], indices[2]};
	int secondaryIndices[3] = {indices[3], indices[1], indices[2]};
	u32 tableWord = DrawLevelOvr1P_Scratch()->selected4x1TableWord;
	u32 drawOrderLow = DrawLevelOvr1P_GetActiveDrawOrderLow();
	s32 primaryNclip;
	s32 secondaryNclip;

	// NOTE(aalhendi): Retail 0x800a3b90 leaves the caller mask in scratch 0x70
	// when the packed-SXY reject exits before final direct-mask storage.
	DrawLevelOvr1P_Scratch()->directMask = allowedMask;

	primaryNclip = DrawLevelOvr1P_NclipProjected(projected, primaryIndices);
	secondaryNclip = DrawLevelOvr1P_NclipProjected(projected, secondaryIndices);

	if (DrawLevelOvr1P_IsProjectedFaceOffscreen(NULL, projected, indices))
	{
		return 0;
	}

	*directMask = DrawLevelOvr1P_SelectDirectBit(primaryNclip, tableWord, drawOrderLow, 1, DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY);
	*directMask |= DrawLevelOvr1P_SelectDirectBit(secondaryNclip, tableWord, drawOrderLow, 0, DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY);
	*directMask &= allowedMask;
	DrawLevelOvr1P_Scratch()->directMask = *directMask;

	return *directMask != 0;
}

static int Ovr226_800a3b90_Ground4x1DirectPreflight(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                    const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                    const struct TextureLayout *texture, int depth, u32 allowedMask, int inheritedOtIndex)
{
	u32 directMask = 0;

	if (!Ovr226_800a3b90_SelectAndStoreGround4x1DirectMask(projected, indices, allowedMask, &directMask))
	{
		return 1;
	}

	if (DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
	{
		return Ovr226_800a3f74_DispatchGround4x1Deepest(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtIndex);
	}

	return Ovr226_800a3eb0_Ground4x1NearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, depth, 0x28, inheritedOtIndex);
}

static int Ovr226_800a39c4_DispatchGround4x1HelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                           struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, const struct TextureLayout *texture,
                                                           int depth, u32 handlerAddress, int inheritedOtIndex)
{
	const int writeClipBytes = DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST;

	switch (handlerAddress)
	{
	case 0x800a39c4:
	{
		const int firstIndices[4] = {0, 4, 2, 3};
		const int secondIndices[4] = {1, 3, 4, 2};

		if (!Ovr226_800a3b90_Ground4x1DirectPreflight(pb, primMem, block, projected, firstIndices, faceIndex, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                              inheritedOtIndex))
		{
			return 0;
		}

		return Ovr226_800a3b90_Ground4x1DirectPreflight(pb, primMem, block, projected, secondIndices, faceIndex, texture, depth,
		                                                DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY, inheritedOtIndex);
	}

	case 0x800a39fc:
	{
		const int firstIndices[4] = {1, 8, 0, 2};
		const int secondIndices[4] = {0, 1, 8, 3};

		if (!Ovr226_800a3b90_Ground4x1DirectPreflight(pb, primMem, block, projected, firstIndices, faceIndex, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                              inheritedOtIndex))
		{
			return 0;
		}

		return Ovr226_800a3b90_Ground4x1DirectPreflight(pb, primMem, block, projected, secondIndices, faceIndex, texture, depth,
		                                                DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY, inheritedOtIndex);
	}

	case 0x800a3a34:
	{
		const int firstIndices[4] = {0, 4, 2, 8};
		const int secondIndices[4] = {4, 1, 8, 3};

		DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x0);
		if (!Ovr226_800a3b90_Ground4x1DirectPreflight(pb, primMem, block, projected, firstIndices, faceIndex, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                              inheritedOtIndex))
		{
			return 0;
		}

		DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0xc);
		return Ovr226_800a3b90_Ground4x1DirectPreflight(pb, primMem, block, projected, secondIndices, faceIndex, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                inheritedOtIndex);
	}

	default:
		return 1;
	}
}

static int Ovr226_800a3c70_SelectAndStoreGround4x1SelectorDirectMask(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, u32 tableWord,
                                                                     u32 *directMask)
{
	int primaryIndices[3] = {indices[0], indices[1], indices[2]};
	int secondaryIndices[3] = {indices[3], indices[1], indices[2]};
	u32 drawOrderLow = DrawLevelOvr1P_GetActiveDrawOrderLow();
	s32 primaryNclip;
	s32 secondaryNclip;

	primaryNclip = DrawLevelOvr1P_NclipProjected(projected, primaryIndices);
	secondaryNclip = DrawLevelOvr1P_NclipProjected(projected, secondaryIndices);

	if (DrawLevelOvr1P_IsProjectedFaceOffscreen(NULL, projected, indices))
	{
		return 0;
	}

	*directMask = DrawLevelOvr1P_SelectDirectBit(primaryNclip, tableWord, drawOrderLow, 1, DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY);
	*directMask |= DrawLevelOvr1P_SelectDirectBit(secondaryNclip, tableWord, drawOrderLow, 0, DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY);
	DrawLevelOvr1P_Scratch()->directMask = *directMask;

	return *directMask != 0;
}

static int Ovr226_800a3d98_ResolveGround4x1SelectorOtIndex(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected, u32 maxDepth,
                                                           int faceIndex)
{
	u32 slotWord = DrawLevelOvr1P_GetProjectedOtSlotWord(projected, faceIndex);
	s8 drawOrder = DrawLevelOvr1P_ReadRetailQuadBlockByte(block, 0x18 + (slotWord >> 2));
	s32 otIndex = (s32)(maxDepth >> 6) + drawOrder;

	// NOTE(aalhendi): Retail 0x800a3df4 clamps only negative GP/OT offsets
	// back to the OT base. There is no upper clamp in this selector span.
	return otIndex < 0 ? 0 : otIndex;
}

static struct TextureLayout *Ovr226_800a3e00_SelectGround4x1SelectorTexture(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected,
                                                                            u32 maxDepth)
{
	struct TextureLayout *texture = DrawLevelOvr1P_ResolveProjectedMidTexture(block, projected);
	u32 mosaicWord;

	if (texture == NULL)
	{
		return NULL;
	}

	mosaicWord = DrawLevelOvr1P_ReadPackedWord((const u8 *)texture + 0x24);
	DrawLevelOvr1P_Scratch()->mosaicTextureWord = mosaicWord;

	if ((s32)maxDepth < DrawLevelOvr1P_RenderScratch()->textureLodDepthThreshold0)
	{
		texture++;
	}

	if ((s32)maxDepth < DrawLevelOvr1P_RenderScratch()->textureLodDepthThreshold1)
	{
		texture++;
	}

	if ((s32)maxDepth < DrawLevelOvr1P_RenderScratch()->topLevelNearDepthThreshold && !DrawLevelOvr1P_TreatAsRetailNegativeTextureWord(mosaicWord))
	{
		texture++;
	}

	return texture;
}

static void Ovr226_800a3e44_WriteGround4x1SelectorUv(struct DrawLevelOvr1PScratchVertex *projected, const int *indices, const struct TextureLayout *texture,
                                                     u32 tableWord)
{
	DrawLevelOvr1P_WriteProjectedUv(projected, indices, texture, tableWord);
}

static int Ovr226_800a3eb0_Ground4x1NearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                 const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset, int inheritedOtIndex)
{
	u32 nearMask = DrawLevelOvr1P_GetProjectedNearMaskAtScratchOffset(projected, indices, thresholdScratchOffset);
	struct DrawLevelOvr1PScratchVertex *sub;
	u32 handlerAddress;

	if (nearMask == 0)
	{
		return Ovr226_800a402c_DispatchGround4x1DirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtIndex);
	}

	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	Ovr226_800a3a78_BuildGround4x1ListSubdivisionFrame(sub, projected, indices);

	handlerAddress = *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_DIRECT_NEAR_HANDLER_TABLE_OFFSET + nearMask);
	DrawLevelOvr1P_SetPreviousRecursiveHandler(handlerAddress);

	return Ovr226_800a39c4_DispatchGround4x1HelperWrappers(pb, primMem, block, sub, faceIndex, texture, depth + 1, handlerAddress, inheritedOtIndex);
}

static void Ovr226_800a3f74_PrepareGround4x1DeepestUv(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	u32 mosaicBase;
	u32 sourceOffset;
	const u8 *source;
	u32 uv0;
	u32 uv1;
	struct DrawLevelOvr1PScratchVertex *mutableProjected;

	if (projected != CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PScratchVertex, DRAW_LEVEL_OVR1P_DEEPEST_PROJECTED_FRAME_OFFSET))
	{
		return;
	}

	mosaicBase = DrawLevelOvr1P_Scratch()->mosaicTextureWord;
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail dereferences scratch 0x84 directly. Native can
	// carry host-rebased level words here, so only follow pointer-shaped
	// values that belong to level texture data.
	if (mosaicBase == 0 || ((s32)mosaicBase < 0 && !DrawLevelOvr1P_IsNativeLevelTexturePointer(mosaicBase)))
	{
		DrawLevelOvr1P_RestoreProjectedUvScratch();
		return;
	}
#endif
	if ((s32)mosaicBase > 0 && !DrawLevelOvr1P_IsNativeLevelTexturePointer(mosaicBase))
	{
		DrawLevelOvr1P_RestoreProjectedUvScratch();
		return;
	}

	if (DrawLevelOvr1P_Scratch()->previousDirectHandlerAddress != 0x800a3a34)
	{
		DrawLevelOvr1P_RestoreProjectedUvScratch();
		return;
	}

	sourceOffset = *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_MOSAIC_SOURCE_INDEX_OFFSET) << 1;
	if ((s32)(DrawLevelOvr1P_Scratch()->selected4x1TableWord << 8) < 0)
	{
		sourceOffset += 0x30;
	}
	sourceOffset += *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_MOSAIC_SOURCE_BIAS_OFFSET);

	source = (const u8 *)(uintptr_t)(mosaicBase + sourceOffset);
	uv0 = DrawLevelOvr1P_ReadPackedWord(source + 0);
	uv1 = DrawLevelOvr1P_ReadPackedWord(source + 4);

	mutableProjected = (struct DrawLevelOvr1PScratchVertex *)projected;
	DrawLevelOvr1P_Scratch()->uv.uv0 = uv0;
	mutableProjected[indices[0]].flags = (u16)uv0;
	DrawLevelOvr1P_Scratch()->uv.uv1 = uv1;
	mutableProjected[indices[1]].flags = (u16)uv1;
	mutableProjected[indices[2]].flags = DrawLevelOvr1P_ReadPackedHalf(source + 8);
	mutableProjected[indices[3]].flags = DrawLevelOvr1P_ReadPackedHalf(source + 10);
}

static int Ovr226_800a3f74_DispatchGround4x1Deepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                    const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                    const struct TextureLayout *texture, int inheritedOtIndex)
{
	int result;

	Ovr226_800a3f74_PrepareGround4x1DeepestUv(projected, indices);
	result = Ovr226_800a402c_DispatchGround4x1DirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtIndex);

	DrawLevelOvr1P_RestoreProjectedUvScratch();
	return result;
}

static uint32_t *Ovr226_800a4158_ResolveGround4x1DirectOtEntry(struct PushBuffer *pb, int inheritedOtIndex)
{
	if (inheritedOtIndex < 0)
	{
		return NULL;
	}

	return &pb->ptrOT[inheritedOtIndex];
}

static int Ovr226_800a4034_EmitGround4x1GT3Raw(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, const struct TextureLayout *texture,
                                               int inheritedOtIndex, int secondary)
{
	int triIndices[3];
	uint32_t *otEntry = Ovr226_800a4158_ResolveGround4x1DirectOtEntry(pb, inheritedOtIndex);
	POLY_GT3 *prim;
	POLY_GT3 *nextPrim;
	u32 uv0;
	u32 uv1;
	u32 uv2;
	u32 code;

	(void)block;
	(void)texture;

	if (otEntry == NULL)
	{
		return 1;
	}

	if (secondary)
	{
		triIndices[0] = indices[1];
		triIndices[1] = indices[3];
		triIndices[2] = indices[2];
	}
	else
	{
		triIndices[0] = indices[0];
		triIndices[1] = indices[1];
		triIndices[2] = indices[2];
	}

	prim = primMem->cursor;
	nextPrim = prim + 1;

	DrawLevelOvr1P_StoreProjectedDirectUvScratch(projected, triIndices, 3);
	uv0 = DrawLevelOvr1P_Scratch()->uv.uv0;
	uv1 = DrawLevelOvr1P_Scratch()->uv.uv1;
	uv2 = DrawLevelOvr1P_Scratch()->uv.uv2;
	code = DrawLevelOvr1P_SelectRawPrimitiveCode(uv1, 0x36, 0x34);

	DrawLevelOvr1P_WriteProjectedGT3(prim, projected, triIndices, code, uv0, uv1, uv2);
	DrawLevelOvr1P_AddRawPrimToOt(primMem, prim, 9, otEntry);
	primMem->cursor = nextPrim;
	return 1;
}

static int Ovr226_800a40b8_EmitGround4x1GT4Raw(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, const struct TextureLayout *texture,
                                               int inheritedOtIndex)
{
	uint32_t *otEntry = Ovr226_800a4158_ResolveGround4x1DirectOtEntry(pb, inheritedOtIndex);
	POLY_GT4 *prim;
	POLY_GT4 *nextPrim;
	u32 uv0;
	u32 uv1;
	u32 uv2;
	u32 code;

	(void)block;
	(void)texture;

	if (otEntry == NULL)
	{
		return 1;
	}

	prim = primMem->cursor;
	nextPrim = prim + 1;

	DrawLevelOvr1P_StoreProjectedDirectUvScratch(projected, indices, 4);
	uv0 = DrawLevelOvr1P_Scratch()->uv.uv0;
	uv1 = DrawLevelOvr1P_Scratch()->uv.uv1;
	uv2 = DrawLevelOvr1P_Scratch()->uv.uv2;
	code = DrawLevelOvr1P_SelectRawPrimitiveCode(uv1, 0x3e, 0x3c);

	DrawLevelOvr1P_WriteProjectedGT4(prim, projected, indices, code, uv0, uv1, uv2);
	DrawLevelOvr1P_AddRawPrimToOt(primMem, prim, 12, otEntry);
	primMem->cursor = nextPrim;
	return 1;
}

static int Ovr226_800a402c_DispatchGround4x1DirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                       const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                       const struct TextureLayout *texture, int inheritedOtIndex)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	(void)faceIndex;

	switch (handlerAddress)
	{
	case 0x800a4034:
		return Ovr226_800a4034_EmitGround4x1GT3Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex, 0);
	case 0x800a402c:
		return Ovr226_800a4034_EmitGround4x1GT3Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex, 1);
	case 0x800a40b8:
		return Ovr226_800a40b8_EmitGround4x1GT4Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex);
	default:
		return 1;
	}
}

static int Ovr226_800a3c70_Ground4x1SelectorNearGate(struct PushBuffer *pb, struct PrimMem *primMem, struct DrawLevelOvr1PScratchVertex *projected,
                                                     struct QuadBlock *block, int faceIndex)
{
	int indices[4];
	u32 tableWord;
	u32 directMask = 0;
	u32 maxDepth;
	int inheritedOtIndex;
	struct TextureLayout *texture;

	tableWord = DrawLevelOvr1P_Select4x1ProjectedFace(projected, block, faceIndex, indices);
	if (!Ovr226_800a3c70_SelectAndStoreGround4x1SelectorDirectMask(projected, indices, tableWord, &directMask))
	{
		return 1;
	}

	maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices);
	inheritedOtIndex = Ovr226_800a3d98_ResolveGround4x1SelectorOtIndex(block, projected, maxDepth, faceIndex);
	texture = Ovr226_800a3e00_SelectGround4x1SelectorTexture(block, projected, maxDepth);
	if (texture != NULL)
	{
		Ovr226_800a3e44_WriteGround4x1SelectorUv(projected, indices, texture, tableWord);
	}

	return Ovr226_800a3eb0_Ground4x1NearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, 0, 0x24, inheritedOtIndex);
}

static uint32_t *Ovr226_800a4978_ResolveGround4x1RenderedOtEntry(struct PushBuffer *pb, const struct QuadBlock *block,
                                                                 const struct DrawLevelOvr1PScratchVertex *projected, u32 maxDepth, int faceIndex)
{
	int otIndex = Ovr226_800a3d98_ResolveGround4x1SelectorOtIndex(block, projected, maxDepth, faceIndex);

	return &pb->ptrOT[otIndex];
}

static struct TextureLayout *Ovr226_800a49e0_SelectGround4x1RenderedTexture(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected,
                                                                            u32 maxDepth)
{
	return Ovr226_800a3e00_SelectGround4x1SelectorTexture(block, projected, maxDepth);
}

static void Ovr226_800a4a28_WriteGround4x1RenderedUv(struct DrawLevelOvr1PScratchVertex *projected, const int *indices, const struct TextureLayout *texture,
                                                     u32 tableWord)
{
	DrawLevelOvr1P_WriteProjectedUv(projected, indices, texture, tableWord);
}

static void Ovr226_800a4950_StoreGround4x1RenderedClipHeader(u32 tableWord)
{
	DrawLevelOvr1P_StoreRenderedClipRecordHeader(tableWord);
}

static int Ovr226_800a4dcc_WriteGround4x1RenderedClippedRecordAtOtEntry(struct PushBuffer *pb, const struct QuadBlock *block,
                                                                        const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count,
                                                                        uint32_t *otEntry)
{
	struct DrawLevelOvr1PClipRecord *record;
	u8 *cursor = DrawLevelOvr1P_GetClipRecordCursor();
	size_t recordSize = DrawLevelOvr1P_GetClipRecordSize(count);

	(void)pb;

	if (otEntry == NULL)
	{
		return 1;
	}

	if (!DrawLevelOvr1P_ShouldWriteRenderedClippedRecord(projected, indices, count))
	{
		return 1;
	}

	record = (struct DrawLevelOvr1PClipRecord *)cursor;
	record->header = DrawLevelOvr1P_GetRenderedClipRecordHeader(block, count);
	record->otEntry = (u32)(uintptr_t)otEntry;
	record->tpage = DrawLevelOvr1P_Scratch()->uv.tpage;
	record->clut = DrawLevelOvr1P_Scratch()->uv.clut;

	for (int i = 0; i < count; i++)
	{
		DrawLevelOvr1P_CopyClipRecordVertex(&record->vertex[i], &projected[indices[i]]);
	}

	DrawLevelOvr1P_SetClipRecordCursor(cursor + recordSize);
	return 1;
}

static int Ovr226_800a4c14_EmitGround4x1RenderedGT3RawOrClip(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                                             const struct TextureLayout *texture, uint32_t *inheritedOtEntry, int secondary)
{
	int triIndices[3];

	if (inheritedOtEntry == NULL)
	{
		return 1;
	}

	if (secondary)
	{
		triIndices[0] = indices[1];
		triIndices[1] = indices[3];
		triIndices[2] = indices[2];
	}
	else
	{
		triIndices[0] = indices[0];
		triIndices[1] = indices[1];
		triIndices[2] = indices[2];
	}

	if (DrawLevelOvr1P_AreProjectedVerticesHalfNearSigned(projected, triIndices, 3))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNearSigned(projected, triIndices, 3))
	{
		return Ovr226_800a4dcc_WriteGround4x1RenderedClippedRecordAtOtEntry(pb, block, projected, triIndices, 3, inheritedOtEntry);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedTriRawCodeAtOtEntry(pb, primMem, block, projected, triIndices, texture, inheritedOtEntry,
	                                                               DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
}

static int Ovr226_800a4cc8_EmitGround4x1RenderedGT4RawOrClip(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                                             const struct TextureLayout *texture, uint32_t *inheritedOtEntry)
{
	if (inheritedOtEntry == NULL)
	{
		return 1;
	}

	if (DrawLevelOvr1P_AreProjectedVerticesHalfNear(projected, indices, 4))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		return Ovr226_800a4dcc_WriteGround4x1RenderedClippedRecordAtOtEntry(pb, block, projected, indices, 4, inheritedOtEntry);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedQuadRawCodeAtOtEntry(pb, primMem, block, projected, indices, texture, inheritedOtEntry,
	                                                                DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
}

static int Ovr226_800a4c0c_DispatchGround4x1RenderedDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                               const struct TextureLayout *texture, uint32_t *inheritedOtEntry)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	(void)faceIndex;

	switch (handlerAddress)
	{
	case 0x800a4c14:
		return Ovr226_800a4c14_EmitGround4x1RenderedGT3RawOrClip(pb, primMem, block, projected, indices, texture, inheritedOtEntry, 0);
	case 0x800a4c0c:
		return Ovr226_800a4c14_EmitGround4x1RenderedGT3RawOrClip(pb, primMem, block, projected, indices, texture, inheritedOtEntry, 1);
	case 0x800a4cc8:
		return Ovr226_800a4cc8_EmitGround4x1RenderedGT4RawOrClip(pb, primMem, block, projected, indices, texture, inheritedOtEntry);
	default:
		return 1;
	}
}

static void Ovr226_800a4b54_PrepareGround4x1RenderedDeepestUv(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	u32 mosaicBase;
	u32 sourceOffset;
	const u8 *source;
	u32 uv0;
	u32 uv1;
	struct DrawLevelOvr1PScratchVertex *mutableProjected;

	if (projected != CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PScratchVertex, DRAW_LEVEL_OVR1P_DEEPEST_PROJECTED_FRAME_OFFSET))
	{
		return;
	}

	mosaicBase = DrawLevelOvr1P_Scratch()->mosaicTextureWord;
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native keeps this as a data-boundary guard for
	// host-rebased level texture words; non-native code follows retail.
	if (mosaicBase == 0 || ((s32)mosaicBase < 0 && !DrawLevelOvr1P_IsNativeLevelTexturePointer(mosaicBase)))
	{
		DrawLevelOvr1P_RestoreProjectedUvScratch();
		return;
	}
#endif
	if ((s32)mosaicBase > 0 && !DrawLevelOvr1P_IsNativeLevelTexturePointer(mosaicBase))
	{
		DrawLevelOvr1P_RestoreProjectedUvScratch();
		return;
	}

	if (DrawLevelOvr1P_Scratch()->previousDirectHandlerAddress != 0x800a4550)
	{
		DrawLevelOvr1P_RestoreProjectedUvScratch();
		return;
	}

	sourceOffset = *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_MOSAIC_SOURCE_INDEX_OFFSET) << 1;
	if ((s32)(DrawLevelOvr1P_Scratch()->selected4x1TableWord << 8) < 0)
	{
		sourceOffset += 0x30;
	}
	sourceOffset += *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_MOSAIC_SOURCE_BIAS_OFFSET);

	source = (const u8 *)(uintptr_t)(mosaicBase + sourceOffset);
	uv0 = DrawLevelOvr1P_ReadPackedWord(source + 0);
	uv1 = DrawLevelOvr1P_ReadPackedWord(source + 4);

	mutableProjected = (struct DrawLevelOvr1PScratchVertex *)projected;
	DrawLevelOvr1P_Scratch()->uv.uv0 = uv0;
	mutableProjected[indices[0]].flags = (u16)uv0;
	DrawLevelOvr1P_Scratch()->uv.uv1 = uv1;
	mutableProjected[indices[1]].flags = (u16)uv1;
	mutableProjected[indices[2]].flags = DrawLevelOvr1P_ReadPackedHalf(source + 8);
	mutableProjected[indices[3]].flags = DrawLevelOvr1P_ReadPackedHalf(source + 10);
}

static int Ovr226_800a4b54_DispatchGround4x1RenderedDeepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                            const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                            const struct TextureLayout *texture, uint32_t *inheritedOtEntry)
{
	int result;

	Ovr226_800a4b54_PrepareGround4x1RenderedDeepestUv(projected, indices);
	result = Ovr226_800a4c0c_DispatchGround4x1RenderedDirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtEntry);

	DrawLevelOvr1P_RestoreProjectedUvScratch();
	return result;
}

static int Ovr226_800a4ad0_Ground4x1RenderedNearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                         const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset, uint32_t *inheritedOtEntry)
{
	u32 nearMask = DrawLevelOvr1P_GetProjectedNearMaskAtScratchOffset(projected, indices, thresholdScratchOffset);
	struct DrawLevelOvr1PScratchVertex *sub;
	u32 handlerAddress;

	if (nearMask == 0)
	{
		return Ovr226_800a4c0c_DispatchGround4x1RenderedDirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtEntry);
	}

	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	Ovr226_800a4594_BuildGround4x1RenderedSubdivisionFrame(sub, projected, indices);

	handlerAddress = *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_DIRECT_NEAR_HANDLER_TABLE_OFFSET + nearMask);
	DrawLevelOvr1P_SetPreviousRecursiveHandler(handlerAddress);

	return Ovr226_800a44e0_DispatchGround4x1RenderedHelperWrappers(pb, primMem, block, sub, faceIndex, texture, depth + 1, handlerAddress, inheritedOtEntry);
}

static int Ovr226_800a46d0_Ground4x1RenderedPreflight(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                      const struct TextureLayout *texture, int depth, u32 allowedMask, uint32_t *inheritedOtEntry)
{
	u32 directMask = 0;

	if (DrawLevelOvr1P_IsProjectedFaceFullyNear(projected, indices))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		directMask = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
		DrawLevelOvr1P_Scratch()->directMask = directMask;
	}
	else if (!Ovr226_800a3b90_SelectAndStoreGround4x1DirectMask(projected, indices, allowedMask, &directMask))
	{
		return 1;
	}

	if (DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
	{
		return Ovr226_800a4b54_DispatchGround4x1RenderedDeepest(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtEntry);
	}

	return Ovr226_800a4ad0_Ground4x1RenderedNearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, depth, 0x28, inheritedOtEntry);
}

static int Ovr226_800a44e0_DispatchGround4x1RenderedHelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                   struct DrawLevelOvr1PScratchVertex *projected, int faceIndex,
                                                                   const struct TextureLayout *texture, int depth, u32 handlerAddress,
                                                                   uint32_t *inheritedOtEntry)
{
	switch (handlerAddress)
	{
	case 0x800a44e0:
	{
		const int firstIndices[4] = {0, 4, 2, 3};
		const int secondIndices[4] = {1, 3, 4, 2};

		if (!Ovr226_800a46d0_Ground4x1RenderedPreflight(pb, primMem, block, projected, firstIndices, faceIndex, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                inheritedOtEntry))
		{
			return 0;
		}

		return Ovr226_800a46d0_Ground4x1RenderedPreflight(pb, primMem, block, projected, secondIndices, faceIndex, texture, depth,
		                                                  DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY, inheritedOtEntry);
	}

	case 0x800a4518:
	{
		const int firstIndices[4] = {1, 8, 0, 2};
		const int secondIndices[4] = {0, 1, 8, 3};

		if (!Ovr226_800a46d0_Ground4x1RenderedPreflight(pb, primMem, block, projected, firstIndices, faceIndex, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                inheritedOtEntry))
		{
			return 0;
		}

		return Ovr226_800a46d0_Ground4x1RenderedPreflight(pb, primMem, block, projected, secondIndices, faceIndex, texture, depth,
		                                                  DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY, inheritedOtEntry);
	}

	case 0x800a4550:
	{
		const int firstIndices[4] = {0, 4, 2, 8};
		const int secondIndices[4] = {4, 1, 8, 3};

		DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x0);
		if (!Ovr226_800a46d0_Ground4x1RenderedPreflight(pb, primMem, block, projected, firstIndices, faceIndex, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                inheritedOtEntry))
		{
			return 0;
		}

		DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0xc);
		return Ovr226_800a46d0_Ground4x1RenderedPreflight(pb, primMem, block, projected, secondIndices, faceIndex, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                  inheritedOtEntry);
	}

	default:
		return 1;
	}
}

static int Ovr226_800a47f4_Ground4x1RenderedSelectorNearGate(struct PushBuffer *pb, struct PrimMem *primMem, struct DrawLevelOvr1PScratchVertex *projected,
                                                             struct QuadBlock *block, int faceIndex)
{
	int indices[4];
	u32 tableWord;
	u32 directMask = 0;
	u32 maxDepth;
	uint32_t *inheritedOtEntry;
	struct TextureLayout *texture;

	tableWord = DrawLevelOvr1P_Select4x1ProjectedFace(projected, block, faceIndex, indices);
	if (DrawLevelOvr1P_IsProjectedFaceFullyNear(projected, indices))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		directMask = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
		DrawLevelOvr1P_Scratch()->directMask = directMask;
	}
	else if (!Ovr226_800a3c70_SelectAndStoreGround4x1SelectorDirectMask(projected, indices, tableWord, &directMask))
	{
		return 1;
	}

	Ovr226_800a4950_StoreGround4x1RenderedClipHeader(tableWord);
	maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices);
	inheritedOtEntry = Ovr226_800a4978_ResolveGround4x1RenderedOtEntry(pb, block, projected, maxDepth, faceIndex);
	texture = Ovr226_800a49e0_SelectGround4x1RenderedTexture(block, projected, maxDepth);
	if (texture != NULL)
	{
		Ovr226_800a4a28_WriteGround4x1RenderedUv(projected, indices, texture, tableWord);
	}

	return Ovr226_800a4ad0_Ground4x1RenderedNearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, 0, 0x24, inheritedOtEntry);
}

static int Ovr226_800a52bc_DispatchGround4x2HelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                           struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, const struct TextureLayout *texture,
                                                           int depth, u32 handlerAddress, int handlerSlot, int inheritedOtIndex);
static int Ovr226_800a5b2c_Ground4x2NearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                 const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset, int inheritedOtIndex);
static int Ovr226_800a5c54_DispatchGround4x2Deepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                    const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                    const struct TextureLayout *texture, int inheritedOtIndex);
static int Ovr226_800a5d0c_DispatchGround4x2DirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                       const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                       const struct TextureLayout *texture, int inheritedOtIndex);

static int Ovr226_800a580c_Ground4x2DirectPreflight(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                    const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                    const struct TextureLayout *texture, int depth, u32 allowedMask, int inheritedOtIndex)
{
	u32 directMask = 0;

	if (!Ovr226_800a3b90_SelectAndStoreGround4x1DirectMask(projected, indices, allowedMask, &directMask))
	{
		return 1;
	}

	if (DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
	{
		return Ovr226_800a5c54_DispatchGround4x2Deepest(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtIndex);
	}

	return Ovr226_800a5b2c_Ground4x2NearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, depth, 0x28, inheritedOtIndex);
}

static int Ovr226_800a52bc_DispatchGround4x2DeepestCompact(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                           const struct DrawLevelOvr1PScratchVertex *projected, int faceIndex,
                                                           const struct TextureLayout *texture, int depth, int handlerSlot, int inheritedOtIndex)
{
	const struct DrawLevelOvr1PNearSubdivisionCase *subdivisionCase = DrawLevelOvr1P_GetDeepestGridCompactCase(handlerSlot);

	for (int i = 0; i < 2; i++)
	{
		int subIndices[4];

		for (int j = 0; j < 4; j++)
		{
			subIndices[j] = subdivisionCase->subIndices[i][j];
		}

		if (subdivisionCase->slotWords[i] != DRAW_LEVEL_OVR1P_SLOT_WORD_PRESERVE)
		{
			DrawLevelOvr1P_SetGridFaceSlotWord(projected, subdivisionCase->slotWords[i]);
		}

		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, subIndices, faceIndex, texture, depth, subdivisionCase->directMasks[i],
		                                              inheritedOtIndex))
		{
			return 0;
		}
	}

	return 1;
}

static int Ovr226_800a52bc_DispatchGround4x2HelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                           struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, const struct TextureLayout *texture,
                                                           int depth, u32 handlerAddress, int handlerSlot, int inheritedOtIndex)
{
	const u32 allowedQuad = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
	const u32 allowedTriPrimary = DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY;
	const u32 allowedTriSecondary = DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY;

	if (DrawLevelOvr1P_HandlerUsesDeepestCompactGrid(handlerAddress) && DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
	{
		return Ovr226_800a52bc_DispatchGround4x2DeepestCompact(pb, primMem, block, projected, faceIndex, texture, depth, handlerSlot, inheritedOtIndex);
	}

	switch (handlerSlot)
	{
	case 0:
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[0], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[1], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex);
	case 1:
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex);
	case 2:
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                              allowedTriPrimary, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex);
	case 3:
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex);
	case 4:
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                              allowedTriPrimary, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex);
	case 7:
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[2], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[3], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex);
	case 9:
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                              allowedTriSecondary, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex);
	case 11:
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                              allowedTriSecondary, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex);
	default:
		break;
	}

	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x0);
	if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, allowedQuad,
	                                              inheritedOtIndex))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0xc);
	if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, allowedQuad,
	                                              inheritedOtIndex))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x18);
	if (!Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, allowedQuad,
	                                              inheritedOtIndex))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x24);
	return Ovr226_800a580c_Ground4x2DirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth, allowedQuad,
	                                                inheritedOtIndex);
}

static int Ovr226_800a5b2c_Ground4x2NearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                 const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset, int inheritedOtIndex)
{
	u32 nearMask = DrawLevelOvr1P_GetProjectedNearMaskAtScratchOffset(projected, indices, thresholdScratchOffset);
	struct DrawLevelOvr1PScratchVertex *sub;
	u32 handlerAddress;
	int handlerSlot;

	if (nearMask == 0)
	{
		return Ovr226_800a5d0c_DispatchGround4x2DirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtIndex);
	}

	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	Ovr226_800a56f4_BuildGround4x2ListSubdivisionFrame(sub, projected, indices);

	handlerAddress = *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_DIRECT_NEAR_HANDLER_TABLE_OFFSET + nearMask);
	handlerSlot = DrawLevelOvr1P_GetNearSubdivisionTableSlot(nearMask);
	DrawLevelOvr1P_SetPreviousRecursiveHandler(handlerAddress);

	return Ovr226_800a52bc_DispatchGround4x2HelperWrappers(pb, primMem, block, sub, faceIndex, texture, depth + 1, handlerAddress, handlerSlot,
	                                                       inheritedOtIndex);
}

static void Ovr226_800a5c54_PrepareGround4x2DeepestUv(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	if (handlerAddress != 0)
	{
		DrawLevelOvr1P_PrepareDeepestMosaicUv(projected, indices, handlerAddress);
	}
}

static int Ovr226_800a5c54_DispatchGround4x2Deepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                    const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                    const struct TextureLayout *texture, int inheritedOtIndex)
{
	int result;

	Ovr226_800a5c54_PrepareGround4x2DeepestUv(projected, indices);
	result = Ovr226_800a5d0c_DispatchGround4x2DirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtIndex);

	DrawLevelOvr1P_RestoreProjectedUvScratch();
	return result;
}

static uint32_t *Ovr226_800a5e38_ResolveGround4x2DirectOtEntry(struct PushBuffer *pb, int inheritedOtIndex)
{
	if (inheritedOtIndex < 0)
	{
		return NULL;
	}

	return &pb->ptrOT[inheritedOtIndex];
}

static int Ovr226_800a5d14_EmitGround4x2GT3Raw(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, const struct TextureLayout *texture,
                                               int inheritedOtIndex, int secondary)
{
	int triIndices[3];
	uint32_t *otEntry = Ovr226_800a5e38_ResolveGround4x2DirectOtEntry(pb, inheritedOtIndex);

	if (otEntry == NULL)
	{
		return 1;
	}

	if (secondary)
	{
		triIndices[0] = indices[1];
		triIndices[1] = indices[3];
		triIndices[2] = indices[2];
	}
	else
	{
		triIndices[0] = indices[0];
		triIndices[1] = indices[1];
		triIndices[2] = indices[2];
	}

	return DrawLevelOvr1P_EmitPreparedProjectedTriRawCodeAtOtEntry(pb, primMem, block, projected, triIndices, texture, otEntry,
	                                                               DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
}

static int Ovr226_800a5d98_EmitGround4x2GT4Raw(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, const struct TextureLayout *texture,
                                               int inheritedOtIndex)
{
	uint32_t *otEntry = Ovr226_800a5e38_ResolveGround4x2DirectOtEntry(pb, inheritedOtIndex);

	if (otEntry == NULL)
	{
		return 1;
	}

	return DrawLevelOvr1P_EmitPreparedProjectedQuadRawCodeAtOtEntry(pb, primMem, block, projected, indices, texture, otEntry, DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
}

static int Ovr226_800a5d0c_DispatchGround4x2DirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                       const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                       const struct TextureLayout *texture, int inheritedOtIndex)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	(void)faceIndex;

	switch (handlerAddress)
	{
	case 0x800a5d14:
		return Ovr226_800a5d14_EmitGround4x2GT3Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex, 0);
	case 0x800a5d0c:
		return Ovr226_800a5d14_EmitGround4x2GT3Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex, 1);
	case 0x800a5d98:
		return Ovr226_800a5d98_EmitGround4x2GT4Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex);
	default:
		return 1;
	}
}

static int Ovr226_800a58ec_Ground4x2SelectorNearGate(struct PushBuffer *pb, struct PrimMem *primMem, struct DrawLevelOvr1PScratchVertex *projected,
                                                     struct QuadBlock *block, int faceIndex)
{
	int indices[4];
	u32 tableWord;
	u32 directMask = 0;
	u32 maxDepth;
	int inheritedOtIndex;
	struct TextureLayout *texture;

	tableWord = DrawLevelOvr1P_Select4x1ProjectedFace(projected, block, faceIndex, indices);
	if (!Ovr226_800a3c70_SelectAndStoreGround4x1SelectorDirectMask(projected, indices, tableWord, &directMask))
	{
		return 1;
	}

	maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices);
	inheritedOtIndex = Ovr226_800a3d98_ResolveGround4x1SelectorOtIndex(block, projected, maxDepth, faceIndex);
	texture = Ovr226_800a3e00_SelectGround4x1SelectorTexture(block, projected, maxDepth);
	if (texture != NULL)
	{
		DrawLevelOvr1P_WriteProjectedUv(projected, indices, texture, tableWord);
	}

	return Ovr226_800a5b2c_Ground4x2NearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, 0, 0x24, inheritedOtIndex);
}

static int Ovr226_800a725c_DispatchDynamicListHelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, const struct TextureLayout *texture,
                                                             int depth, u32 handlerAddress, int handlerSlot, int inheritedOtIndex);
static int Ovr226_800a78a8_DynamicListNearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                   const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                   const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset, int inheritedOtIndex);
static int Ovr226_800a79a0_DispatchDynamicListDeepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                      const struct TextureLayout *texture, int inheritedOtIndex);
static int Ovr226_800a7a58_DispatchDynamicListDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                         const struct TextureLayout *texture, int inheritedOtIndex);

static int Ovr226_800a7588_DynamicListDirectPreflight(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                      const struct TextureLayout *texture, int depth, u32 allowedMask, int inheritedOtIndex)
{
	u32 directMask = 0;

	if (!Ovr226_800a3b90_SelectAndStoreGround4x1DirectMask(projected, indices, allowedMask, &directMask))
	{
		return 1;
	}

	if (DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
	{
		return Ovr226_800a79a0_DispatchDynamicListDeepest(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtIndex);
	}

	return Ovr226_800a78a8_DynamicListNearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, depth, 0x28, inheritedOtIndex);
}

static int Ovr226_800a725c_DispatchDynamicListHelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, const struct TextureLayout *texture,
                                                             int depth, u32 handlerAddress, int handlerSlot, int inheritedOtIndex)
{
	const u32 allowedQuad = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
	const u32 allowedTriPrimary = DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY;
	const u32 allowedTriSecondary = DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY;

	(void)handlerAddress;

	switch (handlerSlot)
	{
	case 0:
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	case 1:
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	case 2:
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                allowedTriPrimary, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	case 3:
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	case 4:
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                allowedTriPrimary, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	case 7:
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	case 9:
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                allowedTriSecondary, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	case 11:
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                allowedTriSecondary, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	default:
		break;
	}

	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x0);
	if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, allowedQuad,
	                                                inheritedOtIndex))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0xc);
	if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, allowedQuad,
	                                                inheritedOtIndex))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x30);
	if (!Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, allowedQuad,
	                                                inheritedOtIndex))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x3c);
	return Ovr226_800a7588_DynamicListDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth, allowedQuad,
	                                                  inheritedOtIndex);
}

static int Ovr226_800a78a8_DynamicListNearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                   const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                   const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset, int inheritedOtIndex)
{
	u32 nearMask = DrawLevelOvr1P_GetProjectedNearMaskAtScratchOffset(projected, indices, thresholdScratchOffset);
	struct DrawLevelOvr1PScratchVertex *sub;
	u32 handlerAddress;
	int handlerSlot;

	if (nearMask == 0)
	{
		return Ovr226_800a7a58_DispatchDynamicListDirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtIndex);
	}

	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	Ovr226_800a74a0_BuildDynamicListSubdivisionFrame(sub, projected, indices);

	handlerAddress = *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_DIRECT_NEAR_HANDLER_TABLE_OFFSET + nearMask);
	handlerSlot = DrawLevelOvr1P_GetNearSubdivisionTableSlot(nearMask);
	DrawLevelOvr1P_SetPreviousRecursiveHandler(handlerAddress);

	return Ovr226_800a725c_DispatchDynamicListHelperWrappers(pb, primMem, block, sub, faceIndex, texture, depth + 1, handlerAddress, handlerSlot,
	                                                         inheritedOtIndex);
}

static void Ovr226_800a79a0_PrepareDynamicListDeepestUv(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	if (handlerAddress != 0)
	{
		DrawLevelOvr1P_PrepareDeepestMosaicUv(projected, indices, handlerAddress);
	}
}

static int Ovr226_800a79a0_DispatchDynamicListDeepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                      const struct TextureLayout *texture, int inheritedOtIndex)
{
	int result;

	Ovr226_800a79a0_PrepareDynamicListDeepestUv(projected, indices);
	result = Ovr226_800a7a58_DispatchDynamicListDirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtIndex);

	DrawLevelOvr1P_RestoreProjectedUvScratch();
	return result;
}

static int Ovr226_800a7a60_EmitDynamicListGT3Raw(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, const struct TextureLayout *texture,
                                                 int inheritedOtIndex, int secondary)
{
	return Ovr226_800a5d14_EmitGround4x2GT3Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex, secondary);
}

static int Ovr226_800a7ae4_EmitDynamicListGT4Raw(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, const struct TextureLayout *texture,
                                                 int inheritedOtIndex)
{
	return Ovr226_800a5d98_EmitGround4x2GT4Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex);
}

static int Ovr226_800a7a58_DispatchDynamicListDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                         const struct TextureLayout *texture, int inheritedOtIndex)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	(void)faceIndex;

	switch (handlerAddress)
	{
	case 0x800a7a60:
		return Ovr226_800a7a60_EmitDynamicListGT3Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex, 0);
	case 0x800a7a58:
		return Ovr226_800a7a60_EmitDynamicListGT3Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex, 1);
	case 0x800a7ae4:
		return Ovr226_800a7ae4_EmitDynamicListGT4Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex);
	default:
		return 1;
	}
}

static int Ovr226_800a7668_DynamicListSelectorNearGate(struct PushBuffer *pb, struct PrimMem *primMem, struct DrawLevelOvr1PScratchVertex *projected,
                                                       struct QuadBlock *block, int faceIndex)
{
	int indices[4];
	u32 tableWord;
	u32 directMask = 0;
	u32 maxDepth;
	int inheritedOtIndex;
	struct TextureLayout *texture;

	tableWord = DrawLevelOvr1P_Select4x1ProjectedFace(projected, block, faceIndex, indices);
	if (!Ovr226_800a3c70_SelectAndStoreGround4x1SelectorDirectMask(projected, indices, tableWord, &directMask))
	{
		return 1;
	}

	maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices);
	inheritedOtIndex = Ovr226_800a3d98_ResolveGround4x1SelectorOtIndex(block, projected, maxDepth, faceIndex);
	texture = Ovr226_800a3e00_SelectGround4x1SelectorTexture(block, projected, maxDepth);
	if (texture != NULL)
	{
		DrawLevelOvr1P_WriteProjectedUv(projected, indices, texture, tableWord);
	}

	return Ovr226_800a78a8_DynamicListNearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, 0, 0x24, inheritedOtIndex);
}

static int Ovr226_800a8e7c_DispatchWideDynamicHelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, const struct TextureLayout *texture,
                                                             int depth, u32 handlerAddress, int handlerSlot, int inheritedOtIndex);
static int Ovr226_800a94c8_WideDynamicNearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                   const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                   const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset, int inheritedOtIndex);
static int Ovr226_800a95c0_DispatchWideDynamicDeepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                      const struct TextureLayout *texture, int inheritedOtIndex);
static int Ovr226_800a9678_DispatchWideDynamicDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                         const struct TextureLayout *texture, int inheritedOtIndex);

static int Ovr226_800a91a8_WideDynamicDirectPreflight(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                      const struct TextureLayout *texture, int depth, u32 allowedMask, int inheritedOtIndex)
{
	u32 directMask = 0;

	if (!Ovr226_800a3b90_SelectAndStoreGround4x1DirectMask(projected, indices, allowedMask, &directMask))
	{
		return 1;
	}

	if (DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
	{
		return Ovr226_800a95c0_DispatchWideDynamicDeepest(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtIndex);
	}

	return Ovr226_800a94c8_WideDynamicNearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, depth, 0x28, inheritedOtIndex);
}

static int Ovr226_800a8e7c_DispatchWideDynamicHelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, const struct TextureLayout *texture,
                                                             int depth, u32 handlerAddress, int handlerSlot, int inheritedOtIndex)
{
	const u32 allowedQuad = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
	const u32 allowedTriPrimary = DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY;
	const u32 allowedTriSecondary = DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY;

	(void)handlerAddress;

	switch (handlerSlot)
	{
	case 0:
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	case 1:
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	case 2:
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                allowedTriPrimary, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	case 3:
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	case 4:
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                allowedTriPrimary, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	case 7:
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	case 9:
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                allowedTriSecondary, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	case 11:
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                allowedTriSecondary, inheritedOtIndex))
		{
			return 0;
		}
		if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtIndex))
		{
			return 0;
		}
		return Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtIndex);
	default:
		break;
	}

	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x0);
	if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, allowedQuad,
	                                                inheritedOtIndex))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0xc);
	if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, allowedQuad,
	                                                inheritedOtIndex))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x30);
	if (!Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, allowedQuad,
	                                                inheritedOtIndex))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x3c);
	return Ovr226_800a91a8_WideDynamicDirectPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth, allowedQuad,
	                                                  inheritedOtIndex);
}

static int Ovr226_800a94c8_WideDynamicNearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                   const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                   const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset, int inheritedOtIndex)
{
	u32 nearMask = DrawLevelOvr1P_GetProjectedNearMaskAtScratchOffset(projected, indices, thresholdScratchOffset);
	struct DrawLevelOvr1PScratchVertex *sub;
	u32 handlerAddress;
	int handlerSlot;

	if (nearMask == 0)
	{
		return Ovr226_800a9678_DispatchWideDynamicDirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtIndex);
	}

	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	Ovr226_800a90c0_BuildWideDynamicSubdivisionFrame(sub, projected, indices);

	handlerAddress = *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_DIRECT_NEAR_HANDLER_TABLE_OFFSET + nearMask);
	handlerSlot = DrawLevelOvr1P_GetNearSubdivisionTableSlot(nearMask);
	DrawLevelOvr1P_SetPreviousRecursiveHandler(handlerAddress);

	return Ovr226_800a8e7c_DispatchWideDynamicHelperWrappers(pb, primMem, block, sub, faceIndex, texture, depth + 1, handlerAddress, handlerSlot,
	                                                         inheritedOtIndex);
}

static void Ovr226_800a95c0_PrepareWideDynamicDeepestUv(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	if (handlerAddress != 0)
	{
		DrawLevelOvr1P_PrepareDeepestMosaicUv(projected, indices, handlerAddress);
	}
}

static int Ovr226_800a95c0_DispatchWideDynamicDeepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                      const struct TextureLayout *texture, int inheritedOtIndex)
{
	int result;

	Ovr226_800a95c0_PrepareWideDynamicDeepestUv(projected, indices);
	result = Ovr226_800a9678_DispatchWideDynamicDirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtIndex);

	DrawLevelOvr1P_RestoreProjectedUvScratch();
	return result;
}

static int Ovr226_800a9680_EmitWideDynamicGT3Raw(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, const struct TextureLayout *texture,
                                                 int inheritedOtIndex, int secondary)
{
	return Ovr226_800a5d14_EmitGround4x2GT3Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex, secondary);
}

static int Ovr226_800a9704_EmitWideDynamicGT4Raw(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, const struct TextureLayout *texture,
                                                 int inheritedOtIndex)
{
	return Ovr226_800a5d98_EmitGround4x2GT4Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex);
}

static int Ovr226_800a9678_DispatchWideDynamicDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                         const struct TextureLayout *texture, int inheritedOtIndex)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	(void)faceIndex;

	switch (handlerAddress)
	{
	case 0x800a9680:
		return Ovr226_800a9680_EmitWideDynamicGT3Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex, 0);
	case 0x800a9678:
		return Ovr226_800a9680_EmitWideDynamicGT3Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex, 1);
	case 0x800a9704:
		return Ovr226_800a9704_EmitWideDynamicGT4Raw(pb, primMem, block, projected, indices, texture, inheritedOtIndex);
	default:
		return 1;
	}
}

static int Ovr226_800a9288_WideDynamicSelectorNearGate(struct PushBuffer *pb, struct PrimMem *primMem, struct DrawLevelOvr1PScratchVertex *projected,
                                                       struct QuadBlock *block, int faceIndex)
{
	int indices[4];
	u32 tableWord;
	u32 directMask = 0;
	u32 maxDepth;
	int inheritedOtIndex;
	struct TextureLayout *texture;

	tableWord = DrawLevelOvr1P_Select4x1ProjectedFace(projected, block, faceIndex, indices);
	if (!Ovr226_800a3c70_SelectAndStoreGround4x1SelectorDirectMask(projected, indices, tableWord, &directMask))
	{
		return 1;
	}

	maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices);
	inheritedOtIndex = Ovr226_800a3d98_ResolveGround4x1SelectorOtIndex(block, projected, maxDepth, faceIndex);
	texture = Ovr226_800a3e00_SelectGround4x1SelectorTexture(block, projected, maxDepth);
	if (texture != NULL)
	{
		DrawLevelOvr1P_WriteProjectedUv(projected, indices, texture, tableWord);
	}

	return Ovr226_800a94c8_WideDynamicNearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, 0, 0x24, inheritedOtIndex);
}

static int Ovr226_800a661c_Ground4x2RenderedPreflight(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                      const struct TextureLayout *texture, int depth, u32 allowedMask, uint32_t *inheritedOtEntry);
static int Ovr226_800a61c0_DispatchGround4x2RenderedHelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                   struct DrawLevelOvr1PScratchVertex *projected, int faceIndex,
                                                                   const struct TextureLayout *texture, int depth, u32 handlerAddress, int handlerSlot,
                                                                   uint32_t *inheritedOtEntry);
static int Ovr226_800a69dc_Ground4x2RenderedNearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                         const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset,
                                                         uint32_t *inheritedOtEntry);
static int Ovr226_800a6af4_DispatchGround4x2RenderedDeepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                            const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                            const struct TextureLayout *texture, uint32_t *inheritedOtEntry);
static int Ovr226_800a6bac_DispatchGround4x2RenderedDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                               const struct TextureLayout *texture, uint32_t *inheritedOtEntry);

static void Ovr226_800a689c_StoreGround4x2RenderedClipHeader(u32 tableWord)
{
	DrawLevelOvr1P_StoreRenderedClipRecordHeader(tableWord);
}

static uint32_t *Ovr226_800a68c8_ResolveGround4x2RenderedOtEntry(struct PushBuffer *pb, const struct QuadBlock *block,
                                                                 const struct DrawLevelOvr1PScratchVertex *projected, u32 maxDepth, int faceIndex)
{
	return Ovr226_800a4978_ResolveGround4x1RenderedOtEntry(pb, block, projected, maxDepth, faceIndex);
}

static struct TextureLayout *Ovr226_800a6930_SelectGround4x2RenderedTexture(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected,
                                                                            u32 maxDepth)
{
	return Ovr226_800a49e0_SelectGround4x1RenderedTexture(block, projected, maxDepth);
}

static void Ovr226_800a6970_WriteGround4x2RenderedUv(struct DrawLevelOvr1PScratchVertex *projected, const int *indices, const struct TextureLayout *texture,
                                                     u32 tableWord)
{
	DrawLevelOvr1P_WriteProjectedUv(projected, indices, texture, tableWord);
}

static int Ovr226_800a6d6c_WriteGround4x2RenderedClippedRecordAtOtEntry(struct PushBuffer *pb, const struct QuadBlock *block,
                                                                        const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count,
                                                                        uint32_t *otEntry)
{
	struct DrawLevelOvr1PClipRecord *record;
	u8 *cursor = DrawLevelOvr1P_GetClipRecordCursor();
	size_t recordSize = DrawLevelOvr1P_GetClipRecordSize(count);

	(void)pb;

	if (otEntry == NULL)
	{
		return 1;
	}

	if (!DrawLevelOvr1P_ShouldWriteRenderedClippedRecord(projected, indices, count))
	{
		return 1;
	}

	record = (struct DrawLevelOvr1PClipRecord *)cursor;
	record->header = DrawLevelOvr1P_GetRenderedClipRecordHeader(block, count);
	record->otEntry = (u32)(uintptr_t)otEntry;
	record->tpage = DrawLevelOvr1P_Scratch()->uv.tpage;
	record->clut = DrawLevelOvr1P_Scratch()->uv.clut;

	for (int i = 0; i < count; i++)
	{
		DrawLevelOvr1P_CopyClipRecordVertex(&record->vertex[i], &projected[indices[i]]);
	}

	DrawLevelOvr1P_SetClipRecordCursor(cursor + recordSize);
	return 1;
}

static int Ovr226_800a6bb4_EmitGround4x2RenderedGT3RawOrClip(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                                             const struct TextureLayout *texture, uint32_t *inheritedOtEntry, int secondary)
{
	int triIndices[3];

	if (inheritedOtEntry == NULL)
	{
		return 1;
	}

	if (secondary)
	{
		triIndices[0] = indices[1];
		triIndices[1] = indices[3];
		triIndices[2] = indices[2];
	}
	else
	{
		triIndices[0] = indices[0];
		triIndices[1] = indices[1];
		triIndices[2] = indices[2];
	}

	if (DrawLevelOvr1P_AreProjectedVerticesHalfNearSigned(projected, triIndices, 3))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNearSigned(projected, triIndices, 3))
	{
		return Ovr226_800a6d6c_WriteGround4x2RenderedClippedRecordAtOtEntry(pb, block, projected, triIndices, 3, inheritedOtEntry);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedTriRawCodeAtOtEntry(pb, primMem, block, projected, triIndices, texture, inheritedOtEntry,
	                                                               DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
}

static int Ovr226_800a6c68_EmitGround4x2RenderedGT4RawOrClip(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                                             const struct TextureLayout *texture, uint32_t *inheritedOtEntry)
{
	if (inheritedOtEntry == NULL)
	{
		return 1;
	}

	if (DrawLevelOvr1P_AreProjectedVerticesHalfNear(projected, indices, 4))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		return Ovr226_800a6d6c_WriteGround4x2RenderedClippedRecordAtOtEntry(pb, block, projected, indices, 4, inheritedOtEntry);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedQuadRawCodeAtOtEntry(pb, primMem, block, projected, indices, texture, inheritedOtEntry,
	                                                                DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
}

static int Ovr226_800a6bac_DispatchGround4x2RenderedDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                               const struct TextureLayout *texture, uint32_t *inheritedOtEntry)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	(void)faceIndex;

	switch (handlerAddress)
	{
	case 0x800a6bb4:
		return Ovr226_800a6bb4_EmitGround4x2RenderedGT3RawOrClip(pb, primMem, block, projected, indices, texture, inheritedOtEntry, 0);
	case 0x800a6bac:
		return Ovr226_800a6bb4_EmitGround4x2RenderedGT3RawOrClip(pb, primMem, block, projected, indices, texture, inheritedOtEntry, 1);
	case 0x800a6c68:
		return Ovr226_800a6c68_EmitGround4x2RenderedGT4RawOrClip(pb, primMem, block, projected, indices, texture, inheritedOtEntry);
	default:
		return 1;
	}
}

static void Ovr226_800a6af4_PrepareGround4x2RenderedDeepestUv(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	if (handlerAddress != 0)
	{
		DrawLevelOvr1P_PrepareDeepestMosaicUv(projected, indices, handlerAddress);
	}
}

static int Ovr226_800a6af4_DispatchGround4x2RenderedDeepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                            const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                            const struct TextureLayout *texture, uint32_t *inheritedOtEntry)
{
	int result;

	Ovr226_800a6af4_PrepareGround4x2RenderedDeepestUv(projected, indices);
	result = Ovr226_800a6bac_DispatchGround4x2RenderedDirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtEntry);

	DrawLevelOvr1P_RestoreProjectedUvScratch();
	return result;
}

static int Ovr226_800a69dc_Ground4x2RenderedNearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                         const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset, uint32_t *inheritedOtEntry)
{
	u32 nearMask = DrawLevelOvr1P_GetProjectedNearMaskAtScratchOffset(projected, indices, thresholdScratchOffset);
	struct DrawLevelOvr1PScratchVertex *sub;
	u32 handlerAddress;
	int handlerSlot;

	if (nearMask == 0)
	{
		return Ovr226_800a6bac_DispatchGround4x2RenderedDirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtEntry);
	}

	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	Ovr226_800a6510_BuildGround4x2RenderedSubdivisionFrame(sub, projected, indices);

	handlerAddress = *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_DIRECT_NEAR_HANDLER_TABLE_OFFSET + nearMask);
	handlerSlot = DrawLevelOvr1P_GetNearSubdivisionTableSlot(nearMask);
	DrawLevelOvr1P_SetPreviousRecursiveHandler(handlerAddress);

	return Ovr226_800a61c0_DispatchGround4x2RenderedHelperWrappers(pb, primMem, block, sub, faceIndex, texture, depth + 1, handlerAddress, handlerSlot,
	                                                               inheritedOtEntry);
}

static int Ovr226_800a661c_Ground4x2RenderedPreflight(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                      const struct TextureLayout *texture, int depth, u32 allowedMask, uint32_t *inheritedOtEntry)
{
	u32 directMask = 0;

	if (DrawLevelOvr1P_IsProjectedFaceFullyNear(projected, indices))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		directMask = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
		DrawLevelOvr1P_Scratch()->directMask = directMask;
	}
	else if (!Ovr226_800a3b90_SelectAndStoreGround4x1DirectMask(projected, indices, allowedMask, &directMask))
	{
		return 1;
	}

	if (DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
	{
		return Ovr226_800a6af4_DispatchGround4x2RenderedDeepest(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtEntry);
	}

	return Ovr226_800a69dc_Ground4x2RenderedNearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, depth, 0x28, inheritedOtEntry);
}

static int Ovr226_800a6260_DispatchGround4x2RenderedDeepestCompact(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                   const struct DrawLevelOvr1PScratchVertex *projected, int faceIndex,
                                                                   const struct TextureLayout *texture, int depth, int handlerSlot, uint32_t *inheritedOtEntry)
{
	const struct DrawLevelOvr1PNearSubdivisionCase *subdivisionCase = DrawLevelOvr1P_GetDeepestGridCompactCase(handlerSlot);

	for (int i = 0; i < 2; i++)
	{
		int subIndices[4];

		for (int j = 0; j < 4; j++)
		{
			subIndices[j] = subdivisionCase->subIndices[i][j];
		}

		if (subdivisionCase->slotWords[i] != DRAW_LEVEL_OVR1P_SLOT_WORD_PRESERVE)
		{
			DrawLevelOvr1P_SetGridFaceSlotWord(projected, subdivisionCase->slotWords[i]);
		}

		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, subIndices, faceIndex, texture, depth, subdivisionCase->directMasks[i],
		                                                inheritedOtEntry))
		{
			return 0;
		}
	}

	return 1;
}

static int Ovr226_800a61c0_DispatchGround4x2RenderedHelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                   struct DrawLevelOvr1PScratchVertex *projected, int faceIndex,
                                                                   const struct TextureLayout *texture, int depth, u32 handlerAddress, int handlerSlot,
                                                                   uint32_t *inheritedOtEntry)
{
	const u32 allowedQuad = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
	const u32 allowedTriPrimary = DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY;
	const u32 allowedTriSecondary = DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY;

	if (DrawLevelOvr1P_HandlerUsesDeepestCompactGrid(handlerAddress) && DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
	{
		return Ovr226_800a6260_DispatchGround4x2RenderedDeepestCompact(pb, primMem, block, projected, faceIndex, texture, depth, handlerSlot, inheritedOtEntry);
	}

	switch (handlerSlot)
	{
	case 0:
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtEntry);
	case 1:
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtEntry);
	case 2:
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                allowedTriPrimary, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtEntry);
	case 3:
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtEntry);
	case 4:
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                allowedTriPrimary, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtEntry);
	case 7:
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtEntry);
	case 9:
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                allowedTriSecondary, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtEntry);
	case 11:
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                allowedTriSecondary, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                  allowedQuad, inheritedOtEntry);
	default:
		break;
	}

	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x0);
	if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, allowedQuad,
	                                                inheritedOtEntry))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0xc);
	if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, allowedQuad,
	                                                inheritedOtEntry))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x18);
	if (!Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, allowedQuad,
	                                                inheritedOtEntry))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x24);
	return Ovr226_800a661c_Ground4x2RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth, allowedQuad,
	                                                  inheritedOtEntry);
}

static int Ovr226_800a6740_Ground4x2RenderedSelectorNearGate(struct PushBuffer *pb, struct PrimMem *primMem, struct DrawLevelOvr1PScratchVertex *projected,
                                                             struct QuadBlock *block, int faceIndex)
{
	int indices[4];
	u32 tableWord;
	u32 directMask = 0;
	u32 maxDepth;
	uint32_t *inheritedOtEntry;
	struct TextureLayout *texture;

	tableWord = DrawLevelOvr1P_Select4x1ProjectedFace(projected, block, faceIndex, indices);
	if (DrawLevelOvr1P_IsProjectedFaceFullyNear(projected, indices))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		directMask = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
		DrawLevelOvr1P_Scratch()->directMask = directMask;
	}
	else if (!Ovr226_800a3c70_SelectAndStoreGround4x1SelectorDirectMask(projected, indices, tableWord, &directMask))
	{
		return 1;
	}

	Ovr226_800a689c_StoreGround4x2RenderedClipHeader(tableWord);
	maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices);
	inheritedOtEntry = Ovr226_800a68c8_ResolveGround4x2RenderedOtEntry(pb, block, projected, maxDepth, faceIndex);
	texture = Ovr226_800a6930_SelectGround4x2RenderedTexture(block, projected, maxDepth);
	if (texture != NULL)
	{
		Ovr226_800a6970_WriteGround4x2RenderedUv(projected, indices, texture, tableWord);
	}

	return Ovr226_800a69dc_Ground4x2RenderedNearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, 0, 0x24, inheritedOtEntry);
}

static int Ovr226_800a825c_DynamicRenderedPreflight(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                    const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                    const struct TextureLayout *texture, int depth, u32 allowedMask, uint32_t *inheritedOtEntry);
static int Ovr226_800a7f0c_DispatchDynamicRenderedHelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                 struct DrawLevelOvr1PScratchVertex *projected, int faceIndex,
                                                                 const struct TextureLayout *texture, int depth, u32 handlerAddress, int handlerSlot,
                                                                 uint32_t *inheritedOtEntry);
static int Ovr226_800a861c_DynamicRenderedNearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                       const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                       const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset, uint32_t *inheritedOtEntry);
static int Ovr226_800a8714_DispatchDynamicRenderedDeepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                          const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                          const struct TextureLayout *texture, uint32_t *inheritedOtEntry);
static int Ovr226_800a87cc_DispatchDynamicRenderedDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                             const struct TextureLayout *texture, uint32_t *inheritedOtEntry);

static void Ovr226_800a84d0_StoreDynamicRenderedClipHeader(u32 tableWord)
{
	DrawLevelOvr1P_StoreRenderedClipRecordHeader(tableWord);
}

static uint32_t *Ovr226_800a8504_ResolveDynamicRenderedOtEntry(struct PushBuffer *pb, const struct QuadBlock *block,
                                                               const struct DrawLevelOvr1PScratchVertex *projected, u32 maxDepth, int faceIndex)
{
	return Ovr226_800a68c8_ResolveGround4x2RenderedOtEntry(pb, block, projected, maxDepth, faceIndex);
}

static struct TextureLayout *Ovr226_800a856c_SelectDynamicRenderedTexture(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected,
                                                                          u32 maxDepth)
{
	return Ovr226_800a6930_SelectGround4x2RenderedTexture(block, projected, maxDepth);
}

static void Ovr226_800a85b0_WriteDynamicRenderedUv(struct DrawLevelOvr1PScratchVertex *projected, const int *indices, const struct TextureLayout *texture,
                                                   u32 tableWord)
{
	DrawLevelOvr1P_WriteProjectedUv(projected, indices, texture, tableWord);
}

static int Ovr226_800a898c_WriteDynamicRenderedClippedRecordAtOtEntry(struct PushBuffer *pb, const struct QuadBlock *block,
                                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count,
                                                                      uint32_t *otEntry)
{
	struct DrawLevelOvr1PClipRecord *record;
	u8 *cursor = DrawLevelOvr1P_GetClipRecordCursor();
	size_t recordSize = DrawLevelOvr1P_GetClipRecordSize(count);

	(void)pb;

	if (otEntry == NULL)
	{
		return 1;
	}

	if (!DrawLevelOvr1P_ShouldWriteRenderedClippedRecord(projected, indices, count))
	{
		return 1;
	}

	record = (struct DrawLevelOvr1PClipRecord *)cursor;
	record->header = DrawLevelOvr1P_GetRenderedClipRecordHeader(block, count);
	record->otEntry = (u32)(uintptr_t)otEntry;
	record->tpage = DrawLevelOvr1P_Scratch()->uv.tpage;
	record->clut = DrawLevelOvr1P_Scratch()->uv.clut;

	for (int i = 0; i < count; i++)
	{
		DrawLevelOvr1P_CopyClipRecordVertex(&record->vertex[i], &projected[indices[i]]);
	}

	DrawLevelOvr1P_SetClipRecordCursor(cursor + recordSize);
	return 1;
}

static int Ovr226_800a87d4_EmitDynamicRenderedGT3RawOrClip(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                           const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                                           const struct TextureLayout *texture, uint32_t *inheritedOtEntry, int secondary)
{
	int triIndices[3];

	if (inheritedOtEntry == NULL)
	{
		return 1;
	}

	if (secondary)
	{
		triIndices[0] = indices[1];
		triIndices[1] = indices[3];
		triIndices[2] = indices[2];
	}
	else
	{
		triIndices[0] = indices[0];
		triIndices[1] = indices[1];
		triIndices[2] = indices[2];
	}

	if (DrawLevelOvr1P_AreProjectedVerticesHalfNearSigned(projected, triIndices, 3))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNearSigned(projected, triIndices, 3))
	{
		return Ovr226_800a898c_WriteDynamicRenderedClippedRecordAtOtEntry(pb, block, projected, triIndices, 3, inheritedOtEntry);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedTriRawCodeAtOtEntry(pb, primMem, block, projected, triIndices, texture, inheritedOtEntry,
	                                                               DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
}

static int Ovr226_800a8888_EmitDynamicRenderedGT4RawOrClip(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                           const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                                           const struct TextureLayout *texture, uint32_t *inheritedOtEntry)
{
	if (inheritedOtEntry == NULL)
	{
		return 1;
	}

	if (DrawLevelOvr1P_AreProjectedVerticesHalfNear(projected, indices, 4))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		return Ovr226_800a898c_WriteDynamicRenderedClippedRecordAtOtEntry(pb, block, projected, indices, 4, inheritedOtEntry);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedQuadRawCodeAtOtEntry(pb, primMem, block, projected, indices, texture, inheritedOtEntry,
	                                                                DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
}

static int Ovr226_800a87cc_DispatchDynamicRenderedDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                             const struct TextureLayout *texture, uint32_t *inheritedOtEntry)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	(void)faceIndex;

	switch (handlerAddress)
	{
	case 0x800a87d4:
		return Ovr226_800a87d4_EmitDynamicRenderedGT3RawOrClip(pb, primMem, block, projected, indices, texture, inheritedOtEntry, 0);
	case 0x800a87cc:
		return Ovr226_800a87d4_EmitDynamicRenderedGT3RawOrClip(pb, primMem, block, projected, indices, texture, inheritedOtEntry, 1);
	case 0x800a8888:
		return Ovr226_800a8888_EmitDynamicRenderedGT4RawOrClip(pb, primMem, block, projected, indices, texture, inheritedOtEntry);
	default:
		return 1;
	}
}

static void Ovr226_800a8714_PrepareDynamicRenderedDeepestUv(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	if (handlerAddress != 0)
	{
		DrawLevelOvr1P_PrepareDeepestMosaicUv(projected, indices, handlerAddress);
	}
}

static int Ovr226_800a8714_DispatchDynamicRenderedDeepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                          const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                          const struct TextureLayout *texture, uint32_t *inheritedOtEntry)
{
	int result;

	Ovr226_800a8714_PrepareDynamicRenderedDeepestUv(projected, indices);
	result = Ovr226_800a87cc_DispatchDynamicRenderedDirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtEntry);

	DrawLevelOvr1P_RestoreProjectedUvScratch();
	return result;
}

static int Ovr226_800a861c_DynamicRenderedNearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                       const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                       const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset, uint32_t *inheritedOtEntry)
{
	u32 nearMask = DrawLevelOvr1P_GetProjectedNearMaskAtScratchOffset(projected, indices, thresholdScratchOffset);
	struct DrawLevelOvr1PScratchVertex *sub;
	u32 handlerAddress;
	int handlerSlot;

	if (nearMask == 0)
	{
		return Ovr226_800a87cc_DispatchDynamicRenderedDirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtEntry);
	}

	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	Ovr226_800a8150_BuildDynamicRenderedSubdivisionFrame(sub, projected, indices);

	handlerAddress = *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_DIRECT_NEAR_HANDLER_TABLE_OFFSET + nearMask);
	handlerSlot = DrawLevelOvr1P_GetNearSubdivisionTableSlot(nearMask);
	DrawLevelOvr1P_SetPreviousRecursiveHandler(handlerAddress);

	return Ovr226_800a7f0c_DispatchDynamicRenderedHelperWrappers(pb, primMem, block, sub, faceIndex, texture, depth + 1, handlerAddress, handlerSlot,
	                                                             inheritedOtEntry);
}

static int Ovr226_800a825c_DynamicRenderedPreflight(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                    const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                    const struct TextureLayout *texture, int depth, u32 allowedMask, uint32_t *inheritedOtEntry)
{
	u32 directMask = 0;

	if (DrawLevelOvr1P_IsProjectedFaceFullyNear(projected, indices))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		directMask = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
		DrawLevelOvr1P_Scratch()->directMask = directMask;
	}
	else if (!Ovr226_800a3b90_SelectAndStoreGround4x1DirectMask(projected, indices, allowedMask, &directMask))
	{
		return 1;
	}

	if (DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
	{
		return Ovr226_800a8714_DispatchDynamicRenderedDeepest(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtEntry);
	}

	return Ovr226_800a861c_DynamicRenderedNearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, depth, 0x28, inheritedOtEntry);
}

static int Ovr226_800a7f0c_DispatchDynamicRenderedHelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                 struct DrawLevelOvr1PScratchVertex *projected, int faceIndex,
                                                                 const struct TextureLayout *texture, int depth, u32 handlerAddress, int handlerSlot,
                                                                 uint32_t *inheritedOtEntry)
{
	const u32 allowedQuad = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
	const u32 allowedTriPrimary = DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY;
	const u32 allowedTriSecondary = DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY;

	(void)handlerAddress;

	switch (handlerSlot)
	{
	case 0:
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[0], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[1], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	case 1:
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	case 2:
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                              allowedTriPrimary, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	case 3:
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	case 4:
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                              allowedTriPrimary, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	case 7:
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[2], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[3], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	case 9:
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                              allowedTriSecondary, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	case 11:
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                              allowedTriSecondary, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	default:
		break;
	}

	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x0);
	if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, allowedQuad,
	                                              inheritedOtEntry))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0xc);
	if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, allowedQuad,
	                                              inheritedOtEntry))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x30);
	if (!Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, allowedQuad,
	                                              inheritedOtEntry))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x3c);
	return Ovr226_800a825c_DynamicRenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth, allowedQuad,
	                                                inheritedOtEntry);
}

static int Ovr226_800a8380_DynamicRenderedSelectorNearGate(struct PushBuffer *pb, struct PrimMem *primMem, struct DrawLevelOvr1PScratchVertex *projected,
                                                           struct QuadBlock *block, int faceIndex)
{
	int indices[4];
	u32 tableWord;
	u32 directMask = 0;
	u32 maxDepth;
	uint32_t *inheritedOtEntry;
	struct TextureLayout *texture;

	tableWord = DrawLevelOvr1P_Select4x1ProjectedFace(projected, block, faceIndex, indices);
	if (DrawLevelOvr1P_IsProjectedFaceFullyNear(projected, indices))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		directMask = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
		DrawLevelOvr1P_Scratch()->directMask = directMask;
	}
	else if (!Ovr226_800a3c70_SelectAndStoreGround4x1SelectorDirectMask(projected, indices, tableWord, &directMask))
	{
		return 1;
	}

	Ovr226_800a84d0_StoreDynamicRenderedClipHeader(tableWord);
	maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices);
	inheritedOtEntry = Ovr226_800a8504_ResolveDynamicRenderedOtEntry(pb, block, projected, maxDepth, faceIndex);
	texture = Ovr226_800a856c_SelectDynamicRenderedTexture(block, projected, maxDepth);
	if (texture != NULL)
	{
		Ovr226_800a85b0_WriteDynamicRenderedUv(projected, indices, texture, tableWord);
	}

	return Ovr226_800a861c_DynamicRenderedNearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, 0, 0x24, inheritedOtEntry);
}

static int Ovr226_800a9e7c_Quad4x4RenderedPreflight(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                    const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                    const struct TextureLayout *texture, int depth, u32 allowedMask, uint32_t *inheritedOtEntry);
static int Ovr226_800a9b2c_DispatchQuad4x4RenderedHelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                 struct DrawLevelOvr1PScratchVertex *projected, int faceIndex,
                                                                 const struct TextureLayout *texture, int depth, u32 handlerAddress, int handlerSlot,
                                                                 uint32_t *inheritedOtEntry);
static int Ovr226_800aa23c_Quad4x4RenderedNearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                       const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                       const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset, uint32_t *inheritedOtEntry);
static int Ovr226_800aa334_DispatchQuad4x4RenderedDeepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                          const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                          const struct TextureLayout *texture, uint32_t *inheritedOtEntry);
static int Ovr226_800aa3ec_DispatchQuad4x4RenderedDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                             const struct TextureLayout *texture, uint32_t *inheritedOtEntry);

static void Ovr226_800aa0fc_StoreQuad4x4RenderedClipHeader(u32 tableWord)
{
	DrawLevelOvr1P_StoreRenderedClipRecordHeader(tableWord);
}

static uint32_t *Ovr226_800aa124_ResolveQuad4x4RenderedOtEntry(struct PushBuffer *pb, const struct QuadBlock *block,
                                                               const struct DrawLevelOvr1PScratchVertex *projected, u32 maxDepth, int faceIndex)
{
	return Ovr226_800a68c8_ResolveGround4x2RenderedOtEntry(pb, block, projected, maxDepth, faceIndex);
}

static struct TextureLayout *Ovr226_800aa18c_SelectQuad4x4RenderedTexture(const struct QuadBlock *block, const struct DrawLevelOvr1PScratchVertex *projected,
                                                                          u32 maxDepth)
{
	return Ovr226_800a6930_SelectGround4x2RenderedTexture(block, projected, maxDepth);
}

static void Ovr226_800aa1d0_WriteQuad4x4RenderedUv(struct DrawLevelOvr1PScratchVertex *projected, const int *indices, const struct TextureLayout *texture,
                                                   u32 tableWord)
{
	DrawLevelOvr1P_WriteProjectedUv(projected, indices, texture, tableWord);
}

static int Ovr226_800aa5ac_WriteQuad4x4RenderedClippedRecordAtOtEntry(struct PushBuffer *pb, const struct QuadBlock *block,
                                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int count,
                                                                      uint32_t *otEntry)
{
	struct DrawLevelOvr1PClipRecord *record;
	u8 *cursor = DrawLevelOvr1P_GetClipRecordCursor();
	size_t recordSize = DrawLevelOvr1P_GetClipRecordSize(count);

	(void)pb;

	if (otEntry == NULL)
	{
		return 1;
	}

	if (!DrawLevelOvr1P_ShouldWriteRenderedClippedRecord(projected, indices, count))
	{
		return 1;
	}

	record = (struct DrawLevelOvr1PClipRecord *)cursor;
	record->header = DrawLevelOvr1P_GetRenderedClipRecordHeader(block, count);
	record->otEntry = (u32)(uintptr_t)otEntry;
	record->tpage = DrawLevelOvr1P_Scratch()->uv.tpage;
	record->clut = DrawLevelOvr1P_Scratch()->uv.clut;

	for (int i = 0; i < count; i++)
	{
		DrawLevelOvr1P_CopyClipRecordVertex(&record->vertex[i], &projected[indices[i]]);
	}

	DrawLevelOvr1P_SetClipRecordCursor(cursor + recordSize);
	return 1;
}

static int Ovr226_800aa3f4_EmitQuad4x4RenderedGT3RawOrClip(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                           const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                                           const struct TextureLayout *texture, uint32_t *inheritedOtEntry, int secondary)
{
	int triIndices[3];

	if (inheritedOtEntry == NULL)
	{
		return 1;
	}

	if (secondary)
	{
		triIndices[0] = indices[1];
		triIndices[1] = indices[3];
		triIndices[2] = indices[2];
	}
	else
	{
		triIndices[0] = indices[0];
		triIndices[1] = indices[1];
		triIndices[2] = indices[2];
	}

	if (DrawLevelOvr1P_AreProjectedVerticesHalfNearSigned(projected, triIndices, 3))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNearSigned(projected, triIndices, 3))
	{
		return Ovr226_800aa5ac_WriteQuad4x4RenderedClippedRecordAtOtEntry(pb, block, projected, triIndices, 3, inheritedOtEntry);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedTriRawCodeAtOtEntry(pb, primMem, block, projected, triIndices, texture, inheritedOtEntry,
	                                                               DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
}

static int Ovr226_800aa4a8_EmitQuad4x4RenderedGT4RawOrClip(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                           const struct DrawLevelOvr1PScratchVertex *projected, const int *indices,
                                                           const struct TextureLayout *texture, uint32_t *inheritedOtEntry)
{
	if (inheritedOtEntry == NULL)
	{
		return 1;
	}

	if (DrawLevelOvr1P_AreProjectedVerticesHalfNear(projected, indices, 4))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		return Ovr226_800aa5ac_WriteQuad4x4RenderedClippedRecordAtOtEntry(pb, block, projected, indices, 4, inheritedOtEntry);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedQuadRawCodeAtOtEntry(pb, primMem, block, projected, indices, texture, inheritedOtEntry,
	                                                                DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
}

static int Ovr226_800aa3ec_DispatchQuad4x4RenderedDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                             const struct TextureLayout *texture, uint32_t *inheritedOtEntry)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	(void)faceIndex;

	switch (handlerAddress)
	{
	case 0x800aa3f4:
		return Ovr226_800aa3f4_EmitQuad4x4RenderedGT3RawOrClip(pb, primMem, block, projected, indices, texture, inheritedOtEntry, 0);
	case 0x800aa3ec:
		return Ovr226_800aa3f4_EmitQuad4x4RenderedGT3RawOrClip(pb, primMem, block, projected, indices, texture, inheritedOtEntry, 1);
	case 0x800aa4a8:
		return Ovr226_800aa4a8_EmitQuad4x4RenderedGT4RawOrClip(pb, primMem, block, projected, indices, texture, inheritedOtEntry);
	default:
		return 1;
	}
}

static void Ovr226_800aa334_PrepareQuad4x4RenderedDeepestUv(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	if (handlerAddress != 0)
	{
		DrawLevelOvr1P_PrepareDeepestMosaicUv(projected, indices, handlerAddress);
	}
}

static int Ovr226_800aa334_DispatchQuad4x4RenderedDeepest(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                          const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                          const struct TextureLayout *texture, uint32_t *inheritedOtEntry)
{
	int result;

	Ovr226_800aa334_PrepareQuad4x4RenderedDeepestUv(projected, indices);
	result = Ovr226_800aa3ec_DispatchQuad4x4RenderedDirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtEntry);

	DrawLevelOvr1P_RestoreProjectedUvScratch();
	return result;
}

static int Ovr226_800aa23c_Quad4x4RenderedNearOrDirect(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                       const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                       const struct TextureLayout *texture, int depth, u32 thresholdScratchOffset, uint32_t *inheritedOtEntry)
{
	u32 nearMask = DrawLevelOvr1P_GetProjectedNearMaskAtScratchOffset(projected, indices, thresholdScratchOffset);
	struct DrawLevelOvr1PScratchVertex *sub;
	u32 handlerAddress;
	int handlerSlot;

	if (nearMask == 0)
	{
		return Ovr226_800aa3ec_DispatchQuad4x4RenderedDirectTail(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtEntry);
	}

	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	Ovr226_800a9d70_BuildQuad4x4RenderedSubdivisionFrame(sub, projected, indices);

	handlerAddress = *CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_DIRECT_NEAR_HANDLER_TABLE_OFFSET + nearMask);
	handlerSlot = DrawLevelOvr1P_GetNearSubdivisionTableSlot(nearMask);
	DrawLevelOvr1P_SetPreviousRecursiveHandler(handlerAddress);

	return Ovr226_800a9b2c_DispatchQuad4x4RenderedHelperWrappers(pb, primMem, block, sub, faceIndex, texture, depth + 1, handlerAddress, handlerSlot,
	                                                             inheritedOtEntry);
}

static int Ovr226_800a9e7c_Quad4x4RenderedPreflight(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                    const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                    const struct TextureLayout *texture, int depth, u32 allowedMask, uint32_t *inheritedOtEntry)
{
	u32 directMask = 0;

	if (DrawLevelOvr1P_IsProjectedFaceFullyNear(projected, indices))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		directMask = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
		DrawLevelOvr1P_Scratch()->directMask = directMask;
	}
	else if (!Ovr226_800a3b90_SelectAndStoreGround4x1DirectMask(projected, indices, allowedMask, &directMask))
	{
		return 1;
	}

	if (DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
	{
		return Ovr226_800aa334_DispatchQuad4x4RenderedDeepest(pb, primMem, block, projected, indices, faceIndex, texture, inheritedOtEntry);
	}

	return Ovr226_800aa23c_Quad4x4RenderedNearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, depth, 0x28, inheritedOtEntry);
}

static int Ovr226_800a9b2c_DispatchQuad4x4RenderedHelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                                 struct DrawLevelOvr1PScratchVertex *projected, int faceIndex,
                                                                 const struct TextureLayout *texture, int depth, u32 handlerAddress, int handlerSlot,
                                                                 uint32_t *inheritedOtEntry)
{
	const u32 allowedQuad = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
	const u32 allowedTriPrimary = DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY;
	const u32 allowedTriSecondary = DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY;

	(void)handlerAddress;

	switch (handlerSlot)
	{
	case 0:
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[0], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[1], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	case 1:
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	case 2:
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                              allowedTriPrimary, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	case 3:
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	case 4:
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                              allowedTriPrimary, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	case 7:
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[2], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[3], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	case 9:
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth,
		                                              allowedTriSecondary, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	case 11:
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, texture, depth,
		                                              allowedQuad, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth,
		                                              allowedTriSecondary, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, allowedQuad,
		                                              inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth,
		                                                allowedQuad, inheritedOtEntry);
	default:
		break;
	}

	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x0);
	if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, texture, depth, allowedQuad,
	                                              inheritedOtEntry))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0xc);
	if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, texture, depth, allowedQuad,
	                                              inheritedOtEntry))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x30);
	if (!Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, texture, depth, allowedQuad,
	                                              inheritedOtEntry))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0x3c);
	return Ovr226_800a9e7c_Quad4x4RenderedPreflight(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, texture, depth, allowedQuad,
	                                                inheritedOtEntry);
}

static int Ovr226_800a9fa0_Quad4x4RenderedSelectorNearGate(struct PushBuffer *pb, struct PrimMem *primMem, struct DrawLevelOvr1PScratchVertex *projected,
                                                           struct QuadBlock *block, int faceIndex)
{
	int indices[4];
	u32 tableWord;
	u32 directMask = 0;
	u32 maxDepth;
	uint32_t *inheritedOtEntry;
	struct TextureLayout *texture;

	tableWord = DrawLevelOvr1P_Select4x1ProjectedFace(projected, block, faceIndex, indices);
	if (DrawLevelOvr1P_IsProjectedFaceFullyNear(projected, indices))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		directMask = DRAW_LEVEL_OVR1P_DIRECT_QUAD;
		DrawLevelOvr1P_Scratch()->directMask = directMask;
	}
	else if (!Ovr226_800a3c70_SelectAndStoreGround4x1SelectorDirectMask(projected, indices, tableWord, &directMask))
	{
		return 1;
	}

	Ovr226_800aa0fc_StoreQuad4x4RenderedClipHeader(tableWord);
	maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices);
	inheritedOtEntry = Ovr226_800aa124_ResolveQuad4x4RenderedOtEntry(pb, block, projected, maxDepth, faceIndex);
	texture = Ovr226_800aa18c_SelectQuad4x4RenderedTexture(block, projected, maxDepth);
	if (texture != NULL)
	{
		Ovr226_800aa1d0_WriteQuad4x4RenderedUv(projected, indices, texture, tableWord);
	}

	return Ovr226_800aa23c_Quad4x4RenderedNearOrDirect(pb, primMem, block, projected, indices, faceIndex, texture, 0, 0x24, inheritedOtEntry);
}

static int DrawLevelOvr1P_Emit4x1ListSelectedFace(struct PushBuffer *pb, struct PrimMem *primMem, struct DrawLevelOvr1PScratchVertex *projected,
                                                  struct QuadBlock *block, int faceIndex)
{
	// NOTE(aalhendi): Retail 0x800a3c70 owns selector decode, table-word
	// storage, offscreen/direct-mask preflight, texture/UV selection, and
	// near-mask dispatch for each 4x1 list face.
	return Ovr226_800a3c70_Ground4x1SelectorNearGate(pb, primMem, projected, block, faceIndex);
}

static void DrawLevelOvr1P_SetRenderedListCursor(struct QuadBlock **renderedList)
{
	DrawLevelOvr1P_Scratch()->renderedOverflowPtr32 = (u32)(uintptr_t)renderedList;
}

static void DrawLevelOvr1P_SetRenderedOverflowBase(struct QuadBlock **renderedList)
{
	sDrawLevelOvr1P_RenderedOverflowBase = renderedList;
}

static struct QuadBlock **DrawLevelOvr1P_GetRenderedOverflowBase(void)
{
	if (sDrawLevelOvr1P_RenderedOverflowBase != NULL)
	{
		return sDrawLevelOvr1P_RenderedOverflowBase;
	}

	return sdata_static.quadBlocksRendered;
}

static struct QuadBlock **DrawLevelOvr1P_GetRenderedListCursor(void)
{
	return (struct QuadBlock **)(uintptr_t)DrawLevelOvr1P_Scratch()->renderedOverflowPtr32;
}

static void DrawLevelOvr1P_AppendRenderedQuadBlock(struct QuadBlock *block)
{
	struct QuadBlock **renderedList = DrawLevelOvr1P_GetRenderedListCursor();

	if (renderedList == NULL)
	{
		return;
	}

	*renderedList = block;
	DrawLevelOvr1P_SetRenderedListCursor(renderedList + 1);
}

static void DrawLevelOvr1P_TerminateRenderedListCursor(void)
{
	struct QuadBlock **renderedList = DrawLevelOvr1P_GetRenderedListCursor();

	if (renderedList != NULL)
	{
		*renderedList = NULL;
	}
}

static int DrawLevelOvr1P_DrawWaterListQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh, struct QuadBlock *block,
                                                 int captureRenderedOverflow);

static void DrawLevelOvr1P_ClearProjectedUv(struct DrawLevelOvr1PScratchVertex *projected, const int *indices)
{
	DrawLevelOvr1P_Scratch()->uv.uv0 = 0;
	DrawLevelOvr1P_Scratch()->uv.uv1 = 0;
	DrawLevelOvr1P_Scratch()->uv.uv2 = 0;
	DrawLevelOvr1P_Scratch()->uv.savedUv0 = 0;
	DrawLevelOvr1P_Scratch()->uv.savedUv1 = 0;

	for (int i = 0; i < 4; i++)
	{
		projected[indices[i]].flags = 0;
	}
}

static struct TextureLayout *Ovr226_800a1058_PrepareFullDynamicLowUv(struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected)
{
	const int *indices = sDrawLevelOvr1PFullDynamicLowIndices;
	struct TextureLayout *texture = block->ptr_texture_low;
	u32 uv0;
	u32 uv1;
	u32 uv2;

	// NOTE(aalhendi): Retail full-dynamic 0x800a0ef4 seeds low-LOD UVs before
	// choosing either the direct low quad or the near/transition helper table.
	DrawLevelOvr1P_Scratch()->selected4x1TableWord = 0;

	uv0 = DrawLevelOvr1P_ReadWord(texture, 0);
	uv1 = DrawLevelOvr1P_ReadWord(texture, 4);
	uv2 = DrawLevelOvr1P_ReadWord(texture, 8);
	DrawLevelOvr1P_Scratch()->uv.uv0 = uv0;
	DrawLevelOvr1P_Scratch()->uv.uv1 = uv1;
	DrawLevelOvr1P_Scratch()->uv.uv2 = uv2;
	projected[indices[0]].flags = (u16)uv0;
	projected[indices[1]].flags = (u16)uv1;
	projected[indices[2]].flags = (u16)uv2;
	projected[indices[3]].flags = (u16)(uv2 >> 16);

	return texture;
}

static int DrawLevelOvr1P_EmitFullDynamicLowQuad(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                 struct DrawLevelOvr1PScratchVertex *projected)
{
	const int *indices = sDrawLevelOvr1PFullDynamicLowIndices;
	struct TextureLayout *texture = Ovr226_800a1058_PrepareFullDynamicLowUv(block, projected);

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

static int Ovr226_800a1534_800a17d8_DispatchFullDynamicHelperSequence(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                                      struct DrawLevelOvr1PScratchVertex *projected, u32 handlerAddress,
                                                                      struct TextureLayout *texture, int depth);

static int Ovr226_800a1ce8_EmitFullDynamicGT3Raw(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                 struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                 struct TextureLayout *texture, int secondary)
{
	int triIndices[3];

	if (secondary)
	{
		triIndices[0] = indices[1];
		triIndices[1] = indices[3];
		triIndices[2] = indices[2];
	}
	else
	{
		triIndices[0] = indices[0];
		triIndices[1] = indices[1];
		triIndices[2] = indices[2];
	}

	return DrawLevelOvr1P_EmitPreparedProjectedTriRawCodeAtOt(pb, primMem, block, projected, triIndices, faceIndex, texture, 0,
	                                                          sOvr226_800a1cc4_InheritedOtIndex, DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
}

static int Ovr226_800a1d6c_EmitFullDynamicGT4Raw(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                 struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                 struct TextureLayout *texture)
{
	return DrawLevelOvr1P_EmitPreparedProjectedQuadRawCodeAtOt(pb, primMem, block, projected, indices, faceIndex, texture, 0, sOvr226_800a1cc4_InheritedOtIndex,
	                                                           DRAW_LEVEL_OVR1P_PRIM_CODE_AUTO);
}

static int Ovr226_800a1cc4_EmitFullDynamicDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                     struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                     struct TextureLayout *texture)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	switch (handlerAddress)
	{
	case 0x800a1ce8:
	case 0x800a3670:
	case 0x800a3724:
		return Ovr226_800a1ce8_EmitFullDynamicGT3Raw(pb, primMem, block, projected, indices, faceIndex, texture, 0);
	case 0x800a1ce0:
	case 0x800a3668:
	case 0x800a371c:
		return Ovr226_800a1ce8_EmitFullDynamicGT3Raw(pb, primMem, block, projected, indices, faceIndex, texture, 1);
	case 0x800a1d6c:
	case 0x800a36f4:
	case 0x800a37a8:
		return Ovr226_800a1d6c_EmitFullDynamicGT4Raw(pb, primMem, block, projected, indices, faceIndex, texture);
	default:
		return 1;
	}
}

static int Ovr226_800a1cc4_IsSplitGroundListADirectHandler(u32 handlerAddress)
{
	switch (handlerAddress)
	{
	case 0x800a3670:
	case 0x800a3668:
	case 0x800a36f4:
	case 0x800a3724:
	case 0x800a371c:
	case 0x800a37a8:
		return 1;
	default:
		return 0;
	}
}

static int Ovr226_800a1cc4_EmitFullDynamicDeepestDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                            struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                            struct TextureLayout *texture)
{
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(DrawLevelOvr1P_Scratch()->directMask);
	int result;

	if (!Ovr226_800a1cc4_IsSplitGroundListADirectHandler(handlerAddress))
	{
		return Ovr226_800a1cc4_EmitFullDynamicDirectTail(pb, primMem, block, projected, indices, faceIndex, texture);
	}

	DrawLevelOvr1P_PrepareDeepestMosaicUv(projected, indices, handlerAddress);
	result = Ovr226_800a1cc4_EmitFullDynamicDirectTail(pb, primMem, block, projected, indices, faceIndex, texture);
	DrawLevelOvr1P_RestoreProjectedUvScratch();
	return result;
}

static u32 Ovr226_800a1be8_SelectFullDynamicNearMask(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, u32 scratchOffset)
{
	return DrawLevelOvr1P_GetProjectedNearMaskAtScratchOffset(projected, indices, scratchOffset);
}

static int Ovr226_800a1be8_DispatchFullDynamicNearSubdivision(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                              struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                              struct TextureLayout *texture, int depth, u32 directMask, u32 thresholdScratchOffset)
{
	struct DrawLevelOvr1PScratchVertex *sub;
	u32 handlerAddress;
	u32 nearMask;

	(void)directMask;
	nearMask = Ovr226_800a1be8_SelectFullDynamicNearMask(projected, indices, thresholdScratchOffset);
	if (nearMask == 0)
	{
		return Ovr226_800a1cc4_EmitFullDynamicDirectTail(pb, primMem, block, projected, indices, faceIndex, texture);
	}

	if (depth >= DRAW_LEVEL_OVR1P_MAX_NEAR_SUBDIV_DEPTH)
	{
		return Ovr226_800a1cc4_EmitFullDynamicDirectTail(pb, primMem, block, projected, indices, faceIndex, texture);
	}

	handlerAddress = DrawLevelOvr1P_GetNearSubdivisionHandlerAddress(nearMask, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST);
	DrawLevelOvr1P_SetPreviousRecursiveHandler(handlerAddress);

	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	Ovr226_800a17d8_BuildFullDynamicSubdivisionFrame(sub, projected, indices);

	return Ovr226_800a1534_800a17d8_DispatchFullDynamicHelperSequence(pb, primMem, block, sub, handlerAddress, texture, depth + 1);
}

static int DrawLevelOvr1P_EmitFullDynamicTerminalFaceSlotMode(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                              struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                              struct TextureLayout *texture, int depth, u32 allowedMask, int updateSlot)
{
	struct DrawLevelOvr1PFullDynamicRecursiveGate gate;

	if (updateSlot)
	{
		DrawLevelOvr1P_SetGridFaceSlot(projected, faceIndex);
	}

	// NOTE(aalhendi): Retail 0x800a18c0..0x800a19a8 owns the full-dynamic
	// packed-SXY cull, direct-mask fold, max-recursion direct handoff, and
	// recursive scratch 0x28 near-threshold selection.
	if (!Ovr226_800a18c0_FullDynamicRecursiveGate(projected, indices, allowedMask, &gate))
	{
		return 1;
	}

	if (gate.forceDirect)
	{
		return Ovr226_800a1cc4_EmitFullDynamicDeepestDirectTail(pb, primMem, block, projected, indices, faceIndex, texture);
	}

	return Ovr226_800a1be8_DispatchFullDynamicNearSubdivision(pb, primMem, block, projected, indices, faceIndex, texture, depth, gate.directMask, 0x28);
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

static int Ovr226_800a19a8_SelectFullDynamicDirectSlot(struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, int *indices,
                                                       struct TextureLayout **texture, u32 *directMask)
{
	u32 maxDepth;
	u32 tableWord;

	tableWord = DrawLevelOvr1P_Select4x1ProjectedFace(projected, block, faceIndex, indices);

	if (DrawLevelOvr1P_IsProjectedPolyOffscreenPacked(projected, indices, 4))
	{
		return 0;
	}

	*directMask = DrawLevelOvr1P_SelectDirectMask(block, projected, indices, tableWord, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	DrawLevelOvr1P_Scratch()->directMask = *directMask;
	if (*directMask == 0)
	{
		return 0;
	}

	maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, indices);
	Ovr226_800a108c_800a1b10_SetFullDynamicInheritedOtIndex(block, projected, maxDepth, faceIndex);
	*texture = DrawLevelOvr1P_GetProjectedMidTexture(block, projected, faceIndex, maxDepth);

	if (*texture != NULL)
	{
		DrawLevelOvr1P_WriteProjectedUv(projected, indices, *texture, tableWord);
	}

	return 1;
}

static int DrawLevelOvr1P_EmitFullDynamicSelectorFace(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                      struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, int depth)
{
	struct TextureLayout *texture = NULL;
	int indices[4];
	u32 directMask;

	// NOTE(aalhendi): Retail 0x800a19a8..0x800a1be8 selects the top-level
	// full-dynamic face slot, rejects empty direct masks before texture/UV
	// selection, and uses scratch 0x24 for the first near-mask handoff.
	if (!Ovr226_800a19a8_SelectFullDynamicDirectSlot(block, projected, faceIndex, indices, &texture, &directMask))
	{
		return 1;
	}

	return Ovr226_800a1be8_DispatchFullDynamicNearSubdivision(pb, primMem, block, projected, indices, faceIndex, texture, depth, directMask, 0x24);
}

static int Ovr226_800a1534_EmitFullDynamicFace0(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth, u32 allowedMask)
{
	return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], 0, texture, depth,
	                                                              allowedMask);
}

static int Ovr226_800a1548_EmitFullDynamicFace1(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth, u32 allowedMask)
{
	return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], 1, texture, depth,
	                                                              allowedMask);
}

static int Ovr226_800a155c_EmitFullDynamicFace2(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth, u32 allowedMask)
{
	return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], 2, texture, depth,
	                                                              allowedMask);
}

static int Ovr226_800a1570_EmitFullDynamicFace3(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth, u32 allowedMask)
{
	return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], 3, texture, depth,
	                                                              allowedMask);
}

static int Ovr226_800a1584_EmitFullDynamicExtraFace0(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                     struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth, u32 allowedMask)
{
	return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], 0, texture, depth,
	                                                              allowedMask);
}

static int Ovr226_800a1598_EmitFullDynamicExtraFace1(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                     struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth, u32 allowedMask)
{
	return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], 1, texture, depth,
	                                                              allowedMask);
}

static int Ovr226_800a15ac_EmitFullDynamicExtraFace2(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                     struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth, u32 allowedMask)
{
	return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], 2, texture, depth,
	                                                              allowedMask);
}

static int Ovr226_800a15c0_EmitFullDynamicExtraFace3(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                     struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth, u32 allowedMask)
{
	return DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], 3, texture, depth,
	                                                              allowedMask);
}

static int Ovr226_800a15d4_FullDynamicHelperSlot0(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                  struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth)
{
	if (!Ovr226_800a1548_EmitFullDynamicFace1(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	if (!Ovr226_800a155c_EmitFullDynamicFace2(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	return Ovr226_800a1534_EmitFullDynamicFace0(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
}

static int Ovr226_800a1614_FullDynamicHelperSlot1(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                  struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth)
{
	if (!Ovr226_800a1548_EmitFullDynamicFace1(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	if (!Ovr226_800a15ac_EmitFullDynamicExtraFace2(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	return Ovr226_800a15c0_EmitFullDynamicExtraFace3(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
}

static int Ovr226_800a1634_FullDynamicHelperSlot3(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                  struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth)
{
	if (!Ovr226_800a1584_EmitFullDynamicExtraFace0(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	if (!Ovr226_800a155c_EmitFullDynamicFace2(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	return Ovr226_800a1598_EmitFullDynamicExtraFace1(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
}

static int Ovr226_800a1654_FullDynamicHelperSlot7(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                  struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth)
{
	if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[2], 0, texture, depth,
	                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	if (!DrawLevelOvr1P_EmitFullDynamicTerminalFacePreserveSlot(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[3], 0, texture, depth,
	                                                            DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	return Ovr226_800a1570_EmitFullDynamicFace3(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
}

static int Ovr226_800a1694_FullDynamicHelperSlot2(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                  struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth)
{
	if (!Ovr226_800a15c0_EmitFullDynamicExtraFace3(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	if (!Ovr226_800a155c_EmitFullDynamicFace2(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY))
	{
		return 0;
	}
	if (!Ovr226_800a1534_EmitFullDynamicFace0(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	return Ovr226_800a1548_EmitFullDynamicFace1(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
}

static int Ovr226_800a16bc_FullDynamicHelperSlot4(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                  struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth)
{
	if (!Ovr226_800a1598_EmitFullDynamicExtraFace1(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	if (!Ovr226_800a1548_EmitFullDynamicFace1(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY))
	{
		return 0;
	}
	if (!Ovr226_800a1534_EmitFullDynamicFace0(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	return Ovr226_800a155c_EmitFullDynamicFace2(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
}

static int Ovr226_800a16e4_FullDynamicHelperSlot9(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                  struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth)
{
	if (!Ovr226_800a15ac_EmitFullDynamicExtraFace2(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	if (!Ovr226_800a155c_EmitFullDynamicFace2(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY))
	{
		return 0;
	}
	if (!Ovr226_800a1548_EmitFullDynamicFace1(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	return Ovr226_800a1570_EmitFullDynamicFace3(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
}

static int Ovr226_800a170c_FullDynamicHelperSlot11(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                   struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth)
{
	if (!Ovr226_800a1584_EmitFullDynamicExtraFace0(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	if (!Ovr226_800a1548_EmitFullDynamicFace1(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY))
	{
		return 0;
	}
	if (!Ovr226_800a155c_EmitFullDynamicFace2(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	return Ovr226_800a1570_EmitFullDynamicFace3(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
}

static int Ovr226_800a1734_FullDynamicDefaultHelper(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                    struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture, int depth)
{
	if (projected == CTR_SCRATCHPAD_PTR(struct DrawLevelOvr1PScratchVertex, DRAW_LEVEL_OVR1P_PROJECTED_FRAME0_OFFSET))
	{
		for (int faceIndex = 0; faceIndex < 4; faceIndex++)
		{
			if (!DrawLevelOvr1P_EmitFullDynamicSelectorFace(pb, primMem, block, projected, faceIndex, depth))
			{
				return 0;
			}
		}

		return 1;
	}

	DrawLevelOvr1P_SetGridFaceSlot(projected, 0);
	if (!Ovr226_800a1534_EmitFullDynamicFace0(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlot(projected, 1);
	if (!Ovr226_800a1548_EmitFullDynamicFace1(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlot(projected, 2);
	if (!Ovr226_800a155c_EmitFullDynamicFace2(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlot(projected, 3);
	return Ovr226_800a1570_EmitFullDynamicFace3(pb, primMem, block, projected, texture, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
}

static int Ovr226_800a1534_800a17d8_DispatchFullDynamicHelperSequence(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                                      struct DrawLevelOvr1PScratchVertex *projected, u32 handlerAddress,
                                                                      struct TextureLayout *texture, int depth)
{
	switch (handlerAddress)
	{
	case 0x800a15d4:
	case 0x800a2eb4:
	case 0x800a2f68:
		return Ovr226_800a15d4_FullDynamicHelperSlot0(pb, primMem, block, projected, texture, depth);
	case 0x800a1614:
	case 0x800a2ef4:
	case 0x800a2fa8:
		return Ovr226_800a1614_FullDynamicHelperSlot1(pb, primMem, block, projected, texture, depth);
	case 0x800a1634:
	case 0x800a2f14:
	case 0x800a2fc8:
		return Ovr226_800a1634_FullDynamicHelperSlot3(pb, primMem, block, projected, texture, depth);
	case 0x800a1654:
	case 0x800a2f34:
	case 0x800a2fe8:
		return Ovr226_800a1654_FullDynamicHelperSlot7(pb, primMem, block, projected, texture, depth);
	case 0x800a1694:
	case 0x800a2f74:
	case 0x800a3028:
		return Ovr226_800a1694_FullDynamicHelperSlot2(pb, primMem, block, projected, texture, depth);
	case 0x800a16bc:
	case 0x800a2f9c:
	case 0x800a3050:
		return Ovr226_800a16bc_FullDynamicHelperSlot4(pb, primMem, block, projected, texture, depth);
	case 0x800a16e4:
	case 0x800a2fc4:
	case 0x800a3078:
		return Ovr226_800a16e4_FullDynamicHelperSlot9(pb, primMem, block, projected, texture, depth);
	case 0x800a170c:
	case 0x800a2fec:
	case 0x800a30a0:
		return Ovr226_800a170c_FullDynamicHelperSlot11(pb, primMem, block, projected, texture, depth);
	case 0x800a1734:
	case 0x800a3014:
	case 0x800a30c8:
		return Ovr226_800a1734_FullDynamicDefaultHelper(pb, primMem, block, projected, texture, depth);
	default:
		return 1;
	}
}

static u32 Ovr226_800a10dc_SelectFullDynamicTopNearMask(const struct DrawLevelOvr1PScratchVertex *projected)
{
	return DrawLevelOvr1P_GetProjectedNearMaskAtScratchOffset(projected, sDrawLevelOvr1PFullDynamicLowIndices, 0x14);
}

static int Ovr226_800a1338_DispatchFullDynamicLowDirect(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                        struct DrawLevelOvr1PScratchVertex *projected, struct TextureLayout *texture)
{
	const int *indices = sDrawLevelOvr1PFullDynamicLowIndices;
	u32 directMask;

	if (DrawLevelOvr1P_IsProjectedPolyOffscreenPacked(projected, indices, 4))
	{
		return 1;
	}

	directMask = DrawLevelOvr1P_SelectDirectMask(block, projected, indices, 0, DRAW_LEVEL_OVR1P_DIRECT_QUAD);
	DrawLevelOvr1P_Scratch()->directMask = directMask;
	if (directMask == 0)
	{
		return 1;
	}

	return Ovr226_800a1cc4_EmitFullDynamicDirectTail(pb, primMem, block, projected, indices, -1, texture);
}

static void Ovr226_800a11e0_ProjectFullDynamicLastPair(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected)
{
	struct LevVertex *vertex7 = &vertices[block->index[7]];
	struct LevVertex *vertex8 = &vertices[block->index[8]];
	s16 unusedSxy[2];
	u32 depth7;
	u32 depth8;
	u32 unusedDepth;

	DrawLevelOvr1P_CopySourceVertex(&projected[7], vertex7);
	DrawLevelOvr1P_CopySourceVertex(&projected[8], vertex8);

	// NOTE(aalhendi): Retail 0x800a11e0 uses ldv01, preserving V2 from vertex 6.
	MTC2(DrawLevelOvr1P_ReadPackedWord(&vertex7->pos.x), 0);
	MTC2(DrawLevelOvr1P_ReadPackedWord(&vertex7->pos.z), 1);
	MTC2(DrawLevelOvr1P_ReadPackedWord(&vertex8->pos.x), 2);
	MTC2(DrawLevelOvr1P_ReadPackedWord(&vertex8->pos.z), 3);
	gte_rtpt();
	CTR_GteStoreSXY3(&projected[7].posScreen[0], &projected[8].posScreen[0], &unusedSxy[0]);
	gte_stsz3(&depth7, &depth8, &unusedDepth);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[7], depth7);
	DrawLevelOvr1P_StoreProjectedDepthWord(&projected[8], depth8);
}

static void Ovr226_800a1128_ProjectFullDynamicHighGrid(struct LevVertex *vertices, const struct QuadBlock *block, struct DrawLevelOvr1PScratchVertex *projected)
{
	Ovr226_800a0f78_ProjectVertexTripleFullDepth(vertices, block, projected, 4, 5, 6);
	Ovr226_800a11e0_ProjectFullDynamicLastPair(vertices, block, projected);
}

static int Ovr226_800a1128_DispatchFullDynamicTransitionGrid(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                             struct LevVertex *vertices, struct DrawLevelOvr1PScratchVertex *projected, u32 nearMask,
                                                             struct TextureLayout *texture)
{
	Ovr226_800a1128_ProjectFullDynamicHighGrid(vertices, block, projected);
	DrawLevelOvr1P_AdjustFullDynamicMidVertices(projected, vertices, block);
	DrawLevelOvr1P_AdjustFullDynamicMidUvs(projected);

	return Ovr226_800a1534_800a17d8_DispatchFullDynamicHelperSequence(
	    pb, primMem, block, projected, DrawLevelOvr1P_GetNearSubdivisionHandlerAddress(nearMask, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST), texture, 0);
}

static int Ovr226_800a0f78_EmitFullDynamicQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block, struct LevVertex *vertices)
{
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();
	struct TextureLayout *texture;
	u32 nearMask;
	u32 maxDepth;

	Ovr226_800a0f78_ProjectFullDynamicLowQuad(vertices, block, projected);
	texture = Ovr226_800a1058_PrepareFullDynamicLowUv(block, projected);
	maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, sDrawLevelOvr1PFullDynamicLowIndices);
	Ovr226_800a108c_800a1b10_SetFullDynamicInheritedOtIndex(block, projected, maxDepth, -1);
	nearMask = Ovr226_800a10dc_SelectFullDynamicTopNearMask(projected);

	if (nearMask == 0)
	{
		return Ovr226_800a1338_DispatchFullDynamicLowDirect(pb, primMem, block, projected, texture);
	}

	return Ovr226_800a1128_DispatchFullDynamicTransitionGrid(pb, primMem, block, vertices, projected, nearMask, texture);
}

static int DrawLevelOvr1P_EmitFullDynamicTransitionQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                             struct LevVertex *vertices, struct DrawLevelOvr1PScratchVertex *projected, u32 nearMask)
{
	struct TextureLayout *texture = Ovr226_800a1058_PrepareFullDynamicLowUv(block, projected);

	DrawLevelOvr1P_AdjustFullDynamicMidVertices(projected, vertices, block);
	DrawLevelOvr1P_AdjustFullDynamicMidUvs(projected);

	// NOTE(aalhendi): Retail jumps through the copied 0x14c helper table after
	// the transition midpoint/UV prep; native dispatches by that handler word.
	return Ovr226_800a1534_800a17d8_DispatchFullDynamicHelperSequence(
	    pb, primMem, block, projected, DrawLevelOvr1P_GetNearSubdivisionHandlerAddress(nearMask, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST), texture, 0);
}

static int DrawLevelOvr1P_EmitFullDynamicQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block, struct LevVertex *vertices,
                                                   struct DrawLevelOvr1PScratchVertex *projected)
{
	u32 nearMask = DrawLevelOvr1P_GetProjectedNearMask(projected, sDrawLevelOvr1PFullDynamicLowIndices);
	u32 maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, sDrawLevelOvr1PFullDynamicLowIndices);

	Ovr226_800a108c_800a1b10_SetFullDynamicInheritedOtIndex(block, projected, maxDepth, -1);

	if (nearMask != 0)
	{
		// NOTE(aalhendi): The high/mid transition records are only needed once
		// the low quad takes the full-dynamic near path.
		(void)DrawLevelOvr1P_ProjectQuadBlockGrid(vertices, block, projected, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST);
		return DrawLevelOvr1P_EmitFullDynamicTransitionQuadBlock(pb, primMem, block, vertices, projected, nearMask);
	}

	return DrawLevelOvr1P_EmitFullDynamicLowQuad(pb, primMem, block, projected);
}

static void Ovr226_800a0f0c_SeedFullDynamicVisibilityScratch(const int *visFaceList, const struct QuadBlock *block)
{
	u32 blockID = (u16)block->blockID;
	const u32 *word = (const u32 *)((const u8 *)visFaceList + ((blockID >> 3) & 0x1fc));

	DrawLevelOvr1P_Scratch()->visibilityWordPtr32 = (u32)(uintptr_t)word;
	DrawLevelOvr1P_Scratch()->visibilityBitIndex = blockID & 0x1f;
	DrawLevelOvr1P_Scratch()->visibilityWord = *word;
}

static int Ovr226_800a0f34_ConsumeFullDynamicVisibilityBit(void)
{
	s32 bitIndex = DrawLevelOvr1P_Scratch()->visibilityBitIndex;
	u32 word = DrawLevelOvr1P_Scratch()->visibilityWord;
	u32 shifted;

	if (bitIndex < 0)
	{
		u32 *wordPtr = (u32 *)(uintptr_t)DrawLevelOvr1P_Scratch()->visibilityWordPtr32;

		bitIndex = 0x1f;
		word = wordPtr[1];
		wordPtr++;
		DrawLevelOvr1P_Scratch()->visibilityWordPtr32 = (u32)(uintptr_t)wordPtr;
		DrawLevelOvr1P_Scratch()->visibilityWord = word;
	}

	shifted = word << bitIndex;
	DrawLevelOvr1P_Scratch()->visibilityBitIndex = bitIndex - 1;
	return (s32)shifted < 0;
}

static int Ovr226_800a0ef4_DrawFullDynamicBspList(struct VisMemBspListNode *slot, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                                  const int *visFaceList)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;

	while (slot != NULL)
	{
		struct BSP *bsp = slot->bsp;
		struct QuadBlock *block = bsp->data.leaf.ptrQuadBlockArray;
		s32 quadCount = bsp->data.leaf.numQuads;

		DrawLevelOvr1P_Scratch()->quadCount = quadCount;
		if (quadCount > 0)
		{
			Ovr226_800a0f0c_SeedFullDynamicVisibilityScratch(visFaceList, block);
		}

		while (quadCount > 0)
		{
			if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, DRAW_LEVEL_OVR1P_BUCKET_RESERVE_FULL_DYNAMIC))
			{
				return 0;
			}

			if (Ovr226_800a0f34_ConsumeFullDynamicVisibilityBit())
			{
				if (!Ovr226_800a0f78_EmitFullDynamicQuadBlock(pb, primMem, block, vertices))
				{
					return 0;
				}
			}

			block++;
			quadCount--;
			DrawLevelOvr1P_Scratch()->quadCount = quadCount;
		}

		slot = slot->next;
	}

	return 1;
}

static void DrawLevelOvr1P_SetSplitGroundThresholdScratch(void)
{
	DrawLevelOvr1P_RenderScratch()->depthScale = 0x780;
	DrawLevelOvr1P_RenderScratch()->textureLodDepthThreshold0 = 0x640;
	DrawLevelOvr1P_RenderScratch()->textureLodDepthThreshold1 = 0x500;
	DrawLevelOvr1P_RenderScratch()->topLevelNearDepthThreshold = 0x280;
	DrawLevelOvr1P_RenderScratch()->recursiveNearDepthThreshold = 0x140;
}

static int DrawLevelOvr1P_ProjectSplitGroundListALowGrid(struct LevVertex *vertices, const struct QuadBlock *block,
                                                         struct DrawLevelOvr1PScratchVertex *projected)
{
	DrawLevelOvr1P_SetGridFaceSlot(projected, 0);
	if (Ovr226_800a3738_ProjectGround4x1ListVertexTriple(vertices, block, projected, 0, 1, 2))
	{
		return 1;
	}

	DrawLevelOvr1P_SetActiveDrawOrderLow(block);
	return Ovr226_800a3738_ProjectGround4x1ListVertexTriple(vertices, block, projected, 3, 4, 5);
}

static int DrawLevelOvr1P_ProjectSplitGroundListATransitionGrid(struct LevVertex *vertices, const struct QuadBlock *block,
                                                                struct DrawLevelOvr1PScratchVertex *projected)
{
	if (Ovr226_800a3738_ProjectGround4x1ListVertexTriple(vertices, block, projected, 6, 7, 8))
	{
		return 1;
	}

	DrawLevelOvr1P_AdjustFullDynamicMidUvs(projected);
	return 0;
}

static int DrawLevelOvr1P_EmitSplitGroundListAQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh, struct QuadBlock *block)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();
	struct TextureLayout *texture;
	u32 maxDepth;
	u32 nearMask;

	if (DrawLevelOvr1P_ProjectSplitGroundListALowGrid(vertices, block, projected))
	{
		DrawLevelOvr1P_AppendRenderedQuadBlock(block);
		return 1;
	}

	texture = Ovr226_800a1058_PrepareFullDynamicLowUv(block, projected);
	maxDepth = DrawLevelOvr1P_GetProjectedMaxDepth(projected, sDrawLevelOvr1PFullDynamicLowIndices);
	Ovr226_800a108c_800a1b10_SetFullDynamicInheritedOtIndex(block, projected, maxDepth, -1);
	nearMask = Ovr226_800a10dc_SelectFullDynamicTopNearMask(projected);
	if (nearMask == 0)
	{
		return Ovr226_800a1338_DispatchFullDynamicLowDirect(pb, primMem, block, projected, texture);
	}

	if (DrawLevelOvr1P_ProjectSplitGroundListATransitionGrid(vertices, block, projected))
	{
		DrawLevelOvr1P_AppendRenderedQuadBlock(block);
		return 1;
	}

	return Ovr226_800a1534_800a17d8_DispatchFullDynamicHelperSequence(
	    pb, primMem, block, projected, DrawLevelOvr1P_GetNearSubdivisionHandlerAddress(nearMask, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST), texture, 0);
}

static int DrawLevelOvr1P_DrawSplitGroundListABspList(struct VisMemBspListNode *slot, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                                      const int *visFaceList)
{
	DrawLevelOvr1P_SetSplitGroundThresholdScratch();

	while (slot != NULL)
	{
		struct BSP *bsp = slot->bsp;
		struct QuadBlock *block = bsp->data.leaf.ptrQuadBlockArray;
		s32 quadCount = bsp->data.leaf.numQuads;

		DrawLevelOvr1P_Scratch()->quadCount = quadCount;
		Ovr226_800a0f0c_SeedFullDynamicVisibilityScratch(visFaceList, block);

		while (quadCount > 0)
		{
			if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, DRAW_LEVEL_OVR1P_BUCKET_RESERVE_DEFAULT))
			{
				return 0;
			}

			if (Ovr226_800a0f34_ConsumeFullDynamicVisibilityBit())
			{
				if (!DrawLevelOvr1P_EmitSplitGroundListAQuadBlock(pb, primMem, mesh, block))
				{
					return 0;
				}
			}

			block++;
			quadCount--;
			DrawLevelOvr1P_Scratch()->quadCount = quadCount;
		}

		slot = slot->next;
	}

	DrawLevelOvr1P_TerminateRenderedListCursor();
	return 1;
}

static int Ovr226_800a3738_EmitGround4x1ListQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh, struct QuadBlock *block)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	if (Ovr226_800a3738_ProjectGround4x1ListGrid(vertices, block, projected))
	{
		DrawLevelOvr1P_AppendRenderedQuadBlock(block);
		return 1;
	}

	for (int faceIndex = 0; faceIndex < 4; faceIndex++)
	{
		if (!DrawLevelOvr1P_Emit4x1ListSelectedFace(pb, primMem, projected, block, faceIndex))
		{
			return 0;
		}
	}

	return 1;
}

static int Ovr226_800a36a8_DrawGround4x1BspList(struct VisMemBspListNode *slot, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                                const int *visFaceList)
{
	if (sDrawLevelOvr1P_ListHandlersSeedRenderedCursor)
	{
		DrawLevelOvr1P_SetRenderedListCursor(DrawLevelOvr1P_GetRenderedOverflowBase());
	}

	while (slot != NULL)
	{
		struct BSP *bsp = slot->bsp;
		struct QuadBlock *block = bsp->data.leaf.ptrQuadBlockArray;
		s32 quadCount = bsp->data.leaf.numQuads;

		DrawLevelOvr1P_Scratch()->quadCount = quadCount;
		Ovr226_800a0f0c_SeedFullDynamicVisibilityScratch(visFaceList, block);

		while (quadCount > 0)
		{
			if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, DRAW_LEVEL_OVR1P_BUCKET_RESERVE_4X1))
			{
				return 0;
			}

			if (Ovr226_800a0f34_ConsumeFullDynamicVisibilityBit())
			{
				if (!Ovr226_800a3738_EmitGround4x1ListQuadBlock(pb, primMem, mesh, block))
				{
					return 0;
				}
			}

			block++;
			quadCount--;
			DrawLevelOvr1P_Scratch()->quadCount = quadCount;
		}

		slot = slot->next;
	}

	DrawLevelOvr1P_TerminateRenderedListCursor();
	return 1;
}

static int Ovr226_800a417c_DrawGround4x1RenderedList(struct QuadBlock **renderedList, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	if (renderedList == NULL)
	{
		return 1;
	}

	while (1)
	{
		struct QuadBlock *block = *renderedList;

		if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, DRAW_LEVEL_OVR1P_BUCKET_RESERVE_4X1))
		{
			return 0;
		}

		DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0);
		if (block == NULL)
		{
			return 1;
		}

		Ovr226_800a417c_ProjectGround4x1RenderedGrid(vertices, block, projected);

		for (int faceIndex = 0; faceIndex < 4; faceIndex++)
		{
			if (!Ovr226_800a47f4_Ground4x1RenderedSelectorNearGate(pb, primMem, projected, block, faceIndex))
			{
				return 0;
			}
		}

		renderedList++;
	}
}

static int Ovr226_800a5e5c_DrawGround4x2RenderedList(struct QuadBlock **renderedList, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	if (renderedList == NULL)
	{
		return 1;
	}

	while (1)
	{
		struct QuadBlock *block = *renderedList;

		if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, DRAW_LEVEL_OVR1P_BUCKET_RESERVE_4X2))
		{
			return 0;
		}

		DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0);
		if (block == NULL)
		{
			return 1;
		}

		Ovr226_800a5e5c_ProjectGround4x2RenderedGrid(vertices, block, projected);

		for (int faceIndex = 0; faceIndex < 4; faceIndex++)
		{
			if (!Ovr226_800a6740_Ground4x2RenderedSelectorNearGate(pb, primMem, projected, block, faceIndex))
			{
				return 0;
			}
		}

		renderedList++;
	}
}

static int Ovr226_800a7ba8_DrawDynamicRenderedList(struct QuadBlock **renderedList, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	if (renderedList == NULL)
	{
		return 1;
	}

	while (1)
	{
		struct QuadBlock *block = *renderedList;

		if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, DRAW_LEVEL_OVR1P_BUCKET_RESERVE_DEFAULT))
		{
			return 0;
		}

		DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0);
		if (block == NULL)
		{
			return 1;
		}

		Ovr226_800a7ba8_ProjectDynamicRenderedGrid(vertices, block, projected);

		for (int faceIndex = 0; faceIndex < 4; faceIndex++)
		{
			if (!Ovr226_800a8380_DynamicRenderedSelectorNearGate(pb, primMem, projected, block, faceIndex))
			{
				return 0;
			}
		}

		renderedList++;
	}
}

static int Ovr226_800a97c8_DrawQuad4x4RenderedList(struct QuadBlock **renderedList, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	if (renderedList == NULL)
	{
		return 1;
	}

	while (1)
	{
		struct QuadBlock *block = *renderedList;

		if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, DRAW_LEVEL_OVR1P_BUCKET_RESERVE_DEFAULT))
		{
			return 0;
		}

		DrawLevelOvr1P_SetGridFaceSlotWord(projected, 0);
		if (block == NULL)
		{
			return 1;
		}

		Ovr226_800a97c8_ProjectQuad4x4RenderedGrid(vertices, block, projected);

		for (int faceIndex = 0; faceIndex < 4; faceIndex++)
		{
			if (!Ovr226_800a9fa0_Quad4x4RenderedSelectorNearGate(pb, primMem, projected, block, faceIndex))
			{
				return 0;
			}
		}

		renderedList++;
	}
}

static int Ovr226_800a5030_EmitGround4x2ListQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh, struct QuadBlock *block)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	if (Ovr226_800a5030_ProjectGround4x2ListGrid(vertices, block, projected))
	{
		DrawLevelOvr1P_AppendRenderedQuadBlock(block);
		return 1;
	}

	for (int faceIndex = 0; faceIndex < 4; faceIndex++)
	{
		if (!Ovr226_800a58ec_Ground4x2SelectorNearGate(pb, primMem, projected, block, faceIndex))
		{
			return 0;
		}
	}

	return 1;
}

static int Ovr226_800a4fa0_DrawGround4x2BspList(struct VisMemBspListNode *slot, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                                const int *visFaceList)
{
	if (sDrawLevelOvr1P_ListHandlersSeedRenderedCursor)
	{
		DrawLevelOvr1P_SetRenderedListCursor(DrawLevelOvr1P_GetRenderedOverflowBase());
	}

	while (slot != NULL)
	{
		struct BSP *bsp = slot->bsp;
		struct QuadBlock *block = bsp->data.leaf.ptrQuadBlockArray;
		s32 quadCount = bsp->data.leaf.numQuads;

		DrawLevelOvr1P_Scratch()->quadCount = quadCount;
		Ovr226_800a0f0c_SeedFullDynamicVisibilityScratch(visFaceList, block);

		while (quadCount > 0)
		{
			if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, DRAW_LEVEL_OVR1P_BUCKET_RESERVE_4X2))
			{
				return 0;
			}

			if (Ovr226_800a0f34_ConsumeFullDynamicVisibilityBit())
			{
				if (!Ovr226_800a5030_EmitGround4x2ListQuadBlock(pb, primMem, mesh, block))
				{
					return 0;
				}
			}

			block++;
			quadCount--;
			DrawLevelOvr1P_Scratch()->quadCount = quadCount;
		}

		slot = slot->next;
	}

	DrawLevelOvr1P_TerminateRenderedListCursor();
	return 1;
}

static int Ovr226_800a6fd0_EmitDynamicListQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh, struct QuadBlock *block)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	if (Ovr226_800a6fd0_ProjectDynamicListGrid(vertices, block, projected))
	{
		DrawLevelOvr1P_AppendRenderedQuadBlock(block);
		return 1;
	}

	for (int faceIndex = 0; faceIndex < 4; faceIndex++)
	{
		if (!Ovr226_800a7668_DynamicListSelectorNearGate(pb, primMem, projected, block, faceIndex))
		{
			return 0;
		}
	}

	return 1;
}

static int Ovr226_800a6f40_DrawDynamicBspList(struct VisMemBspListNode *slot, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                              const int *visFaceList)
{
	if (sDrawLevelOvr1P_ListHandlersSeedRenderedCursor)
	{
		DrawLevelOvr1P_SetRenderedListCursor(DrawLevelOvr1P_GetRenderedOverflowBase());
	}

	while (slot != NULL)
	{
		struct BSP *bsp = slot->bsp;
		struct QuadBlock *block = bsp->data.leaf.ptrQuadBlockArray;
		s32 quadCount = bsp->data.leaf.numQuads;

		DrawLevelOvr1P_Scratch()->quadCount = quadCount;
		Ovr226_800a0f0c_SeedFullDynamicVisibilityScratch(visFaceList, block);

		while (quadCount > 0)
		{
			if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, DRAW_LEVEL_OVR1P_BUCKET_RESERVE_DEFAULT))
			{
				return 0;
			}

			if (Ovr226_800a0f34_ConsumeFullDynamicVisibilityBit())
			{
				if (!Ovr226_800a6fd0_EmitDynamicListQuadBlock(pb, primMem, mesh, block))
				{
					return 0;
				}
			}

			block++;
			quadCount--;
			DrawLevelOvr1P_Scratch()->quadCount = quadCount;
		}

		slot = slot->next;
	}

	DrawLevelOvr1P_TerminateRenderedListCursor();
	return 1;
}

static int Ovr226_800a8bf0_EmitWideDynamicQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh, struct QuadBlock *block)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	if (Ovr226_800a8bf0_ProjectWideDynamicGrid(vertices, block, projected))
	{
		DrawLevelOvr1P_AppendRenderedQuadBlock(block);
		return 1;
	}

	for (int faceIndex = 0; faceIndex < 4; faceIndex++)
	{
		if (!Ovr226_800a9288_WideDynamicSelectorNearGate(pb, primMem, projected, block, faceIndex))
		{
			return 0;
		}
	}

	return 1;
}

static int Ovr226_800a8b60_DrawWideDynamicBspList(struct VisMemBspListNode *slot, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                                  const int *visFaceList)
{
	if (sDrawLevelOvr1P_ListHandlersSeedRenderedCursor)
	{
		DrawLevelOvr1P_SetRenderedListCursor(DrawLevelOvr1P_GetRenderedOverflowBase());
	}

	while (slot != NULL)
	{
		struct BSP *bsp = slot->bsp;
		struct QuadBlock *block = bsp->data.leaf.ptrQuadBlockArray;
		s32 quadCount = bsp->data.leaf.numQuads;

		DrawLevelOvr1P_Scratch()->quadCount = quadCount;
		Ovr226_800a0f0c_SeedFullDynamicVisibilityScratch(visFaceList, block);

		while (quadCount > 0)
		{
			if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, DRAW_LEVEL_OVR1P_BUCKET_RESERVE_DEFAULT))
			{
				return 0;
			}

			if (Ovr226_800a0f34_ConsumeFullDynamicVisibilityBit())
			{
				if (!Ovr226_800a8bf0_EmitWideDynamicQuadBlock(pb, primMem, mesh, block))
				{
					return 0;
				}
			}

			block++;
			quadCount--;
			DrawLevelOvr1P_Scratch()->quadCount = quadCount;
		}

		slot = slot->next;
	}

	DrawLevelOvr1P_TerminateRenderedListCursor();
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

static void DrawLevelOvr1P_ClearRenderedListForRole(struct DrawLevelOvr1PRenderList *renderList, int role)
{
	struct QuadBlock **renderedList = DrawLevelOvr1P_GetRenderedListForRole(renderList, role);

	if (renderedList != NULL)
	{
		*renderedList = NULL;
	}
}

static u32 Ovr226_800a262c_SelectWaterDirectMask(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, u32 allowedMask)
{
	u32 directMask = 0;

	// NOTE(aalhendi): Retail 0x800a261c..0x800a2658 tests full color words at
	// scratch vertex +0x8, then masks the two water GT3 direct bits through t2.
	if ((DrawLevelOvr1P_GetProjectedColorWord(&projected[indices[0]]) | DrawLevelOvr1P_GetProjectedColorWord(&projected[indices[1]]) |
	     DrawLevelOvr1P_GetProjectedColorWord(&projected[indices[2]])) != 0)
	{
		directMask |= DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY;
	}

	if ((DrawLevelOvr1P_GetProjectedColorWord(&projected[indices[1]]) | DrawLevelOvr1P_GetProjectedColorWord(&projected[indices[2]]) |
	     DrawLevelOvr1P_GetProjectedColorWord(&projected[indices[3]])) != 0)
	{
		directMask |= DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY;
	}

	return directMask & allowedMask;
}

static u32 Ovr226_800a262c_SelectAndStoreWaterDirectMask(const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, u32 allowedMask)
{
	u32 directMask = Ovr226_800a262c_SelectWaterDirectMask(projected, indices, allowedMask);

	DrawLevelOvr1P_Scratch()->directMask = directMask;
	return directMask;
}

static void DrawLevelOvr1P_BuildGridSubdivisionFrame(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                     const int *indices, int writeClipBytes)
{
	DrawLevelOvr1P_BuildMidpointFromFirstEndpoint(&sub[0], &sub[4], &projected[indices[0]], &projected[indices[1]], writeClipBytes);
	DrawLevelOvr1P_BuildMidpointFromFirstEndpoint(&sub[1], &sub[7], &projected[indices[1]], &projected[indices[3]], writeClipBytes);
	DrawLevelOvr1P_BuildMidpointFromFirstEndpoint(&sub[2], &sub[5], &projected[indices[2]], &projected[indices[0]], writeClipBytes);
	DrawLevelOvr1P_BuildMidpointFromFirstEndpoint(&sub[3], &sub[8], &projected[indices[3]], &projected[indices[2]], writeClipBytes);
	DrawLevelOvr1P_BuildMidpointFromFirstEndpoint(&sub[1], &sub[6], &projected[indices[1]], &projected[indices[2]], writeClipBytes);
}

static void Ovr226_800a24e8_BuildWaterListSubdivideMidpoint(struct DrawLevelOvr1PScratchVertex *dstA, struct DrawLevelOvr1PScratchVertex *dstMid,
                                                            const struct DrawLevelOvr1PScratchVertex *srcA, const struct DrawLevelOvr1PScratchVertex *srcB)
{
	const u8 *srcAUv = (const u8 *)&srcA->flags;
	const u8 *srcBUv = (const u8 *)&srcB->flags;
	u8 *dstAUv = (u8 *)&dstA->flags;
	u8 *dstMidUv = (u8 *)&dstMid->flags;
	u32 depth;

	for (int i = 0; i < 3; i++)
	{
		dstA->pos[i] = srcA->pos[i];
		dstMid->pos[i] = (s16)(((s32)srcA->pos[i] + (s32)srcB->pos[i]) >> 1);
	}

	CTR_GteLoadS16TripletV0(&dstMid->pos[0]);
	gte_rtps();

	for (int i = 0; i < 3; i++)
	{
		dstA->color_hi[i] = srcA->color_hi[i];
		dstMid->color_hi[i] = (u8)(((u32)srcA->color_hi[i] + (u32)srcB->color_hi[i]) >> 1);
	}

	for (int i = 0; i < 2; i++)
	{
		dstAUv[i] = srcAUv[i];
		dstMidUv[i] = (u8)(((u32)srcAUv[i] + (u32)srcBUv[i]) >> 1);
	}

	DrawLevelOvr1P_CopyProjectedScreenDepth(dstA, srcA);
	CTR_GteStoreSXY(&dstMid->posScreen[0]);
	gte_stsz(&depth);
	DrawLevelOvr1P_StoreProjectedDepthWord(dstMid, depth);
}

static void Ovr226_800a24e8_BuildWaterListSubdivisionFrame(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                           const int *indices)
{
	Ovr226_800a24e8_BuildWaterListSubdivideMidpoint(&sub[0], &sub[4], &projected[indices[0]], &projected[indices[1]]);
	Ovr226_800a24e8_BuildWaterListSubdivideMidpoint(&sub[1], &sub[7], &projected[indices[1]], &projected[indices[3]]);
	Ovr226_800a24e8_BuildWaterListSubdivideMidpoint(&sub[2], &sub[5], &projected[indices[2]], &projected[indices[0]]);
	Ovr226_800a24e8_BuildWaterListSubdivideMidpoint(&sub[3], &sub[8], &projected[indices[3]], &projected[indices[2]]);
	Ovr226_800a24e8_BuildWaterListSubdivideMidpoint(&sub[1], &sub[6], &projected[indices[1]], &projected[indices[2]]);
}

static void Ovr226_800a2fe4_BuildWaterRenderedSubdivideMidpoint(struct DrawLevelOvr1PScratchVertex *dstA, struct DrawLevelOvr1PScratchVertex *dstMid,
                                                                const struct DrawLevelOvr1PScratchVertex *srcA, const struct DrawLevelOvr1PScratchVertex *srcB)
{
	const u8 *srcAUv = (const u8 *)&srcA->flags;
	const u8 *srcBUv = (const u8 *)&srcB->flags;
	u8 *dstAUv = (u8 *)&dstA->flags;
	u8 *dstMidUv = (u8 *)&dstMid->flags;
	u32 depth;

	for (int i = 0; i < 3; i++)
	{
		dstA->pos[i] = srcA->pos[i];
		dstMid->pos[i] = (s16)(((s32)srcA->pos[i] + (s32)srcB->pos[i]) >> 1);
	}

	CTR_GteLoadS16TripletV0(&dstMid->pos[0]);
	gte_rtps();

	for (int i = 0; i < 3; i++)
	{
		dstA->color_hi[i] = srcA->color_hi[i];
		dstMid->color_hi[i] = (u8)(((u32)srcA->color_hi[i] + (u32)srcB->color_hi[i]) >> 1);
	}

	for (int i = 0; i < 2; i++)
	{
		dstAUv[i] = srcAUv[i];
		dstMidUv[i] = (u8)(((u32)srcAUv[i] + (u32)srcBUv[i]) >> 1);
	}

	DrawLevelOvr1P_CopyProjectedScreenDepth(dstA, srcA);
	CTR_GteStoreSXY(&dstMid->posScreen[0]);
	gte_stsz(&depth);
	DrawLevelOvr1P_SetProjectedDepth(dstMid, depth, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
}

static void Ovr226_800a2fe4_BuildWaterRenderedSubdivisionFrame(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                               const int *indices)
{
	Ovr226_800a2fe4_BuildWaterRenderedSubdivideMidpoint(&sub[0], &sub[4], &projected[indices[0]], &projected[indices[1]]);
	Ovr226_800a2fe4_BuildWaterRenderedSubdivideMidpoint(&sub[1], &sub[7], &projected[indices[1]], &projected[indices[3]]);
	Ovr226_800a2fe4_BuildWaterRenderedSubdivideMidpoint(&sub[2], &sub[5], &projected[indices[2]], &projected[indices[0]]);
	Ovr226_800a2fe4_BuildWaterRenderedSubdivideMidpoint(&sub[3], &sub[8], &projected[indices[3]], &projected[indices[2]]);
	Ovr226_800a2fe4_BuildWaterRenderedSubdivideMidpoint(&sub[1], &sub[6], &projected[indices[1]], &projected[indices[2]]);
}

static void DrawLevelOvr1P_BuildGridSubdivisionFrame4x4(struct DrawLevelOvr1PScratchVertex *sub, const struct DrawLevelOvr1PScratchVertex *projected,
                                                        const int *indices, int writeClipBytes)
{
	DrawLevelOvr1P_BuildMidpointFromFirstEndpoint(&sub[0], &sub[4], &projected[indices[0]], &projected[indices[1]], writeClipBytes);
	DrawLevelOvr1P_BuildMidpointFromFirstEndpoint(&sub[1], &sub[7], &projected[indices[1]], &projected[indices[3]], writeClipBytes);
	DrawLevelOvr1P_BuildMidpointFromFirstEndpoint(&sub[2], &sub[5], &projected[indices[2]], &projected[indices[0]], writeClipBytes);
	DrawLevelOvr1P_BuildMidpointFromFirstEndpoint(&sub[3], &sub[8], &projected[indices[3]], &projected[indices[2]], writeClipBytes);
	// NOTE(aalhendi): Retail 4x4 helpers build the center from edge midpoints.
	DrawLevelOvr1P_BuildMidpointFromFirstEndpoint(&sub[4], &sub[6], &sub[4], &sub[8], writeClipBytes);
}

static int Ovr226_800a25d0_WaterListFaceGate(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex, int depth, u32 allowedMask,
                                             uint32_t *inheritedOtEntry);

static int Ovr226_800a22a4_DispatchWaterListHelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                           const struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, int depth, u32 handlerAddress,
                                                           int handlerSlot, u32 allowedMask, uint32_t *inheritedOtEntry)
{
	int slot = handlerSlot;

	switch (handlerAddress)
	{
	case 0x800a22a4:
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, depth, allowedMask,
		                                         inheritedOtEntry);
	case 0x800a22b8:
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, depth, allowedMask,
		                                         inheritedOtEntry);
	case 0x800a22cc:
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, depth, allowedMask,
		                                         inheritedOtEntry);
	case 0x800a22e0:
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, depth, allowedMask,
		                                         inheritedOtEntry);
	case 0x800a22f4:
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, depth, allowedMask,
		                                         inheritedOtEntry);
	case 0x800a2308:
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, depth, allowedMask,
		                                         inheritedOtEntry);
	case 0x800a231c:
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, depth, allowedMask,
		                                         inheritedOtEntry);
	case 0x800a2330:
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, depth, allowedMask,
		                                         inheritedOtEntry);
	case 0x800a2344:
	case 0x800a15cc:
	case 0x800a1680:
		slot = 0;
		break;
	case 0x800a2384:
	case 0x800a160c:
	case 0x800a16c0:
		slot = 1;
		break;
	case 0x800a2404:
	case 0x800a168c:
	case 0x800a1740:
		slot = 2;
		break;
	case 0x800a23a4:
	case 0x800a162c:
	case 0x800a16e0:
		slot = 3;
		break;
	case 0x800a242c:
	case 0x800a16b4:
	case 0x800a1768:
		slot = 4;
		break;
	case 0x800a23c4:
	case 0x800a164c:
	case 0x800a1700:
		slot = 7;
		break;
	case 0x800a2454:
	case 0x800a16dc:
	case 0x800a1790:
		slot = 9;
		break;
	case 0x800a247c:
	case 0x800a1704:
	case 0x800a17b8:
		slot = 11;
		break;
	case 0x800a24a4:
	case 0x800a172c:
	case 0x800a17e0:
		slot = 5;
		break;
	default:
		break;
	}

	// NOTE(aalhendi): Retail 0x800a22a4..0x800a24a4 wrappers are selected by
	// copied scratch 0x14c addresses and pass their own t2 direct-mask constants
	// into shared face helper 0x800a25d0.
	switch (slot)
	{
	case 0:
		// 0x800a2344
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[0], faceIndex, depth,
		                                       DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[1], faceIndex, depth,
		                                       DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, depth,
		                                         DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	case 1:
		// 0x800a2384
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                       inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, depth,
		                                       DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, depth,
		                                         DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	case 2:
		// 0x800a2404
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, depth,
		                                       DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, depth,
		                                       DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                       inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, depth,
		                                         DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	case 3:
		// 0x800a23a4
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, depth,
		                                       DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                       inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, depth,
		                                         DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	case 4:
		// 0x800a242c
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, depth,
		                                       DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, depth,
		                                       DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                       inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, depth,
		                                         DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	case 7:
		// 0x800a23c4
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[2], faceIndex, depth,
		                                       DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[3], faceIndex, depth,
		                                       DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, depth,
		                                         DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	case 9:
		// 0x800a2454
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, depth,
		                                       DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, depth,
		                                       DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                       inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, depth,
		                                         DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	case 11:
		// 0x800a247c
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, depth,
		                                       DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, depth,
		                                       DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                       inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, depth,
		                                         DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	default:
		break;
	}

	// 0x800a24a4
	DrawLevelOvr1P_SetGridFaceSlot(projected, 0);
	if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], 0, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
	                                       inheritedOtEntry))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlot(projected, 1);
	if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], 1, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
	                                       inheritedOtEntry))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlot(projected, 2);
	if (!Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], 2, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
	                                       inheritedOtEntry))
	{
		return 0;
	}
	DrawLevelOvr1P_SetGridFaceSlot(projected, 3);
	return Ovr226_800a25d0_WaterListFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], 3, depth, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
	                                         inheritedOtEntry);
}

static int Ovr226_800a27dc_EmitWaterListGT3Raw(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex, int secondary,
                                               uint32_t *inheritedOtEntry)
{
	int triIndices[3];
	POLY_GT3 *prim;
	POLY_GT3 *nextPrim;
	u32 uv0;
	u32 uv1;
	u32 uv2;

	(void)pb;
	(void)block;
	(void)faceIndex;

	if (secondary)
	{
		// NOTE(aalhendi): Retail label 0x800a27d4 does `s3=s4; s4=s6`
		// before falling through to the fixed-code GT3 writer at 0x800a27dc.
		triIndices[0] = indices[1];
		triIndices[1] = indices[3];
		triIndices[2] = indices[2];
	}
	else
	{
		triIndices[0] = indices[0];
		triIndices[1] = indices[1];
		triIndices[2] = indices[2];
	}

	if (inheritedOtEntry == NULL)
	{
		return 1;
	}

	prim = primMem->cursor;
	nextPrim = prim + 1;
	DrawLevelOvr1P_StoreProjectedDirectUvScratch(projected, triIndices, 3);
	uv0 = DrawLevelOvr1P_Scratch()->uv.uv0;
	uv1 = DrawLevelOvr1P_Scratch()->uv.uv1;
	uv2 = DrawLevelOvr1P_Scratch()->uv.uv2;

	DrawLevelOvr1P_WriteProjectedGT3(prim, projected, triIndices, 0x36, uv0, uv1, uv2);
	DrawLevelOvr1P_AddRawPrimToOt(primMem, prim, 9, inheritedOtEntry);
	primMem->cursor = nextPrim;
	return 1;
}

static int Ovr226_800a2850_EmitWaterListGT4Raw(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                               const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                               uint32_t *inheritedOtEntry)
{
	POLY_GT4 *prim;
	POLY_GT4 *nextPrim;
	u32 uv0;
	u32 uv1;
	u32 uv2;

	(void)pb;
	(void)block;
	(void)faceIndex;

	if (inheritedOtEntry == NULL)
	{
		return 1;
	}

	prim = primMem->cursor;
	nextPrim = prim + 1;
	DrawLevelOvr1P_StoreProjectedDirectUvScratch(projected, indices, 4);
	uv0 = DrawLevelOvr1P_Scratch()->uv.uv0;
	uv1 = DrawLevelOvr1P_Scratch()->uv.uv1;
	uv2 = DrawLevelOvr1P_Scratch()->uv.uv2;

	DrawLevelOvr1P_WriteProjectedGT4(prim, projected, indices, 0x3e, uv0, uv1, uv2);
	DrawLevelOvr1P_AddRawPrimToOt(primMem, prim, 12, inheritedOtEntry);
	primMem->cursor = nextPrim;
	return 1;
}

static int Ovr226_800a27b8_EmitWaterListDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                   const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                   uint32_t *inheritedOtEntry)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	// NOTE(aalhendi): 227/228/229 water-list direct tables use shifted retail
	// labels with the same packet ABI; alias them to the owned native writers.
	switch (handlerAddress)
	{
	case 0x800a27dc:
	case 0x800a28d0:
	case 0x800a1a60:
	case 0x800a1b14:
	case 0x800a25bc:
	case 0x800a2670:
		return Ovr226_800a27dc_EmitWaterListGT3Raw(pb, primMem, block, projected, indices, faceIndex, 0, inheritedOtEntry);
	case 0x800a27d4:
	case 0x800a28c8:
	case 0x800a1a58:
	case 0x800a1b0c:
	case 0x800a25b4:
	case 0x800a2668:
		return Ovr226_800a27dc_EmitWaterListGT3Raw(pb, primMem, block, projected, indices, faceIndex, 1, inheritedOtEntry);
	case 0x800a2850:
	case 0x800a2944:
	case 0x800a1ad4:
	case 0x800a1b88:
	case 0x800a2660:
	case 0x800a2714:
		return Ovr226_800a2850_EmitWaterListGT4Raw(pb, primMem, block, projected, indices, faceIndex, inheritedOtEntry);
	default:
		return 1;
	}
}

static int Ovr226_800a2660_WaterListNearDispatch(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex, int depth,
                                                 u32 handlerAddress, int handlerSlot, u32 allowedMask, uint32_t *inheritedOtEntry)
{
	struct DrawLevelOvr1PScratchVertex *sub;

	if (DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
	{
		// NOTE(aalhendi): Retail 0x800a265c preserves the current t2 mask,
		// and 0x800a2668 jumps directly to the direct table at 0x800a27b8
		// when the recursion frame reaches scratch 0x324.
		DrawLevelOvr1P_Scratch()->directMask = allowedMask;
		return Ovr226_800a27b8_EmitWaterListDirectTail(pb, primMem, block, projected, indices, faceIndex, inheritedOtEntry);
	}

	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	// NOTE(aalhendi): Retail 0x800a272c..0x800a27a4 advances by 0xb8,
	// builds five 0x800a24e8 midpoint pairs, then jumps through scratch 0x148.
	Ovr226_800a24e8_BuildWaterListSubdivisionFrame(sub, projected, indices);

	return Ovr226_800a22a4_DispatchWaterListHelperWrappers(pb, primMem, block, sub, faceIndex, depth + 1, handlerAddress, handlerSlot, allowedMask,
	                                                       inheritedOtEntry);
}

static int Ovr226_800a25d0_WaterListFaceGate(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                             const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex, int depth, u32 allowedMask,
                                             uint32_t *inheritedOtEntry)
{
	u32 directMask;
	u32 nearMask;
	uint32_t *faceOtEntry;

	// NOTE(aalhendi): Retail 0x800a25d0..0x800a27b8 owns the shared water-list
	// face gate: packed rectangle reject, color-derived direct mask, top-frame
	// OT inheritance, near-mask selection, and recursive child dispatch.
	if (DrawLevelOvr1P_IsProjectedFaceOffscreen(pb, projected, indices))
	{
		return 1;
	}

	directMask = Ovr226_800a262c_SelectAndStoreWaterDirectMask(projected, indices, allowedMask);
	if (directMask == 0)
	{
		return 1;
	}

	faceOtEntry = Ovr226_800a2690_ResolveWaterListInheritedOtEntry(pb, block, projected, indices, faceIndex, inheritedOtEntry);
	nearMask = DrawLevelOvr1P_GetProjectedWaterNearMask(projected, indices);
	if (nearMask != 0)
	{
		u32 handlerAddress = DrawLevelOvr1P_GetNearSubdivisionHandlerAddress(nearMask, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST);
		int handlerSlot = DrawLevelOvr1P_GetNearSubdivisionTableSlot(nearMask);

		return Ovr226_800a2660_WaterListNearDispatch(pb, primMem, block, projected, indices, faceIndex, depth, handlerAddress, handlerSlot, directMask,
		                                             faceOtEntry);
	}

	return Ovr226_800a27b8_EmitWaterListDirectTail(pb, primMem, block, projected, indices, faceIndex, faceOtEntry);
}

static int DrawLevelOvr1P_DrawWaterListQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh, struct QuadBlock *block,
                                                 int captureRenderedOverflow)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	if ((block->quadFlags & QUADBLOCK_FLAG_SKIP_WATER_LIST) != 0)
	{
		return 1;
	}

	if (DrawLevelOvr1P_ProjectQuadBlockGrid(vertices, block, projected, DRAW_LEVEL_OVR1P_CLIP_BYTES_LIST))
	{
		if (captureRenderedOverflow)
		{
			DrawLevelOvr1P_AppendRenderedQuadBlock(block);
		}

		return 1;
	}

	Ovr226_800a211c_ApplyWaterListColorFades(projected);

	return Ovr226_800a22a4_DispatchWaterListHelperWrappers(pb, primMem, block, projected, 0, 0, 0x800a24a4, 5, DRAW_LEVEL_OVR1P_DIRECT_QUAD, NULL);
}

static void Ovr226_800a1e30_SeedWaterListState(void)
{
	const struct TextureLayout *waterEnvMap = (const struct TextureLayout *)(uintptr_t)DrawLevelOvr1P_Scratch()->waterEnvMapPtr32;

	// NOTE(aalhendi): Retail 0x800a1e30 uses the global 1P retry list, not the
	// current render-list field, before walking the water BSP list.
	if (sDrawLevelOvr1P_ListHandlersSeedRenderedCursor)
	{
		DrawLevelOvr1P_SetRenderedListCursor(DrawLevelOvr1P_GetRenderedOverflowBase());
	}
	CTC2(0, 21);
	CTC2(0, 22);
	CTC2(0, 23);
	DrawLevelOvr1P_Scratch()->uv.uv0 = DrawLevelOvr1P_ReadPackedWord((const u8 *)waterEnvMap + 0);
	DrawLevelOvr1P_Scratch()->uv.uv1 = DrawLevelOvr1P_ReadPackedWord((const u8 *)waterEnvMap + 4);
}

static void Ovr226_800a1e74_SeedWaterVisibilityScratch(const int *visFaceList, const struct QuadBlock *block)
{
	Ovr226_800a0f0c_SeedFullDynamicVisibilityScratch(visFaceList, block);
}

static int Ovr226_800a1eb0_ConsumeWaterVisibilityBit(void)
{
	return Ovr226_800a0f34_ConsumeFullDynamicVisibilityBit();
}

static int Ovr226_800a1ee0_EmitWaterListQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh, struct QuadBlock *block)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	if (Ovr226_800a1ee0_ProjectWaterListGrid(vertices, block, projected))
	{
		DrawLevelOvr1P_AppendRenderedQuadBlock(block);
		return 1;
	}

	Ovr226_800a211c_ApplyWaterListColorFades(projected);

	return Ovr226_800a22a4_DispatchWaterListHelperWrappers(pb, primMem, block, projected, 0, 0, 0x800a24a4, 5, DRAW_LEVEL_OVR1P_DIRECT_QUAD, NULL);
}

static int Ovr226_800a1e30_DrawWaterBspList(struct VisMemBspListNode *slot, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                            const int *visFaceList)
{
	Ovr226_800a1e30_SeedWaterListState();

	while (slot != NULL)
	{
		struct BSP *bsp = slot->bsp;
		struct QuadBlock *block = bsp->data.leaf.ptrQuadBlockArray;
		s32 quadCount = bsp->data.leaf.numQuads;

		DrawLevelOvr1P_Scratch()->quadCount = quadCount;
		Ovr226_800a1e74_SeedWaterVisibilityScratch(visFaceList, block);

		while (quadCount > 0)
		{
			if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, DRAW_LEVEL_OVR1P_BUCKET_RESERVE_DEFAULT))
			{
				return 0;
			}

			if (Ovr226_800a1eb0_ConsumeWaterVisibilityBit())
			{
				if (!Ovr226_800a1ee0_EmitWaterListQuadBlock(pb, primMem, mesh, block))
				{
					return 0;
				}
			}

			block++;
			quadCount--;
			DrawLevelOvr1P_Scratch()->quadCount = quadCount;
		}

		slot = slot->next;
	}

	DrawLevelOvr1P_TerminateRenderedListCursor();
	return 1;
}

static int Ovr226_800a30f0_WaterRenderedFaceGate(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex, int depth,
                                                 u32 allowedMask, uint32_t *inheritedOtEntry);

static int Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                               const struct DrawLevelOvr1PScratchVertex *projected, int faceIndex, int depth,
                                                               u32 handlerAddress, int handlerSlot, u32 allowedMask, uint32_t *inheritedOtEntry)
{
	(void)handlerSlot;

	// NOTE(aalhendi): Retail 0x800a2da0..0x800a2fe4 wrapper labels either
	// select four projected scratch records and tail-call 0x800a30f0, or call
	// those leaf labels in a fixed sequence before branching to the final leaf.
	// Native uses the C call stack for the retail t9/ra scratch return stack, but
	// the label order, frame+0xb4 writes, and t2 direct-mask constants match the
	// refreshed NTSC-U 926 objdump for this span.
	// 228/229 water-rendered RDATA uses shifted copies of this wrapper table;
	// those labels alias to the same owned helper sequences after target checks.
	switch (handlerAddress)
	{
	case 0x800a2da0:
	case 0x800a2024:
	case 0x800a20d8:
		return Ovr226_800a30f0_WaterRenderedFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[0], faceIndex, depth, allowedMask,
		                                             inheritedOtEntry);
	case 0x800a2db4:
	case 0x800a2038:
	case 0x800a20ec:
		return Ovr226_800a30f0_WaterRenderedFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[1], faceIndex, depth, allowedMask,
		                                             inheritedOtEntry);
	case 0x800a2dc8:
	case 0x800a204c:
	case 0x800a2100:
		return Ovr226_800a30f0_WaterRenderedFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[2], faceIndex, depth, allowedMask,
		                                             inheritedOtEntry);
	case 0x800a2ddc:
	case 0x800a2060:
	case 0x800a2114:
		return Ovr226_800a30f0_WaterRenderedFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridFaceIndices[3], faceIndex, depth, allowedMask,
		                                             inheritedOtEntry);
	case 0x800a2df0:
	case 0x800a2074:
	case 0x800a2128:
		return Ovr226_800a30f0_WaterRenderedFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[0], faceIndex, depth, allowedMask,
		                                             inheritedOtEntry);
	case 0x800a2e04:
	case 0x800a2088:
	case 0x800a213c:
		return Ovr226_800a30f0_WaterRenderedFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[1], faceIndex, depth, allowedMask,
		                                             inheritedOtEntry);
	case 0x800a2e18:
	case 0x800a209c:
	case 0x800a2150:
		return Ovr226_800a30f0_WaterRenderedFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[2], faceIndex, depth, allowedMask,
		                                             inheritedOtEntry);
	case 0x800a2e2c:
	case 0x800a20b0:
	case 0x800a2164:
		return Ovr226_800a30f0_WaterRenderedFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridExtraFaceIndices[3], faceIndex, depth, allowedMask,
		                                             inheritedOtEntry);
	case 0x800a2e40:
	case 0x800a20c4:
	case 0x800a2178:
		if (!Ovr226_800a30f0_WaterRenderedFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[0], faceIndex, depth,
		                                           DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a30f0_WaterRenderedFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[1], faceIndex, depth,
		                                           DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2da0, -1,
		                                                           DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	case 0x800a2e80:
	case 0x800a2104:
	case 0x800a21b8:
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2db4, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                         inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2e18, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                         inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2e2c, -1,
		                                                           DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	case 0x800a2ea0:
	case 0x800a2124:
	case 0x800a21d8:
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2df0, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                         inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2dc8, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                         inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2e04, -1,
		                                                           DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	case 0x800a2ec0:
	case 0x800a2144:
	case 0x800a21f8:
		if (!Ovr226_800a30f0_WaterRenderedFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[2], faceIndex, depth,
		                                           DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a30f0_WaterRenderedFaceGate(pb, primMem, block, projected, sDrawLevelOvr1PGridMixedFaceIndices[3], faceIndex, depth,
		                                           DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2ddc, -1,
		                                                           DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	case 0x800a2f00:
	case 0x800a2184:
	case 0x800a2238:
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2e2c, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                         inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2dc8, -1,
		                                                         DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2da0, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                         inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2db4, -1,
		                                                           DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	case 0x800a2f28:
	case 0x800a21ac:
	case 0x800a2260:
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2e04, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                         inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2db4, -1,
		                                                         DRAW_LEVEL_OVR1P_DIRECT_TRI_PRIMARY, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2da0, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                         inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2dc8, -1,
		                                                           DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	case 0x800a2f50:
	case 0x800a21d4:
	case 0x800a2288:
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2e18, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                         inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2dc8, -1,
		                                                         DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2db4, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                         inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2ddc, -1,
		                                                           DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	case 0x800a2f78:
	case 0x800a21fc:
	case 0x800a22b0:
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2df0, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                         inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2db4, -1,
		                                                         DRAW_LEVEL_OVR1P_DIRECT_TRI_SECONDARY, inheritedOtEntry))
		{
			return 0;
		}
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2dc8, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                         inheritedOtEntry))
		{
			return 0;
		}
		return Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, faceIndex, depth, 0x800a2ddc, -1,
		                                                           DRAW_LEVEL_OVR1P_DIRECT_QUAD, inheritedOtEntry);
	case 0x800a2fa0:
	case 0x800a2224:
	case 0x800a22d8:
		DrawLevelOvr1P_SetGridFaceSlot(projected, 0);
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, 0, depth, 0x800a2da0, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                         inheritedOtEntry))
		{
			return 0;
		}
		DrawLevelOvr1P_SetGridFaceSlot(projected, 1);
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, 1, depth, 0x800a2db4, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                         inheritedOtEntry))
		{
			return 0;
		}
		DrawLevelOvr1P_SetGridFaceSlot(projected, 2);
		if (!Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, 2, depth, 0x800a2dc8, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                         inheritedOtEntry))
		{
			return 0;
		}
		DrawLevelOvr1P_SetGridFaceSlot(projected, 3);
		return Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, 3, depth, 0x800a2ddc, -1, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
		                                                           inheritedOtEntry);
	default:
		return 1;
	}
}

static uint32_t *Ovr226_800a31f0_ResolveWaterRenderedInheritedOtEntry(struct PushBuffer *pb, const struct QuadBlock *block,
                                                                      const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex,
                                                                      uint32_t *inheritedOtEntry)
{
	u32 slotWord;
	s8 drawOrder;
	u32 selectedDepth;
	s32 otIndex;

	if (inheritedOtEntry != NULL)
	{
		return inheritedOtEntry;
	}

	// NOTE(aalhendi): Retail 0x800a31f0..0x800a3238 computes GP only on the
	// top water-rendered scratch frame. Recursive children inherit the pointer.
	if (projected != DrawLevelOvr1P_GetScratchVertices())
	{
		return NULL;
	}

	selectedDepth = DrawLevelOvr1P_GetWaterTopFrameOtDepth_800a2690_800a31f0(projected, indices);
	slotWord = DrawLevelOvr1P_GetProjectedOtSlotWord(projected, faceIndex);
	drawOrder = DrawLevelOvr1P_ReadRetailQuadBlockByte(block, 0x18 + (slotWord >> 2));
	otIndex = (s32)(selectedDepth >> 6) + drawOrder;

	return &pb->ptrOT[otIndex];
}

static int Ovr226_800a333c_EmitWaterRenderedGT3RawOrClip(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int secondary,
                                                         uint32_t *inheritedOtEntry)
{
	int triIndices[3];

	(void)block;

	if (secondary)
	{
		// NOTE(aalhendi): Retail label 0x800a3334 does `s3=s4; s4=s6`
		// before falling through to the water-rendered GT3 terminal at 0x800a333c.
		triIndices[0] = indices[1];
		triIndices[1] = indices[3];
		triIndices[2] = indices[2];
	}
	else
	{
		triIndices[0] = indices[0];
		triIndices[1] = indices[1];
		triIndices[2] = indices[2];
	}

	if (inheritedOtEntry == NULL)
	{
		return 1;
	}

	if (DrawLevelOvr1P_AreProjectedVerticesHalfNearSigned(projected, triIndices, 3))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNearSigned(projected, triIndices, 3))
	{
		return Ovr226_800a34d4_WriteWaterRenderedClippedRecordAtOtEntry(pb, projected, triIndices, 3, inheritedOtEntry);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedTriRawCodeAtOtEntry(pb, primMem, block, projected, triIndices, NULL, inheritedOtEntry, 0x36);
}

static int Ovr226_800a33e0_EmitWaterRenderedGT4RawOrClip(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                         const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, uint32_t *inheritedOtEntry)
{
	if (inheritedOtEntry == NULL)
	{
		return 1;
	}

	if (DrawLevelOvr1P_AreProjectedVerticesHalfNear(projected, indices, 4))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		return Ovr226_800a34d4_WriteWaterRenderedClippedRecordAtOtEntry(pb, projected, indices, 4, inheritedOtEntry);
	}

	return DrawLevelOvr1P_EmitPreparedProjectedQuadRawCodeAtOtEntry(pb, primMem, block, projected, indices, NULL, inheritedOtEntry, 0x3e);
}

static int Ovr226_800a3318_DispatchWaterRenderedDirectTail(struct PushBuffer *pb, struct PrimMem *primMem, const struct QuadBlock *block,
                                                           const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, uint32_t *inheritedOtEntry)
{
	u32 directMask = DrawLevelOvr1P_Scratch()->directMask;
	u32 handlerAddress = DrawLevelOvr1P_GetDirectHandlerAddress(directMask);

	// NOTE(aalhendi): 228/229 water-rendered direct tables use shifted retail
	// labels with the same raw/clip packet ABI; alias them to the owned writers.
	switch (handlerAddress)
	{
	case 0x800a333c:
	case 0x800a25bc:
	case 0x800a2670:
		return Ovr226_800a333c_EmitWaterRenderedGT3RawOrClip(pb, primMem, block, projected, indices, 0, inheritedOtEntry);
	case 0x800a3334:
	case 0x800a25b4:
	case 0x800a2668:
		return Ovr226_800a333c_EmitWaterRenderedGT3RawOrClip(pb, primMem, block, projected, indices, 1, inheritedOtEntry);
	case 0x800a33e0:
	case 0x800a2660:
	case 0x800a2714:
		return Ovr226_800a33e0_EmitWaterRenderedGT4RawOrClip(pb, primMem, block, projected, indices, inheritedOtEntry);
	default:
		return 1;
	}
}

static int Ovr226_800a31bc_WaterRenderedNearDispatch(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                     const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex, int depth,
                                                     u32 allowedMask, uint32_t *inheritedOtEntry)
{
	struct DrawLevelOvr1PScratchVertex *sub;
	u32 nearMask;
	u32 handlerAddress;
	int handlerSlot;
	uint32_t *faceOtEntry;

	DrawLevelOvr1P_Scratch()->directMask = allowedMask;

	if (DrawLevelOvr1P_IsDeepestSubdivisionFrame(projected))
	{
		return Ovr226_800a3318_DispatchWaterRenderedDirectTail(pb, primMem, block, projected, indices, inheritedOtEntry);
	}

	faceOtEntry = Ovr226_800a31f0_ResolveWaterRenderedInheritedOtEntry(pb, block, projected, indices, faceIndex, inheritedOtEntry);
	nearMask = DrawLevelOvr1P_GetProjectedWaterNearMask(projected, indices);
	if (nearMask == 0)
	{
		return Ovr226_800a3318_DispatchWaterRenderedDirectTail(pb, primMem, block, projected, indices, faceOtEntry);
	}

	sub = DrawLevelOvr1P_GetSubdivisionFrame(depth);
	// NOTE(aalhendi): Retail 0x800a31c0 builds the water 3x3 subdivision
	// frame before jumping through the copied recursive-handler table.
	Ovr226_800a2fe4_BuildWaterRenderedSubdivisionFrame(sub, projected, indices);

	handlerAddress = DrawLevelOvr1P_GetNearSubdivisionHandlerAddress(nearMask, DRAW_LEVEL_OVR1P_CLIP_BYTES_RENDERED);
	handlerSlot = DrawLevelOvr1P_GetNearSubdivisionTableSlot(nearMask);
	return Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, sub, faceIndex, depth + 1, handlerAddress, handlerSlot, allowedMask,
	                                                           faceOtEntry);
}

static int Ovr226_800a30f0_WaterRenderedFaceGate(struct PushBuffer *pb, struct PrimMem *primMem, struct QuadBlock *block,
                                                 const struct DrawLevelOvr1PScratchVertex *projected, const int *indices, int faceIndex, int depth,
                                                 u32 allowedMask, uint32_t *inheritedOtEntry)
{
	u32 directMask;

	if (DrawLevelOvr1P_AreProjectedVerticesHalfNear(projected, indices, 4))
	{
		return 1;
	}

	if (DrawLevelOvr1P_HasProjectedVertexNear(projected, indices, 4))
	{
		return Ovr226_800a31bc_WaterRenderedNearDispatch(pb, primMem, block, projected, indices, faceIndex, depth, allowedMask, inheritedOtEntry);
	}

	if (DrawLevelOvr1P_IsProjectedFaceOffscreen(pb, projected, indices))
	{
		return 1;
	}

	directMask = Ovr226_800a262c_SelectAndStoreWaterDirectMask(projected, indices, allowedMask);
	if (directMask == 0)
	{
		return 1;
	}

	return Ovr226_800a31bc_WaterRenderedNearDispatch(pb, primMem, block, projected, indices, faceIndex, depth, directMask, inheritedOtEntry);
}

static int DrawLevelOvr1P_DrawRenderedWaterQuadBlockWithDefaultHandler(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh,
                                                                       struct QuadBlock *block, u32 defaultHandlerAddress)
{
	struct LevVertex *vertices = mesh->ptrVertexArray;
	struct DrawLevelOvr1PScratchVertex *projected = DrawLevelOvr1P_GetScratchVertices();

	Ovr226_800a2924_ProjectWaterRenderedGrid(vertices, block, projected);
	Ovr226_800a2c4c_ApplyWaterRenderedColorFades(projected);

	// NOTE(aalhendi): Retail water-rendered helper dispatch enters the current
	// overlay's default wrapper with no inherited GP/OT pointer.
	return Ovr226_800a2da0_DispatchWaterRenderedHelperWrappers(pb, primMem, block, projected, 0, 0, defaultHandlerAddress, 5, DRAW_LEVEL_OVR1P_DIRECT_QUAD,
	                                                           NULL);
}

static int DrawLevelOvr1P_DrawRenderedWaterQuadBlock(struct PushBuffer *pb, struct PrimMem *primMem, struct mesh_info *mesh, struct QuadBlock *block)
{
	return DrawLevelOvr1P_DrawRenderedWaterQuadBlockWithDefaultHandler(pb, primMem, mesh, block, 0x800a2fa0);
}

static int Ovr226_800a2904_DrawWaterRenderedListWithDefaultHandler(struct QuadBlock **renderedList, struct PushBuffer *pb, struct mesh_info *mesh,
                                                                   struct PrimMem *primMem, u32 defaultHandlerAddress)
{
	if (renderedList == NULL)
	{
		return 1;
	}

	while (1)
	{
		struct QuadBlock *block = *renderedList;

		if (!DrawLevelOvr1P_HasBucketPrimReserve(primMem, DRAW_LEVEL_OVR1P_BUCKET_RESERVE_DEFAULT))
		{
			return 0;
		}

		if (block == NULL)
		{
			*CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_WATER_RENDERED_SENTINEL_OFFSET) = 0;
			return 1;
		}

		if (!DrawLevelOvr1P_DrawRenderedWaterQuadBlockWithDefaultHandler(pb, primMem, mesh, block, defaultHandlerAddress))
		{
			return 0;
		}

		renderedList++;
	}
}

static int Ovr226_800a2904_DrawWaterRenderedList(struct QuadBlock **renderedList, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem)
{
	return Ovr226_800a2904_DrawWaterRenderedListWithDefaultHandler(renderedList, pb, mesh, primMem, 0x800a2fa0);
}

static int DrawLevelOvr1P_DrawRenderedQuadBlocks(struct QuadBlock **renderedList, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                                 int role)
{
	if (renderedList == NULL)
	{
		return 1;
	}

	if (role == DRAW_LEVEL_OVR1P_BUCKET_4X1_RENDERED)
	{
		return Ovr226_800a417c_DrawGround4x1RenderedList(renderedList, pb, mesh, primMem);
	}

	if (role == DRAW_LEVEL_OVR1P_BUCKET_4X2_RENDERED)
	{
		return Ovr226_800a5e5c_DrawGround4x2RenderedList(renderedList, pb, mesh, primMem);
	}

	if (role == DRAW_LEVEL_OVR1P_BUCKET_DYNAMIC_RENDERED)
	{
		return Ovr226_800a7ba8_DrawDynamicRenderedList(renderedList, pb, mesh, primMem);
	}

	if (role == DRAW_LEVEL_OVR1P_BUCKET_4X4_RENDERED)
	{
		return Ovr226_800a97c8_DrawQuad4x4RenderedList(renderedList, pb, mesh, primMem);
	}

	if (role == DRAW_LEVEL_OVR1P_BUCKET_WATER_RENDERED)
	{
		return Ovr226_800a2904_DrawWaterRenderedList(renderedList, pb, mesh, primMem);
	}

	return 0;
}

static void *DrawLevelOvr1P_GetRenderListField(struct DrawLevelOvr1PRenderList *renderList, int offset)
{
	return *(void **)((u8 *)renderList + offset);
}

static void Ovr226_800a0d34_SetEntryGteAndCameraScratch(struct PushBuffer *pb)
{
	s16 *data6 = DrawLevelOvr1P_Scratch()->projectedCenter.v;

	for (int i = 0; i < 8; i++)
	{
		CTC2(DrawLevelOvr1P_ReadWord(&pb->matrix_ViewProj, (u32)(i * 4)), i);
	}

	for (int i = 0; i < 3; i++)
	{
		data6[i] = (s16)((u16)(u8)pb->data6[i * 2] | ((u16)(u8)pb->data6[i * 2 + 1] << 8));
	}

	CTC2((u32)(s32)pb->rect.w << 15, 24);
	CTC2((u32)(s32)pb->rect.h << 15, 25);
	CTC2((u32)pb->distanceToScreen_PREV, 26);

	// NOTE(aalhendi): Retail stores `(distanceToScreen_PREV >> 1) + 1` at scratch 0x5c for 4x1 depth clip flags.
	DrawLevelOvr1P_Scratch()->depthClipThreshold = (pb->distanceToScreen_PREV >> 1) + 1;
	DrawLevelOvr1P_Scratch()->clipWindowPacked = DrawLevelOvr1P_ReadWord(&pb->rect, 4);
}

static const struct DrawLevelOvr1PBucket *Ovr226_800a0e78_FindBucketByHandler(u32 handlerAddress)
{
	for (int i = 0; i < OVR226_BUCKET_COUNT; i++)
	{
		if (R226.bucketHandlerAddresses[i] == handlerAddress)
		{
			return &sDrawLevelOvr1PBuckets[i];
		}
	}

	return NULL;
}

static int Ovr226_800a0e78_DispatchBucketHandler(u32 handlerAddress, void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                                 const int *visFaceList)
{
	const struct DrawLevelOvr1PBucket *bucket = Ovr226_800a0e78_FindBucketByHandler(handlerAddress);

	if (bucket == NULL)
	{
		return 0;
	}

	if (bucket->kind == DRAW_LEVEL_OVR1P_BUCKET_QUADBLOCKS_RENDERED)
	{
		return DrawLevelOvr1P_DrawRenderedQuadBlocks(bucketValue, pb, mesh, primMem, bucket->role);
	}

	if (bucket->role == DRAW_LEVEL_OVR1P_BUCKET_FULL_DYNAMIC_LIST)
	{
		return Ovr226_800a0ef4_DrawFullDynamicBspList(bucketValue, pb, mesh, primMem, visFaceList);
	}

	if (bucket->role == DRAW_LEVEL_OVR1P_BUCKET_WATER_LIST)
	{
		return Ovr226_800a1e30_DrawWaterBspList(bucketValue, pb, mesh, primMem, visFaceList);
	}

	// NOTE(aalhendi): Overlay 226 consumes VisMem BSP-list nodes: word zero is
	// next, word one is the BSP pointer preserved by VisMem initialization.
	if (bucket->role == DRAW_LEVEL_OVR1P_BUCKET_4X1_LIST)
	{
		return Ovr226_800a36a8_DrawGround4x1BspList(bucketValue, pb, mesh, primMem, visFaceList);
	}

	if (bucket->role == DRAW_LEVEL_OVR1P_BUCKET_4X2_LIST)
	{
		return Ovr226_800a4fa0_DrawGround4x2BspList(bucketValue, pb, mesh, primMem, visFaceList);
	}

	if (bucket->role == DRAW_LEVEL_OVR1P_BUCKET_DYNAMIC_LIST)
	{
		return Ovr226_800a6f40_DrawDynamicBspList(bucketValue, pb, mesh, primMem, visFaceList);
	}

	if (bucket->role == DRAW_LEVEL_OVR1P_BUCKET_4X4_LIST)
	{
		return Ovr226_800a8b60_DrawWideDynamicBspList(bucketValue, pb, mesh, primMem, visFaceList);
	}

	return 0;
}

static int Ovr226_800a0e10_DispatchBucketTable(struct DrawLevelOvr1PRenderList *renderList, struct PushBuffer *pb, struct mesh_info *mesh,
                                               struct PrimMem *primMem, const int *visFaceList)
{
	for (s32 renderListOffset = 0x28; renderListOffset >= 0; renderListOffset -= (s32)sizeof(u32))
	{
		u32 bucketIndex = (u32)renderListOffset / sizeof(u32);
		const struct DrawLevelOvr1PBucket *bucket = &sDrawLevelOvr1PBuckets[bucketIndex];
		void *bucketValue = DrawLevelOvr1P_GetRenderListField(renderList, renderListOffset);
		u32 setupAddress = R226.bucketSetupAddresses[bucketIndex];
		u32 handlerAddress = R226.bucketHandlerAddresses[bucketIndex];

		DrawLevelOvr1P_Scratch()->currentBucketOffset = (u32)renderListOffset;

		if (bucketValue == NULL)
		{
			// NOTE(aalhendi): Retail 0x800a0e7c clears the shared rendered
			// quadblock destination before advancing past an empty bucket.
			DrawLevelOvr1P_ClearRenderedListForRole(renderList, bucket->role);
			continue;
		}

		Ovr226_800a0e44_ApplyBucketSetup(setupAddress);

		if (!Ovr226_800a0e78_DispatchBucketHandler(handlerAddress, bucketValue, pb, mesh, primMem, visFaceList))
		{
			return 0;
		}
	}

	return 1;
}

static int Ovr226_800a0cbc_Entry(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10,
                                 const struct TextureLayout *waterEnvMap)
{
	struct DrawLevelOvr1PRenderList *renderList = LevRenderList;
	struct mesh_info *mesh = (struct mesh_info *)bspList;
	const int *visFaceList = VisMem10;
	u32 hostStackAnchor;

	// NOTE(aalhendi): Retail 0x800a0cf0 saves `sp` in scratch 0x38 and
	// restores it at 0x800a0eb8. Native records a stack anchor only so later
	// scratch users see the same entry-owned word; the host ABI owns SP.
	DrawLevelOvr1P_Scratch()->savedStackPtr32 = (u32)(uintptr_t)&hostStackAnchor;

	DrawLevelOvr1P_Scratch()->primMemEndPtr32 = (u32)(uintptr_t)primMem->end;
	DrawLevelOvr1P_Scratch()->visFaceListPtr32 = (u32)(uintptr_t)VisMem10;

	if (VisMem10 == NULL)
	{
		return 1;
	}

	DrawLevelOvr1P_Scratch()->waterEnvMapPtr32 = (u32)(uintptr_t)waterEnvMap;

	if (mesh->ptrQuadBlockArray == NULL)
	{
		return 1;
	}

	DrawLevelOvr1P_SetClipRecordStart(data.PtrClipBuffer[0]);
	DrawLevelOvr1P_SetRenderedOverflowBase(sdata_static.quadBlocksRendered);
	DrawLevelOvr1P_SetPrimReserveBias(0);
	DrawLevelOvr1P_SetListHandlersSeedRenderedCursor(1);
	Ovr226_800a0d20_SeedEntryScratchPointers(renderList, pb);
	Ovr226_800a0d34_SetEntryGteAndCameraScratch(pb);
	Ovr226_800a0dc4_ClearProjectedScratch();
	Ovr226_800a0ddc_CopyScratchInitTable();

	if (!Ovr226_800a0e10_DispatchBucketTable(renderList, pb, mesh, primMem, visFaceList))
	{
		return 0;
	}

	// NOTE(aalhendi): Retail 0x800a0e98 reloads 0x800ab910 into scratch
	// 0x240 immediately before 0x800aa790 consumes clipped records.
	Ovr226_800ab3dc_CopyClipRecordJumpTable();
	if (!DrawLevelOvr1P_ConsumeClipRecords(pb, primMem))
	{
		return 0;
	}

	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800a0cbc-0x800ab970
void DrawLevelOvr1P(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10,
                    const struct TextureLayout *waterEnvMap)
{
	(void)Ovr226_800a0cbc_Entry(LevRenderList, pb, bspList, primMem, VisMem10, waterEnvMap);
}

void DrawLevelOvr3P(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *VisMem14, void *VisMem18,
                    const struct TextureLayout *waterEnvMap)
{
	(void)Ovr228_800a0cbc_Entry(LevRenderList, pb, bspList, primMem, VisMem10, VisMem14, VisMem18, waterEnvMap);
}

void DrawLevelOvr4P(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *VisMem14, void *VisMem18,
                    void *VisMem1C, const struct TextureLayout *waterEnvMap)
{
	(void)Ovr229_800a0cbc_Entry(LevRenderList, pb, bspList, primMem, VisMem10, VisMem14, VisMem18, VisMem1C, waterEnvMap);
}
