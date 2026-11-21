#include <common.h>
#include <chaos/chaos_event.h>
#include <chaos/chaos_gate.h>
#include <platform/chaos/native_chaos_ipc.h>
#include <platform/native_log.h>
#include "RB_ChaosExternal.h"
#include "RB_ChaosRuntime.h"

void Chaos_PublishEffects(void)
{
	const struct ChaosSettings *settings = &sdata->chaosSettings;
	char catalogue[NATIVE_CHAOS_CATALOGUE_SIZE] = {0};
	size_t used = 0;
	const struct GameTracker *gGT = sdata->gGT;
	const bool eligibleRace = gGT != NULL && !gGT->boolDemoMode && !Chaos_UsesPersonalPools() &&
	    ChaosGate_IsRaceEligible(gGT->gameMode1 & ~(PAUSE_ALL | LOADING),
	                             gGT->overlayIndex_Threads, true) && Chaos_HasEnabledPlayer();
	for (int i = 0; settings->armed && eligibleRace && i < CHAOS_EFFECT_COUNT; i++)
	{
		if (settings->effects[i] && ChaosEvent_EffectFitsSelection((ChaosEffectId)i, gGT->gameMode1, gGT->cup.cupID,
		                                                        gGT->numPlyrCurrGame, g_config.extendedMultiplayer,
		                                                        Chaos_UsesPersonalPools(), settings->disabledPlayers))
		{
			const char *slug = ChaosEffect_Def((ChaosEffectId)i)->slug;
			const size_t length = strlen(slug);
			if (length + 1 >= sizeof(catalogue) - used)
			{
				Platform_LogError("[Chaos] Effects catalogue exceeds publication capacity.\n");
				return;
			}
			memcpy(catalogue + used, slug, length);
			used += length;
			catalogue[used++] = '\n';
		}
	}
	NativeChaosIPC_WriteEffectsCatalogue(catalogue);
}

static bool Chaos_ActivateExternalEffect(const char *slug)
{
	const struct ChaosSettings *settings = &sdata->chaosSettings;
	const ChaosEffectId effect = ChaosEffect_FromSlug(slug);
	if (effect == CHAOS_EFFECT_NONE)
	{
		Platform_LogError("[Chaos] Consumed invalid or unknown effect command.\n");
		return false;
	}
	if (!ChaosEffect_IsEnabled(settings->effects, effect))
	{
		Platform_LogError("[Chaos] Consumed disabled effect: %s\n", slug);
		return false;
	}
	if (!Chaos_Activate(effect, settings->durationSeconds * CHAOS_MILLISECONDS_PER_SECOND))
	{
		Platform_LogError("[Chaos] Consumed effect rejected by runtime: %s\n", slug);
		return false;
	}
	return true;
}

bool Chaos_PollExternal(void)
{
	char command[NATIVE_CHAOS_COMMAND_SIZE];
	return NativeChaosIPC_ReadCommand(command, sizeof(command)) == NATIVE_CHAOS_COMMAND &&
	    Chaos_ActivateExternalEffect(command);
}
