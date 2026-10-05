#include <common.h>

#ifdef CTR_NATIVE
static const u32 RenderListsCornerJumpTable[8];
#else
extern const u32 RenderListsCornerJumpTable[8];
#endif

// NOTE(aalhendi): Retail uses checked ADDI for fixed scratch+0x84; GCC emits
// ADDIU for C arithmetic. Native uses its mapped scratch object instead.
#ifdef CTR_NATIVE
#define RL_SET_CORNER_DESTINATION(result, base) ((result) = (u32 *)((u8 *)(base) + 0x84))
#else
#define RL_SET_CORNER_DESTINATION(result, base) __asm__("addi %0,%1,132" : "=r"(result) : "r"(base))
#endif

CTR_PSX_MATCH_SECTION(".RenderLists_PreInit")
void RenderLists_PreInit(void)
{
	// NOTE(aalhendi): These initialized copy operands are not hidden inputs.
	// Keep their retail register lifetimes, including the reserved AT page.
	register u32 *scratchBase CTR_PSX_REGISTER("$1");
	register s32 remaining CTR_PSX_REGISTER("$10");
	register const u32 *source CTR_PSX_REGISTER("$8");
	register u32 *destination CTR_PSX_REGISTER("$9");

	scratchBase = CTR_SCRATCHPAD_PTR(u32, 0);
	CTR_PSX_KEEP_VALUE(scratchBase);
	remaining = 7;
	CTR_PSX_KEEP_VALUE(remaining);
	source = RenderListsCornerJumpTable;
	CTR_PSX_KEEP_VALUE(source);
	RL_SET_CORNER_DESTINATION(destination, scratchBase);
	CTR_PSX_KEEP_VALUE(destination);

	goto copyBody;
copyAgain:
	remaining--;
copyBody:
{
	register u32 value CTR_PSX_REGISTER("$3") = *source++;
	CTR_PSX_KEEP_VALUE(value);
	*destination++ = value;
}
	// NOTE(aalhendi): Keep the pointer advance before the loop decision so
	// GCC fills the branch slot from the backedge countdown instead.
	CTR_PSX_OBSERVE_VALUE(destination);
	if (remaining != 0)
		goto copyAgain;
}

#undef RL_SET_CORNER_DESTINATION

#include "RenderLists_Retail.inc"

#ifdef CTR_NATIVE

enum RenderListsSlot1P2P
{
	RENDER_LIST_SLOT_4X4 = 0,
	RENDER_LIST_SLOT_DYNAMIC_SUBDIV = 1,
	RENDER_LIST_SLOT_4X2 = 2,
	RENDER_LIST_SLOT_4X1 = 3,
	RENDER_LIST_SLOT_WATER = 4,
	RENDER_LIST_SLOT_FULL_DYNAMIC = 5,
};

enum RenderListsScratchOffset
{
	RENDER_LISTS_PLANES_OFFSET = 0x60,
	RENDER_LISTS_COUNT_OFFSET = 0x80,
	RENDER_LISTS_CORNER_JUMP_TABLE_OFFSET = 0x84,
	RENDER_LISTS_LOD_OFFSET = 0xa4,
	RENDER_LISTS_SELECTED_CORNERS_OFFSET = 0xac,
	RENDER_LISTS_STACK_OFFSET = 0xd0,
};

struct RenderListsScratchRecord
{
	BspChildId childID;
	s16 unused;
	struct BoundingBox box;
};

CTR_STATIC_ASSERT(sizeof(struct RenderListsScratchRecord) == 0x10);

static void RenderLists_InitScratch(struct PushBuffer *pb, u8 numPlyr)
{
	u32 *selectedCorners = CTR_SCRATCHPAD_PTR(u32, RENDER_LISTS_SELECTED_CORNERS_OFFSET);
	const u32 *cornerTable = CTR_SCRATCHPAD_PTR(u32, RENDER_LISTS_CORNER_JUMP_TABLE_OFFSET);
	s32 *count = CTR_SCRATCHPAD_PTR(s32, RENDER_LISTS_COUNT_OFFSET);

	*count = 0;
	memcpy(CTR_SCRATCHPAD_PTR(u8, RENDER_LISTS_PLANES_OFFSET), pb->frustumPlanes, 4 * sizeof(pb->frustumPlanes[0]));
	for (s32 i = 0; i < 4; i++)
	{
		selectedCorners[i] = cornerTable[pb->RenderListJmpIndex[i]];
	}
	if (numPlyr <= 2)
	{
		*CTR_SCRATCHPAD_PTR(s32, RENDER_LISTS_LOD_OFFSET) =
		    (numPlyr == 1) ? CTR_SCRATCHPAD_PTR(struct MainRenderLevelGeometryScratch, 0)->bspLodDistanceThreshold : 0x1540;
		selectedCorners[5] = cornerTable[pb->RenderListJmpIndex[5]];
	}
}

