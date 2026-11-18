#include "VehCommon.h"
#include <game/BOTS.h>

void VehPhysProc_TakeControl(struct Driver *driver)
{
	struct Thread *thread = driver->instSelf->thread;
	const int damage = driver->botData.aiDamageState;
	const bool damaged = (driver->botData.botFlags & BOT_FLAG_DAMAGE_ACTIVE) != 0;
	driver->actionsFlagSet &= ~ACTION_BOT;
	thread->modelIndex = DYNAMIC_PLAYER;
	memset(driver->funcPtrs, 0, sizeof(driver->funcPtrs));

	// Bots advance their checkpoint index, not the human anti-cut baseline or
	// lastValid surface. Let the first human collision seed the current surface.
	driver->lastValid = NULL;
	driver->underDriver = NULL;
	driver->currBlockTouching = NULL;
	driver->distanceToFinish_checkpoint = driver->distanceToFinish_curr;
	driver->checkpoint.currentIndex = driver->botData.ai_quadblock_checkpointIndex;
	driver->collisionFlags = 0;
	driver->stepFlagSet = 0;
	driver->posPrev = driver->posCurr;
	// AI camera lag uses a wrapped angle; human recovery integrates a signed
	// offset and its own velocity. Neither carries over from the AI camera.
	driver->rotCurr.w = 0;
	driver->rotPrev.w = 0;

	// The bot reports world velocity separately from the human polar speed.
	driver->velocity = (Vec3){driver->xSpeed, driver->ySpeed, driver->zSpeed};
	VehPhysCrash_ConvertVecToSpeed(driver, &driver->velocity);

	// Ownership changes immediately; an existing bot rescue keeps its mask and
	// descent callback until landing, then installs ordinary human driving.
	if (driver->kartState == KS_MASK_GRABBED && thread->funcThTick == BOTS_ThTick_RevEngine)
	{
		return;
	}
	thread->funcThTick = NULL;
	VehPhysProc_Driving_Init(thread, driver);
	if (damaged && damage == BOTS_DAMAGE_STATE_SPIN)
	{
		driver->kartState = KS_SPINNING;
		driver->KartStates.Spinning.spinDir = 1;
		driver->KartStates.Spinning.driftSpinRate = VEH_PHYS_PROC_SPIN_INITIAL_RATE;
		driver->NoInputTimer = driver->botData.aiPhysics.squishCooldown;
		VehPhysProc_SpinFirst_InitSetUpdate(thread, driver);
	}
	else if (damaged && damage == BOTS_DAMAGE_STATE_BLAST)
	{
		driver->kartState = KS_BLASTED;
		driver->KartStates.Blasted.boolPlayBackwards = false;
		VehStuckProc_Tumble_SetFunctions(driver);
	}
}
