#include <common.h>
#include <chaos/chaos_event.h>
#include "RB_ChaosExternal.h"
#include "RB_ChaosRuntime.h"
#include "RB_ChaosScheduler.h"

enum
{
	CHAOS_RANDOM_GAP_MIN_PERCENT = 30,
	CHAOS_RANDOM_GAP_PERCENT_SCALE = 100,
};

static ChaosEffectId Chaos_PickEventEffect(void)
{
	const struct ChaosSettings *settings = &sdata->chaosSettings;
	const struct GameTracker *gGT = sdata->gGT;
	bool effects[CHAOS_EFFECT_COUNT];
	memcpy(effects, settings->effects, sizeof(effects));
	int enabledCount = settings->enabledCount;
	for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
	{
		if (effects[i] && !ChaosEvent_EffectFitsSelection((ChaosEffectId)i, gGT->gameMode1, gGT->cup.cupID,
		                                                gGT->numPlyrCurrGame, g_config.extendedMultiplayer,
		                                                Chaos_UsesPersonalPools(), settings->disabledPlayers))
		{
			effects[i] = false;
			enabledCount--;
		}
	}
	return ChaosEffect_Pick(effects, enabledCount, &sdata->chaosRng);
}

int Chaos_NextTriggerGap(void)
{
	const struct ChaosSettings *settings = &sdata->chaosSettings;
	const s32 intervalMS = settings->triggerIntervalSeconds * CHAOS_MILLISECONDS_PER_SECOND;
	if (settings->trigger != CHAOS_TRIGGER_RANDOM || intervalMS == 0 ||
	    settings->durationSeconds == 0 || settings->enabledCount == 0)
	{
		return intervalMS;
	}
	const s32 minMS = intervalMS * CHAOS_RANDOM_GAP_MIN_PERCENT / CHAOS_RANDOM_GAP_PERCENT_SCALE;
	const u32 sample = ChaosEffect_NextRandom(&sdata->chaosRng);
	return minMS + (s32)((u64)sample * (intervalMS - minMS) / CHAOS_RANDOM_SAMPLE_MAX);
}

bool Chaos_TriggerForPlayer(int player)
{
	const struct ChaosSettings *settings = &sdata->chaosSettings;
	const s32 durationMS = settings->durationSeconds * CHAOS_MILLISECONDS_PER_SECOND;
	if (durationMS <= 0)
	{
		return false;
	}
	if (!Chaos_UsesPersonalPools())
	{
		return Chaos_Activate(Chaos_PickEventEffect(), durationMS);
	}
	if (!Chaos_PlayerCanReceive(player) || Chaos_IsTimeStopped())
	{
		return false;
	}
	// Remove rejected candidates rather than retrying the same unavailable
	// effect indefinitely. Each remaining candidate keeps equal probability.
	bool candidates[CHAOS_EFFECT_COUNT];
	memcpy(candidates, settings->effects, sizeof(candidates));
	int count = ChaosEffect_CountEnabled(candidates);
	while (count > 0)
	{
		const ChaosEffectId effect = ChaosEffect_Pick(candidates, count, &sdata->chaosRng);
		if (Chaos_ActivateForPlayer(effect, durationMS, player))
		{
			return true;
		}
		candidates[effect] = false;
		count--;
	}
	return false;
}

static void Chaos_UpdateTriggerClock(int player, s32 elapsedMS, bool commonCadence)
{
	struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
	pool->intervalRemainingMS -= elapsedMS;
	while (pool->intervalRemainingMS <= 0)
	{
		const int count = commonCadence ? Chaos_PlayerPoolCount() : player + 1;
		for (int recipient = player; recipient < count; recipient++)
		{
			Chaos_TriggerForPlayer(recipient);
		}
		pool->intervalRemainingMS += Chaos_NextTriggerGap();
		if (Chaos_UsesPersonalPools() && Chaos_IsTimeStopped())
		{
			break;
		}
	}
}

void Chaos_UpdateSchedule(int elapsedMS)
{
	struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[0];
	const struct ChaosSettings *settings = &sdata->chaosSettings;
	if (settings->trigger == CHAOS_TRIGGER_EXTERNAL)
	{
		if (Chaos_UsesPersonalPools())
		{
			return;
		}
		if (pool->intervalRemainingMS > 0)
		{
			pool->intervalRemainingMS -= elapsedMS;
			if (pool->intervalRemainingMS < 0)
			{
				pool->intervalRemainingMS = 0;
			}
		}
		if (sdata->chaosRuntime.players[0].intervalRemainingMS <= 0 && Chaos_PollExternal())
		{
			sdata->chaosRuntime.players[0].intervalRemainingMS = Chaos_NextTriggerGap();
		}
		return;
	}

	if ((settings->trigger != CHAOS_TRIGGER_INTERVAL && settings->trigger != CHAOS_TRIGGER_RANDOM) ||
	    settings->triggerIntervalSeconds == 0)
	{
		return;
	}
	if (settings->trigger == CHAOS_TRIGGER_INTERVAL)
	{
		Chaos_UpdateTriggerClock(0, elapsedMS, true);
		// In Interval the first clock is authoritative, even if its player has
		// finished. Mirror it so every pool carries the same cadence.
		for (int player = 1; player < Chaos_PlayerPoolCount(); player++)
		{
			sdata->chaosRuntime.players[player].intervalRemainingMS = pool->intervalRemainingMS;
		}
		return;
	}
	for (int player = 0; player < Chaos_PlayerPoolCount(); player++)
	{
		if (!Chaos_UsesPersonalPools() || Chaos_PlayerCanReceive(player))
		{
			Chaos_UpdateTriggerClock(player, elapsedMS, false);
		}
		if (Chaos_IsTimeStopped())
		{
			break;
		}
	}
}
