#include <common.h>
#include <ctr_scratchpad.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d094-0x8001d0c4
struct MetaDataMODEL *COLL_LevModelMeta(u32 id)
{
	// use unsigned so -1 is positive
	if (id >= NUM_MDM)
	{
		id = 0;
	}

	return &data.MetaDataModels[id];
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001eb0c-0x8001ebec
void COLL_SearchBSP_CallbackQUADBLK(const SVec3 *top, const SVec3 *bottom, struct ScratchpadStruct *sps, s32 hitRadius)
{
	s32 hitRadiusSquared = CTR_MipsMulLo(hitRadius, hitRadius);

	sps->Input1.hitRadius = hitRadius;
	sps->Union.QuadBlockColl.hitRadius = hitRadius;
	sps->boolDidTouchHitbox = 0;
	sps->numTrianglesTested = 0;
	sps->boolDidTouchQuadblock = 0;
	sps->collision.stepFlags = 0;
	struct mesh_info *meshInfo = sps->ptr_mesh_info;
	sps->numBspHitboxesHit = 0;

	sps->Input1.pos = *top;
	sps->Union.QuadBlockColl.hitPos = *top;
	sps->Union.QuadBlockColl.pos = *bottom;

	sps->hitFraction = COLL_FRACTION_ONE;
	sps->Input1.hitRadiusSquared = hitRadiusSquared;
	sps->Union.QuadBlockColl.hitRadiusSquared = hitRadiusSquared;

	s16 topX = sps->Input1.pos.x;
	s16 topY = sps->Input1.pos.y;
	s16 topZ = sps->Input1.pos.z;
	s16 bottomX = sps->Union.QuadBlockColl.pos.x;
	s16 bottomY = sps->Union.QuadBlockColl.pos.y;
	s16 bottomZ = sps->Union.QuadBlockColl.pos.z;

	s16 min = bottomX;
	s16 max = topX;
	if ((topX - bottomX) < 0)
	{
		min = topX;
		max = bottomX;
	}
	sps->bbox.min.x = min;
	sps->bbox.max.x = max;

	min = bottomY;
	max = topY;
	if ((topY - bottomY) < 0)
	{
		min = topY;
		max = bottomY;
	}
	sps->bbox.min.y = min;
	sps->bbox.max.y = max;

	min = bottomZ;
	max = topZ;
	if ((topZ - bottomZ) < 0)
	{
		min = topZ;
		max = bottomZ;
	}
	sps->bbox.min.z = min;
	sps->bbox.max.z = max;

	COLL_SearchBSP_CallbackPARAM(meshInfo->bspRoot, &sps->bbox, COLL_FIXED_BSPLEAF_TestQuadblocks, sps);
}


internal b32 COLL_SearchBSP_CallbackPARAM_Overlaps(struct BSP *node, const struct BoundingBox *bounds)
{
	return ((node->box.min.y <= bounds->max.y) && (node->box.min.x <= bounds->max.x) && (bounds->min.x <= node->box.max.x) &&
	        (node->box.min.z <= bounds->max.z) && (bounds->min.z <= node->box.max.z) && (bounds->min.y <= node->box.max.y));
}

internal void COLL_SearchBSP_CallbackPARAM_PushChild(struct BSP *root, BspChildId childID, const struct BoundingBox *bounds, BspChildId **stackTop)
{
	u16 rawChildID = (u16)childID;
	if (rawChildID == BSP_CHILD_ID_NONE)
	{
		return;
	}

	struct BSP *child = &root[rawChildID & BSP_CHILD_ID_INDEX_MASK];
	if (!COLL_SearchBSP_CallbackPARAM_Overlaps(child, bounds))
	{
		return;
	}

	**stackTop = childID;
	(*stackTop)++;
}

internal void COLL_SearchBSP_CallbackPARAM_PushChildren(struct BSP *root, struct BSP *node, const struct BoundingBox *bounds, BspChildId **stackTop)
{
	// Retail pushes child 0 then child 1; the scratchpad stack pops child 1 first.
	COLL_SearchBSP_CallbackPARAM_PushChild(root, node->data.branch.childID[0], bounds, stackTop);
	COLL_SearchBSP_CallbackPARAM_PushChild(root, node->data.branch.childID[1], bounds, stackTop);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001ebec-0x8001ede4
void COLL_SearchBSP_CallbackPARAM(struct BSP *root, struct BoundingBox *bbox, CollBspLeafCallback callback, struct ScratchpadStruct *sps)
{
	if (root == NULL)
	{
		return;
	}

	struct BoundingBox bounds = *bbox;

	// Retail stores pending child IDs at scratchpad 0x1f800070 and pops them
	// LIFO, preserving the original BSP traversal order without host recursion.
	BspChildId *stackBase = CTR_SCRATCHPAD_PTR(BspChildId, 0x70);
	BspChildId *stackTop = stackBase;

	COLL_SearchBSP_CallbackPARAM_PushChildren(root, root, &bounds, &stackTop);

	while (stackTop != stackBase)
	{
		stackTop--;
		BspChildId childID = *stackTop;
		u16 rawChildID = (u16)childID;
		struct BSP *child = &root[rawChildID & BSP_CHILD_ID_INDEX_MASK];

		if ((rawChildID & BSP_CHILD_ID_LEAF_FLAG) != 0)
		{
			callback(child, sps);
			continue;
		}

		COLL_SearchBSP_CallbackPARAM_PushChildren(root, child, &bounds, &stackTop);
	}
}


internal s32 Coll_MipsAbsS32(s32 value)
{
	return (value < 0) ? CTR_MipsNegLo(value) : value;
}

internal u8 Coll_BspHitboxClass(const struct BSP *bsp)
{
	// NOTE(aalhendi): Retail reads the hitbox kind with lbu +0x1, not BSP.id.
	return (u8)(bsp->flag >> 8);
}

struct CollTriangleProjection
{
	s32 firstA;
	s32 firstB;
	s32 firstHit;
	s32 secondA;
	s32 secondB;
	s32 secondHit;
};

internal void CollFixed_GteLoadR11R12(u32 value)
{
	CTC2(value, 0);
}

internal void CollFixed_GteLoadR13R21(u32 value)
{
	CTC2(value, 1);
}

internal void CollFixed_GteLoadR22R23(u32 value)
{
	CTC2(value, 2);
}

internal void CollFixed_GteLoadR33(u32 value)
{
	CTC2(value, 4);
}

internal void CollFixed_GteLoadVXY0(u32 value)
{
	MTC2(value, 0);
}

internal void CollFixed_GteLoadVZ0(s32 value)
{
	MTC2_S(value, 1);
}

internal void CollFixed_GteLoadIR0(s32 value)
{
	MTC2_S(value, 8);
}

internal void CollFixed_GteLoadIR(s32 x, s32 y, s32 z)
{
	MTC2_S(x, 9);
	MTC2_S(y, 10);
	MTC2_S(z, 11);
}

internal void CollFixed_GteLoadMAC(s32 x, s32 y, s32 z)
{
	MTC2_S(x, 25);
	MTC2_S(y, 26);
	MTC2_S(z, 27);
}

internal s32 CollFixed_GteReadMAC1(void)
{
	return MFC2_S(25);
}

internal s32 CollFixed_GteReadMAC2(void)
{
	return MFC2_S(26);
}

internal s32 CollFixed_GteReadMAC3(void)
{
	return MFC2_S(27);
}

internal void CollFixed_GteLoadLZCS(s32 value)
{
	MTC2_S(value, 30);
}

internal s32 CollFixed_GteReadLZCR(void)
{
	return MFC2_S(31);
}

internal void CollFixed_GteMVMVA(void)
{
	doCOP2(0x0406012);
}

internal void CollFixed_GteGPL12(void)
{
	doCOP2(0x01a8003e);
}

internal void CollFixed_GteOP0(void)
{
	doCOP2(0x0170000c);
}

internal void CollFixed_GteRTIR(void)
{
	doCOP2(0x049e012);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001ede4-0x8001ef1c
void COLL_FIXED_TRIANGL_Barycentrics(SVec3 *out, const SVec3 *v1, const SVec3 *v2, const SVec3 *point)
{
	s32 v1x = v1->x;
	s32 v1y = v1->y;
	s32 v1z = v1->z;
	s32 edgeX = v2->x - v1x;
	s32 edgeY = v2->y - v1y;
	s32 edgeZ = v2->z - v1z;
	s32 pointX = point->x - v1x;
	s32 pointY = point->y - v1y;
	s32 pointZ = point->z - v1z;

	CollFixed_GteLoadR11R12(CTR_PackS16Pair(edgeX, edgeY));
	CollFixed_GteLoadR13R21(CTR_PackS16Pair(edgeZ, pointX));
	CollFixed_GteLoadR22R23(CTR_PackS16Pair(pointY, pointZ));
	CollFixed_GteLoadVXY0(CTR_PackS16Pair(edgeX, edgeY));
	CollFixed_GteLoadVZ0(edgeZ);

	CollFixed_GteMVMVA();
	s32 pointDot = CollFixed_GteReadMAC2();
	s32 edgeDot = CollFixed_GteReadMAC1();

	CollFixed_GteLoadLZCS(pointDot);
	s32 shift = CollFixed_GteReadLZCR() - 2;

	if (shift < 0)
	{
		shift = 0;
	}
	else if (shift > 12)
	{
		shift = 12;
	}

	pointDot = CTR_MipsSll(pointDot, shift);

	if (shift < 12)
	{
		edgeDot = CTR_MipsSra(edgeDot, 12 - shift);
	}

	s32 factor = 0;

	if (edgeDot != 0)
	{
		factor = CTR_MipsDiv(pointDot, edgeDot);

		if (factor < 0)
		{
			factor = 0;
		}
		else if (factor > COLL_FRACTION_ONE)
		{
			factor = COLL_FRACTION_ONE;
		}
	}

	CollFixed_GteLoadIR0(factor);
	CollFixed_GteLoadMAC(v1x, v1y, v1z);
	CollFixed_GteLoadIR(edgeX, edgeY, edgeZ);
	CollFixed_GteGPL12();

	out->x = (s16)CollFixed_GteReadMAC1();
	out->y = (s16)CollFixed_GteReadMAC2();
	out->z = (s16)CollFixed_GteReadMAC3();
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d0c4-0x8001d610
u32 COLL_FIXED_INSTANC_TestPoint(struct ScratchpadStruct *sps, struct BSP *node)
{
	struct CollInstanceHitboxScratch *scratch = &sps->collision.instanceHitbox;

	if (Coll_BspHitboxClass(node) == BSP_HITBOX_CLASS_TOUCH)
	{
		sps->bspHitbox = node;
		sps->boolDidTouchHitbox = (s16)CTR_MipsAddLo((u16)sps->boolDidTouchHitbox, 1);
	}

	if ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_FORCE_INSTANCE_HIT) != 0)
	{
		CTR_SET_VEC3(sps->hit.plane.normal.v, 0, COLL_FRACTION_ONE, 0);
		sps->hit.reorderResult = COLL_TRIANGLE_CLIP_FACE;
		sps->hitFraction = 0;
		sps->bspHitbox = node;
		sps->Union.QuadBlockColl.hitPos = sps->Union.QuadBlockColl.pos;
		sps->boolDidTouchHitbox = (s16)CTR_MipsAddLo((u16)sps->boolDidTouchHitbox, 1);
		return 6;
	}

	scratch->segmentDelta.y = 0;

	s32 diffX = sps->Input1.pos.x - sps->Union.QuadBlockColl.pos.x;
	s32 diffY = 0;
	s32 diffZ = sps->Input1.pos.z - sps->Union.QuadBlockColl.pos.z;
	scratch->segmentDelta.x = diffX;
	scratch->segmentDelta.z = diffZ;

	s32 centerDiffX = node->data.hitbox.center.x - sps->Union.QuadBlockColl.pos.x;
	s32 centerDiffY = 0;
	s32 centerDiffZ = node->data.hitbox.center.z - sps->Union.QuadBlockColl.pos.z;
	scratch->centerDelta.y = 0;
	scratch->centerDelta.x = centerDiffX;
	scratch->centerDelta.z = centerDiffZ;

	if ((node->flag & BSP_HITBOX_USE_Y_AXIS) != 0)
	{
		diffY = sps->Input1.pos.y - sps->Union.QuadBlockColl.pos.y;
		scratch->segmentDelta.y = diffY;
		centerDiffY = node->data.hitbox.center.y - sps->Union.QuadBlockColl.pos.y;
		scratch->centerDelta.y = centerDiffY;
	}

	CollFixed_GteLoadR11R12(CTR_PackS16Pair(diffX, diffY));
	CollFixed_GteLoadR13R21(CTR_PackS16Pair(diffZ, centerDiffX));
	CollFixed_GteLoadR22R23(CTR_PackS16Pair(centerDiffY, centerDiffZ));
	CollFixed_GteLoadVXY0(CTR_PackS16Pair(diffX, diffY));
	CollFixed_GteLoadVZ0(diffZ);
	CollFixed_GteMVMVA();

	s32 dotSegment = CollFixed_GteReadMAC1();
	s32 dotCenter = CollFixed_GteReadMAC2();
	scratch->segmentDot = dotSegment;
	scratch->centerDot = dotCenter;

	if (dotCenter <= 0)
	{
		return 0;
	}

	CollFixed_GteLoadLZCS(dotCenter);
	s32 shift = CollFixed_GteReadLZCR() - 2;
	if (shift < 0)
	{
		shift = 0;
	}
	else if (shift > 12)
	{
		shift = 12;
	}

	s32 divisor = CTR_MipsSra(dotSegment, 12 - shift);
	dotCenter = CTR_MipsSll(dotCenter, shift);

	if (divisor < 0)
	{
		return 0;
	}

	s32 factor = 0;
	if (divisor != 0)
	{
		factor = CTR_MipsDiv(dotCenter, divisor);
	}
	scratch->lineFactor = factor;

	s32 projX = CTR_MipsSra(CTR_MipsMulLo(factor, diffX), 12);
	s32 projY = 0;
	if ((node->flag & BSP_HITBOX_USE_Y_AXIS) != 0)
	{
		projY = CTR_MipsSra(CTR_MipsMulLo(factor, diffY), 12);
	}
	s32 projZ = CTR_MipsSra(CTR_MipsMulLo(factor, diffZ), 12);

	s32 relX = CTR_MipsSubLo(projX, centerDiffX);
	s32 relY = CTR_MipsSubLo(projY, centerDiffY);
	s32 relZ = CTR_MipsSubLo(projZ, centerDiffZ);
	scratch->projectedDelta.x = projX;
	scratch->projectedDelta.y = projY;
	scratch->projectedDelta.z = projZ;

	CollFixed_GteLoadR11R12(CTR_PackS16Pair(relX, relY));
	CollFixed_GteLoadR13R21(relZ);
	CollFixed_GteLoadVXY0(CTR_PackS16Pair(relX, relY));
	CollFixed_GteLoadVZ0(relZ);
	CollFixed_GteMVMVA();

	s32 radius = node->data.hitbox.radius;
	s32 radiusSquared = sps->Input1.hitRadius + radius;
	radiusSquared = CTR_MipsMulLo(radiusSquared, radiusSquared);
	s32 distSquared = CollFixed_GteReadMAC1();
	scratch->radiusSquared = radiusSquared;
	scratch->distanceSquared = distSquared;

	s32 remaining = CTR_MipsSubLo(radiusSquared, distSquared);
	if (remaining < 0)
	{
		return 0;
	}

	if (remaining != 0)
	{
		if (dotSegment != 0)
		{
			factor = CTR_MipsSubLo(factor, CTR_MipsDiv(CTR_MipsSll(remaining, 12), dotSegment));
		}
		scratch->adjustedFactor = factor;
	}

	if (sps->hitFraction < factor)
	{
		return 0;
	}

	s32 hitX = 0;
	s32 hitY = 0;
	s32 hitZ = 0;
	if (factor > 0)
	{
		hitX = CTR_MipsSra(CTR_MipsMulLo(diffX, factor), 12);
		hitY = CTR_MipsSra(CTR_MipsMulLo(diffY, factor), 12);
		hitZ = CTR_MipsSra(CTR_MipsMulLo(diffZ, factor), 12);
	}

	if ((node->flag & BSP_HITBOX_CHECK_Y_RANGE) != 0)
	{
		s32 centerY = node->data.hitbox.center.y;
		if ((hitY < centerY) && (CTR_MipsAddLo(centerY, node->id) < hitY))
		{
			return 0;
		}
	}

	sps->bspHitbox = node;
	sps->hitFraction = factor;
	sps->boolDidTouchHitbox = (s16)CTR_MipsAddLo((u16)sps->boolDidTouchHitbox, 1);
	scratch->hitDelta.x = hitX;
	scratch->hitDelta.y = hitY;
	scratch->hitDelta.z = hitZ;

	s32 normalX = CTR_MipsSubLo(hitX, centerDiffX);
	s32 normalY = 0;
	if ((node->flag & BSP_HITBOX_USE_Y_AXIS) != 0)
	{
		normalY = CTR_MipsSubLo(hitY, centerDiffY);
	}
	s32 normalZ = CTR_MipsSubLo(hitZ, centerDiffZ);

	CollFixed_GteLoadR11R12(CTR_PackS16Pair(normalX, normalY));
	CollFixed_GteLoadR13R21(normalZ);
	CollFixed_GteLoadVXY0(CTR_PackS16Pair(normalX, normalY));
	CollFixed_GteLoadVZ0(normalZ);
	CollFixed_GteMVMVA();

	s32 len = SquareRoot0(CollFixed_GteReadMAC1());
	s32 invLen = CTR_MipsDiv(0x1000000, len);

	normalX = CTR_MipsSra(CTR_MipsMulLo(normalX, invLen), 12);
	normalY = CTR_MipsSra(CTR_MipsMulLo(normalY, invLen), 12);
	normalZ = CTR_MipsSra(CTR_MipsMulLo(normalZ, invLen), 12);
	scratch->normal.x = normalX;
	scratch->normal.y = normalY;
	scratch->normal.z = normalZ;

	sps->Union.QuadBlockColl.hitPos.x = (s16)CTR_MipsAddLo((u16)sps->Union.QuadBlockColl.pos.x, hitX);
	CTR_SET_VEC3(sps->hit.plane.normal.v, (s16)normalX, (s16)normalY, (s16)normalZ);
	sps->Union.QuadBlockColl.hitPos.z = (s16)CTR_MipsAddLo((u16)sps->Union.QuadBlockColl.pos.z, hitZ);
	sps->Union.QuadBlockColl.hitPos.y = (s16)CTR_MipsAddLo((u16)sps->Union.QuadBlockColl.pos.y, hitY);
	sps->hit.reorderResult = COLL_TRIANGLE_CLIP_FACE;

	s32 scaledX = CTR_MipsSra(CTR_MipsMulLo(normalX, radius), 12);
	s32 scaledY = CTR_MipsSra(CTR_MipsMulLo(normalY, radius), 12);
	s32 scaledZ = CTR_MipsSra(CTR_MipsMulLo(normalZ, radius), 12);
	scratch->scaledNormal.x = scaledX;
	scratch->scaledNormal.y = scaledY;
	scratch->scaledNormal.z = scaledZ;

	sps->hit.pushOut.x = (s16)CTR_MipsAddLo((u16)node->data.hitbox.center.x, scaledX);
	sps->hit.pushOut.y = (s16)CTR_MipsAddLo((u16)node->data.hitbox.center.y, scaledY);
	sps->hit.pushOut.z = (s16)CTR_MipsAddLo((u16)node->data.hitbox.center.z, scaledZ);
	sps->hit.hitPos = sps->hit.pushOut;

	return 0;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d610-0x8001d77c
void COLL_FIXED_BSPLEAF_TestInstance(struct BSP *node, struct ScratchpadStruct *sps)
{
	struct BSP *bspArray = node->data.leaf.bspHitboxArray;

	if (bspArray == NULL)
	{
		return;
	}

	// check every instance hitbox until
	// end of list (null flag) is found
	for (/**/; bspArray->flag != 0; bspArray++)
	{
		struct BoundingBox *bbox = &bspArray->box;

		// 1F8001CC
		s32 arraySize = sps->numBspHitboxesHit - 1;
		for (; arraySize >= 0; arraySize--)
		{
			if (bspArray == sps->bspHitboxesHit[arraySize])
			{
				goto NextBSP;
			}
		}

		if ((
		        // if hitbox data is not tied to an active visible instance
		        (
		            // if collision for instance is disabled
		            ((bspArray->flag & BSP_HITBOX_COLLIDABLE) == 0) ||
		            // if bspHitbox.InstDef doesn't exist
		            (bspArray->data.hitbox.instDef == NULL))

		        ||

		        // if data is valid

		        // allows drawing flag is enabled
		        ((bspArray->data.hitbox.instDef->ptrInstance->flags & DRAW_COLLISION_MASK) != 0)) &&

		    // compare bsp boundingbox to scratchpad boundingbox
		    ((sps->bbox.min.x <= bbox->max.x) &&

		     (bbox->min.x <= sps->bbox.max.x) &&

		     (sps->bbox.min.y <= bbox->max.y) &&

		     (bbox->min.y <= sps->bbox.max.y) &&

		     (sps->bbox.min.z <= bbox->max.z) &&

		     (bbox->min.z <= sps->bbox.max.z)))
		{
			// check with collision for this instance
			COLL_FIXED_INSTANC_TestPoint(sps, bspArray);
		}

	NextBSP:;
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d77c-0x8001d944
void COLL_FIXED_BotsSearch(const SVec3 *posCurr, const SVec3 *posPrev, struct ScratchpadStruct *sps)
{
	s16 radius = sps->Input1.hitRadius;
	s32 sqrRadius = CTR_MipsMulLo(radius, radius);

	sps->Input1.hitRadiusSquared = sqrRadius;
	sps->Union.QuadBlockColl.hitRadiusSquared = sqrRadius;
	sps->Union.QuadBlockColl.hitRadius = radius;
	sps->Input1.pos = *posCurr;
	sps->Union.QuadBlockColl.hitPos = *posCurr;
	sps->Union.QuadBlockColl.pos = *posPrev;

	for (s32 axis = 0; axis < 3; axis++)
	{
		s16 curr = posCurr->v[axis];
		s16 prev = posPrev->v[axis];
		s32 deltaCurr = CTR_MipsSubLo(curr, radius);
		s32 deltaPrev = CTR_MipsSubLo(prev, radius);
		sps->bbox.min.v[axis] = (deltaCurr < deltaPrev) ? deltaCurr : deltaPrev;

		deltaCurr = CTR_MipsAddLo(curr, radius);
		deltaPrev = CTR_MipsAddLo(prev, radius);
		sps->bbox.max.v[axis] = (deltaCurr > deltaPrev) ? deltaCurr : deltaPrev;
	}

	sps->numTrianglesTested = 0;
	sps->boolDidTouchHitbox = 0;

	sps->hitFraction = COLL_FRACTION_ONE;
	sps->numBspHitboxesHit = 0;
	sps->collision.stepFlags = 0;

	COLL_SearchBSP_CallbackPARAM(sps->ptr_mesh_info->bspRoot, &sps->bbox, COLL_FIXED_BSPLEAF_TestInstance, sps);
}


internal void COLL_FIXED_TRIANGL_TestPoint_Body(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2,
                                                struct BspSearchVertex *v3, s32 normalZW);

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001ef1c-0x8001ef50
void COLL_FIXED_TRIANGL_UNUSED(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	// NOTE(aalhendi): Retail skips the TestPoint setup and jumps into the
	// shared body with t2 preloaded from sps+0x58. Native makes a0-a3 and t2
	// explicit.
	COLL_FIXED_TRIANGL_TestPoint_Body(sps, v1, v2, v3, (s32)CTR_PackS16Pair(sps->candidate.plane.normal.z, sps->candidate.plane.halfDistance));
}


internal void COLL_FIXED_TRIANGL_TestPoint_Body(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2,
                                                struct BspSearchVertex *v3, s32 normalZW)
{
	struct CollTriangleProjection projection;

	s32 startX = sps->Union.QuadBlockColl.pos.x;
	s32 startY = sps->Union.QuadBlockColl.pos.y;
	s32 startZ = sps->Union.QuadBlockColl.pos.z;
	s32 deltaX = sps->Union.QuadBlockColl.hitPos.x - startX;
	s32 deltaY = sps->Union.QuadBlockColl.hitPos.y - startY;
	s32 deltaZ = sps->Union.QuadBlockColl.hitPos.z - startZ;

	CollFixed_GteLoadR11R12(CTR_PackS16Pair(startX, startY));
	CollFixed_GteLoadR13R21(CTR_PackS16Pair(startZ, deltaX));
	CollFixed_GteLoadR22R23(CTR_PackS16Pair(deltaY, deltaZ));
	CollFixed_GteLoadVXY0(CTR_PackS16Pair(sps->candidate.plane.normal.x, sps->candidate.plane.normal.y));
	CollFixed_GteLoadVZ0(normalZW);

	normalZW = CTR_MipsSll(CTR_MipsSra(normalZW, 16), 13);

	CollFixed_GteMVMVA();
	s32 lineDot = CollFixed_GteReadMAC2();
	s32 planeDot = CTR_MipsSubLo(CollFixed_GteReadMAC1(), normalZW);

	if (lineDot >= 0)
	{
		return;
	}

	s32 factor = CTR_MipsDiv(CTR_MipsNegLo(planeDot), CTR_MipsSra(lineDot, 12));

	CollFixed_GteLoadMAC(startX, startY, startZ);
	CollFixed_GteLoadIR(deltaX, deltaY, deltaZ);
	CollFixed_GteLoadIR0(factor);

	if ((factor < 0) || (factor > COLL_FRACTION_ONE))
	{
		return;
	}

	CollFixed_GteGPL12();
	s32 hitX = CollFixed_GteReadMAC1();
	s32 hitY = CollFixed_GteReadMAC2();
	s32 hitZ = CollFixed_GteReadMAC3();

	CTR_SET_VEC3(sps->candidate.hitPos.v, (s16)hitX, (s16)hitY, (s16)hitZ);

	struct BspSearchVertex *baryVertex1 = v2;
	struct BspSearchVertex *baryVertex2 = v3;
	CollNormalAxis normalAxis = sps->candidate.normalAxis;

	if (normalAxis == COLL_NORMAL_AXIS_Y)
	{
		s32 origin = v1->pos.z;
		projection.firstA = v2->pos.z - origin;
		projection.firstB = v3->pos.z - origin;
		projection.firstHit = hitZ - origin;

		if (Coll_MipsAbsS32(projection.firstA) < Coll_MipsAbsS32(projection.firstB))
		{
			s32 tmp = projection.firstA;
			projection.firstA = projection.firstB;
			projection.firstB = tmp;
			baryVertex1 = v3;
			baryVertex2 = v2;
		}

		origin = v1->pos.x;
		projection.secondA = baryVertex1->pos.x - origin;
		projection.secondB = baryVertex2->pos.x - origin;
		projection.secondHit = hitX - origin;
	}
	else if (normalAxis == COLL_NORMAL_AXIS_Z)
	{
		s32 origin = v1->pos.x;
		projection.firstA = v2->pos.x - origin;
		projection.firstB = v3->pos.x - origin;
		projection.firstHit = hitX - origin;

		if (Coll_MipsAbsS32(projection.firstA) < Coll_MipsAbsS32(projection.firstB))
		{
			s32 tmp = projection.firstA;
			projection.firstA = projection.firstB;
			projection.firstB = tmp;
			baryVertex1 = v3;
			baryVertex2 = v2;
		}

		origin = v1->pos.y;
		projection.secondA = baryVertex1->pos.y - origin;
		projection.secondB = baryVertex2->pos.y - origin;
		projection.secondHit = hitY - origin;
	}
	else
	{
		s32 origin = v1->pos.y;
		projection.firstA = v2->pos.y - origin;
		projection.firstB = v3->pos.y - origin;
		projection.firstHit = hitY - origin;

		if (Coll_MipsAbsS32(projection.firstA) < Coll_MipsAbsS32(projection.firstB))
		{
			s32 tmp = projection.firstA;
			projection.firstA = projection.firstB;
			projection.firstB = tmp;
			baryVertex1 = v3;
			baryVertex2 = v2;
		}

		origin = v1->pos.z;
		projection.secondA = baryVertex1->pos.z - origin;
		projection.secondB = baryVertex2->pos.z - origin;
		projection.secondHit = hitZ - origin;
	}

	s32 baryA = COLL_FRACTION_INVALID;
	s32 baryB = COLL_FRACTION_INVALID;

	if (projection.firstA != 0)
	{
		s32 denom = CTR_MipsSra(CTR_MipsSubLo(CTR_MipsMulLo(projection.secondB, projection.firstA), CTR_MipsMulLo(projection.firstB, projection.secondA)), 6);

		if (denom != 0)
		{
			baryB = CTR_MipsDiv(
			    CTR_MipsSll(CTR_MipsSubLo(CTR_MipsMulLo(projection.secondHit, projection.firstA), CTR_MipsMulLo(projection.firstHit, projection.secondA)), 6),
			    denom);

			if ((baryB >= 0) && (baryB <= COLL_FRACTION_ONE))
			{
				baryA = CTR_MipsDiv(CTR_MipsSubLo(CTR_MipsSll(projection.firstHit, 12), CTR_MipsMulLo(baryB, projection.firstB)), projection.firstA);
			}
		}
	}
	else
	{
		if (projection.firstB == 0)
		{
			return;
		}

		baryB = CTR_MipsDiv(CTR_MipsSll(projection.firstHit, 12), projection.firstB);

		if ((baryB >= 0) && (baryB <= COLL_FRACTION_ONE))
		{
			baryA = CTR_MipsDiv(CTR_MipsSubLo(CTR_MipsSll(projection.secondHit, 12), CTR_MipsMulLo(baryB, projection.secondB)), projection.secondA);
		}
	}

	struct QuadBlock *quad = sps->candidate.ptrQuadblock;

	if ((baryA < 0) || (CTR_MipsSubLo(CTR_MipsAddLo(baryA, baryB), COLL_FRACTION_ONE) > 0))
	{
		return;
	}

	if ((quad->quadFlags & QUADBLOCK_FLAG_TRIGGER) != 0)
	{
		sps->collision.stepFlags |= (u8)quad->terrain_type;
		return;
	}

	sps->hit.ptrQuadblock = quad;
	sps->hitBarycentrics.v1 = (s16)baryA;
	sps->hitBarycentrics.v2 = (s16)baryB;
	sps->hitLevelTriangle.v0 = v1->pLevelVertex;
	sps->hitLevelTriangle.v1 = baryVertex1->pLevelVertex;
	sps->hitLevelTriangle.v2 = baryVertex2->pLevelVertex;
	sps->boolDidTouchQuadblock = (s16)CTR_MipsAddLo(sps->boolDidTouchQuadblock, 1);
	sps->hit.hitPos = sps->candidate.hitPos;
	sps->Union.QuadBlockColl.hitPos = sps->candidate.hitPos;
	sps->hit.plane = sps->candidate.plane;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001ef50-0x8001f2dc
void COLL_FIXED_TRIANGL_TestPoint(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	s32 normalZW = (s32)CTR_PackS16Pair(v1->plane.normal.z, v1->plane.halfDistance);

	sps->numTrianglesTested = (s16)CTR_MipsAddLo(sps->numTrianglesTested, 1);
	sps->candidate.normalAxis = v1->normalAxis;
	sps->candidate.plane = v1->plane;

	COLL_FIXED_TRIANGL_TestPoint_Body(sps, v1, v2, v3, normalZW);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f2dc-0x8001f41c
void COLL_FIXED_TRIANGL_GetNormVec(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	s32 v1x = v1->pos.x;
	s32 v1y = v1->pos.y;
	s32 v1z = v1->pos.z;
	s32 edgeAX = v3->pos.x - v1x;
	s32 edgeAY = v3->pos.y - v1y;
	s32 edgeAZ = v3->pos.z - v1z;
	s32 edgeBX = v2->pos.x - v1x;
	s32 edgeBY = v2->pos.y - v1y;
	s32 edgeBZ = v2->pos.z - v1z;

	CollFixed_GteLoadR11R12((u16)edgeAX);
	CollFixed_GteLoadR22R23((u16)edgeAY);
	CollFixed_GteLoadR33((u16)edgeAZ);
	CollFixed_GteLoadIR(edgeBX, edgeBY, edgeBZ);

	u32 lodShift = sps->collision.triNormalLodShift;
	u32 normalShift = sps->collision.triNormalVecBitShift;
	s32 scale = sps->collision.triNormalVecDividend;

	CollFixed_GteOP0();

	s32 nx = CollFixed_GteReadMAC1();
	s32 ny = CollFixed_GteReadMAC2();
	s32 nz = CollFixed_GteReadMAC3();

	CollFixed_GteLoadR11R12(CTR_PackS16Pair(v1x, v1y));
	CollFixed_GteLoadR13R21((u16)v1z);

	nx = CTR_MipsSra(CTR_MipsMulLo(CTR_MipsSra(nx, lodShift), scale), normalShift);
	ny = CTR_MipsSra(CTR_MipsMulLo(CTR_MipsSra(ny, lodShift), scale), normalShift);
	nz = CTR_MipsSra(CTR_MipsMulLo(CTR_MipsSra(nz, lodShift), scale), normalShift);

	CollFixed_GteLoadIR(nx, ny, nz);
	v1->plane.normal.x = (s16)nx;
	v1->plane.normal.y = (s16)ny;

	CollFixed_GteRTIR();
	s32 plane = CollFixed_GteReadMAC1();

	v1->plane.normal.z = (s16)nz;
	v1->plane.halfDistance = (s16)CTR_MipsSra(plane, 1);

	s32 absX = Coll_MipsAbsS32(nx);
	s32 absY = Coll_MipsAbsS32(ny);
	s32 absZ = Coll_MipsAbsS32(nz);
	CollNormalAxis dominantAxis = COLL_NORMAL_AXIS_Z;

	if ((absX - absY) < 0)
	{
		if ((absY - absZ) >= 0)
		{
			dominantAxis = COLL_NORMAL_AXIS_Y;
		}
	}
	else if ((absX - absZ) >= 0)
	{
		dominantAxis = COLL_NORMAL_AXIS_X;
	}

	v1->normalAxis = dominantAxis;
}


global_variable struct LevVertex *sCollFixedLoadScratchpadVertsVertexArray;
global_variable struct QuadBlock *sCollFixedLoadScratchpadVertsQuad;

internal void COLL_FIXED_QUADBLK_SetLoadScratchpadVertsContext(struct ScratchpadStruct *sps, struct QuadBlock *quad)
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
	struct BspSearchVertex *bsv = &sps->bspSearchVert[0];

	sps->quadSecondTriIndexA = ptrQuad->index[2];
	sps->quadSecondTriIndexB = ptrQuad->index[3];

	for (const u16 *index = &ptrQuad->index[0]; index < &ptrQuad->index[9]; index++, bsv++)
	{
		struct LevVertex *vertCurr = &ptrVert[*index];
		bsv->pLevelVertex = vertCurr;
		bsv->pos = vertCurr->pos;
		bsv->normalAxis = (CollNormalAxis)vertCurr->flags;
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f67c-0x8001f6f0
void COLL_FIXED_QUADBLK_GetNormVecs_LoLOD(struct ScratchpadStruct *sps, struct QuadBlock *quad)
{
	COLL_FIXED_QUADBLK_SetLoadScratchpadVertsContext(sps, quad);
	COLL_FIXED_QUADBLK_LoadScratchpadVerts(sps);

	// always 2 for low poly (big block)
	sps->collision.triNormalLodShift = 2;

	sps->collision.triNormalVecBitShift = quad->triNormalVecBitShift;

	const s16 *triNormalDividends = &quad->triNormalVecDividend[0];

	struct BspSearchVertex *bsv = &sps->bspSearchVert[0];
	b32 hasSecondTriangle = sps->quadSecondTriIndexA != sps->quadSecondTriIndexB;

	if (hasSecondTriangle)
	{
		sps->collision.triNormalVecDividend = triNormalDividends[QUADBLOCK_TRI_NORMAL_DIVIDEND_LO_1];
		COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[1], &bsv[3], &bsv[2]); // 1, 3, 2
	}
	sps->collision.triNormalVecDividend = triNormalDividends[QUADBLOCK_TRI_NORMAL_DIVIDEND_LO_0];
	COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[0], &bsv[1], &bsv[2]); // 0, 1, 2
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f6f0-0x8001f7f0
void COLL_FIXED_QUADBLK_GetNormVecs_HiLOD(struct ScratchpadStruct *sps, struct QuadBlock *quad)
{
	COLL_FIXED_QUADBLK_SetLoadScratchpadVertsContext(sps, quad);
	COLL_FIXED_QUADBLK_LoadScratchpadVerts(sps);

	// always 0 for high poly (small block)
	sps->collision.triNormalLodShift = 0;

	sps->collision.triNormalVecBitShift = quad->triNormalVecBitShift;

	const s16 *triNormalDividends = &quad->triNormalVecDividend[0];

	// calculate normal vectors for eight triangles,
	// no collision detection here
	struct BspSearchVertex *bsv = &sps->bspSearchVert[0];
	b32 hasSecondTriangle = sps->quadSecondTriIndexA != sps->quadSecondTriIndexB;

	if (hasSecondTriangle)
	{
		sps->collision.triNormalVecDividend = triNormalDividends[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_4];
		COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[8], &bsv[6], &bsv[7]); // 8, 6, 7

		sps->collision.triNormalVecDividend = triNormalDividends[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_5];
		COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[7], &bsv[3], &bsv[8]); // 7, 3, 8

		sps->collision.triNormalVecDividend = triNormalDividends[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_6];
		COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[1], &bsv[7], &bsv[6]); // 1, 7, 6

		sps->collision.triNormalVecDividend = triNormalDividends[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_7];
		COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[2], &bsv[6], &bsv[8]); // 2, 6, 8
	}

	sps->collision.triNormalVecDividend = triNormalDividends[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_0];
	COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[0], &bsv[4], &bsv[5]); // 0, 4, 5

	sps->collision.triNormalVecDividend = triNormalDividends[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_1];
	COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[4], &bsv[6], &bsv[5]); // 4, 6, 5

	sps->collision.triNormalVecDividend = triNormalDividends[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_2];
	COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[6], &bsv[4], &bsv[1]); // 6, 4, 1

