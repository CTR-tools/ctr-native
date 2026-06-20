#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac714-0x800ac840
void CS_DestroyPodium_StartDriving(void)
{
	struct Instance *inst;
	struct Driver *d;
	struct GameTracker *gGT = sdata->gGT;
	struct Thread *t = gGT->threadBuckets[OTHER].thread;

	// enable HUD
	gGT->hudFlags |= 1;

	// loop through all threads
	while (t != NULL)
	{
		if (t->funcThDestroy != CS_Podium_Prize_ThDestroy)
			t->flags |= THREAD_FLAG_DEAD;

		t = t->siblingThread;
	}

	d = gGT->drivers[0];

	// enable collisions for thread,
	// and make instance visible
	inst = d->instSelf;
	inst->thread->flags &= ~THREAD_FLAG_DISABLE_COLLISION;
	inst->flags &= ~(HIDE_MODEL);

	d->kartState = KS_ENGINE_REVVING;
	d->funcPtrs[DRIVER_FUNC_INIT] = VehPhysProc_Driving_Init;

	// if cutscene changed audio, restore backup
	if (D233.CutsceneManipulatesAudio != 0)
	{
		// restore backup of volume variables
		howl_VolumeSet(0, D233.FXVolumeBackup);
		howl_VolumeSet(1, D233.MusicVolumeBackup);
		howl_VolumeSet(2, D233.VoiceVolumeBackup);
	}

	// cam mode be zero to follow you
	gGT->cameraDC[0].cameraMode = 0;
	gGT->pushBuffer[0].distanceToScreen_PREV = 0x100;
	gGT->pushBuffer[0].distanceToScreen_CURR = 0x100;
}
