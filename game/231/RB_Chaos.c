#include <common.h>
#include <chaos_gate.h>
#include <race_field.h>
#include <platform/native_chaos_ipc.h>
#include <platform/native_log.h>

enum
{
	CHAOS_RANDOM_GAP_MIN_PERCENT = 30,
	CHAOS_TIME_STOP_MAX_MS = 10000,
};

static bool Chaos_TargetsDriver(ChaosEffectId effect, const struct Driver *driver)
{
	const ChaosEffectDef *def = ChaosEffect_Def(effect);
	return def != NULL && driver != NULL &&
	       (driver->actionsFlagSet & ACTION_RACE_FINISHED) == 0 &&
	       ChaosEffect_AppliesToDriver(def->scope, driver->driverID, sdata->gGT->numPlyrCurrGame);
}

static bool Chaos_EffectFitsEvent(ChaosEffectId effect)
{
	const ChaosEffectDef *def = ChaosEffect_Def(effect);
	if (effect == CHAOS_EFFECT_BODY_SWAP && sdata->gGT->numPlyrCurrGame < 2)
	{
		return false;
	}
	if (effect == CHAOS_EFFECT_YARDRAT_TRAINING || effect == CHAOS_EFFECT_DEFECTIVE_TELEPORT)
	{
		return (sdata->gGT->gameMode1 & (BATTLE_MODE | CRYSTAL_CHALLENGE)) == 0;
	}
	return def->scope != CHAOS_SCOPE_BOTS || (sdata->gGT->gameMode1 & RACE_FIELD_NO_BOT_MODES) == 0;
}

static ChaosEffectId Chaos_PickEventEffect(void)
{
	const struct ChaosSettings *settings = &sdata->chaosSettings;
	bool effects[CHAOS_EFFECT_COUNT];
	memcpy(effects, settings->effects, sizeof(effects));
	int enabledCount = settings->enabledCount;
	for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
	{
		if (effects[i] && !Chaos_EffectFitsEvent((ChaosEffectId)i))
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
	return sdata->chaosRuntime.raceInitialized && sdata->chaosSettings.armed &&
	       sdata->ptrActiveMenu == NULL && sdata->Loading.stage == LOAD_IDLE &&
	       (gGT->gameMode1 & DEBUG_MENU) == 0 &&
	       ChaosGate_IsRaceEligible(gGT->gameMode1, gGT->overlayIndex_Threads,
	                                gGT->trafficLightsTimer < 1, CHAOS_GATE_ADVENTURE_ONLY);
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
	       (driver->actionsFlagSet & ACTION_RACE_FINISHED) == 0;
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
	for (int i = 0; i < runtime->numActive; i++)
	{
		if (runtime->active[i].effect == CHAOS_EFFECT_ZA_WARUDO)
		{
			return true;
		}
	}
	return false;
}

static void Chaos_BeginBodySwaps(void)
{
	struct GameTracker *tracker = GAME_TRACKER;
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	Chaos_ReturnBodySwaps();
	for (int player = 0; player < tracker->numPlyrCurrGame; player++)
	{
		if (runtime->bodySwapPartner[player] != 0 || !Chaos_CanSwapBody(player))
		{
			continue;
		}
		int candidates[LOAD_CHARACTER_ID_COUNT];
		int count = 0;
		for (int slot = player + 1; slot < (int)len(tracker->drivers); slot++)
		{
			if (runtime->bodySwapPartner[slot] == 0 && Chaos_CanSwapBody(slot))
			{
				candidates[count++] = slot;
			}
		}
		if (count == 0)
		{
			continue;
		}
		const int partner = candidates[ChaosEffect_NextRandom(&sdata->chaosRng) % count];
		runtime->bodySwapPartner[player] = partner + 1;
		runtime->bodySwapPartner[partner] = player + 1;
		Chaos_SwapBodies(player, partner);
	}
	Chaos_RefreshBodySwapModifiers();
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
			Chaos_ReturnBodySwap(slot);
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
					const int original = GAME_CHARACTER_METADATA[GAME_CHARACTER_IDS[driver->driverID]].engineID;
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
	if (sdata->chaosSettings.armed && sdata->chaosSettings.effects[CHAOS_EFFECT_IDENTITY_CRISIS] &&
	    ChaosGate_IsRaceEligible(gGT->gameMode1 & ~(PAUSE_ALL | LOADING),
	                             OVERLAY_INDEX_RACING_OR_BATTLE, true, CHAOS_GATE_ADVENTURE_ONLY))
	{
		Chaos_PreloadCharacterModels(bigfile);
	}
}

void Chaos_BeginRace(struct GameTracker *gGT)
{
	Chaos_Reset();
	Chaos_PublishEffects();
	// Initialization may precede unpausing; GO gates updates, not race setup.
	if (!sdata->chaosSettings.armed ||
	    !ChaosGate_IsRaceEligible(gGT->gameMode1 & ~(PAUSE_ALL | LOADING),
	                             gGT->overlayIndex_Threads, true, CHAOS_GATE_ADVENTURE_ONLY))
	{
		return;
	}
	sdata->chaosRuntime.raceInitialized = true;
	sdata->chaosRuntime.intervalRemainingMS = Chaos_NextTriggerGap();
}

static void Chaos_OnEffectDeactivated(ChaosEffectId effect)
{
	if (effect == CHAOS_EFFECT_BODY_SWAP)
	{
		Chaos_ReturnBodySwaps();
		Chaos_RefreshBodySwapModifiers();
	}
	if (effect != CHAOS_EFFECT_BIG_KART_ENERGY && effect != CHAOS_EFFECT_SMALL_KART_ENERGY)
	{
		return;
	}
	for (int i = 0; i < sdata->gGT->numPlyrCurrGame && i < (int) len(sdata->gGT->drivers); i++)
	{
		struct Driver *driver = sdata->gGT->drivers[i];
		if (driver != NULL)
		{
			Chaos_SetKartScale(CHAOS_KART_SCALE_NORMAL, driver);
		}
	}
}

static void Chaos_RemoveInstantNotice(int index)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	struct ChaosActivation *notice = &runtime->instantNotices[index];
	memmove(notice, notice + 1, (--runtime->numInstantNotices - index) * sizeof(*notice));
	memset(&runtime->instantNotices[runtime->numInstantNotices], 0, sizeof(*notice));
}

