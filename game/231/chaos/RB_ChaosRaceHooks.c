#include <common.h>
#include <game/Vehicle/VehPhysForce.h>
#include "RB_ChaosItems.h"
#include "RB_ChaosRuntime.h"
#include "RB_ChaosScheduler.h"

enum
{
	CHAOS_PERCENT_SCALE = 100,
	CHAOS_LOW_GRAVITY_DIVISOR = 4,
	CHAOS_HEAVY_GRAVITY_MULTIPLIER = 3,
	CHAOS_SUNDAY_DRIVER_SPEED_PERCENT = 70,
	CHAOS_ARENA_CHAMPIONS_SPEED_PERCENT = 175,
	CHAOS_SUPER_SPEED_MULTIPLIER = 2,
	CHAOS_SMALL_FORCE_PERCENT = 70,
	CHAOS_GIANT_SPEED_PERCENT = 130,
};

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

int Chaos_GravityForDriver(const struct Driver *driver)
{
	const int gravity = driver->const_Gravity;
	if (Chaos_IsActiveForDriver(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, driver))
	{
		return Chaos_LowGravity(gravity);
	}
	if (Chaos_IsActiveForDriver(CHAOS_EFFECT_HEAVY_METAL, driver))
	{
		return Chaos_HeavyGravity(gravity);
	}
	return gravity;
}

int Chaos_SpeedForDriver(const struct Driver *driver, int speed)
{
	if (speed <= 0)
	{
		return speed;
	}
	if (Chaos_IsActiveForDriver(CHAOS_EFFECT_BROWN_FIRE, driver))
	{
		speed = Chaos_DefectiveBoostSpeed(driver, speed);
	}
	if (Chaos_IsActiveForDriver(CHAOS_EFFECT_BIG_KART_ENERGY, driver))
	{
		speed = Chaos_PercentOf(speed, CHAOS_GIANT_SPEED_PERCENT);
	}
	if (Chaos_IsActiveForDriver(CHAOS_EFFECT_SMALL_KART_ENERGY, driver))
	{
		speed = Chaos_PercentOf(speed, CHAOS_SMALL_FORCE_PERCENT);
	}
	if (Chaos_IsActiveForDriver(CHAOS_EFFECT_SUNDAY_DRIVER, driver))
	{
		return Chaos_PercentOf(speed, CHAOS_SUNDAY_DRIVER_SPEED_PERCENT);
	}
	if (Chaos_IsActiveForDriver(CHAOS_EFFECT_GOTTA_GO_FAST, driver))
	{
		speed = Chaos_SuperSpeed(speed);
	}
	if (Chaos_IsActiveForDriver(CHAOS_EFFECT_ARENA_CHAMPIONS, driver))
	{
		speed = Chaos_PercentOf(speed, CHAOS_ARENA_CHAMPIONS_SPEED_PERCENT);
	}
	return speed;
}

int Chaos_JumpForceForDriver(const struct Driver *driver, int force)
{
	if (Chaos_IsActiveForDriver(CHAOS_EFFECT_BIG_KART_ENERGY, driver))
	{
		return Chaos_PercentOf(force, CHAOS_GIANT_SPEED_PERCENT);
	}
	if (Chaos_IsActiveForDriver(CHAOS_EFFECT_SMALL_KART_ENERGY, driver))
	{
		return Chaos_PercentOf(force, CHAOS_SMALL_FORCE_PERCENT);
	}
	return force;
}

int Chaos_AccelerationForDriver(const struct Driver *driver, int acceleration)
{
	if (driver->baseSpeed > 0 && acceleration > 0 &&
	    Chaos_IsActiveForDriver(CHAOS_EFFECT_GOTTA_GO_FAST, driver))
	{
		return Chaos_SuperSpeed(acceleration);
	}
	return acceleration;
}

int Chaos_ReservesForDriver(const struct Driver *driver, int reserves)
{
	if (Chaos_IsActiveForDriver(CHAOS_EFFECT_TOO_BOOSTED_TO_FAIL, driver))
	{
		reserves = Chaos_KeepReserves(driver, reserves);
	}
	return reserves;
}

int Chaos_FireLevelForDriver(const struct Driver *driver, int fireLevel)
{
	return Chaos_IsActiveForDriver(CHAOS_EFFECT_THIS_IS_FIRE, driver) ? VEH_PHYS_FORCE_SUPER_TURBO_PAD_FIRE_LEVEL : fireLevel;
}

