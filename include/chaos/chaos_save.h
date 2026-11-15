#ifndef CTR_NATIVE_CHAOS_SAVE_H
#define CTR_NATIVE_CHAOS_SAVE_H

#include <namespace_Memcard.h>

static inline bool ChaosSave_IsArmed(const struct ChaosSettingsSaved *saved)
{
	return saved->format == CHAOS_SAVE_FORMAT_V1 && saved->armed != 0;
}

static inline void ChaosSave_Decode(const struct ChaosSettingsSaved *saved, struct ChaosSettings *settings)
{
	memset(settings, 0, sizeof(*settings));
	if (saved->format != CHAOS_SAVE_FORMAT_V1)
	{
		return;
	}

	settings->durationSeconds = saved->durationSeconds;
	settings->triggerIntervalSeconds = saved->triggerIntervalSeconds;
	settings->trigger = saved->trigger < CHAOS_TRIGGER_COUNT ? saved->trigger : CHAOS_TRIGGER_RANDOM;
	settings->armed = ChaosSave_IsArmed(saved);
	for (int id = 0; id < CHAOS_EFFECT_COUNT; id++)
	{
		settings->effects[id] = (saved->effectBits[id / 8] & (1u << (id % 8))) != 0;
	}
	settings->enabledCount = (u8)ChaosEffect_CountEnabled(settings->effects);
}

static inline void ChaosSave_Encode(const struct ChaosSettings *settings, struct ChaosSettingsSaved *saved)
{
	// Clear first so every unassigned bit stays disabled, even over an old slot.
	memset(saved, 0, sizeof(*saved));
	saved->durationSeconds = settings->durationSeconds;
	saved->triggerIntervalSeconds = settings->triggerIntervalSeconds;
	saved->trigger = settings->trigger;
	saved->armed = settings->armed;
	saved->format = CHAOS_SAVE_FORMAT_V1;
	for (int id = 0; id < CHAOS_EFFECT_COUNT; id++)
	{
		if (settings->effects[id])
		{
			saved->effectBits[id / 8] |= (u8)(1u << (id % 8));
		}
	}
}

#endif