	sps->collision.triNormalVecDividend = triNormalDividends[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_3];
	COLL_FIXED_TRIANGL_GetNormVec(sps, &bsv[5], &bsv[6], &bsv[2]); // 5, 6, 2
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f41c-0x8001f5f0
void COLL_FIXED_QUADBLK_TestTriangles(struct QuadBlock *quad, struct ScratchpadStruct *sps)
{
	struct BspSearchVertex *bsv = &sps->bspSearchVert[0];
	b32 hasSecondTriangle = quad->index[2] != quad->index[3];

	sps->candidate.ptrQuadblock = quad;

	if (((sps->Union.QuadBlockColl.quadFlagsWanted & quad->quadFlags) == 0) || ((sps->Union.QuadBlockColl.quadFlagsIgnored & quad->quadFlags) != 0) ||
	    (quad->bbox.min.x > sps->bbox.max.x) || (quad->bbox.min.y > sps->bbox.max.y) || (quad->bbox.min.z > sps->bbox.max.z) ||
	    (sps->bbox.min.x > quad->bbox.max.x) || (sps->bbox.min.y > quad->bbox.max.y) || (sps->bbox.min.z > quad->bbox.max.z))
	{
		return;
	}

	// if 3P or 4P mode,
	// then use low-LOD quadblock collision (two triangles)
	if ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_HIGH_LOD) == 0)
	{
		COLL_FIXED_QUADBLK_GetNormVecs_LoLOD(sps, quad);

		COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[0], &bsv[1], &bsv[2]); // 0,1,2

		if (hasSecondTriangle)
		{
			COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[1], &bsv[3], &bsv[2]); // 1,3,2
		}
	}
	else
	{
		if ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_REUSE_NORMALS) == 0)
		{
			COLL_FIXED_QUADBLK_GetNormVecs_HiLOD(sps, quad);
		}

		COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[0], &bsv[4], &bsv[5]); // 0, 4, 5
		COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[4], &bsv[6], &bsv[5]); // 4, 6, 5
		COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[6], &bsv[4], &bsv[1]); // 6, 4, 1
		COLL_FIXED_TRIANGL_TestPoint(sps, &bsv[5], &bsv[6], &bsv[2]); // 5, 6, 2

		if (hasSecondTriangle)
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
	// if bsp flag is water
	if ((node->flag & BSP_LEAF_FLAG_WATER) != 0)
	{
		sps->collision.stepFlags |= COLL_STEP_FLAG_WATER_BSP;
	}

	s32 numQuads = node->data.leaf.numQuads;
	struct QuadBlock *ptrQuad = node->data.leaf.ptrQuadBlockArray;

	// loop through all quadblocks
	do
	{
		COLL_FIXED_QUADBLK_TestTriangles(ptrQuad++, sps);
		numQuads--;
	} while (numQuads > 0);

	if ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_TEST_INSTANCES) != 0)
	{
		COLL_FIXED_BSPLEAF_TestInstance(node, sps);
	}
}


