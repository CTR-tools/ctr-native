#include <common.h>
#include <game/DotLights.h>
#include <game/Vehicle/VehPickState.h>
#include <game/Vehicle/VehPhysForce.h>
#include <game/Vehicle/VehPickupItem.h>
#include "../RB_Collision.h"
#include "RB_ChaosAudio.h"
#include "RB_ChaosEffects.h"
#include "RB_ChaosRuntime.h"

// Effect operations only; RB_Chaos.c checks activation and driver eligibility.

enum
{
	CHAOS_PERCENT_SCALE = 100,
	CHAOS_LOW_GRAVITY_DIVISOR = 4,
	CHAOS_HEAVY_GRAVITY_MULTIPLIER = 3,
	CHAOS_SUNDAY_DRIVER_SPEED_PERCENT = 70,
	CHAOS_ARENA_CHAMPIONS_SPEED_PERCENT = 175,
	CHAOS_SUPER_SPEED_MULTIPLIER = 2,
	CHAOS_KART_SCALE_NORMAL = 1,
	CHAOS_GIANT_SCALE = 2,
	CHAOS_SMALL_FORCE_PERCENT = 70,
	CHAOS_GIANT_SPEED_PERCENT = 130,
	CHAOS_THIRD_DRIFT_FIRE_LEVEL = 0x80,
	CHAOS_WUMPA_PICKUP_FRAMES = 5,
	CHAOS_LAUNCH_JUMP_MULTIPLIER = 6,
	CHAOS_WRONG_WAY_CADENCE_MULTIPLIER = 2,
	CHAOS_TRANSFORM_DELAY_MS = 100,
	CHAOS_TRANSFORM_SMOKE_COUNT = 8,
	CHAOS_TRANSFORM_SMOKE_ICON_GROUP = 1,
	CHAOS_TRANSFORM_SMOKE_SCALE = 0x2000,
	CHAOS_TRANSFORM_SMOKE_GROWTH = 0x200,
};

static const float s_chaosSmallScale = 0.5f;

static void Chaos_PlaySenileVoice(struct Driver *driver)
{
	Voiceline_PlayWrongWay(driver);
	sdata->voicelineCooldown /= CHAOS_WRONG_WAY_CADENCE_MULTIPLIER;
}

bool Chaos_OverridesWrongWayWarning(const struct Driver *driver)
{
	return Chaos_IsActiveForDriver(CHAOS_EFFECT_SENILE_MASK, driver);
}

void Chaos_DrawWrongWayWarnings(struct GameTracker *gGT)
{
	for (int i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		const struct Driver *driver = gGT->drivers[i];
		if (Chaos_OverridesWrongWayWarning(driver) && (driver->actionsFlagSet & ACTION_DRIVING_WRONG_WAY) == 0)
		{
			UI_DrawWrongWay(driver);
		}
	}
}

bool Chaos_PlayWrongWayVoice(void)
{
	struct GameTracker *gGT = sdata->gGT;
	for (int i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		struct Driver *driver = gGT->drivers[i];
		if (Chaos_IsTimeStopped() && Chaos_UsesPersonalPools() && !Chaos_PlayerHasTimeStop(i))
		{
			continue;
		}
		if (Chaos_OverridesWrongWayWarning(driver) && (driver->actionsFlagSet & ACTION_DRIVING_WRONG_WAY) == 0)
		{
			Chaos_PlaySenileVoice(driver);
			return true;
		}
	}
	return false;
}

static void Chaos_SetKartScale(float scale, struct Driver *driver)
{
	driver->kartScale = scale;
}

static void Chaos_ClearItems(struct Driver *driver)
{
	VehPhysProc_RemoveHeldItem(driver);
	driver->numHeldItems = 0;
	driver->itemRollTimer = 0;
	driver->noItemTimer = 0;
	driver->maskIsAku = -1;
	driver->actionsFlagSet &= ~ACTION_WEAPON_FIRE_REQUEST;
}

