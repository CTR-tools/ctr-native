#include <shared/chaos_fixture.h>
#include <game/MAIN/MainGameStart.c>

static bool s_teleported;

void VehBirth_TeleportAll(struct GameTracker *gGT, u32 spawnFlags)
{
	(void)gGT;
	(void)spawnFlags;
	s_teleported = true;
	CTR_TEST_CHECK(!sdata->chaosRuntime.raceInitialized, "race setup runs after vehicle initialization");
}

void Music_Stop(void) {}

static s32 LatestNoticeRemaining(void)
{
	const struct ChaosRuntime *runtime = &sdata->chaosRuntime;
	return runtime->numActive > 0 ? runtime->active[runtime->numActive - 1].noticeRemainingMS : 0;
}

static void TestIntervalCadence(void)
{
	CTR_TEST_SECTION("Interval starts after GO, carries frame overshoot and gives new entries full duration");
	SetupRace(CHAOS_TRIGGER_INTERVAL);
	s_teleported = false;
	Chaos_Reset();
	MainGameStart_Initialize(&s_tracker, true);
	CTR_TEST_CHECK(s_teleported && sdata->chaosRuntime.raceInitialized, "the actual race-start callback initializes Chaos after teleporting");
	CTR_TEST_CHECK(!g_config.expandedAdventureOptions, "the creation prompt is disabled in this fixture");
	Tick(5000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "the countdown cannot activate an effect");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.intervalRemainingMS, 5000, "the countdown does not consume trigger time");
	s_tracker.trafficLightsTimer = 0;
	Tick(4992);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "Interval waits for the whole first gap");
	Tick(32);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 1, "an armed file activates despite the disabled creation prompt");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].effect, CHAOS_EFFECT_FLY_ME_TO_THE_MOON, "Interval chooses from selected effects only");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].remainingMS, 30000, "a new activation starts with its full duration");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.intervalRemainingMS, 4976, "frame overshoot is retained rather than drifting the cadence");
	Tick(4976);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 1, "Interval replaces an already active effect");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].remainingMS, 30000, "repeated selection resets the duration");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].elapsedMS, 0, "repeated selection resets the elapsed time");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.intervalRemainingMS, 5000, "the second boundary returns to the original cadence");
}

static void TestIndependentExpiryAndOrder(void)
{
	CTR_TEST_SECTION("reactivation replaces the old entry and preserves unrelated timers");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_GOTTA_GO_FAST] = true;
	sdata->chaosSettings.enabledCount++;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 200), "the first selected effect can activate");
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_GOTTA_GO_FAST, 100), "an unrelated later effect can overlap");
	Tick(50);
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 300), "a repeated activation replaces the old entry");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 2, "reactivation does not add a duplicate slot");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].effect, CHAOS_EFFECT_GOTTA_GO_FAST, "unrelated effects keep their relative order");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].remainingMS, 50, "reactivation leaves unrelated timers unchanged");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[1].effect, CHAOS_EFFECT_FLY_ME_TO_THE_MOON, "reactivation becomes the newest entry");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[1].elapsedMS, 0, "reactivation resets its phase");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[1].noticeRemainingMS, CHAOS_ACTIVATION_NOTICE_MS + CHAOS_ACTIVATION_DOCK_MS, "reactivation restarts its notification");
	Tick(50);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 1, "only the entry reaching its exact expiry is removed");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].remainingMS, 250, "compaction preserves the replacement timer");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_GOTTA_GO_FAST), "an expired effect no longer reports active");
	Tick(100);
	CTR_TEST_CHECK(Chaos_IsActive(CHAOS_EFFECT_FLY_ME_TO_THE_MOON), "the superseded expiry cannot stop the replacement");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].remainingMS, 150, "replacement continues from its new activation time");
	Tick(150);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "the last timer expires exactly at zero");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_FLY_ME_TO_THE_MOON), "no active status remains after final expiry");

	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 100);
	Chaos_Activate(CHAOS_EFFECT_GOTTA_GO_FAST, 300);
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 200);
	Tick(100);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].effect, CHAOS_EFFECT_GOTTA_GO_FAST, "removing the oldest entry preserves the next effect's order");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[1].effect, CHAOS_EFFECT_FLY_ME_TO_THE_MOON, "the newest effect remains last after compaction");
}

static void CheckSuspended(void)
{
	const struct ChaosRuntime before = sdata->chaosRuntime;
	const u32 rng = sdata->chaosRng;
	Tick(1000);
	CTR_TEST_CHECK(memcmp(&before, &sdata->chaosRuntime, sizeof(before)) == 0, "ineligible updates preserve timers and cadence");
	CTR_TEST_CHECK_EQ(sdata->chaosRng, rng, "ineligible updates do not draw Chaos RNG");
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 1000), "ineligible frames reject activation requests");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_FLY_ME_TO_THE_MOON), "effect hooks see no active modifier outside eligible gameplay");
}

static void TestGatesAndPause(void)
{
	CTR_TEST_SECTION("pause, menus, transitions and adjacent modes cannot advance or activate Chaos");
	SetupRace(CHAOS_TRIGGER_INTERVAL);
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 10000);
	const int blocked[] = {PAUSE_1, DEBUG_MENU, ADVENTURE_ARENA, MAIN_MENU, GAME_CUTSCENE, LOADING, END_OF_RACE};
	for (int i = 0; i < (int)len(blocked); i++)
	{
		s_tracker.gameMode1 = ADVENTURE_MODE | blocked[i];
		CheckSuspended();
	}
	s_tracker.gameMode1 = ARCADE_MODE;
	CheckSuspended();
	s_tracker.gameMode1 = BATTLE_MODE;
	CheckSuspended();
	s_tracker.gameMode1 = ADVENTURE_MODE;
	s_tracker.overlayIndex_Threads = OVERLAY_INDEX_ADV_HUB;
	CheckSuspended();
	s_tracker.overlayIndex_Threads = OVERLAY_INDEX_RACING_OR_BATTLE;
	sdata->ptrActiveMenu = &sdata->chaosMenu.menu;
	CheckSuspended();
	sdata->ptrActiveMenu = NULL;
	sdata->Loading.stage = LOAD_REQUESTED;
	CheckSuspended();
	sdata->Loading.stage = LOAD_IDLE;
	sdata->chaosSettings.armed = false;
	CheckSuspended();
	sdata->chaosSettings.armed = true;
	Tick(1000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].remainingMS, 9000, "resuming consumes only the next gameplay delta");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.intervalRemainingMS, 4000, "the trigger resumes its remaining gap");
	const int variants[] = {RELIC_RACE, CRYSTAL_CHALLENGE, ADVENTURE_CUP, (int)ADVENTURE_BOSS};
	for (int i = 0; i < (int)len(variants); i++)
	{
		s_tracker.gameMode1 = ADVENTURE_MODE | variants[i];
		CTR_TEST_CHECK(Chaos_IsActive(CHAOS_EFFECT_FLY_ME_TO_THE_MOON), "every Adventure race type can consume active effects");
	}
}

