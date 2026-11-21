#include <common.h>
#include <game/Vehicle/VehPhysForce.h>
#include <game/Vehicle/VehPickupItem.h>
#include <chaos/chaos_gate.h>
#include <chaos/chaos_event.h>
#include <race_field.h>
#include <platform/native_audio.h>
#include <platform/chaos/native_chaos_ipc.h>
#include <platform/native_log.h>
#include "RB_ChaosAudio.h"
#include "RB_ChaosBodySwap.h"
#include "RB_ChaosControls.h"
#include "RB_ChaosEffects.h"
#include "RB_ChaosExternal.h"
#include "RB_ChaosIdentity.h"
#include "RB_ChaosItems.h"
#include "RB_ChaosReposition.h"
#include "RB_ChaosRestart.h"
#include "RB_ChaosRuntime.h"
#include "RB_ChaosScheduler.h"

enum
{
	CHAOS_TIME_STOP_MAX_MS = 10000,
	CHAOS_BOT_COOLDOWN_MS = 30000,
	CHAOS_BOT_EFFECT_COOLDOWN_MS = 60000,
};

bool Chaos_IsPlayerEnabled(int player)
{
	return player >= 0 && player < CHAOS_MAX_HUMAN_PLAYERS &&
	       !sdata->chaosSettings.disabledPlayers[player];
}

bool Chaos_HasEnabledPlayer(void)
{
	for (int player = 0; player < sdata->gGT->numPlyrCurrGame; player++)
	{
		if (Chaos_IsPlayerEnabled(player))
		{
			return true;
		}
	}
	return false;
}

static bool Chaos_TargetsDriver(ChaosEffectId effect, const struct Driver *driver)
{
	const ChaosEffectDef *def = ChaosEffect_Def(effect);
	return def != NULL && driver != NULL &&
	       (driver->actionsFlagSet & ACTION_RACE_FINISHED) == 0 &&
	       (def->scope != CHAOS_SCOPE_HUMANS || Chaos_IsPlayerEnabled(driver->driverID)) &&
	       ChaosEffect_AppliesToDriver(def->scope, driver->driverID, sdata->gGT->numPlyrCurrGame);
}

bool Chaos_UsesPersonalPools(void)
{
	return sdata->chaosSettings.distribution == CHAOS_DISTRIBUTION_PER_PLAYER;
}

int Chaos_PlayerPoolCount(void)
{
	if (!Chaos_UsesPersonalPools())
	{
		return 1;
	}
	const int players = sdata->gGT->numPlyrCurrGame;
	return players < CHAOS_MAX_HUMAN_PLAYERS ? players : CHAOS_MAX_HUMAN_PLAYERS;
}

bool Chaos_PlayerHasTimeStop(int player)
{
	if (player < 0 || player >= Chaos_PlayerPoolCount())
	{
		return false;
	}
	const struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
	for (int i = 0; i < pool->numActive; i++)
	{
		if (pool->active[i].effect == CHAOS_EFFECT_ZA_WARUDO)
		{
			return true;
		}
	}
	return false;
}

bool Chaos_PlayerCanReceive(int player)
{
	if (player < 0 || player >= Chaos_PlayerPoolCount() || !Chaos_IsPlayerEnabled(player))
	{
		return false;
	}
	const struct Driver *driver = sdata->gGT->drivers[player];
	return driver != NULL && (driver->actionsFlagSet & (ACTION_RACE_FINISHED | ACTION_BOT)) == 0;
}

static bool Chaos_BotEffectBlocked(ChaosEffectId effect)
{
	const struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	if (runtime->botCooldownMS > 0 || runtime->botEffectCooldownMS[effect] > 0)
	{
		return true;
	}
	// The global cooldown starts on completion, so no other bot effect may
	// start while a timed one is still running in any player's pool.
	for (int player = 0; player < Chaos_PlayerPoolCount(); player++)
	{
		const struct ChaosPlayerRuntime *pool = &runtime->players[player];
		for (int i = 0; i < pool->numActive; i++)
		{
			if (ChaosEffect_Def(pool->active[i].effect)->scope == CHAOS_SCOPE_BOTS)
			{
				return true;
			}
		}
	}
	return false;
}

