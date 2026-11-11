#include <common.h>
#include "RB_Collision.h"
#include <platform/native_audio.h>

// Effect operations only; RB_Chaos.c checks activation and driver eligibility.

enum
{
	CHAOS_PERCENT_SCALE = 100,
	CHAOS_STICK_CENTER = 0x80,
	CHAOS_STICK_RANGE = 0x80,
	CHAOS_WOBBLE_RANGE_NUMERATOR = 3,
	CHAOS_WOBBLE_RANGE_DENOMINATOR = 4,
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
	CHAOS_ANALOG_ACTION_DEAD_ZONE = 0x30,
	CHAOS_ENGINE_AUDIO_CLASS_STRIDE = 4,
	CHAOS_TOP_DOWN_HEIGHT = 0xa00,
	CHAOS_TOP_DOWN_PITCH = 0x400,
	CHAOS_TOP_DOWN_YAW_OFFSET = 0x800,
	CHAOS_TOP_DOWN_CUTAWAY_HEIGHT = 0x80,
	CHAOS_WRONG_WAY_CADENCE_MULTIPLIER = 2,
	CHAOS_TRANSFORM_DELAY_MS = 100,
	CHAOS_TRANSFORM_SMOKE_COUNT = 8,
	CHAOS_TRANSFORM_SMOKE_ICON_GROUP = 1,
	CHAOS_TRANSFORM_SMOKE_SCALE = 0x2000,
	CHAOS_TRANSFORM_SMOKE_GROWTH = 0x200,
	CHAOS_TELEPORT_LAP_DIVISOR = 10,
	CHAOS_DEFECTIVE_TELEPORT_RADIUS_DIVISOR = 5,
	CHAOS_DEFECTIVE_TELEPORT_SAMPLE_ATTEMPTS = 64,
	CHAOS_DEFECTIVE_TELEPORT_HEIGHT_LIMIT = 0x100,
	CHAOS_DEFECTIVE_TELEPORT_GEOMETRY_MARGIN = 50,
	CHAOS_TELEPORT_PROBE_HALF_HEIGHT = 0x100,
	CHAOS_PATH_INVALID_NODE = 0xff,
	CHAOS_TRACK_DISTANCE_SCALE = 8,
	CHAOS_TIME_STOP_CRATE_DELAY_MS = 200,
	CHAOS_TIME_STOP_TICK_PERIOD_MS = 192,
	CHAOS_TIME_STOP_TICK_COUNT = 6,
	CHAOS_POTION_HIT_RADIUS_SQUARED = 0x1900,
};

bool Chaos_IsHumanThread(const struct Thread *thread)
{
	if (thread == NULL || thread->modelIndex != DYNAMIC_PLAYER || thread->object == NULL)
	{
		return false;
	}
	const struct Driver *driver = thread->object;
	return driver->driverID < GAME_TRACKER->numPlyrCurrGame && (driver->actionsFlagSet & ACTION_BOT) == 0;
}

