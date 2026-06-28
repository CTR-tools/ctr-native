#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80069284-0x80069370.
void VehTurbo_ProcessBucket(struct Thread *turboThread)
{
	while (turboThread != NULL)
	{
		struct Instance *primaryInst = turboThread->inst;
		struct Turbo *turbo = (struct Turbo *)turboThread->object;
		struct Instance *secondaryInst = turbo->inst;
		struct Instance *driverInst = turbo->driver->instSelf;

		struct InstDrawPerPlayer *primary = INST_GETIDPP(primaryInst);
		struct InstDrawPerPlayer *secondary = INST_GETIDPP(secondaryInst);
		struct InstDrawPerPlayer *driver = INST_GETIDPP(driverInst);

		for (int i = 0; i < sdata->gGT->numPlyrCurrGame; i++)
		{
			if ((driver->instFlags & PUSHBUFFER_EXISTS) == 0)
			{
				u32 driverDrawFlag = driver->instFlags | ~DRAW_SUCCESSFUL;

				secondary->instFlags &= driverDrawFlag;
				primary->instFlags &= driverDrawFlag;

				secondary->otRangeNormal = driver->otRangeNormal;
				primary->otRangeNormal = driver->otRangeNormal;
				secondary->otRangeSecondary = driver->otRangeSecondary;
				primary->otRangeSecondary = driver->otRangeSecondary;

				secondary->depthOffset[0] = driver->depthOffset[0];
				primary->depthOffset[0] = driver->depthOffset[0];
				secondary->depthOffset[1] = driver->depthOffset[1];
				primary->depthOffset[1] = driver->depthOffset[1];
			}

			primary++;
			secondary++;
			driver++;
		}

		turboThread = turboThread->siblingThread;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80069370-0x800693c8.
void VehTurbo_ThDestroy(struct Thread *t)
{
	struct Turbo *turboObj;
	turboObj = t->object;

	struct Driver *d = turboObj->driver;
	d->actionsFlagSet &= ~ACTION_TURBO_ITEM;

	INSTANCE_Death(t->inst);
	INSTANCE_Death(turboObj->inst);
}

static void VehTurbo_TransformOffset(struct Instance *driverInst, s16 x, s16 y, s16 z, s32 *out)
{
	SVECTOR offset = {x, y, z, 0};

	// NOTE(aalhendi): Native expression of retail VXY0/VZ0 loads before gte_rt.
	gte_SetRotMatrix(&driverInst->matrix.m[0][0]);
	gte_SetTransMatrix(&driverInst->matrix.m[0][0]);
	CTR_GteLoadSV0(&offset);
	gte_rt();
	CTR_GteStoreIR(out);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800693c8-0x80069bb0.
void VehTurbo_ThTick(struct Thread *turboThread)
{
	struct GameTracker *gGT = sdata->gGT;

	struct Turbo *turbo = (struct Turbo *)turboThread->object;
	struct Driver *driver = turbo->driver;
	struct Instance *instance = turboThread->inst;
	struct Instance *instanceDriver = driver->instSelf;

	if ((
	        // if not burnt
	        (driver->burnTimer == 0) &&

	        // if alpha of turbo is zero
	        (instance->alphaScale == 0)) &&


	    (instanceDriver->thread->modelIndex != DYNAMIC_GHOST))
	{
		// cut driverInst transparency in half
		instanceDriver->alphaScale = instanceDriver->alphaScale >> 1;
	}

	// if instance is not split by water
	if ((instanceDriver->flags & SPLIT_LINE) == 0)
	{
		// instance flags
		instance->flags &= ~SPLIT_LINE;
		turbo->inst->flags &= ~SPLIT_LINE;
	}

	// if instance is split by water
	else
	{
		// turbos are now split by water, set vertical split height
		instance->flags |= SPLIT_LINE;
		instance->vertSplit = instanceDriver->vertSplit;
		turbo->inst->flags |= SPLIT_LINE;
		turbo->inst->vertSplit = instanceDriver->vertSplit;
	}

	// if driver instance is not reflective
	if ((instanceDriver->flags & REFLECTIVE) == 0)
	{
		// remove reflection from turbo instances
		instance->flags &= ~REFLECTIVE;
		turbo->inst->flags &= ~REFLECTIVE;
	}

	// if driver instance is reflective
	else
	{
		// make turbo instances reflective
		// copy reflection height axis to instance
		instance->flags |= REFLECTIVE;
		instance->vertSplit = instanceDriver->vertSplit;
		turbo->inst->flags |= REFLECTIVE;
		turbo->inst->vertSplit = instanceDriver->vertSplit;
	}

	int fireSize = (int)turbo->fireSize;
	if (8 < (int)turbo->fireSize)
	{
		fireSize = 8;
	}
	if ((int)turbo->fireSize < 4)
	{
		fireSize = 4;
	}

	// matrix of first turbo instance
	instance->matrix.m[0][0] = (s16)(instanceDriver->matrix.m[0][0] * fireSize >> 3);
	instance->matrix.m[0][1] = (s16)(instanceDriver->matrix.m[0][1] * fireSize >> 3);
	instance->matrix.m[0][2] = (s16)(instanceDriver->matrix.m[0][2] * fireSize >> 3);
	instance->matrix.m[1][0] = (s16)(instanceDriver->matrix.m[1][0] * fireSize >> 3);
	instance->matrix.m[1][1] = (s16)(instanceDriver->matrix.m[1][1] * fireSize >> 3);
	instance->matrix.m[1][2] = (s16)(instanceDriver->matrix.m[1][2] * fireSize >> 3);
	instance->matrix.m[2][0] = (s16)(instanceDriver->matrix.m[2][0] * fireSize >> 3);
	instance->matrix.m[2][1] = (s16)(instanceDriver->matrix.m[2][1] * fireSize >> 3);
	instance->matrix.m[2][2] = (s16)(instanceDriver->matrix.m[2][2] * fireSize >> 3);

	VehTurbo_TransformOffset(instanceDriver, instanceDriver->scale.x * 9 >> 0xb, instanceDriver->scale.y * 3 >> 8, instanceDriver->scale.z * -0x34 >> 0xc,
	                         &instance->matrix.t[0]);

	// matrix of second turbo instance, negate X axis
	turbo->inst->matrix.m[0][0] = (s16)(-(int)instanceDriver->matrix.m[0][0] * fireSize >> 3);
	turbo->inst->matrix.m[0][1] = (s16)(instanceDriver->matrix.m[0][1] * fireSize >> 3);
	turbo->inst->matrix.m[0][2] = (s16)(instanceDriver->matrix.m[0][2] * fireSize >> 3);
	turbo->inst->matrix.m[1][0] = (s16)(-(int)instanceDriver->matrix.m[1][0] * fireSize >> 3);
	turbo->inst->matrix.m[1][1] = (s16)(instanceDriver->matrix.m[1][1] * fireSize >> 3);
	turbo->inst->matrix.m[1][2] = (s16)(instanceDriver->matrix.m[1][2] * fireSize >> 3);
	turbo->inst->matrix.m[2][0] = (s16)(-(int)instanceDriver->matrix.m[2][0] * fireSize >> 3);
	turbo->inst->matrix.m[2][1] = (s16)(instanceDriver->matrix.m[2][1] * fireSize >> 3);
	turbo->inst->matrix.m[2][2] = (s16)(instanceDriver->matrix.m[2][2] * fireSize >> 3);

	VehTurbo_TransformOffset(instanceDriver, instanceDriver->scale.x * -0x12 >> 0xc, instanceDriver->scale.y * 3 >> 8, instanceDriver->scale.z * -0x34 >> 0xc,
	                         &turbo->inst->matrix.t[0]);

	// decrease turbo visibility cooldown by elapsed milliseconds per frame, ~32
	s16 elapsedTime = turbo->fireVisibilityCooldown - gGT->elapsedTimeMS;
	turbo->fireVisibilityCooldown = elapsedTime;

	// don't allow negatives
	if (elapsedTime * 0x10000 < 0)
	{
		turbo->fireVisibilityCooldown = 0;
	}

	if (turbo->fireVisibilityCooldown == 0)
	{
		// make fire visible now that there's no cooldown
		instance->flags &= ~HIDE_MODEL;
		turbo->inst->flags &= ~HIDE_MODEL;
	}

	if (instance->alphaScale < 2500)
	{
		// gamepad vibration
		GAMEPAD_ShockFreq(driver, 4, 4);
	}

	// set new model pointer, one of seven
	instance->model = gGT->modelPtr[(int)turbo->fireAnimIndex + STATIC_TURBO_EFFECT];

	// set new model pointer, one of seven

	// STATIC_TURBO_EFFECT
	// STATIC_TURBO_EFFECT1
	// STATIC_TURBO_EFFECT2
	// STATIC_TURBO_EFFECT3
	// STATIC_TURBO_EFFECT4
	// STATIC_TURBO_EFFECT5
	// STATIC_TURBO_EFFECT6
	// STATIC_TURBO_EFFECT7
	turbo->inst->model = gGT->modelPtr[(((int)turbo->fireAnimIndex + 3U) & 7) + STATIC_TURBO_EFFECT];

	turbo->fireAnimIndex++;

	// if higher than 7, back to zero
	turbo->fireAnimIndex &= 7;

	if (turbo->fireDisappearCountdown > 0)
	{
		turbo->fireDisappearCountdown--;
	}

	// player of any kind
	if (instanceDriver->thread->modelIndex == DYNAMIC_PLAYER)
	{
		int fireSfxVolume = 0x100 - (u32)(instance->alphaScale >> 4);

		if (fireSfxVolume < 0)
		{
			fireSfxVolume = 0;
		}
		else
		{
			if (0x82 < fireSfxVolume)
			{
				fireSfxVolume = 0x82;
			}
		}

		u32 fireAudioDistort = (u32)turbo->fireAudioDistort + 0x10;

		if ((int)fireAudioDistort < 0)
		{
			fireAudioDistort = 0;
		}
		else
		{
			if (fireAudioDistort > 0x80)
			{
				fireAudioDistort = 0x80;
			}
		}

		// if echo is required
		u32 echo = ((driver->actionsFlagSet & ACTION_ENGINE_ECHO) != 0);

		// driver audio
		OtherFX_RecycleNew((u32 *)&driver->driverAudioPtrs[3], 0xe, HowlSfx_Pack(HOWL_SFX_LR_CENTER, fireAudioDistort, fireSfxVolume, echo));

		// manipulate turbo audio distort to change sound each frame
		if (turbo->fireAudioDistort < 0xc0)
		{
			turbo->fireAudioDistort++;
		}
	}

	char kartState = driver->kartState;

	if (
	    // if this is a ghost
	    (instanceDriver->thread->modelIndex == DYNAMIC_GHOST) ||

	    ((kartState != KS_MASK_GRABBED) && (kartState != KS_CRASHING)

// lol they found a glitch with this
#if BUILD > SepReview
	     && (kartState != KS_WARP_PAD)
#endif
	         ))
	{
		// if reserves are nearing zero
		if ((driver->reserves < 0x10) || (turbo->fireDisappearCountdown == '\0'))
		{
			// if fully transparent, skip lines
			if (0xfff < instance->alphaScale)
			{
				goto LAB_80069b50;
			}

			if (turbo->fireDisappearCountdown == '\0')
			{
				// increase transparency
				instance->alphaScale += 0x100;
				turbo->inst->alphaScale += 0x100;
			}
			else
			{
				// increase transparency
				instance->alphaScale += 0x40;
				turbo->inst->alphaScale += 0x40;
			}
		}
		else
		{
			// if scale is big, skip lines
			if (0xfff < instance->alphaScale)
			{
				goto LAB_80069b50;
			}
		}
	}

	// if not a ghost, and
	// kart state is mask grab, crashed, or warped
	else
	{
		// restore backup of alpha
		instanceDriver->alphaScale = driver->alphaScaleBackup;
	LAB_80069b50:

		// player of any kind
		if (instanceDriver->thread->modelIndex == DYNAMIC_PLAYER)
		{
			// volume, distortion, left/right
			u32 stopSfxParams = HOWL_SFX_CENTER_NO_DISTORTION;

			// if echo is required
			if ((driver->actionsFlagSet & ACTION_ENGINE_ECHO) != 0)
			{
				// add echo, volume, distortion, left/right
				stopSfxParams = HOWL_SFX_CENTER_NO_DISTORTION | HOWL_SFX_ECHO_FLAG;
			}

			// driver audio
			OtherFX_RecycleNew((u32 *)&driver->driverAudioPtrs[3], 0xffffffff, stopSfxParams);
		}

		// 0x800 = this thread needs to be deleted
		turboThread->flags |= THREAD_FLAG_DEAD;
	}

	// do not use infinite loop optimization,
	// modern GCC "without" the $RA skip is more
	// optimized than PSYQ "with" the $RA skip
}