struct CollFixedPlayerTrig
{
	s32 x;
	s32 z;
};

enum
{
	COLL_FIXED_PLAYER_SEARCH_TOP_Y_OFFSET = 0x80,
	COLL_FIXED_PLAYER_SEARCH_BOTTOM_Y_OFFSET = 0x100,
	COLL_FIXED_PLAYER_SEARCH_COLOR_MAX = 0xff,
	COLL_FIXED_PLAYER_SEARCH_LUMA_RED_WEIGHT = 0x4c,
	COLL_FIXED_PLAYER_SEARCH_LUMA_GREEN_WEIGHT = 0x96,
	COLL_FIXED_PLAYER_SEARCH_LUMA_BLUE_WEIGHT = 0x1e,
	COLL_FIXED_PLAYER_SEARCH_LIGHT_BASE = 0xc00,
	COLL_FIXED_PLAYER_SEARCH_LIGHT_LUMA_SHIFT = 5,
	COLL_FIXED_PLAYER_SEARCH_LIGHT_SCALE_SHIFT = 3,
	COLL_FIXED_PLAYER_SEARCH_LIGHT_SCALE_MAX = 0x8000,
	COLL_FIXED_PLAYER_SEARCH_ALPHA_BLEND_CURRENT_WEIGHT = 200,
	COLL_FIXED_PLAYER_SEARCH_AXIS3_OLD_NORMAL_WEIGHT = 5,
	COLL_FIXED_PLAYER_SEARCH_AXIS3_HIT_NORMAL_WEIGHT = 3,
	COLL_FIXED_PLAYER_SEARCH_NORMAL_SQRT_SHIFT = 0x18,
	COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_TOTAL_WEIGHT = 8,
	COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_SHIFT = 3,
	COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_GROUND_WEIGHT = 6,
	COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_AIRBORNE_WEIGHT = 7,
	COLL_FIXED_PLAYER_SEARCH_COYOTE_TIME_MS = 0xa0,
	COLL_FIXED_PLAYER_SEARCH_HAZARD_WOBBLE_ANGLE_STEP = 0xc,
	COLL_FIXED_PLAYER_SEARCH_HAZARD_WOBBLE_AMPLITUDE = 0x19,
	COLL_FIXED_PLAYER_SEARCH_HAZARD_WOBBLE_SHIFT = 10,
};