bool Chaos_IsHumanCharacter(int character)
{
	for (int i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
	{
		if (GAME_CHARACTER_IDS[i] == character)
		{
			return true;
		}
	}
	return false;
}

bool Chaos_IsHumanOwnedThread(const struct Thread *thread)
{
	if (thread == NULL)
	{
		return false;
	}
	if (Chaos_IsHumanThread(thread) || (thread->flags & THREAD_FLAG_HUMAN_EFFECT) != 0)
	{
		return true;
	}
	const int bucket = thread->flags & 0xff;
	if (thread->object != NULL && thread->funcThTick == VehTurbo_ThTick && bucket == TURBO)
	{
		const struct Driver *driver = ((const struct Turbo *)thread->object)->driver;
		return driver != NULL && driver->instSelf != NULL && Chaos_IsHumanThread(driver->instSelf->thread);
	}
	if (thread->object != NULL && (bucket == MINE || bucket == TRACKING))
	{
		const struct Instance *owner = bucket == MINE ? ((const struct MineWeapon *)thread->object)->instParent :
		    ((const struct TrackerWeapon *)thread->object)->instParent;
		// Level mines also carry a driver pointer; they belong to the world.
		if (thread->inst == NULL || thread->inst->instDef != NULL || owner == NULL)
		{
			return false;
		}
		// Pooled weapon payloads may still contain the previous allocation's owner during birth.
		for (int i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
		{
			const struct Driver *driver = GAME_TRACKER->drivers[i];
			if (driver != NULL && owner == driver->instSelf && (driver->actionsFlagSet & ACTION_BOT) == 0)
			{
				return true;
			}
		}
		return false;
	}
	return Chaos_IsHumanThread(thread->parentThread);
}

bool Chaos_HasDeferredImpact(const struct Thread *thread)
{
	for (int i = 0; i < sdata->chaosRuntime.numDeferredImpacts; i++)
	{
		const struct ChaosDeferredImpact *impact = &sdata->chaosRuntime.deferredImpacts[i];
		if (impact->source == thread || impact->target == thread)
		{
			return true;
		}
	}
	return false;
}

bool Chaos_IsFrozenWorldThread(const struct Thread *thread)
{
	return thread != NULL && Chaos_IsTimeStopped() && !Chaos_IsHumanOwnedThread(thread);
}

static bool Chaos_IsContactTriggeredMine(const struct Thread *thread)
{
	return thread->modelIndex == PU_EXPLOSIVE_CRATE || thread->modelIndex == STATIC_BEAKER_RED || thread->modelIndex == STATIC_BEAKER_GREEN;
}

static bool Chaos_IsWaitingProjectileImpact(enum ChaosImpactKind kind)
{
	return kind == CHAOS_IMPACT_MINE_DRIVER || kind == CHAOS_IMPACT_MOVING_DRIVER;
}

bool Chaos_DeferImpact(enum ChaosImpactKind kind, struct Thread *source, struct Thread *target, struct BSP *bsp)
{
	if (!Chaos_IsTimeStopped() || source == NULL)
	{
		return false;
	}
	const bool delayedMineContact = kind == CHAOS_IMPACT_MINE_DRIVER && Chaos_IsContactTriggeredMine(source);
	if (!delayedMineContact && (!Chaos_IsHumanOwnedThread(source) ||
	    (target != NULL && !Chaos_IsFrozenWorldThread(target))))
	{
		return false;
	}
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	for (int i = 0; i < runtime->numDeferredImpacts; i++)
	{
		const struct ChaosDeferredImpact *impact = &runtime->deferredImpacts[i];
		if (impact->kind == kind && impact->source == source && impact->target == target && impact->bsp == bsp)
		{
			return true;
		}
	}
	if (runtime->numDeferredImpacts == CHAOS_MAX_DEFERRED_IMPACTS)
	{
		return false;
	}
	runtime->deferredImpacts[runtime->numDeferredImpacts++] = (struct ChaosDeferredImpact){
		.kind = kind, .source = source, .target = target, .bsp = bsp,
		.pos = source->inst != NULL ? (SVec3){source->inst->matrix.t[0], source->inst->matrix.t[1], source->inst->matrix.t[2]} : (SVec3){0},
	};
	return true;
}

bool Chaos_DeferDriverDamage(struct Driver *victim, int damage, struct Driver *attacker, int reason)
{
	if (attacker == NULL || attacker->instSelf == NULL || victim->instSelf == NULL ||
	    !Chaos_IsHumanThread(attacker->instSelf->thread) || !Chaos_IsFrozenWorldThread(victim->instSelf->thread))
	{
		return false;
	}
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	if (runtime->numDeferredImpacts == CHAOS_MAX_DEFERRED_IMPACTS)
	{
		return false;
	}
	runtime->deferredImpacts[runtime->numDeferredImpacts++] = (struct ChaosDeferredImpact){
		.kind = CHAOS_IMPACT_DRIVER_DAMAGE, .source = attacker->instSelf->thread, .target = victim->instSelf->thread,
		.damage = damage, .reason = reason, .attackerSlot = attacker->driverID,
	};
	return true;
}

void Chaos_RecordFrozenMaskContacts(struct Driver *attacker)
{
	if (!Chaos_IsTimeStopped() || !g_config.maskDamagesOthers ||
	    (attacker->actionsFlagSet & ACTION_MASK_WEAPON) == 0 || attacker->instSelf == NULL ||
	    !Chaos_IsHumanThread(attacker->instSelf->thread))
	{
		return;
	}
	enum { MASK_DAMAGE = 2, MASK_DAMAGE_REASON = 6 };
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	for (int i = 0; i < (int)len(GAME_TRACKER->drivers); i++)
	{
		struct Driver *victim = GAME_TRACKER->drivers[i];
		if (victim == NULL || victim->instSelf == NULL || !Chaos_IsFrozenWorldThread(victim->instSelf->thread) ||
		    (victim->actionsFlagSet & ACTION_MASK_WEAPON) != 0 ||
		    (victim->instSelf->thread->flags & (THREAD_FLAG_DEAD | THREAD_FLAG_DISABLE_COLLISION)) != 0)
		{
			continue;
		}
		const int radius = VehPhysForce_ScaleValue(attacker, attacker->instSelf->thread->driverHitRadius) +
		    VehPhysForce_ScaleValue(victim, victim->instSelf->thread->driverHitRadius);
		const s64 dx = (s64)attacker->instSelf->matrix.t[0] - victim->instSelf->matrix.t[0];
		const s64 dy = (s64)attacker->instSelf->matrix.t[1] - victim->instSelf->matrix.t[1];
		const s64 dz = (s64)attacker->instSelf->matrix.t[2] - victim->instSelf->matrix.t[2];
		if (dx * dx + dy * dy + dz * dz >= (s64)radius * radius)
		{
			continue;
		}
		bool recorded = false;
		for (int j = 0; j < runtime->numDeferredImpacts; j++)
		{
			const struct ChaosDeferredImpact *impact = &runtime->deferredImpacts[j];
			if (impact->kind == CHAOS_IMPACT_DRIVER_DAMAGE && impact->target == victim->instSelf->thread &&
			    impact->source == attacker->instSelf->thread && impact->reason == MASK_DAMAGE_REASON)
			{
				recorded = true;
				break;
			}
		}
		if (!recorded)
		{
			Chaos_DeferDriverDamage(victim, MASK_DAMAGE, attacker, MASK_DAMAGE_REASON);
		}
	}
}

static void Chaos_ResolveTriggeredMine(struct Thread *source)
{
	struct MineWeapon *mine = source->object;
	if (source->modelIndex != PU_EXPLOSIVE_CRATE)
	{
		// Potions retain their normal contact radius and effects, but use current positions.
		struct Instance *hit = RB_Hazard_CollideWithDrivers(source->inst, mine->parentSafetyFrames,
		    CHAOS_POTION_HIT_RADIUS_SQUARED, mine->instParent);
		if (hit != NULL)
		{
			RB_GenericMine_CollideDriver(source, hit->thread);
			return;
		}
	}
	// Nitro's ordinary blast checks its area; a missed potion still shatters.
	RB_GenericMine_ThDestroy(source, source->inst, mine);
}

static void Chaos_ResolveDeferredImpact(struct ChaosDeferredImpact impact)
{
	const bool triggeredMine = impact.kind == CHAOS_IMPACT_MINE_DRIVER && Chaos_IsContactTriggeredMine(impact.source);
	if (Chaos_IsWaitingProjectileImpact(impact.kind) && (impact.source->flags & THREAD_FLAG_DEAD) != 0)
	{
		return;
	}
	if (!triggeredMine && impact.target != NULL && (impact.target->flags & THREAD_FLAG_DEAD) != 0)
	{
		return;
	}
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
	memset(sps, 0, sizeof(*sps));
	sps->Union.ThBuckColl.thread = impact.source;
	sps->Input1.modelID = impact.source->modelIndex;
	sps->Input1.pos = impact.pos;
	sps->bspHitbox = impact.bsp;
	switch (impact.kind)
	{
	case CHAOS_IMPACT_MINE_DRIVER:
		if (triggeredMine)
		{
			Chaos_ResolveTriggeredMine(impact.source);
		}
		else
		{
			RB_GenericMine_CollideDriver(impact.source, impact.target);
		}
		break;
	case CHAOS_IMPACT_MOVING_DRIVER:
		RB_MovingExplosive_CollideDriver(impact.source, impact.target);
		break;
	case CHAOS_IMPACT_BURST_THREAD:
		RB_Burst_CollThBucket(sps, impact.target);
		break;
	case CHAOS_IMPACT_BURST_LEVEL:
		RB_Burst_CollLevInst(sps, impact.bsp);
		break;
	case CHAOS_IMPACT_HAZARD_THREAD:
		if (impact.target->funcThCollide != NULL)
		{
			((ThreadScratchCollideFunc)impact.target->funcThCollide)(impact.target, impact.source, impact.target->funcThCollide, NULL);
		}
		break;
	case CHAOS_IMPACT_HAZARD_LEVEL:
		RB_Hazard_CollLevInst(sps, impact.source);
		if (impact.source->modelIndex != DYNAMIC_WARPBALL && impact.bsp->data.hitbox.instDef != NULL && impact.bsp->data.hitbox.instDef->modelID == STATIC_TEETH &&
		    impact.bsp->data.hitbox.instDef->ptrInstance != NULL)
		{
			RB_Teeth_OpenDoor(impact.bsp->data.hitbox.instDef->ptrInstance);
		}
		break;
	case CHAOS_IMPACT_DRIVER_DAMAGE:
		RB_Hazard_HurtDriver(impact.target->object, impact.damage, GAME_TRACKER->drivers[impact.attackerSlot], impact.reason);
		((struct Driver *)impact.target->object)->damageColorTimer = 0x1e;
		break;
	}
}

void Chaos_ResolveDeferredImpacts(void)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	if (Chaos_IsTimeStopped())
	{
		return;
	}
	for (int i = 0; i < runtime->numDeferredImpacts; i++)
	{
		Chaos_ResolveDeferredImpact(runtime->deferredImpacts[i]);
	}
	runtime->numDeferredImpacts = 0;
}

bool Chaos_ShouldFreezeThread(const struct Thread *thread)
{
	if (!Chaos_IsTimeStopped())
	{
		return false;
	}
	for (int i = 0; i < sdata->chaosRuntime.numDeferredImpacts; i++)
	{
		const struct ChaosDeferredImpact *impact = &sdata->chaosRuntime.deferredImpacts[i];
		if (impact->source == thread && Chaos_IsWaitingProjectileImpact(impact->kind))
		{
			return true;
		}
	}
	const int bucket = thread->flags & 0xff;
	if (Chaos_IsHumanOwnedThread(thread) || bucket == CAMERA || bucket == HUD || bucket == STARTTEXT)
	{
		return false;
	}
	return true;
}

void Chaos_CollideFrozenThread(struct Thread *thread)
{
	if (!Chaos_IsFrozenWorldThread(thread) || thread->inst == NULL || thread->object == NULL ||
	    (thread->flags & (THREAD_FLAG_DEAD | THREAD_FLAG_DISABLE_COLLISION)) != 0)
	{
		return;
	}
	if (thread->funcThTick == RB_GenericMine_ThTick)
	{
		const struct MineWeapon *mine = thread->object;
		const int radius = (u32)(u16)thread->modelIndex - STATIC_BEAKER_RED < 2 ? CHAOS_POTION_HIT_RADIUS_SQUARED : 0x3840;
		struct Instance *hit = RB_Hazard_CollideWithDrivers(thread->inst, mine->parentSafetyFrames, radius, mine->instParent);
		if (hit != NULL)
		{
			RB_GenericMine_CollideDriver(thread, hit->thread);
		}
	}
	else if (thread->funcThTick == RB_MovingExplosive_ThTick)
	{
		const struct TrackerWeapon *weapon = thread->object;
		struct Instance *hit = RB_Hazard_CollideWithDrivers(thread->inst, weapon->parentSafetyFrames, 0x2400, weapon->instParent);
		if (hit != NULL)
		{
			RB_MovingExplosive_CollideDriver(thread, hit->thread);
		}
	}
	else if (thread->funcThTick == RB_Warpball_ThTick)
	{
		const struct TrackerWeapon *weapon = thread->object;
		struct Instance *hit = RB_Hazard_CollideWithDrivers(thread->inst, weapon->parentSafetyFrames, 0x9000, weapon->instParent);
		if (hit != NULL && hit->thread->object != weapon->driverParent)
		{
			RB_Hazard_HurtDriver(hit->thread->object, 2, weapon->driverParent, 0);
		}
	}
	else if (thread->funcThTick == RB_Minecart_ThTick)
	{
		RB_Minecart_CollideDrivers(thread);
	}
	else if (thread->funcThTick == RB_Snowball_ThTick)
	{
		RB_Snowball_CollideDrivers(thread);
	}
	else if (thread->funcThTick == RB_Baron_ThTick)
	{
		RB_Baron_CollideDrivers(thread);
	}
	else if (thread->funcThTick == RB_Spider_ThTick)
	{
		RB_Spider_CollideDrivers(thread);
	}
	else if (thread->funcThTick == RB_Plant_ThTick_Hungry)
	{
		RB_Plant_ThTick_Hungry(thread);
	}
	else if (thread->funcThTick == RB_FlameJet_ThTick)
	{
		RB_FlameJet_CollideDrivers(thread);
	}
	else if (thread->funcThTick == RB_Armadillo_ThTick_Rolling || thread->funcThTick == RB_Armadillo_ThTick_TurnAround)
	{
		RB_CheckHazardCollisions(thread->inst, thread, 1, 0x2400, 0x71);
	}
	else if (thread->funcThTick == RB_Seal_ThTick_Move || thread->funcThTick == RB_Seal_ThTick_TurnAround)
	{
		RB_CheckHazardCollisions(thread->inst, thread, 1, 0x4000, 0x78);
	}
	else if (thread->funcThTick == RB_Fireball_ThTick)
	{
		const struct Fireball *fireball = thread->object;
		if (fireball->cooldown == 0 && thread->inst->instDef != NULL &&
		    thread->inst->matrix.t[1] >= thread->inst->instDef->pos.y - 0x440)
		{
			RB_CheckHazardCollisions(thread->inst, thread, 4, 0x10000, 0);
		}
	}
}

bool Chaos_ShouldFreezeParticle(const struct Particle *particle)
{
	if (!Chaos_IsTimeStopped() || (particle->flagsAxis & PARTICLE_AXIS_FLAG_HUMAN_EFFECT) != 0)
	{
		return false;
	}
	for (int i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
	{
		const struct Driver *driver = GAME_TRACKER->drivers[i];
		if (driver != NULL && driver->instSelf != NULL && particle->owner.driverInst == driver->instSelf &&
		    Chaos_IsHumanThread(driver->instSelf->thread))
		{
			return false;
		}
	}
	return true;
}

bool Chaos_SetHumanAudio(bool human)
{
	const bool previous = sdata->chaosRuntime.humanAudioContext;
	sdata->chaosRuntime.humanAudioContext = human;
	return previous;
}

void Chaos_TagAudioChannel(struct ChannelStats *channel)
{
	channel->flags &= ~HOWL_CHANNEL_HUMAN_AUDIO;
	if (sdata->chaosRuntime.humanAudioContext)
	{
		channel->flags |= HOWL_CHANNEL_HUMAN_AUDIO;
	}
}

bool Chaos_IsMenuPreviewSong(const struct Song *song)
{
	return sdata->OptionSlider_BoolPlay && sdata->OptionSlider_Index == HOWL_VOLUME_TYPE_MUSIC &&
	    (song->id == CSEQ_SONG_AKU || song->id == CSEQ_SONG_UKA);
}

bool Chaos_ShouldFreezeAudioChannel(const struct ChannelStats *channel)
{
	if (!Chaos_IsTimeStopped())
	{
		return false;
	}
	const u16 soundID = channel->soundID & 0xffff;
	if (channel->type == HOWL_CHANNEL_TYPE_OTHER_FX &&
	    (soundID == MENU_SFX_MOVE || soundID == MENU_SFX_BACK ||
	     soundID == MENU_SFX_CONFIRM || soundID == MENU_SFX_LOCKED))
	{
		return false;
	}
	if (channel->type == HOWL_CHANNEL_TYPE_MUSIC)
	{
		if ((u32)channel->soundID < len(sdata->songSeq))
		{
			const struct SongSeq *sequence = &sdata->songSeq[channel->soundID];
			if (sequence->songPoolIndex < len(sdata->songPool))
			{
				return !Chaos_IsMenuPreviewSong(&sdata->songPool[sequence->songPoolIndex]);
			}
		}
		return true;
	}
	if (channel->type == HOWL_CHANNEL_TYPE_ENGINE_FX)
	{
		for (int i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
		{
			const struct Driver *driver = GAME_TRACKER->drivers[i];
			if (driver != NULL && (driver->actionsFlagSet & ACTION_BOT) == 0 &&
			    channel->soundID == EngineClass_AudioClass(Chaos_EngineClassForDriver(driver, GAME_CHARACTER_METADATA[GAME_CHARACTER_IDS[i]].engineID)) *
			        CHAOS_ENGINE_AUDIO_CLASS_STRIDE + i)
			{
				return false;
			}
		}
		return true;
	}
	return (channel->flags & HOWL_CHANNEL_HUMAN_AUDIO) == 0;
}

void Chaos_UpdateTimeStopAudio(void)
{
	u32 paused = 0;
	for (const struct ChannelStats *channel = (const struct ChannelStats *)sdata->channelTaken.first;
	     channel != NULL; channel = channel->link.links.next)
	{
		if (Chaos_ShouldFreezeAudioChannel(channel))
		{
			paused |= 1u << channel->channelID;
		}
	}
	NativeAudio_SetPausedVoices(paused, Chaos_IsTimeStopped() && !sdata->chaosRuntime.humanXA);
}

static void Chaos_PlayTimeEffectSound(int sound)
{
	const bool previous = Chaos_SetHumanAudio(true);
	OtherFX_Play(sound, 1);
	Chaos_SetHumanAudio(previous);
}

static void Chaos_TickTimeEffectSound(const struct ChaosActivation *entry, s32 elapsedMS)
{
	if (entry->effect != CHAOS_EFFECT_ZA_WARUDO && entry->effect != CHAOS_EFFECT_KING_CRIMSON)
	{
		return;
	}
	if (entry->elapsedMS < CHAOS_TIME_STOP_CRATE_DELAY_MS &&
	    (entry->elapsedMS + elapsedMS >= CHAOS_TIME_STOP_CRATE_DELAY_MS || entry->remainingMS <= elapsedMS))
	{
		Chaos_PlayTimeEffectSound(HOWL_SFX_TIME_CRATE_BREAK);
	}
	if (entry->effect != CHAOS_EFFECT_ZA_WARUDO)
	{
		return;
	}
	s32 cueMS = CHAOS_TIME_STOP_CRATE_DELAY_MS;
	for (int i = 0; i < CHAOS_TIME_STOP_TICK_COUNT; i++)
	{
		if (entry->elapsedMS < cueMS && entry->elapsedMS + elapsedMS >= cueMS && entry->remainingMS > elapsedMS)
		{
			Chaos_PlayTimeEffectSound(HOWL_SFX_TIME_CRATE_TICK);
		}
		cueMS += CHAOS_TIME_STOP_TICK_PERIOD_MS;
	}
}

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
		if (Chaos_OverridesWrongWayWarning(driver) && (driver->actionsFlagSet & ACTION_DRIVING_WRONG_WAY) == 0)
		{
			Chaos_PlaySenileVoice(driver);
			return true;
		}
	}
	return false;
}

static void Chaos_SetTopDownCamera(struct CameraDC *camera, const struct Driver *driver)
{
	struct PushBuffer *pushBuffer = camera->pushBuffer;
	const int x = driver->posCurr.x >> 8;
	const int y = driver->posCurr.y >> 8;
	const int z = driver->posCurr.z >> 8;
	const int height = y + CHAOS_TOP_DOWN_HEIGHT;
	pushBuffer->pos.x = (s16)x;
	pushBuffer->pos.y = (s16)(height > INT16_MAX ? INT16_MAX : height);
	pushBuffer->pos.z = (s16)z;
	pushBuffer->rot.x = CHAOS_TOP_DOWN_PITCH;
	pushBuffer->rot.y = (s16)(driver->angle + CHAOS_TOP_DOWN_YAW_OFFSET);
	pushBuffer->rot.z = 0;
	pushBuffer->distanceToScreen_PREV = pushBuffer->distanceToScreen_CURR;
	struct QuadBlock *ground = driver->underDriver != NULL ? driver->underDriver : driver->currBlockTouching;
	if (ground != NULL)
	{
		camera->ptrQuadBlock = ground;
		camera->quadBlockSearchHit = true;
	}
}

static bool Chaos_TopDownHidesQuadBlock(const struct Driver *driver, const struct QuadBlock *block, const struct LevVertex *vertices)
{
	if (block == driver->underDriver || block == driver->currBlockTouching)
	{
		return false;
	}
	// Same high-detail triangles and normal winding as COLL_FIXED_QUADBLK.
	static const u8 triangles[][3] = {
		{0, 4, 5}, {4, 6, 5}, {6, 4, 1}, {5, 6, 2},
		{8, 6, 7}, {7, 3, 8}, {1, 7, 6}, {2, 6, 8},
	};
	const int numTriangles = block->index[2] == block->index[3] ? 4 : (int)len(triangles);
	bool ceiling = false;
	for (int i = 0; i < numTriangles; i++)
	{
		const SVec3 *a = &vertices[block->index[triangles[i][0]]].pos;
		const SVec3 *b = &vertices[block->index[triangles[i][1]]].pos;
		const SVec3 *c = &vertices[block->index[triangles[i][2]]].pos;
		const s64 normalY = (s64)(c->z - a->z) * (b->x - a->x) - (s64)(c->x - a->x) * (b->z - a->z);
		if (normalY > 0)
		{
			// Keep any road-facing part, including unflagged ramps and mixed blocks.
			return false;
		}
		ceiling |= normalY < 0;
	}
	// Ceiling classification is independent of kart position and block bounds.
	return ceiling;
}

static bool Chaos_TopDownHidesInstance(const struct Driver *driver, const struct PushBuffer *pushBuffer,
                                     const struct Instance *instance, int nearestDepth, bool coversKart)
{
	if (!coversKart || instance->instDef == NULL || instance->thread != NULL)
	{
		return false;
	}
	const int ceiling = (driver->posCurr.y >> 8) + VehPhysForce_ScaleValue(driver, CHAOS_TOP_DOWN_CUTAWAY_HEIGHT);
	return nearestDepth < (pushBuffer->pos.y - ceiling) * 4;
}

static void Chaos_SetTopDownVisibility(struct VisMem *visibility, const struct mesh_info *mesh, int playerIndex)
{
	// Ground-level PVS omits road hidden behind crests and tunnel entrances.
	// Let the overhead camera's frustum select geometry instead.
	memset(visibility->visLeafList[playerIndex], 0xff, ((mesh->numBspNodes + 0x1f) >> 5) * sizeof(int));
	memset(visibility->visFaceList[playerIndex], 0xff, ((mesh->numQuadBlock + 0x1f) >> 5) * sizeof(int));
	// Force retail lists to be decoded again after expiry, even on the same block.
	visibility->visLeafSrc[playerIndex] = NULL;
	visibility->visFaceSrc[playerIndex] = NULL;
}

static const u32 s_chaosControlButtons[CHAOS_CONTROL_COUNT] = {
	BTN_CROSS_one, BTN_SQUARE_one, BTN_CIRCLE, BTN_L1, BTN_R1,
};

static u32 Chaos_AnalogActions(const struct GamepadBuffer *pad)
{
	u32 actions = 0;
	if (pad->stickRY < CHAOS_STICK_CENTER - CHAOS_ANALOG_ACTION_DEAD_ZONE)
	{
		actions |= BTN_CROSS_one;
	}
	if (pad->stickRY > CHAOS_STICK_CENTER + CHAOS_ANALOG_ACTION_DEAD_ZONE ||
	    pad->stickLY > CHAOS_STICK_CENTER + CHAOS_ANALOG_ACTION_DEAD_ZONE)
	{
		actions |= BTN_SQUARE_one;
	}
	return actions;
}

static void Chaos_UpdateControlAxes(void)
{
	for (int i = 0; i < (int)len(sdata->chaosRuntime.controls); i++)
	{
		if (Chaos_IsActiveForDriver(CHAOS_EFFECT_WHO_BOUND_THIS, sdata->gGT->drivers[i]))
		{
			struct ChaosControlShuffle *controls = &sdata->chaosRuntime.controls[i];
			controls->analogPrev = controls->analogHeld;
			controls->analogHeld = Chaos_AnalogActions(&GAMEPADS->gamepad[i]);
		}
	}
}

static void Chaos_ShuffleControls(struct ChaosControlShuffle *controls, const struct GamepadBuffer *pad)
{
	bool changed;
	do
	{
		for (int i = 0; i < CHAOS_CONTROL_COUNT; i++)
		{
			controls->destination[i] = i;
		}
		for (int i = CHAOS_CONTROL_COUNT - 1; i > 0; i--)
		{
			const int other = ChaosEffect_NextRandom(&sdata->chaosRng) % (i + 1);
			const u8 saved = controls->destination[i];
			controls->destination[i] = controls->destination[other];
			controls->destination[other] = saved;
		}
		changed = false;
		for (int i = 0; i < CHAOS_CONTROL_COUNT; i++)
		{
			changed |= controls->destination[i] != i;
		}
	} while (!changed);
	controls->analogHeld = Chaos_AnalogActions(pad);
	controls->analogPrev = controls->analogHeld;
}

static u32 Chaos_RemapButtons(const struct ChaosControlShuffle *controls, u32 buttons)
{
	const u32 mask = BTN_CROSS_one | BTN_SQUARE_one | BTN_CIRCLE | BTN_L1 | BTN_R1;
	u32 remapped = buttons & ~mask;
	for (int i = 0; i < CHAOS_CONTROL_COUNT; i++)
	{
		if ((buttons & s_chaosControlButtons[i]) != 0)
		{
			remapped |= s_chaosControlButtons[controls->destination[i]];
		}
	}
	return remapped;
}

static void Chaos_RemapControls(const struct ChaosControlShuffle *controls, struct GamepadBuffer *pad)
{
	const u32 mask = BTN_CROSS_one | BTN_SQUARE_one | BTN_CIRCLE | BTN_L1 | BTN_R1;
	const u32 held = pad->buttonsHeldCurrFrame | controls->analogHeld;
	const u32 previous = pad->buttonsHeldPrevFrame | controls->analogPrev;
	pad->buttonsHeldCurrFrame = Chaos_RemapButtons(controls, held);
	pad->buttonsHeldPrevFrame = Chaos_RemapButtons(controls, previous);
	pad->buttonsTapped = Chaos_RemapButtons(controls, (pad->buttonsTapped & ~mask) | (held & ~previous & mask));
	pad->buttonsReleased = Chaos_RemapButtons(controls, (pad->buttonsReleased & ~mask) | (previous & ~held & mask));
	pad->stickRY = CHAOS_STICK_CENTER;
	if (pad->stickLY > CHAOS_STICK_CENTER)
	{
		pad->stickLY = CHAOS_STICK_CENTER;
	}
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

static void Chaos_ResetSteeringPose(struct Driver *driver)
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

static void Chaos_EndRestartHold(struct ChaosRestart *restart, struct Driver *driver)
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

static bool Chaos_IsBeingRescued(const struct Driver *driver)
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
	VehPickState_ApplyDamage(driver, VEH_PICK_DAMAGE_SPIN, NULL, 0, true);
	driver->clockReceive = durationMS;
}

static void Chaos_ChangeEngineAudio(const struct Driver *driver, int previous, int next)
{
	previous = EngineClass_AudioClass(previous);
	next = EngineClass_AudioClass(next);
	EngineAudio_Stop(previous * CHAOS_ENGINE_AUDIO_CLASS_STRIDE + driver->driverID);
	EngineAudio_InitOnce(next * CHAOS_ENGINE_AUDIO_CLASS_STRIDE + driver->driverID, HOWL_SFX_CENTER_NO_DISTORTION);
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
	driver->wheelSize = character == NITROS_OXIDE ? 0 : VEH_BIRTH_WHEEL_SIZE;
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
		if (Chaos_IsActive(CHAOS_EFFECT_BIG_KART_ENERGY))
		{
			Chaos_SetKartScale(CHAOS_GIANT_SCALE, driver);
		}
		else if (Chaos_IsActive(CHAOS_EFFECT_SMALL_KART_ENERGY))
		{
			Chaos_SetKartScale(s_chaosSmallScale, driver);
		}
		if (Chaos_IsActive(CHAOS_EFFECT_THIS_IS_FIRE))
		{
			Chaos_UpgradeTurbo(driver);
		}
	}
}

