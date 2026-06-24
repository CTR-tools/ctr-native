#include <common.h>

typedef int (*Ovr229BucketDispatch)(u32 handlerAddress, void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                    const int *visFaceList);
typedef int (*Ovr229ClipConsumer)(struct PushBuffer *pb, struct PrimMem *primMem, u8 *clipCursor, int playerIndex);

enum Ovr229DrawLevelConstants
{
	OVR229_WATER_BSP_LIST_HANDLER = 0x800a1178,
	OVR229_WATER_RENDERED_HANDLER = 0x800a1c3c,
	OVR229_SPLIT_GROUND_LIST_A_HANDLER = 0x800a29dc,
	OVR229_SPLIT_GROUND_RENDERED_A_HANDLER = 0x800a386c,
	OVR229_SPLIT_GROUND_LIST_B_HANDLER = 0x800a481c,
	OVR229_SPLIT_GROUND_RENDERED_B_HANDLER = 0x800a56ac,
	OVR229_WIDE_DYNAMIC_HANDLER = 0x800a665c,
	OVR229_QUAD_4X4_RENDERED_HANDLER = 0x800a72b0,
	OVR229_WATER_RENDERED_DEFAULT_WRAPPER = 0x800a22d8,
	OVR229_WATER_BSP_LIST_PRIM_RESERVE_BIAS = 0x1d40,
	OVR229_SPLIT_GROUND_LIST_B_PRIM_RESERVE_BIAS = 0x23c0,
	OVR229_TERMINAL_PRIM_RESERVE_BIAS = 0x2700,
	OVR229_SPLIT_GROUND_RENDERED_A_SETUP_INDEX = 3,
	OVR229_SPLIT_GROUND_LIST_B_SETUP_INDEX = 4,
	OVR229_SPLIT_GROUND_RENDERED_B_SETUP_INDEX = 5,
	OVR229_WIDE_DYNAMIC_SETUP_INDEX = 6,
	OVR229_QUAD_4X4_RENDERED_SETUP_INDEX = 7,
	OVR229_CANONICAL_GROUND_4X2_LIST_SETUP_INDEX = 5,
	OVR229_CANONICAL_GROUND_4X2_RENDERED_SETUP_INDEX = 6,
	OVR229_CANONICAL_DYNAMIC_RENDERED_SETUP_INDEX = 8,
	OVR229_CANONICAL_WIDE_DYNAMIC_SETUP_INDEX = 9,
	OVR229_CANONICAL_QUAD_4X4_RENDERED_SETUP_INDEX = 10,
};

static const struct OverlayRDATA_229_BucketSetupRecord *Ovr229_800a106c_FindBucketSetupRecord(u32 setupAddress, int *setupIndex)
{
	for (int i = 0; i < OVR229_BUCKET_COUNT; i++)
	{
		u32 recordAddress = OVR229_RDATA_BUCKET_SETUP_BASE + (u32)(i * sizeof(R229.bucketSetups[0]));

		if (recordAddress == setupAddress)
		{
			if (setupIndex != NULL)
				*setupIndex = i;

			return &R229.bucketSetups[i];
		}
	}

	return NULL;
}

static const u32 *Ovr229_800a106c_GetCopySource(const struct OverlayRDATA_229_BucketSetupRecord *setup, int setupIndex,
                                                const struct OverlayRDATA_229_BucketSetupCopy *copy)
{
	u32 recordAddress = OVR229_RDATA_BUCKET_SETUP_BASE + (u32)(setupIndex * sizeof(R229.bucketSetups[0]));
	u32 copy0Address = recordAddress + OFFSETOF(struct OverlayRDATA_229_BucketSetupRecord, copy0);
	u32 copy1Address = recordAddress + OFFSETOF(struct OverlayRDATA_229_BucketSetupRecord, copy1);

	if (copy->sourceAddress == copy0Address)
		return setup->copy0;

	if (copy->sourceAddress == copy1Address)
		return setup->copy1;

	return NULL;
}

