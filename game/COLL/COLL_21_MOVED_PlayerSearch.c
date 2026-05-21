#include <common.h>
#include <ctr_scratchpad.h>

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

	meta = DECOMP_COLL_LevModelMeta(modelID);
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
			DECOMP_COLL_SearchBSP_CallbackPARAM(gGT->level1->ptr_mesh_info->bspRoot, &sps->bbox, COLL_MOVED_BSPLEAF_TestQuadblocks, sps);
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
			int hitboxResult;

			sps->Union.QuadBlockColl.searchFlags &= 0xfff7;
			d->unkAA &= 0xfffd;

			hitboxResult = CollMoved_PlayerSearch_RunHitboxLInC(sps, t);

			if ((hitboxResult == 2) || (bspHitbox->id == 4))
			{
				CollMoved_PlayerSearch_StoreHitbox(sps);
			}
			else
			{
				struct Scrub *scrub;

				COLL_MOVED_FindScrub((struct QuadBlock *)bspHitbox, 0, sps);
				CollMoved_PlayerSearch_BoostHitboxScrub(sps);

				scrub = VehAfterColl_GetSurface((u8)bspHitbox->id);
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