static void Chaos_FollowBody(int player, struct Driver *driver)
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

struct ChaosTrackDestination
{
	Vec3 pos;
	int yaw;
	int checkpoint;
	int distanceToFinish;
};

static void Chaos_ReleaseRepositionHold(struct Driver *driver)
{
	struct Thread *thread = driver->instSelf->thread;
	const bool resumeDriving = Chaos_IsBeingRescued(driver) || driver->kartState == KS_ENGINE_REVVING ||
	    driver->kartState == KS_WARP_PAD;
	struct MaskHeadWeapon *mask = NULL;
	if (thread->funcThTick == BOTS_ThTick_RevEngine)
	{
		mask = driver->botData.maskObj;
	}
	else if (driver->kartState == KS_MASK_GRABBED)
	{
		mask = driver->KartStates.MaskGrab.maskObj;
	}
	else if (driver->kartState == KS_ENGINE_REVVING && driver->KartStates.RevEngine.boolMaskGrab)
	{
		mask = driver->KartStates.RevEngine.maskObj;
	}
	if (mask != NULL)
	{
		mask->scale = MASK_HEAD_SCALE_NORMAL;
		mask->rot.z &= ~MASK_HEAD_ROT_WORLD_SPACE;
		if (!driver->boolHadMaskBeforeOOB)
		{
			mask->duration = 0;
		}
	}
	struct ChaosRestart *restart = &sdata->chaosRuntime.restart[driver->driverID];
	Chaos_EndRestartHold(restart, driver);
	*restart = (struct ChaosRestart){0};
	if (resumeDriving)
	{
		thread->funcThTick = NULL;
		thread->flags &= ~THREAD_FLAG_DISABLE_COLLISION;
		driver->instSelf->flags &= ~HIDE_MODEL;
		driver->botData.maskObj = NULL;
		VehPhysProc_Driving_Init(thread, driver);
		driver->NoInputTimer = 0;
		driver->matrixArray = BAKED_GTE_MATRIX_NONE;
		driver->matrixIndex = 0;
	}
}

