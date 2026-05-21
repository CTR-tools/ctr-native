#include <common.h>

extern void *PlayerMaskGrabFuncTable[13];

void DECOMP_VehStuckProc_MaskGrab_Init(struct Thread *t, struct Driver *d)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Instance *inst = t->inst;

	d->kartState = KS_MASK_GRABBED;

	d->KartStates.MaskGrab.animFrame = 0;

	*(int *)&d->KartStates.MaskGrab.boolParticlesSpawned = 0;

	d->KartStates.MaskGrab.maskObj = DECOMP_VehPickupItem_MaskUseWeapon(d, 1);

	d->matrixArray = 0;
	d->matrixIndex = 0;

	d->turbo_MeterRoomLeft = 0;
	d->turbo_outsideTimer = 0;
	d->reserves = 0;

	d->NoInputTimer = 1440;

	d->actionsFlagSet &= 0xfff7ffbf;

	if ((DECOMP_LOAD_IsOpen_RacingOrBattle() != 0) && ((gGT->gameMode1 & ADVENTURE_ARENA) == 0))
	{
		DECOMP_RB_Player_ModifyWumpa(d, -2);
	}

	if (d->quadBlockHeight + 0x8000 < d->posCurr.y)
	{
		d->numTimesMaskGrab++;

		if ((d->posCurr.y < -0x8000) && ((gGT->level1->configFlags & 2) != 0))
		{
			d->KartStates.MaskGrab.AngleAxis_NormalVec[0] = d->AxisAngle2_normalVec[0];
			d->KartStates.MaskGrab.AngleAxis_NormalVec[1] = d->AxisAngle2_normalVec[1];
			d->KartStates.MaskGrab.AngleAxis_NormalVec[2] = d->AxisAngle2_normalVec[2];

			for (int i = 10; i > 0; i--)
			{
				struct Particle *p = Particle_Init(0, gGT->iconGroup[9], &data.emSet_Falling[0]);
				if (p == NULL)
					break;

				p->unk18 = d->instSelf->unk50;
				p->driverInst = d->instSelf;
				p->unk19 = d->driverID;
			}
		}
		else
		{
			d->KartStates.MaskGrab.boolStillFalling = true;
		}
	}
	else
	{
		d->KartStates.MaskGrab.AngleAxis_NormalVec[0] = d->AxisAngle2_normalVec[0];
		d->KartStates.MaskGrab.AngleAxis_NormalVec[1] = d->AxisAngle2_normalVec[1];
		d->KartStates.MaskGrab.AngleAxis_NormalVec[2] = d->AxisAngle2_normalVec[2];
	}

	d->posCurr.x = inst->matrix.t[0] << 8;
	d->posCurr.y = inst->matrix.t[1] << 8;
	d->posCurr.z = inst->matrix.t[2] << 8;

	d->posPrev.x = d->posCurr.x;
	d->posPrev.y = d->posCurr.y;
	d->posPrev.z = d->posCurr.z;

	for (int i = 0; i < 13; i++)
	{
		d->funcPtrs[i] = PlayerMaskGrabFuncTable[i];
	}

	struct MaskHeadWeapon *mask = d->KartStates.MaskGrab.maskObj;
	if (mask == NULL)
		return;

	mask->rot[2] |= 1;

	mask->pos[0] = d->posCurr.x >> 8;
	mask->pos[1] = (d->posCurr.y >> 8) + 0x140;
	mask->pos[2] = d->posCurr.z >> 8;
}

void DECOMP_VehStuckProc_MaskGrab_Update(struct Thread *t, struct Driver *d);
void DECOMP_VehStuckProc_MaskGrab_PhysLinear(struct Thread *t, struct Driver *d);
void DECOMP_VehStuckProc_MaskGrab_Animate(struct Thread *t, struct Driver *d);

void *PlayerMaskGrabFuncTable[13] = {NULL,
                                     DECOMP_VehStuckProc_MaskGrab_Update,
                                     DECOMP_VehStuckProc_MaskGrab_PhysLinear,
                                     DECOMP_VehPhysProc_Driving_Audio,
                                     DECOMP_VehPhysGeneral_PhysAngular,
                                     DECOMP_VehPhysForce_OnApplyForces,

#ifndef REBUILD_PS1
                                     COLL_MOVED_PlayerSearch,
                                     VehPhysForce_CollideDrivers,
                                     COLL_FIXED_PlayerSearch,
                                     VehPhysGeneral_JumpAndFriction,
                                     VehPhysForce_TranslateMatrix,
#else
// TODO(aalhendi): Port driver collision,
// jump/friction, and matrix translation stages.
#ifdef CTR_NATIVE
                                     COLL_MOVED_PlayerSearch,
#else
                                     NULL,
#endif
                                     NULL, COLL_FIXED_PlayerSearch, NULL, NULL,
#endif
                                     DECOMP_VehStuckProc_MaskGrab_Animate,
#ifndef REBUILD_PS1
                                     VehEmitter_DriverMain
#else
                                     // TODO(aalhendi): Port driver emitter stage.
                                     NULL
#endif
};
