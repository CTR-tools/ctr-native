#include <common.h>

static void Ovr227_800ab45c_CopyClipRecordJumpTable(void);

CTR_STATIC_ASSERT(sizeof(struct OverlayRDATA_227) == 0x564);
CTR_STATIC_ASSERT(sizeof(struct OverlayRDATA_227_BucketSetupRecord) == 0x64);
CTR_STATIC_ASSERT(sizeof(((struct OverlayRDATA_227 *)0)->scratchInitTable) == 0x60);
CTR_STATIC_ASSERT(sizeof(((struct OverlayRDATA_227 *)0)->clipRecordJumpTable) == 0x60);

static u32 Ovr227_TranslateCopiedLabel(u32 address)
{
	for (int i = 0; i < OVR227_BUCKET_COUNT; i++)
	{
		for (int j = 0; j < OVR227_SETUP_COPY0_WORD_COUNT; j++)
		{
			if (R227.bucketSetups[i].copy0[j] == address)
			{
				return R226.bucketSetups[i].copy0[j];
			}
		}

		for (int j = 0; j < OVR227_SETUP_COPY1_WORD_COUNT; j++)
		{
			if (R227.bucketSetups[i].copy1[j] == address)
			{
				return R226.bucketSetups[i].copy1[j];
			}
		}
	}

	for (int i = 0; i < OVR227_CLIP_RECORD_JUMP_WORD_COUNT; i++)
	{
		if (R227.clipRecordJumpTable[i] == address)
		{
			return R226.clipRecordJumpTable[i];
		}
	}

	return address;
}

static void Ovr227_CopyScratchWordsTranslated(const u32 *source, const struct OverlayRDATA_227_BucketSetupCopy *copy)
{
	u32 *scratch = CTR_SCRATCHPAD_PTR(u32, copy->scratchOffset);

	for (u32 i = 0; i <= copy->loopCounter; i++)
	{
		scratch[i] = Ovr227_TranslateCopiedLabel(source[i]);
	}
}

static const struct OverlayRDATA_227_BucketSetupRecord *DrawLevelOvr2P_FindBucketSetupRecord(u32 setupAddress)
{
	for (int i = 0; i < OVR227_BUCKET_COUNT; i++)
	{
		u32 recordAddress = OVR227_RDATA_BUCKET_SETUP_BASE + (u32)(i * sizeof(R227.bucketSetups[0]));

		if (recordAddress == setupAddress)
		{
			return &R227.bucketSetups[i];
		}
	}

	return NULL;
}

static void Ovr227_800a0f04_ApplyBucketSetup(u32 setupAddress, u32 handlerAddress)
{
	const struct OverlayRDATA_227_BucketSetupRecord *setup = DrawLevelOvr2P_FindBucketSetupRecord(setupAddress);

	if (setup == NULL)
	{
		DrawLevelOvr1P_Scratch()->currentHandlerAddress = handlerAddress;
		return;
	}

	Ovr227_CopyScratchWordsTranslated(setup->copy0, &setup->copies[0]);
	Ovr227_CopyScratchWordsTranslated(setup->copy1, &setup->copies[1]);
	DrawLevelOvr1P_Scratch()->currentHandlerAddress = handlerAddress;
}

static void Ovr227_800a0d68_CopyScratchInitTable(void)
{
	u32 *scratch = CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_SCRATCH_INIT_TABLE_OFFSET);

	for (int i = 0; i < OVR227_SCRATCH_INIT_WORD_COUNT; i++)
	{
		scratch[i] = R227.scratchInitTable[i];
	}
}

static void Ovr227_SeedSharedHelperThresholdScratch(void)
{
	// NOTE(aalhendi): Retail 227 bakes these thresholds as immediates in
	// copied BSP handler bodies. Native reuses 226 C helpers that still read
	// the equivalent thresholds from scratch, so seed the shared-helper view.
	DrawLevelOvr1P_RenderScratch()->depthScale = 0x19c0;
	DrawLevelOvr1P_RenderScratch()->textureLodDepthThreshold0 = 0x1000;
	DrawLevelOvr1P_RenderScratch()->textureLodDepthThreshold1 = 0x800;
	DrawLevelOvr1P_RenderScratch()->topLevelNearDepthThreshold = 0x600;
	DrawLevelOvr1P_RenderScratch()->recursiveNearDepthThreshold = 0x300;
}