static s32 Chaos_TickCooldown(s32 remainingMS, s32 elapsedMS)
{
	return remainingMS > elapsedMS ? remainingMS - elapsedMS : 0;
}

static void Chaos_StartBotCooldown(ChaosEffectId effect, s32 elapsedSinceEndMS)
{
	if (effect < 0 || effect >= CHAOS_EFFECT_COUNT)
	{
		return;
	}
	if (Chaos_UsesPersonalPools() && ChaosEffect_Def(effect)->scope == CHAOS_SCOPE_BOTS)
	{
		sdata->chaosRuntime.botCooldownMS = Chaos_TickCooldown(CHAOS_BOT_COOLDOWN_MS, elapsedSinceEndMS);
		sdata->chaosRuntime.botEffectCooldownMS[effect] = Chaos_TickCooldown(CHAOS_BOT_EFFECT_COOLDOWN_MS, elapsedSinceEndMS);
	}
}

bool Chaos_ActivationTargetsDriver(ChaosEffectId effect, int player, const struct Driver *driver)
{
	if (!Chaos_TargetsDriver(effect, driver))
	{
		return false;
	}
	if (!Chaos_UsesPersonalPools() || ChaosEffect_Def(effect)->scope != CHAOS_SCOPE_HUMANS)
	{
		return true;
	}
	if (effect == CHAOS_EFFECT_BODY_SWAP)
	{
		return driver->driverID == player ||
		       sdata->chaosRuntime.bodySwapPartner[player] == driver->driverID + 1;
	}
	return driver->driverID == player;
}

static bool Chaos_EffectsShareTargets(ChaosEffectId first, int firstPlayer, ChaosEffectId second, int secondPlayer)
{
	return firstPlayer == secondPlayer ||
	       ChaosEffect_Def(first)->scope != CHAOS_SCOPE_HUMANS ||
	       ChaosEffect_Def(second)->scope != CHAOS_SCOPE_HUMANS;
}

bool Chaos_IsRunning(const struct GameTracker *gGT)
{
	return sdata->chaosRuntime.raceInitialized && sdata->chaosSettings.armed && !gGT->boolDemoMode &&
	       sdata->ptrActiveMenu == NULL && sdata->Loading.stage == LOAD_IDLE &&
	       (gGT->gameMode1 & DEBUG_MENU) == 0 &&
	       ChaosGate_IsRaceEligible(gGT->gameMode1, gGT->overlayIndex_Threads,
	                                gGT->trafficLightsTimer < 1);
}

void Chaos_Init(u32 seed)
{
	// Seed once per launch; checkpoints restore the exact captured RNG state.
	sdata->chaosRng = seed;
}

bool Chaos_IsTimeStopped(void)
{
	const struct GameTracker *tracker = GAME_TRACKER;
	const struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	// Pause suspends expiry but retains the frozen scene and audio ownership.
	if (!runtime->raceInitialized || sdata->Loading.stage != LOAD_IDLE ||
	    (tracker->gameMode1 & (END_OF_RACE | MAIN_MENU | GAME_CUTSCENE | ADVENTURE_ARENA | LOADING)) != 0)
	{
		return false;
	}
	for (int player = 0; player < Chaos_PlayerPoolCount(); player++)
	{
		if (Chaos_PlayerHasTimeStop(player))
		{
			return true;
		}
	}
	return false;
}

static void Chaos_RemoveActivation(int player, int index, s32 elapsedSinceEndMS);

