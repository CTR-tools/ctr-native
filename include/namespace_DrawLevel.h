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

enum DrawLevelOvr1PUvScratchSlot
{
	DRAW_LEVEL_OVR1P_UV_SCRATCH_SLOT_0,
	DRAW_LEVEL_OVR1P_UV_SCRATCH_SLOT_1,
	DRAW_LEVEL_OVR1P_UV_SCRATCH_SLOT_2,
};

enum DrawLevelOvr1PScratchOffset
{
	DRAW_LEVEL_OVR1P_SCRATCH_INIT_TABLE_OFFSET = 0x0ec,
	DRAW_LEVEL_OVR1P_DIRECT_NEAR_HANDLER_TABLE_OFFSET = 0x148,
	DRAW_LEVEL_OVR1P_NEAR_SUBDIVISION_HANDLER_TABLE_OFFSET = 0x14c,
	DRAW_LEVEL_OVR1P_DIRECT_HANDLER_TABLE_OFFSET = 0x184,
	DRAW_LEVEL_OVR1P_PROJECTED_FRAME0_OFFSET = 0x1b4,
	DRAW_LEVEL_OVR1P_TERMINAL_CLIP_VERTEX_OFFSET = 0x204,
	DRAW_LEVEL_OVR1P_GT3_CLIP_RECORD_JUMP_TABLE_OFFSET = 0x240,
	DRAW_LEVEL_OVR1P_GT4_CLIP_RECORD_JUMP_TABLE_OFFSET = 0x260,
	DRAW_LEVEL_OVR1P_WATER_RENDERED_SENTINEL_OFFSET = 0x268,
	DRAW_LEVEL_OVR1P_TERMINAL_RETURN_PC_OFFSET = 0x2a0,
	DRAW_LEVEL_OVR1P_MOSAIC_SOURCE_INDEX_OFFSET = 0x320,
	DRAW_LEVEL_OVR1P_DEEPEST_PROJECTED_FRAME_OFFSET = 0x324,
	DRAW_LEVEL_OVR1P_MOSAIC_SOURCE_BIAS_OFFSET = 0x3d8,
};

struct DrawLevelOvr1PScratchVertex
{
	union
	{
		SVec3 posVec;
		s16 pos[3];
	};
	u16 flags;
	u8 color_hi[4];
	union
	{
		SVec2 posScreenVec;
		s16 posScreen[2];
	};
	u16 depth;
	u8 clipNear;
	u8 clipHalfNear;
};

struct DrawLevelOvr1PClipRecordVertex
{
	union
	{
		SVec3 posVec;
		s16 pos[3];
	};
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

struct DrawLevelOvr1PUvScratch
{
	union
	{
		u32 uv0;
		struct
		{
			union
			{
				u16 uv0Packed;
				s16 flag0;
			};
			s16 clut;
		};
	};
	union
	{
		u32 uv1;
		struct
		{
			union
			{
				u16 uv1Packed;
				s16 flag1;
			};
			s16 tpage;
		};
	};
	union
	{
		u32 uv2;
		struct
		{
			s16 flag2;
			s16 flag3;
		};
	};
	u32 savedUv0;
	u32 savedUv1;
};

struct DrawLevelOvr1PStableScratch
{
	union
	{
		struct MainRenderLevelGeometryScratch render;
		struct
		{
			u32 playerClipCursorPtr32[4];
			u32 clipCursorPtr32;
		};
	};
	u32 primMemEndPtr32;
	u32 currentBucketOffset;
	u32 savedStackPtr32;
	u8 pad_03c[0x20];
	s32 depthClipThreshold;
	u32 renderListPtr32;
	u32 renderedOverflowPtr32;
	s32 quadCount;
	u32 clipWindowPacked;
	u32 directMask;
	u32 currentHandlerAddress;
	u8 pad_078[0x04];
	u32 drawOrderOrHeader;
	u32 clipRecordHeader;
	u32 mosaicTextureWord;
	struct TextureLayout* waterEnvMapPtr32;
	u8 pad_08c[0x10];
	u32 previousDirectHandlerAddress;
	u8 pad_0a0[0x1c];
	s32 visibilityBitIndex;
	u32 visibilityWordPtr32;
	u32 visibilityWord;
	u32 visFaceListPtr32;
	u32 visFaceListArgPtr32[4];
	u32 pushBufferPtr32[4];
	u8 pad_0ec[0xa8];
	u32 selected4x1TableWord;
	SVec3 projectedCenter;
	u8 pad_19e[0x02];
	struct DrawLevelOvr1PUvScratch uv;
};

CTR_STATIC_ASSERT(sizeof(struct DrawLevelOvr1PScratchVertex) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PScratchVertex, posVec) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PScratchVertex, pos) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PScratchVertex, flags) == 0x06);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PScratchVertex, color_hi) == 0x08);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PScratchVertex, posScreenVec) == 0x0c);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PScratchVertex, posScreen) == 0x0c);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PScratchVertex, depth) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PScratchVertex, clipNear) == 0x12);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PScratchVertex, clipHalfNear) == 0x13);
CTR_STATIC_ASSERT(sizeof(struct DrawLevelOvr1PClipRecordVertex) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PClipRecordVertex, posVec) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PClipRecordVertex, pos) == 0x00);
CTR_STATIC_ASSERT(sizeof(struct DrawLevelOvr1PClipRecord) == 0x3c);
CTR_STATIC_ASSERT(sizeof(struct DrawLevelOvr1PUvScratch) == 0x14);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PUvScratch, uv0) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PUvScratch, flag0) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PUvScratch, clut) == 0x02);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PUvScratch, uv1) == 0x04);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PUvScratch, flag1) == 0x04);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PUvScratch, tpage) == 0x06);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PUvScratch, uv2) == 0x08);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PUvScratch, flag2) == 0x08);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PUvScratch, flag3) == 0x0a);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PUvScratch, savedUv0) == 0x0c);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PUvScratch, savedUv1) == 0x10);
CTR_STATIC_ASSERT(sizeof(struct DrawLevelOvr1PStableScratch) == 0x1b4);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, render) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, playerClipCursorPtr32) == 0x00);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, clipCursorPtr32) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, primMemEndPtr32) == 0x30);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, currentBucketOffset) == 0x34);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, savedStackPtr32) == 0x38);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, depthClipThreshold) == 0x5c);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, renderListPtr32) == 0x60);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, renderedOverflowPtr32) == 0x64);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, quadCount) == 0x68);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, clipWindowPacked) == 0x6c);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, directMask) == 0x70);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, currentHandlerAddress) == 0x74);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, drawOrderOrHeader) == 0x7c);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, clipRecordHeader) == 0x80);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, mosaicTextureWord) == 0x84);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, waterEnvMapPtr32) == 0x88);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, previousDirectHandlerAddress) == 0x9c);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, visibilityBitIndex) == 0xbc);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, visibilityWordPtr32) == 0xc0);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, visibilityWord) == 0xc4);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, visFaceListPtr32) == 0xc8);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, visFaceListArgPtr32) == 0xcc);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, pushBufferPtr32) == 0xdc);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, selected4x1TableWord) == 0x194);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, projectedCenter) == 0x198);
CTR_STATIC_ASSERT(offsetof(struct DrawLevelOvr1PStableScratch, uv) == 0x1a0);
