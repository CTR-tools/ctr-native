#include <common.h>

typedef int (*Ovr228BucketDispatch)(u32 handlerAddress, void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                    const int *visFaceList);
typedef int (*Ovr228ClipConsumer)(struct PushBuffer *pb, struct PrimMem *primMem, u8 *clipCursor, int playerIndex);

enum Ovr228DrawLevelConstants
{
	OVR228_WATER_BSP_LIST_HANDLER = 0x800a10c4,
	OVR228_WATER_RENDERED_HANDLER = 0x800a1b88,
	OVR228_SPLIT_GROUND_LIST_A_HANDLER = 0x800a2928,
	OVR228_SPLIT_GROUND_RENDERED_A_HANDLER = 0x800a37b8,
	OVR228_SPLIT_GROUND_LIST_B_HANDLER = 0x800a4768,
	OVR228_SPLIT_GROUND_RENDERED_B_HANDLER = 0x800a55f8,
	OVR228_WIDE_DYNAMIC_HANDLER = 0x800a65a8,
	OVR228_QUAD_4X4_RENDERED_HANDLER = 0x800a71fc,
	OVR228_WATER_RENDERED_DEFAULT_WRAPPER = 0x800a2224,
	OVR228_WATER_BSP_LIST_PRIM_RESERVE_BIAS = 0x1040,
	OVR228_SPLIT_GROUND_LIST_B_PRIM_RESERVE_BIAS = 0x16c0,
	OVR228_TERMINAL_PRIM_RESERVE_BIAS = 0x1a00,
	OVR228_SPLIT_GROUND_RENDERED_A_SETUP_INDEX = 3,
	OVR228_SPLIT_GROUND_LIST_B_SETUP_INDEX = 4,
	OVR228_SPLIT_GROUND_RENDERED_B_SETUP_INDEX = 5,
	OVR228_WIDE_DYNAMIC_SETUP_INDEX = 6,
	OVR228_QUAD_4X4_RENDERED_SETUP_INDEX = 7,
	OVR228_CANONICAL_GROUND_4X2_LIST_SETUP_INDEX = 5,
	OVR228_CANONICAL_GROUND_4X2_RENDERED_SETUP_INDEX = 6,
	OVR228_CANONICAL_DYNAMIC_RENDERED_SETUP_INDEX = 8,
	OVR228_CANONICAL_WIDE_DYNAMIC_SETUP_INDEX = 9,
	OVR228_CANONICAL_QUAD_4X4_RENDERED_SETUP_INDEX = 10,
};

static const struct OverlayRDATA_228_BucketSetupRecord *Ovr228_800a0fb8_FindBucketSetupRecord(u32 setupAddress, int *setupIndex)
{
	for (int i = 0; i < OVR228_BUCKET_COUNT; i++)
	{
		u32 recordAddress = OVR228_RDATA_BUCKET_SETUP_BASE + (u32)(i * sizeof(R228.bucketSetups[0]));

		if (recordAddress == setupAddress)
		{
			if (setupIndex != NULL)
			{
				*setupIndex = i;
			}

			return &R228.bucketSetups[i];
		}
	}

	return NULL;
}

static const u32 *Ovr228_800a0fb8_GetCopySource(const struct OverlayRDATA_228_BucketSetupRecord *setup, int setupIndex,
                                                const struct OverlayRDATA_228_BucketSetupCopy *copy)
{
	u32 recordAddress = OVR228_RDATA_BUCKET_SETUP_BASE + (u32)(setupIndex * sizeof(R228.bucketSetups[0]));
	u32 copy0Address = recordAddress + OFFSETOF(struct OverlayRDATA_228_BucketSetupRecord, copy0);
	u32 copy1Address = recordAddress + OFFSETOF(struct OverlayRDATA_228_BucketSetupRecord, copy1);

	if (copy->sourceAddress == copy0Address)
	{
		return setup->copy0;
	}

	if (copy->sourceAddress == copy1Address)
	{
		return setup->copy1;
	}

	return NULL;
}