static u32 Ovr229_800a106c_TranslateCopiedWord(int setupIndex, const struct OverlayRDATA_229_BucketSetupCopy *copy, u32 wordIndex, u32 value)
{
	int canonicalSetupIndex = -1;

	if (setupIndex == OVR229_SPLIT_GROUND_RENDERED_A_SETUP_INDEX)
		canonicalSetupIndex = OVR229_CANONICAL_DYNAMIC_RENDERED_SETUP_INDEX;
	else if (setupIndex == OVR229_SPLIT_GROUND_LIST_B_SETUP_INDEX)
		canonicalSetupIndex = OVR229_CANONICAL_GROUND_4X2_LIST_SETUP_INDEX;
	else if (setupIndex == OVR229_SPLIT_GROUND_RENDERED_B_SETUP_INDEX)
		canonicalSetupIndex = OVR229_CANONICAL_GROUND_4X2_RENDERED_SETUP_INDEX;
	else if (setupIndex == OVR229_WIDE_DYNAMIC_SETUP_INDEX)
		canonicalSetupIndex = OVR229_CANONICAL_WIDE_DYNAMIC_SETUP_INDEX;
	else if (setupIndex == OVR229_QUAD_4X4_RENDERED_SETUP_INDEX)
		canonicalSetupIndex = OVR229_CANONICAL_QUAD_4X4_RENDERED_SETUP_INDEX;
	else
		return value;

	// NOTE(aalhendi): Some 229 helper/direct labels overlap 228 virtual labels.
	// Keep R229 exact, then alias copied scratch setup words to canonical 226
	// helper tables only after target objdump proof.
	if ((copy->scratchOffset == 0x14c) && (wordIndex < OVR226_SETUP_COPY0_WORD_COUNT))
		return R226.bucketSetups[canonicalSetupIndex].copy0[wordIndex];

	if ((copy->scratchOffset == 0x188) && (wordIndex < OVR226_SETUP_COPY1_WORD_COUNT))
		return R226.bucketSetups[canonicalSetupIndex].copy1[wordIndex];

	return value;
}

static u32 Ovr229_800a8ebc_TranslateClipRecordLabel(u32 address)
{
	for (int i = 0; i < OVR229_CLIP_RECORD_JUMP_WORD_COUNT; i++)
	{
		if (R229.clipRecordJumpTable[i] == address)
			return R226.clipRecordJumpTable[i];
	}

	return address;
}

static void Ovr229_800a106c_CopyScratchWords(const struct OverlayRDATA_229_BucketSetupRecord *setup, int setupIndex,
                                             const struct OverlayRDATA_229_BucketSetupCopy *copy)
{
	const u32 *source = Ovr229_800a106c_GetCopySource(setup, setupIndex, copy);
	u32 *scratch = CTR_SCRATCHPAD_PTR(u32, copy->scratchOffset);

	if (source == NULL)
		return;

	for (u32 i = 0; i <= copy->loopCounter; i++)
		scratch[i] = Ovr229_800a106c_TranslateCopiedWord(setupIndex, copy, i, source[i]);
}

static void Ovr229_800a106c_ApplyBucketSetup(u32 setupAddress, u32 handlerAddress)
{
	int setupIndex = -1;
	const struct OverlayRDATA_229_BucketSetupRecord *setup = Ovr229_800a106c_FindBucketSetupRecord(setupAddress, &setupIndex);

	if (setup != NULL)
	{
		for (int i = 0; i < 2; i++)
		{
			const struct OverlayRDATA_229_BucketSetupCopy *copy = &setup->copies[i];

			if (copy->loopCounter == 0)
				break;

			Ovr229_800a106c_CopyScratchWords(setup, setupIndex, copy);
		}
	}

	DrawLevelOvr1P_Scratch()->currentHandlerAddress = handlerAddress;
}

static void Ovr229_800a0da0_CopyScratchInitTable(void)
{
	u32 *scratch = CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_SCRATCH_INIT_TABLE_OFFSET);

	for (int i = 0; i < OVR229_SCRATCH_INIT_WORD_COUNT; i++)
		scratch[i] = R229.scratchInitTable[i];
}

static void Ovr229_800a8ebc_CopyClipRecordJumpTable(void)
{
	u32 *clipRecordJumpTable = CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_GT3_CLIP_RECORD_JUMP_TABLE_OFFSET);

	for (int i = 0; i < OVR229_CLIP_RECORD_JUMP_WORD_COUNT; i++)
		clipRecordJumpTable[i] = Ovr229_800a8ebc_TranslateClipRecordLabel(R229.clipRecordJumpTable[i]);
}

