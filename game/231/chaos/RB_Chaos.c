#include <common.h>
#include <game/Vehicle/VehPhysForce.h>
#include <game/Vehicle/VehPickupItem.h>
#include <chaos/chaos_gate.h>
#include <chaos/chaos_event.h>
#include <race_field.h>
#include <platform/chaos/native_chaos_ipc.h>
#include <platform/native_log.h>

enum
{
	CHAOS_RANDOM_GAP_MIN_PERCENT = 30,
	CHAOS_TIME_STOP_MAX_MS = 10000,
	CHAOS_BOT_COOLDOWN_MS = 30000,
	CHAOS_BOT_EFFECT_COOLDOWN_MS = 60000,
};

bool Chaos_IsPlayerEnabled(int player)
{
	return player >= 0 && player < CHAOS_MAX_HUMAN_PLAYERS &&
	       !sdata->chaosSettings.disabledPlayers[player];
}

static bool Chaos_HasEnabledPlayer(void)
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

static int Chaos_PlayerPoolCount(void)
{
	if (!Chaos_UsesPersonalPools())
	{
		return 1;
	}
	const int players = sdata->gGT->numPlyrCurrGame;
	return players < CHAOS_MAX_HUMAN_PLAYERS ? players : CHAOS_MAX_HUMAN_PLAYERS;
}

static bool Chaos_PlayerCanReceive(int player)
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

static bool Chaos_ActivationTargetsDriver(ChaosEffectId effect, int player, const struct Driver *driver)
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

static s32 Chaos_NextTriggerGap(void)
{
	const struct ChaosSettings *settings = &sdata->chaosSettings;
	const s32 intervalMS = settings->triggerIntervalSeconds * CHAOS_MILLISECONDS_PER_SECOND;
	if (settings->trigger != CHAOS_TRIGGER_RANDOM || intervalMS == 0 ||
	    settings->durationSeconds == 0 || settings->enabledCount == 0)
	{
		return intervalMS;
	}
	const s32 minMS = intervalMS * CHAOS_RANDOM_GAP_MIN_PERCENT / CHAOS_PERCENT_SCALE;
	const u32 sample = ChaosEffect_NextRandom(&sdata->chaosRng);
	return minMS + (s32)((u64)sample * (intervalMS - minMS) / CHAOS_RANDOM_SAMPLE_MAX);
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

static bool Chaos_CanSwapBody(int slot)
{
	const struct Driver *driver = GAME_TRACKER->drivers[slot];
	return driver != NULL && driver->instSelf != NULL && driver->instSelf->thread != NULL &&
	       (slot >= GAME_TRACKER->numPlyrCurrGame || Chaos_IsPlayerEnabled(slot)) &&
	       (driver->actionsFlagSet & ACTION_RACE_FINISHED) == 0;
}

static int Chaos_CollectBodySwapPartners(int player, int *candidates)
{
	const struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	if (!Chaos_CanSwapBody(player) || runtime->bodySwapPartner[player] != 0)
	{
		return 0;
	}
	int count = 0;
	for (int slot = 0; slot < (int)len(GAME_TRACKER->drivers); slot++)
	{
		if (slot != player && runtime->bodySwapPartner[slot] == 0 && Chaos_CanSwapBody(slot))
		{
			candidates[count++] = slot;
		}
	}
	return count;
}

static void Chaos_BeginBodySwapPair(int player, int partner)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	runtime->bodySwapPartner[player] = partner + 1;
	runtime->bodySwapPartner[partner] = player + 1;
	Chaos_SwapBodies(player, partner);
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
		const struct ChaosPlayerRuntime *pool = &runtime->players[player];
		for (int i = 0; i < pool->numActive; i++)
		{
			if (pool->active[i].effect == CHAOS_EFFECT_ZA_WARUDO)
			{
				return true;
			}
		}
	}
	return false;
}

static void Chaos_BeginBodySwaps(void)
{
	struct GameTracker *tracker = GAME_TRACKER;
	Chaos_ReturnBodySwaps();
	for (int player = 0; player < tracker->numPlyrCurrGame; player++)
	{
		int candidates[LOAD_CHARACTER_ID_COUNT];
		const int count = Chaos_CollectBodySwapPartners(player, candidates);
		if (count == 0)
		{
			continue;
		}
		const int partner = candidates[ChaosEffect_NextRandom(&sdata->chaosRng) % count];
		Chaos_BeginBodySwapPair(player, partner);
	}
	Chaos_RefreshBodySwapModifiers();
}

static void Chaos_RemoveActivation(int player, int index);

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
				Chaos_RemoveActivation(player, i);
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
				Chaos_SetKartScale(CHAOS_KART_SCALE_NORMAL, driver);
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

static void Chaos_OnEffectDeactivated(const struct ChaosActivation *activation, int player)
{
	const ChaosEffectId effect = activation->effect;
	Chaos_StartBotCooldown(effect, 0);
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
			Chaos_SetKartScale(CHAOS_KART_SCALE_NORMAL, driver);
		}
	}
}