static void Chaos_MoveDriver(struct Driver *driver, const Vec3 *position, int yaw)
{
	Chaos_ReleaseRepositionHold(driver);
	driver->posCurr = *position;
	driver->posPrev = driver->posCurr;
	driver->underDriver = NULL;
	driver->currBlockTouching = NULL;
	driver->distanceDrivenBackwards = 0;
	driver->collisionFlags = 0;
	driver->stepFlagSet = 0;
	driver->actionsFlagSet &= ~(ACTION_DRIVING_WRONG_WAY | ACTION_CHECKPOINT_BRANCH_PENDING | ACTION_WARP);
	driver->angle = yaw;
	driver->rotCurr = (SVec3Slot){0, yaw, 0, 0};
	driver->rotPrev = driver->rotCurr;
	Chaos_ResetSteeringPose(driver);
	driver->axisRotationX = yaw;
	VehPhysForce_RotAxisAngle(&driver->matrixMovingDir, CTR_VECTOR_DATA(&driver->AxisAngle2_normalVec), yaw);
	driver->matrixFacingDir = driver->matrixMovingDir;
	VehPhysForce_ConvertSpeedToVec(driver, &driver->velocity);
	driver->xSpeed = driver->velocity.x;
	driver->ySpeed = driver->velocity.y;
	driver->zSpeed = driver->velocity.z;
	driver->accel = (SVec3){0};
	driver->instSelf->matrix.t[0] = driver->posCurr.x >> FRACTIONAL_BITS_8;
	driver->instSelf->matrix.t[1] = driver->posCurr.y >> FRACTIONAL_BITS_8;
	driver->instSelf->matrix.t[2] = driver->posCurr.z >> FRACTIONAL_BITS_8;
	Chaos_FollowBody(driver->driverID, driver);
	sdata->chaosRuntime.touchingTurboPad[driver->driverID] = false;
	OtherFX_Play(SOUND_INVISIBILITY, 1);
}