static void TestCapacityAndValidation(void)
{
	CTR_TEST_SECTION("capacity rejects new effects but permits reactivation and scheduled refresh");
	SetupRace(CHAOS_TRIGGER_INTERVAL);
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_NONE, 100), "the none sentinel cannot activate");
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_COUNT, 100), "out-of-range effects cannot activate");
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_FAIR_MODE, 100), "disabled catalogue effects cannot activate");
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 0), "zero duration cannot create an entry");
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, -1), "negative duration cannot create an entry");
	const ChaosEffectId effects[] = {CHAOS_EFFECT_FLY_ME_TO_THE_MOON, CHAOS_EFFECT_GROUNDED,
	    CHAOS_EFFECT_FAIR_MODE, CHAOS_EFFECT_WUMPA_TAX};
	for (int i = 0; i < CHAOS_MAX_ACTIVATIONS; i++)
	{
		sdata->chaosSettings.effects[effects[i]] = true;
		CTR_TEST_CHECK(Chaos_Activate(effects[i], 5000 + i), "each available slot accepts a distinct effect");
	}
	sdata->chaosSettings.effects[CHAOS_EFFECT_MIRROR_MIRROR] = true;
	const struct ChaosRuntime full = sdata->chaosRuntime;
	CTR_TEST_CHECK_EQ(full.numActive, 4, "four distinct effects fill the array");
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 9999), "a full queue rejects a fifth distinct effect");
	CTR_TEST_CHECK(memcmp(&full, &sdata->chaosRuntime, sizeof(full)) == 0, "overflow preserves existing activations");
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 0), "invalid reactivation cannot remove the running effect");
	CTR_TEST_CHECK(memcmp(&full, &sdata->chaosRuntime, sizeof(full)) == 0, "invalid reactivation preserves all timers");
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 9999), "reactivation succeeds at full capacity");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 4, "reactivation keeps the array at capacity without duplicates");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[3].remainingMS, 9999, "reactivation replaces the old duration");
	for (int i = 0; i < 3; i++)
	{
		CTR_TEST_CHECK(memcmp(&full.active[i + 1], &sdata->chaosRuntime.active[i], sizeof(full.active[0])) == 0,
		    "reactivation preserves every unrelated entry");
	}
	memset(sdata->chaosSettings.effects, 0, sizeof(sdata->chaosSettings.effects));
	sdata->chaosSettings.effects[CHAOS_EFFECT_FLY_ME_TO_THE_MOON] = true;
	sdata->chaosSettings.enabledCount = 1;
	sdata->chaosRuntime.intervalRemainingMS = 1;
	const u32 rng = sdata->chaosRng;
	Tick(1);
	CTR_TEST_CHECK(sdata->chaosRng != rng, "a scheduled event chooses an effect even at capacity");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[3].remainingMS, 30000, "Interval can refresh an effect at capacity");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.intervalRemainingMS, 5000, "a scheduled refresh retains the configured gap");
	sdata->chaosSettings.effects[CHAOS_EFFECT_FLY_ME_TO_THE_MOON] = false;
	sdata->chaosSettings.effects[CHAOS_EFFECT_MIRROR_MIRROR] = true;
	sdata->chaosRuntime.intervalRemainingMS = 1;
	const int sounds = s_activationSounds;
	const u32 beforeRejectedChoice = sdata->chaosRng;
	Tick(1);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 4, "a scheduled fifth distinct effect cannot evict running effects");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_MIRROR_MIRROR), "capacity also rejects a distinct scheduled effect");
	CTR_TEST_CHECK_EQ(s_activationSounds, sounds, "a rejected scheduled activation cannot announce");
	CTR_TEST_CHECK(sdata->chaosRng != beforeRejectedChoice, "a rejected scheduled event still consumes its effect choice");
	sdata->chaosSettings.effects[CHAOS_EFFECT_MIRROR_MIRROR] = false;
	sdata->chaosSettings.effects[CHAOS_EFFECT_FLY_ME_TO_THE_MOON] = true;
	Tick(5003);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 1, "expired unrelated effects leave only the refreshed effect");
	sdata->chaosSettings.effects[CHAOS_EFFECT_MIRROR_MIRROR] = true;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 1000), "a freed slot can accept a new effect");
}

static void TestResetAndInvalidSettings(void)
{
	CTR_TEST_SECTION("race reset preserves file settings and handles empty or invalid timed settings");
	SetupRace(CHAOS_TRIGGER_INTERVAL);
	const u32 startupSeed = 0x13579bdf;
	Chaos_Init(startupSeed);
	Tick(5000);
	CTR_TEST_CHECK(sdata->chaosRng != startupSeed, "scheduled choices advance the generator initialized at startup");
	CTR_TEST_CHECK_EQ(sdata->randomNumber, 0, "Chaos initialization and choices leave retail RNG untouched");
	const struct ChaosSettings settings = sdata->chaosSettings;
	const u32 rng = sdata->chaosRng;
	Chaos_BeginRace(&s_tracker);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "retry cannot inherit running activations");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.intervalRemainingMS, 5000, "retry starts a fresh first interval");
	CTR_TEST_CHECK(memcmp(&settings, &sdata->chaosSettings, sizeof(settings)) == 0, "race setup does not modify creation settings");
	CTR_TEST_CHECK_EQ(sdata->chaosRng, rng, "race reset does not reseed the dedicated generator");
	Chaos_Reset();
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 1000), "a race must initialize before accepting activations");
	Tick(5000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "teardown cannot restart the engine through an update");
	sdata->chaosSettings.armed = false;
	Chaos_BeginRace(&s_tracker);
	CTR_TEST_CHECK(!sdata->chaosRuntime.raceInitialized, "a Regular file never initializes Chaos");
	sdata->chaosSettings.armed = true;
	s_tracker.gameMode1 |= ADVENTURE_ARENA;
	Chaos_BeginRace(&s_tracker);
	CTR_TEST_CHECK(!sdata->chaosRuntime.raceInitialized, "entering a hub does not initialize a Chaos race");

	SetupRace(CHAOS_TRIGGER_INTERVAL);
	memset(sdata->chaosSettings.effects, 0, sizeof(sdata->chaosSettings.effects));
	sdata->chaosSettings.enabledCount = 0;
	Tick(5000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "an empty armed file produces no activation");
	CTR_TEST_CHECK(sdata->chaosSettings.armed, "an empty selection never reclassifies the file as Regular");
	CTR_TEST_CHECK_EQ(sdata->chaosRng, 0, "empty selection consumes no random choice");
	sdata->chaosSettings.triggerIntervalSeconds = 0;
	Chaos_BeginRace(&s_tracker);
	Tick(5000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "a zero saved interval cannot spin a scheduling loop");
	SetupRace(CHAOS_TRIGGER_INTERVAL);
	sdata->chaosSettings.durationSeconds = 0;
	Tick(5000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "zero saved duration cannot create expired entries");
	CTR_TEST_CHECK_EQ(sdata->chaosRng, 0, "invalid duration does not draw RNG");
	for (int trigger = 0; trigger < CHAOS_TRIGGER_COUNT; trigger++)
	{
		if (trigger == CHAOS_TRIGGER_INTERVAL || trigger == CHAOS_TRIGGER_RANDOM)
		{
			continue;
		}
		SetupRace(trigger);
		Tick(5000);
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "contact and external triggers do not fall back to timed scheduling");
	}
}

