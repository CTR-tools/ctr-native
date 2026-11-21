#include <common.h>
#include <game/Vehicle/VehPhysForce.h>
#include "RB_ChaosAudio.h"
#include "RB_ChaosEffects.h"
#include "RB_ChaosItems.h"
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
	CHAOS_LAUNCH_JUMP_MULTIPLIER = 6,
	CHAOS_WRONG_WAY_CADENCE_MULTIPLIER = 2,
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

void Chaos_ResetKartScale(struct Driver *driver)
{
	Chaos_SetKartScale(CHAOS_KART_SCALE_NORMAL, driver);
}

void Chaos_SetDrivingClass(struct Driver *driver, int next)
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

void Chaos_RefreshBodySwapModifiers(void)
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