internal s32 COLL_FIXED_PlayerSearch_ClampByte(s32 value)
{
	if (value < 0)
	{
		return 0;
	}

	if (value > COLL_FIXED_PLAYER_SEARCH_COLOR_MAX)
	{
		return COLL_FIXED_PLAYER_SEARCH_COLOR_MAX;
	}

	return value;
}

force_inline s32 COLL_FIXED_PlayerSearch_InterpolateColor(s32 baryA, s32 baryB, s32 c0, s32 c1, s32 c2)
{
	s32 term1 = CTR_MipsSra(CTR_MipsMulLo(baryA, CTR_MipsSubLo(c1, c0)), 12);
	s32 term2 = CTR_MipsSra(CTR_MipsMulLo(baryB, CTR_MipsSubLo(c2, c0)), 12);
	return CTR_MipsAddLo(CTR_MipsAddLo(term1, term2), c0);
}

force_inline s32 COLL_FIXED_PlayerSearch_LumaTerm(s32 color, s32 weight)
{
	return CTR_MipsSra(CTR_MipsMulLo(color, weight), 8);
}

force_inline s32 COLL_FIXED_PlayerSearch_AlphaBlend(s32 current, s32 light)
{
	return CTR_MipsSra(CTR_MipsAddLo(CTR_MipsMulLo((u16)current, COLL_FIXED_PLAYER_SEARCH_ALPHA_BLEND_CURRENT_WEIGHT), light), 8);
}

internal struct CollFixedPlayerTrig COLL_FIXED_PlayerSearch_Trig(s32 angle)
{
	struct TrigTable trig = data.trigApprox[ANG_MODULO_HALF_PI(angle)];
	struct CollFixedPlayerTrig out;

	if (IS_ANG_FIRST_OR_THIRD_QUADRANT(angle))
	{
		out.x = trig.sin;
		out.z = trig.cos;

		if (IS_ANG_THIRD_OR_FOURTH_QUADRANT(angle))
		{
			out.x = CTR_MipsNegLo(out.x);
			out.z = CTR_MipsNegLo(out.z);
		}
	}
	else
	{
		out.x = trig.cos;
		out.z = CTR_MipsNegLo(trig.sin);

		if (IS_ANG_THIRD_OR_FOURTH_QUADRANT(angle))
		{
			out.x = CTR_MipsNegLo(out.x);
			out.z = CTR_MipsNegLo(out.z);
		}
	}

	return out;
}

internal void COLL_FIXED_PlayerSearch_SetupSearch(struct ScratchpadStruct *sps, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;
	s32 posX = CTR_MipsSra(d->posCurr.x, 8);
	s32 posY = CTR_MipsSra(d->posCurr.y, 8);
	s32 posZ = CTR_MipsSra(d->posCurr.z, 8);
	SVec3 probeBottom = {
	    .x = (s16)posX,
	    .y = (s16)CTR_MipsAddLo(posY, -COLL_FIXED_PLAYER_SEARCH_BOTTOM_Y_OFFSET),
	    .z = (s16)posZ,
	};
	SVec3 probeTop = {
	    .x = (s16)posX,
	    .y = (s16)CTR_MipsAddLo(posY, COLL_FIXED_PLAYER_SEARCH_TOP_Y_OFFSET),
	    .z = (s16)posZ,
	};

	d->actionsFlagSet &= ~ACTION_ENGINE_ECHO;

	sps->Union.QuadBlockColl.pos = probeTop;
	sps->Input1.pos = probeBottom;

	sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;
	sps->Union.QuadBlockColl.quadFlagsIgnored = QUADBLOCK_FLAG_NO_COLLISION_RESPONSE;
	sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND | QUADBLOCK_FLAG_COLLISION_SURFACE;

	sps->Union.QuadBlockColl.searchFlags = 0;
	if (gGT->numPlyrCurrGame < 3)
	{
		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;
	}

	sps->boolDidTouchQuadblock = 0;
	sps->boolDidTouchHitbox = 0;
	sps->numTrianglesTested = 0;

	sps->bbox.min.x = probeTop.x;
	sps->bbox.max.x = probeTop.x;
	sps->bbox.min.y = (probeBottom.y < probeTop.y) ? probeBottom.y : probeTop.y;
	sps->bbox.max.y = (probeTop.y < probeBottom.y) ? probeBottom.y : probeTop.y;
	sps->bbox.min.z = probeTop.z;
	sps->bbox.max.z = probeTop.z;

	sps->Union.QuadBlockColl.hitPos = probeBottom;
}

internal void COLL_FIXED_PlayerSearch_UpdateLighting(struct ScratchpadStruct *sps, struct Driver *d, struct Instance *inst)
{
	struct LevVertex *v0 = sps->hitLevelTriangle.v0;
	struct LevVertex *v1 = sps->hitLevelTriangle.v1;
	struct LevVertex *v2 = sps->hitLevelTriangle.v2;

	if ((v0 == NULL) || (v1 == NULL) || (v2 == NULL))
	{
		return;
	}

	s32 baryA = sps->hitBarycentrics.v1;
	s32 baryB = sps->hitBarycentrics.v2;
	s32 r0 = v0->color_hi[0];
	s32 g0 = v0->color_hi[1];
	s32 b0 = v0->color_hi[2];
	s32 r = COLL_FIXED_PlayerSearch_InterpolateColor(baryA, baryB, r0, v1->color_hi[0], v2->color_hi[0]);
	s32 g = COLL_FIXED_PlayerSearch_InterpolateColor(baryA, baryB, g0, v1->color_hi[1], v2->color_hi[1]);
	s32 b = COLL_FIXED_PlayerSearch_InterpolateColor(baryA, baryB, b0, v1->color_hi[2], v2->color_hi[2]);

	r = COLL_FIXED_PlayerSearch_ClampByte(r);
	g = COLL_FIXED_PlayerSearch_ClampByte(g);
	b = COLL_FIXED_PlayerSearch_ClampByte(b);

	s32 luma = CTR_MipsAddLo(CTR_MipsAddLo(COLL_FIXED_PlayerSearch_LumaTerm(r, COLL_FIXED_PLAYER_SEARCH_LUMA_RED_WEIGHT),
	                                       COLL_FIXED_PlayerSearch_LumaTerm(g, COLL_FIXED_PLAYER_SEARCH_LUMA_GREEN_WEIGHT)),
	                         COLL_FIXED_PlayerSearch_LumaTerm(b, COLL_FIXED_PLAYER_SEARCH_LUMA_BLUE_WEIGHT));
	s32 light = CTR_MipsSubLo(COLL_FIXED_PLAYER_SEARCH_LIGHT_BASE, CTR_MipsSll(luma, COLL_FIXED_PLAYER_SEARCH_LIGHT_LUMA_SHIFT));

	if (light < 0)
	{
		light = 0;
	}

	s32 scaledLight = CTR_MipsSll(light, COLL_FIXED_PLAYER_SEARCH_LIGHT_SCALE_SHIFT);

	if (light > COLL_FRACTION_ONE)
	{
		light = COLL_FRACTION_ONE;
		scaledLight = COLL_FIXED_PLAYER_SEARCH_LIGHT_SCALE_MAX;
	}

	light = CTR_MipsSll(CTR_MipsSubLo(scaledLight, light), COLL_FIXED_PLAYER_SEARCH_LIGHT_SCALE_SHIFT);

	d->alphaScaleBackup = (s16)COLL_FIXED_PlayerSearch_AlphaBlend(d->alphaScaleBackup, light);
	inst->alphaScale = (s16)COLL_FIXED_PlayerSearch_AlphaBlend(inst->alphaScale, light);
}

internal void COLL_FIXED_PlayerSearch_NormalizeAxis3(struct ScratchpadStruct *sps, struct Driver *d)
{
	s32 x = CTR_MipsAddLo(CTR_MipsMulLo(d->AxisAngle3_normalVec.x, COLL_FIXED_PLAYER_SEARCH_AXIS3_OLD_NORMAL_WEIGHT),
	                      CTR_MipsMulLo(sps->hit.plane.normal.x, COLL_FIXED_PLAYER_SEARCH_AXIS3_HIT_NORMAL_WEIGHT));
	s32 y = CTR_MipsAddLo(CTR_MipsMulLo(d->AxisAngle3_normalVec.y, COLL_FIXED_PLAYER_SEARCH_AXIS3_OLD_NORMAL_WEIGHT),
	                      CTR_MipsMulLo(sps->hit.plane.normal.y, COLL_FIXED_PLAYER_SEARCH_AXIS3_HIT_NORMAL_WEIGHT));
	s32 z = CTR_MipsAddLo(CTR_MipsMulLo(d->AxisAngle3_normalVec.z, COLL_FIXED_PLAYER_SEARCH_AXIS3_OLD_NORMAL_WEIGHT),
	                      CTR_MipsMulLo(sps->hit.plane.normal.z, COLL_FIXED_PLAYER_SEARCH_AXIS3_HIT_NORMAL_WEIGHT));
	u32 sum = (u32)CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(x, x), CTR_MipsMulLo(y, y)), CTR_MipsMulLo(z, z));
	u32 len = VehCalc_FastSqrt(sum, COLL_FIXED_PLAYER_SEARCH_NORMAL_SQRT_SHIFT) >> 12;

	d->AxisAngle3_normalVec.x = (s16)CTR_MipsDiv(CTR_MipsSll(x, 12), (s32)len);
	d->AxisAngle3_normalVec.y = (s16)CTR_MipsDiv(CTR_MipsSll(y, 12), (s32)len);
	d->AxisAngle3_normalVec.z = (s16)CTR_MipsDiv(CTR_MipsSll(z, 12), (s32)len);
}

internal void COLL_FIXED_PlayerSearch_NormalizeAxis2(struct Driver *d, s32 x, s32 y, s32 z)
{
	u32 sum = (u32)CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(x, x), CTR_MipsMulLo(y, y)), CTR_MipsMulLo(z, z));
	u32 len = VehCalc_FastSqrt(sum, COLL_FIXED_PLAYER_SEARCH_NORMAL_SQRT_SHIFT) >> 12;

	d->AxisAngle2_normalVec.x = (s16)CTR_MipsDiv(CTR_MipsSll(x, 12), (s32)len);
	d->AxisAngle2_normalVec.y = (s16)CTR_MipsDiv(CTR_MipsSll(y, 12), (s32)len);
	d->AxisAngle2_normalVec.z = (s16)CTR_MipsDiv(CTR_MipsSll(z, 12), (s32)len);
}