static int RenderLists_IsVisible(const int *visLeafList, BspChildId childID)
{
	u32 rawChildID = (u16)childID;

	if (visLeafList == 0)
	{
		return 1;
	}

	// NOTE(aalhendi): Retail sllv wraps the bitmap word before testing its sign.
	return (((u32)visLeafList[(rawChildID >> 5) & 0x1ff] << (rawChildID & 0x1f)) & 0x80000000u) != 0;
}

static int RenderLists_CopyOverlappingBox(const struct BoundingBox *box, const struct BoundingBox *pbBox, struct BoundingBox *recordBox)
{
	if (box->min.y > pbBox->max.y)
	{
		return 0;
	}
	// NOTE(aalhendi): Retail's branch-delay sw lanes survive rejected children.
	recordBox->min.x = box->min.x;
	recordBox->min.y = box->min.y;
	if (box->min.x > pbBox->max.x || pbBox->min.x > box->max.x || box->min.z > pbBox->max.z || pbBox->min.z > box->max.z)
	{
		return 0;
	}
	recordBox->min.z = box->min.z;
	recordBox->max.x = box->max.x;
	if (pbBox->min.y > box->max.y)
	{
		return 0;
	}
	recordBox->max.y = box->max.y;
	recordBox->max.z = box->max.z;
	return 1;
}

static void RenderLists_SelectBoxCorner(const struct BoundingBox *box, int cornerIndex, s16 *point)
{
	// NOTE(aalhendi): This is the exact corner order produced by retail helpers 0x80070310..0x8007037c: max/max/max through min/min/min by bit index.
	point[0] = (cornerIndex & 1) ? box->min.x : box->max.x;
	point[1] = (cornerIndex & 2) ? box->min.y : box->max.y;
	point[2] = (cornerIndex & 4) ? box->min.z : box->max.z;
}

static void RenderLists_LoadGteState(struct PushBuffer *pb)
{
	if (pb == 0)
	{
		return;
	}

	// NOTE(aalhendi): Retail 0x8006fecc loads RT/TR/OFX/OFY/H once before traversal; frustum helpers only touch the light-matrix/RBK controls.
	gte_SetRotMatrix(&pb->matrix_ViewProj);
	gte_SetTransMatrix(&pb->matrix_ViewProj);
	// NOTE(aalhendi): Retail keeps the half-pixel bit for odd viewport sizes.
	CTC2((u32)(s32)pb->rect.w << 15, 24);
	CTC2((u32)(s32)pb->rect.h << 15, 25);
	gte_SetGeomScreen(pb->distanceToScreen_PREV);
}

static int RenderLists_FrustumRejectsCorner(const struct PushBufferFrustumPlane *plane, const s16 *point)
{
	// NOTE(aalhendi): Retail 0x80070290 tests the selected BSP corner with llv0bk and rejects when IR1 is positive.
	MTC2(CTR_PackS16Pair(point[0], point[1]), 0);
	MTC2((u32)(s32)point[2], 1);
	CTC2(CTR_PackS16Pair(plane->normal.x, plane->normal.y), 8);
	CTC2(CTR_PackS16Pair(plane->normal.z, plane->halfDistance), 9);
	CTC2((u32)(s32)(-2 * plane->halfDistance), 13);
	doCOP2(0x04a2012);

	return MFC2_S(9) > 0;
}

