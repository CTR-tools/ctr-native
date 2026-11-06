#include <common.h>
#include <ctr_scratchpad.h>

static inline u8 Coll_BspHitboxClass(const struct BSP *bsp)
{
	// NOTE(aalhendi): Retail reads the hitbox kind with lbu +0x1, not BSP.id.
	return ((const u8 *)&bsp->flag)[1];
}

enum
{
	COLL_HITBOX_SCRUB_DEPTH_BONUS = 0x200,
	COLL_MOVED_PLAYER_HIT_RADIUS = 0x19,
};

static inline s32 CollMoved_PlayerSearch_StepVelocity(s32 velocity, s32 elapsedTimeMS, s32 multiplier)
{
	return CTR_MipsSra(CTR_MipsMulLo(CTR_MipsSra(CTR_MipsMulLo(velocity, elapsedTimeMS), 5), multiplier), 12);
}

static inline s32 CollMoved_PlayerSearch_Min(struct ScratchpadStruct *sps, s16 current, s16 next)
{
	s32 currentBound = CTR_MipsSubLo(current, sps->Union.QuadBlockColl.hitRadius);
	s32 nextBound = CTR_MipsSubLo(next, sps->Input1.hitRadius);

	return (nextBound < currentBound) ? nextBound : currentBound;
}

static inline s32 CollMoved_PlayerSearch_Max(struct ScratchpadStruct *sps, s16 current, s16 next)
{
	s32 currentBound = CTR_MipsAddLo(current, sps->Union.QuadBlockColl.hitRadius);
	s32 nextBound = CTR_MipsAddLo(next, sps->Input1.hitRadius);

	if (currentBound < nextBound)
	{
		currentBound = nextBound;
	}
	return currentBound;
}

static inline void CollMoved_PlayerSearch_StoreHitbox(struct ScratchpadStruct *sps)
{
	s32 count = sps->numBspHitboxesHit;

	sps->bspHitboxesHit[count++] = sps->bspHitbox;
	sps->numBspHitboxesHit = count;
}

