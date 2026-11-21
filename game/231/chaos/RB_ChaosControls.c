#include <common.h>
#include "RB_ChaosControls.h"
#include "RB_ChaosRuntime.h"

enum
{
	CHAOS_STICK_CENTER = 0x80,
	CHAOS_STICK_RANGE = 0x80,
	CHAOS_WOBBLE_RANGE_NUMERATOR = 3,
	CHAOS_WOBBLE_RANGE_DENOMINATOR = 4,
	CHAOS_ANALOG_ACTION_DEAD_ZONE = 0x30,
};

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

void Chaos_UpdateControlAxes(void)
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

void Chaos_ShuffleControls(struct ChaosControlShuffle *controls, const struct GamepadBuffer *pad)
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

static void Chaos_ApplyPlayerInputRules(int player, const struct Driver *driver, struct GamepadBuffer *pad)
{
	const struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
	for (int i = 0; i < pool->numActive; i++)
	{
		const ChaosEffectId effect = pool->active[i].effect;
		if (!Chaos_ActivationTargetsDriver(effect, player, driver))
		{
			continue;
		}
		if (effect == CHAOS_EFFECT_MIRROR_MIRROR)
		{
			pad->stickLX = Chaos_InvertSteering(pad);
		}
		else if (effect == CHAOS_EFFECT_TOTALLY_SOBER)
		{
			pad->stickLX = Chaos_WobbleSteering(&pool->active[i], pad);
		}
		else if (effect == CHAOS_EFFECT_SABOTAGE)
		{
			Chaos_DisableBraking(pad);
		}
	}
}

void Chaos_ApplyInputRules(const struct Driver *driver, struct GamepadBuffer *pad)
{
	if (!Chaos_IsRunning(sdata->gGT))
	{
		return;
	}
	// Remap physical controls before filtering the resulting gameplay actions.
	if (Chaos_IsActiveForDriver(CHAOS_EFFECT_WHO_BOUND_THIS, driver) &&
	    driver->driverID < (int)len(sdata->chaosRuntime.controls))
	{
		Chaos_RemapControls(&sdata->chaosRuntime.controls[driver->driverID], pad);
	}
	for (int player = 0; player < Chaos_PlayerPoolCount(); player++)
	{
		Chaos_ApplyPlayerInputRules(player, driver, pad);
	}
}