static bool Chaos_RepositionDriver(struct Driver *driver, const struct ChaosTrackDestination *destination)
{
	struct GameTracker *tracker = GAME_TRACKER;
	if (tracker->level1->ptr_mesh_info == NULL)
	{
		return false;
	}
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x108);
	const SVec3 top = {destination->pos.x >> FRACTIONAL_BITS_8,
	    (destination->pos.y >> FRACTIONAL_BITS_8) - CHAOS_TELEPORT_PROBE_HALF_HEIGHT,
	    destination->pos.z >> FRACTIONAL_BITS_8};
	const SVec3 bottom = {top.x, top.y + 2 * CHAOS_TELEPORT_PROBE_HALF_HEIGHT, top.z};
	sps->ptr_mesh_info = tracker->level1->ptr_mesh_info;
	sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND;
	sps->Union.QuadBlockColl.quadFlagsIgnored = QUADBLOCK_FLAG_KILL_PLANE;
	sps->Union.QuadBlockColl.searchFlags = tracker->numPlyrCurrGame < 3 ? COLL_SEARCH_HIGH_LOD : 0;
	COLL_SearchBSP_CallbackQUADBLK(&top, &bottom, sps, 0);
	if (!sps->boolDidTouchQuadblock)
	{
		return false;
	}
	struct QuadBlock *ground = sps->hit.ptrQuadblock;
	const SVec3 normal = sps->hit.plane.normal;
	Vec3 position = destination->pos;
	position.y = sps->Union.QuadBlockColl.hitPos.y * FP8(1);
	driver->AxisAngle1_normalVec = normal;
	driver->AxisAngle2_normalVec = normal;
	driver->AxisAngle3_normalVec = normal;
	driver->normalVecUP = normal;
	Chaos_MoveDriver(driver, &position, destination->yaw);
	driver->underDriver = ground;
	driver->lastValid = ground;
	driver->currBlockTouching = ground;
	driver->quadBlockHeight = position.y;
	driver->currentTerrain = ground->terrain_type;
	driver->terrainMeta1 = VehAfterColl_GetTerrain(driver->currentTerrain);
	driver->terrainMeta2 = driver->terrainMeta1;
	driver->checkpoint.currentIndex = ground->checkpointIndex != CHAOS_PATH_INVALID_NODE ?
	    ground->checkpointIndex : destination->checkpoint;
	driver->distanceToFinish_curr = destination->distanceToFinish;
	driver->distanceToFinish_checkpoint = destination->distanceToFinish;
	return true;
}