void COLL_MOVED_PlayerSearch(struct Thread *t, struct Driver *d)
{
	struct GameTracker *initialGT = GAME_TRACKER;
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
	s32 multiplier;
	s32 hitRadius = COLL_MOVED_PLAYER_HIT_RADIUS;
	s32 iterations;
	s32 noImpactClass;
	struct mesh_info *initialMesh;

	sps->Input1.hitRadius = hitRadius;
	sps->Input1.hitRadiusSquared = hitRadius * hitRadius;
	sps->Union.QuadBlockColl.hitRadius = hitRadius;
	sps->Union.QuadBlockColl.hitRadiusSquared = hitRadius * hitRadius;
	initialMesh = initialGT->level1->ptr_mesh_info;
	sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND | QUADBLOCK_FLAG_COLLISION_SURFACE;
	sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
	sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES;
	sps->ptr_mesh_info = initialMesh;

	if (initialGT->numPlyrCurrGame < 3)
	{
		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES | COLL_SEARCH_HIGH_LOD;
	}

	multiplier = COLL_FRACTION_ONE;
	sps->numBspHitboxesHit = 0;
	sps->Input1.modelID = DYNAMIC_PLAYER;
	sps->collision.stepFlags = 0;

	COLL_MOVED_FindScrub(NULL, 0, sps);

	iterations = 15;
	noImpactClass = 4;
	for (; iterations != 0; iterations--)
	{
		Vec3 velocity;
		u32 searchFlags CTR_PSX_LOCAL_REGISTER_HINT("$3");
		struct GameTracker *loopGT = GAME_TRACKER;

		velocity.x = CollMoved_PlayerSearch_StepVelocity(d->velocity.x, loopGT->elapsedTimeMS, multiplier);
		velocity.y = CollMoved_PlayerSearch_StepVelocity(d->velocity.y, loopGT->elapsedTimeMS, multiplier);
		velocity.z = CollMoved_PlayerSearch_StepVelocity(d->velocity.z, loopGT->elapsedTimeMS, multiplier);

		sps->boolDidTouchQuadblock = 0;
		sps->numTrianglesTested = 0;
		sps->boolDidTouchHitbox = 0;
		sps->resetOnly40 = 0;
		searchFlags = sps->Union.QuadBlockColl.searchFlags;
		sps->Union.QuadBlockColl.searchFlags = searchFlags | COLL_SEARCH_TEST_INSTANCES;
		sps->hitFraction = COLL_FRACTION_ONE;

		// NOTE(aalhendi): Wrap the sweep in 32 bits before narrowing each coordinate.
		sps->Union.QuadBlockColl.pos.x = (s16)CTR_MipsAddLo((u16)d->originToCenter.x, CTR_MipsSra(d->posCurr.x, 8));
		sps->Union.QuadBlockColl.pos.y = (s16)CTR_MipsAddLo((u16)d->originToCenter.y, CTR_MipsSra(d->posCurr.y, 8));
		sps->Union.QuadBlockColl.pos.z = (s16)CTR_MipsAddLo((u16)d->originToCenter.z, CTR_MipsSra(d->posCurr.z, 8));

		sps->Input1.pos.x = (s16)CTR_MipsAddLo((u16)d->originToCenter.x, CTR_MipsSra(CTR_MipsAddLo(d->posCurr.x, velocity.x), 8));
		sps->Input1.pos.y = (s16)CTR_MipsAddLo((u16)d->originToCenter.y, CTR_MipsSra(CTR_MipsAddLo(d->posCurr.y, velocity.y), 8));
		sps->Input1.pos.z = (s16)CTR_MipsAddLo((u16)d->originToCenter.z, CTR_MipsSra(CTR_MipsAddLo(d->posCurr.z, velocity.z), 8));

		if ((sps->Input1.pos.x == sps->Union.QuadBlockColl.pos.x) && (sps->Input1.pos.y == sps->Union.QuadBlockColl.pos.y) &&
		    (sps->Input1.pos.z == sps->Union.QuadBlockColl.pos.z))
		{
			break;
		}

		sps->bbox.min.x = CollMoved_PlayerSearch_Min(sps, sps->Union.QuadBlockColl.pos.x, sps->Input1.pos.x);
		sps->bbox.min.y = CollMoved_PlayerSearch_Min(sps, sps->Union.QuadBlockColl.pos.y, sps->Input1.pos.y);
		sps->bbox.min.z = CollMoved_PlayerSearch_Min(sps, sps->Union.QuadBlockColl.pos.z, sps->Input1.pos.z);
		sps->bbox.max.x = CollMoved_PlayerSearch_Max(sps, sps->Union.QuadBlockColl.pos.x, sps->Input1.pos.x);
		sps->bbox.max.y = CollMoved_PlayerSearch_Max(sps, sps->Union.QuadBlockColl.pos.y, sps->Input1.pos.y);
		sps->bbox.max.z = CollMoved_PlayerSearch_Max(sps, sps->Union.QuadBlockColl.pos.z, sps->Input1.pos.z);

		sps->Union.QuadBlockColl.hitPos.x = sps->Input1.pos.x;
		sps->Union.QuadBlockColl.hitPos.y = sps->Input1.pos.y;
		sps->Union.QuadBlockColl.hitPos.z = sps->Input1.pos.z;

		if (sps->boolDidTouchQuadblock == 0)
		{
			sps->Union.QuadBlockColl.searchFlags = (sps->Union.QuadBlockColl.searchFlags | COLL_SEARCH_TEST_INSTANCES) & ~COLL_SEARCH_REUSE_NORMALS;

			if ((loopGT->level1 != NULL) && (loopGT->level1->ptr_mesh_info != NULL) && (loopGT->level1->ptr_mesh_info->bspRoot != NULL))
			{
				COLL_SearchBSP_CallbackPARAM(loopGT->level1->ptr_mesh_info->bspRoot, &sps->bbox, COLL_MOVED_BSPLEAF_TestQuadblocks, sps);
			}
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
			s32 result = 1;
			struct BSP *bsp;
			struct Instance *linCInstance;
			s32 modelID;
			struct MetaDataMODEL *meta;
			struct Scrub *scrub;

			sps->Union.QuadBlockColl.searchFlags &= ~COLL_SEARCH_REUSE_NORMALS;
			d->collisionFlags &= ~DRIVER_COLL_FLAG_SURFACE_PUSHBACK;

			bsp = sps->bspHitbox;
			if ((((const u8 *)&bsp->flag)[0] & BSP_HITBOX_COLLIDABLE) != 0)
			{
				struct InstDef *instDef = bsp->data.hitbox.instDef;
				if (instDef == NULL)
				{
					goto CollMovedHitboxDispatchDone;
				}

				linCInstance = instDef->ptrInstance;
				if (linCInstance == NULL)
				{
					goto CollMovedHitboxDispatchDone;
				}

				modelID = (s16)instDef->modelID;
				if ((linCInstance->flags & DRAW_COLLISION_MASK) == 0)
				{
					goto CollMovedHitboxDispatchDone;
				}
			}
			else
			{
				if ((((const u8 *)&bsp->flag)[0] & BSP_HITBOX_LINC_USES_INSTDEF) == 0)
				{
					goto CollMovedHitboxDispatchDone;
				}

				// These hitboxes pass the payload itself to LInC, not ptrInstance.
				linCInstance = (struct Instance *)bsp->data.hitbox.instDef;
				if (linCInstance == NULL)
				{
					goto CollMovedHitboxDispatchDone;
				}

				// NOTE(aalhendi): Native retains the InstDef model view for 0x10-only hitboxes.
#ifdef CTR_NATIVE
				modelID = ((struct InstDef *)bsp->data.hitbox.instDef)->model->id;
#else
				modelID = linCInstance->model->id;
#endif
			}

			meta = COLL_LevModelMeta(modelID);
			if ((meta != NULL) && (meta->LInC != NULL))
			{
				result = meta->LInC(linCInstance, t, sps);
			}

		CollMovedHitboxDispatchDone:

			// LInC may replace the current hitbox; reread it before dispatching impact.
			if ((result == 2) || (Coll_BspHitboxClass(sps->bspHitbox) == noImpactClass))
			{
				CollMoved_PlayerSearch_StoreHitbox(sps);
				continue;
			}

			COLL_MOVED_FindScrub((struct QuadBlock *)sps->bspHitbox, 0, sps);
			sps->Input1.scrubDepth = (s16)CTR_MipsAddLo((u16)sps->Input1.scrubDepth, COLL_HITBOX_SCRUB_DEPTH_BONUS);

			result = 0;
			scrub = VehAfterColl_GetSurface(Coll_BspHitboxClass(sps->bspHitbox));
			if (Coll_BspHitboxClass(sps->bspHitbox) == noImpactClass)
			{
				goto CollMovedHitboxAppend;
			}
			result = COLL_MOVED_ScrubImpact(d, t, sps, scrub, &d->velocity);
			if (result != 0)
			{
				goto CollMovedHitboxComplete;
			}

		// Class-four hitboxes append; a handled impact skips that append.
		CollMovedHitboxAppend:
			CollMoved_PlayerSearch_StoreHitbox(sps);
		CollMovedHitboxComplete:
			if (result == 2)
			{
				return;
			}
		}
		else
		{
			struct QuadBlock *quad;
			u32 scrubId;
			// NOTE(aalhendi): Keep the fourth impact argument in its call register.
			register struct Scrub *scrub CTR_PSX_REGISTER("$7");
			u32 impactResult;

			if (sps->boolDidTouchQuadblock == 0)
			{
				break;
			}

			if ((sps->hit.ptrQuadblock->quadFlags & QUADBLOCK_FLAG_KILL_PLANE) != 0)
			{
				d->collisionFlags |= DRIVER_COLL_FLAG_MASK_GRAB_REQUEST;
			}

			COLL_MOVED_FindScrub(sps->hit.ptrQuadblock, sps->hit.triangleID, sps);
			quad = sps->hit.ptrQuadblock;

			if ((quad->quadFlags & QUADBLOCK_FLAG_GROUND) == 0)
			{
				scrubId = 6;
				if ((quad->quadFlags & QUADBLOCK_FLAG_REFLECT_SPLIT_LINE_1) == 0)
				{
					scrubId = 0;
				}
			}
			else
			{
				if ((quad != d->underDriver) && ((quad->quadFlags & 8) != 0))
				{
					d->underDriver = NULL;
				}

				d->currBlockTouching = sps->hit.ptrQuadblock;
				CTR_WriteU32AlignedLE(&d->normalVecUP, CTR_ReadU32AlignedLE(&sps->hit.plane.normal));
				d->normalVecUP.z = sps->hit.plane.normal.z;
				CTR_WriteU32AlignedLE(&d->AxisAngle1_normalVec, CTR_ReadU32AlignedLE(&sps->hit.plane.normal));
				d->AxisAngle1_normalVec.z = sps->hit.plane.normal.z;
				d->collisionFlags |= DRIVER_COLL_FLAG_GROUNDED;
				scrubId = 5;
			}

			scrub = VehAfterColl_GetSurface(scrubId);
			d->collisionFlags |= DRIVER_COLL_FLAG_SURFACE_PUSHBACK;
			CTR_WriteU32AlignedLE(&d->spsHitPos, CTR_ReadU32AlignedLE(&sps->hit.hitPos));
			d->spsHitPos.z = sps->hit.hitPos.z;
			CTR_WriteU32AlignedLE(&d->spsNormalVec, CTR_ReadU32AlignedLE(&sps->hit.plane.normal));
			d->spsNormalVec.z = sps->hit.plane.normal.z;

			impactResult = COLL_MOVED_ScrubImpact(d, t, sps, scrub, &d->velocity);
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
