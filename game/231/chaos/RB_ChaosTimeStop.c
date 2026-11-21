#include <common.h>
#include <game/Vehicle/VehPhysForce.h>
#include "../RB_Collision.h"

enum { CHAOS_POTION_HIT_RADIUS_SQUARED = 0x1900 };

bool Chaos_IsTimeStopImmune(const struct Thread *thread)
{
	return Chaos_UsesPersonalPools() ? Chaos_PlayerHasTimeStop(Chaos_ThreadPlayer(thread)) : Chaos_IsHumanOwnedThread(thread);
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
	return thread != NULL && Chaos_IsTimeStopped() && !Chaos_IsTimeStopImmune(thread);
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
	if (!delayedMineContact && (!Chaos_IsTimeStopImmune(source) ||
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
	    !Chaos_IsTimeStopImmune(attacker->instSelf->thread) || !Chaos_IsFrozenWorldThread(victim->instSelf->thread))
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
	    !Chaos_IsTimeStopImmune(attacker->instSelf->thread))
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
	if (Chaos_IsTimeStopImmune(thread) || bucket == CAMERA || bucket == HUD || bucket == STARTTEXT)
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
	if (!Chaos_IsTimeStopped())
	{
		return false;
	}
	if ((particle->flagsAxis & PARTICLE_AXIS_FLAG_HUMAN_EFFECT) != 0)
	{
		return Chaos_UsesPersonalPools() ? !Chaos_PlayerHasTimeStop(Chaos_ParticlePlayer(particle)) : false;
	}
	for (int i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
	{
		const struct Driver *driver = GAME_TRACKER->drivers[i];
		if (driver != NULL && driver->instSelf != NULL && particle->owner.driverInst == driver->instSelf &&
		    Chaos_IsTimeStopImmune(driver->instSelf->thread))
		{
			return false;
		}
	}
	return true;
}

