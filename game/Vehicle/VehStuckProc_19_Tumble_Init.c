#include <common.h>

void *PlayerBlastedFuncTable[0xD] = {(void *)0x0,
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
	int i;
	int iVar2;
	char bVar3;
	char simpTurnState;

	driver->kartState = KS_BLASTED;
	driver->turbo_MeterRoomLeft = 0;

	if ((LOAD_IsOpen_RacingOrBattle() != 0) && ((sdata->gGT->gameMode1 & ADVENTURE_ARENA) == 0))
	{
		RB_Player_ModifyWumpa(driver, -3);
	}

	driver->instSelf->animIndex = 0;

	iVar2 = VehFrameInst_GetNumAnimFrames(driver->instSelf, 0);
	iVar2 = VehFrameInst_GetStartFrame(0, iVar2);

	driver->instSelf->animFrame = (s16)iVar2;

	iVar2 = MixRNG_Scramble();
	driver->KartStates.Blasted.boolPlayBackwards = iVar2 & 4;

	simpTurnState = driver->simpTurnState;
	if (simpTurnState < 1)
	{
		bVar3 = 0x19;
	}
	else
	{
		bVar3 = 0x29;
	}

	for (i = 0; i < 0xD; i++)
	{
		driver->funcPtrs[i] = PlayerBlastedFuncTable[i];
	}

	GAMEPAD_JogCon1(driver, bVar3, 0x60);
}
