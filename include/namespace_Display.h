struct PrimMem
{
	// 0x0
	u32 capacityBytes;

	// 0x4
	void *start;

	// 0x8
	void *end;

	// 0xC
	void *cursor;

	// 0x10
	// NOTE(aalhendi): Retail keeps 0x100 bytes reserved after this guard.
	void *guardEnd;

	// 0x14
	s32 primitiveCount;

	// 0x18
	// NOTE(aalhendi): Retail mirrors the allocation base here.
	void *allocationStart;
};

struct OTMem
{
	// 0x0
	u32 capacityBytes;

	// 0x4
	uint32_t *start;

	// 0x8
	uint32_t *end;

	// 0xC
	uint32_t *cursor;

	// 0x10
	// NOTE(aalhendi): UI ordering-table pointer, also stored in pushBuffer_UI.
	uint32_t *uiOT;
};

// 0xA4
struct DB
{
	// 0x00
	DRAWENV drawEnv;

	// 0x5C
	DISPENV dispEnv;

	// 0x70
	u8 blurCameraMask;
	u8 blurCameraMaskPadding[3];

	// 0x74
	struct PrimMem primMem;

	// 0x90
	struct OTMem otMem;
};

CTR_STATIC_ASSERT_LAYOUT(sizeof(struct PrimMem) == 0x1C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct PrimMem, capacityBytes) == 0x0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct PrimMem, start) == 0x4);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct PrimMem, end) == 0x8);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct PrimMem, cursor) == 0xC);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct PrimMem, guardEnd) == 0x10);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct PrimMem, primitiveCount) == 0x14);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct PrimMem, allocationStart) == 0x18);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct OTMem) == 0x14);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct OTMem, capacityBytes) == 0x0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct OTMem, start) == 0x4);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct OTMem, end) == 0x8);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct OTMem, cursor) == 0xC);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct OTMem, uiOT) == 0x10);
#ifndef CTR_NATIVE
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct DB, drawEnv) == 0x0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct DB, dispEnv) == 0x5C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct DB, blurCameraMask) == 0x70);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct DB, primMem) == 0x74);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct DB, otMem) == 0x90);
CTR_STATIC_ASSERT_LAYOUT(sizeof(struct DB) == 0xA4);
#endif