static void Chaos_GiveRandomItem(struct Driver *driver)
{
	// Distinct retail Race2 items, with the orb last so it can be excluded.
	static const u8 items[] = {
		HELD_ITEM_TURBO, HELD_ITEM_BOMB_1X, HELD_ITEM_MISSILE_1X,
		HELD_ITEM_TNT, HELD_ITEM_POTION, HELD_ITEM_SHIELD, HELD_ITEM_MASK,
		HELD_ITEM_CLOCK, HELD_ITEM_BOMB_3X, HELD_ITEM_MISSILE_3X, HELD_ITEM_WARPBALL,
	};
	Chaos_ClearItems(driver);
	struct GameTracker *gGT = GAME_TRACKER;
	const u32 random = ChaosEffect_NextRandom(&sdata->chaosRng);
	const int count = len(items) - ((gGT->gameMode1 & WARPBALL_HELD) != 0);
	VehPhysProc_AssignHeldItem(driver, items[random % count]);
	if (driver->heldItemID == HELD_ITEM_MASK)
	{
		driver->maskIsAku = g_config.maskMode == MASK_MODE_RANDOM ? (s8)((random >> 16) & 1) :
		                    (s8)VehPickupItem_ApplyMaskMode(driver);
	}
	VehPhysProc_PlayItemAcquireSound(driver);
}

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

static void Chaos_RestartDriver(struct Driver *driver, struct ChaosRestart *restart)
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

static void Chaos_UpdateRestarts(s32 elapsedMS)
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

static void Chaos_SendWarpball(void)
{
	struct GameTracker *gGT = GAME_TRACKER;
	struct Driver *source = NULL;
	struct Driver *target = NULL;
	for (int rank = 0; rank < (int)len(gGT->driversInRaceOrder); rank++)
	{
		struct Driver *driver = gGT->driversInRaceOrder[rank];
		if (driver == NULL || (driver->actionsFlagSet & ACTION_RACE_FINISHED))
		{
			continue;
		}
		if (driver->driverID >= gGT->numPlyrCurrGame)
		{
			source = driver;
		}
		else if (target == NULL)
		{
			target = driver;
		}
	}
	if (source != NULL && target != NULL)
	{
		VehPickupItem_ShootWarpball(source, target);
	}
}

static void Chaos_GiveWumpas(struct Driver *driver)
{
	const int missing = DRIVER_WUMPA_JUICED_COUNT - driver->numWumpas;
	if (missing <= 0)
	{
		return;
	}
	if ((sdata->gGT->gameMode1 & CRYSTAL_CHALLENGE) != 0)
	{
		// Crystal events use this pickup queue for crystals, not Wumpas.
		RB_Player_ModifyWumpa(driver, missing);
		return;
	}
	if (driver->PickupWumpaHUD.numCollected < missing)
	{
		if (driver->PickupWumpaHUD.numCollected == 0)
		{
			const RECT *viewport = &sdata->gGT->pushBuffer[driver->driverID].rect;
			driver->PickupWumpaHUD.startX = viewport->x + viewport->w / 2;
			driver->PickupWumpaHUD.startY = viewport->y + viewport->h / 2;
			driver->PickupWumpaHUD.cooldown = CHAOS_WUMPA_PICKUP_FRAMES;
		}
		driver->PickupWumpaHUD.numCollected = missing;
	}
}

static void Chaos_GiveMask(struct Driver *driver)
{
	const s8 heldMask = driver->maskIsAku;
	driver->maskIsAku = -1;
	VehPickupItem_MaskUseWeapon(driver, true);
	driver->maskIsAku = heldMask;
}

static void Chaos_GiveShield(struct Driver *driver)
{
	VehPickupItem_ShootNow(driver, HELD_ITEM_SHIELD, 0);
}

static void Chaos_GiveReserves(struct Driver *driver)
{
	if (driver->reserves <= 0 || driver->fireSpeedCap <= 0)
	{
		VehFire_Increment(driver, 0, POWER_SLIDE_HANG_TIME, CHAOS_THIRD_DRIFT_FIRE_LEVEL);
	}
	if (driver->reserves < CHAOS_INJECTION_RESERVES)
	{
		driver->reserves = CHAOS_INJECTION_RESERVES;
	}
}

static void Chaos_UpgradeTurbo(struct Driver *driver)
{
	if (driver->reserves > 0 && driver->fireSpeedCap > 0)
	{
		struct Turbo *turbo = NULL;
		for (struct Thread *thread = GAME_TRACKER->threadBuckets[TURBO].thread; thread != NULL; thread = thread->siblingThread)
		{
			if (((struct Turbo *)thread->object)->driver == driver)
			{
				turbo = thread->object;
				break;
			}
		}
		VehFire_SetLevel(driver, turbo, VEH_PHYS_FORCE_SUPER_TURBO_PAD_FIRE_LEVEL);
	}
}

static void Chaos_ApplyClockCurse(struct Driver *driver, s32 durationMS)
{
	VehPickState_ForceDamage(driver, VEH_PICK_DAMAGE_SPIN);
	driver->clockReceive = durationMS;
}