static int RenderLists_BoxPassesFrustum(struct PushBuffer *pb, const struct BoundingBox *box)
{
	static const u8 planeOrder[] = {1, 3, 0, 2};
	s16 point[3];
	const struct PushBufferFrustumPlane *plane;

	if (pb == 0)
	{
		return 1;
	}

	for (s32 i = 0; i < 4; i++)
	{
		s32 planeIndex = planeOrder[i];
		// NOTE(aalhendi): Early rejection and final GTE lanes depend on this order.
		plane = &pb->frustumPlanes[planeIndex];
		RenderLists_SelectBoxCorner(box, pb->RenderListJmpIndex[planeIndex] & 7, point);

		if (RenderLists_FrustumRejectsCorner(plane, point))
		{
			return 0;
		}
	}

	return 1;
}

static int RenderLists_ProjectDistance(struct PushBuffer *pb, const struct BoundingBox *box)
{
	s16 point[3];
	int distance;

	if (pb == 0)
	{
		return 0;
	}

	RenderLists_SelectBoxCorner(box, pb->RenderListJmpIndex[5] & 7, point);

	CTR_GteLoadS16TripletV0(point);
	gte_rtps();
	gte_stsz(&distance);

	return distance;
}

static int RenderLists_Select1P2PSlot(const struct BSP *bsp, struct PushBuffer *pb, int lodDistanceThreshold)
{
	if ((bsp->flag & BSP_LEAF_FLAG_WATER) != 0)
	{
		return RENDER_LIST_SLOT_WATER;
	}

	if ((bsp->flag & BSP_RENDER_LEAF_FLAG_DYNAMIC_SUBDIV) != 0)
	{
		return RENDER_LIST_SLOT_DYNAMIC_SUBDIV;
	}

	if (RenderLists_ProjectDistance(pb, &bsp->box) > lodDistanceThreshold)
	{
		return RENDER_LIST_SLOT_FULL_DYNAMIC;
	}

	if ((bsp->flag & BSP_RENDER_LEAF_FLAG_4X4) != 0)
	{
		return RENDER_LIST_SLOT_4X4;
	}

	if ((bsp->flag & BSP_RENDER_LEAF_FLAG_4X1) != 0)
	{
		return RENDER_LIST_SLOT_4X1;
	}

	if ((bsp->flag & BSP_RENDER_LEAF_FLAG_4X2) != 0)
	{
		return RENDER_LIST_SLOT_4X2;
	}

	return RENDER_LIST_SLOT_DYNAMIC_SUBDIV;
}

static struct VisMemBspListNode **RenderLists_Get1P2PHead(void *LevRenderList, int slotIndex)
{
	if (slotIndex == RENDER_LIST_SLOT_FULL_DYNAMIC)
	{
		return (struct VisMemBspListNode **)((char *)LevRenderList + 0x28);
	}

	return (struct VisMemBspListNode **)((char *)LevRenderList + slotIndex * 8 + 4);
}

static int RenderLists_Select3P4PSlot(const struct BSP *bsp)
{
	if ((bsp->flag & BSP_LEAF_FLAG_WATER) != 0)
	{
		return 2;
	}

	if ((bsp->flag & BSP_RENDER_LEAF_FLAG_DYNAMIC_SUBDIV) != 0)
	{
		return 3;
	}

	if ((bsp->flag & BSP_RENDER_LEAF_FLAG_4X4) != 0)
	{
		return 0;
	}

	return 1;
}

static void RenderLists_LinkBsp(struct BSP *bspRoot, struct BSP *bsp, struct VisMemBspListNode **head, struct VisMemBspListNode *bspList)
{
	struct VisMemBspListNode *node;
	int bspIndex = bsp - bspRoot;

	node = &bspList[bspIndex];

	node->next = *head;
	*head = node;
}

static void RenderLists_PushChild(struct BSP *bspRoot, const int *visLeafList, struct PushBuffer *pb, BspChildId childID,
                                  struct RenderListsScratchRecord **stack, struct RenderListsScratchRecord *stackEnd)
{
	struct BSP *child;
	struct RenderListsScratchRecord *record;

	if (childID < 0)
	{
		return;
	}

	if (!RenderLists_IsVisible(visLeafList, childID))
	{
		return;
	}

	if (*stack >= stackEnd)
	{
		return;
	}