static const struct DrawLevelOvr1PBucket *Ovr227_800a0ddc_FindBucketByHandler(u32 handlerAddress)
{
	for (int i = 0; i < OVR227_BUCKET_COUNT; i++)
	{
		if (R227.bucketHandlerAddresses[i] == handlerAddress)
		{
			return &sDrawLevelOvr1PBuckets[i];
		}
	}

	return NULL;
}

static int Ovr227_800a0ddc_DispatchBucketHandler(u32 handlerAddress, void *bucketValue, struct PushBuffer *pb, struct mesh_info *mesh, struct PrimMem *primMem,
                                                 const int *visFaceList)
{
	const struct DrawLevelOvr1PBucket *bucket = Ovr227_800a0ddc_FindBucketByHandler(handlerAddress);

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

static void Ovr227_ClearRenderedOverflowBase(int playerIndex)
{
	struct QuadBlock **renderedOverflowBase = (struct QuadBlock **)data.ptrRenderedQuadblockDestination_forEachPlayer[playerIndex];

	if (renderedOverflowBase != NULL)
	{
		*renderedOverflowBase = NULL;
	}
}

static void Ovr227_800a0f60_SetViewportContext(struct PushBuffer *pb, const int *visFaceList, u8 *clipStart, u8 *clipCursor,
                                               struct QuadBlock **renderedOverflowBase)
{
	DrawLevelOvr1P_Scratch()->renderedOverflowPtr32 = (u32)(uintptr_t)renderedOverflowBase;
	DrawLevelOvr1P_Scratch()->clipCursorPtr32 = (u32)(uintptr_t)clipCursor;
	DrawLevelOvr1P_Scratch()->visFaceListPtr32 = (u32)(uintptr_t)visFaceList;
	DrawLevelOvr1P_SetClipRecordStart(clipStart);
	DrawLevelOvr1P_SetRenderedOverflowBase(renderedOverflowBase);
	Ovr226_800a0d34_SetEntryGteAndCameraScratch(pb);
}

static int Ovr227_DrawViewportBucket(struct DrawLevelOvr1PRenderList *renderList, s32 renderListOffset, struct PushBuffer *pb, struct mesh_info *mesh,
                                     struct PrimMem *primMem, const int *visFaceList, u8 **clipCursor, int playerIndex, int applySetup)
{
	u32 bucketIndex = (u32)renderListOffset / sizeof(u32);
	void *bucketValue = DrawLevelOvr1P_GetRenderListField(renderList, renderListOffset);
	u32 setupAddress = R227.bucketSetupAddresses[bucketIndex];
	u32 handlerAddress = R227.bucketHandlerAddresses[bucketIndex];
	struct QuadBlock **renderedOverflowBase = (struct QuadBlock **)data.ptrRenderedQuadblockDestination_forEachPlayer[playerIndex];

	if (bucketValue == NULL)
	{
		Ovr227_ClearRenderedOverflowBase(playerIndex);
		return 1;
	}

	if (applySetup)
	{
		Ovr227_800a0f04_ApplyBucketSetup(setupAddress, handlerAddress);
	}

	Ovr227_800a0f60_SetViewportContext(pb, visFaceList, data.PtrClipBuffer[playerIndex], *clipCursor, renderedOverflowBase);
	if (!Ovr227_800a0ddc_DispatchBucketHandler(handlerAddress, bucketValue, pb, mesh, primMem, visFaceList))
	{
		return 0;
	}

	*clipCursor = DrawLevelOvr1P_GetClipRecordCursor();
	return 1;
}

static int Ovr227_800a0d9c_DispatchBucketTable(struct DrawLevelOvr1PRenderList *renderLists, struct PushBuffer *pushBuffers, struct mesh_info *mesh,
                                               struct PrimMem *primMem, const int *visFaceList0, const int *visFaceList1, u8 **clipCursors)
{
	for (s32 renderListOffset = 0x28; renderListOffset >= 0; renderListOffset -= (s32)sizeof(u32))
	{
		void *viewport0BucketValue = DrawLevelOvr1P_GetRenderListField(&renderLists[0], renderListOffset);

		DrawLevelOvr1P_Scratch()->currentBucketOffset = (u32)renderListOffset;

		if (!Ovr227_DrawViewportBucket(&renderLists[0], renderListOffset, &pushBuffers[0], mesh, primMem, visFaceList0, &clipCursors[0], 0, 1))
		{
			return 0;
		}

		if (!Ovr227_DrawViewportBucket(&renderLists[1], renderListOffset, &pushBuffers[1], mesh, primMem, visFaceList1, &clipCursors[1], 1,
		                               viewport0BucketValue == NULL))
		{
			return 0;
		}
	}

	return 1;
}

static int Ovr227_ConsumeClipRecordsForViewport(struct PushBuffer *pb, struct PrimMem *primMem, u8 *clipCursor, int playerIndex)
{
	u8 *start = data.PtrClipBuffer[playerIndex];

	DrawLevelOvr1P_SetClipRecordStart(start);
	DrawLevelOvr1P_SetClipRecordCursor(clipCursor);
	Ovr227_800ab45c_CopyClipRecordJumpTable();
	return DrawLevelOvr1P_ConsumeClipRecords(pb, primMem);
}

static int Ovr227_800a0cbc_Entry(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *VisMem14,
                                 void *waterEnvMap)
{
	struct DrawLevelOvr1PRenderList *renderLists = LevRenderList;
	struct mesh_info *mesh = (struct mesh_info *)bspList;
	const int *visFaceList0 = VisMem10;
	const int *visFaceList1 = VisMem14;
	u8 *clipCursors[2] = {data.PtrClipBuffer[0], data.PtrClipBuffer[1]};
	u32 hostStackAnchor;

	// NOTE(aalhendi): ASM-audited against NTSC-U 926 227 entry/setup
	// 0x800a0cbc-0x800a1010. Native keeps explicit host pointers while
	// preserving the retail scratch ownership and two-viewport ordering.
	DrawLevelOvr1P_Scratch()->savedStackPtr32 = (u32)(uintptr_t)&hostStackAnchor;
	DrawLevelOvr1P_Scratch()->primMemEndPtr32 = (u32)(uintptr_t)primMem->end;
	DrawLevelOvr1P_Scratch()->visFaceListArgPtr32[0] = (u32)(uintptr_t)VisMem10;
	DrawLevelOvr1P_Scratch()->visFaceListArgPtr32[1] = (u32)(uintptr_t)VisMem14;

	if ((VisMem10 == NULL) || (VisMem14 == NULL))
	{
		return 1;
	}

	DrawLevelOvr1P_Scratch()->waterEnvMapPtr32 = waterEnvMap;
	DrawLevelOvr1P_Scratch()->pushBufferPtr32[0] = (u32)(uintptr_t)&pb[0];
	DrawLevelOvr1P_Scratch()->pushBufferPtr32[1] = (u32)(uintptr_t)&pb[1];
	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[0] = (u32)(uintptr_t)clipCursors[0];
	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[1] = (u32)(uintptr_t)clipCursors[1];

	if (mesh->ptrQuadBlockArray == NULL)
	{
		return 1;
	}

	DrawLevelOvr1P_SetPrimReserveBias(0xd00);
	DrawLevelOvr1P_SetListHandlersSeedRenderedCursor(0);
	Ovr226_800a0dc4_ClearProjectedScratch();
	Ovr227_800a0d68_CopyScratchInitTable();
	Ovr227_SeedSharedHelperThresholdScratch();

	if (!Ovr227_800a0d9c_DispatchBucketTable(renderLists, pb, mesh, primMem, visFaceList0, visFaceList1, clipCursors))
	{
		return 0;
	}

	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[0] = (u32)(uintptr_t)clipCursors[0];
	DrawLevelOvr1P_Scratch()->playerClipCursorPtr32[1] = (u32)(uintptr_t)clipCursors[1];

	if (!Ovr227_ConsumeClipRecordsForViewport(&pb[0], primMem, clipCursors[0], 0))
	{
		return 0;
	}

	if (!Ovr227_ConsumeClipRecordsForViewport(&pb[1], primMem, clipCursors[1], 1))
	{
		return 0;
	}

	return 1;
}

static void Ovr227_800ab45c_CopyClipRecordJumpTable(void)
{
	u32 *clipRecordJumpTable = CTR_SCRATCHPAD_PTR(u32, DRAW_LEVEL_OVR1P_GT3_CLIP_RECORD_JUMP_TABLE_OFFSET);

	for (int i = 0; i < OVR227_CLIP_RECORD_JUMP_WORD_COUNT; i++)
	{
		clipRecordJumpTable[i] = Ovr227_TranslateCopiedLabel(R227.clipRecordJumpTable[i]);
	}
}

void DrawLevelOvr2P(void *LevRenderList, struct PushBuffer *pb, struct BSP *bspList, struct PrimMem *primMem, void *VisMem10, void *VisMem14, void *waterEnvMap)
{
	(void)Ovr227_800a0cbc_Entry(LevRenderList, pb, bspList, primMem, VisMem10, VisMem14, waterEnvMap);
}
