#include <common.h>
#include "RB_Effect.h"

struct Blowup
{
	struct Instance *shockwave;
	struct Instance *explosion;
	u32 reserved; // Retail allocates a third word; these behaviors leave it untouched.
};

void RB_Blowup_ProcessBucket(struct Thread *thread)
{
	for (; thread != NULL; thread = thread->siblingThread)
	{
		struct Blowup *blowup = thread->object;

		{
			s32 i;
			// The explosion inherits the shockwave's per-view visibility and depth.
			for (i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
			{
				if (blowup->shockwave == NULL || blowup->explosion == NULL)
				{
					continue;
				}

				blowup->explosion->idpp[i].instFlags &= blowup->shockwave->idpp[i].instFlags | ~DRAW_SUCCESSFUL;
				blowup->explosion->idpp[i].otRangeNormal = blowup->shockwave->idpp[i].otRangeNormal;
				blowup->explosion->idpp[i].depthOffset[0] = blowup->shockwave->idpp[i].depthOffset[0];
				blowup->explosion->idpp[i].depthOffset[1] = blowup->shockwave->idpp[i].depthOffset[1];
			}
		}
	}
}

void RB_Blowup_ThTick(struct Thread *t)
{
	struct Blowup *blowup;
	blowup = t->object;

	for (;;)
	{
		RB_Effect_UpdateSlot(&blowup->explosion);
		RB_Effect_UpdateSlot(&blowup->shockwave);

		if ((blowup->explosion == NULL) && (blowup->shockwave == NULL))
		{
			t->flags |= THREAD_FLAG_DEAD;
		}
		ThTick_FastRET(t);
		// NOTE(aalhendi): The native scheduler resumes this callback on the next tick.
		return;
	}
}

// Spawn the explosion/shockwave pair, then apply its damage independently.
void RB_Blowup_Init(struct Instance *weaponInst)
{
	s32 modelID;
	struct ScratchpadStruct *sps;
	struct Thread *explosionTh;
	struct Instance *effectInst;
	struct ModelHeader *headers;
	struct Blowup *blowup;
	struct InstanceBirthParams birth;

	birth.modelID = STATIC_CRATE_EXPLOSION;
	birth.name = rb_nameBlowup;
	birth.poolType = SMALL;
	birth.bucket = BLOWUP;
	birth.funcThTick = RB_Blowup_ThTick;
	birth.objSize = sizeof(struct Blowup);
	birth.parent = NULL;
	effectInst = INSTANCE_BirthWithThread_Stack(&birth);

	// NOTE(aalhendi): Retail assumes the thread and instance pools have capacity. Native
	// preserves the explosion damage when either optional visual cannot spawn.
	if (effectInst == NULL)
	{
		goto ApplyDamage;
	}

	effectInst->flags |= (VISIBLE_DURING_GAMEPLAY | DRAW_BILLBOARD);

	explosionTh = effectInst->thread;
	Chaos_InheritThreadOwner(explosionTh, weaponInst->thread);
	blowup = explosionTh->object;

	blowup->explosion = effectInst;

	// copy position and rotation from weapon to explosion
	effectInst->matrix = weaponInst->matrix;

	// TNT uses the red effect; Nitro uses green.
	if (weaponInst->model->id == STATIC_CRATE_TNT)
	{
		// red
		effectInst->colorRGBA = 0xad10000;
	}
	else
	{
		effectInst->colorRGBA = 0x1eac000;
	}

	effectInst->alphaScale = 0x1000;

	// The shockwave shares the explosion's owner thread.

	modelID = weaponInst->model->id == STATIC_CRATE_TNT ? STATIC_SHOCKWAVE_RED : STATIC_SHOCKWAVE_GREEN;
	effectInst = INSTANCE_Birth3D(GAME_TRACKER->modelPtr[modelID], rb_nameShockwave, explosionTh);

	blowup->shockwave = effectInst;

	if (effectInst == NULL)
	{
		goto ApplyDamage;
	}

	effectInst->flags |= PIXEL_LOD;

	effectInst->matrix.t[0] = weaponInst->matrix.t[0];
	effectInst->matrix.t[1] = weaponInst->matrix.t[1];
	effectInst->matrix.t[2] = weaponInst->matrix.t[2];

	// Both shockwave model headers billboard toward each camera.
	headers = effectInst->model->headers;
	headers[0].flags |= 2;
	headers = effectInst->model->headers;
	headers[1].flags |= 2;
	CTR_MatrixSetRotIdentity(&effectInst->matrix);

ApplyDamage:;
	sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);

	// put weapon position on scratchpad
	sps->Input1.pos.x = weaponInst->matrix.t[0];
	sps->Input1.pos.y = weaponInst->matrix.t[1];
	sps->Input1.pos.z = weaponInst->matrix.t[2];

	if (!IS_BOSS_RACE(GAME_TRACKER->gameMode1))
	{
		sps->Input1.hitRadius = 0x140;
		sps->Input1.hitRadiusSquared = 0x19000;
	}

	else
	{
		sps->Input1.hitRadius = 0x100;
		sps->Input1.hitRadiusSquared = 0x10000;
	}

	sps->Union.ThBuckColl.funcCallback = RB_Burst_CollThBucket;
	sps->Union.ThBuckColl.thread = weaponInst->thread;
	sps->Input1.modelID = weaponInst->model->id;

	PROC_StartSearch_Self(sps);

	PROC_CollideHitboxWithBucket(GAME_TRACKER->threadBuckets[ROBOT].thread, sps, 0);
	PROC_CollideHitboxWithBucket(GAME_TRACKER->threadBuckets[MINE].thread, sps, 0);

        if (weaponInst->model->id != STATIC_CRATE_TNT)
        {
          /* Nitro has a smaller player-only base radius. */
          sps->Input1.hitRadius = 0x80;
        }

        sps->Input1.hitRadius = sps->Input1.hitRadius * g_config.tntExplosionRadiusMultiplier / 100;
        sps->Input1.hitRadiusSquared = sps->Input1.hitRadius * sps->Input1.hitRadius;

        PROC_CollideHitboxWithBucket(GAME_TRACKER->threadBuckets[PLAYER].thread, sps, 0);

	sps->Union.ThBuckColl.funcCallback = RB_Burst_CollLevInst;
	return;
}
