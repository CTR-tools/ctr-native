#include <common.h>

void COLL_FIXED_PlayerSearch();

void *PlayerSpinningFuncTable[0xD] = {DECOMP_VehPhysProc_SpinFirst_InitSetUpdate,
                                      0,
                                      DECOMP_VehPhysProc_SpinFirst_PhysLinear,
                                      DECOMP_VehPhysProc_Driving_Audio,
                                      DECOMP_VehPhysProc_SpinFirst_PhysAngular,
                                      DECOMP_VehPhysForce_OnApplyForces,

#ifndef REBUILD_PS1
                                      COLL_MOVED_PlayerSearch,
                                      VehPhysForce_CollideDrivers,
                                      COLL_FIXED_PlayerSearch,
                                      VehPhysGeneral_JumpAndFriction,
                                      VehPhysForce_TranslateMatrix,
                                      VehFrameProc_Spinning,

                                      VehEmitter_DriverMain

#else
// TODO(aalhendi): Port driver collision, jump/friction,
// matrix translation, spin frame, and emitter stages.
#ifdef CTR_NATIVE
                                      COLL_MOVED_PlayerSearch,
#else
                                      NULL,
#endif
                                      NULL,
                                      COLL_FIXED_PlayerSearch,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL
#endif
};

void DECOMP_VehPhysProc_SpinFirst_Init(struct Thread *t, struct Driver *d)
{
	int i;

	d->kartState = KS_SPINNING;

	d->unk_LerpToForwards = 0;
	d->turbo_MeterRoomLeft = 0;

	if (DECOMP_LOAD_IsOpen_RacingOrBattle())
	{
		DECOMP_RB_Player_ModifyWumpa(d, -1);
	}

#ifndef REBUILD_PS1
	Voiceline_RequestPlay(3, data.characterIDs[d->driverID], 0x10);
#endif

	// if spinning left
	d->KartStates.Spinning.spinDir = 1;
	d->KartStates.Spinning.driftSpinRate = 300;

	if (d->ampTurnState < 0)
	{
		// if spinning right
		d->KartStates.Spinning.spinDir = -1;
		d->KartStates.Spinning.driftSpinRate = -300;
	}

	if (d->simpTurnState < 1)
	{
		i = 0x19;
	}

	else
	{
		i = 0x29;
	}

	for (i = 0; i < 0xD; i++)
	{
		d->funcPtrs[i] = PlayerSpinningFuncTable[i];
	}

#ifndef REBUILD_PS1
	GAMEPAD_JogCon1(d, i, 0x60);
#endif
}
