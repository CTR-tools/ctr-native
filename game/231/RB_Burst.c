#include <common.h>
#include <ctr_gte_transfer.h>
#include "RB_Effect.h"

struct Burst
{
	struct Instance *shockwave;
	struct Instance *explosion;
	struct Instance *rotatedExplosion;
};

void RB_Burst_ProcessBucket(struct Thread *thread)
{
	for (; thread != NULL; thread = thread->siblingThread)
	{
		struct Burst *burst = thread->object;

		{
			s32 i;
			for (i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
			{
				if (burst->explosion == NULL)
				{
					continue;
				}

				if (burst->shockwave != NULL)
				{
					burst->shockwave->idpp[i].instFlags &= burst->explosion->idpp[i].instFlags | ~DRAW_SUCCESSFUL;
					burst->shockwave->idpp[i].otRangeNormal = burst->explosion->idpp[i].otRangeNormal;
					burst->shockwave->idpp[i].depthOffset[0] = burst->explosion->idpp[i].depthOffset[0];
					burst->shockwave->idpp[i].depthOffset[1] = burst->explosion->idpp[i].depthOffset[1];
				}

				if (burst->rotatedExplosion != NULL)
				{
					burst->rotatedExplosion->idpp[i].instFlags &= burst->explosion->idpp[i].instFlags | ~DRAW_SUCCESSFUL;
					burst->rotatedExplosion->idpp[i].otRangeNormal = burst->explosion->idpp[i].otRangeNormal;
					burst->rotatedExplosion->idpp[i].depthOffset[0] = burst->explosion->idpp[i].depthOffset[0];
					burst->rotatedExplosion->idpp[i].depthOffset[1] = burst->explosion->idpp[i].depthOffset[1];
				}
			}
		}
	}
}

void RB_Burst_ThTick(struct Thread *t)
{
	struct Burst *burst;
	burst = t->object;

	RB_Effect_UpdateSlot(&burst->explosion);
	RB_Effect_UpdateSlot(&burst->rotatedExplosion);
	RB_Effect_UpdateSlot(&burst->shockwave);

	// The shockwave alone does not keep the burst owner alive.
	if ((burst->explosion == NULL) && (burst->rotatedExplosion == NULL))
	{
		t->flags |= THREAD_FLAG_DEAD;
	}
}

// The collision dispatcher passes an opaque hit object; keep thread handling typed.
static inline void RB_Burst_CollThread(struct ScratchpadStruct *sps, struct Thread *t)
{
	void *weaponObj;
	struct Driver *attacker;
	struct TrackerWeapon *tw;
	s32 model;
	struct Thread *weaponTh;

	weaponTh = sps->Union.ThBuckColl.thread;
	if (Chaos_DeferImpact(CHAOS_IMPACT_BURST_THREAD, weaponTh, t, NULL))
	{
		return;
	}
	tw = weaponTh->object;
	weaponObj = weaponTh->object;
	model = t->modelIndex;

	if ((model == DYNAMIC_PLAYER) || (model == DYNAMIC_ROBOT_CAR))
	{
		s32 weaponModel = weaponTh->modelIndex;

		if ((weaponModel == PU_EXPLOSIVE_CRATE) || (weaponModel == STATIC_BEAKER_RED) || (weaponModel == STATIC_BEAKER_GREEN) ||
		    (weaponModel == STATIC_CRATE_TNT))
		{
			attacker = ((struct MineWeapon *)weaponObj)->instParent->thread->object;

			// blast driver
			RB_Hazard_HurtDriver(t->object, 2, attacker, 2);
		}
		else
		{
			attacker = ((struct TrackerWeapon *)weaponObj)->instParent->thread->object;

			// missile
			if (weaponModel == DYNAMIC_ROCKET)
			{
				RB_Hazard_HurtDriver(t->object, 2, attacker, 3);
			}
			else
			{
				RB_Hazard_HurtDriver(t->object, 2, attacker, 1);
			}

			if (attacker->longestShot < tw->timeAlive)
			{
				attacker->longestShot = tw->timeAlive;
			}
		}

		// NOTE(aalhendi): Reacquire the victim through its owner after damage callbacks.
		// Only human players receive the screen flash.
		if ((((struct Driver *)t->object)->actionsFlagSet & ACTION_BOT) == 0)
		{
			struct GameTracker *gGT = GAME_TRACKER;

			gGT->pushBuffer[((struct Driver *)t->object)->driverID].fadeFromBlack_currentValue = 0x1fff;
			gGT->pushBuffer[((struct Driver *)t->object)->driverID].fadeFromBlack_desiredResult = 0x1000;
			gGT->pushBuffer[((struct Driver *)t->object)->driverID].fade_step = -0x88;
		}

		// icon damage timer, draw icon as red
		((struct Driver *)t->object)->damageColorTimer = 0x1e;

		// get modelID from thread
		model = t->modelIndex;
	}

	switch (model)
	{
	case PU_EXPLOSIVE_CRATE:
	case STATIC_CRATE_TNT:
	case DYNAMIC_ROCKET:
	case STATIC_BEAKER_RED:
	case STATIC_BEAKER_GREEN:
	{
		if (t->funcThCollide != NULL)
		{
			// NOTE(aalhendi): Retail passes 3 in the scratch argument. The mine and
			// missile callbacks ignore it; preserve that word with their actual call type.
			((ThreadScratchCollideFunc)t->funcThCollide)(t, sps->Union.ThBuckColl.thread, t->funcThCollide, (struct ScratchpadStruct *)3);
		}
		break;
	}
	}
	return;
}


void RB_Burst_CollThBucket(struct ScratchpadStruct *sps, void *hitObject)
{
	RB_Burst_CollThread(sps, hitObject);
}

void RB_Burst_CollLevInst(struct ScratchpadStruct *sps, void *hitObject)
{
	struct BSP *bspHitbox = hitObject;
	s16 model;
	struct Instance *inst;
	struct InstDef *instdef;
	struct MetaDataMODEL *meta;

	instdef = bspHitbox->data.hitbox.instDef;
	if (instdef == NULL)
	{
		return;
	}

	inst = instdef->ptrInstance;
	if (inst == NULL)
	{
		return;
	}
	if (Chaos_DeferImpact(CHAOS_IMPACT_BURST_LEVEL, sps->Union.ThBuckColl.thread, NULL, bspHitbox))
	{
		return;
	}

	model = instdef->modelID;
	switch (model)
	{
	case PU_FRUIT_CRATE:
	case PU_RANDOM_CRATE:
	{
		meta = COLL_LevModelMeta(model);
		if (meta == NULL)
		{
			return;
		}

		if (meta->LInC == NULL)
		{
			return;
		}

		meta->LInC(inst, sps->Union.ThBuckColl.thread, sps);
		return;
	}

	case STATIC_TEETH:
	{
		RB_Teeth_OpenDoor(inst);
		break;
	}
	}

	return;
}


static inline void RB_Burst_SetPosition(struct Instance *inst, struct Instance *weaponInst)
{
	inst->matrix.t[0] = weaponInst->matrix.t[0];
	inst->matrix.t[1] = (u32)weaponInst->matrix.t[1] - 0x30;
	inst->matrix.t[2] = weaponInst->matrix.t[2];

	// Smaller effects leave more of each split-screen view visible.
	if (GAME_TRACKER->numPlyrCurrGame > 2)
	{
		inst->scale.x >>= 1;
		inst->scale.y >>= 1;
		inst->scale.z >>= 1;
	}
}

void RB_Burst_InitVisual(struct Instance *weaponInst)
{
	struct InstanceBirthParams birth;
	struct Instance *currInst;
	struct Thread *t;
	struct Burst *burst;

	birth.modelID = STATIC_WARPEDBURST;
	birth.name = rb_nameExplosion;
	birth.poolType = SMALL;
	birth.bucket = BURST;
	birth.funcThTick = RB_Burst_ThTick;
	birth.objSize = sizeof(struct Burst);
	birth.parent = NULL;
	currInst = INSTANCE_BirthWithThread_Stack(&birth);

	t = currInst->thread;
	Chaos_InheritThreadOwner(t, weaponInst->thread);
	burst = t->object;
	burst->explosion = currInst;
	currInst->depthBiasNormal -= 2;
	RB_Burst_SetPosition(currInst, weaponInst);
	CTR_MatrixSetRotIdentity(&currInst->matrix);
	{
		struct ModelHeader *headers = currInst->model->headers;
		headers[0].flags |= 2;
	}

	// A second burst is rotated 90 degrees around its billboard axis.
	currInst = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[STATIC_WARPEDBURST], rb_nameBurstExplosion, t);
	burst->rotatedExplosion = currInst;
	currInst->flags |= VISIBLE_DURING_GAMEPLAY;
	currInst->depthBiasNormal -= 2;
	RB_Burst_SetPosition(currInst, weaponInst);
	currInst->matrix.m[0][1] = -0x1000;
	currInst->matrix.m[0][0] = 0;
	currInst->matrix.m[0][2] = 0;
	currInst->matrix.m[1][0] = 0x1000;
	currInst->matrix.m[1][1] = 0;
	currInst->matrix.m[1][2] = 0;
	currInst->matrix.m[2][0] = 0;
	currInst->matrix.m[2][1] = 0;
	currInst->matrix.m[2][2] = 0x1000;
	{
		struct ModelHeader *headers = currInst->model->headers;
		headers[0].flags |= 2;
	}

	currInst = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[STATIC_SHOCKWAVE_RED], rb_nameShockwave, t);
	burst->shockwave = currInst;
	currInst->flags |= VISIBLE_DURING_GAMEPLAY | DRAW_BILLBOARD;
	currInst->depthBiasNormal -= 2;
	RB_Burst_SetPosition(currInst, weaponInst);
	CTR_MatrixSetRotIdentity(&currInst->matrix);
	{
		struct ModelHeader *headers = currInst->model->headers;
		headers[0].flags |= 2;
	}
	{
		struct ModelHeader *headers = currInst->model->headers;
		headers[1].flags |= 2;
	}
}