internal b32 COLL_FIXED_PlayerSearch_CheckMaskGrabProgress(struct Driver *d, struct QuadBlock *quad)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Level *level = gGT->level1;

	if ((quad->quadFlags & QUADBLOCK_FLAG_KILL_PLANE) != 0)
	{
		return 1;
	}

	if ((d->kartState == KS_MASK_GRABBED) || ((d->collisionFlags & DRIVER_COLL_FLAG_MASK_GRAB_REQUEST) != 0) ||
	    ((quad->quadFlags & QUADBLOCK_FLAG_GROUND) == 0))
	{
		return 0;
	}

	if (quad->checkpointIndex == 0xff)
	{
		if ((u32)(gGT->levelID - GEM_STONE_VALLEY) < 5)
		{
			d->lastValid = quad;
		}
		return 0;
	}

	struct CheckpointNode *node = &level->ptr_restart_points[quad->checkpointIndex];

	if (((d->actionsFlagSet & ACTION_BEHIND_START_LINE) == 0) && (node->nextIndex_forward > 1) &&
	    ((((level->ptr_restart_points[0].distToFinish >> 2) << 3) < (s32)(d->distanceToFinish_checkpoint - CTR_MipsMulLo(node->distToFinish, 8)))))
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

	if ((node->distToFinish < (CTR_MipsMulLo(trackLength, 0xf) >> 4)) && (d->lastValid->checkpointIndex != 0xff) &&
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
	s32 normalBlendWeight;

	COLL_FIXED_PlayerSearch_SetupSearch(sps, d);

	if (d->underDriver != NULL)
	{
		COLL_FIXED_QUADBLK_TestTriangles(d->underDriver, sps);
	}

	if ((sps->boolDidTouchQuadblock == 0) && (sps->ptr_mesh_info != NULL) && (sps->ptr_mesh_info->bspRoot != NULL))
	{
		COLL_SearchBSP_CallbackPARAM(sps->ptr_mesh_info->bspRoot, &sps->bbox, COLL_FIXED_BSPLEAF_TestQuadblocks, sps);
	}

	struct Instance *inst = t->inst;

	if (sps->boolDidTouchQuadblock == 0)
	{
		inst->compressedNormalAndDriverIndex = INST_CompressNormalVectorAndDriverIndex(0, FP_ONE, 0, d->driverID);
		inst->flags &= ~REFLECTIVE;
		d->quadBlockHeight = CTR_MipsSubLo(d->posCurr.y, CTR_MipsSll(COLL_FIXED_PLAYER_SEARCH_BOTTOM_Y_OFFSET, 8));
	}
	else
	{
		struct QuadBlock *quad = sps->hit.ptrQuadblock;
		inst->compressedNormalAndDriverIndex =
		    INST_CompressNormalVectorAndDriverIndex(sps->hit.plane.normal.x, sps->hit.plane.normal.y, sps->hit.plane.normal.z, d->driverID);
		d->quadBlockHeight = CTR_MipsSll(sps->Union.QuadBlockColl.hitPos.y, 8);
		d->collisionFlags |= DRIVER_COLL_FLAG_TOUCHED_QUADBLOCK;

		if ((quad->terrain_type == TERRAIN_MUD) || (quad->terrain_type == TERRAIN_WATER) || (quad->terrain_type == TERRAIN_FASTWATER))
		{
			inst->vertSplit = 0;
			inst->flags |= SPLIT_LINE;
		}

		if (gGT->numPlyrCurrGame < 2)
		{
			u16 quadFlags = quad->quadFlags;

			if ((quadFlags & QUADBLOCK_FLAG_COLLISION_SURFACE) == 0)
			{
				if ((quadFlags & QUADBLOCK_FLAG_REFLECT_SPLIT_LINE_1) != 0)
				{
					inst->flags |= REFLECTIVE;
					inst->vertSplit = level->splitLines[1];
				}
				else if ((quadFlags & QUADBLOCK_FLAG_REFLECT_SPLIT_LINE_0) != 0)
				{
					inst->flags |= REFLECTIVE;
					inst->vertSplit = level->splitLines[0];
				}
				else
				{
					inst->flags &= ~REFLECTIVE;
				}
			}
		}

		COLL_FIXED_PlayerSearch_NormalizeAxis3(sps, d);

		if ((quad->quadFlags & QUADBLOCK_FLAG_ENGINE_ECHO) != 0)
		{
			d->actionsFlagSet |= ACTION_ENGINE_ECHO;
		}

		d->underDriver = quad;

		if ((d->posCurr.y <= d->quadBlockHeight + 0x1000) || ((quad->terrain_type == TERRAIN_MUD) && (d->posCurr.y < 1)))
		{
			if ((quad->quadFlags & QUADBLOCK_FLAG_GROUND) != 0)
			{
				d->normalVecUP = sps->hit.plane.normal;
				d->collisionFlags |= DRIVER_COLL_FLAG_GROUNDED;
			}

			if (d->currBlockTouching == NULL)
			{
				d->currBlockTouching = quad;
				d->AxisAngle1_normalVec = sps->hit.plane.normal;
			}

			COLL_FIXED_PlayerSearch_UpdateLighting(sps, d, inst);
		}
	}

	if (d->quadBlockHeight + 0x8000 < d->posCurr.y)
	{
		d->terrainMeta2 = VehAfterColl_GetTerrain(TERRAIN_NONE);
	}

	if (d->posCurr.y < CTR_MipsSll(CTR_MipsSubLo(level->ptr_mesh_info->bspRoot->box.min.y, 0x40), 8))
	{
		d->collisionFlags |= DRIVER_COLL_FLAG_MASK_GRAB_REQUEST;
	}

	s32 landingDelta = CTR_MipsSubLo(d->velocity.y, d->ySpeed);

	if ((d->currBlockTouching != NULL) && ((d->collisionFlags & (DRIVER_COLL_FLAG_MASK_GRAB_REQUEST | DRIVER_COLL_FLAG_GROUNDED)) == 0) &&
	    (d->kartState != KS_MASK_GRABBED))
	{
		d->velocity.x = CTR_MipsAddLo(d->velocity.x, CTR_MipsSra(d->AxisAngle1_normalVec.x, 1));
		d->velocity.y = CTR_MipsAddLo(d->velocity.y, CTR_MipsSra(d->AxisAngle1_normalVec.y, 1));
		d->velocity.z = CTR_MipsAddLo(d->velocity.z, CTR_MipsSra(d->AxisAngle1_normalVec.z, 1));
	}

	struct QuadBlock *quad = d->currBlockTouching;

	d->xSpeed = d->velocity.x;
	d->ySpeed = d->velocity.y;
	d->zSpeed = d->velocity.z;

	if (quad == NULL)
	{
		goto DriverAirborne;
	}

	if (COLL_FIXED_PlayerSearch_CheckMaskGrabProgress(d, quad) != 0)
	{
		d->collisionFlags |= DRIVER_COLL_FLAG_MASK_GRAB_REQUEST;
	}

	d->jump_LandingBoost = 0;
	d->actionsFlagSet &= ~(ACTION_AIRBORNE | ACTION_HIGH_JUMP);

	if ((d->collisionFlags & DRIVER_COLL_FLAG_GROUNDED) == 0)
	{
		goto DriverAirborne;
	}

	if ((d->rainCloudEffect == RAIN_CLOUD_EFFECT_ICE_TERRAIN) || ((gGT->gameMode2 & CHEAT_ICY) != 0))
	{
		d->currentTerrain = TERRAIN_ICE;
	}
	else
	{
		u8 terrainType = d->currBlockTouching->terrain_type;

		if ((terrainType != TERRAIN_ICE) && (d->currentTerrain == TERRAIN_ICE))
		{
			d->terrainFrictionTimer = 0xfec0;
		}

		d->currentTerrain = terrainType;
	}

	d->terrainMeta1 = VehAfterColl_GetTerrain(d->currentTerrain);
	d->terrainMeta2 = d->terrainMeta1;
	d->jump_CoyoteTimerMS = COLL_FIXED_PLAYER_SEARCH_COYOTE_TIME_MS;

	{
		u32 actions = d->actionsFlagSet;

		d->actionsFlagSet = actions | ACTION_TOUCH_GROUND;

		if ((d->actionsFlagSetPrevFrame & ACTION_TOUCH_GROUND) == 0)
		{
			s32 absLanding = Coll_MipsAbsS32(landingDelta);

			d->actionsFlagSet = actions | ACTION_TOUCH_GROUND | ACTION_STARTED_TOUCH_GROUND | ACTION_TURBO_INPUT_LATCH;
			d->terrainFrictionTimer = 0x140;

			u32 volume = VehCalc_MapToRange(absLanding, 0x100, 0x3c00, 0x78, 0xfa);

			if (d->kartState != KS_MASK_GRABBED)
			{
				u32 echo = ((d->actionsFlagSet & ACTION_ENGINE_ECHO) != 0);
				OtherFX_Play_LowLevel(7, 1, HowlSfx_Pack(HOWL_SFX_LR_CENTER, HOWL_SFX_DISTORTION_NONE, volume, echo));
			}
		}
	}

	normalBlendWeight = COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_GROUND_WEIGHT;
	goto BlendNormal;

DriverAirborne:
	if (d->jump_CooldownMS != 0)
	{
		d->actionsFlagSet |= ACTION_AIRBORNE;
	}

	if (d->jump_HighJumpTimerMS != 0)
	{
		d->actionsFlagSet |= ACTION_HIGH_JUMP;
	}

	d->terrainMeta1 = VehAfterColl_GetTerrain(TERRAIN_NONE);
	d->currentTerrain = TERRAIN_NONE;
	d->actionsFlagSet &= ~ACTION_TOUCH_GROUND;

	d->jump_LandingBoost = (s16)CTR_MipsAddLo((u16)d->jump_LandingBoost, (u16)gGT->elapsedTimeMS);
	d->jump_CoyoteTimerMS = (s16)CTR_MipsSubLo((u16)d->jump_CoyoteTimerMS, (u16)gGT->elapsedTimeMS);
	if (d->jump_CoyoteTimerMS < 0)
	{
		d->jump_CoyoteTimerMS = 0;
	}

	normalBlendWeight = COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_AIRBORNE_WEIGHT;
	if (d->jump_CoyoteTimerMS == 0)
	{
		d->jump_CooldownMS = 0;
		d->jump_HighJumpTimerMS = 0;
	}

BlendNormal:
{
	s32 invWeight = COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_TOTAL_WEIGHT - normalBlendWeight;
	s32 normalX = CTR_MipsSra(CTR_MipsAddLo(CTR_MipsMulLo(normalBlendWeight, d->AxisAngle2_normalVec.x), CTR_MipsMulLo(invWeight, d->normalVecUP.x)),
	                          COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_SHIFT);
	s32 normalY = CTR_MipsSra(CTR_MipsAddLo(CTR_MipsMulLo(normalBlendWeight, d->AxisAngle2_normalVec.y), CTR_MipsMulLo(invWeight, d->normalVecUP.y)),
	                          COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_SHIFT);
	s32 normalZ = CTR_MipsSra(CTR_MipsAddLo(CTR_MipsMulLo(normalBlendWeight, d->AxisAngle2_normalVec.z), CTR_MipsMulLo(invWeight, d->normalVecUP.z)),
	                          COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_SHIFT);

	if (d->hazardTimer > 0)
	{
		struct CollFixedPlayerTrig trig = COLL_FIXED_PlayerSearch_Trig(CTR_MipsMulLo(d->hazardTimer, COLL_FIXED_PLAYER_SEARCH_HAZARD_WOBBLE_ANGLE_STEP));
		s16 input[4] = {
		    (s16)CTR_MipsSra(CTR_MipsMulLo(trig.x, COLL_FIXED_PLAYER_SEARCH_HAZARD_WOBBLE_AMPLITUDE), COLL_FIXED_PLAYER_SEARCH_HAZARD_WOBBLE_SHIFT), 0,
		    (s16)CTR_MipsSra(CTR_MipsMulLo(trig.z, COLL_FIXED_PLAYER_SEARCH_HAZARD_WOBBLE_AMPLITUDE), COLL_FIXED_PLAYER_SEARCH_HAZARD_WOBBLE_SHIFT), 0};
		s32 output[3];

		CTR_GteLoadS16TripletV0(input);
		gte_rtv0();
		CTR_GteStoreMAC(output);

		normalX = CTR_MipsAddLo(normalX, output[0]);
		normalY = CTR_MipsAddLo(normalY, output[1]);
		normalZ = CTR_MipsAddLo(normalZ, output[2]);
	}

	COLL_FIXED_PlayerSearch_NormalizeAxis2(d, normalX, normalY, normalZ);
}

	{
		struct CollFixedPlayerTrig trig = COLL_FIXED_PlayerSearch_Trig(d->angle);

		s32 numerator = CTR_MipsAddLo(CTR_MipsMulLo(CTR_MipsNegLo(d->AxisAngle2_normalVec.x), trig.z), CTR_MipsMulLo(d->AxisAngle2_normalVec.z, trig.x));
		d->rotCurr.z = ratan2(CTR_MipsSra(numerator, 12), d->AxisAngle2_normalVec.y);
	}

	if (d->hazardTimer < 1)
	{
		if ((d->actionsFlagSet & ACTION_TOUCH_GROUND) != 0)
		{
			s32 speed = Coll_MipsAbsS32(d->speed);

			if (speed > 0x1000)
			{
				s32 screenOffset = Coll_MipsAbsS32((s8)d->Screen_OffsetY);

				if ((screenOffset < 4) && ((d->terrainMeta1->flags & TERRAIN_FLAG_RAISE_GROUND_OFFSET) != 0))
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
		s32 screenOffset = Coll_MipsAbsS32((s8)d->Screen_OffsetY);

		if (screenOffset < 4)
		{
			d->distanceFromGround = 4;

			if ((d->kartState != KS_SPINNING) && ((s8)d->Screen_OffsetY > 0))
			{
				OtherFX_Play(0x10, 1);
			}
		}
	}

UpdateGroundOffset:
	if (Coll_MipsAbsS32((s8)d->Screen_OffsetY) > 9)
	{
		d->distanceFromGround = 0;
	}

	if (d->distanceFromGround == 0)
	{
		s8 nextOffset = (s8)CTR_MipsSubLo((u8)d->Screen_OffsetY, 4);

		if ((s8)d->Screen_OffsetY > 0)
		{
			d->Screen_OffsetY = nextOffset;

			if (nextOffset < 1)
			{
				d->Screen_OffsetY = 0;

				if ((d->terrainMeta1->flags & TERRAIN_FLAG_ONESHOT_GROUND_SOUND) != 0)
				{
					u32 echo = ((d->actionsFlagSet & ACTION_ENGINE_ECHO) != 0);
					u32 soundFlags = HowlSfx_Pack(HOWL_SFX_LR_CENTER, HOWL_SFX_DISTORTION_NONE, 0x80, echo);

					OtherFX_Play_LowLevel(d->terrainMeta1->sound, 0, soundFlags);
				}
			}
		}

		nextOffset = (s8)CTR_MipsSubLo((u8)d->Screen_OffsetY, 4);
		d->Screen_OffsetY = nextOffset;

		if (nextOffset < 0)
		{
			d->Screen_OffsetY = 0;
		}
	}
	else
	{
		d->distanceFromGround--;
		d->Screen_OffsetY = (s8)CTR_MipsAddLo((u8)d->Screen_OffsetY, 3);
	}

	if ((d->posCurr.y < -0x8000) && ((level->configFlags & 2) != 0))
	{
		d->collisionFlags |= DRIVER_COLL_FLAG_MASK_GRAB_REQUEST;
	}

	if ((d->kartState != KS_MASK_GRABBED) && ((d->collisionFlags & DRIVER_COLL_FLAG_MASK_GRAB_REQUEST) != 0) && (d->lastValid != NULL) &&
	    ((sdata->HudAndDebugFlags & 0x1000) == 0) && ((d->stepFlagSet & COLL_STEP_TRIGGER_SUPPRESS_MASK_GRAB) == 0))
	{
		VehStuckProc_MaskGrab_Init(t, d);
	}
}


internal void CollMoved_GteRTV0(void)
{
	doCOP2(0x0486012);
}

internal void CollMoved_GteGPF12(void)
{
	doCOP2(0x0198003d);
}

internal void CollMoved_SelectProjection(CollNormalAxis normalAxis, struct BspSearchResult *candidate, struct BspSearchVertex *v1, struct BspSearchVertex **v2,
                                         struct BspSearchVertex **v3, struct CollTriangleProjection *projection)
{
	if (normalAxis == COLL_NORMAL_AXIS_Y)
	{
		s32 origin = v1->pos.z;
		projection->firstA = (*v2)->pos.z - origin;
		projection->firstB = (*v3)->pos.z - origin;
		projection->firstHit = candidate->pushOut.z - origin;

		if (Coll_MipsAbsS32(projection->firstA) < Coll_MipsAbsS32(projection->firstB))
		{
			s32 tmpValue = projection->firstA;
			projection->firstA = projection->firstB;
			projection->firstB = tmpValue;
			struct BspSearchVertex *tmp = *v2;
			*v2 = *v3;
			*v3 = tmp;
		}

		origin = v1->pos.x;
		projection->secondA = (*v2)->pos.x - origin;
		projection->secondB = (*v3)->pos.x - origin;
		projection->secondHit = candidate->pushOut.x - origin;
	}
	else if (normalAxis == COLL_NORMAL_AXIS_Z)
	{
		s32 origin = v1->pos.x;
		projection->firstA = (*v2)->pos.x - origin;
		projection->firstB = (*v3)->pos.x - origin;
		projection->firstHit = candidate->pushOut.x - origin;

		if (Coll_MipsAbsS32(projection->firstA) < Coll_MipsAbsS32(projection->firstB))
		{
			s32 tmpValue = projection->firstA;
			projection->firstA = projection->firstB;
			projection->firstB = tmpValue;
			struct BspSearchVertex *tmp = *v2;
			*v2 = *v3;
			*v3 = tmp;
		}

		origin = v1->pos.y;
		projection->secondA = (*v2)->pos.y - origin;
		projection->secondB = (*v3)->pos.y - origin;
		projection->secondHit = candidate->pushOut.y - origin;
	}
	else
	{
		s32 origin = v1->pos.y;
		projection->firstA = (*v2)->pos.y - origin;
		projection->firstB = (*v3)->pos.y - origin;
		projection->firstHit = candidate->pushOut.y - origin;

		if (Coll_MipsAbsS32(projection->firstA) < Coll_MipsAbsS32(projection->firstB))
		{
			s32 tmpValue = projection->firstA;
			projection->firstA = projection->firstB;
			projection->firstB = tmpValue;
			struct BspSearchVertex *tmp = *v2;
			*v2 = *v3;
			*v3 = tmp;
		}

		origin = v1->pos.z;
		projection->secondA = (*v2)->pos.z - origin;
		projection->secondB = (*v3)->pos.z - origin;
		projection->secondHit = candidate->pushOut.z - origin;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001f928-0x8001fc40
s32 COLL_MOVED_TRIANGL_ReorderNormals(struct BspSearchResult *candidate, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	struct BspSearchVertex *baryV2 = v2;
	struct BspSearchVertex *baryV3 = v3;
	struct CollTriangleProjection projection;
	s32 baryA = COLL_FRACTION_INVALID;
	s32 baryB = COLL_FRACTION_INVALID;

	CollMoved_SelectProjection(candidate->normalAxis, candidate, v1, &baryV2, &baryV3, &projection);

	if (projection.firstA == 0)
	{
		if (projection.firstB == 0)
		{
			return COLL_TRIANGLE_CLIP_MISS;
		}

		baryB = CTR_MipsDiv(CTR_MipsSll(projection.firstHit, 12), projection.firstB);

		if (projection.secondA != 0)
		{
			baryA = CTR_MipsDiv(CTR_MipsSubLo(CTR_MipsSll(projection.secondHit, 12), CTR_MipsMulLo(baryB, projection.secondB)), projection.secondA);
		}
	}
	else
	{
		s32 denom = CTR_MipsSra(CTR_MipsSubLo(CTR_MipsMulLo(projection.secondB, projection.firstA), CTR_MipsMulLo(projection.firstB, projection.secondA)), 6);

		if (denom != 0)
		{
			baryB = CTR_MipsDiv(
			    CTR_MipsSll(CTR_MipsSubLo(CTR_MipsMulLo(projection.secondHit, projection.firstA), CTR_MipsMulLo(projection.firstHit, projection.secondA)), 6),
			    denom);
			baryA = CTR_MipsDiv(CTR_MipsSubLo(CTR_MipsSll(projection.firstHit, 12), CTR_MipsMulLo(baryB, projection.firstB)), projection.firstA);
		}
	}

	if (baryA == COLL_FRACTION_INVALID)
	{
		return COLL_TRIANGLE_CLIP_MISS;
	}

	s32 sum = CTR_MipsSubLo(CTR_MipsAddLo(baryA, baryB), COLL_FRACTION_ONE);

	if (baryA < 0)
	{
		if (baryB < 0)
		{
			candidate->hitPos = v1->pos;
			return COLL_TRIANGLE_CLIP_V1;
		}

		if (sum >= 0)
		{
			candidate->hitPos = baryV3->pos;
			return COLL_TRIANGLE_CLIP_V3;
		}

		COLL_FIXED_TRIANGL_Barycentrics(&candidate->hitPos, &v1->pos, &baryV3->pos, &candidate->pushOut);
		return COLL_TRIANGLE_CLIP_EDGE_V1_V3;
	}

	if (baryB >= 0)
	{
		if (sum <= 0)
		{
			candidate->hitPos = candidate->pushOut;
			return COLL_TRIANGLE_CLIP_FACE;
		}

		COLL_FIXED_TRIANGL_Barycentrics(&candidate->hitPos, &baryV2->pos, &baryV3->pos, &candidate->pushOut);
		return COLL_TRIANGLE_CLIP_EDGE_V2_V3;
	}

	if (sum >= 0)
	{
		candidate->hitPos = baryV2->pos;
		return COLL_TRIANGLE_CLIP_V2;
	}

	COLL_FIXED_TRIANGL_Barycentrics(&candidate->hitPos, &v1->pos, &baryV2->pos, &candidate->pushOut);
	return COLL_TRIANGLE_CLIP_EDGE_V1_V2;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001fc40-0x80020064
void COLL_MOVED_TRIANGL_TestPoint(struct ScratchpadStruct *sps, struct BspSearchVertex *v1, struct BspSearchVertex *v2, struct BspSearchVertex *v3)
{
	s32 usedSegmentProjection;

	sps->numTrianglesTested = (s16)CTR_MipsAddLo(sps->numTrianglesTested, 1);
	sps->candidate.normalAxis = v1->normalAxis;
	sps->candidate.plane = v1->plane;

	struct QuadBlock *quad = sps->candidate.ptrQuadblock;
	s32 normalZW = (s32)CTR_PackS16Pair(sps->candidate.plane.normal.z, sps->candidate.plane.halfDistance);

	if (((quad->quadFlags & QUADBLOCK_FLAG_DOOR) != 0) && (((s32)(s8)quad->terrain_type & sdata->doorAccessFlags) != 0))
	{
		return;
	}

	CollFixed_GteLoadR11R12(CTR_PackS16Pair(sps->Input1.pos.x, sps->Input1.pos.y));
	CollFixed_GteLoadR13R21(CTR_PackS16Pair(sps->Input1.pos.z, sps->Union.QuadBlockColl.pos.x));
	CollFixed_GteLoadR22R23(CTR_PackS16Pair(sps->Union.QuadBlockColl.pos.y, sps->Union.QuadBlockColl.pos.z));
	CollFixed_GteLoadVXY0(CTR_PackS16Pair(sps->candidate.plane.normal.x, sps->candidate.plane.normal.y));
	CollFixed_GteLoadVZ0(normalZW);
	CollMoved_GteRTV0();

	s32 halfDistance = sps->candidate.plane.halfDistance;
	s32 planeOffset = CTR_MipsSll(halfDistance, 1);
	s32 planeNear = CTR_MipsSubLo(CollFixed_GteReadMAC1(), planeOffset);
	s32 planeFar = CTR_MipsSubLo(CollFixed_GteReadMAC2(), planeOffset);

	if (planeFar < 0)
	{
		if (((quad->quadFlags & QUADBLOCK_FLAG_TRIGGER) == 0) && ((quad->draw_order_low & QUADBLOCK_DRAW_ORDER_LOW_DOUBLE_SIDED) == 0))
		{
			goto KeepNormal;
		}

		planeNear = CTR_MipsNegLo(planeNear);
		planeFar = CTR_MipsNegLo(planeFar);
		sps->candidate.plane.normal.x = (s16)CTR_MipsNegLo(sps->candidate.plane.normal.x);
		sps->candidate.plane.normal.y = (s16)CTR_MipsNegLo(sps->candidate.plane.normal.y);
		sps->candidate.plane.normal.z = (s16)CTR_MipsNegLo(sps->candidate.plane.normal.z);
		sps->candidate.plane.halfDistance = (s16)CTR_MipsNegLo(sps->candidate.plane.halfDistance);
	}

KeepNormal:;
	u16 quadFlags = quad->quadFlags;
	sps->numTrianglesTested = (s16)CTR_MipsAddLo(sps->numTrianglesTested, 1);

	if (CTR_MipsSubLo(planeNear, sps->Input1.hitRadius) >= 0)
	{
		return;
	}

	if (planeFar < 0)
	{
		return;
	}

	if (((quadFlags & QUADBLOCK_FLAG_TRIGGER) == 0) && (CTR_MipsSubLo(planeNear, planeFar) > 0))
	{
		return;
	}

	if (planeNear >= 0)
	{
		CollFixed_GteLoadIR0(planeNear);
		CollFixed_GteLoadIR(sps->candidate.plane.normal.x, sps->candidate.plane.normal.y, sps->candidate.plane.normal.z);
		usedSegmentProjection = 0;
	}
	else
	{
		CollFixed_GteLoadIR(CTR_MipsSubLo(sps->Input1.pos.x, sps->Union.QuadBlockColl.pos.x), CTR_MipsSubLo(sps->Input1.pos.y, sps->Union.QuadBlockColl.pos.y),
		                    CTR_MipsSubLo(sps->Input1.pos.z, sps->Union.QuadBlockColl.pos.z));
		CollFixed_GteLoadIR0(CTR_MipsDiv(CTR_MipsSll(CTR_MipsNegLo(planeNear), 12), CTR_MipsSubLo(planeFar, planeNear)));
		usedSegmentProjection = 1;
	}

	CollMoved_GteGPF12();
	s32 hitX = CollFixed_GteReadMAC1();
	s32 hitY = CollFixed_GteReadMAC2();
	s32 hitZ = CollFixed_GteReadMAC3();

	CTR_SET_VEC3(sps->candidate.pushOut.v, (s16)CTR_MipsSubLo(sps->Input1.pos.x, hitX), (s16)CTR_MipsSubLo(sps->Input1.pos.y, hitY),
	             (s16)CTR_MipsSubLo(sps->Input1.pos.z, hitZ));

	sps->hitBspSearchTriangle.v0 = v1;
	sps->hitBspSearchTriangle.v1 = v2;
	sps->hitBspSearchTriangle.v2 = v3;

	s32 reorderResult = COLL_MOVED_TRIANGL_ReorderNormals(&sps->candidate, v1, v2, v3);
	if (reorderResult < 0)
	{
		return;
	}

	if (usedSegmentProjection != 0)
	{
		sps->candidateDelta.x = (s16)CTR_MipsSubLo(sps->candidate.pushOut.x, sps->candidate.hitPos.x);
		sps->candidateDelta.y = (s16)CTR_MipsSubLo(sps->candidate.pushOut.y, sps->candidate.hitPos.y);
		sps->candidateDelta.z = (s16)CTR_MipsSubLo(sps->candidate.pushOut.z, sps->candidate.hitPos.z);
	}
	else
	{
		sps->candidateDelta.x = (s16)CTR_MipsSubLo(sps->Input1.pos.x, sps->candidate.hitPos.x);
		sps->candidateDelta.y = (s16)CTR_MipsSubLo(sps->Input1.pos.y, sps->candidate.hitPos.y);
		sps->candidateDelta.z = (s16)CTR_MipsSubLo(sps->Input1.pos.z, sps->candidate.hitPos.z);
	}

	CollFixed_GteLoadR11R12(CTR_PackS16Pair(sps->candidateDelta.x, sps->candidateDelta.y));
	CollFixed_GteLoadR13R21(sps->candidateDelta.z);
	CollFixed_GteLoadVXY0(CTR_PackS16Pair(sps->candidateDelta.x, sps->candidateDelta.y));
	CollFixed_GteLoadVZ0(sps->candidateDelta.z);
	CollFixed_GteMVMVA();
	s32 distanceSq = CollFixed_GteReadMAC1();

	if (CTR_MipsSubLo(distanceSq, sps->Input1.hitRadiusSquared) > 0)
	{
		return;
	}

	if ((quadFlags & QUADBLOCK_FLAG_TRIGGER) != 0)
	{
		if ((planeNear < 0) || ((CTR_MipsSubLo(planeNear, sps->Input1.hitRadius) | CTR_MipsSubLo(planeFar, sps->Input1.hitRadius)) < 0))
		{
			sps->collision.stepFlags |= (u8)quad->terrain_type;
			return;
		}
	}

	s32 distance = CTR_MipsSubLo(planeFar, planeNear);
	if (distance != 0)
	{
		distance = CTR_MipsSubLo(COLL_FRACTION_ONE, CTR_MipsDiv(CTR_MipsSll(CTR_MipsSubLo(sps->Input1.hitRadius, planeNear), 12), distance));
	}

	if (CTR_MipsSubLo(distance, sps->hitFraction) >= 0)
	{
		return;
	}

	if ((quadFlags & QUADBLOCK_FLAG_NO_COLLISION_RESPONSE) != 0)
	{
		if ((quadFlags & QUADBLOCK_FLAG_KILL_PLANE) != 0)
		{
			sps->collision.stepFlags |= COLL_STEP_FLAG_KILL_PLANE;
		}

		return;
	}

	sps->hitFraction = distance;
	sps->hitLevelTriangle.v0 = v1->pLevelVertex;
	sps->hitLevelTriangle.v1 = v2->pLevelVertex;
	sps->hitLevelTriangle.v2 = v3->pLevelVertex;

	sps->hit.hitPos = sps->candidate.hitPos;
	sps->hit.normalAxis = sps->candidate.normalAxis;
	sps->hit.plane = sps->candidate.plane;
	sps->hit.pushOut = sps->candidate.pushOut;
	sps->hit.ptrQuadblock = quad;

	sps->hit.triangleID = sps->candidate.triangleID;
	sps->hit.reorderResult = (CollTriangleClipResult)reorderResult;

	if (distance <= 0)
	{
		sps->Union.QuadBlockColl.hitPos = sps->Union.QuadBlockColl.pos;
	}
	else
	{
		CollFixed_GteLoadIR0(distance);
		CollFixed_GteLoadIR(CTR_MipsSubLo(sps->Input1.pos.x, sps->Union.QuadBlockColl.pos.x), CTR_MipsSubLo(sps->Input1.pos.y, sps->Union.QuadBlockColl.pos.y),
		                    CTR_MipsSubLo(sps->Input1.pos.z, sps->Union.QuadBlockColl.pos.z));
		CollMoved_GteGPF12();

		sps->Union.QuadBlockColl.hitPos.x = (s16)CTR_MipsAddLo(sps->Union.QuadBlockColl.pos.x, CollFixed_GteReadMAC1());
		sps->Union.QuadBlockColl.hitPos.y = (s16)CTR_MipsAddLo(sps->Union.QuadBlockColl.pos.y, CollFixed_GteReadMAC2());
		sps->Union.QuadBlockColl.hitPos.z = (s16)CTR_MipsAddLo(sps->Union.QuadBlockColl.pos.z, CollFixed_GteReadMAC3());
	}

	sps->boolDidTouchQuadblock = (s16)CTR_MipsAddLo(sps->boolDidTouchQuadblock, 1);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80020064-0x800202a8
void COLL_MOVED_QUADBLK_TestTriangles(struct QuadBlock *quad, struct ScratchpadStruct *sps)
{
	struct BspSearchVertex *bsv = &sps->bspSearchVert[0];
	b32 hasSecondTriangle = quad->index[2] != quad->index[3];

	sps->candidate.ptrQuadblock = quad;

	if (((sps->Union.QuadBlockColl.quadFlagsWanted & quad->quadFlags) == 0) || ((sps->Union.QuadBlockColl.quadFlagsIgnored & quad->quadFlags) != 0) ||
	    (quad->bbox.min.x > sps->bbox.max.x) || (quad->bbox.min.y > sps->bbox.max.y) || (quad->bbox.min.z > sps->bbox.max.z) ||
	    (sps->bbox.min.x > quad->bbox.max.x) || (sps->bbox.min.y > quad->bbox.max.y) || (sps->bbox.min.z > quad->bbox.max.z))
	{
		return;
	}

	// if 3P or 4P mode,
	// then use low-LOD quadblock collision (two triangles)
	if ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_HIGH_LOD) == 0)
	{
		COLL_FIXED_QUADBLK_GetNormVecs_LoLOD(sps, quad);

		sps->candidate.triangleID = COLL_QUAD_TRIANGLE_LO_0;
		COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[0], &bsv[1], &bsv[2]); // 0, 1, 2

		sps->candidate.triangleID = COLL_QUAD_TRIANGLE_LO_1;

		if (hasSecondTriangle)
		{
			COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[1], &bsv[3], &bsv[2]); // 1, 3, 2
		}
	}
	else
	{
		if ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_REUSE_NORMALS) == 0)
		{
			COLL_FIXED_QUADBLK_GetNormVecs_HiLOD(sps, quad);
		}

		sps->candidate.triangleID = COLL_QUAD_TRIANGLE_HI_0;
		COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[0], &bsv[4], &bsv[5]); // 0, 4, 5
		sps->candidate.triangleID = COLL_QUAD_TRIANGLE_HI_1;
		COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[4], &bsv[6], &bsv[5]); // 4, 6, 5
		sps->candidate.triangleID = COLL_QUAD_TRIANGLE_HI_2;
		COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[6], &bsv[4], &bsv[1]); // 6, 4, 1
		sps->candidate.triangleID = COLL_QUAD_TRIANGLE_HI_3;
		COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[5], &bsv[6], &bsv[2]); // 5, 6, 2

		sps->candidate.triangleID = COLL_QUAD_TRIANGLE_HI_4;

		if (hasSecondTriangle)
		{
			COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[8], &bsv[6], &bsv[7]); // 8, 6, 7
			sps->candidate.triangleID = COLL_QUAD_TRIANGLE_HI_5;
			COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[7], &bsv[3], &bsv[8]); // 7, 3, 8
			sps->candidate.triangleID = COLL_QUAD_TRIANGLE_HI_6;
			COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[1], &bsv[7], &bsv[6]); // 1, 7, 6
			sps->candidate.triangleID = COLL_QUAD_TRIANGLE_HI_7;
			COLL_MOVED_TRIANGL_TestPoint(sps, &bsv[2], &bsv[6], &bsv[8]); // 2, 6, 8
		}
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800202a8-0x80020334
void COLL_MOVED_BSPLEAF_TestQuadblocks(struct BSP *node, struct ScratchpadStruct *sps)
{
	// if bsp flag is water
	if ((node->flag & BSP_LEAF_FLAG_WATER) != 0)
	{
		sps->collision.stepFlags |= COLL_STEP_FLAG_WATER_BSP;
	}

	s32 numQuads = node->data.leaf.numQuads;
	struct QuadBlock *ptrQuad = node->data.leaf.ptrQuadBlockArray;

	// loop through all quadblocks
	do
	{
		COLL_MOVED_QUADBLK_TestTriangles(ptrQuad++, sps);
		numQuads--;
	} while (numQuads > 0);

	if ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_TEST_INSTANCES) != 0)
	{
		COLL_FIXED_BSPLEAF_TestInstance(node, sps);
	}
}

