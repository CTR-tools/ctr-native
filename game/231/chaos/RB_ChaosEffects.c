#include <common.h>
#include <game/Vehicle/VehPhysForce.h>
#include "RB_ChaosAudio.h"
#include "RB_ChaosEffects.h"
#include "RB_ChaosItems.h"
#include "RB_ChaosRuntime.h"

// Effect operations only; RB_Chaos.c checks activation and driver eligibility.

enum
{
	CHAOS_KART_SCALE_NORMAL = 1,
	CHAOS_GIANT_SCALE = 2,
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