static void Chaos_RemoveInstantNotice(struct ChaosPlayerRuntime *pool, int index)
{
	struct ChaosActivation *notice = &pool->instantNotices[index];
	memmove(notice, notice + 1, (--pool->numInstantNotices - index) * sizeof(*notice));
	memset(&pool->instantNotices[pool->numInstantNotices], 0, sizeof(*notice));
}

static void Chaos_RemoveActivation(int player, int index)
{
	struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
	const struct ChaosActivation activation = pool->active[index];
	int remaining = pool->numActive - index - 1;
	memmove(&pool->active[index], &pool->active[index + 1], remaining * sizeof(pool->active[0]));
	pool->active[--pool->numActive] = (struct ChaosActivation){0};
	Chaos_OnEffectDeactivated(&activation, player);
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
			Chaos_RemoveActivation(otherPlayer, i);
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

static void Chaos_ApplyOneOff(const struct ChaosActivation *chaosActivation, int player)
{
	const ChaosEffectId effect = chaosActivation->effect;
	if (effect == CHAOS_EFFECT_KING_CRIMSON)
	{
		Chaos_UpdateTimeStopAudio();
		Chaos_ResolveDeferredImpacts();
		Chaos_SkipBotTime();
		Chaos_PlayTimeEffectSound(SOUND_CLOCK);
		return;
	}
	if (effect == CHAOS_EFFECT_BODY_SWAP)
	{
		if (!Chaos_UsesPersonalPools())
		{
			Chaos_BeginBodySwaps();
		}
		return;
	}
	if (effect == CHAOS_EFFECT_YOUVE_GOT_MAIL)
	{
		Chaos_SendWarpball();
	}
	if (effect == CHAOS_EFFECT_CLOCKBLOCKED)
	{
		// Clock sound needs to be only applied once, otherwise it plays once per player
		OtherFX_Play(SOUND_CLOCK, 1);
	}
	if (effect == CHAOS_EFFECT_IDENTITY_CRISIS)
	{
		OtherFX_Play(SOUND_INVISIBILITY, 1);
	}
	for (int i = 0; i < (int)len(sdata->gGT->drivers); i++)
	{
		struct Driver *driver = sdata->gGT->drivers[i];
		if (Chaos_ActivationTargetsDriver(effect, player, driver))
		{
			switch ((int)effect)
			{
				case CHAOS_EFFECT_BIG_KART_ENERGY:
					Chaos_SetKartScale(CHAOS_GIANT_SCALE, driver);
					break;
				case CHAOS_EFFECT_SMALL_KART_ENERGY:
					Chaos_SetKartScale(s_chaosSmallScale, driver);
					break;
				case CHAOS_EFFECT_USE_THIS:
					Chaos_GiveRandomItem(driver);
					break;
				case CHAOS_EFFECT_JUICED_UP:
					Chaos_GiveWumpas(driver);
					break;
				case CHAOS_EFFECT_IT_IS_I:
					Chaos_GiveMask(driver);
					break;
				case CHAOS_EFFECT_PROVIDENCE:
					Chaos_GiveShield(driver);
					break;
				case CHAOS_EFFECT_RESERVE_INJECTION:
					Chaos_GiveReserves(driver);
					break;
				case CHAOS_EFFECT_THIS_IS_FIRE:
					Chaos_UpgradeTurbo(driver);
					break;
				case CHAOS_EFFECT_FRESH_START:
				{
					struct ChaosRestart *restart = &sdata->chaosRuntime.restart[driver->driverID];
					Chaos_EndRestartHold(restart, driver);
					restart->active = false;
					restart->pending = true;
					if (!Chaos_IsBeingRescued(driver))
					{
						Chaos_RestartDriver(driver, restart);
					}
					break;
				}
				case CHAOS_EFFECT_MY_PLANET_NEEDS_ME:
					Chaos_LaunchDriver(driver);
					break;
				case CHAOS_EFFECT_WHO_BOUND_THIS:
					if (driver->driverID < (int)len(sdata->chaosRuntime.controls))
					{
						Chaos_ShuffleControls(&sdata->chaosRuntime.controls[driver->driverID], &GAMEPADS->gamepad[driver->driverID]);
					}
					break;
				case CHAOS_EFFECT_TRY_SOMETHING_NEW:
					if (driver->driverID < (int)len(sdata->chaosRuntime.engineClass))
					{
						Chaos_ChangeDrivingClass(driver);
					}
					break;
				case CHAOS_EFFECT_IDENTITY_CRISIS:
					Chaos_ChangeIdentity(driver);
					break;
				case CHAOS_EFFECT_YARDRAT_TRAINING:
					Chaos_TeleportDriver(driver);
					break;
				case CHAOS_EFFECT_DEFECTIVE_TELEPORT:
					Chaos_DefectiveTeleportDriver(driver);
					break;
				case CHAOS_EFFECT_CLOCKBLOCKED:
					Chaos_ApplyClockCurse(driver, chaosActivation->remainingMS);
					break;
				default:
					break;
			}
		}
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

static bool Chaos_ActivateForPlayer(ChaosEffectId effect, s32 durationMS, int player)
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

static bool Chaos_TriggerForPlayer(int player)
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

static void Chaos_PollExternal(void)
{
	struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[0];
	if (pool->intervalRemainingMS > 0)
	{
		return;
	}
	char command[NATIVE_CHAOS_COMMAND_SIZE];
	if (NativeChaosIPC_ReadCommand(command, sizeof(command)) == NATIVE_CHAOS_COMMAND &&
	    Chaos_ActivateExternalEffect(command))
	{
		pool->intervalRemainingMS = Chaos_NextTriggerGap();
	}
}

static bool Chaos_HasEffectForDriver(ChaosEffectId effect, const struct Driver *driver)
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

static const struct Driver *Chaos_TopDownDriver(const struct PushBuffer *pushBuffer)
{
	const struct GameTracker *tracker = sdata->gGT;
	if (pushBuffer == NULL || !sdata->chaosRuntime.raceInitialized || !sdata->chaosSettings.armed ||
	    !ChaosGate_IsRaceEligible(tracker->gameMode1 & ~PAUSE_ALL, tracker->overlayIndex_Threads,
	                             tracker->trafficLightsTimer < 1))
	{
		return NULL;
	}
	for (int i = 0; i < tracker->numPlyrCurrGame && i < (int)len(tracker->cameraDC); i++)
	{
		const struct CameraDC *camera = &tracker->cameraDC[i];
		const struct Driver *driver = camera->driverToFollow;
		if (camera->pushBuffer == pushBuffer && Chaos_HasEffectForDriver(CHAOS_EFFECT_CLASSIC_GTA, driver) &&
		    (driver->actionsFlagSet & ACTION_BOT) == 0)
		{
			return driver;
		}
	}
	return NULL;
}

bool Chaos_ApplyCameraRules(struct CameraDC *camera)
{
	struct PushBuffer *pushBuffer = camera->pushBuffer;
	const struct Driver *driver = Chaos_TopDownDriver(pushBuffer);
	if (driver == NULL || driver != camera->driverToFollow)
	{
		return false;
	}
	Chaos_SetTopDownCamera(camera, driver);
	return true;
}

bool Chaos_HideCameraQuadBlock(const struct PushBuffer *pushBuffer, const struct QuadBlock *block, const struct LevVertex *vertices)
{
	const struct Driver *driver = Chaos_TopDownDriver(pushBuffer);
	if (driver == NULL)
	{
		return false;
	}
	return Chaos_TopDownHidesQuadBlock(driver, block, vertices);
}

void Chaos_ApplyLevelVisibilityRules(const struct PushBuffer *pushBuffer, struct VisMem *visibility,
                                   const struct mesh_info *mesh, int playerIndex)
{
	if (Chaos_TopDownDriver(pushBuffer) != NULL)
	{
		Chaos_SetTopDownVisibility(visibility, mesh, playerIndex);
	}
}

bool Chaos_HideCameraInstance(const struct PushBuffer *pushBuffer, const struct Instance *instance, int nearestDepth, bool coversKart)
{
	const struct Driver *driver = Chaos_TopDownDriver(pushBuffer);
	if (driver == NULL)
	{
		return false;
	}
	return Chaos_TopDownHidesInstance(driver, pushBuffer, instance, nearestDepth, coversKart);
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
			const ChaosEffectId effect = entry->effect;
			const s32 overshootMS = elapsedMS - entry->remainingMS;
			Chaos_RemoveActivation(player, i);
			Chaos_StartBotCooldown(effect, overshootMS);
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

void Chaos_Update(struct GameTracker *gGT)
{
	if (!Chaos_IsRunning(gGT) || gGT->elapsedTimeMS <= 0)
	{
		return;
	}
	struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[0];
	const struct ChaosSettings *settings = &sdata->chaosSettings;
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
	if (settings->trigger == CHAOS_TRIGGER_EXTERNAL)
	{
		if (Chaos_UsesPersonalPools())
		{
			return;
		}
		if (pool->intervalRemainingMS > 0)
		{
			pool->intervalRemainingMS -= gGT->elapsedTimeMS;
			if (pool->intervalRemainingMS < 0)
			{
				pool->intervalRemainingMS = 0;
			}
		}
		Chaos_PollExternal();
		return;
	}

	if ((settings->trigger != CHAOS_TRIGGER_INTERVAL && settings->trigger != CHAOS_TRIGGER_RANDOM) ||
	    settings->triggerIntervalSeconds == 0)
	{
		return;
	}
	if (settings->trigger == CHAOS_TRIGGER_INTERVAL)
	{
		Chaos_UpdateTriggerClock(0, gGT->elapsedTimeMS, true);
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
			Chaos_UpdateTriggerClock(player, gGT->elapsedTimeMS, false);
		}
		if (Chaos_IsTimeStopped())
		{
			break;
		}
	}
}