static void Chaos_EndPersonalBodySwap(int slot)
{
	const int partner = sdata->chaosRuntime.bodySwapPartner[slot] - 1;
	for (int player = 0; player < Chaos_PlayerPoolCount(); player++)
	{
		if (player != slot && player != partner)
		{
			continue;
		}
		const struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
		for (int i = 0; i < pool->numActive; i++)
		{
			if (pool->active[i].effect == CHAOS_EFFECT_BODY_SWAP)
			{
				Chaos_RemoveActivation(player, i, 0);
				return;
			}
		}
	}
}

void Chaos_ReturnBodySwapsNearFinish(struct GameTracker *tracker, int nearDistance)
{
	if (!sdata->chaosRuntime.raceInitialized || (tracker->gameMode1 & (BATTLE_MODE | CRYSTAL_CHALLENGE)) != 0)
	{
		return;
	}
	bool returned = false;
	for (int slot = 0; slot < (int)len(tracker->drivers); slot++)
	{
		struct Driver *driver = tracker->drivers[slot];
		if (sdata->chaosRuntime.bodySwapPartner[slot] != 0 && driver != NULL &&
		    driver->lapIndex + 1 >= tracker->numLaps &&
		    (driver->distanceToFinish_curr < (u32)nearDistance || sdata->chaosRuntime.teleportForwardCrossings[slot] != 0) &&
		    (driver->actionsFlagSet & ACTION_BEHIND_START_LINE) == 0)
		{
			if (Chaos_UsesPersonalPools())
			{
				// Clear the old deadline as well, before a new pair can form.
				Chaos_EndPersonalBodySwap(slot);
			}
			else
			{
				Chaos_ReturnBodySwap(slot);
			}
			returned = true;
		}
	}
	if (returned)
	{
		Chaos_RefreshBodySwapModifiers();
	}
}

void Chaos_Reset(void)
{
	NativeAudio_SetPausedVoices(0, false);
	if (sdata->chaosRuntime.raceInitialized)
	{
		Chaos_ReturnBodySwaps();
		for (int i = 0; i < (int)len(sdata->gGT->drivers); i++)
		{
			struct Driver *driver = sdata->gGT->drivers[i];
			if (driver != NULL)
			{
				if (i < (int)len(sdata->chaosRuntime.identity) && sdata->chaosRuntime.identity[i].changed)
				{
					const struct ChaosIdentity *identity = &sdata->chaosRuntime.identity[i];
					Chaos_SetCharacterModel(driver, identity->originalCharacter, identity->originalModel);
				}
				if (i < (int)len(sdata->chaosRuntime.restart))
				{
					Chaos_EndRestartHold(&sdata->chaosRuntime.restart[i], driver);
				}
				Chaos_ResetKartScale(driver);
				if (i < (int)len(sdata->chaosRuntime.engineClassChanged) && sdata->chaosRuntime.engineClassChanged[i])
				{
					const int defaultClass = GAME_CHARACTER_METADATA[GAME_CHARACTER_IDS[driver->driverID]].engineID;
					const int original = VehBirth_EngineClassForPlayer(driver->driverID, defaultClass);
					VehBirth_SetConstsForClass(driver, original);
					if (i < sdata->gGT->numPlyrCurrGame)
					{
						Chaos_ChangeEngineAudio(driver, sdata->chaosRuntime.engineClass[i], original);
					}
				}
			}
		}
	}
	memset(&sdata->chaosRuntime, 0, sizeof(sdata->chaosRuntime));
}

void Chaos_LoadCharacterModels(struct GameTracker *gGT, struct BigHeader *bigfile)
{
	const struct ChaosSettings *settings = &sdata->chaosSettings;
	if (!gGT->boolDemoMode && settings->armed && settings->effects[CHAOS_EFFECT_IDENTITY_CRISIS] &&
	    ChaosGate_IsRaceEligible(gGT->gameMode1 & ~(PAUSE_ALL | LOADING),
	                             OVERLAY_INDEX_RACING_OR_BATTLE, true))
	{
		Chaos_PreloadCharacterModels(bigfile);
	}
}