static u32 Ovr228_800a0fb8_TranslateCopiedWord(int setupIndex, const struct OverlayRDATA_228_BucketSetupCopy *copy, u32 wordIndex, u32 value)
{
	int canonicalSetupIndex = -1;

	if (setupIndex == OVR228_SPLIT_GROUND_RENDERED_A_SETUP_INDEX)
	{
		canonicalSetupIndex = OVR228_CANONICAL_DYNAMIC_RENDERED_SETUP_INDEX;
	}
	else if (setupIndex == OVR228_SPLIT_GROUND_LIST_B_SETUP_INDEX)
	{
		canonicalSetupIndex = OVR228_CANONICAL_GROUND_4X2_LIST_SETUP_INDEX;
	}
	else if (setupIndex == OVR228_SPLIT_GROUND_RENDERED_B_SETUP_INDEX)
	{
		canonicalSetupIndex = OVR228_CANONICAL_GROUND_4X2_RENDERED_SETUP_INDEX;
	}
	else if (setupIndex == OVR228_WIDE_DYNAMIC_SETUP_INDEX)
	{
		canonicalSetupIndex = OVR228_CANONICAL_WIDE_DYNAMIC_SETUP_INDEX;
	}
	else if (setupIndex == OVR228_QUAD_4X4_RENDERED_SETUP_INDEX)
	{
		canonicalSetupIndex = OVR228_CANONICAL_QUAD_4X4_RENDERED_SETUP_INDEX;
	}
	else
	{
		return value;
	}

	// NOTE(aalhendi): Some 228 helper/direct labels overlap 229 virtual labels.
	// Keep R228 exact, then alias copied scratch setup words to canonical 226
	// helper tables only after target objdump proof.
	if ((copy->scratchOffset == 0x14c) && (wordIndex < OVR226_SETUP_COPY0_WORD_COUNT))
	{
		return R226.bucketSetups[canonicalSetupIndex].copy0[wordIndex];
	}

	if ((copy->scratchOffset == 0x188) && (wordIndex < OVR226_SETUP_COPY1_WORD_COUNT))
	{
		return R226.bucketSetups[canonicalSetupIndex].copy1[wordIndex];
	}

	return value;
}

static u32 Ovr228_800a8e08_TranslateClipRecordLabel(u32 address)
{
	for (int i = 0; i < OVR228_CLIP_RECORD_JUMP_WORD_COUNT; i++)
	{
		if (R228.clipRecordJumpTable[i] == address)
		{
			return R226.clipRecordJumpTable[i];
		}
	}

	return address;
}

static void Ovr228_800a0fb8_CopyScratchWords(const struct OverlayRDATA_228_BucketSetupRecord *setup, int setupIndex,
                                             const struct OverlayRDATA_228_BucketSetupCopy *copy)
{
	const u32 *source = Ovr228_800a0fb8_GetCopySource(setup, setupIndex, copy);
	u32 *scratch = CTR_SCRATCHPAD_PTR(u32, copy->scratchOffset);

	if (source == NULL)
	{
		return;
	}

	for (u32 i = 0; i <= copy->loopCounter; i++)
	{
		scratch[i] = Ovr228_800a0fb8_TranslateCopiedWord(setupIndex, copy, i, source[i]);
	}
}

static void Ovr228_800a0fb8_ApplyBucketSetup(u32 setupAddress, u32 handlerAddress)
{
	int setupIndex = -1;
	const struct OverlayRDATA_228_BucketSetupRecord *setup = Ovr228_800a0fb8_FindBucketSetupRecord(setupAddress, &setupIndex);

	if (setup != NULL)
	{
		for (int i = 0; i < 2; i++)
		{
			const struct OverlayRDATA_228_BucketSetupCopy *copy = &setup->copies[i];

			if (copy->loopCounter == 0)
			{
				break;
			}

			Ovr228_800a0fb8_CopyScratchWords(setup, setupIndex, copy);
		}
	}

	DrawLevelOvr1P_Scratch()->currentHandlerAddress = handlerAddress;
}

static void Ovr228_800a0d68_CopyScratchInitTable(void)
{
	u32 *scratch = CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_SCRATCH_INIT_TABLE_OFFSET);

	for (int i = 0; i < OVR228_SCRATCH_INIT_WORD_COUNT; i++)
	{
		scratch[i] = R228.scratchInitTable[i];
	}
}