enum
{
	COLL_SCRUB_DEPTH_REPEAT_STEP = 0x100,
	COLL_SCRUB_DEPTH_REPEAT_LIMIT = 0x401,
	COLL_HITBOX_SCRUB_DEPTH_BONUS = 0x200,
	COLL_MOVED_PLAYER_HIT_RADIUS = 0x19,
};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80020334-0x80020410
void COLL_MOVED_FindScrub(struct QuadBlock *qb, s32 triangleID, struct ScratchpadStruct *sps)
{
	struct ScratchpadStructExtended *ext = (struct ScratchpadStructExtended *)sps;
	u16 searchFlags = sps->Union.QuadBlockColl.searchFlags;

	if (qb == NULL)
	{
		sps->Union.QuadBlockColl.searchFlags = searchFlags & ~COLL_SEARCH_REPEAT_SCRUB;
		sps->Input1.scrubDepth = 0;
		ext->numTriangles = 0;
		return;
	}

	for (s32 i = ext->numTriangles - 1; i >= 0; i--)
	{
		struct BspSearchTriangle *tri = &ext->bspSearchTriangle[i];

		if ((tri->quadblock == qb) && (tri->triangleID == triangleID))
		{
			s32 scrubDepth = tri->scrubDepth;
			s16 scrub = scrubDepth;

			if (scrubDepth < COLL_SCRUB_DEPTH_REPEAT_LIMIT)
			{
				scrubDepth = CTR_MipsAddLo(scrubDepth, COLL_SCRUB_DEPTH_REPEAT_STEP);
				scrub = scrubDepth;
				tri->scrubDepth = scrubDepth;
			}

			sps->Union.QuadBlockColl.searchFlags = searchFlags | COLL_SEARCH_REPEAT_SCRUB;
			sps->Input1.scrubDepth = scrub;
			return;
		}
	}

	{
		struct BspSearchTriangle *tri = &ext->bspSearchTriangle[ext->numTriangles];

		tri->quadblock = qb;
		tri->triangleID = triangleID;
		tri->scrubDepth = 0;
	}

	sps->Union.QuadBlockColl.searchFlags = searchFlags & ~COLL_SEARCH_REPEAT_SCRUB;
	sps->Input1.scrubDepth = 0;
	ext->numTriangles++;
}