void Chaos_BeginRace(struct GameTracker *gGT)
{
	Chaos_Reset();
	Chaos_PublishEffects();
	// Initialization may precede unpausing; GO gates updates, not race setup.
	if (!sdata->chaosSettings.armed || gGT->boolDemoMode ||
	    !ChaosGate_IsRaceEligible(gGT->gameMode1 & ~(PAUSE_ALL | LOADING),
	                             gGT->overlayIndex_Threads, true))
	{
		return;
	}
	sdata->chaosRuntime.raceInitialized = true;
	for (int player = 0; player < Chaos_PlayerPoolCount(); player++)
	{
		sdata->chaosRuntime.players[player].intervalRemainingMS = Chaos_NextTriggerGap();
	}
}

static void Chaos_OnEffectDeactivated(const struct ChaosActivation *activation, int player, s32 elapsedSinceEndMS)
{
	const ChaosEffectId effect = activation->effect;
	Chaos_StartBotCooldown(effect, elapsedSinceEndMS);
	if (effect == CHAOS_EFFECT_BODY_SWAP)
	{
		if (!Chaos_UsesPersonalPools())
		{
			Chaos_ReturnBodySwaps();
		}
		else if (sdata->chaosRuntime.bodySwapPartner[player] != 0)
		{
			Chaos_ReturnBodySwap(player);
		}
		Chaos_RefreshBodySwapModifiers();
	}
	if (effect != CHAOS_EFFECT_BIG_KART_ENERGY && effect != CHAOS_EFFECT_SMALL_KART_ENERGY)
	{
		return;
	}
	for (int i = 0; i < sdata->gGT->numPlyrCurrGame && i < (int) len(sdata->gGT->drivers); i++)
	{
		struct Driver *driver = sdata->gGT->drivers[i];
		if (driver != NULL && Chaos_IsPlayerEnabled(i) && (!Chaos_UsesPersonalPools() || driver->driverID == player))
		{
			Chaos_ResetKartScale(driver);
		}
	}
}

static void Chaos_RemoveInstantNotice(struct ChaosPlayerRuntime *pool, int index)
{
	struct ChaosActivation *notice = &pool->instantNotices[index];
	memmove(notice, notice + 1, (--pool->numInstantNotices - index) * sizeof(*notice));
	memset(&pool->instantNotices[pool->numInstantNotices], 0, sizeof(*notice));
}

static void Chaos_RemoveActivation(int player, int index, s32 elapsedSinceEndMS)
{
	struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
	const struct ChaosActivation activation = pool->active[index];
	int remaining = pool->numActive - index - 1;
	memmove(&pool->active[index], &pool->active[index + 1], remaining * sizeof(pool->active[0]));
	pool->active[--pool->numActive] = (struct ChaosActivation){0};
	Chaos_OnEffectDeactivated(&activation, player, elapsedSinceEndMS);
}

static void Chaos_RemovePlayerReplacedEffects(ChaosEffectId effect, ChaosEffectId replaced, int player, int otherPlayer)
{
	struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[otherPlayer];
	for (int i = 0; i < pool->numInstantNotices;)
	{
		const ChaosEffectId notice = pool->instantNotices[i].effect;
		if (notice == replaced && Chaos_EffectsShareTargets(notice, otherPlayer, effect, player))
		{
			Chaos_RemoveInstantNotice(pool, i);
		}
		else
		{
			i++;
		}
	}
	for (int i = 0; i < pool->numActive;)
	{
		const ChaosEffectId active = pool->active[i].effect;
		const bool replacesJumpEffect = effect == CHAOS_EFFECT_MY_PLANET_NEEDS_ME &&
		    (active == CHAOS_EFFECT_GROUNDED || active == CHAOS_EFFECT_HYDRAULICS);
		if ((active == effect || active == replaced || replacesJumpEffect) &&
		    Chaos_EffectsShareTargets(active, otherPlayer, effect, player))
		{
			Chaos_RemoveActivation(otherPlayer, i, 0);
		}
		else
		{
			i++;
		}
	}
}