static void TestPickupTrigger(void)
{
	CTR_TEST_SECTION("Pickup activates selected effects for eligible human contacts only");
	SetupRace(CHAOS_TRIGGER_PICKUP);
	struct Driver human = {.driverID = 0};
	struct Driver partner = {.driverID = 1};
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT};
	CTR_TEST_CHECK(Chaos_OnPickup(&human), "a human pickup activates a selected effect");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].effect, CHAOS_EFFECT_FLY_ME_TO_THE_MOON, "Pickup draws only from enabled effects");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].remainingMS, 30000, "Pickup uses the configured duration");
	Tick(250);
	CTR_TEST_CHECK(Chaos_OnPickup(&partner), "the co-op partner can trigger Pickup");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 1, "another pickup refreshes the same effect without duplicates");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].elapsedMS, 0, "a new pickup restarts the effect phase");
	const u32 rng = sdata->chaosRng;
	CTR_TEST_CHECK(!Chaos_OnPickup(&bot), "bot pickups cannot activate effects");
	CTR_TEST_CHECK(!Chaos_OnPickup(NULL), "missing drivers cannot activate effects");
	human.actionsFlagSet = ACTION_RACE_FINISHED;
	CTR_TEST_CHECK(!Chaos_OnPickup(&human), "finished humans cannot activate effects");
	human.actionsFlagSet = 0;
	const int blocked[] = {PAUSE_1, ADVENTURE_ARENA, LOADING, END_OF_RACE, MAIN_MENU};
	for (unsigned int i = 0; i < len(blocked); i++)
	{
		s_tracker.gameMode1 = ADVENTURE_MODE | blocked[i];
		CTR_TEST_CHECK(!Chaos_OnPickup(&human), "race gates block pickup activation");
	}
	s_tracker.gameMode1 = ARCADE_MODE;
	CTR_TEST_CHECK(!Chaos_OnPickup(&human), "Arcade retains ordinary pickup behavior");
	s_tracker.gameMode1 = ADVENTURE_MODE;
	for (int trigger = 0; trigger < CHAOS_TRIGGER_COUNT; trigger++)
	{
		if (trigger != CHAOS_TRIGGER_PICKUP)
		{
			sdata->chaosSettings.trigger = trigger;
			CTR_TEST_CHECK(!Chaos_OnPickup(&human), "other trigger modes ignore pickup events");
		}
	}
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_PICKUP;
	sdata->chaosSettings.durationSeconds = 0;
	CTR_TEST_CHECK(!Chaos_OnPickup(&human), "zero duration cannot consume a pickup activation");
	sdata->chaosSettings.durationSeconds = 30;
	sdata->chaosSettings.effects[CHAOS_EFFECT_FLY_ME_TO_THE_MOON] = false;
	sdata->chaosSettings.enabledCount = 0;
	CTR_TEST_CHECK(!Chaos_OnPickup(&human), "empty selections cannot activate from pickups");
	CTR_TEST_CHECK_EQ(sdata->chaosRng, rng, "ineligible contacts and invalid settings consume no RNG");
	sdata->chaosSettings.effects[CHAOS_EFFECT_FLY_ME_TO_THE_MOON] = true;
	sdata->chaosSettings.enabledCount = 1;
	const ChaosEffectId fillers[] = {CHAOS_EFFECT_GROUNDED, CHAOS_EFFECT_FAIR_MODE, CHAOS_EFFECT_HOLE_IN_THE_TANK};
	for (unsigned int i = 0; i < len(fillers); i++)
	{
		sdata->chaosSettings.effects[fillers[i]] = true;
		Chaos_Activate(fillers[i], 10000);
		sdata->chaosSettings.effects[fillers[i]] = false;
	}
	CTR_TEST_CHECK(Chaos_OnPickup(&human), "Pickup can refresh a selected effect at capacity");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 4, "Pickup replacement does not exceed capacity");
	sdata->chaosSettings.effects[CHAOS_EFFECT_FLY_ME_TO_THE_MOON] = false;
	sdata->chaosSettings.effects[CHAOS_EFFECT_WUMPA_TAX] = true;
	const struct ChaosRuntime full = sdata->chaosRuntime;
	CTR_TEST_CHECK(!Chaos_OnPickup(&human), "Pickup rejects a fifth unrelated effect");
	CTR_TEST_CHECK(memcmp(&full, &sdata->chaosRuntime, sizeof(full)) == 0, "a rejected pickup preserves all running effects");

	CTR_TEST_SECTION("Turbo pads activate once per continuous traversal and preserve their latch on pause/restore");
	SetupRace(CHAOS_TRIGGER_PICKUP);
	struct Instance inst = {0};
	struct Thread thread = {.inst = &inst, .flags = THREAD_FLAG_DISABLE_COLLISION};
	human = (struct Driver){.driverID = 0, .instSelf = &inst};
	thread.object = &human;
	inst.thread = &thread;
	human.stepFlagSet = COLL_STEP_TRIGGER_TURBO_PAD;
	VehPhysForce_CollideDrivers(&thread, &human);
	CTR_TEST_CHECK_EQ(s_activationSounds, 1, "normal turbo pad physics dispatches a pickup on entry");
	Tick(100);
	VehPhysForce_CollideDrivers(&thread, &human);
	CTR_TEST_CHECK_EQ(s_activationSounds, 1, "remaining on a turbo pad cannot retrigger");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].elapsedMS, 100, "sustained pad contact cannot reset effect timers");
	human.stepFlagSet = COLL_STEP_TRIGGER_SUPER_TURBO_PAD;
	VehPhysForce_CollideDrivers(&thread, &human);
	CTR_TEST_CHECK_EQ(s_activationSounds, 1, "moving between connected normal and super pads is one traversal");
	const struct ChaosRuntime saved = sdata->chaosRuntime;
	s_tracker.gameMode1 |= PAUSE_1;
	Chaos_OnTurboPadContact(&human, false);
	s_tracker.gameMode1 &= ~PAUSE_1;
	Chaos_OnTurboPadContact(&human, true);
	CTR_TEST_CHECK_EQ(s_activationSounds, 1, "pause cannot clear the contact latch");
	human.stepFlagSet = 0;
	VehPhysForce_CollideDrivers(&thread, &human);
	human.stepFlagSet = COLL_STEP_TRIGGER_SUPER_TURBO_PAD;
	VehPhysForce_CollideDrivers(&thread, &human);
	CTR_TEST_CHECK_EQ(s_activationSounds, 2, "leaving then entering a super pad permits a fresh activation");
	sdata->chaosRuntime = saved;
	Chaos_OnTurboPadContact(&human, true);
	CTR_TEST_CHECK_EQ(s_activationSounds, 2, "resident restoration preserves continuous contact");
	Chaos_OnTurboPadContact(&partner, true);
	CTR_TEST_CHECK_EQ(s_activationSounds, 3, "the co-op partner has an independent pad latch");
	Chaos_OnTurboPadContact(&bot, true);
	CTR_TEST_CHECK_EQ(s_activationSounds, 3, "bot pad contact cannot activate effects");
	Chaos_Reset();
	Chaos_BeginRace(&s_tracker);
	Chaos_OnTurboPadContact(&human, true);
	CTR_TEST_CHECK_EQ(s_activationSounds, 4, "race reset clears pad contact latches");
}

static void TestResidentRestore(void)
{
	CTR_TEST_SECTION("restoring resident state resumes timers, activation order and RNG choices");
	SetupRace(CHAOS_TRIGGER_INTERVAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_GOTTA_GO_FAST] = true;
	sdata->chaosSettings.enabledCount++;
	Tick(5000);
	Tick(1234);
	static struct sData saved;
	saved = sdata_static;
	const int deltas[] = {3766, 5000, 5000, 5000, 5000, 5000};
	for (int i = 0; i < (int)len(deltas); i++)
	{
		Tick(deltas[i]);
	}
	const struct ChaosRuntime expected = sdata->chaosRuntime;
	const u32 expectedRng = sdata->chaosRng;
	Chaos_Reset();
	sdata_static = saved;
	for (int i = 0; i < (int)len(deltas); i++)
	{
		Tick(deltas[i]);
	}
	CTR_TEST_CHECK(memcmp(&expected, &sdata->chaosRuntime, sizeof(expected)) == 0, "resident restore reproduces timer expiry, queue order and trigger cadence");
	CTR_TEST_CHECK_EQ(sdata->chaosRng, expectedRng, "resident restore reproduces subsequent effect choices");
	CTR_TEST_CHECK_EQ(sdata->randomNumber, saved.randomNumber, "Chaos scheduling leaves retail RNG untouched");
}