static void Chaos_SetDrivingClass(struct Driver *driver, int next)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	const int player = driver->driverID;
	const int previous = Chaos_EngineClassForDriver(driver, GAME_CHARACTER_METADATA[GAME_CHARACTER_IDS[player]].engineID);
	runtime->engineClass[player] = next;
	runtime->engineClassChanged[player] = true;
	VehBirth_SetConstsForClass(driver, next);
	Chaos_ChangeEngineAudio(driver, previous, next);
}

static void Chaos_ChangeDrivingClass(struct Driver *driver)
{
	const int player = driver->driverID;
	const int current = Chaos_EngineClassForDriver(driver, GAME_CHARACTER_METADATA[GAME_CHARACTER_IDS[player]].engineID);
	int next = ChaosEffect_NextRandom(&sdata->chaosRng) % (NUM_CLASSES - 1);
	if (next >= current)
	{
		next++;
	}
	Chaos_SetDrivingClass(driver, next);
}

int Chaos_EngineClassForDriver(const struct Driver *driver, int engineClass)
{
	if (driver != NULL && driver->driverID < (int)len(sdata->chaosRuntime.engineClass) &&
	    sdata->chaosRuntime.engineClassChanged[driver->driverID] &&
	    sdata->gGT->drivers[driver->driverID] == driver)
	{
		return sdata->chaosRuntime.engineClass[driver->driverID];
	}
	if (driver != NULL && driver->driverID < sdata->gGT->numPlyrCurrGame &&
	    sdata->gGT->drivers[driver->driverID] == driver)
	{
		return VehBirth_EngineClassForPlayer(driver->driverID, engineClass);
	}
	return engineClass;
}

static void Chaos_PreloadCharacterModels(struct BigHeader *bigfile)
{
	for (int character = 0; character < GAME_CHARACTER_COUNT; character++)
	{
		struct Model *model = VehBirth_GetModelByName(GAME_CHARACTER_METADATA[character].name_Debug);
		if (model != NULL)
		{
			sdata->chaosCharacterModels[character] = model;
			continue;
		}
		u32 size;
		u8 *file = LOAD_DramFile(bigfile, BI_RACERMODELHI + character, NULL, &size, -1);
		if (file != NULL)
		{
			sdata->chaosCharacterModels[character] = (struct Model *)(file + LOAD_MODEL_FILE_HEADER_BYTES);
		}
	}
}

static void Chaos_SetCharacterModel(struct Driver *driver, int character, struct Model *model)
{
	struct Instance *instance = driver->instSelf;
	GAME_CHARACTER_IDS[driver->driverID] = character;
	instance->model = model;
	memcpy(instance->name, model->name, sizeof(instance->name));
	int frames = VehFrameInst_GetNumAnimFrames(instance, instance->animIndex);
	if (frames == 0)
	{
		instance->animIndex = 0;
		frames = VehFrameInst_GetNumAnimFrames(instance, 0);
	}
	if (instance->animFrame >= frames)
	{
		instance->animFrame = VehFrameInst_GetStartFrame(instance->animIndex, frames);
	}
	driver->matrixArray = BAKED_GTE_MATRIX_NONE;
	driver->matrixIndex = 0;
	VehBirth_SetWheelSizeForCharacter(driver, character);
}

static void Chaos_SpawnTransformationSmoke(struct Driver *driver)
{
	for (int i = 0; i < CHAOS_TRANSFORM_SMOKE_COUNT; i++)
	{
		struct Particle *particle = Particle_Init(0, GAME_TRACKER->iconGroup[CHAOS_TRANSFORM_SMOKE_ICON_GROUP], data.emSet_BurnSmoke);
		if (particle == NULL)
		{
			break;
		}
		particle->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal = CHAOS_TRANSFORM_SMOKE_SCALE;
		particle->axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].velocity = CHAOS_TRANSFORM_SMOKE_GROWTH;
		VehPhysForce_ScaleParticle(driver, particle);
		particle->owner.driverInst = driver->instSelf;
		particle->otIndexOffset = driver->instSelf->depthBiasNormal;
	}
}

static void Chaos_TickTransformation(struct ChaosIdentity *identity, struct Driver *driver, s32 elapsedMS)
{
	identity->transformRemainingMS -= elapsedMS;
	if (identity->transformRemainingMS <= 0)
	{
		identity->transformRemainingMS = 0;
		Chaos_SetCharacterModel(driver, identity->pendingCharacter, sdata->chaosCharacterModels[identity->pendingCharacter]);
	}
}