static void Chaos_RemoveReplacedEffects(ChaosEffectId effect, int player)
{
	ChaosEffectId replaced = CHAOS_EFFECT_NONE;
	static const ChaosEffectId opposites[][2] = {
		{CHAOS_EFFECT_MIRROR_MIRROR, CHAOS_EFFECT_TOTALLY_SOBER},
		{CHAOS_EFFECT_FLY_ME_TO_THE_MOON, CHAOS_EFFECT_HEAVY_METAL},
		{CHAOS_EFFECT_SUNDAY_DRIVER, CHAOS_EFFECT_GOTTA_GO_FAST},
		{CHAOS_EFFECT_HOLE_IN_THE_TANK, CHAOS_EFFECT_TOO_BOOSTED_TO_FAIL},
		{CHAOS_EFFECT_THIS_IS_FIRE, CHAOS_EFFECT_BROWN_FIRE},
		{CHAOS_EFFECT_GROUNDED, CHAOS_EFFECT_HYDRAULICS},
		{CHAOS_EFFECT_BIG_KART_ENERGY, CHAOS_EFFECT_SMALL_KART_ENERGY},
		{CHAOS_EFFECT_ZA_WARUDO, CHAOS_EFFECT_KING_CRIMSON},
	};
	for (int i = 0; i < (int)len(opposites); i++)
	{
		if (effect == opposites[i][0] || effect == opposites[i][1])
		{
			replaced = effect == opposites[i][0] ? opposites[i][1] : opposites[i][0];
			break;
		}
	}
	// Instant gifts remove a restriction; the reverse is not a conflict.
	if (effect == CHAOS_EFFECT_RESERVE_INJECTION)
	{
		replaced = CHAOS_EFFECT_HOLE_IN_THE_TANK;
	}
	else if (effect == CHAOS_EFFECT_USE_THIS)
	{
		replaced = CHAOS_EFFECT_FAIR_MODE;
	}
	for (int otherPlayer = 0; otherPlayer < Chaos_PlayerPoolCount(); otherPlayer++)
	{
		Chaos_RemovePlayerReplacedEffects(effect, replaced, player, otherPlayer);
	}
}

static void Chaos_EnqueueInstantNotice(const struct ChaosActivation *activation, int player)
{
	for (int otherPlayer = 0; otherPlayer < Chaos_PlayerPoolCount(); otherPlayer++)
	{
		struct ChaosPlayerRuntime *other = &sdata->chaosRuntime.players[otherPlayer];
		for (int i = 0; i < other->numInstantNotices; i++)
		{
			if (other->instantNotices[i].effect == activation->effect &&
			    Chaos_EffectsShareTargets(activation->effect, player, activation->effect, otherPlayer))
			{
				Chaos_RemoveInstantNotice(other, i);
				break;
			}
		}
	}
	struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
	if (pool->numInstantNotices == CHAOS_MAX_INSTANT_NOTICES)
	{
		Chaos_RemoveInstantNotice(pool, 0);
	}
	pool->instantNotices[pool->numInstantNotices++] = *activation;
}

