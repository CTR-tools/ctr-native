#include <common.h>

static const s16 s_warpballFadeScale[6 * 3] = {
    4505, 5120, 4096, 5226, 8192, 4096, 5600, 5501, 4096, 5272, 3183, 4096, 4242, 1411, 3337, 2878, 437, 1668,
};

static const s32 s_warpballFadeY[6] = {
    -64, -256, -87, 57, 167, 228,
};

u16 store_wheelsize = 0;

// NOTE(aalhendi): Native uses retail fade scale/Y table bytes from 0x800b2c88 and 0x800b2cac.
void RB_Warpball_FadeAway(struct Thread *t)
{
	s16 frameId;
	struct TrackerWeapon *tw;
	struct Instance *inst;
	struct Driver *d;
	struct GameTracker *gGT;

	gGT = sdata->gGT;

	tw = t->object;
	inst = t->inst;
	frameId = tw->fadeFrame;

	if (frameId > 5)
	{
		d = tw->driverTarget;

		if (d != NULL)
		{
			// remove 2D square-target being drawn on the player's screen
			d->actionsFlagSet &= ~ACTION_TRACKER_TARGETED;
		}

		// remove active warpball flag
		gGT->gameMode1 &= ~(WARPBALL_HELD);

		// This thread is now dead
		t->flags |= THREAD_FLAG_DEAD;
		return;
	}

	// set scale (x, y, z)
	inst->scale.x = s_warpballFadeScale[(frameId * 3) + 0];
	inst->scale.y = s_warpballFadeScale[(frameId * 3) + 1];
	inst->scale.z = s_warpballFadeScale[(frameId * 3) + 2];

	inst->matrix.t[1] = tw->distFromGround + s_warpballFadeY[frameId];

	tw->fadeFrame += 1;

	return;
}

void RB_Warpball_Death(struct Thread *t)
{
	struct TrackerWeapon *tw;
	struct Instance *inst;
	struct Driver *d;

	tw = t->object;
	inst = t->inst;

	d = tw->driverParent;

	s16 exitDir = tw->dir.x;

	d->forwardDir = exitDir;
    d->jump_ForcedMS = 600;
    d->jump_InitialVelY = d->const_JumpForce * 3;
	VehFire_Increment(tw->driverParent, VEH_PHYS_PROC_SUPER_ENGINE_RESERVES, (TURBO_PAD | SUPER_ENGINE), 128);
	d->wheelSize = store_wheelsize;
	d->invisibleTimer = 0;

	tw->ptrParticle->framesLeftInLife = 0;
	tw->fadeFrame = 0;

	// play sound of warpball death
	tw->distFromGround = inst->matrix.t[1];
	PlaySound3D(0x4f, inst);

	// stop audio of moving
	OtherFX_RecycleMute(&tw->soundIDCount);

	ThTick_SetAndExec(t, &RB_Warpball_FadeAway);
	return;
}

struct CheckpointNode *RB_Warpball_NewPathNode(struct CheckpointNode *cn, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;
	u8 pathIndex;
	u8 targetIndex;
	int foundLeftPath;

	if (d == NULL)
	{
		return &gGT->level1->ptr_restart_points[cn->nextIndex_forward];
	}

	foundLeftPath = 0;
	targetIndex = d->checkpoint.branchChoiceIndex;
	pathIndex = cn->nextIndex_left;

	if (targetIndex == pathIndex)
	{
		return &gGT->level1->ptr_restart_points[pathIndex];
	}

	if (pathIndex != 0xff)
	{
		struct CheckpointNode *currNode = cn;

		for (int i = 0; i < 3; i++)
		{
			if (currNode->nextIndex_left == 0xff)
			{
				pathIndex = currNode->nextIndex_forward;
			}
			else
			{
				pathIndex = currNode->nextIndex_left;
			}

			currNode = &gGT->level1->ptr_restart_points[pathIndex];

			if (targetIndex == currNode->nextIndex_forward)
			{
				foundLeftPath = 1;
				break;
			}
		}
	}

	if (foundLeftPath)
	{
		return &gGT->level1->ptr_restart_points[cn->nextIndex_left];
	}

	return &gGT->level1->ptr_restart_points[cn->nextIndex_forward];
}