static bool Chaos_IsTeleportNearGeometry(const struct mesh_info *mesh, const Vec3 *position)
{
	const int x = position->x >> FRACTIONAL_BITS_8;
	const int y = position->y >> FRACTIONAL_BITS_8;
	const int z = position->z >> FRACTIONAL_BITS_8;
	const struct BoundingBox *bounds = &mesh->bspRoot->box;
	if (x < bounds->min.x || x > bounds->max.x || z < bounds->min.z || z > bounds->max.z)
	{
		return false;
	}
	for (int i = 0; i < mesh->numBspNodes; i++)
	{
		const struct BSP *node = &mesh->bspRoot[i];
		if ((node->flag & BSP_NODE_FLAG_LEAF) == 0 || node->data.leaf.numQuads == 0)
		{
			continue;
		}
		bounds = &node->box;
		if (x >= bounds->min.x - CHAOS_DEFECTIVE_TELEPORT_GEOMETRY_MARGIN &&
		    x <= bounds->max.x + CHAOS_DEFECTIVE_TELEPORT_GEOMETRY_MARGIN &&
		    z >= bounds->min.z - CHAOS_DEFECTIVE_TELEPORT_GEOMETRY_MARGIN &&
		    z <= bounds->max.z + CHAOS_DEFECTIVE_TELEPORT_GEOMETRY_MARGIN &&
		    y >= bounds->min.y - CHAOS_DEFECTIVE_TELEPORT_HEIGHT_LIMIT &&
		    y <= bounds->max.y + CHAOS_DEFECTIVE_TELEPORT_HEIGHT_LIMIT)
		{
			return true;
		}
	}
	return false;
}

