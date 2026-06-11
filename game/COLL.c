#include <common.h>
#include <ctr_scratchpad.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d094-0x8001d0c4
struct MetaDataMODEL *COLL_LevModelMeta(u32 id)
{
	// use unsigned so -1 is positive
	if (id >= 0xe2)
		id = 0;

	return &data.MetaDataModels[id];
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001eb0c-0x8001ebec
void COLL_SearchBSP_CallbackQUADBLK(u32 *posTop, u32 *posBottom, struct ScratchpadStruct *sps, int hitRadius)
{
	u32 topXY;
	u32 bottomXY;
	s16 topZ;
	s16 bottomZ;
	s16 topX;
	s16 topY;
	s16 bottomX;
	s16 bottomY;
	s16 min;
	s16 max;
	int hitRadiusSquared;
	struct mesh_info *meshInfo;

	hitRadiusSquared = hitRadius * hitRadius;

	sps->Input1.hitRadius = hitRadius;
	sps->Union.QuadBlockColl.hitRadius = hitRadius;
	sps->boolDidTouchHitbox = 0;
	sps->unk3C = 0;
	sps->boolDidTouchQuadblock = 0;
	*(int *)&sps->dataOutput[0] = 0;
	meshInfo = sps->ptr_mesh_info;
	sps->numInstHitboxesHit = 0;

	topXY = posTop[0];
	topZ = *(s16 *)(posTop + 1);

	sps->Input1.pos[0] = (s16)topXY;
	sps->Input1.pos[1] = (s16)(topXY >> 16);
	sps->Input1.pos[2] = topZ;
	sps->Union.QuadBlockColl.hitPos[0] = (s16)topXY;
	sps->Union.QuadBlockColl.hitPos[1] = (s16)(topXY >> 16);
	sps->Union.QuadBlockColl.hitPos[2] = topZ;

	bottomXY = posBottom[0];
	bottomZ = *(s16 *)(posBottom + 1);

	sps->Union.QuadBlockColl.pos[0] = (s16)bottomXY;
	sps->Union.QuadBlockColl.pos[1] = (s16)(bottomXY >> 16);
	sps->Union.QuadBlockColl.pos[2] = bottomZ;

	sps->countByOne_ForWhatReason = 0x1000;
	sps->Input1.hitRadiusSquared = hitRadiusSquared;
	sps->Union.QuadBlockColl.hitRadiusSquared = hitRadiusSquared;

	topX = sps->Input1.pos[0];
	topY = sps->Input1.pos[1];
	topZ = sps->Input1.pos[2];
	bottomX = sps->Union.QuadBlockColl.pos[0];
	bottomY = sps->Union.QuadBlockColl.pos[1];
	bottomZ = sps->Union.QuadBlockColl.pos[2];

	min = bottomX;
	max = topX;
	if ((topX - bottomX) < 0)
	{
		min = topX;
		max = bottomX;
	}
	sps->bbox.min[0] = min;
	sps->bbox.max[0] = max;

	min = bottomY;
	max = topY;
	if ((topY - bottomY) < 0)
	{
		min = topY;
		max = bottomY;
	}
	sps->bbox.min[1] = min;
	sps->bbox.max[1] = max;

	min = bottomZ;
	max = topZ;
	if ((topZ - bottomZ) < 0)
	{
		min = topZ;
		max = bottomZ;
	}
	sps->bbox.min[2] = min;
	sps->bbox.max[2] = max;

	COLL_SearchBSP_CallbackPARAM(meshInfo->bspRoot, &sps->bbox, COLL_FIXED_BSPLEAF_TestQuadblocks, sps);
}


static int COLL_SearchBSP_CallbackPARAM_Overlaps(struct BSP *node, s16 minX, s16 minY, s16 minZ, s16 maxX, s16 maxY, s16 maxZ)
{
	return ((node->box.min[1] <= maxY) && (node->box.min[0] <= maxX) && (minX <= node->box.max[0]) && (node->box.min[2] <= maxZ) &&
	        (minZ <= node->box.max[2]) && (minY <= node->box.max[1]));
}

static void COLL_SearchBSP_CallbackPARAM_PushChild(struct BSP *root, u16 childId, s16 minX, s16 minY, s16 minZ, s16 maxX, s16 maxY, s16 maxZ, u16 **stackTop)
{
	struct BSP *child;

	if (childId == 0xffff)
		return;

	child = &root[childId & 0x3fff];
	if (!COLL_SearchBSP_CallbackPARAM_Overlaps(child, minX, minY, minZ, maxX, maxY, maxZ))
		return;

	**stackTop = childId;
	(*stackTop)++;
}

static void COLL_SearchBSP_CallbackPARAM_PushChildren(struct BSP *root, struct BSP *node, s16 minX, s16 minY, s16 minZ, s16 maxX, s16 maxY, s16 maxZ,
                                                      u16 **stackTop)
{
	// Retail pushes child 0 then child 1; the scratchpad stack pops child 1 first.
	COLL_SearchBSP_CallbackPARAM_PushChild(root, (u16)node->data.branch.childID[0], minX, minY, minZ, maxX, maxY, maxZ, stackTop);
	COLL_SearchBSP_CallbackPARAM_PushChild(root, (u16)node->data.branch.childID[1], minX, minY, minZ, maxX, maxY, maxZ, stackTop);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001ebec-0x8001ede4
void COLL_SearchBSP_CallbackPARAM(struct BSP *root, struct BoundingBox *bbox, void (*callback)(struct BSP *, struct ScratchpadStruct *),
                                  struct ScratchpadStruct *param)
{
	u16 *stackBase;
	u16 *stackTop;

	if (root == NULL)
		return;

	s16 minX = bbox->min[0];
	s16 minY = bbox->min[1];
	s16 minZ = bbox->min[2];
	s16 maxX = bbox->max[0];
	s16 maxY = bbox->max[1];
	s16 maxZ = bbox->max[2];

	// Retail stores pending child IDs at scratchpad 0x1f800070 and pops them
	// LIFO, preserving the original BSP traversal order without host recursion.
	stackBase = CTR_SCRATCHPAD_PTR(u16, 0x70);
	stackTop = stackBase;

	COLL_SearchBSP_CallbackPARAM_PushChildren(root, root, minX, minY, minZ, maxX, maxY, maxZ, &stackTop);

	while (stackTop != stackBase)
	{
		u16 childId;
		struct BSP *child;

		stackTop--;
		childId = *stackTop;
		child = &root[childId & 0x3fff];

		if ((childId & 0x4000) != 0)
		{
			callback(child, param);
			continue;
		}

		COLL_SearchBSP_CallbackPARAM_PushChildren(root, child, minX, minY, minZ, maxX, maxY, maxZ, &stackTop);
	}
}


static u32 CollFixed_PackS16Pair(s32 lo, s32 hi)
{
	return (u16)lo | ((u32)(u16)hi << 16);
}

static s32 CollFixed_Sll32(s32 value, u32 shift)
{
	return (s32)((u32)value << (shift & 0x1f));
}

static s32 CollFixed_MulLo(s32 a, s32 b)
{
	return (s32)(u32)((s64)a * (s64)b);
}

static s16 CollFixed_ReadS16(const void *base, int offset)
{
	return *(const s16 *)((const u8 *)base + offset);
}

static u16 CollFixed_ReadU16(const void *base, int offset)
{
	return *(const u16 *)((const u8 *)base + offset);
}

static s32 CollFixed_ReadS32(const void *base, int offset)
{
	return *(const s32 *)((const u8 *)base + offset);
}

static u32 CollFixed_ReadU32(const void *base, int offset)
{
	return *(const u32 *)((const u8 *)base + offset);
}

static void CollFixed_WriteS16(void *base, int offset, s32 value)
{
	*(s16 *)((u8 *)base + offset) = (s16)value;
}

static void CollFixed_WriteS32(void *base, int offset, s32 value)
{
	*(s32 *)((u8 *)base + offset) = value;
}

static void CollFixed_WriteU32(void *base, int offset, u32 value)
{
	*(u32 *)((u8 *)base + offset) = value;
}

static void CollFixed_GteLoadR11R12(u32 value)
{
	CTC2(value, 0);
}

static void CollFixed_GteLoadR13R21(u32 value)
{
	CTC2(value, 1);
}

static void CollFixed_GteLoadR22R23(u32 value)
{
	CTC2(value, 2);
}

static void CollFixed_GteLoadR33(u32 value)
{
	CTC2(value, 4);
}

static void CollFixed_GteLoadVXY0(u32 value)
{
	MTC2(value, 0);
}

static void CollFixed_GteLoadVZ0(s32 value)
{
	MTC2_S(value, 1);
}

static void CollFixed_GteLoadIR0(s32 value)
{
	MTC2_S(value, 8);
}

static void CollFixed_GteLoadIR(s32 x, s32 y, s32 z)
{
	MTC2_S(x, 9);
	MTC2_S(y, 10);
	MTC2_S(z, 11);
}

static void CollFixed_GteLoadMAC(s32 x, s32 y, s32 z)
{
	MTC2_S(x, 25);
	MTC2_S(y, 26);
	MTC2_S(z, 27);
}

static s32 CollFixed_GteReadMAC1(void)
{
	return MFC2_S(25);
}

static s32 CollFixed_GteReadMAC2(void)
{
	return MFC2_S(26);
}

static s32 CollFixed_GteReadMAC3(void)
{
	return MFC2_S(27);
}

static void CollFixed_GteLoadLZCS(s32 value)
{
	MTC2_S(value, 30);
}

static s32 CollFixed_GteReadLZCR(void)
{
	return MFC2_S(31);
}

static void CollFixed_GteMVMVA(void)
{
	doCOP2(0x0406012);
}

static void CollFixed_GteGPL12(void)
{
	doCOP2(0x01a8003e);
}

static void CollFixed_GteOP0(void)
{
	doCOP2(0x0170000c);
}

static void CollFixed_GteRTIR(void)
{
	doCOP2(0x049e012);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001ede4-0x8001ef1c
void COLL_FIXED_TRIANGL_Barycentrics(s16 *out, s16 *v1, s16 *v2, s16 *point)
{
	s32 v1x = v1[0];
	s32 v1y = v1[1];
	s32 v1z = v1[2];
	s32 edgeX = v2[0] - v1x;
	s32 edgeY = v2[1] - v1y;
	s32 edgeZ = v2[2] - v1z;
	s32 pointX = point[0] - v1x;
	s32 pointY = point[1] - v1y;
	s32 pointZ = point[2] - v1z;
	s32 pointDot;
	s32 edgeDot;
	s32 shift;
	s32 factor;

	CollFixed_GteLoadR11R12(CollFixed_PackS16Pair(edgeX, edgeY));
	CollFixed_GteLoadR13R21(CollFixed_PackS16Pair(edgeZ, pointX));
	CollFixed_GteLoadR22R23(CollFixed_PackS16Pair(pointY, pointZ));
	CollFixed_GteLoadVXY0(CollFixed_PackS16Pair(edgeX, edgeY));
	CollFixed_GteLoadVZ0(edgeZ);

	CollFixed_GteMVMVA();
	pointDot = CollFixed_GteReadMAC2();
	edgeDot = CollFixed_GteReadMAC1();

	CollFixed_GteLoadLZCS(pointDot);
	shift = CollFixed_GteReadLZCR() - 2;

	if (shift < 0)
	{
		shift = 0;
	}
	else if (shift > 12)
	{
		shift = 12;
	}

	pointDot = CollFixed_Sll32(pointDot, shift);

	if (shift < 12)
	{
		edgeDot >>= 12 - shift;
	}

	factor = 0;

	if (edgeDot != 0)
	{
		factor = pointDot / edgeDot;

		if (factor < 0)
		{
			factor = 0;
		}
		else if (factor > 0x1000)
		{
			factor = 0x1000;
		}
	}

	CollFixed_GteLoadIR0(factor);
	CollFixed_GteLoadMAC(v1x, v1y, v1z);
	CollFixed_GteLoadIR(edgeX, edgeY, edgeZ);
	CollFixed_GteGPL12();

	out[0] = (s16)CollFixed_GteReadMAC1();
	out[1] = (s16)CollFixed_GteReadMAC2();
	out[2] = (s16)CollFixed_GteReadMAC3();
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d0c4-0x8001d610
u32 COLL_FIXED_INSTANC_TestPoint(struct ScratchpadStruct *sps, struct BSP *node)
{
	u8 *spsBytes = (u8 *)sps;
	u8 *nodeBytes = (u8 *)node;
	s32 diffX;
	s32 diffY;
	s32 diffZ;
	s32 centerDiffX;
	s32 centerDiffY;
	s32 centerDiffZ;
	s32 dotSegment;
	s32 dotCenter;
	s32 shift;
	s32 divisor;
	s32 factor;
	s32 projX;
	s32 projY;
	s32 projZ;
	s32 relX;
	s32 relY;
	s32 relZ;
	s32 radius;
	s32 radiusSquared;
	s32 distSquared;
	s32 remaining;
	s32 hitX;
	s32 hitY;
	s32 hitZ;
	s32 normalX;
	s32 normalY;
	s32 normalZ;
	s32 len;
	s32 invLen;
	s32 scaledX;
	s32 scaledY;
	s32 scaledZ;

	if (nodeBytes[1] == 4)
	{
		*(struct BSP **)(spsBytes + 0x48) = node;
		CollFixed_WriteS16(spsBytes, 0x42, CollFixed_ReadU16(spsBytes, 0x42) + 1);
	}

	if ((CollFixed_ReadU16(spsBytes, 0x22) & 0x40) != 0)
	{
		CollFixed_WriteS16(spsBytes, 0x72, 0x1000);
		CollFixed_WriteS16(spsBytes, 0x70, 0);
		CollFixed_WriteS16(spsBytes, 0x74, 0);
		spsBytes[0x7e] = 6;
		CollFixed_WriteS32(spsBytes, 0x84, 0);
		*(struct BSP **)(spsBytes + 0x48) = node;
		CollFixed_WriteS16(spsBytes, 0x1c, CollFixed_ReadU16(spsBytes, 0x10));
		CollFixed_WriteS16(spsBytes, 0x1e, CollFixed_ReadU16(spsBytes, 0x12));
		CollFixed_WriteS16(spsBytes, 0x20, CollFixed_ReadU16(spsBytes, 0x14));
		CollFixed_WriteS16(spsBytes, 0x42, CollFixed_ReadU16(spsBytes, 0x42) + 1);
		return 6;
	}

	CollFixed_WriteS32(spsBytes, 0x1c8, 0);

	diffX = CollFixed_ReadS16(spsBytes, 0) - CollFixed_ReadS16(spsBytes, 0x10);
	diffY = 0;
	diffZ = CollFixed_ReadS16(spsBytes, 4) - CollFixed_ReadS16(spsBytes, 0x14);
	CollFixed_WriteS32(spsBytes, 0x1c4, diffX);
	CollFixed_WriteS32(spsBytes, 0x1cc, diffZ);

	centerDiffX = CollFixed_ReadS16(nodeBytes, 0x10) - CollFixed_ReadS16(spsBytes, 0x10);
	centerDiffY = 0;
	centerDiffZ = CollFixed_ReadS16(nodeBytes, 0x14) - CollFixed_ReadS16(spsBytes, 0x14);
	CollFixed_WriteS32(spsBytes, 0x1d4, 0);
	CollFixed_WriteS32(spsBytes, 0x1d0, centerDiffX);
	CollFixed_WriteS32(spsBytes, 0x1d8, centerDiffZ);

	if ((nodeBytes[0] & 0x40) != 0)
	{
		diffY = CollFixed_ReadS16(spsBytes, 2) - CollFixed_ReadS16(spsBytes, 0x12);
		CollFixed_WriteS32(spsBytes, 0x1c8, diffY);
		centerDiffY = CollFixed_ReadS16(nodeBytes, 0x12) - CollFixed_ReadS16(spsBytes, 0x12);
		CollFixed_WriteS32(spsBytes, 0x1d4, centerDiffY);
	}

	CollFixed_GteLoadR11R12(CollFixed_PackS16Pair(diffX, diffY));
	CollFixed_GteLoadR13R21(CollFixed_PackS16Pair(diffZ, centerDiffX));
	CollFixed_GteLoadR22R23(CollFixed_PackS16Pair(centerDiffY, centerDiffZ));
	CollFixed_GteLoadVXY0(CollFixed_PackS16Pair(diffX, diffY));
	CollFixed_GteLoadVZ0(diffZ);
	CollFixed_GteMVMVA();

	dotSegment = CollFixed_GteReadMAC1();
	dotCenter = CollFixed_GteReadMAC2();
	CollFixed_WriteS32(spsBytes, 0x1ac, dotSegment);
	CollFixed_WriteS32(spsBytes, 0x1b0, dotCenter);

	if (dotCenter <= 0)
		return 0;

	CollFixed_GteLoadLZCS(dotCenter);
	shift = CollFixed_GteReadLZCR() - 2;
	if (shift < 0)
	{
		shift = 0;
	}
	else if (shift > 12)
	{
		shift = 12;
	}

	divisor = dotSegment >> (12 - shift);
	dotCenter = CollFixed_Sll32(dotCenter, shift);

	if (divisor < 0)
		return 0;

	factor = 0;
	if (divisor != 0)
	{
		factor = dotCenter / divisor;
	}
	CollFixed_WriteS32(spsBytes, 0x1b4, factor);

	projX = CollFixed_MulLo(factor, diffX) >> 12;
	projY = 0;
	if ((nodeBytes[0] & 0x40) != 0)
	{
		projY = CollFixed_MulLo(factor, diffY) >> 12;
	}
	projZ = CollFixed_MulLo(factor, diffZ) >> 12;

	relX = projX - centerDiffX;
	relY = projY - centerDiffY;
	relZ = projZ - centerDiffZ;
	CollFixed_WriteS32(spsBytes, 0x1dc, projX);
	CollFixed_WriteS32(spsBytes, 0x1e0, projY);
	CollFixed_WriteS32(spsBytes, 0x1e4, projZ);

	CollFixed_GteLoadR11R12(CollFixed_PackS16Pair(relX, relY));
	CollFixed_GteLoadR13R21(relZ);
	CollFixed_GteLoadVXY0(CollFixed_PackS16Pair(relX, relY));
	CollFixed_GteLoadVZ0(relZ);
	CollFixed_GteMVMVA();

	radius = CollFixed_ReadS16(nodeBytes, 0x16);
	radiusSquared = CollFixed_ReadS16(spsBytes, 6) + radius;
	radiusSquared = CollFixed_MulLo(radiusSquared, radiusSquared);
	distSquared = CollFixed_GteReadMAC1();
	CollFixed_WriteS32(spsBytes, 0x1b8, radiusSquared);
	CollFixed_WriteS32(spsBytes, 0x1bc, distSquared);

	remaining = radiusSquared - distSquared;
	if (remaining < 0)
		return 0;

	if (remaining != 0)
	{
		if (dotSegment != 0)
		{
			factor -= CollFixed_Sll32(remaining, 12) / dotSegment;
		}
		CollFixed_WriteS32(spsBytes, 0x1c0, factor);
	}

	if (CollFixed_ReadS32(spsBytes, 0x84) < factor)
		return 0;

	hitX = 0;
	hitY = 0;
	hitZ = 0;
	if (factor > 0)
	{
		hitX = CollFixed_MulLo(diffX, factor) >> 12;
		hitY = CollFixed_MulLo(diffY, factor) >> 12;
		hitZ = CollFixed_MulLo(diffZ, factor) >> 12;
	}

	if (((nodeBytes[0] & 0x20) != 0) && (hitY < CollFixed_ReadS16(nodeBytes, 0x12)) &&
	    ((CollFixed_ReadS16(nodeBytes, 0x12) + CollFixed_ReadS16(nodeBytes, 2)) < hitY))
	{
		return 0;
	}

	*(struct BSP **)(spsBytes + 0x48) = node;
	CollFixed_WriteS32(spsBytes, 0x84, factor);
	CollFixed_WriteS16(spsBytes, 0x42, CollFixed_ReadU16(spsBytes, 0x42) + 1);
	CollFixed_WriteS32(spsBytes, 0x1e8, hitX);
	CollFixed_WriteS32(spsBytes, 0x1ec, hitY);
	CollFixed_WriteS32(spsBytes, 0x1f0, hitZ);

	normalX = hitX - centerDiffX;
	normalY = 0;
	if ((nodeBytes[0] & 0x40) != 0)
	{
		normalY = hitY - centerDiffY;
	}
	normalZ = hitZ - centerDiffZ;

	CollFixed_GteLoadR11R12(CollFixed_PackS16Pair(normalX, normalY));
	CollFixed_GteLoadR13R21(normalZ);
	CollFixed_GteLoadVXY0(CollFixed_PackS16Pair(normalX, normalY));
	CollFixed_GteLoadVZ0(normalZ);
	CollFixed_GteMVMVA();

	len = SquareRoot0(CollFixed_GteReadMAC1());
	invLen = 0x1000000 / len;

	normalX = CollFixed_MulLo(normalX, invLen) >> 12;
	normalY = CollFixed_MulLo(normalY, invLen) >> 12;
	normalZ = CollFixed_MulLo(normalZ, invLen) >> 12;
	CollFixed_WriteS32(spsBytes, 0x1f4, normalX);
	CollFixed_WriteS32(spsBytes, 0x1f8, normalY);
	CollFixed_WriteS32(spsBytes, 0x1fc, normalZ);

	CollFixed_WriteS16(spsBytes, 0x1c, CollFixed_ReadU16(spsBytes, 0x10) + hitX);
	CollFixed_WriteS16(spsBytes, 0x70, normalX);
	CollFixed_WriteS16(spsBytes, 0x72, normalY);
	CollFixed_WriteS16(spsBytes, 0x74, normalZ);
	CollFixed_WriteS16(spsBytes, 0x20, CollFixed_ReadU16(spsBytes, 0x14) + hitZ);
	CollFixed_WriteS16(spsBytes, 0x1e, CollFixed_ReadU16(spsBytes, 0x12) + hitY);
	spsBytes[0x7e] = 6;

	scaledX = CollFixed_MulLo(normalX, radius) >> 12;
	scaledY = CollFixed_MulLo(normalY, radius) >> 12;
	scaledZ = CollFixed_MulLo(normalZ, radius) >> 12;
	CollFixed_WriteS32(spsBytes, 0x200, scaledX);
	CollFixed_WriteS32(spsBytes, 0x204, scaledY);
	CollFixed_WriteS32(spsBytes, 0x208, scaledZ);

	CollFixed_WriteS16(spsBytes, 0x78, CollFixed_ReadU16(nodeBytes, 0x10) + scaledX);
	CollFixed_WriteS16(spsBytes, 0x68, CollFixed_ReadU16(nodeBytes, 0x10) + scaledX);
	CollFixed_WriteS16(spsBytes, 0x7a, CollFixed_ReadU16(nodeBytes, 0x12) + scaledY);
	CollFixed_WriteS16(spsBytes, 0x6a, CollFixed_ReadU16(nodeBytes, 0x12) + scaledY);
	CollFixed_WriteS16(spsBytes, 0x7c, CollFixed_ReadU16(nodeBytes, 0x14) + scaledZ);
	CollFixed_WriteS16(spsBytes, 0x6c, CollFixed_ReadU16(nodeBytes, 0x14) + scaledZ);

	return 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d610-0x8001d77c
void COLL_FIXED_BSPLEAF_TestInstance(struct BSP *node, struct ScratchpadStruct *sps)
{
	int arraySize;
	struct BSP *bspArray = node->data.leaf.bspHitboxArray;
	struct BoundingBox *bbox;

	if (bspArray == NULL)
		return;

	// check every instance hitbox until
	// end of list (null flag) is found
	for (/**/; bspArray->flag != 0; bspArray++)
	{
		bbox = &bspArray->box;

		// 1F8001CC
		arraySize = sps->numInstHitboxesHit - 1;
		for (; arraySize >= 0; arraySize--)
		{
			if (bspArray == sps->bspInstHitboxArr[arraySize])
				goto NextBSP;
		}

		if ((
		        // if data is invalid (what?)
		        (
		            // if collision for instance is disabled
		            ((bspArray->flag & 0x80) == 0) ||
		            // if bspHitbox.InstDef doesn't exist
		            (bspArray->data.hitbox.instDef == NULL))

		        ||

		        // if data is valid

		        // allows drawing flag is enabled
		        ((bspArray->data.hitbox.instDef->ptrInstance->flags & 0xf) != 0)) &&

		    // compare bsp boundingbox to scratchpad boundingbox
		    ((sps->bbox.min[0] <= bbox->max[0]) &&

		     (bbox->min[0] <= sps->bbox.max[0]) &&

		     (sps->bbox.min[1] <= bbox->max[1]) &&

		     (bbox->min[1] <= sps->bbox.max[1]) &&

		     (sps->bbox.min[2] <= bbox->max[2]) &&

		     (bbox->min[2] <= sps->bbox.max[2])))
		{
			// check with collision for this instance
			COLL_FIXED_INSTANC_TestPoint(sps, bspArray);
		}

	NextBSP:
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d77c-0x8001d944
void COLL_FIXED_BotsSearch(s16 *posCurr, s16 *posPrev, struct ScratchpadStruct *sps)
{
	char i;
	s16 radius = sps->Input1.hitRadius;
	int sqrRadius = CollFixed_MulLo(radius, radius);
	s16 deltaCurr;
	s16 deltaPrev;

	sps->Input1.hitRadiusSquared = sqrRadius;
	sps->Union.QuadBlockColl.hitRadiusSquared = sqrRadius;
	sps->Union.QuadBlockColl.hitRadius = radius;

	for (i = 0; i < 3; i++)
	{
		sps->Input1.pos[i] = posCurr[i];
		sps->Union.QuadBlockColl.hitPos[i] = posCurr[i];
		sps->Union.QuadBlockColl.pos[i] = posPrev[i];

		deltaCurr = posCurr[i] - radius;
		deltaPrev = posPrev[i] - radius;
		sps->bbox.min[i] = (deltaCurr < deltaPrev) ? deltaCurr : deltaPrev;

		deltaCurr = posCurr[i] + radius;
		deltaPrev = posPrev[i] + radius;
		sps->bbox.max[i] = (deltaCurr > deltaPrev) ? deltaCurr : deltaPrev;
	}

	sps->unk3C = 0;
	sps->boolDidTouchHitbox = 0;

	sps->countByOne_ForWhatReason = 0x1000;
	sps->numInstHitboxesHit = 0;
	*(u32 *)&sps->dataOutput[0] = 0;

	COLL_SearchBSP_CallbackPARAM(sps->ptr_mesh_info->bspRoot, &sps->bbox, COLL_FIXED_BSPLEAF_TestInstance, sps);
}


static void COLL_FIXED_TRIANGL_TestPoint_Body(u8 *sps, u8 *v1, u8 *v2, u8 *v3, s32 normalZW);

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001ef1c-0x8001ef50
void COLL_FIXED_TRIANGL_UNUSED(void *sps, void *v1, void *v2, void *v3)
{
	u8 *spsBytes = (u8 *)sps;

	// NOTE(aalhendi): Retail skips the TestPoint setup and jumps into the
	// shared body with t2 preloaded from sps+0x58. Native makes a0-a3 and t2
	// explicit.
	COLL_FIXED_TRIANGL_TestPoint_Body(spsBytes, (u8 *)v1, (u8 *)v2, (u8 *)v3, CollFixed_ReadS32(spsBytes, 0x58));
}


static void COLL_FIXED_TRIANGL_TestPoint_Body(u8 *sps, u8 *v1, u8 *v2, u8 *v3, s32 normalZW)
{
	s32 startX;
	s32 startY;
	s32 startZ;
	s32 deltaX;
	s32 deltaY;
	s32 deltaZ;
	s32 planeDot;
	s32 lineDot;
	s32 factor;
	s32 hitX;
	s32 hitY;
	s32 hitZ;
	s32 normalAxis;
	s32 firstA;
	s32 firstB;
	s32 firstHit;
	s32 secondA;
	s32 secondB;
	s32 secondHit;
	s32 denom;
	s32 baryA;
	s32 baryB;
	u8 *baryV2;
	u8 *baryV3;
	struct QuadBlock *quad;

	startX = CollFixed_ReadS16(sps, 0x10);
	startY = CollFixed_ReadS16(sps, 0x12);
	startZ = CollFixed_ReadS16(sps, 0x14);
	deltaX = CollFixed_ReadS16(sps, 0x1c) - startX;
	deltaY = CollFixed_ReadS16(sps, 0x1e) - startY;
	deltaZ = CollFixed_ReadS16(sps, 0x20) - startZ;

	CollFixed_GteLoadR11R12(CollFixed_ReadU32(sps, 0x10));
	CollFixed_GteLoadR13R21(CollFixed_PackS16Pair(startZ, deltaX));
	CollFixed_GteLoadR22R23(CollFixed_PackS16Pair(deltaY, deltaZ));
	CollFixed_GteLoadVXY0(CollFixed_ReadU32(sps, 0x54));
	CollFixed_GteLoadVZ0(normalZW);

	normalZW = CollFixed_Sll32(normalZW >> 16, 13);

	CollFixed_GteMVMVA();
	lineDot = CollFixed_GteReadMAC2();
	planeDot = CollFixed_GteReadMAC1() - normalZW;

	if (lineDot >= 0)
		return;

	factor = -planeDot / (lineDot >> 12);

	CollFixed_GteLoadMAC(startX, startY, startZ);
	CollFixed_GteLoadIR(deltaX, deltaY, deltaZ);
	CollFixed_GteLoadIR0(factor);

	if ((factor < 0) || (factor > 0x1000))
		return;

	CollFixed_GteGPL12();
	hitX = CollFixed_GteReadMAC1();
	hitY = CollFixed_GteReadMAC2();
	hitZ = CollFixed_GteReadMAC3();

	CollFixed_WriteS16(sps, 0x4c, hitX);
	CollFixed_WriteS16(sps, 0x4e, hitY);
	CollFixed_WriteS16(sps, 0x50, hitZ);

	baryV2 = v2;
	baryV3 = v3;
	normalAxis = CollFixed_ReadS16(sps, 0x52);

	if (normalAxis == 3)
	{
		s32 origin = CollFixed_ReadS16(v1, 4);
		firstA = CollFixed_ReadS16(v2, 4) - origin;
		firstB = CollFixed_ReadS16(v3, 4) - origin;
		firstHit = hitZ - origin;

		if (abs(firstA) < abs(firstB))
		{
			s32 tmp = firstA;
			firstA = firstB;
			firstB = tmp;
			baryV2 = v3;
			baryV3 = v2;
		}

		origin = CollFixed_ReadS16(v1, 0);
		secondA = CollFixed_ReadS16(baryV2, 0) - origin;
		secondB = CollFixed_ReadS16(baryV3, 0) - origin;
		secondHit = hitX - origin;
	}
	else if (normalAxis == 1)
	{
		s32 origin = CollFixed_ReadS16(v1, 0);
		firstA = CollFixed_ReadS16(v2, 0) - origin;
		firstB = CollFixed_ReadS16(v3, 0) - origin;
		firstHit = hitX - origin;

		if (abs(firstA) < abs(firstB))
		{
			s32 tmp = firstA;
			firstA = firstB;
			firstB = tmp;
			baryV2 = v3;
			baryV3 = v2;
		}

		origin = CollFixed_ReadS16(v1, 2);
		secondA = CollFixed_ReadS16(baryV2, 2) - origin;
		secondB = CollFixed_ReadS16(baryV3, 2) - origin;
		secondHit = hitY - origin;
	}
	else
	{
		s32 origin = CollFixed_ReadS16(v1, 2);
		firstA = CollFixed_ReadS16(v2, 2) - origin;
		firstB = CollFixed_ReadS16(v3, 2) - origin;
		firstHit = hitY - origin;

		if (abs(firstA) < abs(firstB))
		{
			s32 tmp = firstA;
			firstA = firstB;
			firstB = tmp;
			baryV2 = v3;
			baryV3 = v2;
		}

		origin = CollFixed_ReadS16(v1, 4);
		secondA = CollFixed_ReadS16(baryV2, 4) - origin;
		secondB = CollFixed_ReadS16(baryV3, 4) - origin;
		secondHit = hitZ - origin;
	}

	baryA = -0x1000;
	baryB = -0x1000;

	if (firstA != 0)
	{
		denom = (CollFixed_MulLo(secondB, firstA) - CollFixed_MulLo(firstB, secondA)) >> 6;

		if (denom != 0)
		{
			baryB = CollFixed_MulLo(CollFixed_MulLo(secondHit, firstA) - CollFixed_MulLo(firstHit, secondA), 0x40) / denom;

			if ((baryB >= 0) && (baryB <= 0x1000))
			{
				baryA = (CollFixed_Sll32(firstHit, 12) - CollFixed_MulLo(baryB, firstB)) / firstA;
			}
		}
	}
	else
	{
		if (firstB == 0)
			return;

		baryB = CollFixed_Sll32(firstHit, 12) / firstB;

		if ((baryB >= 0) && (baryB <= 0x1000))
		{
			baryA = (CollFixed_Sll32(secondHit, 12) - CollFixed_MulLo(baryB, secondB)) / secondA;
		}
	}

	quad = *(struct QuadBlock **)(sps + 0x64);

	if ((baryA < 0) || ((baryA + baryB - 0x1000) > 0))
		return;

	if ((quad->quadFlags & 0x40) != 0)
	{
		CollFixed_WriteU32(sps, 0x1a4, CollFixed_ReadU32(sps, 0x1a4) | (u8)quad->terrain_type);
		return;
	}

	*(struct QuadBlock **)(sps + 0x80) = quad;
	CollFixed_WriteS16(sps, 0xc8, baryA);
	CollFixed_WriteS16(sps, 0xca, baryB);
	*(struct LevVertex **)(sps + 0xcc) = *(struct LevVertex **)(v1 + 8);
	*(struct LevVertex **)(sps + 0xd0) = *(struct LevVertex **)(baryV2 + 8);
	*(struct LevVertex **)(sps + 0xd4) = *(struct LevVertex **)(baryV3 + 8);
	CollFixed_WriteS16(sps, 0x3e, CollFixed_ReadS16(sps, 0x3e) + 1);
	CollFixed_WriteU32(sps, 0x68, CollFixed_ReadU32(sps, 0x4c));
	CollFixed_WriteU32(sps, 0x1c, CollFixed_ReadU32(sps, 0x4c));
	CollFixed_WriteS16(sps, 0x6c, CollFixed_ReadS16(sps, 0x50));
	CollFixed_WriteS16(sps, 0x20, CollFixed_ReadS16(sps, 0x50));
	CollFixed_WriteU32(sps, 0x70, CollFixed_ReadU32(sps, 0x54));
	CollFixed_WriteU32(sps, 0x74, CollFixed_ReadU32(sps, 0x58));
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001ef50-0x8001f2dc
void COLL_FIXED_TRIANGL_TestPoint(void *sps, void *v1, void *v2, void *v3)
{
	u8 *spsBytes = (u8 *)sps;
	u8 *v1Bytes = (u8 *)v1;
	s16 count = CollFixed_ReadS16(spsBytes, 0x3c);
	s32 normalZW = CollFixed_ReadS32(v1Bytes, 0x10);

	CollFixed_WriteS16(spsBytes, 0x3c, count + 1);
	CollFixed_WriteS16(spsBytes, 0x52, CollFixed_ReadS16(v1Bytes, 6));
	CollFixed_WriteU32(spsBytes, 0x54, CollFixed_ReadU32(v1Bytes, 0xc));
	CollFixed_WriteS32(spsBytes, 0x58, normalZW);

	COLL_FIXED_TRIANGL_TestPoint_Body(spsBytes, v1Bytes, (u8 *)v2, (u8 *)v3, normalZW);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f2dc-0x8001f41c
void COLL_FIXED_TRIANGL_GetNormVec(void *sps, void *v1, void *v2, void *v3)
{
	u8 *spsBytes = (u8 *)sps;
	u8 *v1Bytes = (u8 *)v1;
	s32 v1x = CollFixed_ReadS16(v1Bytes, 0);
	s32 v1y = CollFixed_ReadS16(v1Bytes, 2);
	s32 v1z = CollFixed_ReadS16(v1Bytes, 4);
	s32 edgeAX = CollFixed_ReadS16(v3, 0) - v1x;
	s32 edgeAY = CollFixed_ReadS16(v3, 2) - v1y;
	s32 edgeAZ = CollFixed_ReadS16(v3, 4) - v1z;
	s32 edgeBX = CollFixed_ReadS16(v2, 0) - v1x;
	s32 edgeBY = CollFixed_ReadS16(v2, 2) - v1y;
	s32 edgeBZ = CollFixed_ReadS16(v2, 4) - v1z;
	u32 lodShift;
	u32 normalShift;
	s32 scale;
	s32 nx;
	s32 ny;
	s32 nz;
	s32 plane;
	s32 absX;
	s32 absY;
	s32 absZ;
	s16 dominantAxis;

	CollFixed_GteLoadR11R12((u16)edgeAX);
	CollFixed_GteLoadR22R23((u16)edgeAY);
	CollFixed_GteLoadR33((u16)edgeAZ);
	CollFixed_GteLoadIR(edgeBX, edgeBY, edgeBZ);

	lodShift = *(u8 *)(spsBytes + 0x1ab);
	normalShift = *(u8 *)(spsBytes + 0x1aa);
	scale = CollFixed_ReadS16(spsBytes, 0x1a8);

	CollFixed_GteOP0();

	nx = CollFixed_GteReadMAC1();
	ny = CollFixed_GteReadMAC2();
	nz = CollFixed_GteReadMAC3();

	CollFixed_GteLoadR11R12(CollFixed_ReadU32(v1Bytes, 0));
	CollFixed_GteLoadR13R21(CollFixed_ReadU16(v1Bytes, 4));

	nx = CollFixed_MulLo(nx >> (lodShift & 0x1f), scale) >> (normalShift & 0x1f);
	ny = CollFixed_MulLo(ny >> (lodShift & 0x1f), scale) >> (normalShift & 0x1f);
	nz = CollFixed_MulLo(nz >> (lodShift & 0x1f), scale) >> (normalShift & 0x1f);

	CollFixed_GteLoadIR(nx, ny, nz);
	CollFixed_WriteS16(v1Bytes, 0xc, nx);
	CollFixed_WriteS16(v1Bytes, 0xe, ny);

	CollFixed_GteRTIR();
	plane = CollFixed_GteReadMAC1();

	CollFixed_WriteS16(v1Bytes, 0x10, nz);
	CollFixed_WriteS16(v1Bytes, 0x12, plane >> 1);

	absX = nx < 0 ? -nx : nx;
	absY = ny < 0 ? -ny : ny;
	absZ = nz < 0 ? -nz : nz;
	dominantAxis = 1;

	if ((absX - absY) < 0)
	{
		if ((absY - absZ) >= 0)
		{
			dominantAxis = 3;
		}
	}
	else if ((absX - absZ) >= 0)
	{
		dominantAxis = 2;
	}

	CollFixed_WriteS16(v1Bytes, 6, dominantAxis);
}


static struct LevVertex *sCollFixedLoadScratchpadVertsVertexArray;
static struct QuadBlock *sCollFixedLoadScratchpadVertsQuad;

static void COLL_FIXED_QUADBLK_SetLoadScratchpadVertsContext(struct ScratchpadStruct *sps, struct QuadBlock *quad)
{
	// NOTE(aalhendi): Retail passes these through implicit MIPS registers t8/t9.
	// Native records that register state explicitly before calling the loader.
	sCollFixedLoadScratchpadVertsVertexArray = sps->ptr_mesh_info->ptrVertexArray;
	sCollFixedLoadScratchpadVertsQuad = quad;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f7f0-0x8001f928
void COLL_FIXED_QUADBLK_LoadScratchpadVerts(struct ScratchpadStruct *sps)
{
	struct LevVertex *ptrVert = sCollFixedLoadScratchpadVertsVertexArray;
	struct QuadBlock *ptrQuad = sCollFixedLoadScratchpadVertsQuad;
	struct BspSearchVertex *bsv;
	struct LevVertex *vertCurr;
	u16 *index;

	bsv = &sps->bspSearchVert[0];
	*(u32 *)&sps->unkVecE8[2] = *(u32 *)&ptrQuad->index[2];

	for (index = (u16 *)&ptrQuad->index[0]; index < (u16 *)&ptrQuad->index[9]; index++, bsv++)
	{
		vertCurr = &ptrVert[*index];
		bsv->pLevelVertex = vertCurr;
		*(int *)&bsv->pos[0] = *(int *)&vertCurr->pos[0];
		*(int *)&bsv->pos[2] = *(int *)&vertCurr->pos[2];
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f67c-0x8001f6f0
void COLL_FIXED_QUADBLK_GetNormVecs_LoLOD(struct ScratchpadStruct *sps, struct QuadBlock *quad)
{
	COLL_FIXED_QUADBLK_SetLoadScratchpadVertsContext(sps, quad);
	COLL_FIXED_QUADBLK_LoadScratchpadVerts(sps);

	// always 2 for low poly (big block)
	sps->dataOutput[7] = 2;

	sps->dataOutput[6] = quad->triNormalVecBitShift;

	// TriNormalVecDividend
	s16 *SPS_TNVD = &sps->dataOutput[4];
	s16 *QBL_TNVD = &quad->triNormalVecDividend[0];

	struct BspSearchVertex *bsv = &sps->bspSearchVert[0];

	if ((u16)sps->unkVecE8[2] != (u16)sps->unkVecE8[3])
	{
		*SPS_TNVD = QBL_TNVD[9];
		COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[1], &bsv[3], &bsv[2]); // 1, 3, 2
	}
	*SPS_TNVD = QBL_TNVD[8];
	COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[0], &bsv[1], &bsv[2]); // 0, 1, 2
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f6f0-0x8001f7f0
void COLL_FIXED_QUADBLK_GetNormVecs_HiLOD(struct ScratchpadStruct *sps, struct QuadBlock *quad)
{
	COLL_FIXED_QUADBLK_SetLoadScratchpadVertsContext(sps, quad);
	COLL_FIXED_QUADBLK_LoadScratchpadVerts(sps);

	// always 0 for high poly (small block)
	sps->dataOutput[7] = 0;

	sps->dataOutput[6] = quad->triNormalVecBitShift;

	// TriNormalVecDividend
	s16 *SPS_TNVD = &sps->dataOutput[4];
	s16 *QBL_TNVD = &quad->triNormalVecDividend[0];

	// calculate normal vectors for eight triangles,
	// no collision detection here
	struct BspSearchVertex *bsv = &sps->bspSearchVert[0];

	if ((u16)sps->unkVecE8[2] != (u16)sps->unkVecE8[3])
	{
		*SPS_TNVD = QBL_TNVD[4];                                       // triangle 4
		COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[8], &bsv[6], &bsv[7]); // 8, 6, 7

		*SPS_TNVD = QBL_TNVD[5];                                       // triangle 5
		COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[7], &bsv[3], &bsv[8]); // 7, 3, 8

		*SPS_TNVD = QBL_TNVD[6];                                       // triangle 6
		COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[1], &bsv[7], &bsv[6]); // 1, 7, 6

		*SPS_TNVD = QBL_TNVD[7];                                       // triangle 7
		COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[2], &bsv[6], &bsv[8]); // 2, 6, 8
	}

	*SPS_TNVD = QBL_TNVD[0];                                       // triangle 0
	COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[0], &bsv[4], &bsv[5]); // 0, 4, 5

	*SPS_TNVD = QBL_TNVD[1];                                       // triangle 1
	COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[4], &bsv[6], &bsv[5]); // 4, 6, 5

	*SPS_TNVD = QBL_TNVD[2];                                       // triangle 2
	COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[6], &bsv[4], &bsv[1]); // 6, 4, 1

	*SPS_TNVD = QBL_TNVD[3];                                       // triangle 3
	COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[5], &bsv[6], &bsv[2]); // 5, 6, 2
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f41c-0x8001f5f0
void COLL_FIXED_QUADBLK_TestTriangles(struct QuadBlock *quad, struct ScratchpadStruct *sps)
{
	struct BspSearchVertex *bsv = &sps->bspSearchVert[0];

	sps->Set1.ptrQuadblock = quad;

	if (((sps->Union.QuadBlockColl.qbFlagsWanted & quad->quadFlags) == 0) || ((sps->Union.QuadBlockColl.qbFlagsIgnored & quad->quadFlags) != 0) ||
	    (quad->bbox.min[0] > sps->bbox.max[0]) || (quad->bbox.min[1] > sps->bbox.max[1]) || (quad->bbox.min[2] > sps->bbox.max[2]) ||
	    (sps->bbox.min[0] > quad->bbox.max[0]) || (sps->bbox.min[1] > quad->bbox.max[1]) || (sps->bbox.min[2] > quad->bbox.max[2]))
	{
		return;
	}

	// if 3P or 4P mode,
	// then use low-LOD quadblock collision (two triangles)
	if ((sps->Union.QuadBlockColl.searchFlags & 2) == 0)
	{
		COLL_FIXED_QUADBLK_GetNormVecs_LoLOD(sps, quad);

		COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[0], &bsv[1], &bsv[2]); // 0,1,2

		// If this is a quad instead of a triangle
		if (quad->index[2] != quad->index[3])
		{
			COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[1], &bsv[3], &bsv[2]); // 1,3,2
		}
	}
	else
	{
		if ((sps->Union.QuadBlockColl.searchFlags & 8) == 0)
		{
			COLL_FIXED_QUADBLK_GetNormVecs_HiLOD(sps, quad);
		}

		COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[0], &bsv[4], &bsv[5]); // 0, 4, 5
		COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[4], &bsv[6], &bsv[5]); // 4, 6, 5
		COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[6], &bsv[4], &bsv[1]); // 6, 4, 1
		COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[5], &bsv[6], &bsv[2]); // 5, 6, 2

		// If this is a quad instead of a triangle
		if (quad->index[2] != quad->index[3])
		{
			COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[8], &bsv[6], &bsv[7]); // 8, 6, 7
			COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[7], &bsv[3], &bsv[8]); // 7, 3, 8
			COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[1], &bsv[7], &bsv[6]); // 1, 7, 6
			COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[2], &bsv[6], &bsv[8]); // 2, 6, 8
		}
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f5f0-0x8001f67c
void COLL_FIXED_BSPLEAF_TestQuadblocks(struct BSP *node, struct ScratchpadStruct *sps)
{
	int numQuads;
	struct QuadBlock *ptrQuad;

	// if bsp flag is water
	if ((node->flag & 2) != 0)
	{
		*(int *)&sps->dataOutput[0] |= 0x8000;
	}

	numQuads = node->data.leaf.numQuads;
	ptrQuad = node->data.leaf.ptrQuadBlockArray;

	// loop through all quadblocks
	do
	{
		COLL_FIXED_QUADBLK_TestTriangles(ptrQuad++, sps);
		numQuads--;
	} while (numQuads > 0);

	if ((sps->Union.QuadBlockColl.searchFlags & 1) != 0)
	{
		COLL_FIXED_BSPLEAF_TestInstance(node, sps);
	}
}