static void Chaos_UpdateTransformations(struct GameTracker *gGT)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	for (int i = 0; i < (int)len(runtime->identity); i++)
	{
		struct ChaosIdentity *identity = &runtime->identity[i];
		struct Driver *driver = gGT->drivers[i];
		if (identity->transformRemainingMS > 0 && driver != NULL)
		{
			if ((driver->actionsFlagSet & ACTION_RACE_FINISHED) != 0)
			{
				identity->transformRemainingMS = 0;
			}
			else
			{
				Chaos_TickTransformation(identity, driver, gGT->elapsedTimeMS);
			}
		}
	}
}

static void Chaos_ChangeIdentity(struct Driver *driver)
{
	bool present[GAME_CHARACTER_COUNT] = {0};
	for (int i = 0; i < (int)len(GAME_TRACKER->drivers); i++)
	{
		if (GAME_TRACKER->drivers[i] != NULL)
		{
			present[GAME_CHARACTER_IDS[i]] = true;
		}
	}
	for (int i = 0; i < (int)len(sdata->chaosRuntime.identity); i++)
	{
		const struct ChaosIdentity *identity = &sdata->chaosRuntime.identity[i];
		if (identity->transformRemainingMS > 0)
		{
			present[identity->pendingCharacter] = true;
		}
	}
	int choices[GAME_CHARACTER_COUNT];
	int count = 0;
	for (int character = 0; character < GAME_CHARACTER_COUNT; character++)
	{
		if (!present[character] && sdata->chaosCharacterModels[character] != NULL)
		{
			choices[count++] = character;
		}
	}
	if (count == 0)
	{
		return;
	}
	const int player = driver->driverID;
	const int character = choices[ChaosEffect_NextRandom(&sdata->chaosRng) % count];
	struct ChaosIdentity *identity = &sdata->chaosRuntime.identity[player];
	if (!identity->changed)
	{
		identity->originalCharacter = GAME_CHARACTER_IDS[player];
		identity->originalModel = driver->instSelf->model;
		identity->changed = true;
	}
	Chaos_SetDrivingClass(driver, GAME_CHARACTER_METADATA[character].engineID);
	identity->pendingCharacter = character;
	identity->transformRemainingMS = CHAOS_TRANSFORM_DELAY_MS;
	Chaos_SpawnTransformationSmoke(driver);
}

static void Chaos_MoveDriverThread(struct Driver *driver, int from, int to)
{
	struct Thread *thread = driver->instSelf->thread;
	struct Thread **link = &GAME_TRACKER->threadBuckets[from].thread;
	while (*link != thread)
	{
		link = &(*link)->siblingThread;
	}
	*link = thread->siblingThread;
	thread->siblingThread = GAME_TRACKER->threadBuckets[to].thread;
	GAME_TRACKER->threadBuckets[to].thread = thread;
	thread->flags = (thread->flags & ~0xffu) | to;
}

static void Chaos_RefreshBodySwapModifiers(void)
{
	for (int player = 0; player < GAME_TRACKER->numPlyrCurrGame; player++)
	{
		struct Driver *driver = GAME_TRACKER->drivers[player];
		if (driver == NULL || (driver->actionsFlagSet & ACTION_RACE_FINISHED) != 0)
		{
			continue;
		}
		if (Chaos_IsActiveForDriver(CHAOS_EFFECT_BIG_KART_ENERGY, driver))
		{
			Chaos_SetKartScale(CHAOS_GIANT_SCALE, driver);
		}
		else if (Chaos_IsActiveForDriver(CHAOS_EFFECT_SMALL_KART_ENERGY, driver))
		{
			Chaos_SetKartScale(s_chaosSmallScale, driver);
		}
		if (Chaos_IsActiveForDriver(CHAOS_EFFECT_THIS_IS_FIRE, driver))
		{
			Chaos_UpgradeTurbo(driver);
		}
	}
}

void Chaos_FollowBody(int player, struct Driver *driver)
{
	struct CameraDC *camera = &GAME_TRACKER->cameraDC[player];
	camera->driverToFollow = driver;
	camera->flags &= ~CAMERA_FLAG_MASK_GRAB;
	camera->flags |= CAMERA_FLAG_DIRECTION_CHANGED | CAMERA_FLAG_RESET_RAIN_POS;
	memset(&camera->BlastedLerp, 0, sizeof(camera->BlastedLerp));
	camera->heightSmoothing = (struct CameraHeightSmoothing){0};
	sdata->chaosRuntime.cameraBlendPending[player] = true;
}