static void Ovr228_800a8e08_CopyClipRecordJumpTable(void)
{
	u32 *clipRecordJumpTable = CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_GT3_CLIP_RECORD_JUMP_TABLE_OFFSET);

	for (int i = 0; i < OVR228_CLIP_RECORD_JUMP_WORD_COUNT; i++)
	{
		clipRecordJumpTable[i] = Ovr228_800a8e08_TranslateClipRecordLabel(R228.clipRecordJumpTable[i]);
	}
}

static void Ovr228_800a1014_SetViewportContext(struct PushBuffer *pb, const int *visFaceList, u8 *clipStart, u8 *clipCursor,
                                               struct QuadBlock **renderedOverflowBase)
{
	DrawLevelOvr1P_Scratch()->renderedOverflowPtr32 = (u32)(uintptr_t)renderedOverflowBase;
	DrawLevelOvr1P_Scratch()->clipCursorPtr32 = (u32)(uintptr_t)clipCursor;
	DrawLevelOvr1P_Scratch()->visFaceListPtr32 = (u32)(uintptr_t)visFaceList;
	DrawLevelOvr1P_SetClipRecordStart(clipStart);
	DrawLevelOvr1P_SetRenderedOverflowBase(renderedOverflowBase);
	Ovr226_800a0d34_SetEntryGteAndCameraScratch(pb);
}

static void Ovr228_ClearRenderedOverflowBase(int playerIndex)
{
	struct QuadBlock **renderedOverflowBase = (struct QuadBlock **)data.ptrRenderedQuadblockDestination_forEachPlayer[playerIndex];

	if (renderedOverflowBase != NULL)
	{
		*renderedOverflowBase = NULL;
	}
}

static int Ovr228_DrawViewportBucket(struct DrawLevelOvr1PRenderList *renderList, s32 renderListOffset, struct PushBuffer *pb, struct mesh_info *mesh,
                                     struct PrimMem *primMem, const int *visFaceList, u8 **clipCursor, int playerIndex, int applySetup,
                                     Ovr228BucketDispatch dispatch, int *didDispatch)
{
	u32 bucketIndex = (u32)renderListOffset / sizeof(u32);
	void *bucketValue = DrawLevelOvr1P_GetRenderListField(renderList, renderListOffset);
	u32 setupAddress = R228.bucketSetupAddresses[bucketIndex];
	u32 handlerAddress = R228.bucketHandlerAddresses[bucketIndex];
	struct QuadBlock **renderedOverflowBase = (struct QuadBlock **)data.ptrRenderedQuadblockDestination_forEachPlayer[playerIndex];

	*didDispatch = 0;

	if (bucketValue == NULL)
	{
		Ovr228_ClearRenderedOverflowBase(playerIndex);
		return 1;
	}

	if (applySetup)
	{
		Ovr228_800a0fb8_ApplyBucketSetup(setupAddress, handlerAddress);
	}

	Ovr228_800a1014_SetViewportContext(pb, visFaceList, data.PtrClipBuffer[playerIndex], *clipCursor, renderedOverflowBase);
	if (!dispatch(handlerAddress, bucketValue, pb, mesh, primMem, visFaceList))
	{
		return 0;
	}

	*clipCursor = DrawLevelOvr1P_GetClipRecordCursor();
	*didDispatch = 1;
	return 1;
}

static int Ovr228_800a0d9c_DispatchBucketTable(struct DrawLevelOvr1PRenderList *renderLists, struct PushBuffer *pushBuffers, struct mesh_info *mesh,
                                               struct PrimMem *primMem, const int *visFaceList0, const int *visFaceList1, const int *visFaceList2,
                                               u8 **clipCursors, Ovr228BucketDispatch dispatch)
{
	for (s32 renderListOffset = 0x1c; renderListOffset >= 0; renderListOffset -= (s32)sizeof(u32))
	{
		int setupApplied = 0;
		int didDispatch = 0;

		DrawLevelOvr1P_Scratch()->currentBucketOffset = (u32)renderListOffset;

		if (!Ovr228_DrawViewportBucket(&renderLists[0], renderListOffset, &pushBuffers[0], mesh, primMem, visFaceList0, &clipCursors[0], 0, 1, dispatch,
		                               &didDispatch))
		{
			return 0;
		}
		setupApplied |= didDispatch;

		if (!Ovr228_DrawViewportBucket(&renderLists[1], renderListOffset, &pushBuffers[1], mesh, primMem, visFaceList1, &clipCursors[1], 1, !setupApplied,
		                               dispatch, &didDispatch))
		{
			return 0;
		}
		setupApplied |= didDispatch;

		if (!Ovr228_DrawViewportBucket(&renderLists[2], renderListOffset, &pushBuffers[2], mesh, primMem, visFaceList2, &clipCursors[2], 2, !setupApplied,
		                               dispatch, &didDispatch))
		{
			return 0;
		}
	}

	return 1;
}