static void TestRandomTrigger(void)
{
	CTR_TEST_SECTION("Random schedules selected gameplay effects with bounded, reproducible gaps");
	EnableInputEffects();
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_RANDOM;
	Chaos_BeginRace(&s_tracker);
	const int firstGap = sdata->chaosRuntime.intervalRemainingMS;
	CTR_TEST_CHECK(firstGap >= 1500 && firstGap <= 5000, "the first Random gap is between 30 and 100 percent of the configured interval");
	Tick(firstGap - 1);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "Random waits for its initial gap instead of activating immediately");
	Tick(1);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 1, "Random activates at its first deadline");
	const ChaosEffectId first = sdata->chaosRuntime.active[0].effect;
	struct Driver driver = {.driverID = 0};
	CTR_TEST_CHECK(Chaos_IsActiveForDriver(first, &driver), "a Random activation reaches the gameplay effect query");
	CTR_TEST_CHECK(first == CHAOS_EFFECT_MIRROR_MIRROR || first == CHAOS_EFFECT_GROUNDED,
	               "Random chooses only the effects selected for this file");
	CTR_TEST_CHECK_EQ(LatestNoticeRemaining(), CHAOS_ACTIVATION_NOTICE_MS + CHAOS_ACTIVATION_DOCK_MS, "Random activation also starts the visual announcement");
	const struct ChaosRuntime beforePause = sdata->chaosRuntime;
	const u32 pausedRng = sdata->chaosRng;
	s_tracker.gameMode1 |= PAUSE_1;
	Tick(5000);
	CTR_TEST_CHECK(memcmp(&beforePause, &sdata->chaosRuntime, sizeof(beforePause)) == 0, "pause preserves the Random deadline and active timers");
	CTR_TEST_CHECK_EQ(sdata->chaosRng, pausedRng, "pause does not redraw the Random gap");
	s_tracker.gameMode1 &= ~PAUSE_1;
	bool varied = false;
	for (int i = 0; i < 32; i++)
	{
		const int gap = sdata->chaosRuntime.intervalRemainingMS;
		CTR_TEST_CHECK(gap >= 1500 && gap <= 5000, "every new Random gap stays within the configured bounds");
		varied |= gap != firstGap;
		int surviving = 0;
		for (int entry = 0; entry < sdata->chaosRuntime.numActive; entry++)
		{
			if (sdata->chaosRuntime.active[entry].remainingMS > gap)
			{
				surviving++;
			}
		}
		const int soundsBefore = s_activationSounds;
		const struct ChaosRuntime before = sdata->chaosRuntime;
		Tick(gap);
		const ChaosEffectId selected = sdata->chaosRuntime.active[sdata->chaosRuntime.numActive - 1].effect;
		bool refreshed = false;
		for (int entry = 0; entry < before.numActive; entry++)
		{
			refreshed |= before.active[entry].effect == selected && before.active[entry].remainingMS > gap;
		}
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, surviving + (refreshed ? 0 : 1), "Random replaces a matching survivor or adds a distinct effect");
		CTR_TEST_CHECK_EQ(s_activationSounds, soundsBefore + 1, "every accepted Random activation announces once");
		CTR_TEST_CHECK_EQ(LatestNoticeRemaining(), CHAOS_ACTIVATION_NOTICE_MS + CHAOS_ACTIVATION_DOCK_MS, "a Random refresh starts a fresh announcement");
		for (int entry = 0; entry < sdata->chaosRuntime.numActive; entry++)
		{
			for (int other = entry + 1; other < sdata->chaosRuntime.numActive; other++)
			{
				CTR_TEST_CHECK(sdata->chaosRuntime.active[entry].effect != sdata->chaosRuntime.active[other].effect,
				    "Random scheduling never creates duplicate effect IDs");
			}
		}
	}
	CTR_TEST_CHECK(varied, "Random does not silently use a fixed Interval cadence");
	const struct ChaosRuntime saved = sdata->chaosRuntime;
	const u32 savedRng = sdata->chaosRng;
	Tick(saved.intervalRemainingMS);
	const struct ChaosRuntime expected = sdata->chaosRuntime;
	const u32 expectedRng = sdata->chaosRng;
	sdata->chaosRuntime = saved;
	sdata->chaosRng = savedRng;
	Tick(saved.intervalRemainingMS);
	CTR_TEST_CHECK(memcmp(&expected, &sdata->chaosRuntime, sizeof(expected)) == 0, "restoring Random state reproduces the next effect and gap");
	CTR_TEST_CHECK_EQ(sdata->chaosRng, expectedRng, "Random uses the captured dedicated RNG stream");
	CTR_TEST_CHECK_EQ(sdata->randomNumber, 0, "Random scheduling leaves retail RNG untouched");

	sdata->chaosSettings.triggerIntervalSeconds = 120;
	Chaos_BeginRace(&s_tracker);
	bool longGap = false;
	for (int i = 0; i < 32; i++)
	{
		const int gap = sdata->chaosRuntime.intervalRemainingMS;
		CTR_TEST_CHECK(gap >= 36000 && gap <= 120000, "long configured intervals retain the full Random range");
		longGap |= gap > 90000;
		Tick(gap);
	}
	CTR_TEST_CHECK(longGap, "15-bit RNG samples are scaled rather than truncating long gaps with a modulus");
}

static void TestActivationNotice(void)
{
	CTR_TEST_SECTION("activation notices use Final Lap styling and a shared full-screen center in 2P");
	EnableInputEffects();
	static struct DB buffer;
	u32 uiOT[5];
	u32 viewportOT[5];
	buffer.otMem.uiOT = uiOT;
	s_tracker.backBuffer = &buffer;
	s_tracker.pushBuffer[0].ptrOT = viewportOT;
	s_tracker.hudFlags = HUD_FLAG_RACE_HUD;
	s_tracker.numPlyrCurrGame = 1;
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 0, "there is no message before an activation");
	Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 10000);
	CTR_TEST_CHECK_EQ(s_activationSounds, 1, "a successful activation plays one notification sound");
	CTR_TEST_CHECK_EQ(s_lastSound, 0x65u, "the activation sound is the transition swish");
	CTR_TEST_CHECK((s_lastSoundFlags & HOWL_SFX_GAIN_DOUBLE_FLAG) != 0, "Chaos requests double gain for its notification");
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 1, "a successful activation submits its notice");
	CTR_TEST_CHECK(strcmp(s_noticeText, "Mirror, Mirror") == 0, "the notice uses the catalogue effect name");
	CTR_TEST_CHECK(s_noticeX >= SCREEN_WIDTH, "the activation name begins its entrance off the right edge");
	Tick(320);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeX, SCREEN_WIDTH / 2, "the name settles horizontally centered");
	CTR_TEST_CHECK_EQ(s_noticeY, SCREEN_HEIGHT / 4, "solo placement follows the Final Lap quarter-height position");
	CTR_TEST_CHECK_EQ(s_noticeFont, FONT_BIG, "the activation name uses the Final Lap font");
	CTR_TEST_CHECK_EQ(s_noticeFlags, JUSTIFY_CENTER | ORANGE, "the activation name uses centered orange text");
	CTR_TEST_CHECK(s_noticeOT == uiOT, "the message uses the full-screen UI ordering table");
	const struct ChaosRuntime beforeDraw = sdata->chaosRuntime;
	DrawNotice();
	CTR_TEST_CHECK(memcmp(&beforeDraw, &sdata->chaosRuntime, sizeof(beforeDraw)) == 0, "drawing cannot advance notification or effect timers");
	CTR_TEST_CHECK_EQ(s_activationSounds, 1, "rendering an ongoing banner does not replay the activation sound");

	s_tracker.numPlyrCurrGame = 2;
	s_tracker.pushBuffer[0].rect = (RECT){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 2};
	s_tracker.pushBuffer[1].rect = (RECT){0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT / 2};
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 1, "2P gets one shared announcement rather than two viewport messages");
	CTR_TEST_CHECK_EQ(s_noticeX, 256, "the 2P announcement is horizontally centered on the whole screen");
	CTR_TEST_CHECK_EQ(s_noticeY, 100, "the 2P announcement matches the central continue-prompt baseline");
	CTR_TEST_CHECK(s_noticeOT == uiOT && s_noticeOT != viewportOT, "2P text is not clipped to either player's OT");

	s_tracker.gameMode1 |= PAUSE_1;
	Tick(1000);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 0, "pause hides the activation announcement");
	CTR_TEST_CHECK_EQ(LatestNoticeRemaining(), 3000, "pause freezes announcement time together with effect timers");
	s_tracker.gameMode1 &= ~PAUSE_1;
	s_tracker.hudFlags = 0;
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 0, "a disabled race HUD hides the announcement");
	s_tracker.hudFlags = HUD_FLAG_RACE_HUD;
	sdata->ptrActiveMenu = &sdata->chaosMenu.menu;
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 0, "menu screens do not receive the race announcement");
	sdata->ptrActiveMenu = NULL;
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 10000);
	CTR_TEST_CHECK_EQ(s_activationSounds, 2, "a second successful activation requests its own swish");
	CTR_TEST_CHECK_EQ(LatestNoticeRemaining(), CHAOS_ACTIVATION_NOTICE_MS + CHAOS_ACTIVATION_DOCK_MS, "a later activation starts its own announcement lifetime");
	Tick(320);
	DrawNotice();
	CTR_TEST_CHECK(strcmp(s_noticeText, "Grounded") == 0, "the newest activation replaces the displayed name");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 2, "replacing the announcement preserves both running effects");
	const struct ChaosRuntime beforeReject = sdata->chaosRuntime;
	Chaos_Activate(CHAOS_EFFECT_FAIR_MODE, 1000);
	CTR_TEST_CHECK_EQ(s_activationSounds, 2, "a rejected activation does not request a sound");
	CTR_TEST_CHECK(memcmp(&beforeReject, &sdata->chaosRuntime, sizeof(beforeReject)) == 0, "a rejected activation cannot replace the current announcement");
	Tick(2840);
	DrawNotice();
	CTR_TEST_CHECK(s_drawX[0] > SCREEN_WIDTH / 2 && s_drawX[0] < UI_CHAOS_LIST_RIGHT, "the newest name moves toward the right-hand table after its initial three seconds");
	CTR_TEST_CHECK_EQ(s_drawFont[0], FONT_SMALL, "the docking name changes to the compact font");
	CTR_TEST_CHECK_EQ(s_drawY[0], UI_CHAOS_LIST_MULTIPLAYER_TOP, "the newest docking effect occupies the high first row");
	Tick(160);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 2, "both running effects remain visible after the announcement finishes");
	CTR_TEST_CHECK_EQ(s_noticeX, UI_CHAOS_LIST_RIGHT, "the persistent names share a right-hand anchor");
	CTR_TEST_CHECK_EQ(s_noticeFlags, JUSTIFY_RIGHT | ORANGE, "persistent rows are right-aligned orange text");
	CTR_TEST_CHECK(Chaos_IsActive(CHAOS_EFFECT_GROUNDED), "notification expiry does not expire the effect itself");
	Chaos_Reset();
	CTR_TEST_CHECK_EQ(LatestNoticeRemaining(), 0, "race reset also clears pending announcement state");
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 0, "race reset removes the persistent list too");
}