struct CollFixedPlayerTrig
{
	s32 x;
	s32 z;
};

static int COLL_FIXED_PlayerSearch_Abs(int value)
{
	return (value < 0) ? (s32)(0u - (u32)value) : value;
}

static s32 COLL_FIXED_PlayerSearch_ClampByte(s32 value)
{
	if (value < 0)
		return 0;

	if (value > 0xff)
		return 0xff;

	return value;
}

static struct CollFixedPlayerTrig COLL_FIXED_PlayerSearch_Trig(s32 angle)
{
	struct TrigTable trig = data.trigApprox[angle & 0x3ff];
	struct CollFixedPlayerTrig out;

	if ((angle & 0x400) == 0)
	{
		out.x = trig.sin;
		out.z = trig.cos;

		if ((angle & 0x800) != 0)
		{
			out.x = -out.x;
			out.z = -out.z;
		}
	}
	else
	{
		out.x = trig.cos;
		out.z = -trig.sin;

		if ((angle & 0x800) != 0)
		{
			out.x = -out.x;
			out.z = -out.z;
		}
	}

	return out;
}

static u32 COLL_FIXED_PlayerSearch_CompressNormal(s16 normalX, s16 normalY, s16 normalZ, u8 driverID)
{
	return (((u16)normalX >> 6) & 0xff) | ((((u16)normalY >> 6) & 0xff) << 8) | ((((u16)normalZ >> 6) & 0xff) << 16) | ((driverID + 1) << 24);
}