static void Chaos_RemoveActivation(int index)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	ChaosEffectId effect = runtime->active[index].effect;
	int remaining = runtime->numActive - index - 1;
	memmove(&runtime->active[index], &runtime->active[index + 1], remaining * sizeof(runtime->active[0]));
	runtime->active[--runtime->numActive] = (struct ChaosActivation){0};
	Chaos_OnEffectDeactivated(effect);
}

static void Chaos_RemoveReplacedEffects(ChaosEffectId effect)
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
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	for (int i = 0; i < runtime->numInstantNotices;)
	{
		struct ChaosActivation *notice = &runtime->instantNotices[i];
		if (notice->effect == replaced)
		{
			Chaos_RemoveInstantNotice(i);
		}
		else
		{
			i++;
		}
	}
	for (int i = 0; i < runtime->numActive;)
	{
		const ChaosEffectId active = runtime->active[i].effect;
		const bool replacesJumpEffect = effect == CHAOS_EFFECT_MY_PLANET_NEEDS_ME &&
		    (active == CHAOS_EFFECT_GROUNDED || active == CHAOS_EFFECT_HYDRAULICS);
		if (active == effect || active == replaced || replacesJumpEffect)
		{
			Chaos_RemoveActivation(i);
		}
		else
		{
			i++;
		}
	}
}

static void Chaos_ApplyOneOff(const struct ChaosActivation *chaosActivation)
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
		Chaos_BeginBodySwaps();
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
		if (Chaos_TargetsDriver(effect, driver))
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

static void Chaos_EnqueueInstantNotice(const struct ChaosActivation *activation)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	// Refresh duplicates; overflow drops the oldest banner.
	for (int i = 0; i < runtime->numInstantNotices; i++)
	{
		if (runtime->instantNotices[i].effect == activation->effect)
		{
			Chaos_RemoveInstantNotice(i);
			break;
		}
	}
	if (runtime->numInstantNotices == CHAOS_MAX_INSTANT_NOTICES)
	{
		Chaos_RemoveInstantNotice(0);
	}
	runtime->instantNotices[runtime->numInstantNotices++] = *activation;
}