bool Chaos_ActivateForPlayer(ChaosEffectId effect, s32 durationMS, int player)
{
	const struct GameTracker *gGT = sdata->gGT;
	if (!Chaos_IsRunning(gGT) || durationMS <= 0 || player < 0 || player >= Chaos_PlayerPoolCount() ||
	    !Chaos_HasEnabledPlayer() ||
	    !ChaosEffect_IsEnabled(sdata->chaosSettings.effects, effect) ||
	    !ChaosEvent_EffectFitsSelection(effect, gGT->gameMode1, gGT->cup.cupID,
	                                   gGT->numPlyrCurrGame, g_config.extendedMultiplayer,
	                                   Chaos_UsesPersonalPools(), sdata->chaosSettings.disabledPlayers))
	{
		return false;
	}
	struct ChaosActivation activation = {
		.effect = effect,
	};
	const bool personal = Chaos_UsesPersonalPools();
	if (personal && (!Chaos_PlayerCanReceive(player) || Chaos_IsTimeStopped() ||
	    (ChaosEffect_Def(effect)->scope == CHAOS_SCOPE_BOTS && Chaos_BotEffectBlocked(effect))))
	{
		return false;
	}
	int bodySwapCandidates[LOAD_CHARACTER_ID_COUNT];
	int bodySwapCount = 0;
	if (personal && effect == CHAOS_EFFECT_BODY_SWAP)
	{
		bodySwapCount = Chaos_CollectBodySwapPartners(player, bodySwapCandidates);
		if (bodySwapCount == 0)
		{
			return false;
		}
	}
	Chaos_RemoveReplacedEffects(effect, player);
	if (effect == CHAOS_EFFECT_ZA_WARUDO && durationMS > CHAOS_TIME_STOP_MAX_MS)
	{
		durationMS = CHAOS_TIME_STOP_MAX_MS;
	}
	const bool instant = ChaosEffect_IsInstant(effect);
	struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
	if (!instant && pool->numActive == CHAOS_MAX_ACTIVATIONS)
	{
		return false;
	}
	activation.remainingMS = durationMS;
	activation.noticeRemainingMS = CHAOS_ACTIVATION_NOTICE_MS + (instant ? 0 : CHAOS_ACTIVATION_DOCK_MS);
	if (instant)
	{
		Chaos_EnqueueInstantNotice(&activation, player);
	}
	else
	{
		pool->active[pool->numActive++] = activation;
	}
	if (bodySwapCount > 0)
	{
		const int partner = bodySwapCandidates[ChaosEffect_NextRandom(&sdata->chaosRng) % bodySwapCount];
		Chaos_BeginBodySwapPair(player, partner);
		Chaos_RefreshBodySwapModifiers();
	}
	Chaos_ApplyOneOff(&activation, player);
	if (instant)
	{
		Chaos_StartBotCooldown(effect, 0);
	}
	if (effect == CHAOS_EFFECT_ZA_WARUDO)
	{
		Chaos_PlayTimeEffectSound(SOUND_CLOCK);
		Chaos_UpdateTimeStopAudio();
	}
	OtherFX_Play_LowLevel(HOWL_SFX_SWISH, 1, HOWL_SFX_DEFAULT_FLAGS | HOWL_SFX_GAIN_DOUBLE_FLAG);
	return true;
}

bool Chaos_Activate(ChaosEffectId effect, s32 durationMS)
{
	return Chaos_ActivateForPlayer(effect, durationMS, 0);
}

bool Chaos_IsActive(ChaosEffectId effect)
{
	if (ChaosEffect_Def(effect) == NULL || !Chaos_IsRunning(sdata->gGT))
	{
		return false;
	}
	for (int player = 0; player < Chaos_PlayerPoolCount(); player++)
	{
		const struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
		for (int i = 0; i < pool->numActive; i++)
		{
			if (pool->active[i].effect == effect)
			{
				return true;
			}
		}
	}
	return false;
}

bool Chaos_UsesTimePalette(void)
{
	if (Chaos_IsTimeStopped())
	{
		return true;
	}
	if (!Chaos_IsRunning(GAME_TRACKER))
	{
		return false;
	}
	const struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	for (int player = 0; player < Chaos_PlayerPoolCount(); player++)
	{
		const struct ChaosPlayerRuntime *pool = &runtime->players[player];
		for (int i = 0; i < pool->numInstantNotices; i++)
		{
			if (pool->instantNotices[i].effect == CHAOS_EFFECT_KING_CRIMSON &&
			    pool->instantNotices[i].elapsedMS < CHAOS_TIME_SKIP_PALETTE_MS)
			{
				return true;
			}
		}
	}
	return false;
}