static void Chaos_SwapBodies(int first, int second)
{
	struct GameTracker *tracker = GAME_TRACKER;
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	struct Driver *a = tracker->drivers[first];
	struct Driver *b = tracker->drivers[second];
	const bool aHuman = first < tracker->numPlyrCurrGame;
	const bool bHuman = second < tracker->numPlyrCurrGame;
	const int aEngine = Chaos_EngineClassForDriver(a, GAME_CHARACTER_METADATA[GAME_CHARACTER_IDS[first]].engineID);
	const int bEngine = Chaos_EngineClassForDriver(b, GAME_CHARACTER_METADATA[GAME_CHARACTER_IDS[second]].engineID);
	const s16 character = GAME_CHARACTER_IDS[first];
	GAME_CHARACTER_IDS[first] = GAME_CHARACTER_IDS[second];
	GAME_CHARACTER_IDS[second] = character;
	tracker->drivers[first] = b;
	tracker->drivers[second] = a;
	a->driverID = second;
	b->driverID = first;
	// The rank number belongs to the viewport, not the physical kart.
	struct Instance *bigNum = a->instBigNum;
	a->instBigNum = b->instBigNum;
	b->instBigNum = bigNum;
	// Rank icon animation, unlike the viewport number, follows the kart.
	s16 rankIcon = data.rankIconsCurr[first];
	data.rankIconsCurr[first] = data.rankIconsCurr[second];
	data.rankIconsCurr[second] = rankIcon;
	rankIcon = data.rankIconsDesired[first];
	data.rankIconsDesired[first] = data.rankIconsDesired[second];
	data.rankIconsDesired[second] = rankIcon;
	rankIcon = data.rankIconsTransitionTimer[first];
	data.rankIconsTransitionTimer[first] = data.rankIconsTransitionTimer[second];
	data.rankIconsTransitionTimer[second] = rankIcon;
	if (aHuman)
	{
		tracker->humanPlayerPositions[first] = b->driverRank;
	}
	if (bHuman)
	{
		tracker->humanPlayerPositions[second] = a->driverRank;
	}

	const struct ChaosIdentity identity = runtime->identity[first];
	runtime->identity[first] = runtime->identity[second];
	runtime->identity[second] = identity;
	const u8 engine = runtime->engineClass[first];
	const bool changed = runtime->engineClassChanged[first];
	runtime->engineClass[first] = runtime->engineClass[second];
	runtime->engineClassChanged[first] = runtime->engineClassChanged[second];
	runtime->engineClass[second] = engine;
	runtime->engineClassChanged[second] = changed;
	const u8 crossings = runtime->teleportForwardCrossings[first];
	runtime->teleportForwardCrossings[first] = runtime->teleportForwardCrossings[second];
	runtime->teleportForwardCrossings[second] = crossings;

	Chaos_SetKartScale(CHAOS_KART_SCALE_NORMAL, a);
	Chaos_SetKartScale(CHAOS_KART_SCALE_NORMAL, b);
	if (aHuman != bHuman)
	{
		struct Driver *human = aHuman ? b : a;
		struct Driver *bot = aHuman ? a : b;
		Chaos_MoveDriverThread(human, ROBOT, PLAYER);
		Chaos_MoveDriverThread(bot, PLAYER, ROBOT);
		LIST_RemoveMember(&sdata->navBotList[human->botData.botPath], &human->botData.item);
		VehPhysProc_TakeControl(human);
		bot->instSelf->thread->modelIndex = DYNAMIC_ROBOT_CAR;
		BOTS_Driver_Resume(bot);
	}
	if (aHuman)
	{
		Chaos_ChangeEngineAudio(b, aEngine, bEngine);
		Chaos_FollowBody(first, b);
		runtime->touchingTurboPad[first] = false;
	}
	if (bHuman)
	{
		Chaos_ChangeEngineAudio(a, bEngine, aEngine);
		Chaos_FollowBody(second, a);
		runtime->touchingTurboPad[second] = false;
	}
}

static void Chaos_ReturnBodySwap(int first)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	const int second = runtime->bodySwapPartner[first] - 1;
	runtime->bodySwapPartner[first] = 0;
	runtime->bodySwapPartner[second] = 0;
	Chaos_SwapBodies(first, second);
}

