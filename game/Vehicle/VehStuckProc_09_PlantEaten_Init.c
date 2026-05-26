#include <common.h>

extern void *PlayerEatenFuncTable[13];

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

	// drop bits for airborne and kart-on-ground
	d->actionsFlagSet &= ~(0x80000 | 0x40);

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
	t->flags |= 0x1000;

	// make invisible
	inst->flags |= HIDE_MODEL;

	OtherFX_Stop1((int)d->driverAudioPtrs[1]);
	d->driverAudioPtrs[1] = NULL;
	OtherFX_Stop1((int)d->driverAudioPtrs[2]);
	d->driverAudioPtrs[2] = NULL;
	OtherFX_Stop1((int)d->driverAudioPtrs[0]);
	d->driverAudioPtrs[0] = NULL;

	for (i = 0; i < 13; i++)
		d->funcPtrs[i] = PlayerEatenFuncTable[i];
}

void *PlayerEatenFuncTable[13] = {
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