static void COLL_FIXED_PlayerSearch_SetupSearch(struct ScratchpadStruct *sps, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;
	s16 topX = d->posCurr.x >> 8;
	s16 topY = (d->posCurr.y >> 8) + 0x80;
	s16 topZ = d->posCurr.z >> 8;
	s16 bottomY = (d->posCurr.y >> 8) - 0x100;

	d->actionsFlagSet &= 0xfffeffff;

	sps->Union.QuadBlockColl.pos[0] = topX;
	sps->Union.QuadBlockColl.pos[1] = topY;
	sps->Union.QuadBlockColl.pos[2] = topZ;

	sps->Input1.pos[0] = topX;
	sps->Input1.pos[1] = bottomY;
	sps->Input1.pos[2] = topZ;

	sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;
	sps->Union.QuadBlockColl.qbFlagsIgnored = 0x10;
	sps->Union.QuadBlockColl.qbFlagsWanted = 0x3000;

	sps->Union.QuadBlockColl.searchFlags = 0;
	if (gGT->numPlyrCurrGame < 3)
	{
		sps->Union.QuadBlockColl.searchFlags = 2;
	}

	sps->boolDidTouchQuadblock = 0;
	sps->boolDidTouchHitbox = 0;
	sps->unk3C = 0;

	sps->bbox.min[0] = topX;
	sps->bbox.max[0] = topX;
	sps->bbox.min[1] = (bottomY < topY) ? bottomY : topY;
	sps->bbox.max[1] = (topY < bottomY) ? bottomY : topY;
	sps->bbox.min[2] = topZ;
	sps->bbox.max[2] = topZ;

	sps->Union.QuadBlockColl.hitPos[0] = sps->Input1.pos[0];
	sps->Union.QuadBlockColl.hitPos[1] = sps->Input1.pos[1];
	sps->Union.QuadBlockColl.hitPos[2] = sps->Input1.pos[2];
}

