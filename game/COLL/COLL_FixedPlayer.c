#include <common.h>
#include <ctr_scratchpad.h>

static inline u32 CollFixed_PlayerSearch_Length(u32 sum)
{
	return VehCalc_FastSqrt(sum, 24) >> 12;
}

static inline s32 CollFixed_PlayerSearch_Abs(s32 value)
{
	return (value < 0) ? (s32)(0u - (u32)value) : value;
}

enum
{
	COLL_FIXED_PLAYER_SEARCH_TOP_Y_OFFSET = 0x80,
	COLL_FIXED_PLAYER_SEARCH_BOTTOM_Y_OFFSET = 0x100,
	COLL_FIXED_PLAYER_SEARCH_AXIS3_OLD_NORMAL_WEIGHT = 5,
	COLL_FIXED_PLAYER_SEARCH_AXIS3_HIT_NORMAL_WEIGHT = 3,
	COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_TOTAL_WEIGHT = 8,
	COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_SHIFT = 3,
	COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_GROUND_WEIGHT = 6,
	COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_AIRBORNE_WEIGHT = 7,
	COLL_FIXED_PLAYER_SEARCH_COYOTE_TIME_MS = 0xa0,
};

void COLL_FIXED_PlayerSearch(struct Thread *t, struct Driver *d)
{
	struct ScratchpadStruct *sps;
	s32 normalBlendWeight;
	struct Instance *inst;
	struct QuadBlock *quad;
	s32 landingDelta;
	s32 work;
	const struct TrigTable *orientationTable;

	{
		s32 topX, topY, topZ;
		s32 bottomX, bottomY, bottomZ;
		struct GameTracker *gGT;

		d->actionsFlagSet &= ~ACTION_ENGINE_ECHO;
		sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
		// Sample both probes independently, then compare their signed-short bounds.
		topX = d->posCurr.x >> 8;
		sps->Union.QuadBlockColl.pos.x = (s16)topX;
		topY = (s32)((u32)(d->posCurr.y >> 8) + COLL_FIXED_PLAYER_SEARCH_TOP_Y_OFFSET);
		sps->Union.QuadBlockColl.pos.y = (s16)topY;
		topZ = d->posCurr.z >> 8;
		sps->Union.QuadBlockColl.pos.z = (s16)topZ;
		bottomX = d->posCurr.x >> 8;
		sps->Input1.pos.x = (s16)bottomX;
		bottomY = (s32)((u32)(d->posCurr.y >> 8) - COLL_FIXED_PLAYER_SEARCH_BOTTOM_Y_OFFSET);
		sps->Input1.pos.y = (s16)bottomY;
		bottomZ = d->posCurr.z >> 8;
		sps->Input1.pos.z = (s16)bottomZ;

		gGT = GAME_TRACKER;
		sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;
		sps->Union.QuadBlockColl.quadFlagsIgnored = QUADBLOCK_FLAG_NO_COLLISION_RESPONSE;
		sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND | QUADBLOCK_FLAG_COLLISION_SURFACE;
		sps->Union.QuadBlockColl.searchFlags = 0;
		if (gGT->numPlyrCurrGame < 3)
		{
			sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;
		}

		sps->boolDidTouchQuadblock = 0;
#if defined(CTR_NATIVE)
		sps->boolDidTouchHitbox = 0;
#endif
		sps->numTrianglesTested = 0;

		sps->bbox.min.x = ((s16)bottomX < (s16)topX) ? (s16)bottomX : (s16)topX;
		sps->bbox.min.y = ((s16)bottomY < (s16)topY) ? (s16)bottomY : (s16)topY;
		sps->bbox.min.z = ((s16)bottomZ < (s16)topZ) ? (s16)bottomZ : (s16)topZ;
		sps->bbox.max.x = ((s16)topX < (s16)bottomX) ? (s16)bottomX : (s16)topX;
		sps->bbox.max.y = ((s16)topY < (s16)bottomY) ? (s16)bottomY : (s16)topY;
		sps->bbox.max.z = ((s16)topZ < (s16)bottomZ) ? (s16)bottomZ : (s16)topZ;
		sps->Union.QuadBlockColl.hitPos.x = (s16)bottomX;
		sps->Union.QuadBlockColl.hitPos.y = (s16)bottomY;
		sps->Union.QuadBlockColl.hitPos.z = (s16)bottomZ;
	}

	if (d->underDriver != NULL)
	{
		COLL_FIXED_QUADBLK_TestTriangles(d->underDriver, sps);
	}

	if ((sps->boolDidTouchQuadblock == 0) && (sps->ptr_mesh_info != NULL) && (sps->ptr_mesh_info->bspRoot != NULL))
	{
		COLL_SearchBSP_CallbackPARAM(sps->ptr_mesh_info->bspRoot, &sps->bbox, COLL_FIXED_BSPLEAF_TestQuadblocks, sps);
	}

	inst = t->inst;

	if (sps->boolDidTouchQuadblock != 0)
	{
		inst->compressedNormalAndDriverIndex = ((((u32)(u16)sps->hit.plane.normal.x >> 6) & 0xffu) | (((u32)(u16)sps->hit.plane.normal.y << 2) & 0xff00u) |
		                                        ((((u32)(u16)sps->hit.plane.normal.z >> 6) & 0xffu) << 16) | (((u32)d->driverID + 1u) << 24));
		d->quadBlockHeight = (s32)((u32)sps->Union.QuadBlockColl.hitPos.y << 8);
		d->collisionFlags |= DRIVER_COLL_FLAG_TOUCHED_QUADBLOCK;

		if ((sps->hit.ptrQuadblock->terrain_type == TERRAIN_MUD) || (sps->hit.ptrQuadblock->terrain_type == TERRAIN_WATER) ||
		    (sps->hit.ptrQuadblock->terrain_type == TERRAIN_FASTWATER))
		{
			inst->vertSplit = 0;
			inst->flags |= SPLIT_LINE;
		}

		if (GAME_TRACKER->numPlyrCurrGame < 2)
		{
			u16 quadFlags = sps->hit.ptrQuadblock->quadFlags;

			if ((quadFlags & QUADBLOCK_FLAG_COLLISION_SURFACE) == 0)
			{
				if ((quadFlags & QUADBLOCK_FLAG_REFLECT_SPLIT_LINE_1) != 0)
				{
					inst->flags |= REFLECTIVE;
					inst->vertSplit = GAME_TRACKER->level1->splitLines[1];
				}
				else if ((quadFlags & QUADBLOCK_FLAG_REFLECT_SPLIT_LINE_0) != 0)
				{
					inst->flags |= REFLECTIVE;
					inst->vertSplit = GAME_TRACKER->level1->splitLines[0];
				}
				else
				{
					inst->flags &= ~REFLECTIVE;
				}
			}
		}

		{
			s32 x = (s32)((u32)d->AxisAngle3_normalVec.x * COLL_FIXED_PLAYER_SEARCH_AXIS3_OLD_NORMAL_WEIGHT +
			              (u32)sps->hit.plane.normal.x * COLL_FIXED_PLAYER_SEARCH_AXIS3_HIT_NORMAL_WEIGHT);
			s32 y = (s32)((u32)d->AxisAngle3_normalVec.y * COLL_FIXED_PLAYER_SEARCH_AXIS3_OLD_NORMAL_WEIGHT +
			              (u32)sps->hit.plane.normal.y * COLL_FIXED_PLAYER_SEARCH_AXIS3_HIT_NORMAL_WEIGHT);
			s32 z = (s32)((u32)d->AxisAngle3_normalVec.z * COLL_FIXED_PLAYER_SEARCH_AXIS3_OLD_NORMAL_WEIGHT +
			              (u32)sps->hit.plane.normal.z * COLL_FIXED_PLAYER_SEARCH_AXIS3_HIT_NORMAL_WEIGHT);
			// Products and sums intentionally retain their low 32 bits.
			u32 sum = (u32)x * (u32)x + (u32)y * (u32)y + (u32)z * (u32)z;
			work = CollFixed_PlayerSearch_Length(sum);
#if defined(CTR_NATIVE)
			if (work == 0)
			{
				CTR_TRAP();
			}
#endif

			d->AxisAngle3_normalVec.x = (s16)((s32)((u32)x << 12) / work);
			d->AxisAngle3_normalVec.y = (s16)((s32)((u32)y << 12) / work);
			d->AxisAngle3_normalVec.z = (s16)((s32)((u32)z << 12) / work);
		}

		if ((sps->hit.ptrQuadblock->quadFlags & QUADBLOCK_FLAG_ENGINE_ECHO) != 0)
		{
			d->actionsFlagSet |= ACTION_ENGINE_ECHO;
		}

		d->underDriver = sps->hit.ptrQuadblock;

		if ((d->posCurr.y <= (s32)((u32)d->quadBlockHeight + 0x1000u)) || ((sps->hit.ptrQuadblock->terrain_type == TERRAIN_MUD) && (d->posCurr.y < 1)))
		{
			if ((sps->hit.ptrQuadblock->quadFlags & QUADBLOCK_FLAG_GROUND) != 0)
			{
				CTR_WriteU32AlignedLE(&d->normalVecUP, CTR_ReadU32AlignedLE(&sps->hit.plane.normal));
				d->normalVecUP.z = sps->hit.plane.normal.z;
				d->collisionFlags |= DRIVER_COLL_FLAG_GROUNDED;
			}

			if (d->currBlockTouching == NULL)
			{
				d->currBlockTouching = sps->hit.ptrQuadblock;
				CTR_WriteU32AlignedLE(&d->AxisAngle1_normalVec, CTR_ReadU32AlignedLE(&sps->hit.plane.normal));
				d->AxisAngle1_normalVec.z = sps->hit.plane.normal.z;
			}

			{
				struct LevVertex *v1, *v2, *v0;
				s32 baryA, baryB;
				s32 rA, rB, gA, gB, bA, bB;
				s32 r, g, b, luma;

				v0 = sps->hitLevelTriangle.v0;
				if (v0 == NULL)
				{
					goto LightingDone;
				}
				v1 = sps->hitLevelTriangle.v1;
				if (v1 == NULL)
				{
					goto LightingDone;
				}
				v2 = sps->hitLevelTriangle.v2;
				if (v2 == NULL)
				{
					goto LightingDone;
				}

				baryA = sps->hitBarycentrics.v1;
				baryB = sps->hitBarycentrics.v2;

				rA = baryA * (v1->color_hi[0] - v0->color_hi[0]);
				rB = baryB * (v2->color_hi[0] - v0->color_hi[0]);
				gA = baryA * (v1->color_hi[1] - v0->color_hi[1]);
				gB = baryB * (v2->color_hi[1] - v0->color_hi[1]);
				bA = baryA * (v1->color_hi[2] - v0->color_hi[2]);
				bB = baryB * (v2->color_hi[2] - v0->color_hi[2]);
				r = (rA >> 12) + (rB >> 12) + v0->color_hi[0];
				g = (gA >> 12) + (gB >> 12) + v0->color_hi[1];
				b = (bA >> 12) + (bB >> 12) + v0->color_hi[2];

				if (r < 0)
				{
					r = 0;
				}
				if (r > 255)
				{
					r = 255;
				}
				if (g < 0)
				{
					g = 0;
				}
				if (g > 255)
				{
					g = 255;
				}
				if (b < 0)
				{
					b = 0;
				}
				if (b > 255)
				{
					b = 255;
				}

				luma = ((r * 0x4c) >> 8) + ((g * 0x96) >> 8) + ((b * 0x1e) >> 8);
				b = 0xc00 - (luma << 5);
				if (b < 0)
				{
					b = 0;
				}
				if (b > 0x1000)
				{
					b = 0x1000;
				}
				r = b << 3;
				r -= b;
				r <<= 3;
				d->alphaScaleBackup = (s16)(((u16)d->alphaScaleBackup * 200 + r) >> 8);
				inst->alphaScale = (s16)(((u16)inst->alphaScale * 200 + r) >> 8);

			LightingDone:;
			}
		}
	}
	else
	{
		inst->compressedNormalAndDriverIndex = INST_CompressNormalVectorAndDriverIndex(0, FP_ONE, 0, d->driverID);
		inst->flags &= ~REFLECTIVE;
		d->quadBlockHeight = (s32)((u32)d->posCurr.y - 0x10000u);
	}

	if ((s32)((u32)d->quadBlockHeight + 0x8000u) < d->posCurr.y)
	{
		d->terrainMeta2 = VehAfterColl_GetTerrain(TERRAIN_NONE);
	}

	if (d->posCurr.y < (s32)(((u32)GAME_TRACKER->level1->ptr_mesh_info->bspRoot->box.min.y - 0x40u) << 8))
	{
		d->collisionFlags |= DRIVER_COLL_FLAG_MASK_GRAB_REQUEST;
	}

	landingDelta = (s32)((u32)d->velocity.y - (u32)d->ySpeed);

	if ((d->currBlockTouching != NULL) && ((d->collisionFlags & (DRIVER_COLL_FLAG_MASK_GRAB_REQUEST | DRIVER_COLL_FLAG_GROUNDED)) == 0) &&
	    (d->kartState != KS_MASK_GRABBED))
	{
		d->velocity.x = (s32)((u32)d->velocity.x + (u32)(d->AxisAngle1_normalVec.x >> 1));
		d->velocity.y = (s32)((u32)d->velocity.y + (u32)(d->AxisAngle1_normalVec.y >> 1));
		d->velocity.z = (s32)((u32)d->velocity.z + (u32)(d->AxisAngle1_normalVec.z >> 1));
	}

	d->xSpeed = d->velocity.x;
	d->ySpeed = d->velocity.y;
	d->zSpeed = d->velocity.z;
	quad = d->currBlockTouching;

	if (quad == NULL)
	{
		goto DriverAirborne;
	}

	{
		u16 quadFlags = quad->quadFlags;
		s32 checkpointIndex;
		struct CheckpointNode *node;
		struct CheckpointNode *nodes;
		struct CheckpointNode *restart;
		u16 trackLength;

		if ((quadFlags & QUADBLOCK_FLAG_KILL_PLANE) != 0)
		{
			goto RequestMask;
		}
		if ((d->kartState == KS_MASK_GRABBED) || ((d->collisionFlags & DRIVER_COLL_FLAG_MASK_GRAB_REQUEST) != 0) || ((quadFlags & QUADBLOCK_FLAG_GROUND) == 0))
		{
			goto MaskChecked;
		}

		if (quad->checkpointIndex != 0xff)
		{
			checkpointIndex = quad->checkpointIndex;
			nodes = GAME_TRACKER->level1->ptr_restart_points;
			node = &nodes[checkpointIndex];
			if (((d->actionsFlagSet & ACTION_BEHIND_START_LINE) == 0) && (node->nextIndex_forward > 1) &&
			    ((s32)((u32)d->distanceToFinish_checkpoint - ((u32)node->distToFinish << 3)) > ((nodes[0].distToFinish >> 2) << 3)))
			{
				d->collisionFlags |= DRIVER_COLL_FLAG_MASK_GRAB_REQUEST;
				goto MaskChecked;
			}

			restart = GAME_TRACKER->level1->ptr_restart_points;
			trackLength = restart[0].distToFinish;
#if defined(CTR_NATIVE)
			// NOTE(aalhendi): Native cannot mirror retail low-memory null-space reads.
			if (d->lastValid == NULL)
			{
				d->lastValid = d->currBlockTouching;
				goto MaskChecked;
			}
#endif
			if ((node->distToFinish < ((trackLength * 15) >> 4)) && (d->lastValid->checkpointIndex != 0xff) &&
			    (restart[d->lastValid->checkpointIndex].distToFinish + (trackLength >> 2) < node->distToFinish))
			{
			RequestMask:
				d->collisionFlags |= DRIVER_COLL_FLAG_MASK_GRAB_REQUEST;
				goto MaskChecked;
			}
			d->lastValid = d->currBlockTouching;
			goto MaskChecked;
		}
		if (((u32)GAME_TRACKER->levelID - GEM_STONE_VALLEY) < 5)
		{
			d->lastValid = quad;
		}
	MaskChecked:;
	}

	d->jump_LandingBoost = 0;
	d->actionsFlagSet &= ~(ACTION_AIRBORNE | ACTION_HIGH_JUMP);

	if ((d->collisionFlags & DRIVER_COLL_FLAG_GROUNDED) == 0)
	{
		goto DriverAirborne;
	}

	{
		s32 terrainType;
		if ((d->rainCloudEffect == RAIN_CLOUD_EFFECT_ICE_TERRAIN) || ((GAME_TRACKER->gameMode2 & CHEAT_ICY) != 0))
		{
			d->currentTerrain = TERRAIN_ICE;
			terrainType = TERRAIN_ICE;
		}
		else
		{
			terrainType = d->currBlockTouching->terrain_type;

			if ((terrainType != TERRAIN_ICE) && (d->currentTerrain == TERRAIN_ICE))
			{
				d->terrainFrictionTimer = 0xfec0;
			}

			d->currentTerrain = terrainType;
		}

		d->terrainMeta1 = VehAfterColl_GetTerrain(terrainType);
	}
	d->terrainMeta2 = d->terrainMeta1;
	d->jump_CoyoteTimerMS = COLL_FIXED_PLAYER_SEARCH_COYOTE_TIME_MS;
	// NOTE(aalhendi): Compiler-only boundary after actual terrain/coyote publication.
	CTR_PSX_OBSERVE_MEMORY(d->jump_CoyoteTimerMS);

	{
		u32 actions;
		// NOTE(aalhendi): Soft local preference leaves the live current-actions value separate.
		u32 wasTouchingGround CTR_PSX_LOCAL_REGISTER_HINT("$3");

		actions = d->actionsFlagSet;
		actions |= ACTION_TOUCH_GROUND;
		d->actionsFlagSet = actions;

		wasTouchingGround = d->actionsFlagSetPrevFrame & ACTION_TOUCH_GROUND;
		if (wasTouchingGround == 0)
		{
			s32 absLanding = CollFixed_PlayerSearch_Abs(landingDelta);
			u32 volume;
			// NOTE(aalhendi): Soft preference keeps this pending timer separate from live actions.
			s16 landingFriction CTR_PSX_LOCAL_REGISTER_HINT("$3");

			landingFriction = 0x140;
			actions |= ACTION_STARTED_TOUCH_GROUND | ACTION_TURBO_INPUT_LATCH;
			d->actionsFlagSet = actions;
			d->terrainFrictionTimer = landingFriction;

			volume = VehCalc_MapToRange(absLanding, 0x100, 0x3c00, 0x78, 0xfa);

			if (d->kartState != KS_MASK_GRABBED)
			{
				u32 soundFlags = ((volume & 0xffu) << 16);
				if ((d->actionsFlagSet & ACTION_ENGINE_ECHO) != 0)
				{
					soundFlags |= 0x1008080u;
				}
				else
				{
					soundFlags |= 0x8080u;
				}
				OtherFX_Play_LowLevel(7, 1, soundFlags);
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

	d->jump_LandingBoost = (s16)((u32)(u16)d->jump_LandingBoost + (u16)GAME_TRACKER->elapsedTimeMS);
	d->jump_CoyoteTimerMS = (s16)((u32)(u16)d->jump_CoyoteTimerMS - (u16)GAME_TRACKER->elapsedTimeMS);
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
	Vec3 blended;
	s32 invWeight = COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_TOTAL_WEIGHT - normalBlendWeight;
	blended.x =
	    (s32)((u32)normalBlendWeight * (u32)d->AxisAngle2_normalVec.x + (u32)invWeight * (u32)d->normalVecUP.x) >> COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_SHIFT;
	blended.y =
	    (s32)((u32)normalBlendWeight * (u32)d->AxisAngle2_normalVec.y + (u32)invWeight * (u32)d->normalVecUP.y) >> COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_SHIFT;
	blended.z =
	    (s32)((u32)normalBlendWeight * (u32)d->AxisAngle2_normalVec.z + (u32)invWeight * (u32)d->normalVecUP.z) >> COLL_FIXED_PLAYER_SEARCH_NORMAL_BLEND_SHIFT;

	if (d->hazardTimer > 0)
	{
		const struct TrigTable *table = data.trigApprox;
		s32 angle = d->hazardTimer * 12;
		s32 x = (s32)CTR_ReadU32AlignedLE(&table[ANG_MODULO_HALF_PI(angle)]);
		s32 z;
		if (!IS_ANG_FIRST_OR_THIRD_QUADRANT(angle))
		{
			z = (s16)x;
			x >>= 16;
			if (IS_ANG_THIRD_OR_FOURTH_QUADRANT(angle))
			{
				x = -x;
			}
			else
			{
				z = -z;
			}
		}
		else
		{
			z = x >> 16;
			x = (s32)((u32)x << 16);
			x >>= 16;
			if (IS_ANG_THIRD_OR_FOURTH_QUADRANT(angle))
			{
				z = -z;
				x = -x;
			}
		}

		x = (x * 25) >> 10;
		z = (z * 25) >> 10;
		MTC2((u32)(u16)x, 0);
		// NOTE(aalhendi): Transfer signed scalar Z, not a zero-packed pair of shorts.
		MTC2_S(z, 1);
		CTR_GteLoadDelay();
		gte_rtv0();
		x = MFC2_S(25);
		angle = MFC2_S(26);
		z = MFC2_S(27);
		blended.x = (s32)((u32)blended.x + (u32)x);
		blended.y = (s32)((u32)blended.y + (u32)angle);
		blended.z = (s32)((u32)blended.z + (u32)z);
	}

	{
		u32 sum = (u32)blended.x * (u32)blended.x + (u32)blended.y * (u32)blended.y + (u32)blended.z * (u32)blended.z;
		work = CollFixed_PlayerSearch_Length(sum);
#if defined(CTR_NATIVE)
		if (work == 0)
		{
			CTR_TRAP();
		}
#endif

		d->AxisAngle2_normalVec.x = (s16)((s32)((u32)blended.x << 12) / work);
		d->AxisAngle2_normalVec.y = (s16)((s32)((u32)blended.y << 12) / work);
		orientationTable = data.trigApprox;
		d->AxisAngle2_normalVec.z = (s16)((s32)((u32)blended.z << 12) / work);
	}
}

	{
		s32 x, z;
		work = d->angle;
		x = (s32)CTR_ReadU32AlignedLE(&orientationTable[ANG_MODULO_HALF_PI(work)]);
		if (!IS_ANG_FIRST_OR_THIRD_QUADRANT(work))
		{
			z = (s16)x;
			x >>= 16;
			if (IS_ANG_THIRD_OR_FOURTH_QUADRANT(work))
			{
				x = -x;
			}
			else
			{
				z = -z;
			}
		}
		else
		{
			z = x >> 16;
			x = (s32)((u32)x << 16);
			x >>= 16;
			if (IS_ANG_THIRD_OR_FOURTH_QUADRANT(work))
			{
				z = -z;
				x = -x;
			}
		}

		work = (s32)((u32)(-(s32)d->AxisAngle2_normalVec.x * z) + (u32)((s32)d->AxisAngle2_normalVec.z * x));
		d->rotCurr.z = ratan2(work >> 12, d->AxisAngle2_normalVec.y);
	}

	if (d->hazardTimer > 0)
	{
		s32 screenOffset = CollFixed_PlayerSearch_Abs((s8)d->Screen_OffsetY);

		if (screenOffset < 4)
		{
			d->distanceFromGround = 4;

			if ((d->kartState != KS_SPINNING) && ((s8)d->Screen_OffsetY > 0))
			{
				OtherFX_Play(0x10, 1);
			}
		}
	}
	else
	{
		if ((d->actionsFlagSet & ACTION_TOUCH_GROUND) != 0)
		{
			s32 speed = CollFixed_PlayerSearch_Abs(d->speed);

			if (speed > 0x1000)
			{
				s32 screenOffset = CollFixed_PlayerSearch_Abs((s8)d->Screen_OffsetY);

				if ((screenOffset < 4) && ((d->terrainMeta1->flags & TERRAIN_FLAG_RAISE_GROUND_OFFSET) != 0))
				{
					d->distanceFromGround = 4;
				}
				// NOTE(aalhendi): Keep the prior counter when either high-speed raise condition fails.
				goto UpdateGroundOffset;
			}
		}

		d->distanceFromGround = 0;
	}

UpdateGroundOffset:
	if (CollFixed_PlayerSearch_Abs((s8)d->Screen_OffsetY) > 9)
	{
		d->distanceFromGround = 0;
	}

	if (d->distanceFromGround != 0)
	{
		d->distanceFromGround--;
		d->Screen_OffsetY = (s8)((u8)d->Screen_OffsetY + 3u);
	}
	else
	{
		s8 nextOffset;

		if ((s8)d->Screen_OffsetY > 0)
		{
			nextOffset = (s8)((u8)d->Screen_OffsetY - 4u);
			d->Screen_OffsetY = nextOffset;

			if (nextOffset < 1)
			{
				d->Screen_OffsetY = 0;

				if ((d->terrainMeta1->flags & TERRAIN_FLAG_ONESHOT_GROUND_SOUND) != 0)
				{
					u32 soundID = (u16)d->terrainMeta1->sound;
					u32 soundFlags = 0x808080u;
					if ((d->actionsFlagSet & ACTION_ENGINE_ECHO) != 0)
					{
						soundFlags |= 0x1000000u;
					}

					OtherFX_Play_LowLevel(soundID, 0, soundFlags);
				}
			}
		}

		nextOffset = (s8)((u8)d->Screen_OffsetY - 4u);
		d->Screen_OffsetY = nextOffset;

		if (nextOffset < 0)
		{
			d->Screen_OffsetY = 0;
		}
	}

	if ((d->posCurr.y < -0x8000) && ((GAME_TRACKER->level1->configFlags & 2) != 0))
	{
		d->collisionFlags |= DRIVER_COLL_FLAG_MASK_GRAB_REQUEST;
	}

	if ((d->kartState != KS_MASK_GRABBED) && ((d->collisionFlags & DRIVER_COLL_FLAG_MASK_GRAB_REQUEST) != 0) && (d->lastValid != NULL) &&
	    ((GAME_SDATA_NAME(HudAndDebugFlags) & 0x1000) == 0) && ((d->stepFlagSet & COLL_STEP_TRIGGER_SUPPRESS_MASK_GRAB) == 0))
	{
		VehStuckProc_MaskGrab_Init(t, d);
	}
}
