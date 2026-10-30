#include <common.h>
#include <ctr_scratchpad.h>

struct MetaDataMODEL *COLL_LevModelMeta(u32 id)
{
	// NOTE(aalhendi): Invalid unsigned IDs, including -1, select the fallback row.
	if (id >= NUM_MDM)
	{
		return &data.MetaDataModels[0];
	}

	return &data.MetaDataModels[id];
}


#include "COLL/COLL_CallbackQuadBlock.c"


#include "COLL/COLL_BspSearch.c"


#include "COLL/COLL_FixedTriangleShared.h"

internal u8 Coll_BspHitboxClass(const struct BSP *bsp)
{
	// NOTE(aalhendi): Retail reads the hitbox kind with lbu +0x1, not BSP.id.
	return (u8)(bsp->flag >> 8);
}

#include "COLL/COLL_Barycentrics.c"


#include "COLL/COLL_InstancePoint.c"


#include "COLL/COLL_TestPoint.c"


#include "COLL/COLL_GetNormVec.c"


#include "COLL/COLL_QuadBlockNormals.c"
#include "COLL/COLL_LoadScratchpadVerts.c"


#include "COLL/COLL_TestTriangles.c"


#include "COLL/COLL_BspLeaf.c"


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

static s32 COLL_FIXED_PlayerSearch_InterpolateColor(s32 baryA, s32 baryB, s32 c0, s32 c1, s32 c2)
{
	s32 term1 = CTR_MipsSra(CTR_MipsMulLo(baryA, CTR_MipsSubLo(c1, c0)), 12);
	s32 term2 = CTR_MipsSra(CTR_MipsMulLo(baryB, CTR_MipsSubLo(c2, c0)), 12);
	return CTR_MipsAddLo(CTR_MipsAddLo(term1, term2), c0);
}

static s32 COLL_FIXED_PlayerSearch_LumaTerm(s32 color, s32 weight)
{
	return CTR_MipsSra(CTR_MipsMulLo(color, weight), 8);
}

static s32 COLL_FIXED_PlayerSearch_AlphaBlend(s32 current, s32 light)
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

#include "COLL/COLL_MovedReorderNormals.c"

#include "COLL/COLL_MovedTrianglePoint.c"

#include "COLL/COLL_MovedQuadBlock.c"


#include "COLL/COLL_MovedBspLeaf.c"

enum
{
	COLL_HITBOX_SCRUB_DEPTH_BONUS = 0x200,
	COLL_MOVED_PLAYER_HIT_RADIUS = 0x19,
};

#include "COLL/COLL_FindScrub.c"


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

	CTR_VECTOR_DATA(&(sps->bbox.min))[axis] = (minNext < minCurrent) ? minNext : minCurrent;
	CTR_VECTOR_DATA(&(sps->bbox.max))[axis] = (maxCurrent < maxNext) ? maxNext : maxCurrent;
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
			d->wallRubTimer = DRIVER_WALL_RUB_TIMER_START;
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
					d->matrixArray = BAKED_GTE_MATRIX_CRASH_FALL;
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
