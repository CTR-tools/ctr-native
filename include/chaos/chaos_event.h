#ifndef CTR_NATIVE_CHAOS_EVENT_H
#define CTR_NATIVE_CHAOS_EVENT_H

#include <race_field.h>

// Shared by setup (prospective event) and runtime (current race).
static inline bool ChaosEvent_EffectFits(ChaosEffectId effect, int gameMode1, s32 cupID,
                                       int players, bool extendedMultiplayer, bool personal)
{
	const ChaosEffectDef *def = ChaosEffect_Def(effect);
	if (def == NULL)
	{
		return false;
	}
	if (effect == CHAOS_EFFECT_BODY_SWAP && players < 2)
	{
		return personal &&
		       RaceField_BotCount(gameMode1, cupID, players, extendedMultiplayer) > 0;
	}
	if (effect == CHAOS_EFFECT_YARDRAT_TRAINING || effect == CHAOS_EFFECT_DEFECTIVE_TELEPORT)
	{
		return (gameMode1 & (BATTLE_MODE | CRYSTAL_CHALLENGE)) == 0;
	}
	return def->scope != CHAOS_SCOPE_BOTS || RaceField_BotCount(gameMode1, cupID, players, extendedMultiplayer) > 0;
}

static inline bool ChaosEvent_EffectFitsSelection(ChaosEffectId effect, int gameMode1, s32 cupID,
                                                int players, bool extendedMultiplayer, bool personal,
                                                const bool *disabledPlayers)
{
	if (!ChaosEvent_EffectFits(effect, gameMode1, cupID, players, extendedMultiplayer, personal))
	{
		return false;
	}
	if (effect != CHAOS_EFFECT_BODY_SWAP)
	{
		return true;
	}
	int participants = 0;
	for (int player = 0; player < players && player < CHAOS_MAX_HUMAN_PLAYERS; player++)
	{
		participants += !disabledPlayers[player];
	}
	return participants >= 2 || (participants == 1 &&
	       RaceField_BotCount(gameMode1, cupID, players, extendedMultiplayer) > 0);
}

#endif
