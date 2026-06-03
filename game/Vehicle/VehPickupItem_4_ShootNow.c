#include <common.h>

void RB_GenericMine_ThTick(struct Thread *t);
u16 RB_Hazard_CollLevInst(struct ScratchpadStruct *sps, struct Thread *th);
void RB_GenericMine_ThDestroy(struct Thread *t, struct Instance *inst, struct MineWeapon *mw);
void RB_ShieldDark_ThTick_Grow(struct Thread *t);
void RB_Warpball_ThTick(struct Thread *t);
struct CheckpointNode *RB_Warpball_NewPathNode(struct CheckpointNode *ptrNodeCurr, struct Driver *victim);
void RB_Warpball_Start(struct TrackerWeapon *tw);
void RB_Warpball_SetTargetDriver(struct TrackerWeapon *tw);
struct Driver *RB_Warpball_GetDriverTarget(struct TrackerWeapon *tw, struct Instance *inst);
void RB_Warpball_SeekDriver(struct TrackerWeapon *tw, u32 param_2, struct Driver *d);

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
			boost = 0x100;

		VehFire_Increment(d, 0x960, 9, boost);
	}
	break;

	// Shared code for Bomb and Missile
	case 2:
		if (gGT->numMissiles >= 12)
			return;

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
						continue;
					if (tempD == d)
						continue;
					if (tempD->invisibleTimer != 0)
						continue;
					if (tempD->kartState == KS_MASK_GRABBED)
						continue;
					if (tempD->BattleHUD.teamID == d->BattleHUD.teamID)
						continue;

					int distX = tempD->posCurr.x - d->posCurr.x;
					int distZ = tempD->posCurr.z - d->posCurr.z;
					distX = distX >> 8;
					distZ = distZ >> 8;

					int dist = distX * distX + distZ * distZ;
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

		// copy matrix
		*(int *)&weaponInst->matrix.m[0][0] = *(int *)&dInst->matrix.m[0][0];
		*(int *)&weaponInst->matrix.m[0][2] = *(int *)&dInst->matrix.m[0][2];
		*(int *)&weaponInst->matrix.m[1][1] = *(int *)&dInst->matrix.m[1][1];
		*(int *)&weaponInst->matrix.m[2][0] = *(int *)&dInst->matrix.m[2][0];
		weaponInst->matrix.m[2][2] = dInst->matrix.m[2][2];
		weaponInst->matrix.t[0] = dInst->matrix.t[0];
		weaponInst->matrix.t[1] = dInst->matrix.t[1];
		weaponInst->matrix.t[2] = dInst->matrix.t[2];

		VehPhysForce_RotAxisAngle(&weaponInst->matrix, (s16 *)&d->AxisAngle1_normalVec, d->rotCurr.y);

		weaponTh = weaponInst->thread;
		weaponTh->funcThDestroy = PROC_DestroyTracker;
		weaponTh->funcThCollide = RB_Hazard_ThCollide_Missile;

		tw = weaponTh->object;
		tw->flags = 0;
		tw->framesSeekMine = 0;
		tw->audioPtr = 0;
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

			// Original Code
			s16 rot[3];
			CTR_MatrixToRot((SVECTOR *)&rot[0], &weaponInst->matrix, 0x11);

			// not a typo, required like this
			tw->dir[0] = rot[1];
			tw->dir[1] = rot[0];
			tw->dir[2] = rot[2];

			PlaySound3D(0x47, weaponInst);
		}

		// missile
		else
		{
			talk = 11;

			if (victim != 0)
				if (victim->thTrackingMe == 0)
					victim->thTrackingMe = RB_GetThread_ClosestTracker(victim);

			PlaySound3D(0x4a, weaponInst);
		}

		// if human and not AI
		if ((d->actionsFlagSet & 0x100000) == 0)
		{
			Voiceline_RequestPlay(talk, data.characterIDs[d->driverID], 0x10);
		}

		tw->rotY = d->rotCurr.y;

		// do NOT patch for 60fps,
		// velocity uses elapsedTime
		tw->vel[1] = 0;
		tw->vel[0] = (weaponInst->matrix.m[0][2] * 3) >> 7;
		tw->vel[2] = (weaponInst->matrix.m[2][2] * 3) >> 7;

		if (d->numWumpas >= 10)
		{
			tw->flags |= 1;
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
				tw->flags |= 0x20;

				tw->vel[0] = -(((tw->vel[0] >> 1) * 3) / 5);
				tw->vel[2] = -(((tw->vel[2] >> 1) * 3) / 5);
			}
		}

		// missile
		else
		{
			if (d->numWumpas < 10)
			{
				// do NOT patch for 60fps,
				// velocity uses elapsedTime
				tw->vel[0] = (weaponInst->matrix.m[0][2] * 5) >> 8;
				tw->vel[2] = (weaponInst->matrix.m[2][2] * 5) >> 8;
			}
		}

		tw->frameCount_DontHurtParent = 60;
		tw->frameCount_Blind = 0;
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

		// copy matrix
		*(int *)&weaponInst->matrix.m[0][0] = *(int *)&dInst->matrix.m[0][0];
		*(int *)&weaponInst->matrix.m[0][2] = *(int *)&dInst->matrix.m[0][2];
		*(int *)&weaponInst->matrix.m[1][1] = *(int *)&dInst->matrix.m[1][1];
		*(int *)&weaponInst->matrix.m[2][0] = *(int *)&dInst->matrix.m[2][0];
		weaponInst->matrix.m[2][2] = dInst->matrix.m[2][2];
		weaponInst->matrix.t[0] = dInst->matrix.t[0];
		weaponInst->matrix.t[1] = dInst->matrix.t[1];
		weaponInst->matrix.t[2] = dInst->matrix.t[2];

		weaponInst->scale[0] = 0;
		weaponInst->scale[1] = 0;
		weaponInst->scale[2] = 0;

		weaponTh = weaponInst->thread;
		weaponTh->funcThDestroy = PROC_DestroyInstance;
		weaponTh->funcThCollide = RB_Hazard_ThCollide_Generic;

		PlaySound3D(0x52, weaponInst);

		// if human and not AI
		if ((d->actionsFlagSet & 0x100000) == 0)
		{
			Voiceline_RequestPlay(0xf, data.characterIDs[d->driverID], 0x10);
		}

		mw = weaponTh->object;
		mw->driverTarget = 0;
		mw->instParent = dInst;
		mw->crateInst = 0;
		*(int *)&mw->velocity[0] = 0;
		*(int *)&mw->velocity[2] = 0;
		mw->boolDestroyed = 0;
		mw->frameCount_DontHurtParent = 10;
		mw->tntSpinY = 0;
		mw->extraFlags = 0;

		RB_MinePool_Add(mw);
		VehPickupItem_PotionThrow(mw, weaponInst, flags);
		mineHitModel = weaponInst->model->id | 0x8000;
		mineShouldInitFollower = (flags == 0);

	RunMineCOLL:

		s16 pos1[3];
		s16 pos2[3];

		pos1[0] = weaponInst->matrix.t[0];
		pos1[1] = weaponInst->matrix.t[1] - 400;
		pos1[2] = weaponInst->matrix.t[2];

		pos2[0] = weaponInst->matrix.t[0];
		pos2[1] = weaponInst->matrix.t[1] + 64;
		pos2[2] = weaponInst->matrix.t[2];

		struct ScratchpadStruct *sps = (struct ScratchpadStruct *)0x1f800108;

		sps->Union.QuadBlockColl.qbFlagsWanted = 0x1000;
		sps->Union.QuadBlockColl.qbFlagsIgnored = 0;

		sps->Union.QuadBlockColl.searchFlags = 1;
		if (gGT->numPlyrCurrGame < 3)
			sps->Union.QuadBlockColl.searchFlags = 3;

		sps->ptr_mesh_info = gGT->level1->ptr_mesh_info;

		COLL_SearchBSP_CallbackQUADBLK((u32 *)pos1, (u32 *)pos2, sps, 0x40);

		if (sps->boolDidTouchHitbox != 0)
		{
			sps->Input1.modelID = mineHitModel;

			RB_Hazard_CollLevInst(sps, weaponTh);

			struct InstDef *instDef = sps->bspHitbox->data.hitbox.instDef;

			int modelTouched = instDef->modelID;

			// fruit crate or weapon crate
			if ((modelTouched == 7) || (modelTouched == 8))
			{
				mw->crateInst = instDef->ptrInstance;
			}

			else
			{
				RB_GenericMine_ThDestroy(weaponTh, weaponInst, mw);
			}

			sps->Union.QuadBlockColl.searchFlags = 0;
			COLL_SearchBSP_CallbackQUADBLK((u32 *)pos1, (u32 *)pos2, sps, 0);
		}

		RB_MakeInstanceReflective(sps, weaponInst);

		s16 *rotPtr = (s16 *)0x1f800178;

		if (sps->boolDidTouchQuadblock == 0)
		{
			rotPtr[0] = 0;
			rotPtr[1] = 0x1000;
			rotPtr[2] = 0;
			rotPtr[3] = 0;

			mw->stopFallAtY = weaponInst->matrix.t[1];
		}

		else
		{
			mw->stopFallAtY = sps->Union.QuadBlockColl.hitPos[1];
		}

		VehPhysForce_RotAxisAngle(&weaponInst->matrix, rotPtr, d->angle);

		d->instTntSend = weaponInst;

		// dropped a mine
		d->actionsFlagSet |= 0x80000000;

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
				return;
		}
		else
		{
			modelID = STATIC_BEAKER_RED;

			weaponInst = INSTANCE_BirthWithThread(modelID, sdata->s_beaker1, SMALL, MINE, RB_GenericMine_ThTick, sizeof(struct MineWeapon), 0);
		}

		dInst = d->instSelf;

		// copy matrix
		*(int *)&weaponInst->matrix.m[0][0] = *(int *)&dInst->matrix.m[0][0];
		*(int *)&weaponInst->matrix.m[0][2] = *(int *)&dInst->matrix.m[0][2];
		*(int *)&weaponInst->matrix.m[1][1] = *(int *)&dInst->matrix.m[1][1];
		*(int *)&weaponInst->matrix.m[2][0] = *(int *)&dInst->matrix.m[2][0];
		weaponInst->matrix.m[2][2] = dInst->matrix.m[2][2];
		weaponInst->matrix.t[0] = dInst->matrix.t[0];
		weaponInst->matrix.t[1] = dInst->matrix.t[1];
		weaponInst->matrix.t[2] = dInst->matrix.t[2];

		// potion always faces camera
		weaponInst->model->headers[0].flags |= 2;

		weaponTh = weaponInst->thread;
		weaponTh->funcThDestroy = PROC_DestroyInstance;
		weaponTh->funcThCollide = RB_Hazard_ThCollide_Generic;

		PlaySound3D(0x52, weaponInst);

		// if human and not AI
		if ((d->actionsFlagSet & 0x100000) == 0)
		{
			Voiceline_RequestPlay(0xf, data.characterIDs[d->driverID], 0x10);
		}

		mw = weaponTh->object;
		mw->driverTarget = 0;
		mw->instParent = dInst;
		mw->crateInst = 0;
		mw->boolDestroyed = 0;
		mw->frameCount_DontHurtParent = 10;
		mw->extraFlags = 0;
		if (modelID == STATIC_BEAKER_RED)
			mw->extraFlags = 1;

		struct GamepadBuffer *gb = &sdata->gGamepads->gamepad[d->driverID];

		// throw potion forward
		if ((gb->buttonsHeldCurrFrame & BTN_UP) != 0)
			flags |= 4;

		RB_MinePool_Add(mw);
		int ret = VehPickupItem_PotionThrow(mw, weaponInst, flags);

		if (ret == 0)
		{
			// similar to TNT, but
			// if VehPickupItem_PotionThrow == 0?
			weaponInst->scale[0] = 0;
			weaponInst->scale[1] = 0;
			weaponInst->scale[2] = 0;

			// similar to TNT, but
			// If VehPickupItem_PotionThrow == 0?
			*(int *)&mw->velocity[0] = 0;
			*(int *)&mw->velocity[2] = 0;

			mineHitModel = weaponInst->model->id;
			mineShouldInitFollower = 1;
			goto RunMineCOLL;
		}
		break;

	// Shield Bubble
	case 6:

		char *shieldDarkName = rdata.s_shielddark;
		char *highlightName = rdata.s_highlight;

		weaponInst = INSTANCE_BirthWithThread(0x5a, shieldDarkName, MEDIUM, OTHER, RB_ShieldDark_ThTick_Grow, sizeof(struct Shield), d->instSelf->thread);

		weaponTh = weaponInst->thread;
		weaponInst->scale[0] = 0x700;
		weaponInst->scale[1] = 0x700;
		weaponInst->scale[2] = 0x700;
		weaponTh->funcThDestroy = PROC_DestroyInstance;
		OtherFX_Play(0x57, 1);

		modelID = DYNAMIC_SHIELD_GREEN;
		if (d->numWumpas >= 10)
			modelID = DYNAMIC_SHIELD;

		struct Instance *instColor = INSTANCE_Birth3D(gGT->modelPtr[modelID], sdata->s_shield, 0);

		struct Instance *instHighlight = INSTANCE_Birth3D(gGT->modelPtr[DYNAMIC_HIGHLIGHT], highlightName, weaponTh);

		instColor->scale[0] = 0x700;
		instColor->scale[1] = 0x700;
		instColor->scale[2] = 0x700;

		instHighlight->scale[0] = 0x700;
		instHighlight->scale[1] = 0x700;
		instHighlight->scale[2] = 0x700;

		struct Shield *shieldObj = weaponTh->object;
		shieldObj->animFrame = 0;
		shieldObj->flags = 0;
		shieldObj->instColor = instColor;
		shieldObj->instHighlight = instHighlight;
		shieldObj->highlightRot[0] = 0;
		shieldObj->highlightRot[1] = 0xc00;
		shieldObj->highlightRot[2] = 0;
		shieldObj->highlightTimer = 0;

		if (d->numWumpas < 10)
		{
			shieldObj->duration = 0x2d00;
		}
		else
		{
			shieldObj->flags = 4;
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

		if ((d->actionsFlagSet & 0x100000) == 0)
		{
			Voiceline_RequestPlay(0xe, data.characterIDs[d->driverID], 0x10);
		}

		int hurtVal = 0x1e00;
		if (d->numWumpas >= 10)
			hurtVal = 0x2d00;

		struct Driver **dptr;

		for (dptr = &gGT->drivers[0]; dptr < &gGT->drivers[8]; dptr++)
		{
			struct Driver *victim = *dptr;

			if (victim == 0)
				continue;

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

		*(int *)&weaponInst->matrix.m[0][0] = 0x1000;
		*(int *)&weaponInst->matrix.m[0][2] = 0;
		*(int *)&weaponInst->matrix.m[1][1] = 0x1000;
		*(int *)&weaponInst->matrix.m[2][0] = 0;
		weaponInst->matrix.m[2][2] = 0x1000;

		weaponInst->matrix.t[0] = d->posCurr.x >> 8;
		weaponInst->matrix.t[1] = d->posCurr.y >> 8;
		weaponInst->matrix.t[2] = d->posCurr.z >> 8;

		weaponTh = weaponInst->thread;
		weaponTh->funcThDestroy = PROC_DestroyInstance;

		PlaySound3D(0x4d, weaponInst);

		// if human and not AI (AIs can not use Warpball)
		if ((d->actionsFlagSet & 0x100000) == 0)
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
		tw->flags = 8;
		tw->audioPtr = 0;
		tw->ptrNodeNext = 0;
		tw->respawnPointIndex = 0;
		tw->turnAround = 0;
		tw->driverParent = d;
		tw->driverTarget = victim;
		tw->instParent = dInst;

		if (d->numWumpas >= 10)
			tw->flags |= 1;

		// sets nodeCurrIndex
		RB_Warpball_SeekDriver(tw, d->unknown_lap_related[1], d);

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
			tw->flags &= 0xfff7;
		}

		tw->ptrNodeNext = RB_Warpball_NewPathNode(tw->ptrNodeCurr, victim);

		tw->vel[1] = 0;
		tw->rotY = d->angle;
		tw->frameCount_DontHurtParent = 10;

		// do NOT patch for 60fps,
		// velocity uses elapsedTime
		tw->vel[0] = (dInst->matrix.m[0][2] * 7) >> 8;
		tw->vel[2] = (dInst->matrix.m[2][2] * 7) >> 8;

		struct Particle *p = Particle_Init(0, gGT->iconGroup[0], &data.emSet_Warpball[0]);

		tw->ptrParticle = p;

		if (p != 0)
			p->unk18 = 250;

		break;

	// invisibility
	case 0xc:

		if (d->invisibleTimer == 0)
		{
			d->instFlagsBackup = d->instSelf->flags;

			d->instSelf->flags = (d->instSelf->flags & 0xfff8ffff) | 0x60000;

			OtherFX_Play(0x61, 1);
		}

		int time = 0x1e00;
		if (d->numWumpas >= 10)
			time = 0x2d00;

		d->invisibleTimer = time;
		break;


	// Super Engine
	case 0xd:
	{
		int engine = 0x1e00;
		if (d->numWumpas >= 10)
			engine = 0x2d00;

		d->superEngineTimer = engine;
	}
	break;
	}
}
