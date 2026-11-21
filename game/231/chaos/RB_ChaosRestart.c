#include <common.h>
#include <game/DotLights.h>
#include <game/Vehicle/VehPickupItem.h>
#include "RB_ChaosRestart.h"

void Chaos_ResetSteeringPose(struct Driver *driver)
{
	driver->turnAngleCurr = 0;
	driver->turnAnglePrev = 0;
	driver->turnAngleLerpTarget = 0;
	driver->turnAngleLerpVel = 0;
	driver->turnWobbleTimer = 0;
	driver->turnWobbleAngle = 0;
	driver->turnWobbleVelocity = 0;
}

static void Chaos_NeutralRestartPose(struct Driver *driver)
{
	driver->matrixArray = BAKED_GTE_MATRIX_NONE;
	driver->matrixIndex = 0;
	driver->simpTurnState = 0;
	driver->ampTurnState = 0;
	driver->rotationSpinRate = 0;
	Chaos_ResetSteeringPose(driver);
	// Driving reconstructs visual yaw from angle plus drift/wobble offsets.
	driver->angle = ANG_MODULO_TWO_PI(driver->rotCurr.y);
	driver->rotCurr.x = 0;
	driver->rotCurr.z = 0;
	driver->rotCurr.w = 0;
	driver->rotPrev = driver->rotCurr;
	driver->instSelf->animIndex = 0;
	driver->instSelf->animFrame = VehFrameInst_GetStartFrame(0, VehFrameInst_GetNumAnimFrames(driver->instSelf, 0));
}

void Chaos_EndRestartHold(struct ChaosRestart *restart, struct Driver *driver)
{
	if (restart->mask != NULL)
	{
		restart->mask->duration = 0;
		restart->mask = NULL;
	}
	if (restart->floating && driver->kartState == KS_ENGINE_REVVING)
	{
		Chaos_NeutralRestartPose(driver);
	}
	restart->floating = false;
}

static void Chaos_UpdateRestartMask(struct ChaosRestart *restart, const struct Driver *driver)
{
	if (restart->mask != NULL)
	{
		restart->mask->duration = MASK_HEAD_DURATION_NORMAL;
		restart->mask->pos = (SVec3){driver->posCurr.x >> 8, driver->posCurr.y >> 8, driver->posCurr.z >> 8};
	}
}

bool Chaos_IsBeingRescued(const struct Driver *driver)
{
	return driver->kartState == KS_MASK_GRABBED ||
	    (driver->kartState == KS_ENGINE_REVVING && driver->KartStates.RevEngine.boolMaskGrab);
}

void Chaos_RestartDriver(struct Driver *driver, struct ChaosRestart *restart)
{
	restart->active = true;
	restart->pending = false;
	restart->timer = DOT_LIGHT_TIMER_HIDE;
	restart->lightsPrevious = DOT_LIGHT_TIMER_HIDE;
	restart->cameraBlendPending = true;
	restart->floating = (driver->actionsFlagSet & ACTION_AIRBORNE) != 0;
	driver->posPrev = driver->posCurr;
	driver->velocity = (Vec3){0};
	driver->speed = 0;
	driver->speedApprox = 0;
	driver->baseSpeed = 0;
	driver->fireSpeed = 0;
	driver->ySpeed = 0;
	driver->reserves = 0;
	driver->turbo_outsideTimer = 0;
	driver->turbo_MeterRoomLeft = 0;
	driver->jump_TenBuffer = 0;
	driver->jump_ForcedMS = 0;
	driver->jump_InitialVelY = 0;
	driver->jump_HighJumpTimerMS = 0;
	driver->jump_LandingBoost = 0;
	driver->NoInputTimer = 0;
	Chaos_NeutralRestartPose(driver);
	driver->actionsFlagSet &= ~(ACTION_HIGH_JUMP | ACTION_JUMP_STARTED |
	    ACTION_JUMP_BUTTON_HELD | ACTION_BACK_SKID | ACTION_FRONT_SKID | ACTION_WARP);
	VehStuckProc_RevEngine_Init(driver->instSelf->thread, driver);
	if (restart->floating)
	{
		VehStuckProc_MaskGrab_SetFallPose(driver);
		restart->mask = VehPickupItem_MaskCreateRescue(driver);
		if (restart->mask != NULL)
		{
			restart->mask->rot.z |= MASK_HEAD_ROT_WORLD_SPACE;
			Chaos_UpdateRestartMask(restart, driver);
		}
	}
}