void Chaos_ApplyRescueRules(struct Driver *driver)
{
	if (Chaos_IsActiveForDriver(CHAOS_EFFECT_ANXIOUS_MASK, driver))
	{
		VehPhysForce_RequestOffroadRescue(driver);
	}
}

void Chaos_ApplyCrateRules(struct Driver *driver, struct Instance *crateInst)
{
	if (Chaos_IsActiveForDriver(CHAOS_EFFECT_UNBOXING_GONE_WRONG, driver))
	{
		Chaos_ExplodeCrate(driver, crateInst);
	}
}

bool Chaos_IsHardCrateForDriver(const struct Instance *crateInst, const struct Thread *collidingTh,
                               const struct ScratchpadStruct *sps)
{
	return crateInst->scale.x == 0x1000 && sps->Input1.modelID == DYNAMIC_PLAYER &&
	       Chaos_IsActiveForDriver(CHAOS_EFFECT_UNBREAKABLE, collidingTh->object);
}

void Chaos_ApplyItemRules(struct Driver *driver)
{
	if (Chaos_IsActiveForDriver(CHAOS_EFFECT_FAIR_MODE, driver))
	{
		Chaos_ClearItems(driver);
	}
}

bool Chaos_OnPickup(struct Driver *driver)
{
	const struct ChaosSettings *settings = &sdata->chaosSettings;
	if (settings->trigger != CHAOS_TRIGGER_PICKUP || !Chaos_IsRunning(sdata->gGT) ||
	    driver == NULL || (driver->actionsFlagSet & (ACTION_RACE_FINISHED | ACTION_BOT)) != 0 ||
	    !Chaos_IsPlayerEnabled(driver->driverID) ||
	    !ChaosEffect_AppliesToDriver(CHAOS_SCOPE_HUMANS, driver->driverID, sdata->gGT->numPlyrCurrGame) ||
	    settings->durationSeconds == 0)
	{
		return false;
	}
	return Chaos_TriggerForPlayer(Chaos_UsesPersonalPools() ? driver->driverID : 0);
}

void Chaos_OnTurboPadContact(struct Driver *driver, bool touching)
{
	if (sdata->chaosSettings.trigger != CHAOS_TRIGGER_PICKUP || !Chaos_IsRunning(sdata->gGT) || driver == NULL ||
	    !ChaosEffect_AppliesToDriver(CHAOS_SCOPE_HUMANS, driver->driverID, sdata->gGT->numPlyrCurrGame) ||
	    driver->driverID >= (int)len(sdata->chaosRuntime.touchingTurboPad))
	{
		return;
	}
	bool *wasTouching = &sdata->chaosRuntime.touchingTurboPad[driver->driverID];
	const bool entered = touching && !*wasTouching;
	*wasTouching = touching;
	if (entered)
	{
		Chaos_OnPickup(driver);
	}
}

static u32 Chaos_ApplyPlayerDriver(int player, struct Driver *driver, u32 actions)
{
	const struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
	for (int i = 0; i < pool->numActive; i++)
	{
		const ChaosEffectId effect = pool->active[i].effect;
		if (!Chaos_ActivationTargetsDriver(effect, player, driver))
		{
			continue;
		}
		if (effect == CHAOS_EFFECT_GROUNDED)
		{
			actions = Chaos_PreventHopping(driver, actions);
		}
		else if (effect == CHAOS_EFFECT_HYDRAULICS)
		{
			if ((driver->kartState == KS_NORMAL || driver->kartState == KS_DRIFTING || driver->kartState == KS_ANTIVSHIFT) &&
			    driver->rainCloudEffect != RAIN_CLOUD_EFFECT_JUMP_LOCKOUT)
			{
				Chaos_RequestHop(driver);
			}
		}
		else if (effect == CHAOS_EFFECT_HOLE_IN_THE_TANK)
		{
			Chaos_ClearReserves(driver);
		}
		else if (effect == CHAOS_EFFECT_WUMPA_TAX && !Chaos_IsTimeStopped())
		{
			Chaos_TickWumpaDrain(pool->active[i].elapsedMS, driver);
		}
	}
	return actions;
}

u32 Chaos_ApplyDriver(struct Driver *driver, u32 actions)
{
	if (!Chaos_IsRunning(sdata->gGT))
	{
		return actions;
	}
	for (int player = 0; player < Chaos_PlayerPoolCount(); player++)
	{
		actions = Chaos_ApplyPlayerDriver(player, driver, actions);
	}
	return actions;
}