static void Chaos_DefectiveTeleportDriver(struct Driver *driver)
{
	const struct Level *level = GAME_TRACKER->level1;
	if (level == NULL || level->ptr_restart_points == NULL || level->cnt_restart_points == 0 ||
	    level->ptr_mesh_info == NULL || level->ptr_mesh_info->bspRoot == NULL)
	{
		return;
	}
	const int radius = level->ptr_restart_points[0].distToFinish * CHAOS_TRACK_DISTANCE_SCALE /
	    CHAOS_DEFECTIVE_TELEPORT_RADIUS_DIVISOR;
	if (radius == 0)
	{
		return;
	}
	const int yaw = driver->angle;
	const int sine = MATH_Sin(yaw);
	const int cosine = MATH_Sin(yaw + ANG_HALF_PI);
	const int heightLimit = radius < CHAOS_DEFECTIVE_TELEPORT_HEIGHT_LIMIT ? radius : CHAOS_DEFECTIVE_TELEPORT_HEIGHT_LIMIT;
	const s64 worldRadius = radius * FP8(1);
	const s64 worldRadiusSquared = worldRadius * worldRadius;
	Vec3 position = driver->posCurr;
	for (int attempt = 0; attempt < CHAOS_DEFECTIVE_TELEPORT_SAMPLE_ATTEMPTS; attempt++)
	{
		// Keep the rearward range, but flatten the sample vertically near level geometry.
		const int back = (u64)ChaosEffect_NextRandom(&sdata->chaosRng) * (radius + 1) / (CHAOS_RANDOM_SAMPLE_MAX + 1);
		const int side = (int)((u64)ChaosEffect_NextRandom(&sdata->chaosRng) * (2 * radius + 1) / (CHAOS_RANDOM_SAMPLE_MAX + 1)) - radius;
		const int height = (int)((u64)ChaosEffect_NextRandom(&sdata->chaosRng) * (2 * heightLimit + 1) / (CHAOS_RANDOM_SAMPLE_MAX + 1)) - heightLimit;
		// A vertical-only fallback remains possible at the world-coordinate edges.
		position.y = driver->posCurr.y + height * FP8(1);
		const Vec3 candidate = {
			driver->posCurr.x + (s64)(side * cosine - back * sine) * FP8(1) / FP_ONE,
			driver->posCurr.y + height * FP8(1),
			driver->posCurr.z + (s64)(-side * sine - back * cosine) * FP8(1) / FP_ONE,
		};
		const s64 x = candidate.x - driver->posCurr.x;
		const s64 y = candidate.y - driver->posCurr.y;
		const s64 z = candidate.z - driver->posCurr.z;
		if (x * x + y * y + z * z > worldRadiusSquared)
		{
			continue;
		}
		// Collision and camera coordinates still use signed 16-bit world units.
		if (candidate.y >= INT16_MIN * FP8(1) && candidate.y <= INT16_MAX * FP8(1) &&
		    Chaos_IsTeleportNearGeometry(level->ptr_mesh_info, &candidate))
		{
			position = candidate;
			break;
		}
	}
	position.y = position.y < INT16_MIN * FP8(1) ? INT16_MIN * FP8(1) :
	             position.y > INT16_MAX * FP8(1) ? INT16_MAX * FP8(1) : position.y;
	if (!Chaos_IsTeleportNearGeometry(level->ptr_mesh_info, &position))
	{
		return;
	}
	Chaos_MoveDriver(driver, &position, yaw);
	driver->actionsFlagSet &= ~ACTION_TOUCH_GROUND;
	driver->actionsFlagSet |= ACTION_AIRBORNE;
	driver->currentTerrain = TERRAIN_NONE;
	driver->terrainMeta1 = VehAfterColl_GetTerrain(TERRAIN_NONE);
	driver->terrainMeta2 = driver->terrainMeta1;
}

static int Chaos_GetTrackLength(const struct Level *level)
{
	if (level == NULL || level->ptr_restart_points == NULL ||
	    level->cnt_restart_points < 2 || level->cnt_restart_points > CHAOS_PATH_INVALID_NODE)
	{
		return 0;
	}
	const int length = level->ptr_restart_points[0].distToFinish * CHAOS_TRACK_DISTANCE_SCALE;
	return length > 0 ? length : 0;
}

static bool Chaos_FindTrackDestination(struct Driver *driver, int target, struct ChaosTrackDestination *destination)
{
	const struct Level *level = GAME_TRACKER->level1;
	const int length = Chaos_GetTrackLength(level);
	if (length == 0 || driver->checkpoint.currentIndex >= level->cnt_restart_points)
	{
		return false;
	}
	struct CheckpointNode *node = &level->ptr_restart_points[driver->checkpoint.currentIndex];
	for (int visited = 0; visited < level->cnt_restart_points; visited++)
	{
		struct CheckpointNode *next = RB_Warpball_NewPathNode(node,
		    driver->checkpoint.branchChoiceIndex < level->cnt_restart_points ? driver : NULL);
		const int span = ((node->distToFinish - next->distToFinish) * CHAOS_TRACK_DISTANCE_SCALE + length) % length;
		const int along = (node->distToFinish * CHAOS_TRACK_DISTANCE_SCALE - target + length) % length;
		if (span > 0 && along < span)
		{
			*destination = (struct ChaosTrackDestination){
				.pos = {
					node->pos.x * FP8(1) + (s64)(next->pos.x - node->pos.x) * FP8(1) * along / span,
					node->pos.y * FP8(1) + (s64)(next->pos.y - node->pos.y) * FP8(1) * along / span,
					node->pos.z * FP8(1) + (s64)(next->pos.z - node->pos.z) * FP8(1) * along / span,
				},
				.yaw = ratan2(next->pos.x - node->pos.x, next->pos.z - node->pos.z),
				.checkpoint = (int)(node - level->ptr_restart_points),
				.distanceToFinish = target,
			};
			return true;
		}
		node = next;
	}
	return false;
}

static void Chaos_TeleportDriver(struct Driver *driver)
{
	const int length = Chaos_GetTrackLength(GAME_TRACKER->level1);
	if (length < CHAOS_TELEPORT_LAP_DIVISOR)
	{
		return;
	}
	const int unwrapped = driver->distanceToFinish_curr % length - length / CHAOS_TELEPORT_LAP_DIVISOR;
	struct ChaosTrackDestination destination;
	if (Chaos_FindTrackDestination(driver, (unwrapped + length) % length, &destination) &&
	    Chaos_RepositionDriver(driver, &destination) && unwrapped < 0 &&
	    sdata->chaosRuntime.teleportForwardCrossings[driver->driverID] < UINT8_MAX)
	{
		sdata->chaosRuntime.teleportForwardCrossings[driver->driverID]++;
	}
}

bool Chaos_ConsumeTeleportLapCrossing(const struct Driver *driver)
{
	u8 *crossings = &sdata->chaosRuntime.teleportForwardCrossings[driver->driverID];
	if (*crossings == 0)
	{
		return false;
	}
	(*crossings)--;
	return true;
}

enum
{
	CHAOS_TIME_SKIP_MS = 10000,
	CHAOS_TIME_SKIP_PALETTE_MS = 500,
	// Bot navigation integrates speed * milliseconds / 32 in 24.8 positions.
	CHAOS_BOT_DISTANCE_TIME_SCALE = 32 * FP8(1),
};