static void TestActiveEffectList(void)
{
	CTR_TEST_SECTION("the borderless effect list deduplicates independent timers and removes expired rows");
	EnableInputEffects();
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_EXTERNAL;
	static struct DB buffer;
	u32 uiOT[5];
	buffer.otMem.uiOT = uiOT;
	s_tracker.backBuffer = &buffer;
	s_tracker.hudFlags = HUD_FLAG_RACE_HUD;
	Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 10000);
	Tick(CHAOS_ACTIVATION_NOTICE_MS + CHAOS_ACTIVATION_DOCK_MS);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 1, "a docked effect remains visible without a running notice timer");
	CTR_TEST_CHECK_EQ(s_noticeFont, FONT_SMALL, "docked effects use the compact font");
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 12000);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 2, "a new banner does not hide older active effects");
	CTR_TEST_CHECK_EQ(s_drawFont[0], FONT_SMALL, "the earlier effect stays in its compact table row during a new banner");
	CTR_TEST_CHECK_EQ(s_noticeFont, FONT_BIG, "the newest activation still gets its large announcement");
	Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 15000);
	Tick(CHAOS_ACTIVATION_NOTICE_MS + CHAOS_ACTIVATION_DOCK_MS);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 2, "overlapping copies of one effect share a single row");
	CTR_TEST_CHECK(strcmp(s_drawText[0], "Mirror, Mirror") == 0 && strcmp(s_drawText[1], "Grounded") == 0,
	               "distinct rows follow activation order");
	CTR_TEST_CHECK_EQ(s_drawX[0], s_drawX[1], "all table rows use the same horizontal anchor");
	CTR_TEST_CHECK_EQ(s_drawY[1] - s_drawY[0], UI_CHAOS_LIST_ROW_HEIGHT, "table rows use a fixed vertical spacing");
	Tick(3360);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 2, "expiry of the original copy preserves the later activation's row");
	Tick(5320);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 1, "a fully expired effect no longer occupies a row");
	CTR_TEST_CHECK(strcmp(s_noticeText, "Mirror, Mirror") == 0, "only the surviving effect remains listed");
	CTR_TEST_CHECK_EQ(s_noticeY, UI_CHAOS_LIST_MULTIPLAYER_TOP, "the surviving 2P rows compact without gaps");
	Tick(3000);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 0, "the final expiry empties the table");

	for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
	{
		sdata->chaosSettings.effects[i] = true;
	}
	sdata->chaosSettings.enabledCount = CHAOS_EFFECT_COUNT;
	for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
	{
		if (!ChaosEffect_IsInstant((ChaosEffectId)i))
		{
			Chaos_Activate((ChaosEffectId)i, 30000);
		}
	}
	s_tracker.numPlyrCurrGame = 2;
	s_tracker.pushBuffer[0].rect = (RECT){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 2};
	s_tracker.pushBuffer[1].rect = (RECT){0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT / 2};
	sdata->finalLapTextTimer[0] = 30;
	sdata->finalLapTextTimer[1] = 30;
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, CHAOS_MAX_ACTIVATIONS, "all four accepted simultaneous announcements remain visible");
	for (int i = 0; i < s_noticeDraws; i++)
	{
		CTR_TEST_CHECK(s_drawY[i] + UI_CHAOS_COMPACT_BANNER_ROW_HEIGHT <= SCREEN_HEIGHT,
		               "crowded announcements fit on screen even with both Final Lap messages");
	}
	sdata->finalLapTextTimer[0] = 0;
	sdata->finalLapTextTimer[1] = 0;
	Tick(CHAOS_ACTIVATION_NOTICE_MS + CHAOS_ACTIVATION_DOCK_MS);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, UI_CHAOS_MAX_DOCKED, "the docked table shows at most the four latest distinct effects");
	CTR_TEST_CHECK(s_noticeY + UI_CHAOS_LIST_ROW_HEIGHT <= SCREEN_HEIGHT, "all active docked effects fit within the screen height");
	Chaos_Reset();
	Chaos_BeginRace(&s_tracker);
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 100);
	Tick(100);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 0, "a short-lived effect cannot leave a banner or table row after expiry");
}

static void TestHudVisibilityAndDockLimit(void)
{
	CTR_TEST_SECTION("1P and 2P show all four active effects and hide the HUD when racing ends");
	EnableInputEffects();
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_EXTERNAL;
	for (int effect = 0; effect < CHAOS_EFFECT_COUNT; effect++)
	{
		sdata->chaosSettings.effects[effect] = true;
	}
	sdata->chaosSettings.enabledCount = CHAOS_EFFECT_COUNT;
	static struct DB buffer;
	u32 uiOT[5];
	buffer.otMem.uiOT = uiOT;
	s_tracker.backBuffer = &buffer;
	s_tracker.hudFlags = HUD_FLAG_RACE_HUD;
	s_tracker.numPlyrCurrGame = 2;
	const ChaosEffectId effects[] = {
		CHAOS_EFFECT_MIRROR_MIRROR, CHAOS_EFFECT_GROUNDED,
		CHAOS_EFFECT_FLY_ME_TO_THE_MOON, CHAOS_EFFECT_FAIR_MODE,
	};
	for (int effect = 0; effect < (int)len(effects); effect++)
	{
		Chaos_Activate(effects[effect], effect == 0 ? 5000 : 30000);
	}
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 5000), "an active effect can refresh even when all slots are occupied");
	Tick(CHAOS_ACTIVATION_NOTICE_MS + CHAOS_ACTIVATION_DOCK_MS);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 4, "2P limits the docked list to four distinct effects");
	CTR_TEST_CHECK(strcmp(s_drawText[0], "Mirror, Mirror") == 0, "reactivation moves the effect to the newest row");
	CTR_TEST_CHECK(strcmp(s_drawText[1], "Fair Mode") == 0 && strcmp(s_drawText[3], "Grounded") == 0,
	               "the docked list orders all four accepted effects by activation recency");
	CTR_TEST_CHECK_EQ(s_drawY[0], UI_CHAOS_LIST_MULTIPLAYER_TOP, "the 2P docked list starts near the top edge");
	const int firstPlayerLapCounterY = 71;
	CTR_TEST_CHECK(s_drawY[3] + UI_CHAOS_LIST_ROW_HEIGHT < firstPlayerLapCounterY,
	               "all four 2P docked rows stay above the lap counter");
	Tick(5000 - CHAOS_ACTIVATION_NOTICE_MS - CHAOS_ACTIVATION_DOCK_MS);
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_SUNDAY_DRIVER, 30000), "expiry frees a slot for a new activation");
	Tick(UI_CHAOS_NOTICE_SLIDE_MS);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 4, "a central announcement can coexist with the three surviving docked effects");
	CTR_TEST_CHECK_EQ(s_noticeX, SCREEN_WIDTH / 2, "the new 2P announcement is horizontally centered");
	CTR_TEST_CHECK_EQ(s_noticeY, UI_CHAOS_NOTICE_MULTIPLAYER_Y, "docked effects cannot push a new 2P announcement toward the bottom");
	s_allHumansFinished = true;
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 0, "all humans finishing hides both central and docked messages before results initialization");
	s_allHumansFinished = false;
	s_tracker.gameMode1 |= END_OF_RACE;
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 0, "the results mode also hides central and docked messages");
	s_tracker.gameMode1 &= ~END_OF_RACE;
	s_tracker.numPlyrCurrGame = 1;
	Tick(CHAOS_ACTIVATION_NOTICE_MS);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 4, "1P uses the same four-effect docked limit");
	CTR_TEST_CHECK(strcmp(s_drawText[0], "Sunday Driver") == 0, "the latest newly docked effect occupies the first row");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 4, "the HUD preserves all four running activations");
}