static void Ovr229_800a10c8_SetViewportContext(struct PushBuffer *pb, const int *visFaceList, u8 *clipStart, u8 *clipCursor,
                                               struct QuadBlock **renderedOverflowBase)
{
	DrawLevelOvr1P_Scratch()->renderedOverflowPtr32 = (u32)(uintptr_t)renderedOverflowBase;
	DrawLevelOvr1P_Scratch()->clipCursorPtr32 = (u32)(uintptr_t)clipCursor;
	DrawLevelOvr1P_Scratch()->visFaceListPtr32 = (u32)(uintptr_t)visFaceList;
	DrawLevelOvr1P_SetClipRecordStart(clipStart);
	DrawLevelOvr1P_SetRenderedOverflowBase(renderedOverflowBase);
	Ovr226_800a0d34_SetEntryGteAndCameraScratch(pb);
}

static void Ovr229_ClearRenderedOverflowBase(int playerIndex)
{
	struct QuadBlock **renderedOverflowBase = (struct QuadBlock **)data.ptrRenderedQuadblockDestination_forEachPlayer[playerIndex];

	if (renderedOverflowBase != NULL)
		*renderedOverflowBase = NULL;
}

static int Ovr229_DrawViewportBucket(struct DrawLevelOvr1PRenderList *renderList, s32 renderListOffset, struct PushBuffer *pb, struct mesh_info *mesh,
                                     struct PrimMem *primMem, const int *visFaceList, u8 **clipCursor, int playerIndex, int applySetup,
                                     Ovr229BucketDispatch dispatch, int *didDispatch)
{
	u32 bucketIndex = (u32)renderListOffset / sizeof(u32);
	void *bucketValue = DrawLevelOvr1P_GetRenderListField(renderList, renderListOffset);
	u32 setupAddress = R229.bucketSetupAddresses[bucketIndex];
	u32 handlerAddress = R229.bucketHandlerAddresses[bucketIndex];
	struct QuadBlock **renderedOverflowBase = (struct QuadBlock **)data.ptrRenderedQuadblockDestination_forEachPlayer[playerIndex];

	*didDispatch = 0;

	if (bucketValue == NULL)
	{
		Ovr229_ClearRenderedOverflowBase(playerIndex);
		return 1;
	}

	if (applySetup)
		Ovr229_800a106c_ApplyBucketSetup(setupAddress, handlerAddress);

	Ovr229_800a10c8_SetViewportContext(pb, visFaceList, data.PtrClipBuffer[playerIndex], *clipCursor, renderedOverflowBase);
	if (!dispatch(handlerAddress, bucketValue, pb, mesh, primMem, visFaceList))
		return 0;

	*clipCursor = DrawLevelOvr1P_GetClipRecordCursor();
	*didDispatch = 1;
	return 1;
}

static int Ovr229_800a0dd0_DispatchBucketTable(struct DrawLevelOvr1PRenderList *renderLists, struct PushBuffer *pushBuffers, struct mesh_info *mesh,
                                               struct PrimMem *primMem, const int *visFaceList0, const int *visFaceList1, const int *visFaceList2,
                                               const int *visFaceList3, u8 **clipCursors, Ovr229BucketDispatch dispatch)
{
	for (s32 renderListOffset = 0x1c; renderListOffset >= 0; renderListOffset -= (s32)sizeof(u32))
	{
		int setupApplied = 0;
		int didDispatch = 0;

		DrawLevelOvr1P_Scratch()->currentBucketOffset = (u32)renderListOffset;

		if (!Ovr229_DrawViewportBucket(&renderLists[0], renderListOffset, &pushBuffers[0], mesh, primMem, visFaceList0, &clipCursors[0], 0, 1, dispatch,
		                               &didDispatch))
			return 0;
		setupApplied |= didDispatch;

		if (!Ovr229_DrawViewportBucket(&renderLists[1], renderListOffset, &pushBuffers[1], mesh, primMem, visFaceList1, &clipCursors[1], 1, !setupApplied,
		                               dispatch, &didDispatch))
			return 0;
		setupApplied |= didDispatch;

		if (!Ovr229_DrawViewportBucket(&renderLists[2], renderListOffset, &pushBuffers[2], mesh, primMem, visFaceList2, &clipCursors[2], 2, !setupApplied,
		                               dispatch, &didDispatch))
			return 0;
		setupApplied |= didDispatch;

		if (!Ovr229_DrawViewportBucket(&renderLists[3], renderListOffset, &pushBuffers[3], mesh, primMem, visFaceList3, &clipCursors[3], 3, !setupApplied,
		                               dispatch, &didDispatch))
			return 0;
	}

	return 1;
}