	record = *stack;
	child = &bspRoot[((u16)childID) & BSP_CHILD_ID_INDEX_MASK];
	if (!RenderLists_CopyOverlappingBox(&child->box, &pb->bbox, &record->box))
	{
		return;
	}

	record->childID = childID;
	// NOTE(aalhendi): Retail sh writes only childID, preserving the next halfword.
	*stack = record + 1;
}

static struct VisMemBspListNode **RenderLists_SelectHead(struct BSP *bsp, struct PushBuffer *pb, void *LevRenderList, u8 numPlyr, s32 lodDistanceThreshold)
{
	if (numPlyr <= 2)
	{
		return RenderLists_Get1P2PHead(LevRenderList, RenderLists_Select1P2PSlot(bsp, pb, lodDistanceThreshold));
	}
	return (struct VisMemBspListNode **)((u8 *)LevRenderList + RenderLists_Select3P4PSlot(bsp) * 8 + 4);
}

static int RenderLists_Walk(struct BSP *bspRoot, const int *visLeafList, struct PushBuffer *pb, void *LevRenderList, struct VisMemBspListNode *bspList,
                            u8 numPlyr)
{
	struct RenderListsScratchRecord *stackBase = CTR_SCRATCHPAD_PTR(struct RenderListsScratchRecord, RENDER_LISTS_STACK_OFFSET);
	struct RenderListsScratchRecord *stackEnd = CTR_SCRATCHPAD_PTR(struct RenderListsScratchRecord, CTR_SCRATCHPAD_SIZE);
	struct RenderListsScratchRecord *stack = stackBase;
	struct BSP *branch = bspRoot;
	int lodDistanceThreshold = (numPlyr == 1) ? CTR_SCRATCHPAD_PTR(struct MainRenderLevelGeometryScratch, 0)->bspLodDistanceThreshold : 0x1540;
	int count = 0;

	if (bspRoot == 0 || pb == 0)
	{
		return 0;
	}

	if ((bspRoot->flag & BSP_NODE_FLAG_LEAF) != 0)
	{
		RenderLists_LinkBsp(bspRoot, bspRoot, RenderLists_SelectHead(bspRoot, pb, LevRenderList, numPlyr, lodDistanceThreshold), bspList);
		return 1;
	}

	for (;;)
	{
		RenderLists_PushChild(bspRoot, visLeafList, pb, branch->data.branch.childID[0], &stack, stackEnd);
		RenderLists_PushChild(bspRoot, visLeafList, pb, branch->data.branch.childID[1], &stack, stackEnd);

		while (stack != stackBase)
		{
			struct RenderListsScratchRecord record = *--stack;
			struct BSP *bsp = &bspRoot[((u16)record.childID) & BSP_CHILD_ID_INDEX_MASK];

			if (((u16)record.childID & BSP_CHILD_ID_LEAF_FLAG) == 0)
			{
				branch = bsp;
				goto nextBranch;
			}

			if (!RenderLists_BoxPassesFrustum(pb, &record.box))
			{
				continue;
			}

			RenderLists_LinkBsp(bspRoot, bsp, RenderLists_SelectHead(bsp, pb, LevRenderList, numPlyr, lodDistanceThreshold), bspList);
			count++;
			*CTR_SCRATCHPAD_PTR(s32, RENDER_LISTS_COUNT_OFFSET) = count;
		}

		return count;

	nextBranch:
		continue;
	}
}

int RenderLists_Init1P2P(struct BSP *bspRoot, int *visLeafList, struct PushBuffer *pb, u32 LevRenderList, void *bspList, u8 numPlyr)
{
	RenderLists_LoadGteState(pb);
	if (pb != 0)
	{
		RenderLists_InitScratch(pb, numPlyr);
	}
	return RenderLists_Walk(bspRoot, visLeafList, pb, (void *)LevRenderList, bspList, numPlyr);
}

int RenderLists_Init3P4P(struct BSP *bspRoot, int *visLeafList, struct PushBuffer *pb, u32 LevRenderList, void *bspList)
{
	RenderLists_LoadGteState(pb);
	if (pb != 0)
	{
		RenderLists_InitScratch(pb, 3);
	}
	return RenderLists_Walk(bspRoot, visLeafList, pb, (void *)LevRenderList, bspList, 3);
}

#endif