static void TestHudLayeringAndSoloStack(void)
{
	CTR_TEST_SECTION("solo banners stack downward and docked labels keep fixed rows behind lap statistics");
	EnableInputEffects();
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_EXTERNAL;
	static struct DB buffer;
	u32 uiOT[5];
	buffer.otMem.uiOT = uiOT;
	s_tracker.backBuffer = &buffer;
	s_tracker.hudFlags = HUD_FLAG_RACE_HUD;
	s_tracker.numPlyrCurrGame = 1;
	s_tracker.pushBuffer[0].rect = (RECT){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
	Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 20000);
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 20000);
	Tick(UI_CHAOS_NOTICE_SLIDE_MS);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_drawY[1], SCREEN_HEIGHT / 4, "the newest solo banner stays on the normal row");
	CTR_TEST_CHECK_EQ(s_drawY[0], SCREEN_HEIGHT / 4 + UI_CHAOS_BANNER_ROW_HEIGHT,
	                  "the other solo banner goes below the normal row instead of over the item icon");
	Tick(CHAOS_ACTIVATION_NOTICE_MS);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 2, "both effects finish docking");
	sdata->finalLapTextTimer[0] = 30;
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_drawY[0], UI_CHAOS_LIST_TOP, "Final Lap does not move the first solo docked row");
	CTR_TEST_CHECK_EQ(s_drawY[1], UI_CHAOS_LIST_TOP + UI_CHAOS_LIST_ROW_HEIGHT,
	                  "Final Lap does not insert gaps into the docked list");
	s_tracker.numPlyrCurrGame = 2;
	s_tracker.pushBuffer[0].rect = (RECT){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 2};
	s_tracker.pushBuffer[1].rect = (RECT){0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT / 2};
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_drawY[0], 32, "the 2P dock preserves the user's adjusted vertical position even during Final Lap");
	for (int row = 0; row < s_noticeDraws; row++)
	{
		CTR_TEST_CHECK_EQ(s_drawScale[row], FP(0.5), "docked glyphs preserve the user's half-size scale");
		CTR_TEST_CHECK(s_drawOT[row] == &uiOT[UI_HUD_ORDERING_SLOT_BEHIND_HUD],
		               "docked effects use the lower UI layer behind finishing lap statistics");
	}
	sdata->chaosSettings.effects[CHAOS_EFFECT_TOTALLY_SOBER] = true;
	sdata->chaosSettings.enabledCount++;
	Chaos_Activate(CHAOS_EFFECT_TOTALLY_SOBER, 20000);
	Tick(UI_CHAOS_NOTICE_SLIDE_MS);
	DrawNotice();
	CTR_TEST_CHECK(s_noticeOT == uiOT, "central banners retain the normal foreground UI layer");
	Tick(2840);
	DrawNotice();
	CTR_TEST_CHECK(s_drawOT[0] == &uiOT[UI_HUD_ORDERING_SLOT_BEHIND_HUD],
	               "a label already moves behind lap statistics during its docking animation");
}

static void TestBannerBaselineWithDockedEffects(void)
{
	CTR_TEST_SECTION("docked rows do not displace the Final Lap baseline or the shared 2P center");
	EnableInputEffects();
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_EXTERNAL;
	for (int effect = 0; effect < CHAOS_EFFECT_COUNT; effect++)
	{
		sdata->chaosSettings.effects[effect] = true;
	}
	sdata->chaosSettings.enabledCount = CHAOS_EFFECT_COUNT;
	static struct DB buffer;
	u32 uiOT[5];
	buffer.otMem.uiOT = uiOT;
	s_tracker.backBuffer = &buffer;
	s_tracker.hudFlags = HUD_FLAG_RACE_HUD;
	s_tracker.numPlyrCurrGame = 1;
	s_tracker.pushBuffer[0].rect = (RECT){0, 0, SCREEN_WIDTH, 200};
	const ChaosEffectId docked[] = {
		CHAOS_EFFECT_MIRROR_MIRROR, CHAOS_EFFECT_GROUNDED, CHAOS_EFFECT_FLY_ME_TO_THE_MOON,
	};
	for (int effect = 0; effect < (int)len(docked); effect++)
	{
		Chaos_Activate(docked[effect], 30000);
	}
	Tick(CHAOS_ACTIVATION_NOTICE_MS + CHAOS_ACTIVATION_DOCK_MS);
	Chaos_Activate(CHAOS_EFFECT_SUNDAY_DRIVER, 30000);
	Tick(UI_CHAOS_NOTICE_SLIDE_MS);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeY, 50, "a solo banner uses exactly the viewport quarter-height used by Final Lap");
	CTR_TEST_CHECK_EQ(s_drawY[1] - s_drawY[0], 6, "half-size docked labels use the tighter six-pixel row spacing");
	CTR_TEST_CHECK_EQ(s_drawScale[0], FP(0.5), "tighter spacing preserves the user's docked font scale");
	sdata->finalLapTextTimer[0] = 30;
	DrawNotice();
	CTR_TEST_CHECK(s_noticeY >= 50 + UI_CHAOS_FINAL_LAP_HEIGHT,
	               "a visible Final Lap message reserves the normal solo banner row");
	sdata->finalLapTextTimer[0] = 0;
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeY, 50, "the banner returns to the normal row as soon as Final Lap releases it");
	s_tracker.numPlyrCurrGame = 2;
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeY, UI_CHAOS_NOTICE_MULTIPLAYER_Y,
	                  "three docked rows do not shift the shared 2P center");
	CTR_TEST_CHECK_EQ(s_drawY[0], 32, "tighter row spacing preserves the user's 2P dock position");
}

