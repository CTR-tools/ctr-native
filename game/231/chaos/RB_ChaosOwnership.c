#include <common.h>

// Attribution lives with the pools; births clear it before reusing a slot.
bool Chaos_IsHumanThread(const struct Thread *thread)
{
	if (thread == NULL || thread->modelIndex != DYNAMIC_PLAYER || thread->object == NULL)
	{
		return false;
	}
	const struct Driver *driver = thread->object;
	return driver->driverID < GAME_TRACKER->numPlyrCurrGame && (driver->actionsFlagSet & ACTION_BOT) == 0;
}

static u8 *Chaos_AllocatePoolOwners(const struct JitPool *pool)
{
	if (pool->maxItems <= 0)
	{
		return NULL;
	}
	u8 *owners = MEMPACK_AllocMem(pool->maxItems, NULL);
	memset(owners, 0, pool->maxItems);
	return owners;
}

static u8 *Chaos_PoolOwnerSlot(const struct JitPool *pool, u8 *owners, const void *item, size_t itemSize)
{
	if (item == NULL || pool->ptrPoolData == NULL || owners == NULL ||
	    pool->maxItems <= 0 || pool->itemSize != itemSize)
	{
		return NULL;
	}
	const uintptr_t address = (uintptr_t)item;
	const uintptr_t base = (uintptr_t)pool->ptrPoolData;
	if (address < base)
	{
		return NULL;
	}
	const uintptr_t offset = address - base;
	const uintptr_t index = offset / pool->itemSize;
	if (offset % pool->itemSize != 0 || index >= (uintptr_t)pool->maxItems)
	{
		return NULL;
	}
	return &owners[index];
}

static u8 *Chaos_ThreadOwnerSlot(const struct Thread *thread)
{
	return Chaos_PoolOwnerSlot(&GAME_TRACKER->JitPools.thread, sdata->chaosThreadOwners, thread, sizeof(*thread));
}

void Chaos_InitThreadOwners(void)
{
	sdata->chaosThreadOwners = Chaos_AllocatePoolOwners(&GAME_TRACKER->JitPools.thread);
}

void Chaos_ClearThreadOwner(struct Thread *thread)
{
	u8 *owner = Chaos_ThreadOwnerSlot(thread);
	if (owner != NULL)
	{
		*owner = 0;
	}
}

int Chaos_ThreadPlayer(const struct Thread *thread)
{
	if (thread == NULL)
	{
		return -1;
	}
	if (Chaos_IsHumanThread(thread))
	{
		return ((const struct Driver *)thread->object)->driverID;
	}
	const u8 *ownerSlot = Chaos_ThreadOwnerSlot(thread);
	if ((thread->flags & THREAD_FLAG_HUMAN_EFFECT) != 0 && ownerSlot != NULL &&
	    *ownerSlot > 0 && *ownerSlot <= CHAOS_MAX_HUMAN_PLAYERS)
	{
		return *ownerSlot - 1;
	}
	const int bucket = thread->flags & 0xff;
	if (thread->object != NULL && thread->funcThTick == VehTurbo_ThTick && bucket == TURBO)
	{
		const struct Driver *driver = ((const struct Turbo *)thread->object)->driver;
		return driver != NULL && driver->instSelf != NULL && Chaos_IsHumanThread(driver->instSelf->thread)
		    ? driver->driverID : -1;
	}
	if (thread->object != NULL && (bucket == MINE || bucket == TRACKING))
	{
		const struct Instance *owner = bucket == MINE ? ((const struct MineWeapon *)thread->object)->instParent :
		    ((const struct TrackerWeapon *)thread->object)->instParent;
		// Level mines also carry a driver pointer; they belong to the world.
		if (thread->inst == NULL || thread->inst->instDef != NULL || owner == NULL)
		{
			return -1;
		}
		// Pooled weapon payloads may still contain the previous allocation's owner during birth.
		for (int i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
		{
			const struct Driver *driver = GAME_TRACKER->drivers[i];
			if (driver != NULL && owner == driver->instSelf && (driver->actionsFlagSet & ACTION_BOT) == 0)
			{
				return i;
			}
		}
		return -1;
	}
	return Chaos_IsHumanThread(thread->parentThread) ? ((const struct Driver *)thread->parentThread->object)->driverID : -1;
}

bool Chaos_IsHumanOwnedThread(const struct Thread *thread)
{
	return thread != NULL &&
	       ((thread->flags & THREAD_FLAG_HUMAN_EFFECT) != 0 || Chaos_ThreadPlayer(thread) >= 0);
}

void Chaos_InheritThreadOwner(struct Thread *thread, const struct Thread *source)
{
	const int player = Chaos_ThreadPlayer(source);
	const bool human = Chaos_IsHumanOwnedThread(source);
	u8 *owner = Chaos_ThreadOwnerSlot(thread);
	if (owner != NULL)
	{
		*owner = player + 1;
	}
	thread->flags &= ~THREAD_FLAG_HUMAN_EFFECT;
	if (human)
	{
		thread->flags |= THREAD_FLAG_HUMAN_EFFECT;
	}
}

void Chaos_InitParticleOwners(void)
{
	sdata->chaosParticleOwners = Chaos_AllocatePoolOwners(&GAME_TRACKER->JitPools.particle);
}

static u8 *Chaos_ParticleOwnerSlot(const struct Particle *particle)
{
	return Chaos_PoolOwnerSlot(&GAME_TRACKER->JitPools.particle, sdata->chaosParticleOwners, particle, sizeof(*particle));
}

void Chaos_ClearParticleOwner(struct Particle *particle)
{
	u8 *owner = Chaos_ParticleOwnerSlot(particle);
	if (owner != NULL)
	{
		*owner = 0;
	}
}

int Chaos_ParticlePlayer(const struct Particle *particle)
{
	const u8 *owner = Chaos_ParticleOwnerSlot(particle);
	return owner != NULL && *owner > 0 && *owner <= CHAOS_MAX_HUMAN_PLAYERS ? *owner - 1 : -1;
}

void Chaos_InheritParticleOwner(struct Particle *particle, const struct Thread *source)
{
	u8 *owner = Chaos_ParticleOwnerSlot(particle);
	if (owner != NULL)
	{
		*owner = Chaos_ThreadPlayer(source) + 1;
	}
	particle->flagsAxis &= ~PARTICLE_AXIS_FLAG_HUMAN_EFFECT;
	if (Chaos_IsHumanOwnedThread(source))
	{
		particle->flagsAxis |= PARTICLE_AXIS_FLAG_HUMAN_EFFECT;
	}
}