bool Chaos_Activate(ChaosEffectId effect, s32 durationMS)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	if (!Chaos_IsRunning(sdata->gGT) || durationMS <= 0 ||
	    !ChaosEffect_IsEnabled(sdata->chaosSettings.effects, effect) || !Chaos_EffectFitsEvent(effect))
	{
		return false;
	}
	Chaos_RemoveReplacedEffects(effect);
	if (effect == CHAOS_EFFECT_ZA_WARUDO && durationMS > CHAOS_TIME_STOP_MAX_MS)
	{
		durationMS = CHAOS_TIME_STOP_MAX_MS;
	}
	const bool instant = ChaosEffect_IsInstant(effect);
	if (!instant && runtime->numActive == CHAOS_MAX_ACTIVATIONS)
	{
		return false;
	}
	struct ChaosActivation activation = {
		.effect = effect,
		.remainingMS = durationMS,
		.noticeRemainingMS = CHAOS_ACTIVATION_NOTICE_MS + (instant ? 0 : CHAOS_ACTIVATION_DOCK_MS),
		.elapsedMS = 0,
	};
	if (instant)
	{
		Chaos_EnqueueInstantNotice(&activation);
	}
	else
	{
		runtime->active[runtime->numActive++] = activation;
	}
	Chaos_ApplyOneOff(&activation);
	if (effect == CHAOS_EFFECT_ZA_WARUDO)
	{
		Chaos_PlayTimeEffectSound(SOUND_CLOCK);
		Chaos_UpdateTimeStopAudio();
	}
	OtherFX_Play_LowLevel(HOWL_SFX_SWISH, 1, HOWL_SFX_DEFAULT_FLAGS | HOWL_SFX_GAIN_DOUBLE_FLAG);
	return true;
}