static int Ovr229_ConsumeClipRecordsForViewport(struct PushBuffer *pb, struct PrimMem *primMem, u8 *clipCursor, int playerIndex, Ovr229ClipConsumer consume)
{
	u8 *start = data.PtrClipBuffer[playerIndex];

	DrawLevelOvr1P_SetClipRecordStart(start);
	DrawLevelOvr1P_SetClipRecordCursor(clipCursor);
	return consume(pb, primMem, clipCursor, playerIndex);
}

static int Ovr229_800a1178_DrawWaterBspList(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem, const int *visFaceList)
{
	DrawLevelOvr1P_SetPrimReserveBias(OVR229_WATER_BSP_LIST_PRIM_RESERVE_BIAS);
	return Ovr226_800a1e30_DrawWaterBspList((struct VisMemBspListNode *)bucketValue, pb, mesh, primMem, visFaceList);
}

static int Ovr229_800a1c3c_DrawWaterRenderedList(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem)
{
	DrawLevelOvr1P_SetPrimReserveBias(OVR229_WATER_BSP_LIST_PRIM_RESERVE_BIAS);
	return Ovr226_800a2904_DrawWaterRenderedListWithDefaultHandler((struct QuadBlock **)bucketValue, pb, mesh, primMem, OVR229_WATER_RENDERED_DEFAULT_WRAPPER);
}

static int Ovr229_800a29dc_DrawSplitGroundListA(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                                const int *visFaceList)
{
	DrawLevelOvr1P_SetPrimReserveBias(OVR229_WATER_BSP_LIST_PRIM_RESERVE_BIAS);
	return DrawLevelOvr1P_DrawSplitGroundListABspList((struct VisMemBspListNode *)bucketValue, pb, mesh, primMem, visFaceList);
}

static int Ovr229_800a386c_DrawSplitGroundRenderedA(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem)
{
	DrawLevelOvr1P_SetPrimReserveBias(OVR229_WATER_BSP_LIST_PRIM_RESERVE_BIAS);
	DrawLevelOvr1P_SetSplitGroundThresholdScratch();
	return Ovr226_800a7ba8_DrawDynamicRenderedList((struct QuadBlock **)bucketValue, pb, mesh, primMem);
}

static int Ovr229_800a481c_DrawSplitGroundListB(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                                const int *visFaceList)
{
	int result;

	DrawLevelOvr1P_SetPrimReserveBias(OVR229_SPLIT_GROUND_LIST_B_PRIM_RESERVE_BIAS);
	DrawLevelOvr1P_SetSplitGroundThresholdScratch();
	DrawLevelOvr1P_SetMosaicReloadSpanOverride(DRAW_LEVEL_OVR1P_SPLIT_GROUND_MOSAIC_RELOAD_SPAN);
	result = Ovr226_800a4fa0_DrawGround4x2BspList((struct VisMemBspListNode *)bucketValue, pb, mesh, primMem, visFaceList);
	DrawLevelOvr1P_SetMosaicReloadSpanOverride(0);
	return result;
}

static int Ovr229_800a56ac_DrawSplitGroundRenderedB(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem)
{
	int result;

	DrawLevelOvr1P_SetPrimReserveBias(OVR229_SPLIT_GROUND_LIST_B_PRIM_RESERVE_BIAS);
	DrawLevelOvr1P_SetSplitGroundThresholdScratch();
	DrawLevelOvr1P_SetMosaicReloadSpanOverride(DRAW_LEVEL_OVR1P_SPLIT_GROUND_MOSAIC_RELOAD_SPAN);
	result = Ovr226_800a5e5c_DrawGround4x2RenderedList((struct QuadBlock **)bucketValue, pb, mesh, primMem);
	DrawLevelOvr1P_SetMosaicReloadSpanOverride(0);
	return result;
}