void RB_Burst_Init(struct Instance *weaponInst)
{
	struct TrackerWeapon *tw = weaponInst->thread->object;
	struct ScratchpadStruct *sps;
	RB_Burst_InitVisual(weaponInst);

	// Damage originates at the weapon, not the lowered visual effects.
	sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
	sps->Input1.pos.x = weaponInst->matrix.t[0];
	sps->Input1.pos.y = weaponInst->matrix.t[1];
	sps->Input1.pos.z = weaponInst->matrix.t[2];

	if (weaponInst->model->id == DYNAMIC_ROCKET)
	{
		sps->Input1.hitRadius = 0x80;
		sps->Input1.hitRadiusSquared = 0x4000;
	}
	else if (((struct TrackerWeapon *)weaponInst->thread->object)->flags & TRACKER_FLAG_POWERED_UP)
	{
		sps->Input1.hitRadius = 0x200;
		sps->Input1.hitRadiusSquared = 0x40000;
	}
	else
	{
		sps->Input1.hitRadius = 0x140;
		sps->Input1.hitRadiusSquared = 0x19000;
	}

    if (weaponInst->model->id == DYNAMIC_BOMB)
    {
      sps->Input1.hitRadius = sps->Input1.hitRadius * g_config.bombExplosionRadiusMultiplier / 100;
      sps->Input1.hitRadiusSquared = sps->Input1.hitRadius * sps->Input1.hitRadius;
    }

	sps->Union.ThBuckColl.funcCallback = RB_Burst_CollThBucket;
	sps->Union.ThBuckColl.thread = weaponInst->thread;
	sps->Input1.modelID = weaponInst->model->id;

	// The firing driver is excluded from both driver buckets.
	PROC_CollideHitboxWithBucket(GAME_TRACKER->threadBuckets[PLAYER].thread, sps, tw->driverParent->instSelf->thread);
	PROC_CollideHitboxWithBucket(GAME_TRACKER->threadBuckets[ROBOT].thread, sps, tw->driverParent->instSelf->thread);
	PROC_CollideHitboxWithBucket(GAME_TRACKER->threadBuckets[MINE].thread, sps, NULL);
	PROC_CollideHitboxWithBucket(GAME_TRACKER->threadBuckets[TRACKING].thread, sps, NULL);
	sps->Union.ThBuckColl.funcCallback = RB_Burst_CollLevInst;
	PROC_StartSearch_Self(sps);
}