void RB_Warpball_Start(struct TrackerWeapon *tw)
{
	//tw->driverTarget = tw->driverParent;
	store_wheelsize = tw->driverParent->wheelSize;

	tw->orbTimeAlive = 0;
	tw->ptrNodeCurr = RB_Warpball_NewPathNode(tw->ptrNodeCurr, tw->driverTarget);
	tw->ptrNodeNext = RB_Warpball_NewPathNode(tw->ptrNodeCurr, tw->driverTarget);
	return;
}

static const s16 s_warpballParticleHeight = 0xff;

static void RB_Warpball_AdvanceStraight(struct TrackerWeapon *tw, struct Instance *inst, int elapsedTime)
{
	inst->matrix.t[0] += ((int)tw->vel.x * elapsedTime) >> 5;
	inst->matrix.t[1] += ((int)tw->vel.y * elapsedTime) >> 5;
	inst->matrix.t[2] += ((int)tw->vel.z * elapsedTime) >> 5;
}

static int RB_Warpball_NodeDeltaLength(struct CheckpointNode *curr, struct CheckpointNode *next, int *dx, int *dy, int *dz)
{
	*dx = next->pos.x - curr->pos.x;
	*dy = next->pos.y - curr->pos.y;
	*dz = next->pos.z - curr->pos.z;

	return SquareRoot0_stub(((*dx) * (*dx)) + ((*dy) * (*dy)) + ((*dz) * (*dz)));
}

static void RB_Warpball_SetQuadblockIndex(struct TrackerWeapon *tw, struct ScratchpadStruct *sps)
{
	if (sps->hit.ptrQuadblock->checkpointIndex != 0xff)
	{
		tw->nodeNextIndex = sps->hit.ptrQuadblock->checkpointIndex;
	}
}

void RB_Warpball_TurnAround(struct Thread *t)
{
	struct TrackerWeapon *tw;
	struct Instance *inst;
	TrackerWeaponFlags flags;
	struct GameTracker *gGT = sdata->gGT;
	s16 rot;

	tw = t->object;
	inst = t->inst;
	flags = tw->flags;

	if (
	    // if turnaround was requested
	    ((flags & TRACKER_FLAG_WARPBALL_TURN_AROUND) != 0) ||

	    // if no driver is being chased
	    (tw->driverTarget == NULL))
	{
		if ((flags & TRACKER_FLAG_WARPBALL_TARGET_PATH) != 0)
		{
			tw->flags = (flags & ~TRACKER_FLAG_WARPBALL_TARGET_PATH) | TRACKER_FLAG_WARPBALL_BACKTRACKING | TRACKER_FLAG_WARPBALL_FALLBACK_PATH;
		}

		tw->vel.x = -tw->vel.x;
		tw->vel.y = -tw->vel.y;
		tw->vel.z = -tw->vel.z;

		inst->matrix.t[0] += ((int)tw->vel.x * gGT->elapsedTimeMS) >> 5;
		inst->matrix.t[1] += ((int)tw->vel.y * gGT->elapsedTimeMS) >> 5;
		inst->matrix.t[2] += ((int)tw->vel.z * gGT->elapsedTimeMS) >> 5;

		// increment counter
		tw->turnAroundFrames++;

		// if attempted to turn around 3 times
		if ((tw->turnAroundFrames & 3) == 0)
		{
			tw->ptrNodeNext = tw->ptrNodeCurr;

			struct CheckpointNode *first = &sdata->gGT->level1->ptr_restart_points[0];

			// set new end to 10 path indices ahead of current
			tw->ptrNodeCurr = &first[tw->ptrNodeCurr->nextIndex_backward];
		}

		struct CheckpointNode *cn = tw->ptrNodeCurr;

		// rotation
		rot = ratan2(cn->pos.x - inst->matrix.t[0], cn->pos.z - inst->matrix.t[2]);

		// rotation
		tw->dir.y = rot;
	}
	return;
}

void RB_Warpball_SeekDriver(struct TrackerWeapon *tw, u32 checkpointIndex, struct Driver *d)
{
	checkpointIndex &= 0xff;

	/*
	if (d == 0)
	{
		return;
	}
	if (checkpointIndex == 0xff)
	{
		return;
	}
	*/

	struct CheckpointNode *first = &sdata->gGT->level1->ptr_restart_points[0];

	// pointer to path node
	struct CheckpointNode *cn = &first[checkpointIndex];

	while ((d->distanceToFinish_curr <= (u32)(cn->distToFinish << 3)) &&

	       // node is not first node
	       (cn != first))
	{
		cn = RB_Warpball_NewPathNode(cn, tw->driverTarget);
	}

	// path index = pathPtr2 - pathPtr1
	tw->nodeCurrIndex = (u8)(cn - first);

	return;
}