bool Chaos_IsActive(ChaosEffectId effect)
{
	if (ChaosEffect_Def(effect) == NULL || !Chaos_IsRunning(sdata->gGT))
	{
		return false;
	}
	for (int i = 0; i < sdata->chaosRuntime.numActive; i++)
	{
		if (sdata->chaosRuntime.active[i].effect == effect)
		{
			return true;
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
	for (int i = 0; i < runtime->numInstantNotices; i++)
	{
		if (runtime->instantNotices[i].effect == CHAOS_EFFECT_KING_CRIMSON &&
		    runtime->instantNotices[i].elapsedMS < CHAOS_TIME_SKIP_PALETTE_MS)
		{
			return true;
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
	const bool eligibleRace = gGT != NULL &&
	    ChaosGate_IsRaceEligible(gGT->gameMode1 & ~(PAUSE_ALL | LOADING),
	                             gGT->overlayIndex_Threads, true, CHAOS_GATE_ADVENTURE_ONLY);
	for (int i = 0; settings->armed && eligibleRace && i < CHAOS_EFFECT_COUNT; i++)
	{
		if (settings->effects[i] && Chaos_EffectFitsEvent((ChaosEffectId)i))
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
	const ChaosEffectId effect = ChaosEffect_FromSlug(slug);
	if (effect == CHAOS_EFFECT_NONE)
	{
		Platform_LogError("[Chaos] Consumed invalid or unknown effect command.\n");
		return false;
	}
	if (!ChaosEffect_IsEnabled(sdata->chaosSettings.effects, effect))
	{
		Platform_LogError("[Chaos] Consumed disabled effect: %s\n", slug);
		return false;
	}
	if (!Chaos_Activate(effect, sdata->chaosSettings.durationSeconds * CHAOS_MILLISECONDS_PER_SECOND))
	{
		Platform_LogError("[Chaos] Consumed effect rejected by runtime: %s\n", slug);
		return false;
	}
	return true;
}

static void Chaos_PollExternal(void)
{
	if (sdata->chaosRuntime.intervalRemainingMS > 0)
	{
		return;
	}
	char command[NATIVE_CHAOS_COMMAND_SIZE];
	if (NativeChaosIPC_ReadCommand(command, sizeof(command)) == NATIVE_CHAOS_COMMAND &&
	    Chaos_ActivateExternalEffect(command))
	{
		sdata->chaosRuntime.intervalRemainingMS = Chaos_NextTriggerGap();
	}
}

bool Chaos_IsActiveForDriver(ChaosEffectId effect, const struct Driver *driver)
{
	return Chaos_TargetsDriver(effect, driver) && Chaos_IsActive(effect);
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
		Chaos_RequestOffroadRescue(driver);
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
	    !ChaosEffect_AppliesToDriver(CHAOS_SCOPE_HUMANS, driver->driverID, sdata->gGT->numPlyrCurrGame) ||
	    settings->durationSeconds == 0)
	{
		return false;
	}
	const ChaosEffectId effect = Chaos_PickEventEffect();
	return Chaos_Activate(effect, settings->durationSeconds * CHAOS_MILLISECONDS_PER_SECOND);
}

void Chaos_OnTurboPadContact(struct Driver *driver, bool touching)
{
	if (sdata->chaosSettings.trigger != CHAOS_TRIGGER_PICKUP || !Chaos_IsRunning(sdata->gGT) || driver == NULL ||
	    !ChaosEffect_AppliesToDriver(CHAOS_SCOPE_HUMANS, driver->driverID, sdata->gGT->numPlyrCurrGame) ||
	    driver->driverID >= ADV_MULTIPLAYER_NUM_PLAYERS_MAX)
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
	const struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	for (int i = 0; i < runtime->numActive; i++)
	{
		const ChaosEffectId effect = runtime->active[i].effect;
		if (!Chaos_TargetsDriver(effect, driver))
		{
			continue;
		}
		if (effect == CHAOS_EFFECT_MIRROR_MIRROR)
		{
			pad->stickLX = Chaos_InvertSteering(pad);
		}
		else if (effect == CHAOS_EFFECT_TOTALLY_SOBER)
		{
			pad->stickLX = Chaos_WobbleSteering(&runtime->active[i], pad);
		}
		else if (effect == CHAOS_EFFECT_SABOTAGE)
		{
			Chaos_DisableBraking(pad);
		}
	}
}

static const struct Driver *Chaos_TopDownDriver(const struct PushBuffer *pushBuffer)
{
	const struct GameTracker *tracker = sdata->gGT;
	if (pushBuffer == NULL || !sdata->chaosRuntime.raceInitialized || !sdata->chaosSettings.armed ||
	    !ChaosGate_IsRaceEligible(tracker->gameMode1 & ~PAUSE_ALL, tracker->overlayIndex_Threads,
	                             tracker->trafficLightsTimer < 1, CHAOS_GATE_ADVENTURE_ONLY))
	{
		return NULL;
	}
	bool active = false;
	for (int i = 0; i < sdata->chaosRuntime.numActive; i++)
	{
		if (sdata->chaosRuntime.active[i].effect == CHAOS_EFFECT_CLASSIC_GTA)
		{
			active = true;
			break;
		}
	}
	if (!active)
	{
		return NULL;
	}
	for (int i = 0; i < tracker->numPlyrCurrGame && i < (int)len(tracker->cameraDC); i++)
	{
		const struct CameraDC *camera = &tracker->cameraDC[i];
		const struct Driver *driver = camera->driverToFollow;
		if (camera->pushBuffer == pushBuffer && Chaos_TargetsDriver(CHAOS_EFFECT_CLASSIC_GTA, driver) &&
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

u32 Chaos_ApplyDriver(struct Driver *driver, u32 actions)
{
	const struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	if (!Chaos_IsRunning(sdata->gGT))
	{
		return actions;
	}
	for (int i = 0; i < runtime->numActive; i++)
	{
		const ChaosEffectId effect = runtime->active[i].effect;
		if (!Chaos_TargetsDriver(effect, driver))
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
			Chaos_TickWumpaDrain(runtime->active[i].elapsedMS, driver);
		}
	}
	return actions;
}

static void Chaos_TickActivations(s32 elapsedMS)
{
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	const bool timeStopped = Chaos_IsTimeStopped();
	for (int i = 0; i < runtime->numInstantNotices;)
	{
		struct ChaosActivation *notice = &runtime->instantNotices[i];
		if (timeStopped)
		{
			break;
		}
		Chaos_TickTimeEffectSound(notice, elapsedMS);
		notice->elapsedMS += elapsedMS;
		notice->noticeRemainingMS -= elapsedMS;
		if (notice->noticeRemainingMS <= 0)
		{
			Chaos_RemoveInstantNotice(i);
		}
		else
		{
			i++;
		}
	}
	for (int i = 0; i < runtime->numActive;)
	{
		struct ChaosActivation *entry = &runtime->active[i];
		if (timeStopped && entry->effect != CHAOS_EFFECT_ZA_WARUDO)
		{
			i++;
			continue;
		}
		Chaos_TickTimeEffectSound(entry, elapsedMS);
		if (entry->remainingMS <= elapsedMS)
		{
			Chaos_RemoveActivation(i);
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

void Chaos_Update(struct GameTracker *gGT)
{
	if (!Chaos_IsRunning(gGT) || gGT->elapsedTimeMS <= 0)
	{
		return;
	}
	struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	const struct ChaosSettings *settings = &sdata->chaosSettings;
	const bool wasTimeStopped = Chaos_IsTimeStopped();
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
		if (runtime->intervalRemainingMS > 0)
		{
			runtime->intervalRemainingMS -= gGT->elapsedTimeMS;
			if (runtime->intervalRemainingMS < 0)
			{
				runtime->intervalRemainingMS = 0;
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
	const s32 durationMS = settings->durationSeconds * CHAOS_MILLISECONDS_PER_SECOND;
	runtime->intervalRemainingMS -= gGT->elapsedTimeMS;
	while (runtime->intervalRemainingMS <= 0)
	{
		if (durationMS > 0)
		{
			const ChaosEffectId effect = Chaos_PickEventEffect();
			Chaos_Activate(effect, durationMS);
		}
		runtime->intervalRemainingMS += Chaos_NextTriggerGap();
	}
}