static int Ovr228_ConsumeClipRecordsForViewport(struct PushBuffer *pb, struct PrimMem *primMem, u8 *clipCursor, int playerIndex, Ovr228ClipConsumer consume)
{
	u8 *start = data.PtrClipBuffer[playerIndex];

	DrawLevelOvr1P_SetClipRecordStart(start);
	DrawLevelOvr1P_SetClipRecordCursor(clipCursor);
	return consume(pb, primMem, clipCursor, playerIndex);
}

static int Ovr228_800a10c4_DrawWaterBspList(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem, const int *visFaceList)
{
	DrawLevelOvr1P_SetPrimReserveBias(OVR228_WATER_BSP_LIST_PRIM_RESERVE_BIAS);
	return Ovr226_800a1e30_DrawWaterBspList((struct VisMemBspListNode *)bucketValue, pb, mesh, primMem, visFaceList);
}

static int Ovr228_800a1b88_DrawWaterRenderedList(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem)
{
	DrawLevelOvr1P_SetPrimReserveBias(OVR228_WATER_BSP_LIST_PRIM_RESERVE_BIAS);
	return Ovr226_800a2904_DrawWaterRenderedListWithDefaultHandler((struct QuadBlock **)bucketValue, pb, mesh, primMem, OVR228_WATER_RENDERED_DEFAULT_WRAPPER);
}

static int Ovr228_800a2928_DrawSplitGroundListA(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                                const int *visFaceList)
{
	DrawLevelOvr1P_SetPrimReserveBias(OVR228_WATER_BSP_LIST_PRIM_RESERVE_BIAS);
	return DrawLevelOvr1P_DrawSplitGroundListABspList((struct VisMemBspListNode *)bucketValue, pb, mesh, primMem, visFaceList);
}

static int Ovr228_800a37b8_DrawSplitGroundRenderedA(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem)
{
	DrawLevelOvr1P_SetPrimReserveBias(OVR228_WATER_BSP_LIST_PRIM_RESERVE_BIAS);
	DrawLevelOvr1P_SetSplitGroundThresholdScratch();
	return Ovr226_800a7ba8_DrawDynamicRenderedList((struct QuadBlock **)bucketValue, pb, mesh, primMem);
}

static int Ovr228_800a4768_DrawSplitGroundListB(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                                const int *visFaceList)
{
	int result;

	DrawLevelOvr1P_SetPrimReserveBias(OVR228_SPLIT_GROUND_LIST_B_PRIM_RESERVE_BIAS);
	DrawLevelOvr1P_SetSplitGroundThresholdScratch();
	DrawLevelOvr1P_SetMosaicReloadSpanOverride(DRAW_LEVEL_OVR1P_SPLIT_GROUND_MOSAIC_RELOAD_SPAN);
	result = Ovr226_800a4fa0_DrawGround4x2BspList((struct VisMemBspListNode *)bucketValue, pb, mesh, primMem, visFaceList);
	DrawLevelOvr1P_SetMosaicReloadSpanOverride(0);
	return result;
}

static int Ovr228_800a55f8_DrawSplitGroundRenderedB(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem)
{
	int result;

	DrawLevelOvr1P_SetPrimReserveBias(OVR228_SPLIT_GROUND_LIST_B_PRIM_RESERVE_BIAS);
	DrawLevelOvr1P_SetSplitGroundThresholdScratch();
	DrawLevelOvr1P_SetMosaicReloadSpanOverride(DRAW_LEVEL_OVR1P_SPLIT_GROUND_MOSAIC_RELOAD_SPAN);
	result = Ovr226_800a5e5c_DrawGround4x2RenderedList((struct QuadBlock **)bucketValue, pb, mesh, primMem);
	DrawLevelOvr1P_SetMosaicReloadSpanOverride(0);
	return result;
}