static void Chaos_MoveSkippedBot(struct Driver *driver, const struct ChaosTrackDestination *destination, struct NavFrame *next)
{
	driver->posCurr = destination->pos;
	driver->posPrev = destination->pos;
	driver->botData.positionBackup = destination->pos;
	driver->botData.botNavFrame = next;
	BOTS_SetRotation(driver, 0);
	// Retail projects from the segment after the surface checkpoint.
	const int checkpoint = GAME_TRACKER->level1->ptr_restart_points[destination->checkpoint].nextIndex_backward;
	driver->checkpoint.currentIndex = checkpoint;
	driver->botData.ai_quadblock_checkpointIndex = checkpoint;
	driver->distanceToFinish_curr = destination->distanceToFinish;
	driver->distanceToFinish_checkpoint = destination->distanceToFinish;
	driver->distanceDrivenBackwards = 0;
	driver->underDriver = NULL;
	driver->currBlockTouching = NULL;
	for (int axis = 0; axis < 3; axis++)
	{
		driver->instSelf->matrix.t[axis] = CTR_VECTOR_DATA(&driver->posCurr)[axis] >> FRACTIONAL_BITS_8;
	}
}

struct ChaosBotSkip
{
	struct Driver *driver;
	struct ChaosTrackDestination destination;
	struct NavFrame *next;
	int speed;
	s64 nextCrossingDistance;
};

static void Chaos_PrepareBotSkip(struct ChaosBotSkip *skip, struct Driver *driver, int length)
{
	if (driver == NULL || (driver->actionsFlagSet & (ACTION_BOT | ACTION_RACE_FINISHED)) != ACTION_BOT)
	{
		return;
	}
	const int speed = Chaos_SpeedForDriver(driver, driver->botData.aiPhysics.speedLinear);
	if (speed <= 0)
	{
		return;
	}
	const int before = driver->distanceToFinish_curr % length;
	const s64 distance = (s64)speed * CHAOS_TIME_SKIP_MS / CHAOS_BOT_DISTANCE_TIME_SCALE;
	const int target = (before - distance % length + length) % length;
	if (!Chaos_FindTrackDestination(driver, target, &skip->destination))
	{
		return;
	}
	skip->next = BOTS_FindClosestNavFrame(driver->botData.botPath, &skip->destination.pos);
	if (skip->next != NULL)
	{
		skip->driver = driver;
		skip->speed = speed;
		skip->nextCrossingDistance = before;
	}
}

static void Chaos_SkipBotTime(void)
{
	struct GameTracker *tracker = GAME_TRACKER;
	const int startTime = tracker->elapsedEventTime;
	const int length = Chaos_GetTrackLength(tracker->level1);
	if (length == 0)
	{
		return;
	}
	struct ChaosBotSkip skips[LOAD_CHARACTER_ID_COUNT] = {0};
	for (int slot = tracker->numPlyrCurrGame; slot < (int)len(tracker->drivers); slot++)
	{
		Chaos_PrepareBotSkip(&skips[slot], tracker->drivers[slot], length);
	}
	// Dispatch crossings chronologically so finish order and clocks use skip time.
	for (;;)
	{
		int first = -1;
		s64 firstMS = CHAOS_TIME_SKIP_MS + 1;
		for (int slot = 0; slot < (int)len(skips); slot++)
		{
			if (skips[slot].driver == NULL || (skips[slot].driver->actionsFlagSet & ACTION_RACE_FINISHED) != 0)
			{
				continue;
			}
			const s64 crossingMS = (skips[slot].nextCrossingDistance * CHAOS_BOT_DISTANCE_TIME_SCALE +
			    skips[slot].speed - 1) / skips[slot].speed;
			if (crossingMS < firstMS)
			{
				first = slot;
				firstMS = crossingMS;
			}
		}
		if (first < 0)
		{
			break;
		}
		tracker->elapsedEventTime = startTime + firstMS;
		skips[first].driver->timeElapsedInRace = tracker->elapsedEventTime;
		PlayLevel_CrossFinishLine(skips[first].driver, false, false);
		skips[first].nextCrossingDistance += length;
	}
	for (int slot = 0; slot < (int)len(skips); slot++)
	{
		if (skips[slot].driver != NULL)
		{
			Chaos_MoveSkippedBot(skips[slot].driver, &skips[slot].destination, skips[slot].next);
		}
	}
	tracker->elapsedEventTime = startTime + CHAOS_TIME_SKIP_MS;
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

static void Chaos_RequestOffroadRescue(struct Driver *driver)
{
	const struct Terrain *terrain = VehAfterColl_GetTerrain(driver->currBlockTouching->terrain_type);
	if (terrain->speedMultiplier < VEH_PHYS_FORCE_TERRAIN_SCALE_NEUTRAL)
	{
		driver->collisionFlags |= DRIVER_COLL_FLAG_MASK_GRAB_REQUEST;
	}
}

static void Chaos_ExplodeCrate(struct Driver *driver, struct Instance *crateInst)
{
	RB_Burst_InitVisual(crateInst);
	PlaySound3D(HOWL_SFX_TNT_EXPLOSION, crateInst);
	VehPickState_ApplyDamage(driver, VEH_PICK_DAMAGE_BLAST, NULL, 0, true);
}

static int Chaos_InvertSteering(const struct GamepadBuffer *pad)
{
	const int center = pad->rwd != NULL ? pad->rwd->gamepadCenter : CHAOS_STICK_CENTER;
	// Keep 256 signed when mirroring the local input.
	return 2 * center - pad->stickLX;
}

static void Chaos_DisableBraking(struct GamepadBuffer *pad)
{
	pad->buttonsHeldCurrFrame &= ~BTN_SQUARE_one;
	if (pad->stickRY > CHAOS_STICK_CENTER)
	{
		pad->stickRY = CHAOS_STICK_CENTER;
	}
	if (pad->stickLY > CHAOS_STICK_CENTER)
	{
		pad->stickLY = CHAOS_STICK_CENTER;
	}
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

static int Chaos_WobbleSteering(const struct ChaosActivation *entry, const struct GamepadBuffer *pad)
{
	const int center = pad->rwd != NULL ? pad->rwd->gamepadCenter : CHAOS_STICK_CENTER;
	const int range = pad->rwd != NULL ? pad->rwd->range : CHAOS_STICK_RANGE;
	if (range <= 0)
	{
		return pad->stickLX;
	}
	const int amplitude = range * CHAOS_WOBBLE_RANGE_NUMERATOR / CHAOS_WOBBLE_RANGE_DENOMINATOR;
	const u32 angle = (entry->elapsedMS % CHAOS_WOBBLE_PERIOD_MS) * ANG_TWO_PI / CHAOS_WOBBLE_PERIOD_MS;
	int steering = pad->stickLX + MATH_Sin(angle) * amplitude / FP(1.0);
	if (steering < center - range)
	{
		steering = center - range;
	}
	else if (steering > center + range)
	{
		steering = center + range;
	}
	return steering;
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