static int Ovr229_800a665c_DrawWideDynamicList(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                               const int *visFaceList)
{
	DrawLevelOvr1P_SetPrimReserveBias(OVR229_WATER_BSP_LIST_PRIM_RESERVE_BIAS);
	DrawLevelOvr1P_SetSplitGroundThresholdScratch();
	return Ovr226_800a8b60_DrawWideDynamicBspList((struct VisMemBspListNode *)bucketValue, pb, mesh, primMem, visFaceList);
}

static int Ovr229_800a72b0_DrawQuad4x4RenderedList(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem)
{
	DrawLevelOvr1P_SetPrimReserveBias(OVR229_WATER_BSP_LIST_PRIM_RESERVE_BIAS);
	DrawLevelOvr1P_SetSplitGroundThresholdScratch();
	return Ovr226_800a97c8_DrawQuad4x4RenderedList((struct QuadBlock **)bucketValue, pb, mesh, primMem);
}

static int Ovr229_800a1178_800a8270_BucketDispatch(u32 handlerAddress, void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh,
                                                   struct PrimMem *primMem, const int *visFaceList)
{
	if (handlerAddress == OVR229_WATER_BSP_LIST_HANDLER)
		return Ovr229_800a1178_DrawWaterBspList(bucketValue, pb, mesh, primMem, visFaceList);

	if (handlerAddress == OVR229_WATER_RENDERED_HANDLER)
		return Ovr229_800a1c3c_DrawWaterRenderedList(bucketValue, pb, mesh, primMem);

	if (handlerAddress == OVR229_SPLIT_GROUND_LIST_A_HANDLER)
		return Ovr229_800a29dc_DrawSplitGroundListA(bucketValue, pb, mesh, primMem, visFaceList);

	if (handlerAddress == OVR229_SPLIT_GROUND_RENDERED_A_HANDLER)
		return Ovr229_800a386c_DrawSplitGroundRenderedA(bucketValue, pb, mesh, primMem);

	if (handlerAddress == OVR229_SPLIT_GROUND_LIST_B_HANDLER)
		return Ovr229_800a481c_DrawSplitGroundListB(bucketValue, pb, mesh, primMem, visFaceList);

	if (handlerAddress == OVR229_SPLIT_GROUND_RENDERED_B_HANDLER)
		return Ovr229_800a56ac_DrawSplitGroundRenderedB(bucketValue, pb, mesh, primMem);

	if (handlerAddress == OVR229_WIDE_DYNAMIC_HANDLER)
		return Ovr229_800a665c_DrawWideDynamicList(bucketValue, pb, mesh, primMem, visFaceList);

	if (handlerAddress == OVR229_QUAD_4X4_RENDERED_HANDLER)
		return Ovr229_800a72b0_DrawQuad4x4RenderedList(bucketValue, pb, mesh, primMem);

	// NOTE(aalhendi): Reject handler addresses that are not present in the
	// recovered R229 bucket table.
	return 0;
}

static int Ovr229_800a8270_ConsumeClipRecords(struct PushBuffer *pb, struct PrimMem *primMem, u8 *clipCursor, int playerIndex)
{
	(void)clipCursor;
	(void)playerIndex;

	DrawLevelOvr1P_SetPrimReserveBias(OVR229_TERMINAL_PRIM_RESERVE_BIAS);
	return DrawLevelOvr1P_ConsumeClipRecords(pb, primMem);
}