static void Chaos_ReturnBodySwaps(void)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	for (int first = 0; first < (int)len(runtime->bodySwapPartner); first++)
	{
		const int second = runtime->bodySwapPartner[first] - 1;
		if (second > first)
		{
			Chaos_ReturnBodySwap(first);
		}
	}
}

static void Chaos_LaunchDriver(struct Driver *driver)
{
	Vec3 velocity;
	VehPhysForce_ConvertSpeedToVec(driver, &velocity);
	// Canonical speed stores the whole vector length in a signed 16-bit field.
	const s64 maxSpeedSquared = (s64)INT16_MAX * INT16_MAX;
	s64 horizontalSquared = (s64)velocity.x * velocity.x + (s64)velocity.z * velocity.z;
	if (horizontalSquared > maxSpeedSquared)
	{
		// Account for the negative-speed endpoint and fixed-point trig rounding.
		int horizontalSpeed = VehCalc_FastSqrt((u32)horizontalSquared, 0);
		if ((s64)horizontalSpeed * horizontalSpeed < horizontalSquared)
		{
			horizontalSpeed++;
		}
		velocity.x = (s64)velocity.x * INT16_MAX / horizontalSpeed;
		velocity.z = (s64)velocity.z * INT16_MAX / horizontalSpeed;
		horizontalSquared = (s64)velocity.x * velocity.x + (s64)velocity.z * velocity.z;
	}
	const int maxVerticalSpeed = VehCalc_FastSqrt((u32)(maxSpeedSquared - horizontalSquared), 0);
	const s64 verticalSpeed = (s64)velocity.y +
	    (s64)Chaos_JumpForceForDriver(driver, driver->const_JumpForce) * CHAOS_LAUNCH_JUMP_MULTIPLIER;
	velocity.y = verticalSpeed > maxVerticalSpeed ? maxVerticalSpeed :
	             verticalSpeed < -maxVerticalSpeed ? -maxVerticalSpeed : (int)verticalSpeed;
	VehPhysCrash_ConvertVecToSpeed(driver, &velocity);
	driver->velocity = velocity;
}

static int Chaos_LowGravity(int gravity)
{
	return gravity / CHAOS_LOW_GRAVITY_DIVISOR;
}

static int Chaos_HeavyGravity(int gravity)
{
	return gravity * CHAOS_HEAVY_GRAVITY_MULTIPLIER;
}

static int Chaos_DefectiveBoostSpeed(const struct Driver *driver, int speed)
{
	if (driver->reserves != 0)
	{
		// Retail already added the fire bonus; subtract it twice to invert it.
		speed -= 2 * driver->fireSpeedCap;
	}
	return speed > 0 ? speed : 0;
}

static int Chaos_PercentOf(int value, int percent)
{
	return (s64)value * percent / CHAOS_PERCENT_SCALE;
}

static int Chaos_SuperSpeed(int speed)
{
	return speed * CHAOS_SUPER_SPEED_MULTIPLIER;
}

static void Chaos_ClearReserves(struct Driver *driver)
{
	driver->reserves = 0;
	driver->turbo_outsideTimer = 0;
}

static int Chaos_KeepReserves(const struct Driver *driver, int reserves)
{
	return reserves < driver->reserves ? driver->reserves : reserves;
}

static void Chaos_ExplodeCrate(struct Driver *driver, struct Instance *crateInst)
{
	RB_Burst_InitVisual(crateInst);
	PlaySound3D(HOWL_SFX_TNT_EXPLOSION, crateInst);
	VehPickState_ForceDamage(driver, VEH_PICK_DAMAGE_BLAST);
}

static u32 Chaos_PreventHopping(struct Driver *driver, u32 actions)
{
	driver->jump_TenBuffer = 0;
	return actions & ~ACTION_JUMP_BUTTON_HELD;
}

static void Chaos_RequestHop(struct Driver *driver)
{
	// Renew for this physics step; normal jump eligibility and force stay intact.
	if (driver->jump_TenBuffer == 0)
	{
		driver->jump_TenBuffer = 1;
	}
}

static void Chaos_TickWumpaDrain(s32 elapsedMS, struct Driver *driver)
{
	const s32 frameMS = sdata->gGT->elapsedTimeMS;
	const s32 previousMS = elapsedMS > frameMS ? elapsedMS - frameMS : 0;
	const bool didPulse = elapsedMS / CHAOS_WUMPA_DRAIN_PERIOD_MS > previousMS / CHAOS_WUMPA_DRAIN_PERIOD_MS;
	if (didPulse && driver->numWumpas > 0)
	{
		driver->numWumpas -= 1;
	}
}
