#include <common.h>

static const s16 s_warpballFadeScale[6 * 3] = {
    4505, 5120, 4096, 5226, 8192, 4096, 5600, 5501, 4096, 5272, 3183, 4096, 4242, 1411, 3337, 2878, 437, 1668,
};

static const s32 s_warpballFadeY[6] = {
    -64, -256, -87, 57, 167, 228,
};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ae524-0x800ae604.
// NOTE(aalhendi): Native uses retail fade scale/Y table bytes from 0x800b2c88 and 0x800b2cac.
void RB_Warpball_FadeAway(struct Thread *t)
{
	s16 frameId;
	int iVar2;
	struct TrackerWeapon *tw;
	struct Instance *inst;
	struct Driver *d;
	struct GameTracker *gGT;

	gGT = sdata->gGT;

	tw = t->object;
	inst = t->inst;
	frameId = tw->fadeAway_frameCount5;

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

	tw->fadeAway_frameCount5 += 1;

	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ae604-0x800ae668.
void RB_Warpball_Death(struct Thread *t)
{
	struct TrackerWeapon *tw;

	tw = t->object;
	tw->ptrParticle->framesLeftInLife = 0;
	tw->fadeAway_frameCount5 = 0;

	// play sound of warpball death
	struct Instance *inst = t->inst;
	tw->distFromGround = inst->matrix.t[1];
	PlaySound3D(0x4f, inst);

	// stop audio of moving
	OtherFX_RecycleMute(&tw->audioPtr);

	ThTick_SetAndExec(t, &RB_Warpball_FadeAway);
	return;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800ae668-0x800ae778.
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

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800ae778-0x800ae7dc.
void RB_Warpball_Start(struct TrackerWeapon *tw)
{
	tw->ptrNodeCurr = RB_Warpball_NewPathNode(tw->ptrNodeCurr, tw->driverTarget);
	tw->ptrNodeNext = RB_Warpball_NewPathNode(tw->ptrNodeCurr, tw->driverTarget);
	return;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800ae7dc-0x800aeaac.
struct Driver *RB_Warpball_GetDriverTarget(struct TrackerWeapon *tw, struct Instance *inst)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *bestDriver = NULL;

	if ((tw->flags & 1) == 0)
	{
		for (int i = 0; i < 8; i++)
		{
			struct Driver *driver = gGT->driversInRaceOrder[i];

			if ((driver != NULL) && (driver != tw->driverParent) && ((driver->actionsFlagSet & ACTION_RACE_FINISHED) == 0))
			{
				return driver;
			}
		}

		return bestDriver;
	}

	struct CheckpointNode *nodes = gGT->level1->ptr_restart_points;
	struct CheckpointNode *node1 = &nodes[tw->ptrNodeCurr->nextIndex_forward];
	struct CheckpointNode *node2 = &nodes[node1->nextIndex_forward];
	int trackDistance = nodes[0].distToFinish << 3;
	SVec3 pathVector;
	SVec3 orbVector;
	s32 projectedDistance;
	int bestDistance = 0x7fffffff;

	pathVector.x = (s16)(node1->pos.x - node2->pos.x);
	pathVector.y = (s16)(node1->pos.y - node2->pos.y);
	pathVector.z = (s16)(node1->pos.z - node2->pos.z);
	MATH_VectorNormalize(&pathVector);

	orbVector.x = (s16)(inst->matrix.t[0] - node1->pos.x);
	orbVector.y = (s16)(inst->matrix.t[1] - node1->pos.y);
	orbVector.z = (s16)(inst->matrix.t[2] - node1->pos.z);

	// NOTE(aalhendi): Retail uses GTE MVMVA MAC1; this is the same row0 dot product.
	projectedDistance = ((s32)pathVector.x * orbVector.x) + ((s32)pathVector.y * orbVector.y) + ((s32)pathVector.z * orbVector.z);

	projectedDistance = ((node1->distToFinish << 3) + (projectedDistance >> 12) + 0x200) % trackDistance;

	for (int i = 0; i < 8; i++)
	{
		struct Driver *driver = gGT->drivers[i];

		if ((driver != NULL) && ((tw->driversHit & (1u << (i & 0x1f))) == 0) && ((driver->actionsFlagSet & ACTION_RACE_FINISHED) == 0) &&
		    (driver->kartState != KS_MASK_GRABBED))
		{
			int distance = projectedDistance - driver->distanceToFinish_curr;

			if (distance < 0)
			{
				distance += trackDistance;
			}

			if (distance < bestDistance)
			{
				bestDistance = distance;
				bestDriver = driver;
			}
		}
	}

	return bestDriver;
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800aeaac-0x800aece0.
void RB_Warpball_SetTargetDriver(struct TrackerWeapon *tw)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *target = tw->driverTarget;

	if (target == NULL)
	{
		return;
	}

	struct CheckpointNode *nodes = gGT->level1->ptr_restart_points;
	struct CheckpointNode *targetNode = &nodes[target->checkpoint.currentIndex];
	struct CheckpointNode *prevNode = targetNode;
	int targetDistance = target->distanceToFinish_curr;

	while ((((int)targetNode->distToFinish << 3) >= targetDistance) && (targetNode != nodes))
	{
		prevNode = targetNode;
		targetNode = RB_Warpball_NewPathNode(targetNode, tw->driverTarget);
	}
	targetNode = prevNode;

	struct CheckpointNode *rightPathNode = NULL;

	if ((tw->flags & 4) == 0)
	{
		struct CheckpointNode *pathStarts[2];

		pathStarts[0] = tw->ptrNodeCurr;
		pathStarts[1] = rightPathNode;

		for (int i = 0; (i < 2) && ((tw->flags & 4) == 0); i++)
		{
			struct CheckpointNode *pathNode = pathStarts[i];

			if (pathNode == NULL)
			{
				continue;
			}

			for (int j = 0; j < 3; j++)
			{
				if (pathNode == targetNode)
				{
					tw->flags = (tw->flags & ~8) | 4;
					break;
				}

				if (pathNode->nextIndex_right != 0xff)
				{
					rightPathNode = &nodes[pathNode->nextIndex_right];
					pathStarts[1] = rightPathNode;
				}

				pathNode = &nodes[pathNode->nextIndex_backward];
			}
		}
	}

	struct CheckpointNode *pathNode = tw->ptrNodeCurr;

	for (int i = 0; i < 3; i++)
	{
		if (pathNode == targetNode)
		{
			tw->flags = (tw->flags & ~8) | 4;
			return;
		}

		pathNode = RB_Warpball_NewPathNode(pathNode, tw->driverTarget);
	}
}

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800aece0-0x800aede0.
void RB_Warpball_SeekDriver(struct TrackerWeapon *tw, u32 param_2, struct Driver *d)
{
	param_2 &= 0xff;

	if (d == 0)
	{
		return;
	}
	if (param_2 == 0xff)
	{
		return;
	}

	struct CheckpointNode *first = &sdata->gGT->level1->ptr_restart_points[0];

	// pointer to path node
	struct CheckpointNode *cn = &first[param_2];

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

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800aede0-0x800aef9c.
void RB_Warpball_TurnAround(struct Thread *t)
{
	struct TrackerWeapon *tw;
	struct Instance *inst;
	u16 flags;
	struct GameTracker *gGT = sdata->gGT;
	s16 rot;

	tw = t->object;
	inst = t->inst;
	flags = tw->flags;

	if (
	    // if not snap to point???
	    ((flags & 0x100) != 0) ||

	    // if no driver is being chased
	    (tw->driverTarget == NULL))
	{
		if ((flags & 4) != 0)
		{
			tw->flags = (flags & 0xfffb) | 0x208;
		}

		tw->vel.x = -tw->vel.x;
		tw->vel.y = -tw->vel.y;
		tw->vel.z = -tw->vel.z;

		inst->matrix.t[0] += ((int)tw->vel.x * gGT->elapsedTimeMS) >> 5;
		inst->matrix.t[1] += ((int)tw->vel.y * gGT->elapsedTimeMS) >> 5;
		inst->matrix.t[2] += ((int)tw->vel.z * gGT->elapsedTimeMS) >> 5;

		// increment counter
		tw->turnAround++;

		if (
		    // if count too high
		    (0x78 < tw->turnAround) ||

		    // pointer to driver being chased,
		    // is null, so warpball is chasing nobody
		    (tw->driverTarget == 0))
		{
			tw->driverParent->instBombThrow = 0;

			// play sound warpball death
			PlaySound3D(0x4f, inst);

			RB_Warpball_Death(t);
		}

		// if attempted to turn around 3 times
		if ((tw->turnAround & 3) == 0)
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

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800aef9c-0x800afb70.
// NOTE(aalhendi): Native uses the extracted warpball particle-height halfword from RDATA 0x800b2c84.
void RB_Warpball_ThTick(struct Thread *t)
{
	struct GameTracker *gGT;
	struct TrackerWeapon *tw;
	struct Instance *inst;
	struct Driver *target;
	struct ScratchpadStruct *sps;
	struct Instance *hitInst;
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

	((s16 *)&tw->unk4c)[0] = (s16)inst->matrix.t[0];
	((s16 *)&tw->unk4c)[1] = (s16)inst->matrix.t[1];
	tw->unk50 = (s16)inst->matrix.t[2];

	if ((int)inst->animFrame + 1 < INSTANCE_GetNumAnimFrames(inst, 0))
	{
		inst->animFrame++;
	}
	else
	{
		inst->animFrame = 0;
	}

	if (tw->driverTarget != NULL)
	{
		if ((tw->driverTarget->kartState == KS_MASK_GRABBED) && ((tw->flags & 4) != 0))
		{
			struct CheckpointNode *nodes = gGT->level1->ptr_restart_points;

			tw->flags = (tw->flags & 0xfffb) | 0x18;
			tw->ptrNodeCurr = &nodes[tw->nodeNextIndex];
			tw->ptrNodeNext = RB_Warpball_NewPathNode(tw->ptrNodeCurr, tw->driverTarget);
			tw->driverTarget = RB_Warpball_GetDriverTarget(tw, inst);
			RB_Warpball_SetTargetDriver(tw);
		}

		if ((tw->flags & 0x204) == 0)
		{
			tw->driverTarget = RB_Warpball_GetDriverTarget(tw, inst);

			if (tw->driverTarget != NULL)
			{
				RB_Warpball_SetTargetDriver(tw);
			}
		}
	}

	target = tw->driverTarget;
	tw->flags &= 0xfdff;
	elapsedTime = gGT->elapsedTimeMS;

	if (target != NULL)
	{
		distX = (target->posCurr.x >> 8) - inst->matrix.t[0];
		distZ = (target->posCurr.z >> 8) - inst->matrix.t[2];
		distY = (target->posCurr.y >> 8) - inst->matrix.t[1];
		distXZ = (distX * distX) + (distZ * distZ);
		tw->distanceToTarget = distXZ;
		target->thTrackingMe = RB_GetThread_ClosestTracker(target);

		if ((tw->flags & 0xc) != 0)
		{
			s16 rotSpeed = 0x100;

			if ((tw->flags & 4) == 0)
			{
				struct CheckpointNode *pathNode = tw->ptrNodeCurr;

				distX = pathNode->pos.x - inst->matrix.t[0];
				distZ = pathNode->pos.z - inst->matrix.t[2];
				distY = pathNode->pos.y - inst->matrix.t[1];
				distXZ = (distX * distX) + (distZ * distZ);

				if (distXZ < 0x4000)
				{
					tw->flags = (tw->flags & 0xfff7) | 0x100;
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

			if (tw->frameCount_DontHurtParent > 0)
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
			int progress = tw->respawnPointIndex + ((elapsedTime * 0x70) >> 5);
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

			tw->respawnPointIndex = progress;
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

	PlaySound3D_Flags((u32 *)&tw->audioPtr, 0x4e, inst);

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

	if ((sps->collision.stepFlags & COLL_STEP_TRIGGER_WEAPON_REACT) != 0)
	{
		RB_Warpball_TurnAround(t);
	}

	if (sps->boolDidTouchHitbox != 0)
	{
		sps->Input1.modelID = 0x36;

		if (RB_Hazard_CollLevInst(sps, t) == 1)
		{
			RB_Warpball_TurnAround(t);
		}
	}

	if (sps->boolDidTouchQuadblock != 0)
	{
		tw->flags |= 0x100;
		RB_Warpball_SetQuadblockIndex(tw, sps);
		tw->vel.y = 0;

		if (((tw->flags & 0xc) != 0) && (inst->matrix.t[1] < sps->hit.hitPos.y))
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
			tw->flags |= 0x100;
			RB_Warpball_SetQuadblockIndex(tw, sps);
		}

		if ((sps->boolDidTouchQuadblock == 0) && (((tw->flags & 0xc) != 0) || (tw->driverTarget == NULL)))
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

	hitInst = RB_Hazard_CollideWithDrivers(inst, tw->frameCount_DontHurtParent, 0x9000, tw->instParent);

	if (hitInst != NULL)
	{
		struct Driver *hitDriver = hitInst->thread->object;

		if (hitDriver != tw->driverParent)
		{
			u16 hadTargetPathFlag = tw->flags & 4;
			u16 flagsBeforeHit;

			RB_Hazard_HurtDriver(hitDriver, 2, tw->driverParent, 0);
			hitDriver->damageColorTimer = 0x1e;

			flagsBeforeHit = tw->flags | 0x40;
			tw->flags = flagsBeforeHit;

			if ((((flagsBeforeHit & 1) == 0) && (tw->driverTarget == hitDriver)) || (hitDriver->driverRank == 0))
			{
				tw->driverParent->instBombThrow = NULL;
				RB_Warpball_Death(t);
				return;
			}

			tw->driversHit |= 1u << (hitDriver->driverID & 0x1f);

			for (int rank = hitDriver->driverRank; rank < 8; rank++)
			{
				struct Driver *rankDriver = gGT->driversInRaceOrder[rank];

				if (rankDriver != NULL)
				{
					tw->driversHit |= 1u << (rankDriver->driverID & 0x1f);
				}
			}

			if (tw->driverTarget == hitDriver)
			{
				tw->flags &= 0xfffb;
				tw->driverTarget = RB_Warpball_GetDriverTarget(tw, inst);

				if (tw->driverTarget == NULL)
				{
					tw->driverParent->instBombThrow = NULL;
					RB_Warpball_Death(t);
				}
				else
				{
					RB_Warpball_SetTargetDriver(tw);
				}

				if (hadTargetPathFlag != 0)
				{
					RB_Warpball_SeekDriver(tw, hitDriver->checkpoint.currentIndex, hitDriver);
				}

				if (((tw->flags & 4) == 0) && (hadTargetPathFlag != 0))
				{
					if (tw->nodeCurrIndex != 0xff)
					{
						struct CheckpointNode *nodes = gGT->level1->ptr_restart_points;

						tw->ptrNodeCurr = &nodes[tw->nodeCurrIndex];
						tw->ptrNodeNext = RB_Warpball_NewPathNode(tw->ptrNodeCurr, tw->driverTarget);
					}

					tw->flags |= 8;
				}
			}
		}
	}
	else
	{
		hitInst = RB_Hazard_CollideWithBucket(inst, t, gGT->threadBuckets[MINE].thread, tw->frameCount_DontHurtParent, 0x2400, tw->instParent);

		if (hitInst != NULL)
		{
			struct Thread *hitTh = hitInst->thread;

			hitTh->funcThCollide(hitTh);
		}
	}

	if (tw->frameCount_DontHurtParent != 0)
	{
		tw->frameCount_DontHurtParent--;
	}
}