static int Ovr228_800a65a8_DrawWideDynamicList(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                               const int *visFaceList)
{
	DrawLevelOvr1P_SetPrimReserveBias(OVR228_WATER_BSP_LIST_PRIM_RESERVE_BIAS);
	DrawLevelOvr1P_SetSplitGroundThresholdScratch();
	return Ovr226_800a8b60_DrawWideDynamicBspList((struct VisMemBspListNode *)bucketValue, pb, mesh, primMem, visFaceList);
}

static int Ovr228_800a71fc_DrawQuad4x4RenderedList(void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem)
{
	DrawLevelOvr1P_SetPrimReserveBias(OVR228_WATER_BSP_LIST_PRIM_RESERVE_BIAS);
	DrawLevelOvr1P_SetSplitGroundThresholdScratch();
	return Ovr226_800a97c8_DrawQuad4x4RenderedList((struct QuadBlock **)bucketValue, pb, mesh, primMem);
}

static int Ovr228_800a10c4_800a81bc_BucketDispatch(u32 handlerAddress, void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh,
                                                   struct PrimMem *primMem, const int *visFaceList)
{
	if (handlerAddress == OVR228_WATER_BSP_LIST_HANDLER)
	{
		return Ovr228_800a10c4_DrawWaterBspList(bucketValue, pb, mesh, primMem, visFaceList);
	}

	if (handlerAddress == OVR228_WATER_RENDERED_HANDLER)
	{
		return Ovr228_800a1b88_DrawWaterRenderedList(bucketValue, pb, mesh, primMem);
	}

	if (handlerAddress == OVR228_SPLIT_GROUND_LIST_A_HANDLER)
	{
		return Ovr228_800a2928_DrawSplitGroundListA(bucketValue, pb, mesh, primMem, visFaceList);
	}

	if (handlerAddress == OVR228_SPLIT_GROUND_RENDERED_A_HANDLER)
	{
		return Ovr228_800a37b8_DrawSplitGroundRenderedA(bucketValue, pb, mesh, primMem);
	}

	if (handlerAddress == OVR228_SPLIT_GROUND_LIST_B_HANDLER)
	{
		return Ovr228_800a4768_DrawSplitGroundListB(bucketValue, pb, mesh, primMem, visFaceList);
	}

	if (handlerAddress == OVR228_SPLIT_GROUND_RENDERED_B_HANDLER)
	{
		return Ovr228_800a55f8_DrawSplitGroundRenderedB(bucketValue, pb, mesh, primMem);
	}

	if (handlerAddress == OVR228_WIDE_DYNAMIC_HANDLER)
	{
		return Ovr228_800a65a8_DrawWideDynamicList(bucketValue, pb, mesh, primMem, visFaceList);
	}

	if (handlerAddress == OVR228_QUAD_4X4_RENDERED_HANDLER)
	{
		return Ovr228_800a71fc_DrawQuad4x4RenderedList(bucketValue, pb, mesh, primMem);
	}

	// NOTE(aalhendi): Reject handler addresses that are not present in the
	// recovered R228 bucket table.
	return 0;
}

static int Ovr228_800a81bc_ConsumeClipRecords(struct PushBuffer *pb, struct PrimMem *primMem, u8 *clipCursor, int playerIndex)
{
	(void)clipCursor;
	(void)playerIndex;

	DrawLevelOvr1P_SetPrimReserveBias(OVR228_TERMINAL_PRIM_RESERVE_BIAS);
	return DrawLevelOvr1P_ConsumeClipRecords(pb, primMem);
}