static void Chaos_TickRestart(struct ChaosRestart *restart, struct Driver *driver, s32 elapsedMS)
{
	restart->timer -= elapsedMS;
	Chaos_UpdateRestartMask(restart, driver);
	if (restart->timer <= 0 || (driver->actionsFlagSet & ACTION_RACE_FINISHED))
	{
		Chaos_EndRestartHold(restart, driver);
	}
	if (restart->timer < DOT_LIGHT_TIMER_SHOW_MIN || (driver->actionsFlagSet & ACTION_RACE_FINISHED))
	{
		restart->active = false;
	}
}

int Chaos_StartTimerForDriver(const struct Driver *driver)
{
	const struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	if (driver->driverID < (int)len(runtime->restart) && runtime->restart[driver->driverID].active)
	{
		return runtime->restart[driver->driverID].timer;
	}
	return GAME_TRACKER->trafficLightsTimer;
}

bool Chaos_IsWaitingForStart(const struct Driver *driver)
{
	return driver->driverID < (int)len(sdata->chaosRuntime.restart) &&
	    sdata->chaosRuntime.restart[driver->driverID].active &&
	    sdata->chaosRuntime.restart[driver->driverID].timer > 0;
}

bool Chaos_ConsumeCameraBlend(struct CameraDC *camera)
{
	const struct Driver *driver = camera->driverToFollow;
	if (driver == NULL || driver->driverID >= (int)len(sdata->chaosRuntime.restart) || camera->cameraMode != 0 ||
	    (camera->flags & CAMERA_FLAG_ARCADE_END_OF_RACE_REQUESTED))
	{
		return false;
	}
	struct ChaosRestart *restart = &sdata->chaosRuntime.restart[driver->driverID];
	bool pending = sdata->chaosRuntime.cameraBlendPending[driver->driverID];
	sdata->chaosRuntime.cameraBlendPending[driver->driverID] = false;
	if (Chaos_IsWaitingForStart(driver))
	{
		pending |= restart->cameraBlendPending;
		restart->cameraBlendPending = false;
	}
	return pending;
}

void Chaos_ApplyStartPose(struct Driver *driver)
{
	if (Chaos_IsWaitingForStart(driver) && sdata->chaosRuntime.restart[driver->driverID].floating)
	{
		VehStuckProc_MaskGrab_SetFallPose(driver);
	}
}

void Chaos_UpdateRestarts(s32 elapsedMS)
{
	for (int i = 0; i < (int)len(sdata->chaosRuntime.restart); i++)
	{
		struct ChaosRestart *restart = &sdata->chaosRuntime.restart[i];
		struct Driver *driver = sdata->gGT->drivers[i];
		if (driver == NULL)
		{
			continue;
		}
		if ((driver->actionsFlagSet & ACTION_RACE_FINISHED) && restart->pending)
		{
			restart->pending = false;
		}
		if (restart->pending && !Chaos_IsBeingRescued(driver))
		{
			Chaos_RestartDriver(driver, restart);
			continue;
		}
		if (!restart->active)
		{
			continue;
		}
		Chaos_TickRestart(restart, driver, elapsedMS);
	}
}

void Chaos_DrawStartLights(struct GameTracker *gGT)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	u32 drawn = 0;
	for (int i = 0; i < (int)len(runtime->restart); i++)
	{
		struct ChaosRestart *restart = &runtime->restart[i];
		if (!restart->active || (drawn & (1u << i)))
		{
			continue;
		}
		u32 players = 0;
		const s32 previous = restart->lightsPrevious;
		for (int j = i; j < (int)len(runtime->restart); j++)
		{
			struct ChaosRestart *other = &runtime->restart[j];
			if (other->active && other->timer == restart->timer && other->lightsPrevious == previous)
			{
				players |= 1u << j;
				other->lightsPrevious = other->timer;
			}
		}
		DotLights_Sequence(gGT, restart->timer, previous, players);
		drawn |= players;
	}
}