// NOTE(aalhendi): Native uses the extracted warpball particle-height halfword from RDATA 0x800b2c84.
void RB_Warpball_ThTick(struct Thread *t)
{
	struct GameTracker *gGT;
	struct TrackerWeapon *tw;
	struct Instance *inst;
	struct Driver *target;
	struct ScratchpadStruct *sps;
	SVec3 posTop;
	SVec3 posBottom;
	int elapsedTime;
	int distX;
	int distY;
	int distZ;
	int distXZ;

	gGT = sdata->gGT;
	inst = t->inst;
	tw = t->object;
	tw->orbTimeAlive += gGT->elapsedTimeMS;
	int juiced_duration = 2000;
	int normal_duration = 1600;

	if (tw->driverParent->invisibleTimer == 0)
	{
		tw->driverParent->wheelSize = 0;
		tw->driverParent->instFlagsBackup = tw->driverParent->instSelf->flags;
		tw->driverParent->instSelf->flags = (tw->driverParent->instSelf->flags & INVISIBILITY_CLEAR_DRAW_FLAGS) | GHOST_DRAW_TRANSPARENT;
		int time = normal_duration;
		if (tw->driverParent->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
		{
			time = juiced_duration;
		}

		tw->driverParent->invisibleTimer = time;
	}

	if (tw->driverParent->numWumpas >= DRIVER_WUMPA_JUICED_COUNT)
	{
		if (tw->orbTimeAlive >= juiced_duration)
		{
			RB_Warpball_Death(t);
		}
	}
	else
	{
		if (tw->orbTimeAlive >= normal_duration)
		{
			RB_Warpball_Death(t);
		}
	}

	CTR_WriteU16LE(&tw->savedPosXY, (u16)inst->matrix.t[0]);
	CTR_WriteU16LE((u8 *)&tw->savedPosXY + 2, (u16)inst->matrix.t[1]);
	tw->savedPosZ = (s16)inst->matrix.t[2];

	if ((int)inst->animFrame + 1 < INSTANCE_GetNumAnimFrames(inst, 0))
	{
		inst->animFrame++;
	}
	else
	{
		inst->animFrame = 0;
	}

	target = tw->driverTarget;
	tw->flags &= ~TRACKER_FLAG_WARPBALL_BACKTRACKING;
	elapsedTime = gGT->elapsedTimeMS;

	if (target != NULL)
	{
		distX = (target->posCurr.x >> 8) - inst->matrix.t[0];
		distZ = (target->posCurr.z >> 8) - inst->matrix.t[2];
		distY = (target->posCurr.y >> 8) - inst->matrix.t[1];
		distXZ = (distX * distX) + (distZ * distZ);
		tw->distanceToTarget = distXZ;

		if ((tw->flags & TRACKER_FLAG_WARPBALL_PATH_MODE) != 0)
		{
			s16 rotSpeed = 0x100;

			if ((tw->flags & TRACKER_FLAG_WARPBALL_TARGET_PATH) == 0)
			{
				struct CheckpointNode *pathNode = tw->ptrNodeCurr;

				distX = pathNode->pos.x - inst->matrix.t[0];
				distZ = pathNode->pos.z - inst->matrix.t[2];
				distY = pathNode->pos.y - inst->matrix.t[1];
				distXZ = (distX * distX) + (distZ * distZ);

				if (distXZ < 0x4000)
				{
					tw->flags = (tw->flags & ~TRACKER_FLAG_WARPBALL_FALLBACK_PATH) | TRACKER_FLAG_WARPBALL_TURN_AROUND;
				}
				else if (distXZ < 0x24000)
				{
					rotSpeed = 0x400;
				}
			}

			if (distXZ < 0x90000)
			{
				rotSpeed = 0x400 - (distXZ >> 9);

				if (rotSpeed < 0x100)
				{
					rotSpeed = 0x100;
				}
			}

			if (tw->parentSafetyFrames > 0)
			{
				rotSpeed = 0x40;
			}

			tw->dir.x = 0;
			tw->dir.y = RB_Hazard_InterpolateValue(tw->dir.y, ratan2(distX, distZ), rotSpeed);
			tw->dir.z = 0;
			tw->vel.x = (MATH_Sin(tw->dir.y) * 7) >> 8;
			tw->vel.z = (MATH_Cos(tw->dir.y) * 7) >> 8;

			if (distY > 0)
			{
				tw->vel.y += (elapsedTime << 2) >> 5;

				if (distY < tw->vel.y)
				{
					tw->vel.y = distY;
				}

				if (tw->vel.y > 0x60)
				{
					tw->vel.y = 0x60;
				}
			}
			else if (distY < 0)
			{
				tw->vel.y -= (elapsedTime << 2) >> 5;

				if (tw->vel.y < distY)
				{
					tw->vel.y = distY;
				}

				if (tw->vel.y < -0x60)
				{
					tw->vel.y = -0x60;
				}
			}

			RB_Warpball_AdvanceStraight(tw, inst, elapsedTime);
		}
		else
		{
			struct CheckpointNode *curr = tw->ptrNodeCurr;
			struct CheckpointNode *next = tw->ptrNodeNext;
			int segmentLength = RB_Warpball_NodeDeltaLength(curr, next, &distX, &distY, &distZ);
			int progress = tw->pathProgress + ((elapsedTime * 0xd0) >> 5);
			int fraction;

			if (segmentLength <= progress)
			{
				progress -= segmentLength;

				do
				{
					int keepAdvancing;

					curr = next;
					next = RB_Warpball_NewPathNode(curr, tw->driverTarget);
					segmentLength = RB_Warpball_NodeDeltaLength(curr, next, &distX, &distY, &distZ);
					keepAdvancing = segmentLength <= progress;
					progress -= segmentLength;

					if (!keepAdvancing)
					{
						break;
					}
				} while (1);

				progress += segmentLength;
			}

			tw->pathProgress = progress;
			tw->ptrNodeCurr = curr;
			tw->ptrNodeNext = next;

			if (segmentLength == 0)
			{
				fraction = 0;
			}
			else
			{
				fraction = (progress << 12) / segmentLength;
			}

			inst->matrix.t[0] = curr->pos.x + ((distX * fraction) >> 12);
			inst->matrix.t[1] = curr->pos.y + ((distY * fraction) >> 12);
			inst->matrix.t[2] = curr->pos.z + ((distZ * fraction) >> 12);

			tw->dir.y = ratan2(distX, distZ);
			tw->vel.x = (MATH_Sin(tw->dir.y) * 7) >> 8;
			tw->vel.z = (MATH_Cos(tw->dir.y) * 7) >> 8;
			tw->vel.y = 0;
		}
	}
	else
	{
		RB_Warpball_AdvanceStraight(tw, inst, elapsedTime);
	}

	int vertical_offset = 1;
	tw->driverParent->posCurr.x = inst->matrix.t[0] <<8;
	tw->driverParent->posCurr.y = (inst->matrix.t[1] + vertical_offset) << 8;
	tw->driverParent->posCurr.z = inst->matrix.t[2] << 8;

	tw->driverParent->instSelf->matrix.t[0] = inst->matrix.t[0];
	tw->driverParent->instSelf->matrix.t[1] = (inst->matrix.t[1] + vertical_offset);
	tw->driverParent->instSelf->matrix.t[2] = inst->matrix.t[2];

	tw->driverParent->rotCurr.x = tw->dir.y;
	if (tw->orbTimeAlive > 400) 
	{
		tw->driverParent->angle = tw->dir.y;
	}
	tw->driverParent->forwardDir = tw->dir.x;
	tw->driverParent->reserves += 12;

	PlaySound3D_Flags(&tw->soundIDCount, 0x4e, inst);

	posTop.x = (s16)inst->matrix.t[0];
	posTop.y = (s16)(inst->matrix.t[1] - 0x80);
	posTop.z = (s16)inst->matrix.t[2];
	posBottom.x = (s16)inst->matrix.t[0];
	posBottom.y = (s16)(inst->matrix.t[1] + 0x80);
	posBottom.z = (s16)inst->matrix.t[2];

	sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
	sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND | QUADBLOCK_FLAG_TRIGGER;
	sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
	sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES | COLL_SEARCH_FORCE_INSTANCE_HIT;

	if (gGT->numPlyrCurrGame < 3)
	{
		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES | COLL_SEARCH_HIGH_LOD | COLL_SEARCH_FORCE_INSTANCE_HIT;
	}

	sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;
	COLL_SearchBSP_CallbackQUADBLK(&posTop, &posBottom, sps, 0);
	RB_MakeInstanceReflective(sps, inst);

	if ((tw->driverParent->actionsFlagSet & ACTION_BOT) != 0) {
		WarpTurbo_ResyncBotNav(tw->driverParent);
	}

	if ((sps->collision.stepFlags & COLL_STEP_TRIGGER_WEAPON_REACT) != 0)
	{
		RB_Warpball_TurnAround(t);
	}

	if (sps->boolDidTouchHitbox != 0)
	{
		sps->Input1.modelID = DYNAMIC_WARPBALL;

		if (RB_Hazard_CollLevInst(sps, t) == 1)
		{
			RB_Warpball_TurnAround(t);
		}
	}

	if (sps->boolDidTouchQuadblock != 0)
	{
		tw->flags |= TRACKER_FLAG_WARPBALL_TURN_AROUND;
		RB_Warpball_SetQuadblockIndex(tw, sps);
		tw->vel.y = 0;

		if (((tw->flags & TRACKER_FLAG_WARPBALL_PATH_MODE) != 0) && (inst->matrix.t[1] < sps->hit.hitPos.y))
		{
			inst->matrix.t[1] = sps->hit.hitPos.y;
			inst->depthBiasNormal = sps->hit.ptrQuadblock->draw_order_low - 1;
		}
	}
	else
	{
		posTop.x = (s16)inst->matrix.t[0];
		posTop.y = (s16)(inst->matrix.t[1] - 0x900);
		posTop.z = (s16)inst->matrix.t[2];
		COLL_SearchBSP_CallbackQUADBLK(&posTop, &posBottom, sps, 0);

		if (sps->boolDidTouchQuadblock != 0)
		{
			tw->flags |= TRACKER_FLAG_WARPBALL_TURN_AROUND;
			RB_Warpball_SetQuadblockIndex(tw, sps);
		}

		if ((sps->boolDidTouchQuadblock == 0) && (((tw->flags & TRACKER_FLAG_WARPBALL_PATH_MODE) != 0) || (tw->driverTarget == NULL)))
		{
			RB_Warpball_TurnAround(t);
		}
	}

	if ((s_warpballParticleHeight != 0) && (tw->ptrParticle != NULL))
	{
		struct Particle *p = tw->ptrParticle;

		p->axis[0].startVal = inst->matrix.t[0] << 8;
		p->axis[1].startVal = (inst->matrix.t[1] + s_warpballParticleHeight) << 8;
		p->axis[2].startVal = inst->matrix.t[2] << 8;
		p->axis[3].startVal = s_warpballParticleHeight << 8;
		p->axis[4].startVal = s_warpballParticleHeight * 0xc0;
		p->axis[5].startVal = s_warpballParticleHeight << 7;
		p->otIndexOffset = inst->depthBiasNormal + 1;
		p->framesLeftInLife = -1;
	}
	

	if (tw->parentSafetyFrames != 0)
	{
		tw->parentSafetyFrames--;
	}
}

void WarpTurbo_ResyncBotNav(struct Driver *bot)
{
    const int path = bot->botData.botPath;
    struct NavFrame *first = sdata->NavPath_ptrNavFrameArray[path];
    struct NavFrame *last = sdata->NavPath_ptrHeader[path]->last;
    struct NavFrame *best = first;
    int bestDistSq = INT_MAX;

    const int x = bot->posCurr.x >> 8;
    const int y = bot->posCurr.y >> 8;
    const int z = bot->posCurr.z >> 8;

    for (struct NavFrame *frame = first; frame < last; frame++)
    {
        int dx = frame->pos.x - x;
        int dy = frame->pos.y - y;
        int dz = frame->pos.z - z;
        int distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            best = frame;
        }
    }

    bot->botData.botNavFrame = best;
    bot->botData.positionBackup = bot->posCurr;
    bot->botData.navProgressRemainder = 0;
    BOTS_SetRotation(bot, 0);
}