static void COLL_FIXED_PlayerSearch_UpdateLighting(struct ScratchpadStruct *sps, struct Driver *d, struct Instance *inst)
{
	struct LevVertex *v0 = sps->levVertHit[0];
	struct LevVertex *v1 = sps->levVertHit[1];
	struct LevVertex *v2 = sps->levVertHit[2];

	if ((v0 == NULL) || (v1 == NULL) || (v2 == NULL))
		return;

	s32 baryA = sps->barycentrics[0];
	s32 baryB = sps->barycentrics[1];
	s32 r0 = v0->color_hi[0];
	s32 g0 = v0->color_hi[1];
	s32 b0 = v0->color_hi[2];
	s32 r = (CollFixed_MulLo(baryA, v1->color_hi[0] - r0) >> 12) + (CollFixed_MulLo(baryB, v2->color_hi[0] - r0) >> 12) + r0;
	s32 g = (CollFixed_MulLo(baryA, v1->color_hi[1] - g0) >> 12) + (CollFixed_MulLo(baryB, v2->color_hi[1] - g0) >> 12) + g0;
	s32 b = (CollFixed_MulLo(baryA, v1->color_hi[2] - b0) >> 12) + (CollFixed_MulLo(baryB, v2->color_hi[2] - b0) >> 12) + b0;

	r = COLL_FIXED_PlayerSearch_ClampByte(r);
	g = COLL_FIXED_PlayerSearch_ClampByte(g);
	b = COLL_FIXED_PlayerSearch_ClampByte(b);

	s32 light = CollFixed_MulLo(((CollFixed_MulLo(r, 0x4c) >> 8) + (CollFixed_MulLo(g, 0x96) >> 8) + (CollFixed_MulLo(b, 0x1e) >> 8)), -0x20) + 0xc00;
	s32 scaledLight;

	if (light < 0)
	{
		light = 0;
	}

	scaledLight = light << 3;

	if (light > 0x1000)
	{
		light = 0x1000;
		scaledLight = 0x8000;
	}

	light = CollFixed_MulLo(scaledLight - light, 8);

	d->alphaScaleBackup = (CollFixed_MulLo(d->alphaScaleBackup, 200) + light) >> 8;
	inst->alphaScale = (CollFixed_MulLo(inst->alphaScale, 200) + light) >> 8;
}

static void COLL_FIXED_PlayerSearch_NormalizeAxis3(struct ScratchpadStruct *sps, struct Driver *d)
{
	s32 x = CollFixed_MulLo(d->AxisAngle3_normalVec[0], 5) + CollFixed_MulLo(sps->Set2.normalVec[0], 3);
	s32 y = CollFixed_MulLo(d->AxisAngle3_normalVec[1], 5) + CollFixed_MulLo(sps->Set2.normalVec[1], 3);
	s32 z = CollFixed_MulLo(d->AxisAngle3_normalVec[2], 5) + CollFixed_MulLo(sps->Set2.normalVec[2], 3);
	u32 sum = (u32)CollFixed_MulLo(x, x) + (u32)CollFixed_MulLo(y, y) + (u32)CollFixed_MulLo(z, z);
	u32 len = VehCalc_FastSqrt(sum, 0x18) >> 12;

	d->AxisAngle3_normalVec[0] = CollFixed_Sll32(x, 12) / (s32)len;
	d->AxisAngle3_normalVec[1] = CollFixed_Sll32(y, 12) / (s32)len;
	d->AxisAngle3_normalVec[2] = CollFixed_Sll32(z, 12) / (s32)len;
}

static void COLL_FIXED_PlayerSearch_NormalizeAxis2(struct Driver *d, s32 x, s32 y, s32 z)
{
	u32 sum = (u32)CollFixed_MulLo(x, x) + (u32)CollFixed_MulLo(y, y) + (u32)CollFixed_MulLo(z, z);
	u32 len = VehCalc_FastSqrt(sum, 0x18) >> 12;

	d->AxisAngle2_normalVec[0] = CollFixed_Sll32(x, 12) / (s32)len;
	d->AxisAngle2_normalVec[1] = CollFixed_Sll32(y, 12) / (s32)len;
	d->AxisAngle2_normalVec[2] = CollFixed_Sll32(z, 12) / (s32)len;
}

static int COLL_FIXED_PlayerSearch_CheckMaskGrabProgress(struct Driver *d, struct QuadBlock *quad)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Level *level = gGT->level1;

	if ((quad->quadFlags & 0x200) != 0)
		return 1;

	if ((d->kartState == KS_MASK_GRABBED) || ((d->unkAA & 1) != 0) || ((quad->quadFlags & 0x1000) == 0))
		return 0;

	if (quad->checkpointIndex == 0xff)
	{
		if ((u32)(gGT->levelID - GEM_STONE_VALLEY) < 5)
		{
			d->lastValid = quad;
		}
		return 0;
	}

	struct CheckpointNode *node = &level->ptr_restart_points[quad->checkpointIndex];

	if (((d->actionsFlagSet & 0x1000000) == 0) && (node->nextIndex_forward > 1) &&
	    ((((level->ptr_restart_points[0].distToFinish >> 2) << 3) < (int)(d->distanceToFinish_checkpoint - CollFixed_MulLo(node->distToFinish, 8)))))
	{
		return 1;
	}

	u16 trackLength = level->ptr_restart_points[0].distToFinish;

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Retail reaches directly through lastValid here. Native
	// can enter this early checkpoint path before spawn/collision has seeded
	// lastValid, and cannot mirror PS1 low-memory null-space reads.
	if (d->lastValid == NULL)
	{
		d->lastValid = d->currBlockTouching;
		return 0;
	}