void RB_Burst_DrawAll(struct GameTracker *gGT)
{
	s32 selectedFrame[4];
	struct Thread *selectedThread[4];
	s32 playerIndex;
	struct Thread *thread;
	struct Burst *burst;

	for (playerIndex = 0; playerIndex < gGT->numPlyrCurrGame; playerIndex++)
	{
		MATRIX *view = &gGT->pushBuffer[playerIndex].matrix_ViewProj;
		s32 distanceToScreen;

		thread = gGT->threadBuckets[BURST].thread;
		selectedFrame[playerIndex] = 0x10000;
		selectedThread[playerIndex] = NULL;
		distanceToScreen = (u32)gGT->pushBuffer[playerIndex].distanceToScreen_PREV << 1;

		SetRotMatrix(view);
		SetTransMatrix(view);

		for (; thread != NULL; thread = thread->siblingThread)
		{
			struct Instance *burstInst;
			SVec4 pos;
			VECTOR transformed;
			VECTOR distance;

			burst = thread->object;
			burstInst = burst->explosion;

			// NOTE(aalhendi): Retail can survive the one-frame null low-RAM read.
			if (burstInst == NULL)
			{
				continue;
			}

			pos.x = burstInst->matrix.t[0];
			pos.y = burstInst->matrix.t[1];
			pos.z = burstInst->matrix.t[2];

			CTR_GteLoadPositionV0(&pos);
			gte_rt();
			CTR_PSX_STORE_COP2_WORD(&transformed.vx, 25);
			CTR_PSX_STORE_COP2_WORD(&transformed.vy, 26);
			CTR_PSX_STORE_COP2_WORD(&transformed.vz, 27);

			distance.vx = abs(transformed.vx);
			distance.vy = abs(transformed.vy);
			distance.vz = abs(transformed.vz);

			if ((distance.vx < 0x100) && (distance.vy < 0x100) && (distance.vz < distanceToScreen))
			{
				if (burstInst->animFrame < selectedFrame[playerIndex])
				{
					selectedFrame[playerIndex] = burstInst->animFrame;
					selectedThread[playerIndex] = thread;
				}

				if (burst->explosion->animFrame == 1)
				{
					s16 flash = 0x1fff - ((distance.vx + distance.vy) << 3);
					gGT->pushBuffer[playerIndex].fadeFromBlack_desiredResult = 0x1000;
					gGT->pushBuffer[playerIndex].fade_step = -0x88;
					gGT->pushBuffer[playerIndex].fadeFromBlack_currentValue = flash;
				}
			}
		}
	}

	for (playerIndex = 0; playerIndex < gGT->numPlyrCurrGame; playerIndex++)
	{
		for (thread = gGT->threadBuckets[BURST].thread; thread != NULL; thread = thread->siblingThread)
		{
			burst = thread->object;
			if ((selectedThread[playerIndex] != NULL) && (selectedThread[playerIndex] != thread))
			{
				if (burst->explosion != NULL)
				{
					burst->explosion->idpp[playerIndex].pushBuffer = NULL;
				}
				if (burst->rotatedExplosion != NULL)
				{
					burst->rotatedExplosion->idpp[playerIndex].pushBuffer = NULL;
				}
				if (burst->shockwave != NULL)
				{
					burst->shockwave->idpp[playerIndex].pushBuffer = NULL;
				}
			}
			else
			{
				if (burst->explosion != NULL)
				{
					burst->explosion->idpp[playerIndex].pushBuffer = &gGT->pushBuffer[playerIndex];
				}
				if (burst->rotatedExplosion != NULL)
				{
					burst->rotatedExplosion->idpp[playerIndex].pushBuffer = &gGT->pushBuffer[playerIndex];
				}
				if (burst->shockwave != NULL)
				{
					burst->shockwave->idpp[playerIndex].pushBuffer = &gGT->pushBuffer[playerIndex];
				}
			}
		}
	}
}
