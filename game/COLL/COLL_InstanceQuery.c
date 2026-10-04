#include <common.h>

void COLL_FIXED_BSPLEAF_TestInstance(struct BSP *node, struct ScratchpadStruct *sps)
{
	struct BSP *hitboxes = node->data.leaf.bspHitboxArray;
	struct BSP *hitbox;

	if (hitboxes == NULL)
	{
		return;
	}

	// NOTE(aalhendi): Keep the leaf's array head separate from the advancing
	// cursor. The terminator is the complete flag/id word, not just flag.
	hitbox = hitboxes;
	if (CTR_ReadU32AlignedLE(hitbox) == 0)
	{
		return;
	}

	do
	{
		struct BoundingBox *bounds = &hitbox->box;
		s32 hitIndex = sps->numBspHitboxesHit - 1;

		// A hitbox may occur in more than one leaf. Do not test a hit twice.
		for (; hitIndex >= 0; hitIndex--)
		{
			if (hitbox == sps->bspHitboxesHit[hitIndex])
			{
				goto NextHitbox;
			}
		}

		// Instance-backed collidable hitboxes require an active view flag.
		// Unbound or noncollidable records still use the geometric test.
		if ((((*(const u8 *)&hitbox->flag & BSP_HITBOX_COLLIDABLE) == 0) || (hitbox->data.hitbox.instDef == NULL) ||
		     ((hitbox->data.hitbox.instDef->ptrInstance->flags & DRAW_COLLISION_MASK) != 0)) &&
		    (sps->bbox.min.x <= bounds->max.x) && (bounds->min.x <= sps->bbox.max.x) && (sps->bbox.min.y <= bounds->max.y) &&
		    (bounds->min.y <= sps->bbox.max.y) && (sps->bbox.min.z <= bounds->max.z) && (bounds->min.z <= sps->bbox.max.z))
		{
			COLL_FIXED_INSTANC_TestPoint(sps, hitbox);
		}

	NextHitbox:
		hitbox++;
	} while (CTR_ReadU32AlignedLE(hitbox) != 0);
}


void COLL_FIXED_BotsSearch(const SVec3 *posCurr, const SVec3 *posPrev, struct ScratchpadStruct *sps)
{
	// NOTE(aalhendi): Component copies and reloads retain retail halfword accesses.
	sps->Input1.hitRadiusSquared = CTR_MipsMulLo(sps->Input1.hitRadius, sps->Input1.hitRadius);
	sps->Input1.pos.x = posCurr->x;
	sps->Input1.pos.y = posCurr->y;
	sps->Input1.pos.z = posCurr->z;
	sps->Union.QuadBlockColl.hitPos.x = sps->Input1.pos.x;
	sps->Union.QuadBlockColl.hitPos.y = sps->Input1.pos.y;
	sps->Union.QuadBlockColl.hitPos.z = sps->Input1.pos.z;
	sps->Union.QuadBlockColl.hitRadius = sps->Input1.hitRadius;
	sps->Union.QuadBlockColl.hitRadiusSquared = sps->Input1.hitRadiusSquared;
	sps->Union.QuadBlockColl.pos.x = posPrev->x;
	sps->Union.QuadBlockColl.pos.y = posPrev->y;
	sps->Union.QuadBlockColl.pos.z = posPrev->z;

	// The inclusive swept box stores all minimum axes before maximum axes.
	// Signed deltas narrow only at each s16 destination.
	{
		s32 curr = (sps->Input1.pos.x - sps->Input1.hitRadius);
		s32 prev = (sps->Union.QuadBlockColl.pos.x - sps->Union.QuadBlockColl.hitRadius);
		sps->bbox.min.x = (curr < prev) ? curr : prev;
	}
	{
		s32 curr = (sps->Input1.pos.y - sps->Input1.hitRadius);
		s32 prev = (sps->Union.QuadBlockColl.pos.y - sps->Union.QuadBlockColl.hitRadius);
		sps->bbox.min.y = (curr < prev) ? curr : prev;
	}
	{
		s32 curr = (sps->Input1.pos.z - sps->Input1.hitRadius);
		s32 prev = (sps->Union.QuadBlockColl.pos.z - sps->Union.QuadBlockColl.hitRadius);
		sps->bbox.min.z = (curr < prev) ? curr : prev;
	}
	{
		s32 curr = (sps->Input1.pos.x + sps->Input1.hitRadius);
		s32 prev = (sps->Union.QuadBlockColl.pos.x + sps->Union.QuadBlockColl.hitRadius);
		sps->bbox.max.x = (curr > prev) ? curr : prev;
	}
	{
		s32 curr = (sps->Input1.pos.y + sps->Input1.hitRadius);
		s32 prev = (sps->Union.QuadBlockColl.pos.y + sps->Union.QuadBlockColl.hitRadius);
		sps->bbox.max.y = (curr > prev) ? curr : prev;
	}
	{
		s32 curr = (sps->Input1.pos.z + sps->Input1.hitRadius);
		s32 prev = (sps->Union.QuadBlockColl.pos.z + sps->Union.QuadBlockColl.hitRadius);
		sps->bbox.max.z = (curr > prev) ? curr : prev;
	}

	sps->boolDidTouchHitbox = 0;
	sps->numTrianglesTested = 0;
	sps->hitFraction = COLL_FRACTION_ONE;
	sps->collision.stepFlags = 0;
	sps->numBspHitboxesHit = 0;
	COLL_SearchBSP_CallbackPARAM(sps->ptr_mesh_info->bspRoot, &sps->bbox, COLL_FIXED_BSPLEAF_TestInstance, sps);
}