#endif

	if ((node->distToFinish < (CollFixed_MulLo(trackLength, 0xf) >> 4)) && (d->lastValid->checkpointIndex != 0xff) &&
	    ((level->ptr_restart_points[d->lastValid->checkpointIndex].distToFinish + (trackLength >> 2)) < node->distToFinish))
	{
		return 1;
	}

	d->lastValid = d->currBlockTouching;
	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d944-0x8001eb0c
void COLL_FIXED_PlayerSearch(struct Thread *t, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Level *level = gGT->level1;
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
	struct Instance *inst;
	struct QuadBlock *quad;
	s32 landingDelta;
	s32 lerpFrames;

	COLL_FIXED_PlayerSearch_SetupSearch(sps, d);

	if (d->underDriver != NULL)
	{
		COLL_FIXED_QUADBLK_TestTriangles(d->underDriver, sps);
	}

	if ((sps->boolDidTouchQuadblock == 0) && (sps->ptr_mesh_info != NULL) && (sps->ptr_mesh_info->bspRoot != NULL))
	{
		COLL_SearchBSP_CallbackPARAM(sps->ptr_mesh_info->bspRoot, &sps->bbox, COLL_FIXED_BSPLEAF_TestQuadblocks, sps);
	}

	inst = t->inst;

	if (sps->boolDidTouchQuadblock == 0)
	{
		inst->bitCompressed_NormalVector_AndDriverIndex = ((d->driverID + 1) << 24) | 0x4000;
		inst->flags &= ~REFLECTIVE;
		d->quadBlockHeight = d->posCurr.y - 0x10000;
	}
	else
	{
		quad = sps->Set2.ptrQuadblock;
		inst->bitCompressed_NormalVector_AndDriverIndex =
		    COLL_FIXED_PlayerSearch_CompressNormal(sps->Set2.normalVec[0], sps->Set2.normalVec[1], sps->Set2.normalVec[2], d->driverID);
		d->quadBlockHeight = sps->Union.QuadBlockColl.hitPos[1] << 8;
		d->unkAA |= 4;

		if ((quad->terrain_type == TERRAIN_MUD) || (quad->terrain_type == TERRAIN_WATER) || (quad->terrain_type == TERRAIN_FASTWATER))
		{
			inst->vertSplit = 0;
			inst->flags |= SPLIT_LINE;
		}

		if (gGT->numPlyrCurrGame < 2)
		{
			u16 quadFlags = quad->quadFlags;

			if ((quadFlags & 0x2000) == 0)
			{
				if ((quadFlags & 1) != 0)
				{
					inst->flags |= REFLECTIVE;
					inst->vertSplit = *(s16 *)((u8 *)level + 0x186);
				}
				else if ((quadFlags & 4) != 0)
				{
					inst->flags |= REFLECTIVE;
					inst->vertSplit = *(s16 *)((u8 *)level + 0x184);
				}
				else
				{
					inst->flags &= ~REFLECTIVE;
				}
			}
		}

		COLL_FIXED_PlayerSearch_NormalizeAxis3(sps, d);

		if ((quad->quadFlags & 0x80) != 0)
		{
			d->actionsFlagSet |= 0x10000;
		}

		d->underDriver = quad;

		if ((d->posCurr.y <= d->quadBlockHeight + 0x1000) || ((quad->terrain_type == TERRAIN_MUD) && (d->posCurr.y < 1)))
		{
			if ((quad->quadFlags & 0x1000) != 0)
			{
				d->normalVecUP.x = sps->Set2.normalVec[0];
				d->normalVecUP.y = sps->Set2.normalVec[1];
				d->normalVecUP.z = sps->Set2.normalVec[2];
				d->unkAA |= 8;
			}

			if (d->currBlockTouching == NULL)
			{
				d->currBlockTouching = quad;
				d->AxisAngle1_normalVec.x = sps->Set2.normalVec[0];
				d->AxisAngle1_normalVec.y = sps->Set2.normalVec[1];
				d->AxisAngle1_normalVec.z = sps->Set2.normalVec[2];
			}

			COLL_FIXED_PlayerSearch_UpdateLighting(sps, d, inst);
		}
	}

	if (d->quadBlockHeight + 0x8000 < d->posCurr.y)
	{
		d->terrainMeta2 = VehAfterColl_GetTerrain(TERRAIN_NONE);
	}

	if (d->posCurr.y < ((s32)level->ptr_mesh_info->bspRoot->box.min[1] - 0x40) * 0x100)
	{
		d->unkAA |= 1;
	}

	landingDelta = d->velocity.y - d->ySpeed;

	if ((d->currBlockTouching != NULL) && ((d->unkAA & 9) == 0) && (d->kartState != KS_MASK_GRABBED))
	{
		d->velocity.x += d->AxisAngle1_normalVec.x >> 1;
		d->velocity.y += d->AxisAngle1_normalVec.y >> 1;
		d->velocity.z += d->AxisAngle1_normalVec.z >> 1;
	}

	quad = d->currBlockTouching;

	d->xSpeed = d->velocity.x;
	d->ySpeed = d->velocity.y;
	d->zSpeed = d->velocity.z;

	if (quad == NULL)
	{
		goto DriverAirborne;
	}

	if (COLL_FIXED_PlayerSearch_CheckMaskGrabProgress(d, quad) != 0)
	{
		d->unkAA |= 1;
	}

	d->jump_LandingBoost = 0;
	d->actionsFlagSet &= 0xfff7ffbf;

	if ((d->unkAA & 8) == 0)
	{
		goto DriverAirborne;
	}

	if ((d->driverRankItemValue == 2) || ((gGT->gameMode2 & 0x80000) != 0))
	{
		d->currentTerrain = TERRAIN_ICE;
	}
	else
	{
		u8 terrainType = d->currBlockTouching->terrain_type;

		if ((terrainType != TERRAIN_ICE) && (d->currentTerrain == TERRAIN_ICE))
		{
			d->filler_short = 0xfec0;
		}

		d->currentTerrain = terrainType;
	}

	d->terrainMeta1 = VehAfterColl_GetTerrain(d->currentTerrain);
	d->terrainMeta2 = d->terrainMeta1;
	d->jump_CoyoteTimerMS = 0xa0;

	{
		u32 actions = d->actionsFlagSet;

		d->actionsFlagSet = actions | 1;

		if ((d->actionsFlagSetPrevFrame & 1) == 0)
		{
			s32 absLanding = COLL_FIXED_PlayerSearch_Abs(landingDelta);
			u32 volume;

			d->actionsFlagSet = actions | 0x83;
			d->filler_short = 0x140;

			volume = VehCalc_MapToRange(absLanding, 0x100, 0x3c00, 0x78, 0xfa);

			if (d->kartState != KS_MASK_GRABBED)
			{
				volume = (volume & 0xff) << 16;

				if ((d->actionsFlagSet & 0x10000) == 0)
				{
					volume |= 0x8080;
				}
				else
				{
					volume |= 0x1008080;
				}

				OtherFX_Play_LowLevel(7, 1, volume);
			}
		}
	}

	lerpFrames = 6;
	goto BlendNormal;

DriverAirborne:
	if (d->jump_CooldownMS != 0)
	{
		d->actionsFlagSet |= 0x80000;
	}

	if (d->jump_unknown != 0)
	{
		d->actionsFlagSet |= 0x40;
	}

	d->terrainMeta1 = VehAfterColl_GetTerrain(TERRAIN_NONE);
	d->currentTerrain = TERRAIN_NONE;
	d->actionsFlagSet &= ~1u;

	d->jump_LandingBoost += gGT->elapsedTimeMS;
	d->jump_CoyoteTimerMS -= gGT->elapsedTimeMS;
	if (d->jump_CoyoteTimerMS < 0)
	{
		d->jump_CoyoteTimerMS = 0;
	}

	lerpFrames = 7;
	if (d->jump_CoyoteTimerMS == 0)
	{
		d->jump_CooldownMS = 0;
		d->jump_unknown = 0;
	}

BlendNormal:
{
	s32 invFrames = 8 - lerpFrames;
	s32 normalX = (CollFixed_MulLo(lerpFrames, d->AxisAngle2_normalVec[0]) + CollFixed_MulLo(invFrames, d->normalVecUP.x)) >> 3;
	s32 normalY = (CollFixed_MulLo(lerpFrames, d->AxisAngle2_normalVec[1]) + CollFixed_MulLo(invFrames, d->normalVecUP.y)) >> 3;
	s32 normalZ = (CollFixed_MulLo(lerpFrames, d->AxisAngle2_normalVec[2]) + CollFixed_MulLo(invFrames, d->normalVecUP.z)) >> 3;

	if (d->hazardTimer > 0)
	{
		struct CollFixedPlayerTrig trig = COLL_FIXED_PlayerSearch_Trig(CollFixed_MulLo(d->hazardTimer, 0xc));
		s16 input[4] = {CollFixed_MulLo(trig.x, 0x19) >> 10, 0, CollFixed_MulLo(trig.z, 0x19) >> 10, 0};
		int output[3];

		gte_ldv0(input);
		gte_rtv0();
		gte_stlvnl(output);

		normalX += output[0];
		normalY += output[1];
		normalZ += output[2];
	}

	COLL_FIXED_PlayerSearch_NormalizeAxis2(d, normalX, normalY, normalZ);
}

	{
		struct CollFixedPlayerTrig trig = COLL_FIXED_PlayerSearch_Trig(d->angle);

		d->rotCurr.z = ratan2((CollFixed_MulLo(-d->AxisAngle2_normalVec[0], trig.z) + CollFixed_MulLo(d->AxisAngle2_normalVec[2], trig.x)) >> 12,
		                      d->AxisAngle2_normalVec[1]);
	}

	if (d->hazardTimer < 1)
	{
		if ((d->actionsFlagSet & 1) != 0)
		{
			s32 speed = COLL_FIXED_PlayerSearch_Abs(d->speed);

			if (speed > 0x1000)
			{
				s32 screenOffset = COLL_FIXED_PlayerSearch_Abs((s8)d->Screen_OffsetY);

				if ((screenOffset < 4) && ((d->terrainMeta1->flags & 1) != 0))
				{
					d->distanceFromGround = 4;
					goto UpdateGroundOffset;
				}
			}
		}

		d->distanceFromGround = 0;
	}
	else
	{
		s32 screenOffset = COLL_FIXED_PlayerSearch_Abs((s8)d->Screen_OffsetY);

		if (screenOffset < 4)
		{
			d->distanceFromGround = 4;

			if ((d->kartState != 3) && ((s8)d->Screen_OffsetY > 0))
			{
				OtherFX_Play(0x10, 1);
			}
		}
	}

UpdateGroundOffset:
	if (COLL_FIXED_PlayerSearch_Abs((s8)d->Screen_OffsetY) > 9)
	{
		d->distanceFromGround = 0;
	}

	if (d->distanceFromGround == 0)
	{
		s8 nextOffset = (s8)d->Screen_OffsetY - 4;

		if ((s8)d->Screen_OffsetY > 0)
		{
			d->Screen_OffsetY = nextOffset;

			if (nextOffset < 1)
			{
				d->Screen_OffsetY = 0;

				if ((d->terrainMeta1->flags & 0x20) != 0)
				{
					u32 soundFlags = ((d->actionsFlagSet & 0x10000) != 0) ? 0x1808080 : 0x808080;

					OtherFX_Play_LowLevel(d->terrainMeta1->sound, 0, soundFlags);
				}
			}
		}

		nextOffset = (s8)d->Screen_OffsetY - 4;
		d->Screen_OffsetY = nextOffset;

		if (nextOffset < 0)
		{
			d->Screen_OffsetY = 0;
		}
	}
	else
	{
		d->distanceFromGround--;
		d->Screen_OffsetY += 3;
	}

	if ((d->posCurr.y < -0x8000) && ((level->configFlags & 2) != 0))
	{
		d->unkAA |= 1;
	}

	if ((d->kartState != KS_MASK_GRABBED) && ((d->unkAA & 1) != 0) && (d->lastValid != NULL) && ((sdata->HudAndDebugFlags & 0x1000) == 0) &&
	    ((d->stepFlagSet & 8) == 0))
	{
		VehStuckProc_MaskGrab_Init(t, d);
	}
}


static void CollMoved_GteRTV0(void)
{
	doCOP2(0x0486012);
}

static void CollMoved_GteGPF12(void)
{
	doCOP2(0x0198003d);
}

static void CollMoved_CopyVertexPos(void *dst, const void *src)
{
	CollFixed_WriteU32(dst, 0, CollFixed_ReadU32(src, 0));
	CollFixed_WriteS16(dst, 4, CollFixed_ReadS16(src, 4));
}

