#include <common.h>

static void VehStuckProc_MaskGrab_SearchBsp(struct Driver *d, struct ScratchpadStruct *sps)
{
	struct GameTracker *gGT = sdata->gGT;
	s16 topX = (s16)CTR_MipsSra(d->posCurr.x, 8);
	s16 topY = (s16)CTR_MipsSra(d->posCurr.y, 8);
	s16 topZ = (s16)CTR_MipsSra(d->posCurr.z, 8);
	s16 bottomY = (s16)CTR_MipsSubLo(topY, 0x100);

	sps->Input1.pos.x = topX;
	sps->Input1.pos.y = bottomY;
	sps->Input1.pos.z = topZ;

	sps->Union.QuadBlockColl.pos.x = topX;
	sps->Union.QuadBlockColl.pos.y = topY;
	sps->Union.QuadBlockColl.pos.z = topZ;

	sps->Union.QuadBlockColl.searchFlags = 0;
	if (gGT->numPlyrCurrGame < 3)
	{
		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;
	}

	sps->boolDidTouchQuadblock = 0;
	sps->numTrianglesTested = 0;
	sps->hitFraction = COLL_FRACTION_ONE;
	sps->collision.stepFlags = 0;

	sps->bbox.min.x = topX;
	sps->bbox.max.x = topX;
	sps->bbox.min.y = (bottomY < topY) ? bottomY : topY;
	sps->bbox.max.y = (topY < bottomY) ? bottomY : topY;
	sps->bbox.min.z = topZ;
	sps->bbox.max.z = topZ;

	sps->Union.QuadBlockColl.hitPos = sps->Input1.pos;

	COLL_SearchBSP_CallbackPARAM(sps->ptr_mesh_info->bspRoot, &sps->bbox, COLL_FIXED_BSPLEAF_TestQuadblocks, sps);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006677c-0x80066cb0.
void VehStuckProc_MaskGrab_FindDestPos(struct Driver *d, struct QuadBlock *quad)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Level *level = gGT->level1;
	struct mesh_info *mesh = level->ptr_mesh_info;

	if ((level->cnt_restart_points < 1) || (level->ptr_restart_points == NULL) || (quad->checkpointIndex == 0xff))
	{
		struct LevVertex *verts = mesh->ptrVertexArray;
		struct LevVertex *v0 = &verts[quad->index[0]];
		struct LevVertex *v3 = &verts[quad->index[3]];

		d->posCurr.x = CTR_MipsSll(CTR_MipsAddLo(v0->pos.x, v3->pos.x), 7);
		d->posCurr.y = CTR_MipsSll(CTR_MipsAddLo(CTR_MipsAddLo(v0->pos.y, v3->pos.y), 0x80), 7);
		d->posCurr.z = CTR_MipsSll(CTR_MipsAddLo(v0->pos.z, v3->pos.z), 7);
	}
	else
	{
		struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
		struct Thread *driverThread = d->instSelf->thread;
		struct CheckpointNode *respawn = &level->ptr_restart_points[quad->checkpointIndex];
		struct CheckpointNode *nextRespawn;

		sps->Input1.hitRadius = driverThread->driverHitRadius;
		sps->Input1.hitRadiusSquared = driverThread->driverHitRadiusSquared;
		sps->Union.QuadBlockColl.hitRadius = driverThread->driverHitRadius;
		sps->Union.QuadBlockColl.hitRadiusSquared = driverThread->driverHitRadiusSquared;
		sps->ptr_mesh_info = mesh;
		sps->Union.QuadBlockColl.quadFlagsIgnored = QUADBLOCK_FLAG_NO_CAMERA_RESPAWN_PROBE | QUADBLOCK_FLAG_NO_COLLISION_RESPONSE;
		sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND;
		d->distanceDrivenBackwards = 0;

		do
		{
			do
			{
				nextRespawn = &level->ptr_restart_points[respawn->nextIndex_forward];

				d->posCurr.x = CTR_MipsSll(respawn->pos.x, 8);
				d->posCurr.y = CTR_MipsSll(CTR_MipsAddLo(respawn->pos.y, 0x80), 8);
				d->posCurr.z = CTR_MipsSll(respawn->pos.z, 8);

				d->rotCurr.x = 0;
				d->rotCurr.y = ratan2(CTR_MipsSubLo(nextRespawn->pos.x, respawn->pos.x), CTR_MipsSubLo(nextRespawn->pos.z, respawn->pos.z));
				d->rotCurr.z = 0;

				VehStuckProc_MaskGrab_SearchBsp(d, sps);
				respawn = nextRespawn;
			} while ((sps->boolDidTouchQuadblock == 0) || ((sps->collision.stepFlags & COLL_STEP_FLAG_KILL_PLANE) != 0));

			struct Thread *playerThread = gGT->threadBuckets[PLAYER].thread;
			while (playerThread != NULL)
			{
				struct Driver *other = playerThread->object;

				if (other != d)
				{
					int diffX = CTR_MipsSubLo(d->posCurr.x, other->posCurr.x);
					int diffZ = CTR_MipsSubLo(d->posCurr.z, other->posCurr.z);

					if (diffX < 0)
						diffX = CTR_MipsNegLo(diffX);

					if (diffX < 0x2000)
						break;

					if (diffZ < 0)
						diffZ = CTR_MipsNegLo(diffZ);

					if (diffZ < 0x2000)
						break;
				}

				playerThread = playerThread->siblingThread;
			}

			if (playerThread == NULL)
				break;
		} while (1);
	}

	gGT->cameraDC[d->driverID].flags |= 1;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80066cb0-0x80066d4c.
void VehStuckProc_MaskGrab_Particles(struct Driver *d)
{
	struct Particle *p;

	for (char i = 10; i > 0; i--)
	{
		// Create instance in particle pool
		p = Particle_Init(0, sdata->gGT->iconGroup[0], &data.emSet_Maskgrab[0]);

		if (p == NULL)
			return;

		// position variables
		p->axis[0].startVal = CTR_MipsAddLo(p->axis[0].startVal, d->posCurr.x);
		p->axis[1].startVal = CTR_MipsAddLo(p->axis[1].startVal, d->posCurr.y);
		p->axis[2].startVal = CTR_MipsAddLo(p->axis[2].startVal, d->posCurr.z);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80066d4c-0x80066e3c.
void VehStuckProc_MaskGrab_Update(struct Thread *t, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;

	d->NoInputTimer = (s16)CTR_MipsSubLo((u16)d->NoInputTimer, (u16)gGT->elapsedTimeMS);

	if (d->NoInputTimer < 0)
		d->NoInputTimer = 0;

	if (d->NoInputTimer != 0)
		return;

	// when input is allowed,
	// which is when driver is spawned back over track

	struct MaskHeadWeapon *mask = d->KartStates.MaskGrab.maskObj;

	if (mask != NULL)
	{
		// mask rotZ
		mask->rot.z &= ~(1);

		// scale = 100%
		mask->scale = 0x1000;
	}


	// CameraDC flag
	gGT->cameraDC[d->driverID].flags |= 8;


	VehStuckProc_MaskGrab_FindDestPos(d, d->lastValid);

	VehBirth_TeleportSelf(d, 0, 0x80);

	VehStuckProc_RevEngine_Init(t, d);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80066e3c-0x80066e8c.
void VehStuckProc_MaskGrab_PhysLinear(struct Thread *t, struct Driver *d)
{
	VehPhysProc_Driving_PhysLinear(t, d);

	d->baseSpeed = 0;
	d->fireSpeed = 0;
	d->jump_TenBuffer = 0;

	// reset turning state
	d->simpTurnState = 0;

	d->actionsFlagSet &= ~(ACTION_REVERSING_ENGINE | ACTION_BRAKE_WITH_ACCEL | ACTION_JUMP_BUTTON_HELD);
	d->actionsFlagSet |= ACTION_ACCEL_PREVENTION;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80066e8c-0x800671b0.
void VehStuckProc_MaskGrab_Animate(struct Thread *t, struct Driver *d)
{
	char frame;
	int numFrames;
	struct GameTracker *gGT = sdata->gGT;
	struct Instance *inst = t->inst;

	// if driver touched ground before mask grab
	if (d->KartStates.MaskGrab.boolStillFalling == false)
	{
		d->matrixArray = 0;
		d->matrixIndex = 0;
		inst->animIndex = 0;

		numFrames = VehFrameInst_GetNumAnimFrames(inst, 0);

		inst->animFrame = VehFrameInst_GetStartFrame(0, numFrames);

		d->AxisAngle2_normalVec = d->KartStates.MaskGrab.AngleAxis_NormalVec;
	}

	// if driver did not touch ground (and is falling)
	else
	{
		if (
		    // if whistle sound has not played
		    (d->KartStates.MaskGrab.boolWhistle == false) &&

		    // no input less than 1 sec
		    (d->NoInputTimer < 960))

		{
			// whistle sound has played
			d->KartStates.MaskGrab.boolWhistle = true;

			// "falling" sound, like a whistle
			OtherFX_Play(0x55, 1);
		}


		// Crashing animation at a frozen frame
		// makes it look like the driver is falling
		d->matrixArray = 4;
		inst->animIndex = 2;


		int maskGrabAnimFrame = d->KartStates.MaskGrab.animFrame;


		// logic specific to matrix set
		if (maskGrabAnimFrame < 3)
			d->matrixIndex = 7;
		else
			d->matrixIndex = maskGrabAnimFrame + 5;


		// logic specific to instance
		frame = 7;
		if (2 < maskGrabAnimFrame)
			frame = maskGrabAnimFrame + 5;
		inst->animFrame = frame;


		// logic specific to maskgrab
		frame = maskGrabAnimFrame + 1;

		if (frame > 7)
			frame = 7;
		d->KartStates.MaskGrab.animFrame = frame;

		// no input is less than 1.35 s
		if (d->NoInputTimer < 1296)
		{
			// Crashing
			d->matrixArray = 4;

			d->matrixIndex = 12;

			// set animation
			inst->animIndex = 2;

			// set animation frame
			inst->animFrame = 12;

			if (d->NoInputTimer < 0x3c1)
			{
				d->jumpSquishStretch = (s16)CTR_MipsSubLo((u16)d->jumpSquishStretch, 800);
				if (d->jumpSquishStretch < 0)
					d->jumpSquishStretch = 0;
			}
			else
			{
				// if particles are not spawned
				if (d->KartStates.MaskGrab.boolParticlesSpawned == false)
				{
					VehStuckProc_MaskGrab_Particles(d);

					// now they are spawned
					d->KartStates.MaskGrab.boolParticlesSpawned = true;
				}

				d->jumpSquishStretch = (s16)CTR_MipsAddLo((u16)d->jumpSquishStretch, 0x2d0);
				if (d->jumpSquishStretch > 8000)
					d->jumpSquishStretch = 8000;
			}
		}
		else
		{
			// reset Speed and Speed Approximate
			d->speed = 0;
			d->speedApprox = 0;

			// position backups
			d->posCurr = d->posPrev;
		}
	}

	struct MaskHeadWeapon *mask = d->KartStates.MaskGrab.maskObj;

	// if maskObj
	if (mask == 0)
		return;

	// set mask duration
	mask->duration = 7680;

	// less than 0.5s after player fell
	if (d->NoInputTimer > 960)
	{
		// scale = 0%
		mask->scale = 0;
		return;
	}

	// if more than 0.5s after player fell

	// if not lifting player
	if (d->KartStates.MaskGrab.boolLiftingPlayer == false)
	{
		// decrease mask posY by elapsed time
		mask->pos.y = (s16)CTR_MipsSubLo((u16)mask->pos.y, (u16)gGT->elapsedTimeMS);
	}

	// if lifting player (if driver isn't falling infinitely)
	else
	{
		d->speed = 0;

		// increase driver height, both posCurr and posPrev
		d->posCurr.y = CTR_MipsAddLo(d->posCurr.y, CTR_MipsSll(gGT->elapsedTimeMS, 7));
		d->posPrev.y = d->posCurr.y;
	}

	// maskPosX = driverPosX
	mask->pos.x = (s16)CTR_MipsSra(d->posCurr.x, 8);

	// set mask posZ
	mask->pos.z = (s16)CTR_MipsSra(d->posCurr.z, 8);

	// if mask posY < driver posY
	if (mask->pos.y < (s16)CTR_MipsSra(d->posCurr.y, 8))
	{
		// mask posY = driver posY
		mask->pos.y = (s16)CTR_MipsSra(d->posCurr.y, 8);

		d->KartStates.MaskGrab.boolLiftingPlayer = true;
	}

	// if more than halfway through mask pickup
	if (d->NoInputTimer < 721)
	{
		// scale = 100%
		mask->scale = 0x1000;
	}

	// if less than half
	else
	{
		// interpolate scale
		mask->scale = (s16)(CTR_MipsSll(CTR_MipsSubLo(960, d->NoInputTimer), 0xc) / 0xf0);
	}
}


extern DriverFunc PlayerMaskGrabFuncTable[DRIVER_FUNC_COUNT];

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800671b0-0x8006749c.
void VehStuckProc_MaskGrab_Init(struct Thread *t, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Instance *inst = t->inst;

	d->kartState = KS_MASK_GRABBED;

	d->KartStates.MaskGrab.animFrame = 0;

	d->KartStates.MaskGrab.boolParticlesSpawned = false;
	d->KartStates.MaskGrab.boolStillFalling = false;
	d->KartStates.MaskGrab.boolLiftingPlayer = false;
	d->KartStates.MaskGrab.boolWhistle = false;

	d->KartStates.MaskGrab.maskObj = VehPickupItem_MaskUseWeapon(d, 1);

	d->matrixArray = 0;
	d->matrixIndex = 0;

	d->turbo_MeterRoomLeft = 0;
	d->turbo_outsideTimer = 0;
	d->reserves = 0;

	d->NoInputTimer = 1440;

	d->actionsFlagSet &= ~(ACTION_AIRBORNE | ACTION_HIGH_JUMP);

	if ((LOAD_IsOpen_RacingOrBattle() != 0) && ((gGT->gameMode1 & ADVENTURE_ARENA) == 0))
	{
		RB_Player_ModifyWumpa(d, -2);
	}

	if (CTR_MipsAddLo(d->quadBlockHeight, 0x8000) < d->posCurr.y)
	{
		d->numTimesMaskGrab++;

		if ((d->posCurr.y < -0x8000) && ((gGT->level1->configFlags & 2) != 0))
		{
			d->KartStates.MaskGrab.AngleAxis_NormalVec = d->AxisAngle2_normalVec;

			for (int i = 10; i > 0; i--)
			{
				struct Particle *p = Particle_Init(0, gGT->iconGroup[9], &data.emSet_Falling[0]);
				if (p == NULL)
					break;

				p->otIndexOffset = d->instSelf->depthBiasNormal;
				p->driverInst = d->instSelf;
				p->driverID = d->driverID;
			}
		}
		else
		{
			d->KartStates.MaskGrab.boolStillFalling = true;
		}
	}
	else
	{
		d->KartStates.MaskGrab.AngleAxis_NormalVec = d->AxisAngle2_normalVec;
	}

	d->posCurr.x = CTR_MipsSll(inst->matrix.t[0], 8);
	d->posCurr.y = CTR_MipsSll(inst->matrix.t[1], 8);
	d->posCurr.z = CTR_MipsSll(inst->matrix.t[2], 8);

	d->posPrev = d->posCurr;

	for (int i = 0; i < DRIVER_FUNC_COUNT; i++)
	{
		d->funcPtrs[i] = PlayerMaskGrabFuncTable[i];
	}

	struct MaskHeadWeapon *mask = d->KartStates.MaskGrab.maskObj;
	if (mask == NULL)
		return;

	mask->rot.z |= 1;

	mask->pos.x = (s16)CTR_MipsSra(d->posCurr.x, 8);
	mask->pos.y = (s16)CTR_MipsAddLo(CTR_MipsSra(d->posCurr.y, 8), 0x140);
	mask->pos.z = (s16)CTR_MipsSra(d->posCurr.z, 8);
}


DriverFunc PlayerMaskGrabFuncTable[DRIVER_FUNC_COUNT] = {NULL,
                                                         VehStuckProc_MaskGrab_Update,
                                                         VehStuckProc_MaskGrab_PhysLinear,
                                                         VehPhysProc_Driving_Audio,
                                                         VehPhysGeneral_PhysAngular,
                                                         VehPhysForce_OnApplyForces,
                                                         COLL_MOVED_PlayerSearch,
                                                         VehPhysForce_CollideDrivers,
                                                         COLL_FIXED_PlayerSearch,
                                                         VehPhysGeneral_JumpAndFriction,
                                                         VehPhysForce_TranslateMatrix,
                                                         VehStuckProc_MaskGrab_Animate,
                                                         VehEmitter_DriverMain};


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006749c-0x80067554.
void VehStuckProc_PlantEaten_Update(struct Thread *t, struct Driver *d)
{
	d->NoInputTimer = (s16)CTR_MipsSubLo((u16)d->NoInputTimer, (u16)sdata->gGT->elapsedTimeMS);

	if (d->NoInputTimer <= 0)
	{
		d->NoInputTimer = 0;

		// respawn driver at last valid quadblock
		VehStuckProc_MaskGrab_FindDestPos(d, d->lastValid);
		VehBirth_TeleportSelf(d, 0, 0x80);

		// enable collision, make visible
		t->flags &= ~THREAD_FLAG_DISABLE_COLLISION;
		t->inst->flags &= ~(HIDE_MODEL);

		// this lets you rev engine while falling
		VehStuckProc_RevEngine_Init(t, d);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80067554-0x800675c0.
void VehStuckProc_PlantEaten_PhysLinear(struct Thread *t, struct Driver *d)
{
	VehPhysProc_Driving_PhysLinear(t, d);

	d->simpTurnState = 0;

	// reset two speed variables
	d->fireSpeed = 0;
	d->baseSpeed = 0;

	// reset jump variable
	d->jump_TenBuffer = 0;

	// acceleration prevention,
	// drop jump-button, gas+brake, and reversing engine bits.
	d->actionsFlagSet &= ~(ACTION_REVERSING_ENGINE | ACTION_BRAKE_WITH_ACCEL | ACTION_JUMP_BUTTON_HELD);
	d->actionsFlagSet |= ACTION_ACCEL_PREVENTION;

	d->timeSpentEaten = CTR_MipsAddLo(d->timeSpentEaten, sdata->gGT->elapsedTimeMS);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800675c0-0x800677d0.
void VehStuckProc_PlantEaten_Animate(struct Thread *t, struct Driver *d)
{
	s32 dist;
	struct Instance *inst;
	SVECTOR plantVector;
	VECTOR camVec;
	s32 gteFlags[2];

	struct GameTracker *gGT = sdata->gGT;

	struct Thread *plant = d->plantEatingMe;

	// if any plant is eating me
	if (((plant != NULL) &&

	     // if not initialized
	     (d->KartStates.EatenByPlant.boolInited == false)) &&

	    // if more than 0.5s since player death
	    (d->NoInputTimer < 2880))
	{
		// get instance from thread
		inst = plant->inst;

		// initialized, player eaten
		d->KartStates.EatenByPlant.boolInited = true;

		plantVector.vx = (((struct Plant *)plant->object)->LeftOrRight == 0) ? 250 : -250;
		plantVector.vy = 0;
		plantVector.vz = 750;

		SetRotMatrix(&inst->matrix);

		SetTransMatrix(&inst->matrix);

		camVec.vx = 0;
		camVec.vy = 0;
		camVec.vz = 0;
		gteFlags[0] = 0;
		gteFlags[1] = 0;

		RotTrans(&plantVector, &camVec, gteFlags);

		struct PushBuffer *pb = &gGT->pushBuffer[d->driverID];

		pb->pos.x = camVec.vx;
		pb->pos.y = CTR_MipsAddLo(inst->matrix.t[1], 0xc0);
		pb->pos.z = camVec.vz;

		int camX = CTR_MipsSubLo(camVec.vx, inst->matrix.t[0]);
		int camZ = CTR_MipsSubLo(camVec.vz, inst->matrix.t[2]);

		pb->rot.y = (s16)ratan2(camX, camZ);

		// get distance between car and camera
		dist = SquareRoot0_stub(CTR_MipsAddLo(CTR_MipsMulLo(camX, camX), CTR_MipsMulLo(camZ, camZ)));

		pb->rot.x = CTR_MipsSubLo(0x800, ratan2(CTR_MipsSubLo(pb->pos.y, inst->matrix.t[1]), dist));

		pb->rot.z = 0;
	}
}


extern DriverFunc PlayerEatenFuncTable[DRIVER_FUNC_COUNT];

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800677d0-0x80067930.
// when eaten by plant on papu pyramid
void VehStuckProc_PlantEaten_Init(struct Thread *t, struct Driver *d)
{
	// when this function executes, you are lifted
	// above the track by the mask, where you respawn

	char i;
	struct Instance *inst = t->inst;

	// set state to mask grab, so nobody hits you with weapon
	d->kartState = KS_MASK_GRABBED;

	d->KartStates.EatenByPlant.boolInited = false;

	d->turbo_MeterRoomLeft = 0;
	d->turbo_outsideTimer = 0;
	d->reserves = 0;

	// drop bits for airborne and high-jump state
	d->actionsFlagSet &= ~(ACTION_AIRBORNE | ACTION_HIGH_JUMP);

	// "cloud" is the raincloud after hitting red potion

	// if thread of "cloud" exists
	if (d->thCloud != NULL)
	{
		((struct RainCloud *)d->thCloud->object)->timeMS = 0;

		d->thCloud->funcThTick = RB_RainCloud_FadeAway;
		d->thCloud = NULL;
	}

	if ((LOAD_IsOpen_RacingOrBattle() != 0) && ((sdata->gGT->gameMode1 & ADVENTURE_ARENA) == 0))
	{
		RB_Player_ModifyWumpa(d, -2);
	}

	// allow this thread to ignore all collisions
	t->flags |= THREAD_FLAG_DISABLE_COLLISION;

	// make invisible
	inst->flags |= HIDE_MODEL;

	OtherFX_Stop1((int)d->driverAudioPtrs[1]);
	d->driverAudioPtrs[1] = NULL;
	OtherFX_Stop1((int)d->driverAudioPtrs[2]);
	d->driverAudioPtrs[2] = NULL;
	OtherFX_Stop1((int)d->driverAudioPtrs[0]);
	d->driverAudioPtrs[0] = NULL;

	for (i = 0; i < DRIVER_FUNC_COUNT; i++)
		d->funcPtrs[i] = PlayerEatenFuncTable[i];
}

DriverFunc PlayerEatenFuncTable[DRIVER_FUNC_COUNT] = {
    NULL,
    VehStuckProc_PlantEaten_Update,
    VehStuckProc_PlantEaten_PhysLinear,
    VehPhysProc_Driving_Audio,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    VehPhysForce_TranslateMatrix,
    VehStuckProc_PlantEaten_Animate,
    NULL,
};


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80067930-0x80067960.
void VehStuckProc_RIP_Init(struct Thread *t, struct Driver *d)
{
	VehStuckProc_PlantEaten_Init(t, d);
	d->invisibleTimer = 0;
	d->funcPtrs[DRIVER_FUNC_UPDATE] = NULL;
	d->funcPtrs[DRIVER_FUNC_ANIMATE] = NULL;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80067960-0x80067a74.
void VehStuckProc_RevEngine_Update(struct Thread *t, struct Driver *d)
{
	int revFireLevel;

	// If race has not started
	if (d->KartStates.RevEngine.boolMaskGrab == false)
	{
		// If Traffic Lights are not done counting down
		if (0 < sdata->gGT->trafficLightsTimer)
		{
			// Dont continue with the function,
			// let your kart stay in a revving state
			return;
		}
	}

	// If race has started
	else
	{
		// If mask grab has not lowered you close
		// enough to the track to let you go
		if (CTR_MipsAddLo(d->quadBlockHeight, 0x4000) <= d->posCurr.y)
		{
			// Dont continue with the function,
			// let your kart stay in a revving state
			return;
		}
	}

	// Assume it's time to transition out of being
	// frozen, and into driving, last iteration of
	// this function

	if ((d->KartStates.RevEngine.boolMaskGrab == true) && (d->KartStates.RevEngine.maskObj != NULL))
		d->KartStates.RevEngine.maskObj->duration = 0;

	if ((d->const_AccelSpeed_ClassStat < d->KartStates.RevEngine.fireLevel) && (d->KartStates.RevEngine.lockoutFlags & REV_ENGINE_LOCKOUT_ALL) == 0)
	{
		// While not moving, if you rev'd your engine less than...
		if (d->KartStates.RevEngine.boostMeter < CTR_MipsAddLo(d->const_AccelSpeed_ClassStat, d->const_SacredFireSpeed))
		{
			// You get a small boost
			revFireLevel = 0x20;
		}

		// if you rev'd your engine high
		else
		{
			// you get a big boost
			revFireLevel = 0x80;
		}

		// one full second of reserves
		VehFire_Increment(d, 960, 0, revFireLevel);
	}

	// full meter
	d->turbo_MeterRoomLeft = 0;
	d->revEngineState = 0;

	VehPhysProc_Driving_Init(t, d);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80067a74-0x80067b7c.
void VehStuckProc_RevEngine_PhysLinear(struct Thread *t, struct Driver *d)
{
	u32 cooldownTimer;

	struct GameTracker *gGT = sdata->gGT;

	cooldownTimer = (u16)d->KartStates.RevEngine.releaseCooldownTimerMS;
	cooldownTimer = CTR_MipsSubLo(cooldownTimer, (u16)gGT->elapsedTimeMS);
	if ((cooldownTimer & 0x8000) != 0)
		cooldownTimer = 0;
	d->KartStates.RevEngine.releaseCooldownTimerMS = (s16)cooldownTimer;

	cooldownTimer = (u16)d->KartStates.RevEngine.emptyCooldownTimerMS;
	cooldownTimer = CTR_MipsSubLo(cooldownTimer, (u16)gGT->elapsedTimeMS);
	if ((cooldownTimer & 0x8000) != 0)
		cooldownTimer = 0;
	d->KartStates.RevEngine.emptyCooldownTimerMS = (s16)cooldownTimer;

	VehPhysProc_Driving_PhysLinear(t, d);

	if (d->KartStates.RevEngine.boolMaskGrab == 0)
		return;

	d->posCurr.y = CTR_MipsSubLo(d->posCurr.y, 0x200);

	// if maskObj exists
	if (d->KartStates.RevEngine.maskObj != 0)
		d->KartStates.RevEngine.maskObj->duration = 7680;

	struct CameraDC *cDC = &gGT->cameraDC[d->driverID];
	cDC->flags |= 0x10;
	cDC->maskGrabHeightOffset = 0x40;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80067b7c-0x80067f4c.
void VehStuckProc_RevEngine_Animate(struct Thread *t, struct Driver *d)
{
	struct Instance *inst = t->inst;

	if ((d->fireSpeed > 0) && (d->KartStates.RevEngine.releaseCooldownTimerMS == 0) && ((d->KartStates.RevEngine.lockoutFlags & REV_ENGINE_LOCKOUT_ALL) == 0))
	{
		int revDelta = CTR_MipsSubLo(d->KartStates.RevEngine.fireLevel, d->KartStates.RevEngine.boostMeter);
		if (revDelta < 0)
			revDelta = CTR_MipsNegLo(revDelta);

		revDelta = CTR_MipsSra(revDelta, 1);

		int fillStep = revDelta;

		// Speed of filling the meter changes
		// depending on how full the meter is,
		// there are two speeds
		if (5000 < revDelta)
			fillStep = 5000;

		if (revDelta < 0x100)
			fillStep = 0x100;

		int revLevel = VehCalc_InterpBySpeed(d->KartStates.RevEngine.fireLevel, fillStep, d->KartStates.RevEngine.boostMeter);

		d->KartStates.RevEngine.fireLevel = revLevel;
		d->KartStates.RevEngine.chargeState = REV_ENGINE_CHARGE_ACTIVE;

		if (revLevel < d->KartStates.RevEngine.boostMeter)
		{
			d->KartStates.RevEngine.overRevTimerMS = 0;
		}
		else
		{
			s16 overRevTimerMS = (s16)CTR_MipsAddLo((u16)d->KartStates.RevEngine.overRevTimerMS, (u16)sdata->gGT->elapsedTimeMS);
			d->KartStates.RevEngine.overRevTimerMS = overRevTimerMS;

			if (192 < overRevTimerMS)
			{
				d->KartStates.RevEngine.chargeState = REV_ENGINE_CHARGE_IDLE;
				d->KartStates.RevEngine.lockoutFlags |= REV_ENGINE_LOCKOUT_ALL;

				OtherFX_Play_Echo(0xf, 1, d->actionsFlagSet & ACTION_ENGINE_ECHO);
			}
		}
		goto LAB_80067dec;
	}
	d->KartStates.RevEngine.overRevTimerMS = 0;

	if (d->KartStates.RevEngine.chargeState == REV_ENGINE_CHARGE_ACTIVE)
	{
		d->KartStates.RevEngine.releaseCooldownTimerMS = 0x100;
		d->KartStates.RevEngine.chargeState = REV_ENGINE_CHARGE_IDLE;

		if (d->const_AccelSpeed_ClassStat < d->KartStates.RevEngine.fireLevel)
			d->KartStates.RevEngine.chargeState = REV_ENGINE_CHARGE_RELEASED_ABOVE_ACCEL;
	}

	if ((d->KartStates.RevEngine.chargeState != REV_ENGINE_CHARGE_IDLE) && (d->KartStates.RevEngine.fireLevel < d->const_AccelSpeed_ClassStat))
	{
		d->KartStates.RevEngine.chargeState = REV_ENGINE_CHARGE_IDLE;

		int boostMeter = VehCalc_InterpBySpeed(d->KartStates.RevEngine.boostMeter, CTR_MipsAddLo(d->const_SacredFireSpeed / 3, 3),
		                                       CTR_MipsAddLo(d->const_SacredFireSpeed, d->const_AccelSpeed_ClassStat));
		d->KartStates.RevEngine.boostMeter = boostMeter;
	}

	if (d->KartStates.RevEngine.fireLevel < 1)
	{
		d->KartStates.RevEngine.lockoutFlags &= ~REV_ENGINE_LOCKOUT_REV_DECAY;
		d->KartStates.RevEngine.boostMeter = CTR_MipsAddLo(d->const_AccelSpeed_ClassStat, d->const_SacredFireSpeed / 3);
	}
	else
	{
		u32 decayStep = CTR_MipsSra(d->KartStates.RevEngine.fireLevel, 1);
		u8 decayBelowMinimum;

		if ((d->KartStates.RevEngine.lockoutFlags & REV_ENGINE_LOCKOUT_REV_DECAY) == 0)
		{
			decayBelowMinimum = (int)decayStep < 0x100;

			if (1000 < (int)decayStep)
			{
				decayStep = 1000;
				decayBelowMinimum = decayStep < 0x100;
			}
		}
		else
		{
			decayBelowMinimum = (int)decayStep < 0x100;

			if (3000 < (int)decayStep)
			{
				decayStep = 3000;
				decayBelowMinimum = decayStep < 0x100;
			}
		}

		if (decayBelowMinimum)
			decayStep = 0x100;

		int revLevel = CTR_MipsSubLo(d->KartStates.RevEngine.fireLevel, decayStep);
		d->KartStates.RevEngine.fireLevel = revLevel;

		if (revLevel < 1)
		{
			d->KartStates.RevEngine.emptyCooldownTimerMS = 0xc0;
			d->KartStates.RevEngine.fireLevel = 0;
		}
	}

	if (d->fireSpeed < 1)
		d->KartStates.RevEngine.lockoutFlags &= ~REV_ENGINE_LOCKOUT_PEDAL_HELD;

LAB_80067dec:

	u32 packedStatus = ((u32)(u16)d->KartStates.RevEngine.emptyCooldownTimerMS) | ((u32)d->KartStates.RevEngine.chargeState << 16) |
	                   ((u32)d->KartStates.RevEngine.lockoutFlags << 24);
	if ((packedStatus & REV_ENGINE_PACKED_BUSY_MASK) == 0)
	{
		if (d->KartStates.RevEngine.fireLevel < d->const_AccelSpeed_ClassStat)
			d->revEngineState = 0;
		else
			d->revEngineState = 1;
	}
	else
	{
		d->revEngineState = 2;
	}

	int accelClassStat = d->const_AccelSpeed_ClassStat;

	d->speedometerNeedleValue = d->KartStates.RevEngine.fireLevel;

	u8 meterRoomStart;
	int meterRoomEnd;
	int meterMin;
	int meterMax;

	if (d->KartStates.RevEngine.fireLevel < accelClassStat)
	{
		// 476 and 447 can be absolutely any value,
		// by default they are 15 and 30, but as long as
		// they are proportional (1 and 2, 4 and 8), they
		// behave the same as 15 and 30

		meterRoomStart = d->const_turboMaxRoom;

		// 477 changes when meter turns red
		meterRoomEnd = CTR_MipsAddLo(CTR_MipsSll((u8)d->const_turboLowRoomWarning, 5), 1);

		meterMin = 0;
		meterMax = accelClassStat;
	}
	else
	{
		// 477 changes when meter turns red
		meterRoomStart = d->const_turboLowRoomWarning;

		meterRoomEnd = 1;

		meterMin = accelClassStat;
		meterMax = CTR_MipsAddLo(accelClassStat, d->const_SacredFireSpeed);
	}

	s16 meterRoomLeft = VehCalc_MapToRange(d->KartStates.RevEngine.fireLevel, meterMin, meterMax, CTR_MipsSll(meterRoomStart, 5), meterRoomEnd);
	d->turbo_MeterRoomLeft = meterRoomLeft;

	d->distanceDrivenBackwards = 0;
	int squishScale = CTR_MipsSra((s16)d->speedometerNeedleValue, 6);

	if (squishScale < 0x401)
	{
		if (squishScale < 0)
			squishScale = 0;
	}
	else
	{
		squishScale = 0x400;
	}

	// Set the scale of the car while revving the engine,
	// this is a basic "squash and stretch" concept of animation, before motion

	// Reduce height a little
	inst->scale.y = (s16)CTR_MipsSubLo(3276, squishScale);
	inst->scale.x = (s16)CTR_MipsAddLo(CTR_MipsMulLo(squishScale, 6) / 10, 3276);
	inst->scale.z = (s16)CTR_MipsAddLo(CTR_MipsMulLo(squishScale, 6) / 10, 3276);

	d->jumpSquishStretch = squishScale;
}


extern DriverFunc PlayerRevEngineFuncTable[DRIVER_FUNC_COUNT];

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80067f4c-0x8006809c.
void VehStuckProc_RevEngine_Init(struct Thread *t, struct Driver *d)
{
	// spawn function that waits for traffic lights

	// kart state to rev
	d->kartState = KS_ENGINE_REVVING;
	d->revEngineState = 0;

	// assume reason for revving is: start of race
	d->KartStates.RevEngine.boolMaskGrab = false;
	d->KartStates.RevEngine.maskObj = NULL;
	d->KartStates.RevEngine.fireLevel = 0;

	// if this is a mask grab
	if (CTR_MipsAddLo(d->quadBlockHeight, 0x1000) < d->posCurr.y)
	{
		// assume reason for revving is: mask grab
		d->KartStates.RevEngine.boolMaskGrab = true;
		d->KartStates.RevEngine.maskObj = VehPickupItem_MaskUseWeapon(d, 0);

		d->actionsFlagSet &= ~ACTION_TOUCH_GROUND;

		// CameraDC flag
		sdata->gGT->cameraDC[d->driverID].flags |= 8;
	}

	for (char i = 0; i < DRIVER_FUNC_COUNT; i++)
	{
		d->funcPtrs[i] = PlayerRevEngineFuncTable[i];
	}

	d->boolFirstFrameSinceRevEngine = true;

	d->KartStates.RevEngine.overRevTimerMS = 0;
	d->KartStates.RevEngine.releaseCooldownTimerMS = 0;
	d->KartStates.RevEngine.emptyCooldownTimerMS = 0;
	d->KartStates.RevEngine.chargeState = REV_ENGINE_CHARGE_IDLE;
	d->KartStates.RevEngine.lockoutFlags = 0;

	d->KartStates.RevEngine.boostMeter = CTR_MipsAddLo(d->const_AccelSpeed_ClassStat, d->const_AccelSpeed_ClassStat / 3);
}

DriverFunc PlayerRevEngineFuncTable[DRIVER_FUNC_COUNT] = {
    NULL, VehStuckProc_RevEngine_Update, VehStuckProc_RevEngine_PhysLinear, VehPhysProc_Driving_Audio, NULL, NULL, NULL, NULL, NULL,
    NULL, VehPhysForce_TranslateMatrix,  VehStuckProc_RevEngine_Animate,    VehEmitter_DriverMain,
};


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006809c-0x800680d0.
void VehStuckProc_Tumble_Update(struct Thread *thread, struct Driver *driver)
{
	if (driver->NoInputTimer != 0)
		return;

	driver->matrixArray = 0;
	driver->matrixIndex = 0;
	VehPhysProc_Driving_Init(thread, driver);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800680d0-0x80068150.
void VehStuckProc_Tumble_PhysLinear(struct Thread *thread, struct Driver *driver)
{
	driver->NoInputTimer = (s16)CTR_MipsSubLo((u16)driver->NoInputTimer, (u16)sdata->gGT->elapsedTimeMS);

	if (driver->NoInputTimer < 0)
		driver->NoInputTimer = 0;

	VehPhysProc_Driving_PhysLinear(thread, driver);

	driver->jump_ForcedMS = 0x60;
	driver->baseSpeed = 0;
	driver->fireSpeed = 0;
	driver->actionsFlagSet |= ACTION_WARP | ACTION_FRONT_SKID | ACTION_BACK_SKID | ACTION_ACCEL_PREVENTION;
	driver->jump_InitialVelY = (s16)CTR_MipsAddLo(CTR_MipsSll((u16)driver->NoInputTimer, 1), 6000);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80068150-0x80068244.
void VehStuckProc_Tumble_PhysAngular(struct Thread *thread, struct Driver *driver)
{
	int elapsedTimeMS = sdata->gGT->elapsedTimeMS;

	driver->numFramesSpentSteering = 10000;

	driver->turnWobbleAngle = (s16)CTR_MipsSubLo((u16)driver->turnWobbleAngle, CTR_MipsSra(driver->turnWobbleAngle, 3));
	driver->rotationSpinRate = (s16)CTR_MipsSubLo((u16)driver->rotationSpinRate, CTR_MipsSra(driver->rotationSpinRate, 3));
	driver->turnAngleLerpVel = (s16)CTR_MipsSubLo((u16)driver->turnAngleLerpVel, CTR_MipsSra(driver->turnAngleLerpVel, 3));

	driver->ampTurnState = driver->rotationSpinRate;

	driver->turnAngleCurr = (s16)CTR_MipsSubLo(CTR_MipsAddLo(CTR_MipsAddLo((u16)driver->turnAngleCurr, (u16)driver->turnAngleLerpVel), 0x800) & 0xfff, 0x800);

	driver->angle = (s16)(CTR_MipsAddLo((u16)driver->angle, CTR_MipsSra(CTR_MipsMulLo(driver->rotationSpinRate, elapsedTimeMS), 0xd)) & 0xfff);

	(driver->rotCurr).y = (s16)CTR_MipsAddLo(CTR_MipsAddLo((u16)driver->turnWobbleAngle, (u16)driver->angle), (u16)driver->turnAngleCurr);

	(driver->rotCurr).w = VehCalc_InterpBySpeed((int)(driver->rotCurr).w, CTR_MipsSra(CTR_MipsSll(elapsedTimeMS, 5), 5), 0);

	VehPhysForce_RotAxisAngle(&driver->matrixMovingDir, driver->AxisAngle1_normalVec.v, driver->angle);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80068244-0x800682a4.
void VehStuckProc_Tumble_Animate(struct Thread *thread, struct Driver *driver)
{
	int matrixIndex;
	int arrLength;
	int quotient;

	driver->matrixArray = 6;
	arrLength = data.bakedGteMath[6].numEntries;

	// divide by 32ms to get frame index
	matrixIndex = CTR_MipsSra(driver->NoInputTimer, 5);

	// modulus to wrap repeat animation
	quotient = CTR_MipsDiv(matrixIndex, arrLength);
	matrixIndex = CTR_MipsSubLo(matrixIndex, CTR_MipsMulLo(quotient, arrLength));

	if (driver->KartStates.Blasted.boolPlayBackwards != 0)
	{
		matrixIndex = CTR_MipsSubLo(arrLength, CTR_MipsAddLo(matrixIndex, 1));
	}

	driver->matrixIndex = matrixIndex;
}


DriverFunc PlayerBlastedFuncTable[DRIVER_FUNC_COUNT] = {NULL,
                                                        VehStuckProc_Tumble_Update,
                                                        VehStuckProc_Tumble_PhysLinear,
                                                        VehPhysProc_Driving_Audio,
                                                        VehStuckProc_Tumble_PhysAngular,
                                                        VehPhysForce_OnApplyForces,
                                                        COLL_MOVED_PlayerSearch,
                                                        VehPhysForce_CollideDrivers,
                                                        COLL_FIXED_PlayerSearch,
                                                        VehPhysGeneral_JumpAndFriction,
                                                        VehPhysForce_TranslateMatrix,
                                                        VehStuckProc_Tumble_Animate,
                                                        VehEmitter_DriverMain};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800682a4-0x800683f4.
void VehStuckProc_Tumble_Init(struct Thread *thread, struct Driver *driver)
{
	driver->kartState = KS_BLASTED;
	driver->turbo_MeterRoomLeft = 0;

	if ((LOAD_IsOpen_RacingOrBattle() != 0) && ((sdata->gGT->gameMode1 & ADVENTURE_ARENA) == 0))
	{
		RB_Player_ModifyWumpa(driver, -3);
	}

	driver->instSelf->animIndex = 0;

	int numAnimFrames = VehFrameInst_GetNumAnimFrames(driver->instSelf, 0);
	int animFrame = VehFrameInst_GetStartFrame(0, numAnimFrames);

	driver->instSelf->animFrame = (s16)animFrame;

	int rng = MixRNG_Scramble();
	driver->KartStates.Blasted.boolPlayBackwards = rng & 4;

	s8 simpTurnState = driver->simpTurnState;
	char rumbleStrength;
	if (simpTurnState < 1)
	{
		rumbleStrength = 0x19;
	}
	else
	{
		rumbleStrength = 0x29;
	}

	for (int i = 0; i < DRIVER_FUNC_COUNT; i++)
	{
		driver->funcPtrs[i] = PlayerBlastedFuncTable[i];
	}

	GAMEPAD_JogCon1(driver, rumbleStrength, 0x60);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800683f4-0x800685b0.
void VehStuckProc_Warp_MoveDustPuff(s16 *points, int span, int radius, s16 *jitterScale)
{
	int radiusHalf = CTR_MipsSra(radius, 1);

	int jitterX = CTR_MipsSra(CTR_MipsMulLo(MixRNG_Scramble() & 0xfff, radius), 0xc);
	if (jitterX < radiusHalf)
		jitterX = CTR_MipsSubLo(jitterX, radius);

	int jitterY = CTR_MipsSra(CTR_MipsMulLo(MixRNG_Scramble() & 0xfff, radius), 0xc);
	if (jitterY < radiusHalf)
		jitterY = CTR_MipsSubLo(jitterY, radius);

	int jitterZ = CTR_MipsSra(CTR_MipsMulLo(MixRNG_Scramble() & 0xfff, radius), 0xc);
	if (jitterZ < radiusHalf)
		jitterZ = CTR_MipsSubLo(jitterZ, radius);

	s16 *end = points + span * 4;
	int halfSpan = CTR_MipsSra(span, 1);
	s16 *mid = points + halfSpan * 4;

	mid[0] = (s16)CTR_MipsAddLo(CTR_MipsSra(CTR_MipsAddLo(points[0], end[0]), 1), CTR_MipsSra(CTR_MipsMulLo(jitterScale[0], jitterX), 0xc));
	mid[1] = (s16)CTR_MipsAddLo(CTR_MipsSra(CTR_MipsAddLo(points[1], end[1]), 1), CTR_MipsSra(CTR_MipsMulLo(jitterScale[1], jitterY), 0xc));
	mid[2] = (s16)CTR_MipsAddLo(CTR_MipsSra(CTR_MipsAddLo(points[2], end[2]), 1), CTR_MipsSra(CTR_MipsMulLo(jitterScale[2], jitterZ), 0xc));

	if (span > 2)
	{
		int nextRadius = CTR_MipsSra(CTR_MipsMulLo(radius, 0xc00), 0xc);
		VehStuckProc_Warp_MoveDustPuff(points, halfSpan, nextRadius, jitterScale);
		VehStuckProc_Warp_MoveDustPuff(mid, halfSpan, nextRadius, jitterScale);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800685b0-0x80068644.
void VehStuckProc_Warp_AddDustPuff1(struct ScratchpadStruct *sps)
{
	struct GameTracker *gGT = sdata->gGT;

	// if even frame don't spawn
	if (gGT->timer & 1)
		return;

	struct Particle *p = Particle_Init(0, gGT->iconGroup[1], &data.emSet_Warppad[0]);

	if (p == NULL)
		return;

	// position variables
	for (char i = 0; i < 3; i++)
		p->axis[i].startVal = CTR_MipsAddLo(p->axis[i].startVal, CTR_MipsSll(sps->Input1.pos.v[i], 8));
}


#define VEH_WARP_DUST_SEGMENTS 16

struct VehWarpDustProjected
{
	u32 sxy0;
	u32 sxy1;
	u32 sxy2;
	u32 depth;
};

struct VehWarpDustScratch
{
	u8 pad_000[0x108];
	SVECTOR points[VEH_WARP_DUST_SEGMENTS + 1];
	SVECTOR projectLeft;
	SVECTOR projectRight;
	struct VehWarpDustProjected prev;
	struct VehWarpDustProjected curr;
	SVec3 jitterScale;
};

struct VehWarpDustG4Body
{
	u32 color0AndCode;
	u32 xy0;
	u32 color1;
	u32 xy1;
	u32 color2;
	u32 xy2;
	u32 color3;
	u32 xy3;
};

struct VehWarpDustPacket
{
	u32 tag;
	u32 drawMode;
	struct VehWarpDustG4Body leftStrip;
	struct VehWarpDustG4Body rightStrip;
};

CTR_STATIC_ASSERT_LAYOUT(sizeof(struct VehWarpDustG4Body) == 0x20);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustG4Body, color0AndCode) == 0x00);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustG4Body, xy0) == 0x04);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustG4Body, color1) == 0x08);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustG4Body, xy1) == 0x0C);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustG4Body, color2) == 0x10);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustG4Body, xy2) == 0x14);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustG4Body, color3) == 0x18);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustG4Body, xy3) == 0x1C);

CTR_STATIC_ASSERT_LAYOUT(sizeof(struct VehWarpDustPacket) == 0x48);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustPacket, tag) == 0x00);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustPacket, drawMode) == 0x04);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustPacket, leftStrip) == 0x08);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustPacket, rightStrip) == 0x28);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustScratch, points) == 0x108);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustScratch, projectLeft) == 0x190);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustScratch, projectRight) == 0x198);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustScratch, prev) == 0x1a0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustScratch, curr) == 0x1b0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct VehWarpDustScratch, jitterScale) == 0x1c0);