internal s32 CollMoved_PlayerSearch_StepVelocity(s32 velocity, s32 elapsedTimeMS, s32 multiplier)
{
	return CTR_MipsSra(CTR_MipsMulLo(CTR_MipsSra(CTR_MipsMulLo(velocity, elapsedTimeMS), 5), multiplier), 12);
}

internal void CollMoved_PlayerSearch_SetBBoxAxis(struct ScratchpadStruct *sps, s32 axis, s16 current, s16 next)
{
	s16 radius = sps->Input1.hitRadius;
	s32 minCurrent = CTR_MipsSubLo(current, radius);
	s32 minNext = CTR_MipsSubLo(next, radius);
	s32 maxCurrent = CTR_MipsAddLo(current, radius);
	s32 maxNext = CTR_MipsAddLo(next, radius);

	sps->bbox.min.v[axis] = (minNext < minCurrent) ? minNext : minCurrent;
	sps->bbox.max.v[axis] = (maxCurrent < maxNext) ? maxNext : maxCurrent;
}

internal int CollMoved_PlayerSearch_RunHitboxLInC(struct ScratchpadStruct *sps, struct Thread *t)
{
	struct BSP *bsp = sps->bspHitbox;
	struct Instance *linCInstance;
	s16 modelID;

	if ((bsp->flag & BSP_HITBOX_COLLIDABLE) != 0)
	{
		struct InstDef *instDef = bsp->data.hitbox.instDef;
		if (instDef == NULL)
		{
			return 1;
		}

		linCInstance = instDef->ptrInstance;
		if (linCInstance == NULL)
		{
			return 1;
		}

		if ((linCInstance->flags & DRAW_COLLISION_MASK) == 0)
		{
			return 1;
		}

		modelID = instDef->modelID;
	}
	else
	{
		if ((bsp->flag & BSP_HITBOX_LINC_USES_INSTDEF) == 0)
		{
			return 1;
		}

		struct InstDef *instDef = bsp->data.hitbox.instDef;
		if (instDef == NULL)
		{
			return 1;
		}

		// Retail passes the InstDef pointer for these hitboxes, not ptrInstance.
		linCInstance = (struct Instance *)instDef;
		modelID = instDef->model->id;
	}

	struct MetaDataMODEL *meta = COLL_LevModelMeta(modelID);
	if ((meta != NULL) && (meta->LInC != NULL))
	{
		return meta->LInC(linCInstance, t, sps);
	}

	return 1;
}