static void CollMoved_SelectProjection(s32 normalAxis, void *set1, void *v1, void **v2, void **v3, s32 *firstA, s32 *firstB, s32 *firstHit, s32 *secondA,
                                       s32 *secondB, s32 *secondHit)
{
	s32 origin;
	void *tmp;

	if (normalAxis == 3)
	{
		origin = CollFixed_ReadS16(v1, 4);
		*firstA = CollFixed_ReadS16(*v2, 4) - origin;
		*firstB = CollFixed_ReadS16(*v3, 4) - origin;
		*firstHit = CollFixed_ReadS16(set1, 0x14) - origin;

		if (((*firstA < 0) ? -*firstA : *firstA) < ((*firstB < 0) ? -*firstB : *firstB))
		{
			s32 tmpValue = *firstA;
			*firstA = *firstB;
			*firstB = tmpValue;
			tmp = *v2;
			*v2 = *v3;
			*v3 = tmp;
		}

		origin = CollFixed_ReadS16(v1, 0);
		*secondA = CollFixed_ReadS16(*v2, 0) - origin;
		*secondB = CollFixed_ReadS16(*v3, 0) - origin;
		*secondHit = CollFixed_ReadS16(set1, 0x10) - origin;
	}
	else if (normalAxis == 1)
	{
		origin = CollFixed_ReadS16(v1, 0);
		*firstA = CollFixed_ReadS16(*v2, 0) - origin;
		*firstB = CollFixed_ReadS16(*v3, 0) - origin;
		*firstHit = CollFixed_ReadS16(set1, 0x10) - origin;

		if (((*firstA < 0) ? -*firstA : *firstA) < ((*firstB < 0) ? -*firstB : *firstB))
		{
			s32 tmpValue = *firstA;
			*firstA = *firstB;
			*firstB = tmpValue;
			tmp = *v2;
			*v2 = *v3;
			*v3 = tmp;
		}

		origin = CollFixed_ReadS16(v1, 2);
		*secondA = CollFixed_ReadS16(*v2, 2) - origin;
		*secondB = CollFixed_ReadS16(*v3, 2) - origin;
		*secondHit = CollFixed_ReadS16(set1, 0x12) - origin;
	}
	else
	{
		origin = CollFixed_ReadS16(v1, 2);
		*firstA = CollFixed_ReadS16(*v2, 2) - origin;
		*firstB = CollFixed_ReadS16(*v3, 2) - origin;
		*firstHit = CollFixed_ReadS16(set1, 0x12) - origin;

		if (((*firstA < 0) ? -*firstA : *firstA) < ((*firstB < 0) ? -*firstB : *firstB))
		{
			s32 tmpValue = *firstA;
			*firstA = *firstB;
			*firstB = tmpValue;
			tmp = *v2;
			*v2 = *v3;
			*v3 = tmp;
		}

		origin = CollFixed_ReadS16(v1, 4);
		*secondA = CollFixed_ReadS16(*v2, 4) - origin;
		*secondB = CollFixed_ReadS16(*v3, 4) - origin;
		*secondHit = CollFixed_ReadS16(set1, 0x14) - origin;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f928-0x8001fc40
s32 COLL_MOVED_TRIANGL_ReorderNormals(void *set1, void *v1, void *v2, void *v3)
{
	void *baryV2 = v2;
	void *baryV3 = v3;
	s32 firstA;
	s32 firstB;
	s32 firstHit;
	s32 secondA;
	s32 secondB;
	s32 secondHit;
	s32 baryA = -0x1000;
	s32 baryB = -0x1000;
	s32 sum;

	CollMoved_SelectProjection(CollFixed_ReadS16(set1, 6), set1, v1, &baryV2, &baryV3, &firstA, &firstB, &firstHit, &secondA, &secondB, &secondHit);

	if (firstA == 0)
	{
		if (firstB == 0)
			return -1;

		baryB = CollFixed_Sll32(firstHit, 12) / firstB;

		if (secondA != 0)
			baryA = (CollFixed_Sll32(secondHit, 12) - CollFixed_MulLo(baryB, secondB)) / secondA;
	}
	else
	{
		s32 denom = (CollFixed_MulLo(secondB, firstA) - CollFixed_MulLo(firstB, secondA)) >> 6;

		if (denom != 0)
		{
			baryB = CollFixed_MulLo(CollFixed_MulLo(secondHit, firstA) - CollFixed_MulLo(firstHit, secondA), 0x40) / denom;
			baryA = (CollFixed_Sll32(firstHit, 12) - CollFixed_MulLo(baryB, firstB)) / firstA;
		}
	}

	if (baryA == -0x1000)
		return -1;

	sum = baryA + baryB - 0x1000;

	if (baryA < 0)
	{
		if (baryB < 0)
		{
			CollMoved_CopyVertexPos(set1, v1);
			return 0;
		}

		if (sum >= 0)
		{
			CollMoved_CopyVertexPos(set1, baryV3);
			return 4;
		}

		COLL_FIXED_TRIANGL_Barycentrics((s16 *)set1, (s16 *)v1, (s16 *)baryV3, (s16 *)((u8 *)set1 + 0x10));
		return 5;
	}

	if (baryB >= 0)
	{
		if (sum <= 0)
		{
			CollMoved_CopyVertexPos(set1, (u8 *)set1 + 0x10);
			return 6;
		}

		COLL_FIXED_TRIANGL_Barycentrics((s16 *)set1, (s16 *)baryV2, (s16 *)baryV3, (s16 *)((u8 *)set1 + 0x10));
		return 3;
	}

	if (sum >= 0)
	{
		CollMoved_CopyVertexPos(set1, baryV2);
		return 2;
	}

	COLL_FIXED_TRIANGL_Barycentrics((s16 *)set1, (s16 *)v1, (s16 *)baryV2, (s16 *)baryV3);
	return 1;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001fc40-0x80020064
void COLL_MOVED_TRIANGL_TestPoint(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	u8 *spsBytes = (u8 *)sps;
	u8 *v1Bytes = (u8 *)v1;
	struct QuadBlock *quad;
	u16 quadFlags;
	s32 planeNear;
	s32 planeFar;
	s32 normalW;
	s32 projectedFromInput;
	s32 distanceSq;
	s32 distance;
	s32 hitX;
	s32 hitY;
	s32 hitZ;
	s32 reorderResult;

	CollFixed_WriteS16(spsBytes, 0x3c, CollFixed_ReadS16(spsBytes, 0x3c) + 1);
	CollFixed_WriteS16(spsBytes, 0x52, CollFixed_ReadS16(v1Bytes, 6));
	CollFixed_WriteU32(spsBytes, 0x54, CollFixed_ReadU32(v1Bytes, 0xc));
	CollFixed_WriteU32(spsBytes, 0x58, CollFixed_ReadU32(v1Bytes, 0x10));

	quad = *(struct QuadBlock **)(spsBytes + 0x64);

	if (((quad->quadFlags & 0x400) != 0) && (((s32)(s8)quad->terrain_type & sdata->doorAccessFlags) != 0))
		return;

	CollFixed_GteLoadR11R12(CollFixed_ReadU32(spsBytes, 0));
	CollFixed_GteLoadR13R21(CollFixed_ReadU32(spsBytes, 0x10) << 16 | CollFixed_ReadU16(spsBytes, 4));
	CollFixed_GteLoadR22R23((CollFixed_ReadU32(spsBytes, 0x10) >> 16) | ((u32)CollFixed_ReadU16(spsBytes, 0x14) << 16));
	CollFixed_GteLoadVXY0(CollFixed_ReadU32(spsBytes, 0x54));
	CollFixed_GteLoadVZ0(CollFixed_ReadS32(spsBytes, 0x58));
	CollMoved_GteRTV0();

	normalW = CollFixed_ReadS16(spsBytes, 0x5a);
	planeNear = CollFixed_GteReadMAC1() + normalW * -2;
	planeFar = CollFixed_GteReadMAC2() + normalW * -2;

	if (planeFar < 0)
	{
		if (((quad->quadFlags & 0x40) == 0) && ((s32)quad->draw_order_low >= 0))
			goto KeepNormal;

		planeNear = -planeNear;
		planeFar = -planeFar;
		CollFixed_WriteS16(spsBytes, 0x54, -CollFixed_ReadS16(spsBytes, 0x54));
		CollFixed_WriteS16(spsBytes, 0x56, -CollFixed_ReadS16(spsBytes, 0x56));
		CollFixed_WriteS16(spsBytes, 0x58, -CollFixed_ReadS16(spsBytes, 0x58));
		CollFixed_WriteS16(spsBytes, 0x5a, -CollFixed_ReadS16(spsBytes, 0x5a));
	}

KeepNormal:
	quadFlags = quad->quadFlags;
	CollFixed_WriteS16(spsBytes, 0x3c, CollFixed_ReadS16(spsBytes, 0x3c) + 1);

	if ((planeNear - CollFixed_ReadS16(spsBytes, 6)) >= 0)
		return;

	if (planeFar < 0)
		return;

	if (((quadFlags & 0x40) == 0) && ((planeNear - planeFar) > 0))
		return;

	if (planeNear >= 0)
	{
		CollFixed_GteLoadIR0(planeNear);
		CollFixed_GteLoadIR(CollFixed_ReadS16(spsBytes, 0x54), CollFixed_ReadS16(spsBytes, 0x56), CollFixed_ReadS16(spsBytes, 0x58));
		projectedFromInput = 0;
	}
	else
	{
		CollFixed_GteLoadIR(CollFixed_ReadS16(spsBytes, 0) - CollFixed_ReadS16(spsBytes, 0x10),
		                    CollFixed_ReadS16(spsBytes, 2) - CollFixed_ReadS16(spsBytes, 0x12),
		                    CollFixed_ReadS16(spsBytes, 4) - CollFixed_ReadS16(spsBytes, 0x14));
		CollFixed_GteLoadIR0((CollFixed_MulLo(planeNear, -0x1000)) / (planeFar - planeNear));
		projectedFromInput = 1;
	}

	CollMoved_GteGPF12();
	hitX = CollFixed_GteReadMAC1();
	hitY = CollFixed_GteReadMAC2();
	hitZ = CollFixed_GteReadMAC3();

	CollFixed_WriteS16(spsBytes, 0x5c, CollFixed_ReadS16(spsBytes, 0) - hitX);
	CollFixed_WriteS16(spsBytes, 0x5e, CollFixed_ReadS16(spsBytes, 2) - hitY);
	CollFixed_WriteS16(spsBytes, 0x60, CollFixed_ReadS16(spsBytes, 4) - hitZ);

	*(struct BspSearchVertex **)(spsBytes + 0xd8) = v1;
	*(struct BspSearchVertex **)(spsBytes + 0xdc) = v2;
	*(struct BspSearchVertex **)(spsBytes + 0xe0) = v3;

	reorderResult = COLL_MOVED_TRIANGL_ReorderNormals(spsBytes + 0x4c, v1, v2, v3);
	if (reorderResult < 0)
		return;

	if (projectedFromInput != 0)
	{
		CollFixed_WriteS16(spsBytes, 0xe4, CollFixed_ReadS16(spsBytes, 0x5c) - CollFixed_ReadS16(spsBytes, 0x4c));
		CollFixed_WriteS16(spsBytes, 0xe6, CollFixed_ReadS16(spsBytes, 0x5e) - CollFixed_ReadS16(spsBytes, 0x4e));
		CollFixed_WriteS16(spsBytes, 0xe8, CollFixed_ReadS16(spsBytes, 0x60) - CollFixed_ReadS16(spsBytes, 0x50));
	}
	else
	{
		CollFixed_WriteS16(spsBytes, 0xe4, CollFixed_ReadS16(spsBytes, 0) - CollFixed_ReadS16(spsBytes, 0x4c));
		CollFixed_WriteS16(spsBytes, 0xe6, CollFixed_ReadS16(spsBytes, 2) - CollFixed_ReadS16(spsBytes, 0x4e));
		CollFixed_WriteS16(spsBytes, 0xe8, CollFixed_ReadS16(spsBytes, 4) - CollFixed_ReadS16(spsBytes, 0x50));
	}

	CollFixed_GteLoadR11R12(CollFixed_ReadU32(spsBytes, 0xe4));
	CollFixed_GteLoadR13R21(CollFixed_ReadS16(spsBytes, 0xe8));
	CollFixed_GteLoadVXY0(CollFixed_ReadU32(spsBytes, 0xe4));
	CollFixed_GteLoadVZ0(CollFixed_ReadS16(spsBytes, 0xe8));
	CollFixed_GteMVMVA();
	distanceSq = CollFixed_GteReadMAC1();

	if ((distanceSq - CollFixed_ReadS32(spsBytes, 8)) > 0)
		return;

	if ((quadFlags & 0x40) != 0)
	{
		if ((planeNear < 0) || (((planeNear - CollFixed_ReadS16(spsBytes, 6)) | (planeFar - CollFixed_ReadS16(spsBytes, 6))) < 0))
		{
			CollFixed_WriteU32(spsBytes, 0x1a4, CollFixed_ReadU32(spsBytes, 0x1a4) | (u8)quad->terrain_type);
			return;
		}
	}

	distance = planeFar - planeNear;
	if (distance != 0)
		distance = 0x1000 - (CollFixed_MulLo(CollFixed_ReadS16(spsBytes, 6) - planeNear, 0x1000) / distance);

	if ((distance - CollFixed_ReadS32(spsBytes, 0x84)) >= 0)
		return;

	if ((quadFlags & 0x10) != 0)
	{
		if ((quadFlags & 0x200) != 0)
			CollFixed_WriteU32(spsBytes, 0x1a4, CollFixed_ReadU32(spsBytes, 0x1a4) | 0x4000);

		return;
	}

	CollFixed_WriteS32(spsBytes, 0x84, distance);
	sps->levVertHit[0] = v1->pLevelVertex;
	sps->levVertHit[1] = v2->pLevelVertex;
	sps->levVertHit[2] = v3->pLevelVertex;

	CollFixed_WriteU32(spsBytes, 0x68, CollFixed_ReadU32(spsBytes, 0x4c));
	CollFixed_WriteU32(spsBytes, 0x6c, CollFixed_ReadU32(spsBytes, 0x50));
	CollFixed_WriteU32(spsBytes, 0x70, CollFixed_ReadU32(spsBytes, 0x54));
	CollFixed_WriteU32(spsBytes, 0x74, CollFixed_ReadU32(spsBytes, 0x58));
	CollFixed_WriteU32(spsBytes, 0x78, CollFixed_ReadU32(spsBytes, 0x5c));
	CollFixed_WriteU32(spsBytes, 0x7c, CollFixed_ReadU32(spsBytes, 0x60));
	*(struct QuadBlock **)(spsBytes + 0x80) = quad;

	*(u8 *)(spsBytes + 0x7f) = *(u8 *)(spsBytes + 0x63);
	*(s8 *)(spsBytes + 0x7e) = (s8)reorderResult;

	if (distance <= 0)
	{
		CollFixed_WriteU32(spsBytes, 0x1c, CollFixed_ReadU32(spsBytes, 0x10));
		CollFixed_WriteS16(spsBytes, 0x20, CollFixed_ReadS16(spsBytes, 0x14));
	}
	else
	{
		CollFixed_GteLoadIR0(distance);
		CollFixed_GteLoadIR(CollFixed_ReadS16(spsBytes, 0) - CollFixed_ReadS16(spsBytes, 0x10),
		                    CollFixed_ReadS16(spsBytes, 2) - CollFixed_ReadS16(spsBytes, 0x12),
		                    CollFixed_ReadS16(spsBytes, 4) - CollFixed_ReadS16(spsBytes, 0x14));
		CollMoved_GteGPF12();

		CollFixed_WriteS16(spsBytes, 0x1c, CollFixed_ReadS16(spsBytes, 0x10) + CollFixed_GteReadMAC1());
		CollFixed_WriteS16(spsBytes, 0x1e, CollFixed_ReadS16(spsBytes, 0x12) + CollFixed_GteReadMAC2());
		CollFixed_WriteS16(spsBytes, 0x20, CollFixed_ReadS16(spsBytes, 0x14) + CollFixed_GteReadMAC3());
	}

	CollFixed_WriteS16(spsBytes, 0x3e, CollFixed_ReadS16(spsBytes, 0x3e) + 1);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80020064-0x800202a8
void COLL_MOVED_QUADBLK_TestTriangles(struct QuadBlock *quad, struct ScratchpadStruct *sps)
{
	struct BspSearchVertex *bsv = &sps->bspSearchVert[0];

	sps->Set1.ptrQuadblock = quad;

	if (((sps->Union.QuadBlockColl.qbFlagsWanted & quad->quadFlags) == 0) || ((sps->Union.QuadBlockColl.qbFlagsIgnored & quad->quadFlags) != 0) ||
	    (quad->bbox.min[0] > sps->bbox.max[0]) || (quad->bbox.min[1] > sps->bbox.max[1]) || (quad->bbox.min[2] > sps->bbox.max[2]) ||
	    (sps->bbox.min[0] > quad->bbox.max[0]) || (sps->bbox.min[1] > quad->bbox.max[1]) || (sps->bbox.min[2] > quad->bbox.max[2]))
	{
		return;
	}

	// if 3P or 4P mode,
	// then use low-LOD quadblock collision (two triangles)
	if ((sps->Union.QuadBlockColl.searchFlags & 2) == 0)
	{
		COLL_FIXED_QUADBLK_GetNormVecs_LoLOD(sps, quad);

		sps->Set1.unk63_someIndex = 0;
		COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[0], &bsv[1], &bsv[2]); // 0, 1, 2

		sps->Set1.unk63_someIndex = 1;

		// If this is a quad instead of a triangle
		if (quad->index[2] != quad->index[3])
		{
			COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[1], &bsv[3], &bsv[2]); // 1, 3, 2
		}
	}
	else
	{
		if ((sps->Union.QuadBlockColl.searchFlags & 8) == 0)
		{
			COLL_FIXED_QUADBLK_GetNormVecs_HiLOD(sps, quad);
		}

		sps->Set1.unk63_someIndex = 2;
		COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[0], &bsv[4], &bsv[5]); // 0, 4, 5
		sps->Set1.unk63_someIndex = 3;
		COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[4], &bsv[6], &bsv[5]); // 4, 6, 5
		sps->Set1.unk63_someIndex = 4;
		COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[6], &bsv[4], &bsv[1]); // 6, 4, 1
		sps->Set1.unk63_someIndex = 5;
		COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[5], &bsv[6], &bsv[2]); // 5, 6, 2

		sps->Set1.unk63_someIndex = 6;

		// If this is a quad instead of a triangle
		if (quad->index[2] != quad->index[3])
		{
			COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[8], &bsv[6], &bsv[7]); // 8, 6, 7
			sps->Set1.unk63_someIndex = 7;
			COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[7], &bsv[3], &bsv[8]); // 7, 3, 8
			sps->Set1.unk63_someIndex = 8;
			COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[1], &bsv[7], &bsv[6]); // 1, 7, 6
			sps->Set1.unk63_someIndex = 9;
			COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[2], &bsv[6], &bsv[8]); // 2, 6, 8
		}
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800202a8-0x80020334
void COLL_MOVED_BSPLEAF_TestQuadblocks(struct BSP *node, struct ScratchpadStruct *sps)
{
	int numQuads;
	struct QuadBlock *ptrQuad;

	// if bsp flag is water
	if ((node->flag & 2) != 0)
	{
		*(int *)&sps->dataOutput[0] |= 0x8000;
	}

	numQuads = node->data.leaf.numQuads;
	ptrQuad = node->data.leaf.ptrQuadBlockArray;

	// loop through all quadblocks
	do
	{
		COLL_MOVED_QUADBLK_TestTriangles(ptrQuad++, sps);
		numQuads--;
	} while (numQuads > 0);

	if ((sps->Union.QuadBlockColl.searchFlags & 1) != 0)
	{
		COLL_FIXED_BSPLEAF_TestInstance(node, sps);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80020334-0x80020410; scrub depth is SPS+0x0e.
void COLL_MOVED_FindScrub(struct QuadBlock *qb, int triangleID, struct ScratchpadStruct *sps)
{
	struct ScratchpadStructExtended *ext = (struct ScratchpadStructExtended *)sps;
	u16 searchFlags = sps->Union.QuadBlockColl.searchFlags;

	if (qb == NULL)
	{
		sps->Union.QuadBlockColl.searchFlags = searchFlags & 0xffdf;
		CollFixed_WriteS16(sps, 0xe, 0);
		ext->numTriangles = 0;
		return;
	}

	for (s32 i = ext->numTriangles - 1; i >= 0; i--)
	{
		struct BspSearchTriangle *tri = &ext->bspSearchTriangle[i];

		if ((tri->quadblock == qb) && (tri->triangleID == triangleID))
		{
			s32 numCollision = tri->numCollision;
			s16 scrub = numCollision;

			if (numCollision < 0x401)
			{
				numCollision += 0x100;
				scrub = numCollision;
				tri->numCollision = numCollision;
			}

			sps->Union.QuadBlockColl.searchFlags = searchFlags | 0x20;
			CollFixed_WriteS16(sps, 0xe, scrub);
			return;
		}
	}

	{
		struct BspSearchTriangle *tri = &ext->bspSearchTriangle[ext->numTriangles];

		tri->quadblock = qb;
		tri->triangleID = triangleID;
		tri->numCollision = 0;
	}

	sps->Union.QuadBlockColl.searchFlags = searchFlags & 0xffdf;
	CollFixed_WriteS16(sps, 0xe, 0);
	ext->numTriangles++;
}


static s32 CollMoved_PlayerSearch_StepVelocity(s32 velocity, s32 elapsedTimeMS, s32 multiplier)
{
	return CollFixed_MulLo(CollFixed_MulLo(velocity, elapsedTimeMS) >> 5, multiplier) >> 12;
}

static s16 CollMoved_PlayerSearch_Min(s16 a, s16 b)
{
	return (b < a) ? b : a;
}

static s16 CollMoved_PlayerSearch_Max(s16 a, s16 b)
{
	return (a < b) ? b : a;
}

static void CollMoved_PlayerSearch_SetBBoxAxis(struct ScratchpadStruct *sps, int axis, s16 current, s16 next)
{
	s16 radius = sps->Input1.hitRadius;

	sps->bbox.min[axis] = CollMoved_PlayerSearch_Min(current - radius, next - radius);
	sps->bbox.max[axis] = CollMoved_PlayerSearch_Max(current + radius, next + radius);
}

static int CollMoved_PlayerSearch_RunHitboxLInC(struct ScratchpadStruct *sps, struct Thread *t)
{
	struct BSP *bsp = sps->bspHitbox;
	struct InstDef *instDef;
	struct Instance *inst;
	s16 modelID;
	struct MetaDataMODEL *meta;

	if ((bsp->flag & 0x80) != 0)
	{
		instDef = bsp->data.hitbox.instDef;
		if (instDef == NULL)
			return 1;

		inst = instDef->ptrInstance;
		if (inst == NULL)
			return 1;

		if ((inst->flags & 0xf) == 0)
			return 1;

		modelID = instDef->modelID;
	}
	else
	{
		if ((bsp->flag & 0x10) == 0)
			return 1;

		instDef = bsp->data.hitbox.instDef;
		if (instDef == NULL)
			return 1;

		// Retail passes the InstDef pointer for 0x10 hitboxes, not ptrInstance.
		inst = (struct Instance *)instDef;
		modelID = instDef->model->id;
	}

	meta = COLL_LevModelMeta(modelID);
	if ((meta != NULL) && (meta->LInC != NULL))
	{
		return meta->LInC(inst, t, sps);
	}

	return 1;
}

static void CollMoved_PlayerSearch_StoreHitbox(struct ScratchpadStruct *sps)
{
	sps->bspInstHitboxArr[sps->numInstHitboxesHit] = sps->bspHitbox;
	sps->numInstHitboxesHit++;
}

static void CollMoved_PlayerSearch_CopyGroundNormal(struct ScratchpadStruct *sps, struct Driver *d)
{
	d->normalVecUP.x = sps->Set2.normalVec[0];
	d->normalVecUP.y = sps->Set2.normalVec[1];
	d->normalVecUP.z = sps->Set2.normalVec[2];

	d->AxisAngle1_normalVec.x = sps->Set2.normalVec[0];
	d->AxisAngle1_normalVec.y = sps->Set2.normalVec[1];
	d->AxisAngle1_normalVec.z = sps->Set2.normalVec[2];
}

static void CollMoved_PlayerSearch_CopyHitOutput(struct ScratchpadStruct *sps, struct Driver *d)
{
	d->spsHitPos[0] = sps->Set2.hitPos[0];
	d->spsHitPos[1] = sps->Set2.hitPos[1];
	d->spsHitPos[2] = sps->Set2.hitPos[2];

	d->spsNormalVec[0] = sps->Set2.normalVec[0];
	d->spsNormalVec[1] = sps->Set2.normalVec[1];
	d->spsNormalVec[2] = sps->Set2.normalVec[2];
}

static void CollMoved_PlayerSearch_SetModelID(struct ScratchpadStruct *sps, s32 modelID)
{
	CollFixed_WriteS16(sps, 0xc, modelID);
}

static void CollMoved_PlayerSearch_BoostHitboxScrub(struct ScratchpadStruct *sps)
{
	// Retail bumps scratchpad offset 0x0e, the upper halfword of the typed modelID slot.
	CollFixed_WriteS16(sps, 0xe, CollFixed_ReadS16(sps, 0xe) + 0x200);
}

static u8 CollMoved_PlayerSearch_HitboxId(struct BSP *bsp)
{
	// NOTE(aalhendi): Retail reads the hitbox kind with lbu +0x1, not BSP.id.
	return ((u8 *)bsp)[1];
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80020410-0x80020c58
void COLL_MOVED_PlayerSearch(struct Thread *t, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
	s32 multiplier = 0x1000;
	s16 hitRadius = 0x19;

	sps->Input1.hitRadius = hitRadius;
	sps->Input1.hitRadiusSquared = hitRadius * hitRadius;
	sps->Union.QuadBlockColl.hitRadius = hitRadius;
	sps->Union.QuadBlockColl.hitRadiusSquared = hitRadius * hitRadius;
	sps->Union.QuadBlockColl.qbFlagsWanted = 0x3000;
	sps->Union.QuadBlockColl.qbFlagsIgnored = 0;
	sps->Union.QuadBlockColl.searchFlags = 1;
	sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;

	if (gGT->numPlyrCurrGame < 3)
	{
		sps->Union.QuadBlockColl.searchFlags = 3;
	}

	sps->numInstHitboxesHit = 0;
	CollMoved_PlayerSearch_SetModelID(sps, DYNAMIC_PLAYER);
	*(u32 *)&sps->dataOutput[0] = 0;

	COLL_MOVED_FindScrub(NULL, 0, sps);

	for (s32 iterations = 15; iterations != 0; iterations--)
	{
		s32 velocity[3];
		s16 current[3];
		s16 next[3];

		velocity[0] = CollMoved_PlayerSearch_StepVelocity(d->velocity.x, gGT->elapsedTimeMS, multiplier);
		velocity[1] = CollMoved_PlayerSearch_StepVelocity(d->velocity.y, gGT->elapsedTimeMS, multiplier);
		velocity[2] = CollMoved_PlayerSearch_StepVelocity(d->velocity.z, gGT->elapsedTimeMS, multiplier);

		sps->boolDidTouchQuadblock = 0;
		sps->unk3C = 0;
		sps->boolDidTouchHitbox = 0;
		sps->unk40 = 0;
		sps->Union.QuadBlockColl.searchFlags |= 1;
		sps->countByOne_ForWhatReason = 0x1000;

		current[0] = (s16)d->originToCenter.x + (d->posCurr.x >> 8);
		current[1] = (s16)d->originToCenter.y + (d->posCurr.y >> 8);
		current[2] = (s16)d->originToCenter.z + (d->posCurr.z >> 8);

		next[0] = (s16)d->originToCenter.x + ((d->posCurr.x + velocity[0]) >> 8);
		next[1] = (s16)d->originToCenter.y + ((d->posCurr.y + velocity[1]) >> 8);
		next[2] = (s16)d->originToCenter.z + ((d->posCurr.z + velocity[2]) >> 8);

		sps->Union.QuadBlockColl.pos[0] = current[0];
		sps->Union.QuadBlockColl.pos[1] = current[1];
		sps->Union.QuadBlockColl.pos[2] = current[2];
		sps->Input1.pos[0] = next[0];
		sps->Input1.pos[1] = next[1];
		sps->Input1.pos[2] = next[2];

		if ((sps->Input1.pos[0] == sps->Union.QuadBlockColl.pos[0]) && (sps->Input1.pos[1] == sps->Union.QuadBlockColl.pos[1]) &&
		    (sps->Input1.pos[2] == sps->Union.QuadBlockColl.pos[2]))
		{
			break;
		}

		CollMoved_PlayerSearch_SetBBoxAxis(sps, 0, current[0], next[0]);
		CollMoved_PlayerSearch_SetBBoxAxis(sps, 1, current[1], next[1]);
		CollMoved_PlayerSearch_SetBBoxAxis(sps, 2, current[2], next[2]);

		sps->Union.QuadBlockColl.hitPos[0] = sps->Input1.pos[0];
		sps->Union.QuadBlockColl.hitPos[1] = sps->Input1.pos[1];
		sps->Union.QuadBlockColl.hitPos[2] = sps->Input1.pos[2];

		sps->Union.QuadBlockColl.searchFlags = (sps->Union.QuadBlockColl.searchFlags | 1) & 0xfff7;

		if ((gGT->level1 != NULL) && (gGT->level1->ptr_mesh_info != NULL) && (gGT->level1->ptr_mesh_info->bspRoot != NULL))
		{
			COLL_SearchBSP_CallbackPARAM(gGT->level1->ptr_mesh_info->bspRoot, &sps->bbox, COLL_MOVED_BSPLEAF_TestQuadblocks, sps);
		}

		if (sps->boolDidTouchQuadblock != 0)
		{
			d->unkAA |= 4;
		}

		if (sps->countByOne_ForWhatReason > 0)
		{
			d->posCurr.x += CollFixed_MulLo(velocity[0], sps->countByOne_ForWhatReason) >> 12;
			d->posCurr.y += CollFixed_MulLo(velocity[1], sps->countByOne_ForWhatReason) >> 12;
			d->posCurr.z += CollFixed_MulLo(velocity[2], sps->countByOne_ForWhatReason) >> 12;
		}

		if (sps->boolDidTouchHitbox != 0)
		{
			struct BSP *bspHitbox = sps->bspHitbox;
			u8 hitboxId;
			int hitboxResult;

			sps->Union.QuadBlockColl.searchFlags &= 0xfff7;
			d->unkAA &= 0xfffd;

			hitboxResult = CollMoved_PlayerSearch_RunHitboxLInC(sps, t);
			hitboxId = CollMoved_PlayerSearch_HitboxId(bspHitbox);

			if ((hitboxResult == 2) || (hitboxId == 4))
			{
				CollMoved_PlayerSearch_StoreHitbox(sps);
			}
			else
			{
				struct Scrub *scrub;

				COLL_MOVED_FindScrub((struct QuadBlock *)bspHitbox, 0, sps);
				CollMoved_PlayerSearch_BoostHitboxScrub(sps);

				scrub = VehAfterColl_GetSurface(hitboxId);
				hitboxResult = COLL_MOVED_ScrubImpact(d, t, sps, scrub, &d->velocity.x);

				if (hitboxResult == 0)
				{
					CollMoved_PlayerSearch_StoreHitbox(sps);
				}

				if (hitboxResult == 2)
				{
					return;
				}
			}
		}
		else
		{
			struct QuadBlock *quad;
			struct Scrub *scrub;
			u32 scrubId;
			u32 impactResult;

			if (sps->boolDidTouchQuadblock == 0)
			{
				break;
			}

			quad = sps->Set2.ptrQuadblock;

			if ((quad->quadFlags & 0x200) != 0)
			{
				d->unkAA |= 1;
			}

			COLL_MOVED_FindScrub(quad, ((u8 *)sps)[0x7f], sps);

			if ((quad->quadFlags & 0x1000) == 0)
			{
				scrubId = ((quad->quadFlags & 1) == 0) ? 0 : 6;
			}
			else
			{
				if ((quad != d->underDriver) && ((quad->quadFlags & 8) != 0))
				{
					d->underDriver = NULL;
				}

				d->currBlockTouching = quad;
				CollMoved_PlayerSearch_CopyGroundNormal(sps, d);
				d->unkAA |= 8;
				scrubId = 5;
			}

			scrub = VehAfterColl_GetSurface(scrubId);
			d->unkAA |= 2;
			CollMoved_PlayerSearch_CopyHitOutput(sps, d);

			impactResult = COLL_MOVED_ScrubImpact(d, t, sps, scrub, &d->velocity.x);
			if (impactResult == 2)
			{
				return;
			}

			if (sps->countByOne_ForWhatReason > 0)
			{
				multiplier -= CollFixed_MulLo(multiplier, sps->countByOne_ForWhatReason) >> 12;
				if (multiplier < 100)
				{
					break;
				}
			}

			sps->Union.QuadBlockColl.searchFlags |= 8;
		}
	}

	d->stepFlagSet = *(u32 *)&sps->dataOutput[0];
}


static s32 CollMoved_ScrubImpact_Abs(s32 value)
{
	return (value < 0) ? (s32)(0u - (u32)value) : value;
}

static u32 CollMoved_ScrubImpact_PackXY(s32 x, s32 y)
{
	return ((u32)(u16)x) | ((u32)y << 16);
}

static s32 CollMoved_ScrubImpact_TrigX(s32 angle)
{
	struct TrigTable trig = data.trigApprox[angle & 0x3ff];
	s32 value = ((angle & 0x400) == 0) ? trig.sin : trig.cos;

	if ((angle & 0x800) != 0)
	{
		value = -value;
	}

	return value;
}

static void CollMoved_ScrubImpact_GteLLV0(s32 x, s32 y, s32 z, s32 *outX, s32 *outY, s32 *outZ)
{
	MTC2(CollMoved_ScrubImpact_PackXY(x, y), 0);
	MTC2(z, 1);
	doCOP2(0x04a6012);
	*outX = MFC2_S(25);
	*outY = MFC2_S(26);
	*outZ = MFC2_S(27);
}

static void CollMoved_ScrubImpact_GteOP12(void)
{
	doCOP2(0x0178000c);
}

static void CollMoved_ScrubImpact_ProjectWallVelocity(s16 normalX, s16 normalY, s16 normalZ, s32 oldVelX, s32 oldVelZ, s32 *outX, s32 *outY, s32 *outZ)
{
	u32 r11;
	u32 r22;
	u32 r33;
	s32 crossX;
	s32 crossY;
	s32 crossZ;

	MTC2(oldVelX, 9);
	MTC2(0, 10);
	MTC2(oldVelZ, 11);
	CTC2(normalX, 0);
	CTC2(normalY, 2);
	CTC2(normalZ, 4);

	CollMoved_ScrubImpact_GteOP12();

	r11 = CFC2(0);
	r22 = CFC2(2);
	r33 = CFC2(4);

	MTC2(r11, 9);
	MTC2(r22, 10);
	MTC2(r33, 11);

	crossX = MFC2_S(25);
	crossY = MFC2_S(26);
	crossZ = MFC2_S(27);

	CTC2(crossX, 0);
	CTC2(crossY, 2);
	CTC2(crossZ, 4);

	CollMoved_ScrubImpact_GteOP12();

	*outX = MFC2_S(25);
	*outY = MFC2_S(26);
	*outZ = MFC2_S(27);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80020c58-0x80021500; scrub depth is SPS+0x0e.
u32 COLL_MOVED_ScrubImpact(struct Driver *d, struct Thread *t, struct ScratchpadStruct *sps, struct Scrub *scrub, int *velocity)
{
	s16 normalX = sps->Set2.normalVec[0];
	s16 normalY = sps->Set2.normalVec[1];
	s16 normalZ = sps->Set2.normalVec[2];
	s32 dot;
	u32 ret;

	if ((d->unknownTraction != 0) && (sps->boolDidTouchQuadblock != 0) && ((sps->Set2.ptrQuadblock->quadFlags & 0x1000) != 0) && (((u8 *)sps)[0x7e] != 6) &&
	    (sps->Set2.ptrQuadblock != d->underDriver))
	{
		if ((CollMoved_ScrubImpact_Abs(d->speedApprox) < 0x300) && (CollMoved_ScrubImpact_Abs(d->jumpHeightCurr) < 0x300) && (d->fireSpeed == 0))
		{
			s32 diffX = (d->posCurr.x >> 8) - sps->Set2.hitPos[0];
			s32 diffZ = (d->posCurr.z >> 8) - sps->Set2.hitPos[2];
			s32 diffY = (d->posCurr.y >> 8) - sps->Set2.hitPos[1];

			if ((diffX | diffY | diffZ) != 0)
			{
				s32 len = VehCalc_FastSqrt((u32)CollFixed_MulLo(diffX, diffX) + (u32)CollFixed_MulLo(diffY, diffY) + (u32)CollFixed_MulLo(diffZ, diffZ), 0);

				normalX = CollFixed_Sll32(diffX, 12) / len;
				normalY = CollFixed_Sll32(diffY, 12) / len;
				normalZ = CollFixed_Sll32(diffZ, 12) / len;
			}
		}
	}

	dot = ((velocity[0] >> 3) * normalX + (velocity[1] >> 3) * normalY + (velocity[2] >> 3) * normalZ) >> 9;

	if (dot < -0xa00)
	{
		d->actionsFlagSet |= 0x80;
	}

	dot -= CollFixed_ReadS16(sps, 0xe);

	ret = 0;
	if (dot < 0)
	{
		u32 scrubFlags = scrub->flags;
		s32 scrubSpeed;

		if ((scrubFlags & 4) == 0)
		{
			d->actionsFlagSet |= 0x2000;
		}

		if ((scrubFlags & 8) == 0)
		{
			d->reserves = 0;
			d->turbo_outsideTimer = 0;
		}

		scrubSpeed = scrub->unk_0x8;

		if (!((d->set_0xF0_OnWallRub == 0) ? (0x3e7ff < scrubSpeed) : (d->scrubMeta8 < scrubSpeed)))
		{
			d->set_0xF0_OnWallRub = 0xf0;
			d->scrubMeta8 = scrubSpeed;
			d->posWallColl[0] = sps->Set2.hitPos[0];
			d->posWallColl[1] = sps->Set2.hitPos[1];
			d->posWallColl[2] = sps->Set2.hitPos[2];
		}

		ret = 0;

		if ((scrubFlags & 1) != 0)
		{
			s32 impactX = CollFixed_MulLo(dot, normalX) >> 12;
			s32 impactY = CollFixed_MulLo(dot, normalY) >> 12;
			s32 impactZ = CollFixed_MulLo(dot, normalZ) >> 12;
			s32 speedSq = 0;
			s32 oldVelX;
			s32 oldVelZ;
			s32 wallVelX;
			s32 wallVelY;
			s32 wallVelZ;

			if (scrub->unk_angle != 0)
			{
				speedSq = ((u32)CollFixed_MulLo(velocity[0], velocity[0]) + (u32)CollFixed_MulLo(velocity[1], velocity[1]) +
				           (u32)CollFixed_MulLo(velocity[2], velocity[2])) >>
				          15;
			}

			oldVelX = velocity[0];
			oldVelZ = velocity[2];
			velocity[0] = oldVelX - impactX;
			velocity[2] = oldVelZ - impactZ;
			velocity[1] -= impactY;

			CollMoved_ScrubImpact_GteLLV0(impactX, impactY, impactZ, &impactX, &impactY, &impactZ);

			if ((sps->boolDidTouchQuadblock != 0) && ((sps->Union.QuadBlockColl.searchFlags & 0x10) == 0) && ((d->actionsFlagSetPrevFrame & 1) == 0) &&
			    ((sps->Set2.ptrQuadblock->quadFlags & 0x1000) != 0))
			{
				CollMoved_ScrubImpact_ProjectWallVelocity(normalX, normalY, normalZ, oldVelX, oldVelZ, &wallVelX, &wallVelY, &wallVelZ);

				{
					u32 wallSpeed = VehCalc_FastSqrt((u32)CollFixed_MulLo(wallVelX, wallVelX) + (u32)CollFixed_MulLo(wallVelY, wallVelY) +
					                                     (u32)CollFixed_MulLo(wallVelZ, wallVelZ),
					                                 0x10) >>
					                8;
					s32 speedApprox = d->speedApprox;

					if ((wallSpeed != 0) && (speedApprox > 0))
					{
						sps->Union.QuadBlockColl.searchFlags |= 0x10;
						velocity[0] = CollFixed_MulLo(wallVelX, speedApprox) / (s32)wallSpeed;
						velocity[1] = CollFixed_MulLo(wallVelY, speedApprox) / (s32)wallSpeed;
						velocity[2] = CollFixed_MulLo(wallVelZ, speedApprox) / (s32)wallSpeed;
						velocity[0] -= normalX >> 1;
						velocity[1] -= normalY >> 1;
						velocity[2] -= normalZ >> 1;
					}
				}
			}

			if (((scrubFlags & 2) != 0) && (dot < -0x13ff) && ((u32)CollFixed_MulLo(impactY, impactY) + (u32)CollFixed_MulLo(impactZ, impactZ) > 0x1900000))
			{
				if (d->kartState != KS_MASK_GRABBED)
				{
					GAMEPAD_JogCon1(d, (d->simpTurnState < 1) ? 0x1f : 0x2f, 0x60);
				}

				if (scrub->unk_angle != 0)
				{
					s32 trig = CollMoved_ScrubImpact_TrigX(scrub->unk_angle);

					if ((CollFixed_MulLo((CollFixed_MulLo(speedSq, trig) >> 12), trig) >> 12) >= (CollFixed_MulLo(dot, dot) >> 15))
						return 1;
				}

				if ((d->kartState != KS_MASK_GRABBED) && ((u32)CollFixed_MulLo(impactY, impactY) + (u32)CollFixed_MulLo(impactZ, impactZ) > 0x1900000))
				{
					u32 soundFlags = 0xff8080;

					if ((d->actionsFlagSet & 0x10000) != 0)
					{
						soundFlags = 0x1ff8080;
					}

					OtherFX_Play_LowLevel(6, 1, soundFlags);
					Voiceline_RequestPlay(6, data.characterIDs[d->driverID], 0x10);
					GAMEPAD_ShockFreq(d, 8, 0);
					GAMEPAD_ShockForce1(d, 8, 0x7f);

					if (d->kartState == KS_DRIFTING)
					{
						s16 turnAngle = d->turnAngleCurr;

						d->turnAngleCurr = 0;
						d->angle += turnAngle;
						d->rotCurr.w -= turnAngle;
					}

					d->instSelf->animIndex = 2;
					d->instSelf->animFrame = 0;
					d->matrixArray = 4;
					d->matrixIndex = 0;

					VehPhysProc_SlamWall_Init(t, d);
					return 2;
				}
			}

			ret = 1;
		}
	}

	return ret;
}