bool Chaos_HasEffectForDriver(ChaosEffectId effect, const struct Driver *driver)
{
	for (int player = 0; player < Chaos_PlayerPoolCount(); player++)
	{
		const struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
		for (int i = 0; i < pool->numActive; i++)
		{
			if (pool->active[i].effect == effect && Chaos_ActivationTargetsDriver(effect, player, driver))
			{
				return true;
			}
		}
	}
	return false;
}

bool Chaos_IsActiveForDriver(ChaosEffectId effect, const struct Driver *driver)
{
	return Chaos_IsRunning(sdata->gGT) && Chaos_HasEffectForDriver(effect, driver);
}

static void Chaos_TickPlayerActivations(int player, s32 elapsedMS, bool timeStopped)
{
	struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
	for (int i = 0; i < pool->numInstantNotices;)
	{
		struct ChaosActivation *notice = &pool->instantNotices[i];
		if (timeStopped)
		{
			break;
		}
		Chaos_TickTimeEffectSound(notice, elapsedMS);
		notice->elapsedMS += elapsedMS;
		notice->noticeRemainingMS -= elapsedMS;
		if (notice->noticeRemainingMS <= 0)
		{
			Chaos_RemoveInstantNotice(pool, i);
		}
		else
		{
			i++;
		}
	}
	for (int i = 0; i < pool->numActive;)
	{
		struct ChaosActivation *entry = &pool->active[i];
		if (timeStopped && entry->effect != CHAOS_EFFECT_ZA_WARUDO)
		{
			i++;
			continue;
		}
		Chaos_TickTimeEffectSound(entry, elapsedMS);
		if (entry->remainingMS <= elapsedMS)
		{
			const s32 overshootMS = elapsedMS - entry->remainingMS;
			Chaos_RemoveActivation(player, i, overshootMS);
			continue;
		}
		entry->remainingMS -= elapsedMS;
		entry->elapsedMS += elapsedMS;
		entry->noticeRemainingMS -= elapsedMS;
		if (entry->noticeRemainingMS < 0)
		{
			entry->noticeRemainingMS = 0;
		}
		i++;
	}
}

static void Chaos_TickActivations(s32 elapsedMS)
{
	const bool timeStopped = Chaos_IsTimeStopped();
	for (int player = 0; player < Chaos_PlayerPoolCount(); player++)
	{
		Chaos_TickPlayerActivations(player, elapsedMS, timeStopped);
	}
}

static void Chaos_TickBotCooldowns(s32 elapsedMS)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	runtime->botCooldownMS = Chaos_TickCooldown(runtime->botCooldownMS, elapsedMS);
	for (int effect = 0; effect < CHAOS_EFFECT_COUNT; effect++)
	{
		runtime->botEffectCooldownMS[effect] = Chaos_TickCooldown(runtime->botEffectCooldownMS[effect], elapsedMS);
	}
}

void Chaos_Update(struct GameTracker *gGT)
{
	if (!Chaos_IsRunning(gGT) || gGT->elapsedTimeMS <= 0)
	{
		return;
	}
	const bool wasTimeStopped = Chaos_IsTimeStopped();
	if (!wasTimeStopped && Chaos_UsesPersonalPools())
	{
		// Tick existing cooldowns before expiry starts any new ones this frame.
		Chaos_TickBotCooldowns(gGT->elapsedTimeMS);
	}
	Chaos_TickActivations(gGT->elapsedTimeMS);
	if (wasTimeStopped)
	{
		return;
	}
	Chaos_UpdateRestarts(gGT->elapsedTimeMS);
	Chaos_UpdateTransformations(gGT);
	Chaos_UpdateControlAxes();
	Chaos_UpdateSchedule(gGT->elapsedTimeMS);
}