static s16 VehWarpDust_AddHalf(s16 value, int delta)
{
	return (s16)CTR_MipsAddLo((u16)value, delta);
}

static void VehWarpDust_Project(struct VehWarpDustScratch *scratch, SVECTOR *point, int offsetX, int offsetY, int offsetZ, struct VehWarpDustProjected *out)
{
	SVECTOR *left = &scratch->projectLeft;
	SVECTOR *right = &scratch->projectRight;

	left->vx = VehWarpDust_AddHalf(point->vx, offsetX);
	left->vy = VehWarpDust_AddHalf(point->vy, offsetY);
	left->vz = VehWarpDust_AddHalf(point->vz, offsetZ);

	right->vx = VehWarpDust_AddHalf(point->vx, CTR_MipsNegLo(offsetX));
	right->vy = VehWarpDust_AddHalf(point->vy, CTR_MipsNegLo(offsetY));
	right->vz = VehWarpDust_AddHalf(point->vz, CTR_MipsNegLo(offsetZ));

	CTR_GteLoadSV3(left, point, right);
	gte_rtpt_b();

	out->sxy0 = MFC2(12);
	out->sxy1 = MFC2(13);
	out->sxy2 = MFC2(14);
	out->depth = MFC2(17);
}

static void VehWarpDust_EmitSegment(u32 **primCursor, struct PushBuffer *pb, const struct VehWarpDustProjected *prev, const struct VehWarpDustProjected *curr)
{
	struct VehWarpDustPacket *packet = (struct VehWarpDustPacket *)*primCursor;
	uint32_t *ot = pb->ptrOT + CTR_MipsSra((s32)curr->depth, 6);

	packet->drawMode = 0xe1000a20;

	packet->leftStrip.color0AndCode = 0x3a000000;
	packet->leftStrip.xy0 = curr->sxy0;
	packet->leftStrip.color1 = 0x007f1f3f;
	packet->leftStrip.xy1 = curr->sxy1;
	packet->leftStrip.color2 = 0;
	packet->leftStrip.xy2 = prev->sxy0;
	packet->leftStrip.color3 = 0x007f1f3f;
	packet->leftStrip.xy3 = prev->sxy1;

	packet->rightStrip.color0AndCode = 0x3a000000;
	packet->rightStrip.xy0 = curr->sxy2;
	packet->rightStrip.color1 = 0x007f1f3f;
	packet->rightStrip.xy1 = curr->sxy1;
	packet->rightStrip.color2 = 0;
	packet->rightStrip.xy2 = prev->sxy2;
	packet->rightStrip.color3 = 0x007f1f3f;
	packet->rightStrip.xy3 = prev->sxy1;

	CtrGpu_LinkPacket24(ot, &packet->tag, packet, 0x11000000);
	*primCursor = (u32 *)(packet + 1);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80068644-0x80068be8.
void VehStuckProc_Warp_AddDustPuff2(struct Driver *d, int *warp)
{
	struct GameTracker *gGT = sdata->gGT;
	struct PushBuffer *pb = &gGT->pushBuffer[d->driverID];
	struct DB *backBuffer = gGT->backBuffer;
	u32 *prim = backBuffer->primMem.cursor;
	struct VehWarpDustScratch *scratch = CTR_SCRATCHPAD_PTR(struct VehWarpDustScratch, 0);
	SVECTOR *points = scratch->points;
	SVECTOR *endpoint = &points[VEH_WARP_DUST_SEGMENTS];
	s16 *jitterScale = scratch->jitterScale.v;
	int offsetX;
	int offsetY;
	int offsetZ;

	gte_SetRotMatrix(&pb->matrix_ViewProj);
	gte_SetTransMatrix(&pb->matrix_ViewProj);

	jitterScale[0] = (s16)CTR_MipsSra(CTR_MipsAddLo(pb->matrix_CameraTranspose.m[0][0], pb->matrix_CameraTranspose.m[0][1]), 5);
	jitterScale[1] = (s16)CTR_MipsSra(CTR_MipsAddLo(pb->matrix_CameraTranspose.m[1][0], pb->matrix_CameraTranspose.m[1][1]), 5);
	jitterScale[2] = (s16)CTR_MipsSra(CTR_MipsAddLo(pb->matrix_CameraTranspose.m[2][0], pb->matrix_CameraTranspose.m[2][1]), 5);

	offsetX = CTR_MipsSra(pb->matrix_CameraTranspose.m[0][0], 10);
	offsetY = CTR_MipsSra(pb->matrix_CameraTranspose.m[1][0], 10);
	offsetZ = CTR_MipsSra(pb->matrix_CameraTranspose.m[2][0], 10);

	if ((d->instSelf->flags & HIDE_MODEL) != 0)
	{
		endpoint->vx = (s16)CTR_MipsSra(d->posCurr.x, 8);
		endpoint->vy = (s16)CTR_MipsSra(warp[4], 8);
		endpoint->vz = (s16)CTR_MipsSra(d->posCurr.z, 8);
		VehStuckProc_Warp_AddDustPuff1((struct ScratchpadStruct *)endpoint);
	}

	for (int ring = 0; ring < 6; ring++)
	{
		int baseAngle = CTR_MipsAddLo((CTR_MipsSll(ring, 12) / 6), warp[3]);
		struct VehWarpDustProjected *prev = &scratch->prev;
		struct VehWarpDustProjected *curr = &scratch->curr;

		points[0].vx = (s16)CTR_MipsSubLo(CTR_MipsSra(d->posCurr.x, 8), CTR_MipsSra(MATH_Sin(baseAngle), 5));
		points[0].vy = (s16)CTR_MipsSra(warp[2], 8);
		points[0].vz = (s16)CTR_MipsSubLo(CTR_MipsSra(d->posCurr.z, 8), CTR_MipsSra(MATH_Cos(baseAngle), 5));

		endpoint->vx = (s16)CTR_MipsSra(d->posCurr.x, 8);
		endpoint->vy = (s16)CTR_MipsSra(warp[4], 8);
		endpoint->vz = (s16)CTR_MipsSra(d->posCurr.z, 8);

		if ((d->instSelf->flags & HIDE_MODEL) == 0)
		{
			points[0].vx = VehWarpDust_AddHalf(points[0].vx, CTR_MipsNegLo(CTR_MipsSra(MATH_Sin(baseAngle), 6)));
			points[0].vz = VehWarpDust_AddHalf(points[0].vz, CTR_MipsNegLo(CTR_MipsSra(MATH_Cos(baseAngle), 6)));
			endpoint->vx = VehWarpDust_AddHalf(endpoint->vx, CTR_MipsSra(MATH_Sin(baseAngle), 8));
			endpoint->vz = VehWarpDust_AddHalf(endpoint->vz, CTR_MipsSra(MATH_Cos(baseAngle), 8));
		}
		else
		{
			VehStuckProc_Warp_AddDustPuff1((struct ScratchpadStruct *)points);
		}

		VehStuckProc_Warp_MoveDustPuff((s16 *)points, VEH_WARP_DUST_SEGMENTS, 0x100, jitterScale);

		for (int i = 1; i < VEH_WARP_DUST_SEGMENTS; i++)
			points[i].vy = VehWarpDust_AddHalf(points[i].vy, CTR_MipsSra(MATH_Sin(CTR_MipsSll(i, 7)), 7));

		VehWarpDust_Project(scratch, &points[0], offsetX, offsetY, offsetZ, prev);

		for (int seg = 0; seg < VEH_WARP_DUST_SEGMENTS; seg++)
		{
			struct VehWarpDustProjected *tmp;

			VehWarpDust_Project(scratch, &points[seg + 1], offsetX, offsetY, offsetZ, curr);
			VehWarpDust_EmitSegment(&prim, pb, prev, curr);

			tmp = prev;
			prev = curr;
			curr = tmp;
		}
	}

	backBuffer->primMem.cursor = prim;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80068be8-0x80068e04.
void VehStuckProc_Warp_PhysAngular(struct Thread *th, struct Driver *d)
{
	int warpTimer;
	SVec4 flarePos;

	// get instance from driver object
	struct Instance *inst = d->instSelf;

	// if driver is visible
	if ((inst->flags & HIDE_MODEL) == 0)
	{
		// height + 0x100
		int beamHeight = CTR_MipsAddLo(d->posCurr.y, 0x100);

		if (beamHeight < d->KartStates.Warp.quadHeight)
			beamHeight = d->KartStates.Warp.quadHeight;

		d->KartStates.Warp.beamHeight = beamHeight;

		d->KartStates.Warp.numParticle = CTR_MipsSubLo(d->KartStates.Warp.numParticle, 100);

		// add dust puff
		VehStuckProc_Warp_AddDustPuff2(d, &d->KartStates.Warp.timer);
	}

	warpTimer = d->KartStates.Warp.timer;
	warpTimer = CTR_MipsAddLo(warpTimer, 26);

	if (warpTimer <= 800)
	{
		// interpolate until scale is [0x12c0, 0x960, 0x12c0],
		// car is wide and s16

		for (char i = 0; i < 3; i++)
			inst->scale.v[i] = VehCalc_InterpBySpeed(inst->scale.v[i], 120, 4800 >> (i & 1));

		if (d->posCurr.y < CTR_MipsAddLo(d->quadBlockHeight, 0x8000))
			d->posCurr.y = CTR_MipsAddLo(d->posCurr.y, 0x800);
	}
	else
	{
		// cap to 800
		warpTimer = 800;

		d->revEngineState = 2;

		// interpolate until scale is [0, 24000, 0],
		// car is tall and thin

		for (char i = 0; i < 3; i++)
			inst->scale.v[i] = VehCalc_InterpBySpeed(inst->scale.v[i], (i == 1) ? 3200 : 600, 24000 * (i & 1));

		// if scale shrinks to zero
		if (inst->scale.x == 0)
		{
			// if car is visible
			if ((inst->flags & HIDE_MODEL) == 0)
			{
				// position above kart
				flarePos.x = (s16)CTR_MipsSra(d->posCurr.x, 8);
				flarePos.y = (s16)CTR_MipsAddLo(CTR_MipsSra(d->KartStates.Warp.quadHeight, 8), 0x40);
				flarePos.z = (s16)CTR_MipsSra(d->posCurr.z, 8);

				FLARE_Init(flarePos.v);
			}

			// make invisible
			inst->flags |= HIDE_MODEL;
		}

		else
		{
			d->KartStates.Warp.heightOffset = CTR_MipsSubLo(d->KartStates.Warp.heightOffset, 0x1800);
			d->posCurr.y = CTR_MipsAddLo(d->posCurr.y, d->KartStates.Warp.heightOffset);
		}
	}

	// drift angle = ((drift angle + warp timer + 0x800) & 0xfff) - 0x800
	s16 wrappedTurnAngle = (s16)CTR_MipsSubLo(CTR_MipsAddLo(CTR_MipsAddLo((u16)d->turnAngleCurr, (u16)warpTimer), 0x800) & 0xfff, 0x800);
	d->turnAngleCurr = wrappedTurnAngle;

	// cameraRotY = wobble angle + kart angle + wrapped warp turn angle
	d->rotCurr.y = (s16)CTR_MipsAddLo(CTR_MipsAddLo((u16)d->turnWobbleAngle, (u16)d->angle), (u16)wrappedTurnAngle);

	// driver is warping
	d->actionsFlagSet |= ACTION_WARP;

	d->KartStates.Warp.timer = warpTimer;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80068e04-0x80068f90.
void VehStuckProc_Warp_Init(struct Thread *th, struct Driver *d)
{
	if (d->kartState == KS_WARP_PAD)
		return;

	// If you are not in a warp pad

	d->KartStates.Warp.timer = 0x3c;
	d->KartStates.Warp.heightOffset = 0;
	d->KartStates.Warp.quadHeight = d->quadBlockHeight;

	// Warp sound?
	OtherFX_Play(0x97, 1);

	OtherFX_Stop1((int)d->driverAudioPtrs[1]);
	d->driverAudioPtrs[1] = NULL;
	OtherFX_Stop1((int)d->driverAudioPtrs[2]);
	d->driverAudioPtrs[2] = NULL;
	OtherFX_Stop1((int)d->driverAudioPtrs[0]);
	d->driverAudioPtrs[0] = NULL;

	u8 playerID = d->driverID;

	int engine = data.MetaDataCharacters[data.characterIDs[playerID]].engineID;

	EngineAudio_Stop((engine * 4) + playerID);

	// CameraDC, freecam mode
	sdata->gGT->cameraDC[playerID].cameraMode = 3;

	// driver -> instSelf
	struct Instance *inst = d->instSelf;

	// instance flags, now reflective
	inst->flags |= REFLECTIVE;

	// vertical line for split or reflection
	inst->vertSplit = (s16)CTR_MipsSra(d->quadBlockHeight, 8);

	// you are now in a warp pad
	d->kartState = KS_WARP_PAD;

	d->speed = 0;
	d->speedApprox = 0;

	d->funcPtrs[DRIVER_FUNC_INIT] = NULL;
	d->funcPtrs[DRIVER_FUNC_UPDATE] = NULL;
	d->funcPtrs[DRIVER_FUNC_PHYS_LINEAR] = NULL;
	d->funcPtrs[DRIVER_FUNC_AUDIO] = VehPhysProc_Driving_Audio;
	d->funcPtrs[DRIVER_FUNC_PHYS_ANGULAR] = VehStuckProc_Warp_PhysAngular;
	d->funcPtrs[DRIVER_FUNC_APPLY_FORCES] = NULL;
	d->funcPtrs[DRIVER_FUNC_COLL_MOVED] = NULL;
	d->funcPtrs[DRIVER_FUNC_COLLIDE_DRIVERS] = NULL;
	d->funcPtrs[DRIVER_FUNC_COLL_FIXED] = NULL;
	d->funcPtrs[DRIVER_FUNC_JUMP_FRICTION] = NULL;
	d->funcPtrs[DRIVER_FUNC_TRANSLATE_MATRIX] = VehPhysForce_TranslateMatrix;
	d->funcPtrs[DRIVER_FUNC_ANIMATE] = VehFrameProc_Driving;
	d->funcPtrs[DRIVER_FUNC_PARTICLES] = VehEmitter_DriverMain;

	// driver is warping
	d->actionsFlagSet |= ACTION_WARP;
}