static int Ovr229_800a0cbc_EntryWithCallbacks(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10,
                                              void *VisMem14, void *VisMem18, void *VisMem1C, struct TextureLayout* waterEnvMap, Ovr229BucketDispatch dispatch,
                                              Ovr229ClipConsumer consume)
{
	struct DrawLevelOvr1PRenderList *renderLists = LevRenderList;
	struct mesh_info *mesh = (struct mesh_info *)bspList;
	const int *visFaceList0 = VisMem10;
	const int *visFaceList1 = VisMem14;
	const int *visFaceList2 = VisMem18;
	const int *visFaceList3 = VisMem1C;
	u8 *clipCursors[4];
	u32 hostStackAnchor;

	// NOTE(aalhendi): ASM-audited against NTSC-U 926 229 entry/setup
	// 0x800a0cbc-0x800a1178. Runtime proof is tracked separately from
	// source ownership and public route promotion.
	DrawLevelOvr1P_Scratch()->savedStackPtr32 = (u32)(uintptr_t)&hostStackAnchor;
	DrawLevelOvr1P_Scratch()->visFaceListArgPtr32[0] = (u32)(uintptr_t)VisMem10;
	if (VisMem10 == NULL)
		return 1;

	DrawLevelOvr1P_Scratch()->visFaceListArgPtr32[1] = (u32)(uintptr_t)VisMem14;
	if (VisMem14 == NULL)
		return 1;

	DrawLevelOvr1P_Scratch()->visFaceListArgPtr32[2] = (u32)(uintptr_t)VisMem18;
	if (VisMem18 == NULL)
		return 1;

	DrawLevelOvr1P_Scratch()->visFaceListArgPtr32[3] = (u32)(uintptr_t)VisMem1C;
	if (VisMem1C == NULL)
		return 1;

	DrawLevelOvr1P_Scratch()->waterEnvMapPtr32 = waterEnvMap;
	DrawLevelOvr1P_Scratch()->primMemEndPtr32 = (u32)(uintptr_t)primMem->end;

	if (mesh->ptrQuadBlockArray == NULL)
		return 1;

	clipCursors[0] = data.PtrClipBuffer[0];
	clipCursors[1] = data.PtrClipBuffer[1];
	clipCursors[2] = data.PtrClipBuffer[2];
	clipCursors[3] = data.PtrClipBuffer[3];

	DrawLevelOvr1P_Scratch()->pushBufferPtr32[0] = (u32)(uintptr_t)&pb[0];
	DrawLevelOvr1P_Scratch()->pushBufferPtr32[1] = (u32)(uintptr_t)&pb[1];
	DrawLevelOvr1P_Scratch()->pushBufferPtr32[2] = (u32)(uintptr_t)&pb[2];
	DrawLevelOvr1P_Scratch()->pushBufferPtr32[3] = (u32)(uintptr_t)&pb[3];
	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[0] = (u32)(uintptr_t)clipCursors[0];
	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[1] = (u32)(uintptr_t)clipCursors[1];
	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[2] = (u32)(uintptr_t)clipCursors[2];
	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[3] = (u32)(uintptr_t)clipCursors[3];

	DrawLevelOvr1P_SetPrimReserveBias(0);
	DrawLevelOvr1P_SetListHandlersSeedRenderedCursor(0);
	Ovr226_800a0dc4_ClearProjectedScratch();
	Ovr229_800a0da0_CopyScratchInitTable();
	DrawLevelOvr1P_Scratch()->renderListPtr32 = (u32)(uintptr_t)LevRenderList;

	if (!Ovr229_800a0dd0_DispatchBucketTable(renderLists, pb, mesh, primMem, visFaceList0, visFaceList1, visFaceList2, visFaceList3, clipCursors, dispatch))
		return 0;

	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[0] = (u32)(uintptr_t)clipCursors[0];
	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[1] = (u32)(uintptr_t)clipCursors[1];
	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[2] = (u32)(uintptr_t)clipCursors[2];
	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[3] = (u32)(uintptr_t)clipCursors[3];

	Ovr229_800a8ebc_CopyClipRecordJumpTable();
	if (!Ovr229_ConsumeClipRecordsForViewport(&pb[0], primMem, clipCursors[0], 0, consume))
		return 0;

	if (!Ovr229_ConsumeClipRecordsForViewport(&pb[1], primMem, clipCursors[1], 1, consume))
		return 0;

	if (!Ovr229_ConsumeClipRecordsForViewport(&pb[2], primMem, clipCursors[2], 2, consume))
		return 0;

	if (!Ovr229_ConsumeClipRecordsForViewport(&pb[3], primMem, clipCursors[3], 3, consume))
		return 0;

	return 1;
}

int Ovr229_800a0cbc_Entry(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *VisMem14,
                          void *VisMem18, void *VisMem1C, struct TextureLayout* waterEnvMap)
{
	return Ovr229_800a0cbc_EntryWithCallbacks(LevRenderList, pb, bspList, primMem, VisMem10, VisMem14, VisMem18, VisMem1C, waterEnvMap,
	                                          Ovr229_800a1178_800a8270_BucketDispatch, Ovr229_800a8270_ConsumeClipRecords);
}