static int Ovr228_800a0cbc_EntryWithCallbacks(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10,
                                              void *VisMem14, void *VisMem18, struct TextureLayout* waterEnvMap, Ovr228BucketDispatch dispatch, Ovr228ClipConsumer consume)
{
	struct DrawLevelOvr1PRenderList *renderLists = LevRenderList;
	struct mesh_info *mesh = (struct mesh_info *)bspList;
	const int *visFaceList0 = VisMem10;
	const int *visFaceList1 = VisMem14;
	const int *visFaceList2 = VisMem18;
	u8 *clipCursors[3];
	u32 hostStackAnchor;

	// NOTE(aalhendi): ASM-audited against NTSC-U 926 228 entry/setup
	// 0x800a0cbc-0x800a10c4. Runtime proof is tracked separately from
	// source ownership and public route promotion.
	DrawLevelOvr1P_Scratch()->savedStackPtr32 = (u32)(uintptr_t)&hostStackAnchor;
	DrawLevelOvr1P_Scratch()->visFaceListArgPtr32[0] = (u32)(uintptr_t)VisMem10;
	if (VisMem10 == NULL)
	{
		return 1;
	}

	DrawLevelOvr1P_Scratch()->visFaceListArgPtr32[1] = (u32)(uintptr_t)VisMem14;
	if (VisMem14 == NULL)
	{
		return 1;
	}

	DrawLevelOvr1P_Scratch()->visFaceListArgPtr32[2] = (u32)(uintptr_t)VisMem18;
	if (VisMem18 == NULL)
	{
		return 1;
	}

	DrawLevelOvr1P_Scratch()->waterEnvMapPtr32 = waterEnvMap;
	DrawLevelOvr1P_Scratch()->primMemEndPtr32 = (u32)(uintptr_t)primMem->end;

	if (mesh->ptrQuadBlockArray == NULL)
	{
		return 1;
	}

	clipCursors[0] = data.PtrClipBuffer[0];
	clipCursors[1] = data.PtrClipBuffer[1];
	clipCursors[2] = data.PtrClipBuffer[2];

	DrawLevelOvr1P_Scratch()->pushBufferPtr32[0] = (u32)(uintptr_t)&pb[0];
	DrawLevelOvr1P_Scratch()->pushBufferPtr32[1] = (u32)(uintptr_t)&pb[1];
	DrawLevelOvr1P_Scratch()->pushBufferPtr32[2] = (u32)(uintptr_t)&pb[2];
	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[0] = (u32)(uintptr_t)clipCursors[0];
	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[1] = (u32)(uintptr_t)clipCursors[1];
	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[2] = (u32)(uintptr_t)clipCursors[2];

	DrawLevelOvr1P_SetPrimReserveBias(0);
	DrawLevelOvr1P_SetListHandlersSeedRenderedCursor(0);
	Ovr226_800a0dc4_ClearProjectedScratch();
	Ovr228_800a0d68_CopyScratchInitTable();
	DrawLevelOvr1P_Scratch()->renderListPtr32 = (u32)(uintptr_t)LevRenderList;

	if (!Ovr228_800a0d9c_DispatchBucketTable(renderLists, pb, mesh, primMem, visFaceList0, visFaceList1, visFaceList2, clipCursors, dispatch))
	{
		return 0;
	}

	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[0] = (u32)(uintptr_t)clipCursors[0];
	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[1] = (u32)(uintptr_t)clipCursors[1];
	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[2] = (u32)(uintptr_t)clipCursors[2];

	Ovr228_800a8e08_CopyClipRecordJumpTable();
	if (!Ovr228_ConsumeClipRecordsForViewport(&pb[0], primMem, clipCursors[0], 0, consume))
	{
		return 0;
	}

	if (!Ovr228_ConsumeClipRecordsForViewport(&pb[1], primMem, clipCursors[1], 1, consume))
	{
		return 0;
	}

	if (!Ovr228_ConsumeClipRecordsForViewport(&pb[2], primMem, clipCursors[2], 2, consume))
	{
		return 0;
	}

	return 1;
}

int Ovr228_800a0cbc_Entry(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *VisMem14,
                          void *VisMem18, struct TextureLayout* waterEnvMap)
{
	return Ovr228_800a0cbc_EntryWithCallbacks(LevRenderList, pb, bspList, primMem, VisMem10, VisMem14, VisMem18, waterEnvMap,
	                                          Ovr228_800a10c4_800a81bc_BucketDispatch, Ovr228_800a81bc_ConsumeClipRecords);
}
