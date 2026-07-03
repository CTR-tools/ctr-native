#include <common.h>

static inline void VehPickupItem_CopyMatrix(MATRIX *dst, const MATRIX *src)
{
	dst->m[0][0] = src->m[0][0];
	dst->m[0][1] = src->m[0][1];
	dst->m[0][2] = src->m[0][2];
	dst->m[1][0] = src->m[1][0];
	dst->m[1][1] = src->m[1][1];
	dst->m[1][2] = src->m[1][2];
	dst->m[2][0] = src->m[2][0];
	dst->m[2][1] = src->m[2][1];
	dst->m[2][2] = src->m[2][2];
	dst->t[0] = src->t[0];
	dst->t[1] = src->t[1];
	dst->t[2] = src->t[2];
}

static inline void VehPickupItem_ClearMineMotion(struct MineWeapon *mine)
{
	mine->velocity.x = 0;
	mine->velocity.y = 0;
	mine->velocity.z = 0;
	mine->stopFallAtY = 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80064be4-0x80064c38.
int VehPickupItem_MaskBoolGoodGuy(struct Driver *d)
{
	int charID;
	charID = data.characterIDs[d->driverID];

	// Crash, Coco, Pura, Polar, Penta
	u32 maskBits = 0x20c9;

	return (maskBits >> charID) & 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80064c38-0x80064f94.
// boolPlaySound only gates sound when refreshing an existing mask object.
struct MaskHeadWeapon *VehPickupItem_MaskUseWeapon(struct Driver *driver, b32 boolPlaySound)

{
	struct Thread *currThread;
	struct MaskHeadWeapon *maskObj;
	struct Model *modelPtr;
	struct Thread *t;
	struct GameTracker *gGT;
	struct Instance *instance;
	int soundID;

	gGT = sdata->gGT;

	if ((LOAD_IsOpen_RacingOrBattle() == 0) || ((gGT->gameMode1 & ADVENTURE_ARENA) != 0))
	{
		// no mask object in adv arena
		maskObj = NULL;
		return maskObj;
	}

	t = driver->instSelf->thread;

	// check for existing mask
	for (currThread = t->childThread; currThread != 0; currThread = currThread->siblingThread)
	{
		// if thread->modelIndex is NOT Aku or Uka
		if ((u32)(currThread->modelIndex - STATIC_AKUAKU) >= 2)
		{
			continue;
		}

		currThread->funcThTick = RB_MaskWeapon_ThTick;

		maskObj = currThread->object;
		maskObj->duration = (driver->numWumpas < 10) ? 0x1e00 : 0x2d00;

		if (
		    // If this is human and not AI
		    ((driver->actionsFlagSet & ACTION_BOT) == 0) &&

		    (boolPlaySound != 0))
		{
			// 0x3a: uka model
			// 0x39: aku model

			// 0x54: uka sound
			// 0x53: aku model

			soundID = currThread->modelIndex + 0x1A;
			OtherFX_Play_Echo(soundID, 1, driver->actionsFlagSet & ACTION_ENGINE_ECHO);
		}

		// un-kill thread
		currThread->flags &= ~THREAD_FLAG_DEAD;

		// return object attached to thread
		return (struct MaskHeadWeapon *)currThread->object;
	}

	b32 boolGoodGuy = VehPickupItem_MaskBoolGoodGuy(driver);

	int modelID = STATIC_UKAUKA - boolGoodGuy;

	// 0x3a: uka head model idx in modelPtr array
	instance = INSTANCE_BirthWithThread(modelID, sdata->s_doctor1, SMALL, OTHER, RB_MaskWeapon_ThTick, sizeof(struct MaskHeadWeapon), t);

	soundID = modelID + 0x1A;


	if (
	    // If this is human and not AI
	    ((driver->actionsFlagSet & ACTION_BOT) == 0) &&

	    (OtherFX_Play_Echo(soundID, 1, driver->actionsFlagSet & ACTION_ENGINE_ECHO),

	     (driver->kartState != KS_ENGINE_REVVING) && (driver->kartState != KS_MASK_GRABBED)))
	{
		if (boolGoodGuy == 0)
		{
			gGT->gameMode1 &= ~(AKU_SONG);
			gGT->gameMode1 |= UKA_SONG;
		}

		else
		{
			gGT->gameMode1 &= ~(UKA_SONG);
			gGT->gameMode1 |= AKU_SONG;
		}
	}

	// 0x3a: uka model
	// 0x39: aku model

	// 0x40: uka beam
	// 0x3E: aku beam

	modelPtr = gGT->modelPtr[STATIC_AKUBEAM + ((modelID - STATIC_AKUAKU) * 2)];

	t = instance->thread;

	maskObj = (struct MaskHeadWeapon *)t->object;

// NOTE(aalhendi): Native keeps this model lookup string host-side; PS1 uses
// the retail RDATA symbol.
#ifdef CTR_NATIVE
	maskObj->maskBeamInst = INSTANCE_Birth3D(modelPtr, "akubeam1", t);
#else
	maskObj->maskBeamInst = INSTANCE_Birth3D(modelPtr, rdata.s_akubeam1, t);
#endif

	t->funcThDestroy = PROC_DestroyInstance;

	t->flags |= THREAD_FLAG_DISABLE_COLLISION;
	instance->flags |= HIDE_MODEL;
	maskObj->maskBeamInst->flags |= HIDE_MODEL;
	maskObj->duration = (driver->numWumpas > 9) ? 0x2d00 : 0x1e00;
	maskObj->rot.x = 0x40;
	maskObj->rot.y = 0;
	maskObj->rot.z = 0;
	maskObj->scale = 0x1000; // scale

	return maskObj;
}

static struct PushBuffer *VehPickupItem_GetDriverPushBuffer(struct GameTracker *gGT, u8 driverID)
{
	return (struct PushBuffer *)((u8 *)&gGT->pushBuffer[0] + (driverID * sizeof(struct PushBuffer)));
}

static void VehPickupItem_MissileLoadPlayerView(struct GameTracker *gGT, struct Driver *driver)
{
	struct PushBuffer *pb = VehPickupItem_GetDriverPushBuffer(gGT, driver->driverID);

	SetRotMatrix(&pb->matrix_ViewProj);
	SetTransMatrix(&pb->matrix_ViewProj);
}

static void VehPickupItem_MissileLoadAiView(struct Driver *driver)
{
	SVECTOR rot = {driver->rotCurr.x, driver->rotCurr.y, driver->rotCurr.z, 0};
	MATRIX matrix = {0};
	MATRIX unusedInverse;

	RotMatrix(&rot, &matrix);
	matrix.t[0] = CTR_MipsSra(driver->posCurr.x, 8);
	matrix.t[1] = CTR_MipsSra(driver->posCurr.y, 8);
	matrix.t[2] = CTR_MipsSra(driver->posCurr.z, 8);

	MATH_HitboxMatrix(&unusedInverse, &matrix);

	SetRotMatrix(&matrix);
	SetTransMatrix(&matrix);
}

static int VehPickupItem_MissileCandidateVisible(struct PushBuffer *pb, struct Driver *candidate)
{
	struct Instance *inst = candidate->instSelf;
	u32 sxy;
	s32 gteFlag;
	s16 screenX;
	s16 screenY;

	MTC2(((u32)(u16)inst->matrix.t[0]) | ((u32)(u16)inst->matrix.t[1] << 16), 0);
	MTC2((s32)(s16)inst->matrix.t[2], 1);
	gte_rtps();

	sxy = MFC2(14);
	gteFlag = CFC2(31);
	if ((gteFlag & 0x40000) != 0)
	{
		return 0;
	}

	screenX = (s16)sxy;
	if (screenX < 0x1f)
	{
		return 0;
	}
	if (screenX >= pb->rect.w - 0x1e)
	{
		return 0;
	}

	screenY = (s16)(sxy >> 16);
	if (screenY < 0x15)
	{
		return 0;
	}
	if (screenY >= pb->rect.h - 0x14)
	{
		return 0;
	}

	return 1;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80064f94-0x800652c8.
struct Driver *VehPickupItem_MissileGetTargetDriver(struct Driver *driver)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Driver *target = NULL;
	s32 closestDistance = 0x7fffffff;
	struct PushBuffer *pb = VehPickupItem_GetDriverPushBuffer(gGT, driver->driverID);

	if (driver->instSelf->thread->modelIndex == DYNAMIC_PLAYER)
	{
		VehPickupItem_MissileLoadPlayerView(gGT, driver);
	}
	else
	{
		VehPickupItem_MissileLoadAiView(driver);
	}

	for (int i = 0; i < 8; i++)
	{
		struct Driver *candidate = gGT->drivers[i];

		if (candidate == NULL)
		{
			continue;
		}
		if (candidate == driver)
		{
			continue;
		}
		if (candidate->kartState == KS_MASK_GRABBED)
		{
			continue;
		}

		if (((gGT->gameMode1 & BATTLE_MODE) != 0) && (candidate->BattleHUD.teamID == driver->BattleHUD.teamID))
		{
			continue;
		}

		if (candidate->invisibleTimer != 0)
		{
			continue;
		}

		if (!VehPickupItem_MissileCandidateVisible(pb, candidate))
		{
			continue;
		}

		s32 dx = CTR_MipsSra(CTR_MipsSubLo(candidate->posCurr.x, driver->posCurr.x), 8);
		s32 dz = CTR_MipsSra(CTR_MipsSubLo(candidate->posCurr.z, driver->posCurr.z), 8);
		s32 distance = CTR_MipsAddLo(CTR_MipsMulLo(dx, dx), CTR_MipsMulLo(dz, dz));
		if (distance < closestDistance)
		{
			closestDistance = distance;
			target = candidate;
		}
	}

	return target;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800652c8-0x8006540c.
u32 VehPickupItem_PotionThrow(struct MineWeapon *mine, struct Instance *inst, u32 flags)
{
	s32 throwVelocity;

	if ((flags & 4) == 0)
	{
		if ((flags & 2) == 0)
		{
			if ((flags & 1) == 0)
			{
				return 0;
			}

			throwVelocity = (MixRNG_Scramble() & 0x1f) - 0x10;
		}
		else
		{
			throwVelocity = -0x78;
		}
	}
	else
	{
		throwVelocity = 0x78;
	}

	mine->velocity.x = (inst->matrix.m[0][2] * throwVelocity) >> 12;
	mine->velocity.y = 0x30;
	mine->velocity.z = (inst->matrix.m[2][2] * throwVelocity) >> 12;
	mine->crateInst = NULL;
	mine->flags |= MINE_WEAPON_FLAG_THROWN;

	return 1;
}

void VehPickupItem_ShootNow(struct Driver *d, int weaponID, int flags)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006540c-0x800666e4.
	struct Instance *dInst;
	struct Thread *weaponTh;
	struct Instance *weaponInst;
	struct MineWeapon *mw;
	struct TrackerWeapon *tw;
	struct GameTracker *gGT = sdata->gGT;
	int modelID;
	int mineHitModel = 0;
	int mineShouldInitFollower = 0;

	switch (weaponID)
	{
	// Turbo
	case 0:
	{
		int boost = 0x80;
		if (d->numWumpas >= 10)
		{
			boost = 0x100;
		}

		VehFire_Increment(d, 0x960, 9, boost);
	}
	break;

	// Shared code for Bomb and Missile
	case 2:
		if (gGT->numMissiles >= 12)
		{
			return;
		}

		gGT->numMissiles++;
		d->numTimesMissileLaunched++;

		GAMEPAD_ShockFreq(d, 8, 0);
		GAMEPAD_ShockForce1(d, 8, 0x7f);

		struct Driver *victim = VehPickupItem_MissileGetTargetDriver(d);

		// if driver not found
		if (victim == 0)
		{
			// if not battle mode
			if ((gGT->gameMode1 & 0x20) == 0)
			{
				if (gGT->elapsedEventTime & 1)
				{
					// if not DYNAMIC_PLAYER
					if (d->instSelf->thread->modelIndex != DYNAMIC_PLAYER)
					{
						int rank = d->driverRank;
						if (rank != 0)
						{
							victim = gGT->driversInRaceOrder[rank - 1];
						}
					}
				}
			}

			else
			{
				int closest = 0x7fffffff;

				for (int i = 0; i < 8; i++)
				{
					struct Driver *tempD = gGT->drivers[i];

					if (tempD == 0)
					{
						continue;
					}
					if (tempD == d)
					{
						continue;
					}
					if (tempD->invisibleTimer != 0)
					{
						continue;
					}
					if (tempD->kartState == KS_MASK_GRABBED)
					{
						continue;
					}
					if (tempD->BattleHUD.teamID == d->BattleHUD.teamID)
					{
						continue;
					}

					int distX = CTR_MipsSra(CTR_MipsSubLo(tempD->posCurr.x, d->posCurr.x), 8);
					int distZ = CTR_MipsSra(CTR_MipsSubLo(tempD->posCurr.z, d->posCurr.z), 8);

					int dist = CTR_MipsAddLo(CTR_MipsMulLo(distX, distX), CTR_MipsMulLo(distZ, distZ));
					if (dist < closest)
					{
						closest = dist;
						victim = tempD;
					}
				}
			}
		}

		dInst = d->instSelf;

		// set up missile
		modelID = DYNAMIC_ROCKET;
		int bucket = TRACKING;
		struct Thread *parentTh = 0;
		char *weaponName = rdata.s_bombtracker1;

		// bomb
		if ((d->heldItemID == 1) || (d->heldItemID == 10))
		{
			modelID = DYNAMIC_BOMB;
			bucket = OTHER;
			parentTh = dInst->thread;
			weaponName = sdata->s_bomb1;
		}

		// medium stack pool
		weaponInst = INSTANCE_BirthWithThread(modelID, weaponName, MEDIUM, bucket, RB_MovingExplosive_ThTick, sizeof(struct TrackerWeapon), parentTh);

		// NOTE(aalhendi): Native low-RAM audit candidate only. Retail
		// dereferences weapon birth results before later checks in several
		// branches of this function; keep unpatched until memory pressure or
		// gameplay repro proves the semantic fallback.

		VehPickupItem_CopyMatrix(&weaponInst->matrix, &dInst->matrix);

		VehPhysForce_RotAxisAngle(&weaponInst->matrix, d->AxisAngle1_normalVec.v, d->rotCurr.y);

		weaponTh = weaponInst->thread;
		weaponTh->funcThDestroy = PROC_DestroyTracker;
		weaponTh->funcThCollide = (void *)RB_Hazard_ThCollide_Missile;

		tw = weaponTh->object;
		tw->flags = 0;
		tw->framesSeekTargetTnt = 0;
		tw->soundIDCount = 0;
		tw->timeAlive = 0;
		tw->driverParent = d;
		tw->driverTarget = victim;
		tw->instParent = dInst;

		int talk;

		// bomb
		if (modelID == DYNAMIC_BOMB)
		{
			talk = 10;
			d->instBombThrow = weaponInst;

			SVECTOR rot;
			CTR_MatrixToRot(&rot, &weaponInst->matrix, 0x11);

			// not a typo, required like this
			tw->dir.x = rot.vy;
			tw->dir.y = rot.vx;
			tw->dir.z = rot.vz;

			PlaySound3D(0x47, weaponInst);
		}

		// missile
		else
		{
			talk = 11;

			if (victim != 0)
			{
				if (victim->thTrackingMe == 0)
				{
					victim->thTrackingMe = RB_GetThread_ClosestTracker(victim);
				}
			}

			PlaySound3D(0x4a, weaponInst);
		}

		// if human and not AI
		if ((d->actionsFlagSet & ACTION_BOT) == 0)
		{
			Voiceline_RequestPlay(talk, data.characterIDs[d->driverID], 0x10);
		}

		tw->rotY = d->rotCurr.y;

		// do NOT patch for 60fps,
		// velocity uses elapsedTime
		tw->vel.y = 0;
		tw->vel.x = (weaponInst->matrix.m[0][2] * 3) >> 7;
		tw->vel.z = (weaponInst->matrix.m[2][2] * 3) >> 7;

		if (d->numWumpas >= 10)
		{
			tw->flags |= TRACKER_FLAG_POWERED_UP;
		}

		// bomb
		if (modelID == DYNAMIC_BOMB)
		{
			struct GamepadBuffer *gb = &sdata->gGamepads->gamepad[d->driverID];

			if (
			    // hold d-pad DOWN
			    ((gb->buttonsHeldCurrFrame & BTN_DOWN) != 0) ||

			    // pinstripe
			    ((flags & 2) != 0))
			{
				tw->flags |= TRACKER_FLAG_BOMB_BACKWARD;

				tw->vel.x = -(((tw->vel.x >> 1) * 3) / 5);
				tw->vel.z = -(((tw->vel.z >> 1) * 3) / 5);
			}
		}

		// missile
		else
		{
			if (d->numWumpas < 10)
			{
				// do NOT patch for 60fps,
				// velocity uses elapsedTime
				tw->vel.x = (weaponInst->matrix.m[0][2] * 5) >> 8;
				tw->vel.z = (weaponInst->matrix.m[2][2] * 5) >> 8;
			}
		}

		tw->parentSafetyFrames = 60;
		tw->blindFrames = 0;
		break;

	// TNT/Nitro
	case 3:

		// tnt or nitro
		modelID = STATIC_CRATE_TNT;
		char *mineName = sdata->s_tnt1;
		if (d->numWumpas >= 10)
		{
			modelID = PU_EXPLOSIVE_CRATE;
			mineName = sdata->s_nitro1;
		}

		weaponInst = INSTANCE_BirthWithThread(modelID, mineName, SMALL, MINE, RB_GenericMine_ThTick, sizeof(struct MineWeapon), 0);

		dInst = d->instSelf;

		VehPickupItem_CopyMatrix(&weaponInst->matrix, &dInst->matrix);

		weaponInst->scale.x = 0;
		weaponInst->scale.y = 0;
		weaponInst->scale.z = 0;

		weaponTh = weaponInst->thread;
		weaponTh->funcThDestroy = PROC_DestroyInstance;
		weaponTh->funcThCollide = (void *)RB_Hazard_ThCollide_Generic;

		PlaySound3D(0x52, weaponInst);

		// if human and not AI
		if ((d->actionsFlagSet & ACTION_BOT) == 0)
		{
			Voiceline_RequestPlay(0xf, data.characterIDs[d->driverID], 0x10);
		}

		mw = weaponTh->object;
		mw->driverTarget = 0;
		mw->instParent = dInst;
		mw->crateInst = 0;
		VehPickupItem_ClearMineMotion(mw);
		mw->boolDestroyed = 0;
		mw->parentSafetyFrames = 10;
		mw->tntSpinY = 0;
		mw->flags = 0;

		RB_MinePool_Add(mw);
		VehPickupItem_PotionThrow(mw, weaponInst, flags);
		mineHitModel = weaponInst->model->id | COLL_MODELID_BLOCKAGE_FLAG;
		mineShouldInitFollower = (flags == 0);

	RunMineCOLL:;

		SVec3 probeTop;
		SVec3 probeBottom;

		probeTop.x = (s16)(u16)weaponInst->matrix.t[0];
		probeTop.y = (s16)CTR_MipsAddLo((u16)weaponInst->matrix.t[1], -400);
		probeTop.z = (s16)(u16)weaponInst->matrix.t[2];

		probeBottom.x = (s16)(u16)weaponInst->matrix.t[0];
		probeBottom.y = (s16)CTR_MipsAddLo((u16)weaponInst->matrix.t[1], 64);
		probeBottom.z = (s16)(u16)weaponInst->matrix.t[2];

		struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);

		sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND;
		sps->Union.QuadBlockColl.quadFlagsIgnored = 0;

		sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES;
		if (gGT->numPlyrCurrGame < 3)
		{
			sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_TEST_INSTANCES | COLL_SEARCH_HIGH_LOD;
		}

		sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;

		COLL_SearchBSP_CallbackQUADBLK(&probeTop, &probeBottom, sps, 0x40);

		if (sps->boolDidTouchHitbox != 0)
		{
			sps->Input1.modelID = mineHitModel;

			RB_Hazard_CollLevInst(sps, weaponTh);

			struct InstDef *instDef = sps->bspHitbox->data.hitbox.instDef;

			s16 modelTouched = instDef->modelID;
			if ((modelTouched == PU_FRUIT_CRATE) || (modelTouched == PU_RANDOM_CRATE))
			{
				mw->crateInst = instDef->ptrInstance;
			}

			else
			{
				RB_GenericMine_ThDestroy(weaponTh, weaponInst, mw);
			}

			sps->Union.QuadBlockColl.searchFlags = 0;
			COLL_SearchBSP_CallbackQUADBLK(&probeTop, &probeBottom, sps, 0);
		}

		RB_MakeInstanceReflective(sps, weaponInst);

		SVec3 fallbackNormal;
		s16 *rotationNormal;

		if (sps->boolDidTouchQuadblock == 0)
		{
			fallbackNormal = (SVec3){.x = 0, .y = COLL_FRACTION_ONE, .z = 0};
			rotationNormal = fallbackNormal.v;

			mw->stopFallAtY = weaponInst->matrix.t[1];
		}

		else
		{
			mw->stopFallAtY = sps->Union.QuadBlockColl.hitPos.y;
			rotationNormal = sps->hit.plane.normal.v;
		}

		VehPhysForce_RotAxisAngle(&weaponInst->matrix, rotationNormal, d->angle);

		d->instTntSend = weaponInst;

		// dropped a mine
		d->actionsFlagSet |= ACTION_DROPPING_MINE;

		if (mineShouldInitFollower != 0)
		{
			RB_Follower_Init(d, weaponTh);
		}
		break;

	// Beaker
	case 4:

		if (d->numWumpas < 10)
		{
			modelID = STATIC_BEAKER_GREEN;

			weaponInst = INSTANCE_BirthWithThread(modelID, sdata->s_beaker1, SMALL, MINE, RB_GenericMine_ThTick, sizeof(struct MineWeapon), 0);
			if (weaponInst == 0)
			{
				return;
			}
		}
		else
		{
			modelID = STATIC_BEAKER_RED;

			weaponInst = INSTANCE_BirthWithThread(modelID, sdata->s_beaker1, SMALL, MINE, RB_GenericMine_ThTick, sizeof(struct MineWeapon), 0);
		}

		dInst = d->instSelf;

		VehPickupItem_CopyMatrix(&weaponInst->matrix, &dInst->matrix);

		// potion always faces camera
		weaponInst->model->headers[0].flags |= 2;

		weaponTh = weaponInst->thread;
		weaponTh->funcThDestroy = PROC_DestroyInstance;
		weaponTh->funcThCollide = (void *)RB_Hazard_ThCollide_Generic;

		PlaySound3D(0x52, weaponInst);

		// if human and not AI
		if ((d->actionsFlagSet & ACTION_BOT) == 0)
		{
			Voiceline_RequestPlay(0xf, data.characterIDs[d->driverID], 0x10);
		}

		mw = weaponTh->object;
		mw->driverTarget = 0;
		mw->instParent = dInst;
		mw->crateInst = 0;
		mw->boolDestroyed = 0;
		mw->parentSafetyFrames = 10;
		mw->flags = 0;
		if (modelID == STATIC_BEAKER_RED)
		{
			mw->flags = MINE_WEAPON_FLAG_RED_BEAKER;
		}

		struct GamepadBuffer *gb = &sdata->gGamepads->gamepad[d->driverID];

		// throw potion forward
		if ((gb->buttonsHeldCurrFrame & BTN_UP) != 0)
		{
			flags |= 4;
		}

		RB_MinePool_Add(mw);
		int ret = VehPickupItem_PotionThrow(mw, weaponInst, flags);

		if (ret == 0)
		{
			weaponInst->scale.x = 0;
			weaponInst->scale.y = 0;
			weaponInst->scale.z = 0;

			VehPickupItem_ClearMineMotion(mw);

			mineHitModel = weaponInst->model->id;
			mineShouldInitFollower = 1;
			goto RunMineCOLL;
		}
		break;

	// Shield Bubble
	case 6:;

		char *shieldDarkName = rdata.s_shielddark;
		char *highlightName = rdata.s_highlight;

		weaponInst = INSTANCE_BirthWithThread(0x5a, shieldDarkName, MEDIUM, OTHER, RB_ShieldDark_ThTick_Grow, sizeof(struct Shield), d->instSelf->thread);

		weaponTh = weaponInst->thread;
		weaponInst->scale.x = 0x700;
		weaponInst->scale.y = 0x700;
		weaponInst->scale.z = 0x700;
		weaponTh->funcThDestroy = PROC_DestroyInstance;
		OtherFX_Play(0x57, 1);

		modelID = DYNAMIC_SHIELD_GREEN;
		if (d->numWumpas >= 10)
		{
			modelID = DYNAMIC_SHIELD;
		}

		struct Instance *instColor = INSTANCE_Birth3D(gGT->modelPtr[modelID], sdata->s_shield, 0);

		struct Instance *instHighlight = INSTANCE_Birth3D(gGT->modelPtr[DYNAMIC_HIGHLIGHT], highlightName, weaponTh);

		instColor->scale.x = 0x700;
		instColor->scale.y = 0x700;
		instColor->scale.z = 0x700;

		instHighlight->scale.x = 0x700;
		instHighlight->scale.y = 0x700;
		instHighlight->scale.z = 0x700;

		struct Shield *shieldObj = weaponTh->object;
		shieldObj->animFrame = 0;
		shieldObj->flags = 0;
		shieldObj->instColor = instColor;
		shieldObj->instHighlight = instHighlight;
		shieldObj->highlightRot.x = 0;
		shieldObj->highlightRot.y = 0xc00;
		shieldObj->highlightRot.z = 0;
		shieldObj->highlightTimer = 0;

		if (d->numWumpas < 10)
		{
			shieldObj->duration = 0x2d00;
		}
		else
		{
			shieldObj->flags = SHIELD_FLAG_BLUE;
		}

		weaponInst->alphaScale = 0x400;
		d->instBubbleHold = weaponInst;
		break;

	// Mask
	case 7:
		VehPickupItem_MaskUseWeapon(d, 1);
		break;

	// Clock
	case 8:

		d->numTimesClockWeaponUsed++;

		OtherFX_Play(0x44, 1);

		if ((d->actionsFlagSet & ACTION_BOT) == 0)
		{
			Voiceline_RequestPlay(0xe, data.characterIDs[d->driverID], 0x10);
		}

		int hurtVal = 0x1e00;
		if (d->numWumpas >= 10)
		{
			hurtVal = 0x2d00;
		}

		struct Driver **dptr;

		for (dptr = &gGT->drivers[0]; dptr < &gGT->drivers[8]; dptr++)
		{
			struct Driver *victim = *dptr;

			if (victim == 0)
			{
				continue;
			}

			victim->clockFlash = 4;

			if (victim == d)
			{
				d->clockSend = 0x1e;
				continue;
			}

			// if spin out driver
			if (RB_Hazard_HurtDriver(victim, 1, 0, 0) != 0)
			{
				victim->clockReceive = hurtVal;
			}
		}
		break;

	// Warpball
	case 9:

		dInst = d->instSelf;
		GAMEPAD_ShockFreq(d, 8, 0);
		GAMEPAD_ShockForce1(d, 8, 0x7f);

		// MEDIUM
		char *warpballName = rdata.s_warpball;

		weaponInst = INSTANCE_BirthWithThread(0x36, warpballName, MEDIUM, TRACKING, RB_Warpball_ThTick, sizeof(struct TrackerWeapon), 0);

		weaponInst->matrix.m[0][0] = 0x1000;
		weaponInst->matrix.m[0][1] = 0;
		weaponInst->matrix.m[0][2] = 0;
		weaponInst->matrix.m[1][0] = 0;
		weaponInst->matrix.m[1][1] = 0x1000;
		weaponInst->matrix.m[1][2] = 0;
		weaponInst->matrix.m[2][0] = 0;
		weaponInst->matrix.m[2][1] = 0;
		weaponInst->matrix.m[2][2] = 0x1000;

		weaponInst->matrix.t[0] = CTR_MipsSra(d->posCurr.x, 8);
		weaponInst->matrix.t[1] = CTR_MipsSra(d->posCurr.y, 8);
		weaponInst->matrix.t[2] = CTR_MipsSra(d->posCurr.z, 8);

		weaponTh = weaponInst->thread;
		weaponTh->funcThDestroy = PROC_DestroyInstance;

		PlaySound3D(0x4d, weaponInst);

		// if human and not AI (AIs can not use Warpball)
		if ((d->actionsFlagSet & ACTION_BOT) == 0)
		{
			Voiceline_RequestPlay(0xc, data.characterIDs[d->driverID], 0x10);
		}

		// used by RB_Warpball_SeekDriver
		victim = 0;
		int rank = d->driverRank;
		if (rank != 0)
		{
			victim = gGT->driversInRaceOrder[rank - 1];
		}

		tw = weaponTh->object;
		tw->flags = TRACKER_FLAG_WARPBALL_FALLBACK_PATH;
		tw->soundIDCount = 0;
		tw->ptrNodeNext = 0;
		tw->pathProgress = 0;
		tw->turnAroundFrames = 0;
		tw->driverParent = d;
		tw->driverTarget = victim;
		tw->instParent = dInst;

		if (d->numWumpas >= 10)
		{
			tw->flags |= TRACKER_FLAG_POWERED_UP;
		}

		// sets nodeCurrIndex
		RB_Warpball_SeekDriver(tw, d->checkpoint.currentIndex, d);

		struct CheckpointNode *cn = gGT->level1->ptr_restart_points;
		tw->nodeNextIndex = tw->nodeCurrIndex;
		tw->ptrNodeCurr = &cn[tw->nodeCurrIndex];

		// make this driver invincible
		tw->driversHit = 1 << d->driverID;

		victim = 0;
		if (rank != 0)
		{
			victim = RB_Warpball_GetDriverTarget(tw, weaponInst);
		}
		tw->driverTarget = victim;

		if (victim != 0)
		{
			RB_Warpball_SetTargetDriver(tw);
		}

		if ((tw->flags & 4) == 0)
		{
			RB_Warpball_Start(tw);
		}
		else
		{
			tw->flags &= ~TRACKER_FLAG_WARPBALL_FALLBACK_PATH;
		}

		tw->ptrNodeNext = RB_Warpball_NewPathNode(tw->ptrNodeCurr, victim);

		tw->vel.y = 0;
		tw->rotY = d->angle;
		tw->parentSafetyFrames = 10;

		// do NOT patch for 60fps,
		// velocity uses elapsedTime
		tw->vel.x = (dInst->matrix.m[0][2] * 7) >> 8;
		tw->vel.z = (dInst->matrix.m[2][2] * 7) >> 8;

		struct Particle *p = Particle_Init(0, gGT->iconGroup[0], &data.emSet_Warpball[0]);

		tw->ptrParticle = p;

		if (p != 0)
		{
			p->otIndexOffset = (s8)0xfa;
		}

		break;

	// invisibility
	case 0xc:

		if (d->invisibleTimer == 0)
		{
			d->instFlagsBackup = d->instSelf->flags;

			d->instSelf->flags = (d->instSelf->flags & 0xfff8ffff) | GHOST_DRAW_TRANSPARENT;

			OtherFX_Play(0x61, 1);
		}

		int time = 0x1e00;
		if (d->numWumpas >= 10)
		{
			time = 0x2d00;
		}

		d->invisibleTimer = time;
		break;


	// Super Engine
	case 0xd:
	{
		int engine = 0x1e00;
		if (d->numWumpas >= 10)
		{
			engine = 0x2d00;
		}

		d->superEngineTimer = engine;
	}
	break;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800666e4-0x8006677c.
void VehPickupItem_ShootOnCirclePress(struct Driver *d)
{
	u8 weapon;

	if (d->pendingDamageType != 0)
	{
		VehPickState_NewState(d, d->pendingDamageType, d->pendingDamageAttacker, d->pendingDamageReasonByte);
	}

	// If you want to fire a weapon
	if ((d->actionsFlagSet & ACTION_WEAPON_FIRE_REQUEST) == 0)
	{
		return;
	}

	// Remove the request to fire a weapon, since we will fire it now
	d->actionsFlagSet &= ~ACTION_WEAPON_FIRE_REQUEST;

	weapon = d->heldItemID;

	// Missiles and Bombs share code,
	// Change Bomb1x, Bomb3x, Missile3x, to Missile1x
	if ((weapon == 1) || (weapon == 10) || (weapon == 11))
	{
		weapon = 2;
	}

	VehPickupItem_ShootNow(d, (int)weapon, 0);
}