static void TestNotificationRows(void)
{
	CTR_TEST_SECTION("Final Lap, simultaneous activations and docked effects occupy disjoint rows");
	EnableInputEffects();
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_EXTERNAL;
	static struct DB buffer;
	u32 uiOT[5];
	buffer.otMem.uiOT = uiOT;
	s_tracker.backBuffer = &buffer;
	s_tracker.hudFlags = HUD_FLAG_RACE_HUD;
	s_tracker.numPlyrCurrGame = 1;
	s_tracker.pushBuffer[0].rect = (RECT){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
	Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 20000);
	Tick(1000);
	const s32 firstNoticeTime = LatestNoticeRemaining();
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 20000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].noticeRemainingMS, firstNoticeTime,
	                  "a second activation does not replace or restart the first announcement");
	Tick(320);
	sdata->finalLapTextTimer[0] = 1;
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 2, "both recent activations remain visible at once");
	CTR_TEST_CHECK_EQ(s_drawFont[0], FONT_BIG, "the first activation retains its introductory banner");
	CTR_TEST_CHECK_EQ(s_drawFont[1], FONT_BIG, "the second activation also gets a large banner");
	CTR_TEST_CHECK(s_drawY[0] >= SCREEN_HEIGHT / 4 + UI_CHAOS_FINAL_LAP_HEIGHT,
	               "even the last rendered frame of Final Lap reserves its row");
	CTR_TEST_CHECK(s_drawY[0] >= s_drawY[1] + UI_CHAOS_BANNER_ROW_HEIGHT,
	               "concurrent activation banners cannot overlap vertically");
	sdata->finalLapTextTimer[0] = 0;
	Tick(2000);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_drawFont[0], FONT_SMALL, "the older activation docks while the newer one is still announcing");
	CTR_TEST_CHECK_EQ(s_drawScale[0], FP(0.5), "docked text preserves the user-selected half-size font");
	CTR_TEST_CHECK_EQ(s_drawFont[1], FONT_BIG, "docking an older activation does not shrink a newer banner");
	CTR_TEST_CHECK_EQ(s_drawY[1], SCREEN_HEIGHT / 4,
	                  "the solo banner keeps its normal baseline while older effects are docked");

	s_tracker.numPlyrCurrGame = 2;
	s_tracker.pushBuffer[0].rect = (RECT){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 2};
	s_tracker.pushBuffer[1].rect = (RECT){0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, SCREEN_HEIGHT / 2};
	sdata->finalLapTextTimer[0] = 30;
	sdata->finalLapTextTimer[1] = 30;
	DrawNotice();
	const int bottomLapY = SCREEN_HEIGHT / 2 + SCREEN_HEIGHT / 8;
	for (int i = 0; i < s_noticeDraws; i++)
	{
		CTR_TEST_CHECK(s_drawY[i] + UI_CHAOS_FINAL_LAP_HEIGHT <= bottomLapY ||
		               s_drawY[i] >= bottomLapY + UI_CHAOS_FINAL_LAP_HEIGHT,
		               "the shared 2P table avoids player two's Final Lap row");
	}
	const struct ChaosRuntime beforeDraw = sdata->chaosRuntime;
	DrawNotice();
	CTR_TEST_CHECK(memcmp(&beforeDraw, &sdata->chaosRuntime, sizeof(beforeDraw)) == 0,
	               "table layout and drawing do not advance independent notice timers");

	struct Icon lightIcon = {0};
	lightIcon.texLayout.u1 = 24;
	lightIcon.texLayout.v2 = 24;
	s_tracker.trafficLightIcon[0] = &lightIcon;
	const int lightTimers[] = {2880, 960, 0, -480};
	for (int players = 1; players <= 2; players++)
	{
		s_tracker.numPlyrCurrGame = players;
		for (int player = 0; player < players; player++)
		{
			s_tracker.pushBuffer[player].rect = (RECT){0, player * SCREEN_HEIGHT / players, SCREEN_WIDTH, SCREEN_HEIGHT / players};
		}
		for (int timer = 0; timer < (int)len(lightTimers); timer++)
		{
			for (int player = 0; player < players; player++)
			{
				sdata->chaosRuntime.restart[player] = (struct ChaosRestart){.active = true, .timer = lightTimers[timer]};
			}
			DrawNotice();
			for (int player = 0; player < players; player++)
			{
				RECT lights;
				CTR_TEST_CHECK(DotLights_GetBounds(&s_tracker, player, lightTimers[timer], &lights), "visible lights publish their actual viewport bounds");
				for (int row = 0; row < s_noticeDraws; row++)
				{
					const int height = s_drawScale[row] == UI_CHAOS_LIST_SCALE ? UI_CHAOS_LIST_ROW_HEIGHT :
					    (s_drawFont[row] == FONT_BIG ? UI_CHAOS_BANNER_ROW_HEIGHT : UI_CHAOS_COMPACT_BANNER_ROW_HEIGHT);
					CTR_TEST_CHECK(s_drawY[row] + height <= lights.y || s_drawY[row] >= lights.y + lights.h,
					    "banners and docked rows avoid each viewport's traffic lights through red, green and exit");
				}
			}
		}
	}
}

static void TestExternalCommands(void)
{
	CTR_TEST_SECTION("External waits from GO, consumes one command, and preserves pending requests across gates");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	const struct ChaosSettings saved = sdata->chaosSettings;
	s_externalCommand = "fly-me-to-the-moon";
	s_externalReads = 0;
	Tick(0);
	CTR_TEST_CHECK_EQ(s_externalReads, 0, "commands are not read before the initial interval");
	CTR_TEST_CHECK(strcmp(s_externalCatalogue, "fly-me-to-the-moon\n") == 0, "the catalogue contains only enabled stable slugs");
	s_tracker.trafficLightsTimer = 100;
	Tick(5000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.intervalRemainingMS, 5000, "countdown does not spend the initial interval");
	s_tracker.trafficLightsTimer = 0;
	Tick(4999);
	CTR_TEST_CHECK_EQ(s_externalReads, 0, "the whole initial interval must elapse");
	Tick(1);
	CTR_TEST_CHECK(s_externalCommand == NULL, "activation consumes the pending command");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].remainingMS, 30000, "external activation receives the full duration");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.intervalRemainingMS, 5000, "successful activation restarts a full interval");
	s_externalCommand = "fly-me-to-the-moon";
	s_tracker.gameMode1 |= PAUSE_ALL;
	Tick(5000);
	CTR_TEST_CHECK(s_externalCommand != NULL, "pause preserves the next request");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.intervalRemainingMS, 5000, "pause freezes the activation interval");
	s_tracker.gameMode1 &= ~PAUSE_ALL;
	Tick(5001);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 1, "external reactivation preserves unique IDs");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].remainingMS, 30000, "reactivation refreshes the saved duration");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.intervalRemainingMS, 5000, "frame overshoot cannot shorten the next interval");

	Tick(5000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.intervalRemainingMS, 0, "an empty slot leaves the provider ready");
	const char *invalid[] = {"", "missing", "big-kart-energy", "fly-me-to-the-moon 2", "effect fly-me-to-the-moon", "reset", "status"};
	s_externalErrors = 0;
	for (size_t i = 0; i < len(invalid); i++)
	{
		s_externalCommand = invalid[i];
		Tick(1);
		CTR_TEST_CHECK(s_externalCommand == NULL, "invalid and disabled requests are consumed");
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.intervalRemainingMS, 0, "rejections do not restart the interval");
	}
	CTR_TEST_CHECK_EQ(s_externalErrors, len(invalid), "each rejected command is logged");
	s_externalCommand = "fly-me-to-the-moon";
	s_tracker.overlayIndex_Threads = OVERLAY_INDEX_ADV_HUB;
	Tick(1);
	CTR_TEST_CHECK(s_externalCommand != NULL, "hub transitions preserve pending commands");
	s_tracker.overlayIndex_Threads = OVERLAY_INDEX_RACING_OR_BATTLE;
	Chaos_BeginRace(&s_tracker);
	Tick(0);
	CTR_TEST_CHECK(s_externalCommand != NULL, "a new race starts a fresh interval with the same pending command");
	Tick(5000);
	CTR_TEST_CHECK(s_externalCommand == NULL, "the pending command can run in the next race");
	CTR_TEST_CHECK(memcmp(&saved, &sdata->chaosSettings, sizeof(saved)) == 0, "provider commands never change creation settings");
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_INTERVAL;
	s_externalCommand = "fly-me-to-the-moon";
	Tick(1);
	CTR_TEST_CHECK(s_externalCommand != NULL, "other triggers leave the external slot untouched");
	sdata->chaosSettings.effects[CHAOS_EFFECT_FLY_ME_TO_THE_MOON] = false;
	Chaos_BeginRace(&s_tracker);
	Tick(1);
	CTR_TEST_CHECK(s_externalCatalogue[0] == '\0', "empty selections replace the previous catalogue with an empty list");
	sdata->chaosSettings.armed = false;
	sdata->chaosSettings.effects[CHAOS_EFFECT_FLY_ME_TO_THE_MOON] = true;
	Chaos_BeginRace(&s_tracker);
	Tick(1);
	CTR_TEST_CHECK(s_externalCatalogue[0] == '\0', "leaving Chaos clears stale available effects");
	s_externalCommand = NULL;
}