internal void CollMoved_PlayerSearch_StoreHitbox(struct ScratchpadStruct *sps)
{
	sps->bspHitboxesHit[sps->numBspHitboxesHit] = sps->bspHitbox;
	sps->numBspHitboxesHit++;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80020410-0x80020c58
void COLL_MOVED_PlayerSearch(struct Thread *t, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
	s32 multiplier = COLL_FRACTION_ONE;
	s16 hitRadius = COLL_MOVED_PLAYER_HIT_RADIUS;

	sps->Input1.hitRadius = hitRadius;
	sps->Input1.hitRadiusSquared = hitRadius * hitRadius;
	sps->Union.QuadBlockColl.hitRadius = hitRadius;
	sps->Union.QuadBlockColl.hitRadiusSquared = hitRadius * hitRadius;
	sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND | QUADBLOCK_FLAG_COLLISION_SURFACE;
	sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
	sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES;
	sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;

	if (gGT->numPlyrCurrGame < 3)
	{
		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES | COLL_SEARCH_HIGH_LOD;
	}

	sps->numBspHitboxesHit = 0;
	sps->Input1.modelID = DYNAMIC_PLAYER;
	sps->collision.stepFlags = 0;

	COLL_MOVED_FindScrub(NULL, 0, sps);

	for (s32 iterations = 15; iterations != 0; iterations--)
	{
		Vec3 velocity = {
		    .x = CollMoved_PlayerSearch_StepVelocity(d->velocity.x, gGT->elapsedTimeMS, multiplier),
		    .y = CollMoved_PlayerSearch_StepVelocity(d->velocity.y, gGT->elapsedTimeMS, multiplier),
		    .z = CollMoved_PlayerSearch_StepVelocity(d->velocity.z, gGT->elapsedTimeMS, multiplier),
		};

		sps->boolDidTouchQuadblock = 0;
		sps->numTrianglesTested = 0;
		sps->boolDidTouchHitbox = 0;
		sps->resetOnly40 = 0;
		sps->Union.QuadBlockColl.searchFlags |= COLL_SEARCH_TEST_INSTANCES;
		sps->hitFraction = COLL_FRACTION_ONE;

		// Retail uses addu/sra/lhu/sh for these sweep points.
		SVec3 current = {
		    .x = (s16)CTR_MipsAddLo((u16)d->originToCenter.x, CTR_MipsSra(d->posCurr.x, 8)),
		    .y = (s16)CTR_MipsAddLo((u16)d->originToCenter.y, CTR_MipsSra(d->posCurr.y, 8)),
		    .z = (s16)CTR_MipsAddLo((u16)d->originToCenter.z, CTR_MipsSra(d->posCurr.z, 8)),
		};

		SVec3 next = {
		    .x = (s16)CTR_MipsAddLo((u16)d->originToCenter.x, CTR_MipsSra(CTR_MipsAddLo(d->posCurr.x, velocity.x), 8)),
		    .y = (s16)CTR_MipsAddLo((u16)d->originToCenter.y, CTR_MipsSra(CTR_MipsAddLo(d->posCurr.y, velocity.y), 8)),
		    .z = (s16)CTR_MipsAddLo((u16)d->originToCenter.z, CTR_MipsSra(CTR_MipsAddLo(d->posCurr.z, velocity.z), 8)),
		};

		sps->Union.QuadBlockColl.pos = current;
		sps->Input1.pos = next;

		if ((next.x == current.x) && (next.y == current.y) && (next.z == current.z))
		{
			break;
		}

		CollMoved_PlayerSearch_SetBBoxAxis(sps, 0, current.x, next.x);
		CollMoved_PlayerSearch_SetBBoxAxis(sps, 1, current.y, next.y);
		CollMoved_PlayerSearch_SetBBoxAxis(sps, 2, current.z, next.z);

		sps->Union.QuadBlockColl.hitPos = sps->Input1.pos;

		sps->Union.QuadBlockColl.searchFlags = (sps->Union.QuadBlockColl.searchFlags | COLL_SEARCH_TEST_INSTANCES) & ~COLL_SEARCH_REUSE_NORMALS;

		if ((gGT->level1 != NULL) && (gGT->level1->ptr_mesh_info != NULL) && (gGT->level1->ptr_mesh_info->bspRoot != NULL))
		{
			COLL_SearchBSP_CallbackPARAM(gGT->level1->ptr_mesh_info->bspRoot, &sps->bbox, COLL_MOVED_BSPLEAF_TestQuadblocks, sps);
		}

		if (sps->boolDidTouchQuadblock != 0)
		{
			d->collisionFlags |= DRIVER_COLL_FLAG_TOUCHED_QUADBLOCK;
		}

		if (sps->hitFraction > 0)
		{
			d->posCurr.x = CTR_MipsAddLo(d->posCurr.x, CTR_MipsSra(CTR_MipsMulLo(velocity.x, sps->hitFraction), 12));
			d->posCurr.y = CTR_MipsAddLo(d->posCurr.y, CTR_MipsSra(CTR_MipsMulLo(velocity.y, sps->hitFraction), 12));
			d->posCurr.z = CTR_MipsAddLo(d->posCurr.z, CTR_MipsSra(CTR_MipsMulLo(velocity.z, sps->hitFraction), 12));
		}

		if (sps->boolDidTouchHitbox != 0)
		{
			struct BSP *bspHitbox = sps->bspHitbox;

			sps->Union.QuadBlockColl.searchFlags &= ~COLL_SEARCH_REUSE_NORMALS;
			d->collisionFlags &= ~DRIVER_COLL_FLAG_SURFACE_PUSHBACK;

			s32 hitboxResult = CollMoved_PlayerSearch_RunHitboxLInC(sps, t);
			u8 hitboxId = Coll_BspHitboxClass(bspHitbox);

			if ((hitboxResult == 2) || (hitboxId == 4))
			{
				CollMoved_PlayerSearch_StoreHitbox(sps);
			}
			else
			{
				COLL_MOVED_FindScrub((struct QuadBlock *)bspHitbox, 0, sps);
				sps->Input1.scrubDepth = (s16)CTR_MipsAddLo((u16)sps->Input1.scrubDepth, COLL_HITBOX_SCRUB_DEPTH_BONUS);

				struct Scrub *scrub = VehAfterColl_GetSurface(hitboxId);
				hitboxResult = COLL_MOVED_ScrubImpact(d, t, sps, scrub, &d->velocity);

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
			if (sps->boolDidTouchQuadblock == 0)
			{
				break;
			}

			struct QuadBlock *quad = sps->hit.ptrQuadblock;

			if ((quad->quadFlags & QUADBLOCK_FLAG_KILL_PLANE) != 0)
			{
				d->collisionFlags |= DRIVER_COLL_FLAG_MASK_GRAB_REQUEST;
			}

			COLL_MOVED_FindScrub(quad, sps->hit.triangleID, sps);

			u32 scrubId;
			if ((quad->quadFlags & QUADBLOCK_FLAG_GROUND) == 0)
			{
				scrubId = ((quad->quadFlags & QUADBLOCK_FLAG_REFLECT_SPLIT_LINE_1) == 0) ? 0 : 6;
			}
			else
			{
				if ((quad != d->underDriver) && ((quad->quadFlags & 8) != 0))
				{
					d->underDriver = NULL;
				}

				d->currBlockTouching = quad;
				d->normalVecUP = sps->hit.plane.normal;
				d->AxisAngle1_normalVec = sps->hit.plane.normal;
				d->collisionFlags |= DRIVER_COLL_FLAG_GROUNDED;
				scrubId = 5;
			}

			struct Scrub *scrub = VehAfterColl_GetSurface(scrubId);
			d->collisionFlags |= DRIVER_COLL_FLAG_SURFACE_PUSHBACK;
			d->spsHitPos = sps->hit.hitPos;
			d->spsNormalVec = sps->hit.plane.normal;

			u32 impactResult = COLL_MOVED_ScrubImpact(d, t, sps, scrub, &d->velocity);
			if (impactResult == 2)
			{
				return;
			}

			if (sps->hitFraction > 0)
			{
				multiplier = CTR_MipsSubLo(multiplier, CTR_MipsSra(CTR_MipsMulLo(multiplier, sps->hitFraction), 12));
				if (multiplier < 100)
				{
					break;
				}
			}

			sps->Union.QuadBlockColl.searchFlags |= COLL_SEARCH_REUSE_NORMALS;
		}
	}

	d->stepFlagSet = sps->collision.stepFlags;
}


internal s32 CollMoved_ScrubImpact_TrigX(s32 angle)
{
	struct TrigTable trig = data.trigApprox[ANG_MODULO_HALF_PI(angle)];
	s32 value = IS_ANG_FIRST_OR_THIRD_QUADRANT(angle) ? trig.sin : trig.cos;

	if (IS_ANG_THIRD_OR_FOURTH_QUADRANT(angle))
	{
		value = CTR_MipsNegLo(value);
	}

	return value;
}

internal Vec3 CollMoved_ScrubImpact_GteLLV0(s32 x, s32 y, s32 z)
{
	MTC2(CTR_PackS16Pair(x, y), 0);
	MTC2(z, 1);
	doCOP2(0x04a6012);

	Vec3 out = {
	    .x = MFC2_S(25),
	    .y = MFC2_S(26),
	    .z = MFC2_S(27),
	};
	return out;
}

internal void CollMoved_ScrubImpact_GteOP12(void)
{
	doCOP2(0x0178000c);
}

internal void CollMoved_ScrubImpact_ProjectWallVelocity(const SVec3 *normal, s32 oldVelX, s32 oldVelZ, Vec3 *out)
{
	MTC2(oldVelX, 9);
	MTC2(0, 10);
	MTC2(oldVelZ, 11);
	CTC2(normal->x, 0);
	CTC2(normal->y, 2);
	CTC2(normal->z, 4);

	CollMoved_ScrubImpact_GteOP12();

	u32 r11 = CFC2(0);
	u32 r22 = CFC2(2);
	u32 r33 = CFC2(4);

	MTC2(r11, 9);
	MTC2(r22, 10);
	MTC2(r33, 11);

	s32 crossX = MFC2_S(25);
	s32 crossY = MFC2_S(26);
	s32 crossZ = MFC2_S(27);

	CTC2(crossX, 0);
	CTC2(crossY, 2);
	CTC2(crossZ, 4);

	CollMoved_ScrubImpact_GteOP12();

	out->x = MFC2_S(25);
	out->y = MFC2_S(26);
	out->z = MFC2_S(27);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80020c58-0x80021500
u32 COLL_MOVED_ScrubImpact(struct Driver *d, struct Thread *t, struct ScratchpadStruct *sps, struct Scrub *scrub, Vec3 *velocity)
{
	SVec3 normal = sps->hit.plane.normal;

	if ((d->vShiftCount != 0) && (sps->boolDidTouchQuadblock != 0) && ((sps->hit.ptrQuadblock->quadFlags & QUADBLOCK_FLAG_GROUND) != 0) &&
	    (sps->hit.reorderResult != COLL_TRIANGLE_CLIP_FACE) && (sps->hit.ptrQuadblock != d->underDriver))
	{
		if ((Coll_MipsAbsS32(d->speedApprox) < 0x300) && (Coll_MipsAbsS32(d->jumpHeightCurr) < 0x300) && (d->fireSpeed == 0))
		{
			s32 diffX = CTR_MipsSubLo(CTR_MipsSra(d->posCurr.x, 8), sps->hit.hitPos.x);
			s32 diffZ = CTR_MipsSubLo(CTR_MipsSra(d->posCurr.z, 8), sps->hit.hitPos.z);
			s32 diffY = CTR_MipsSubLo(CTR_MipsSra(d->posCurr.y, 8), sps->hit.hitPos.y);

			if ((diffX | diffY | diffZ) != 0)
			{
				s32 len = VehCalc_FastSqrt((u32)CTR_MipsMulLo(diffX, diffX) + (u32)CTR_MipsMulLo(diffY, diffY) + (u32)CTR_MipsMulLo(diffZ, diffZ), 0);

				normal.x = (s16)CTR_MipsDiv(CTR_MipsSll(diffX, 12), len);
				normal.y = (s16)CTR_MipsDiv(CTR_MipsSll(diffY, 12), len);
				normal.z = (s16)CTR_MipsDiv(CTR_MipsSll(diffZ, 12), len);
			}
		}
	}

	s32 dot =
	    CTR_MipsSra(CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(CTR_MipsSra(velocity->x, 3), normal.x), CTR_MipsMulLo(CTR_MipsSra(velocity->y, 3), normal.y)),
	                              CTR_MipsMulLo(CTR_MipsSra(velocity->z, 3), normal.z)),
	                9);

	if (dot < -0xa00)
	{
		d->actionsFlagSet |= ACTION_TURBO_INPUT_LATCH;
	}

	dot = CTR_MipsSubLo(dot, sps->Input1.scrubDepth);

	u32 ret = 0;
	if (dot < 0)
	{
		u32 scrubFlags = scrub->flags;

		if ((scrubFlags & SCRUB_FLAG_SKIP_WALL_RUB_TIMER) == 0)
		{
			d->actionsFlagSet |= ACTION_DRIVING_AGAINST_WALL;
		}

		if ((scrubFlags & SCRUB_FLAG_KEEP_RESERVES) == 0)
		{
			d->reserves = 0;
			d->turbo_outsideTimer = 0;
		}

		s32 scrubSpeed = scrub->speedLimit;

		if (!((d->wallRubTimer == 0) ? (0x3e7ff < scrubSpeed) : (d->wallRubSpeedLimit < scrubSpeed)))
		{
			d->wallRubTimer = 0xf0;
			d->wallRubSpeedLimit = scrubSpeed;
			d->posWallColl = sps->hit.hitPos;
		}

		ret = 0;

		if ((scrubFlags & SCRUB_FLAG_APPLY_IMPACT) != 0)
		{
			Vec3 impact = {
			    .x = CTR_MipsSra(CTR_MipsMulLo(dot, normal.x), 12),
			    .y = CTR_MipsSra(CTR_MipsMulLo(dot, normal.y), 12),
			    .z = CTR_MipsSra(CTR_MipsMulLo(dot, normal.z), 12),
			};
			s32 speedSq = 0;

			if (scrub->impactAngle != 0)
			{
				speedSq = CTR_MipsSra(CTR_MipsAddLo(CTR_MipsAddLo(CTR_MipsMulLo(velocity->x, velocity->x), CTR_MipsMulLo(velocity->y, velocity->y)),
				                                    CTR_MipsMulLo(velocity->z, velocity->z)),
				                      15);
			}

			s32 oldVelX = velocity->x;
			s32 oldVelZ = velocity->z;
			velocity->x = CTR_MipsSubLo(oldVelX, impact.x);
			velocity->z = CTR_MipsSubLo(oldVelZ, impact.z);
			velocity->y = CTR_MipsSubLo(velocity->y, impact.y);

			impact = CollMoved_ScrubImpact_GteLLV0(impact.x, impact.y, impact.z);

			if ((sps->boolDidTouchQuadblock != 0) && ((sps->Union.QuadBlockColl.searchFlags & COLL_SEARCH_WALL_PROJECTION_DONE) == 0) &&
			    ((d->actionsFlagSetPrevFrame & ACTION_TOUCH_GROUND) == 0) && ((sps->hit.ptrQuadblock->quadFlags & QUADBLOCK_FLAG_GROUND) != 0))
			{
				Vec3 wallVelocity;
				CollMoved_ScrubImpact_ProjectWallVelocity(&normal, oldVelX, oldVelZ, &wallVelocity);

				u32 wallSpeedSq = (u32)CTR_MipsMulLo(wallVelocity.x, wallVelocity.x) + (u32)CTR_MipsMulLo(wallVelocity.y, wallVelocity.y) +
				                  (u32)CTR_MipsMulLo(wallVelocity.z, wallVelocity.z);
				u32 wallSpeed = VehCalc_FastSqrt(wallSpeedSq, 0x10) >> 8;
				s32 speedApprox = d->speedApprox;

				if ((wallSpeed != 0) && (speedApprox > 0))
				{
					sps->Union.QuadBlockColl.searchFlags |= COLL_SEARCH_WALL_PROJECTION_DONE;
					velocity->x = CTR_MipsDiv(CTR_MipsMulLo(wallVelocity.x, speedApprox), (s32)wallSpeed);
					velocity->y = CTR_MipsDiv(CTR_MipsMulLo(wallVelocity.y, speedApprox), (s32)wallSpeed);
					velocity->z = CTR_MipsDiv(CTR_MipsMulLo(wallVelocity.z, speedApprox), (s32)wallSpeed);
					velocity->x = CTR_MipsSubLo(velocity->x, CTR_MipsSra(normal.x, 1));
					velocity->y = CTR_MipsSubLo(velocity->y, CTR_MipsSra(normal.y, 1));
					velocity->z = CTR_MipsSubLo(velocity->z, CTR_MipsSra(normal.z, 1));
				}
			}

			s32 transformedImpactXZ = CTR_MipsAddLo(CTR_MipsMulLo(impact.x, impact.x), CTR_MipsMulLo(impact.z, impact.z));

			if (((scrubFlags & SCRUB_FLAG_SLAM_ON_HARD_IMPACT) != 0) && (dot < -0x13ff) && (transformedImpactXZ > 0x1900000))
			{
				if (d->kartState != KS_MASK_GRABBED)
				{
					GAMEPAD_JogCon1(d, (d->simpTurnState < 1) ? 0x1f : 0x2f, 0x60);
				}

				if (scrub->impactAngle != 0)
				{
					s32 trig = CollMoved_ScrubImpact_TrigX(scrub->impactAngle);
					s32 scaledSpeed = CTR_MipsSra(CTR_MipsMulLo(speedSq, trig), 12);
					s32 angleLimit = CTR_MipsSra(CTR_MipsMulLo(scaledSpeed, trig), 12);
					s32 dotSq = CTR_MipsSra(CTR_MipsMulLo(dot, dot), 15);

					if (angleLimit >= dotSq)
					{
						return 1;
					}
				}

				if ((d->kartState != KS_MASK_GRABBED) && (transformedImpactXZ > 0x1900000))
				{
					u32 echo = ((d->actionsFlagSet & ACTION_ENGINE_ECHO) != 0);
					u32 soundFlags = HowlSfx_Pack(HOWL_SFX_LR_CENTER, HOWL_SFX_DISTORTION_NONE, HOWL_SFX_VOLUME_MAX, echo);

					OtherFX_Play_LowLevel(6, 1, soundFlags);
					Voiceline_RequestPlay(6, data.characterIDs[d->driverID], 0x10);
					GAMEPAD_ShockFreq(d, 8, 0);
					GAMEPAD_ShockForce1(d, 8, 0x7f);

					if (d->kartState == KS_DRIFTING)
					{
						s16 turnAngle = d->turnAngleCurr;

						d->turnAngleCurr = 0;
						d->angle = (s16)CTR_MipsAddLo((u16)d->angle, (u16)turnAngle);
						d->rotCurr.w = (s16)CTR_MipsSubLo((u16)d->rotCurr.w, (u16)turnAngle);
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