static void TestCataloguePublicationLifecycle(void)
{
	CTR_TEST_SECTION("catalogue publication occurs only during initialization");
	s_cataloguePublications = 0;
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	CTR_TEST_CHECK_EQ(s_cataloguePublications, 1, "initialization publishes the active selection once");
	Tick(0);
	Tick(1);
	CTR_TEST_CHECK_EQ(s_cataloguePublications, 1, "frame updates never publish the catalogue");
	sdata->chaosSettings.effects[CHAOS_EFFECT_BIG_KART_ENERGY] = true;
	Chaos_BeginRace(&s_tracker);
	Tick(0);
	CTR_TEST_CHECK_EQ(s_cataloguePublications, 2, "the next initialization publishes its selection exactly once");
	CTR_TEST_CHECK(strstr(s_externalCatalogue, "big-kart-energy\n") != NULL, "the new publication reflects the new profile");
	s_tracker.gameMode1 |= ADVENTURE_ARENA;
	Chaos_BeginRace(&s_tracker);
	CTR_TEST_CHECK(s_externalCatalogue[0] == '\0', "hubs publish no applicable race commands");
}

static void TestBotEffectEventScope(void)
{
	CTR_TEST_SECTION("bot effects are filtered by event type, never by live bot availability");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_ARENA_CHAMPIONS] = true;
	sdata->chaosSettings.enabledCount++;
	sdata->chaosSettings.effects[CHAOS_EFFECT_YOUVE_GOT_MAIL] = true;
	sdata->chaosSettings.enabledCount++;
	Chaos_PublishEffects();
	CTR_TEST_CHECK(strstr(s_externalCatalogue, "arena-champions\n") != NULL, "race publication includes Arena Champions without inspecting the field");
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_ARENA_CHAMPIONS, 1000), "normal races accept Arena Champions even without registered bots");
	CTR_TEST_CHECK(strstr(s_externalCatalogue, "youve-got-mail\n") != NULL, "race publication includes You've Got Mail without inspecting the field");
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_YOUVE_GOT_MAIL, 1000), "normal races accept You've Got Mail even without registered bots");
	struct Driver finishedBot = {.driverID = 2, .actionsFlagSet = ACTION_BOT | ACTION_RACE_FINISHED};
	s_tracker.drivers[2] = &finishedBot;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_ARENA_CHAMPIONS, 1000), "finished bots do not prevent activating Arena Champions");
	const int botlessEvents[] = {RELIC_RACE, CRYSTAL_CHALLENGE, TIME_TRIAL, BATTLE_MODE};
	for (int i = 0; i < (int)len(botlessEvents); i++)
	{
		s_tracker.gameMode1 = ADVENTURE_MODE | botlessEvents[i];
		Chaos_BeginRace(&s_tracker);
		CTR_TEST_CHECK(strcmp(s_externalCatalogue, "fly-me-to-the-moon\n") == 0, "botless event types omit Arena Champions from publication");
		CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_ARENA_CHAMPIONS, 1000), "botless event types reject Arena Champions requests");
		CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_YOUVE_GOT_MAIL, 1000), "botless event types reject You've Got Mail requests");
		sdata->chaosSettings.trigger = CHAOS_TRIGGER_PICKUP;
		CTR_TEST_CHECK(Chaos_OnPickup(s_tracker.drivers[0]), "event filtering retains human pickup effects");
		CTR_TEST_CHECK(Chaos_IsActive(CHAOS_EFFECT_FLY_ME_TO_THE_MOON), "pickup selection draws from the event's eligible enabled effects");
		sdata->chaosSettings.trigger = CHAOS_TRIGGER_INTERVAL;
		sdata->chaosRuntime.intervalRemainingMS = 1;
		Tick(1);
		CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_ARENA_CHAMPIONS), "interval selection excludes bot effects in botless event types");
	}
	s_tracker.drivers[2] = NULL;
	Chaos_Reset();
}

static void TestBodySwapPlayerCount(void)
{
	CTR_TEST_SECTION("Body Swap is available only in multiplayer events");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	memset(sdata->chaosSettings.effects, 0, sizeof(sdata->chaosSettings.effects));
	sdata->chaosSettings.effects[CHAOS_EFFECT_BODY_SWAP] = true;
	sdata->chaosSettings.enabledCount = 1;
	s_tracker.numPlyrCurrGame = 1;
	Chaos_PublishEffects();
	CTR_TEST_CHECK(strstr(s_externalCatalogue, "body-swap\n") == NULL, "single-player catalogue excludes Body Swap");
	CTR_TEST_CHECK(Chaos_PickEventEffect() == CHAOS_EFFECT_NONE, "random selection excludes Body Swap in single-player");
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_BODY_SWAP, 1000), "single-player rejects direct activation");
	CTR_TEST_CHECK(!Chaos_ActivateExternalEffect("body-swap"), "single-player rejects external commands");
	s_tracker.numPlyrCurrGame = 2;
	Chaos_PublishEffects();
	CTR_TEST_CHECK(strstr(s_externalCatalogue, "body-swap\n") != NULL, "multiplayer catalogue includes Body Swap");
	CTR_TEST_CHECK(Chaos_PickEventEffect() == CHAOS_EFFECT_BODY_SWAP, "random selection includes Body Swap in multiplayer");
}

static void TestLocalizedVoiceFallback(void)
{
	CTR_TEST_SECTION("localized voice fallback preserves event");
	SetupRace(CHAOS_TRIGGER_INTERVAL);
	s16 original[] = {16};
	s16 pal[] = {0, 1};
	data.voiceID[21] = 8;
	data.voiceData[0].voiceSet[8].ptr = original;
	data.voiceData[0].voiceSet[8].num = 1;
	data.voiceData[0].voiceSet[0].ptr = pal;
	data.voiceData[0].voiceSet[0].num = 2;
	struct VoicelineItem line = {.characterID = 0, .voiceID = 21};
	g_config.voiceLanguage = 5;
	s_testLocalizedVoiceID = -1;
	s_testFallbackVoiceID = -1;
	s_testLocalizedVoiceSucceeds = false;
	Voiceline_StartPlay(&line.item);
	CTR_TEST_CHECK(s_testLocalizedVoiceID == 0 || s_testLocalizedVoiceID == 1, "PAL set 0 is tried first");
	CTR_TEST_CHECK_EQ(s_testFallbackVoiceID, 16, "unavailable PAL audio falls back to the original English event");
	s_testFallbackVoiceID = -1;
	s_testLocalizedVoiceSucceeds = true;
	Voiceline_StartPlay(&line.item);
	CTR_TEST_CHECK_EQ(s_testFallbackVoiceID, -1, "a playable PAL event does not start English audio");
}

int main(void)
{
	TestBodySwapPlayerCount();
	TestLocalizedVoiceFallback();
	TestBotEffectEventScope();
	TestCataloguePublicationLifecycle();
	TestExternalCommands();
	TestIntervalCadence();
	TestIndependentExpiryAndOrder();
	TestGatesAndPause();
	TestCapacityAndValidation();
	TestResetAndInvalidSettings();
	TestPickupTrigger();
	TestResidentRestore();
	TestRandomTrigger();
	TestActivationNotice();
	TestActiveEffectList();
	TestHudVisibilityAndDockLimit();
	TestHudLayeringAndSoloStack();
	TestBannerBaselineWithDockedEffects();
	TestNotificationRows();
	return CtrTest_Summary();
}
