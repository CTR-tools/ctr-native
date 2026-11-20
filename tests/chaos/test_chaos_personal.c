#include <chaos/chaos_fixture.h>

// Exercise the internal player-pool entry point before personal triggers and
// multi-driver effects are wired into the menu. The public API remains shared.
static struct Driver s_humans[CHAOS_MAX_HUMAN_PLAYERS];

static void SetupPersonalWorld(int mode, int players)
{
	SetupRace(CHAOS_TRIGGER_PICKUP);
	s_tracker.gameMode1 = mode;
	s_tracker.numPlyrCurrGame = players;
	sdata->chaosSettings.distribution = CHAOS_DISTRIBUTION_PER_PLAYER;
	memset(s_humans, 0, sizeof(s_humans));
	for (int player = 0; player < players; player++)
	{
		s_humans[player].driverID = player;
		s_humans[player].const_Gravity = 100;
		// Reserve gifts need no asset-backed ignition when a fire is already lit.
		s_humans[player].reserves = 1;
		s_humans[player].fireSpeedCap = 1;
		s_tracker.drivers[player] = &s_humans[player];
	}
	for (int effect = 0; effect < CHAOS_EFFECT_COUNT; effect++)
	{
		sdata->chaosSettings.effects[effect] = true;
	}
	sdata->chaosSettings.enabledCount = CHAOS_EFFECT_COUNT;
}

static void TestPersonalIsolation(void)
{
	CTR_TEST_SECTION("personal activations and coincident effects are isolated across 1-4 controllers");
	const int modes[] = {ARCADE_MODE, 0, BATTLE_MODE, ARCADE_MODE | RELIC_RACE,
	                     ARCADE_MODE | CRYSTAL_CHALLENGE};
	for (int mode = 0; mode < (int)len(modes); mode++)
	{
		for (int players = 1; players <= CHAOS_MAX_HUMAN_PLAYERS; players++)
		{
			SetupPersonalWorld(modes[mode], players);
			for (int owner = 0; owner < players; owner++)
			{
				CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 1000 + owner * 100, owner),
				               "each controller can receive the same effect independently");
				for (int player = 0; player < players; player++)
				{
					CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&s_humans[player]), player <= owner ? 25 : 100,
					                  "gravity changes only for controllers with their own activation");
				}
			}
			for (int player = 0; player < players; player++)
			{
				CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].numActive, 1, "coincident effects occupy separate player pools");
			}
			Tick(1000);
			CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&s_humans[0]), 100, "the first owner's effect expires independently");
			CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[0].numActive, 0, "expiry clears only the first player's pool");
			for (int player = 1; player < players; player++)
			{
				CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&s_humans[player]), 25, "another owner's effect survives the first expiry");
				CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].active[0].remainingMS, player * 100,
				                  "each remaining timer keeps its original duration");
			}
		}
	}
}

static void TestPersonalConflictsAndRefresh(void)
{
	CTR_TEST_SECTION("opposites and refresh replace only the same recipient's activations");
	SetupPersonalWorld(BATTLE_MODE, 4);
	for (int player = 0; player < 4; player++)
	{
		CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 1000, player), "every player starts with low gravity");
	}
	Tick(100);
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_HEAVY_METAL, 2000, 1), "one player can replace low gravity with heavy gravity");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[1].numActive, 1, "a personal opposite replaces an entry within its own pool");
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&s_humans[1]), 300, "only the recipient receives heavy gravity");
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&s_humans[0]), 25, "another recipient retains low gravity");
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 3000, 2), "refresh accepts an existing owner's effect");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[2].numActive, 1, "refresh never adds a duplicate entry to its pool");
	Tick(900);
	CTR_TEST_CHECK(sdata->chaosRuntime.players[0].numActive == 0 && sdata->chaosRuntime.players[3].numActive == 0,
	               "unrefreshed owners expire at their own boundary");
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&s_humans[1]), 300, "the opposite keeps its own replacement timer");
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&s_humans[2]), 25, "the refreshed copy survives the other owners' expiry");
}

static void TestPersonalInputAndOneOff(void)
{
	CTR_TEST_SECTION("input and instant gifts use controller ownership rather than a global active flag");
	SetupPersonalWorld(0, 4);
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_MIRROR_MIRROR, 1000, 3), "player four can receive mirrored input");
	for (int player = 0; player < 4; player++)
	{
		struct GamepadBuffer pad = {.stickLX = 100};
		Chaos_ApplyInputRules(&s_humans[player], &pad);
		CTR_TEST_CHECK_EQ(pad.stickLX, player == 3 ? 156 : 100, "input is changed only in the targeted controller slot");
	}
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_RESERVE_INJECTION, 1000, 1), "a personal instant gift can activate");
	for (int player = 0; player < 4; player++)
	{
		CTR_TEST_CHECK_EQ(s_humans[player].reserves, player == 1 ? CHAOS_INJECTION_RESERVES : 1,
		                  "an instant reserve injection benefits only its owner");
	}
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_RESERVE_INJECTION, 1000, 2), "a coincident gift has its own notice");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[1].numInstantNotices, 1, "another player's gift preserves the first notice pool");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[2].numInstantNotices, 1, "the coincident gift has a separate notice pool");
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_RESERVE_INJECTION, 1000, 1), "a repeat gift refreshes its own notice");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[1].numInstantNotices, 1, "refresh replaces the same owner's notice only");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[2].numInstantNotices, 1, "refresh leaves the other owner's notice intact");
}

static void TestPersonalCapacity(void)
{
	CTR_TEST_SECTION("each owner retains four timed slots without taking another player's budget");
	SetupPersonalWorld(BATTLE_MODE, 4);
	const ChaosEffectId effects[] = {CHAOS_EFFECT_FLY_ME_TO_THE_MOON, CHAOS_EFFECT_MIRROR_MIRROR,
	                                 CHAOS_EFFECT_SABOTAGE, CHAOS_EFFECT_WUMPA_TAX};
	for (int player = 0; player < 4; player++)
	{
		for (int effect = 0; effect < (int)len(effects); effect++)
		{
			CTR_TEST_CHECK(Chaos_ActivateForPlayer(effects[effect], 1000, player), "a player's four nonconflicting timed effects fit");
		}
		CTR_TEST_CHECK(!Chaos_ActivateForPlayer(CHAOS_EFFECT_FAIR_MODE, 1000, player), "a fifth timed effect cannot exceed its owner's budget");
		CTR_TEST_CHECK(Chaos_ActivateForPlayer(effects[0], 2000, player), "an owner can refresh a timed effect at capacity");
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].numActive, CHAOS_MAX_ACTIVATIONS, "each pool accommodates four personal activations");
	}
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_RESERVE_INJECTION, 1000, 0), "instant gifts do not need timed capacity");
	Chaos_Reset();
	for (int player = 0; player < 4; player++)
	{
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].numActive, 0, "reset clears every personal timed pool");
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].numInstantNotices, 0, "reset clears every personal notice pool");
	}
}

static void TestPersonalScaleAndCamera(void)
{
	CTR_TEST_SECTION("scale cleanup and camera overrides remain local to their recipients");
	SetupPersonalWorld(0, 4);
	for (int player = 0; player < 4; player++)
	{
		s_humans[player].kartScale = CHAOS_KART_SCALE_NORMAL;
		s_tracker.cameraDC[player].driverToFollow = &s_humans[player];
		s_tracker.cameraDC[player].pushBuffer = &s_tracker.pushBuffer[player];
	}
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_BIG_KART_ENERGY, 100, 0), "one player receives giant scale");
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_SMALL_KART_ENERGY, 200, 1), "another player receives small scale without cancelling the giant");
	CTR_TEST_CHECK(s_humans[0].kartScale == CHAOS_GIANT_SCALE && s_humans[1].kartScale == s_chaosSmallScale,
	               "opposite scales can coexist on different controllers");
	Tick(100);
	CTR_TEST_CHECK(s_humans[0].kartScale == CHAOS_KART_SCALE_NORMAL && s_humans[1].kartScale == s_chaosSmallScale,
	               "giant expiry does not reset another player's small kart");
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_CLASSIC_GTA, 1000, 2), "player three receives a personal top-down camera");
	for (int player = 0; player < 4; player++)
	{
		CTR_TEST_CHECK(Chaos_ApplyCameraRules(&s_tracker.cameraDC[player]) == (player == 2),
		               "camera overrides apply only to the activation's controller viewport");
	}
	s_tracker.gameMode1 |= PAUSE_1;
	CTR_TEST_CHECK(Chaos_ApplyCameraRules(&s_tracker.cameraDC[2]), "the targeted camera retains its view while paused");
	CTR_TEST_CHECK(!Chaos_ApplyCameraRules(&s_tracker.cameraDC[0]), "pause does not broaden a personal camera override");
	s_tracker.gameMode1 &= ~PAUSE_1;
	Tick(100);
	CTR_TEST_CHECK(s_humans[1].kartScale == CHAOS_KART_SCALE_NORMAL, "the small kart resets at its independent expiry");
}

static void TestPersonalNoticeCapacity(void)
{
	CTR_TEST_SECTION("notice overflow drops only the oldest banner belonging to the same owner");
	SetupPersonalWorld(0, 4);
	const ChaosEffectId gifts[] = {CHAOS_EFFECT_USE_THIS, CHAOS_EFFECT_JUICED_UP, CHAOS_EFFECT_IT_IS_I,
	                               CHAOS_EFFECT_PROVIDENCE, CHAOS_EFFECT_RESERVE_INJECTION};
	// Test queue policy without requiring asset-backed shield or mask births.
	for (int player = 0; player < 4; player++)
	{
		for (int gift = 0; gift < (int)len(gifts); gift++)
		{
			const struct ChaosActivation notice = {
				.effect = gifts[gift],
				.noticeRemainingMS = CHAOS_ACTIVATION_NOTICE_MS,
			};
			Chaos_EnqueueInstantNotice(&notice, player);
		}
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].numInstantNotices, CHAOS_MAX_INSTANT_NOTICES,
		                  "each owner adds a full independent notice budget");
	}
	for (int player = 0; player < 4; player++)
	{
		const struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
		CTR_TEST_CHECK_EQ(pool->numInstantNotices, CHAOS_MAX_INSTANT_NOTICES, "overflow preserves every other player's notice pool");
		for (int index = 0; index < pool->numInstantNotices; index++)
		{
			CTR_TEST_CHECK_EQ(pool->instantNotices[index].effect, gifts[index + 1], "only that player's oldest instant gift is dropped");
		}
	}
}

static void TestSlotOwnershipAndRestore(void)
{
	CTR_TEST_SECTION("personal effects follow controller slots and survive resident snapshots");
	SetupPersonalWorld(0, 2);
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 1000, 0), "the first slot receives low gravity");
	// Body Swap changes driverID and the controller-to-kart mapping in this way.
	s_humans[0].driverID = 1;
	s_humans[1].driverID = 0;
	s_tracker.drivers[0] = &s_humans[1];
	s_tracker.drivers[1] = &s_humans[0];
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&s_humans[1]), 25, "the newly controlled body inherits the slot's effect");
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&s_humans[0]), 100, "the old physical body does not retain a controller effect");
	static struct sData snapshot;
	memcpy(&snapshot, &sdata_static, sizeof(snapshot));
	Tick(1000);
	memcpy(&sdata_static, &snapshot, sizeof(snapshot));
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&s_humans[1]), 25, "restoring SDATA restores ownership along with duration");
	Tick(999);
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&s_humans[1]), 25, "restored expiry resumes at its captured time");
	Tick(1);
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&s_humans[1]), 100, "restored expiry releases only its controller slot");
}

static void TestInvalidOwners(void)
{
	CTR_TEST_SECTION("unavailable controller recipients cannot create personal activations");
	SetupPersonalWorld(0, 2);
	CTR_TEST_CHECK(!Chaos_ActivateForPlayer(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 1000, 2), "an unselected player is rejected");
	CTR_TEST_CHECK(!Chaos_ActivateForPlayer(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 1000, 255), "an out-of-range player cannot address a nonexistent controller slot");
	CTR_TEST_CHECK(!Chaos_ActivateForPlayer(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 1000, -1), "a negative player index is rejected before accessing a pool");
	s_humans[1].actionsFlagSet = ACTION_BOT;
	CTR_TEST_CHECK(!Chaos_ActivateForPlayer(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 1000, 1), "a bot cannot own a personal activation");
	s_humans[1].actionsFlagSet = 0;
	s_humans[1].actionsFlagSet |= ACTION_RACE_FINISHED;
	CTR_TEST_CHECK(!Chaos_ActivateForPlayer(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 1000, 1), "a finished recipient is rejected");
	s_tracker.drivers[1] = NULL;
	CTR_TEST_CHECK(!Chaos_ActivateForPlayer(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 1000, 1), "a missing recipient is rejected");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[1].numActive, 0, "invalid recipients never consume timed storage");
}

static void TestSharedEntryPoint(void)
{
	CTR_TEST_SECTION("the existing shared entry point still targets and announces all humans");
	EnableInputEffects();
	static struct DB buffer;
	u32 uiOT[5] = {0};
	buffer.otMem.uiOT = uiOT;
	s_tracker.backBuffer = &buffer;
	s_tracker.hudFlags = HUD_FLAG_RACE_HUD;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 1000), "the public activation entry point remains shared");
	for (int player = 0; player < s_tracker.numPlyrCurrGame; player++)
	{
		CTR_TEST_CHECK(Chaos_IsActiveForDriver(CHAOS_EFFECT_MIRROR_MIRROR, s_tracker.drivers[player]), "a shared activation still reaches every human");
	}
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, 1, "a shared effect still produces one global announcement");
}

static void TestGlobalEffectsAcrossPools(void)
{
	CTR_TEST_SECTION("bot effects keep global targets while their activations live in the triggering player's pool");
	SetupPersonalWorld(ARCADE_MODE, 4);
	g_config.extendedMultiplayer = true;
	struct Driver bot = {.driverID = 4, .actionsFlagSet = ACTION_BOT};
	s_tracker.drivers[4] = &bot;
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_ARENA_CHAMPIONS, 1000, 2), "player three can trigger a bot effect");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[2].numActive, 1, "the bot effect occupies the triggering player's pool");
	CTR_TEST_CHECK(Chaos_IsActive(CHAOS_EFFECT_ARENA_CHAMPIONS), "the global active query searches beyond the first pool");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&bot, 100), 175, "a bot receives a global effect triggered from a later pool");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&s_humans[2], 100), 100, "the triggering player does not inherit a bot effect");
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_ARENA_CHAMPIONS, 2000, 3), "another player can refresh a global bot effect");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[2].numActive, 0, "global replacement removes the previous pool's copy");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[3].numActive, 1, "the replacement resides in the new triggering pool");
	Tick(1000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&bot, 100), 175, "the old pool's deadline cannot expire its global replacement");
	Tick(1000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&bot, 100), 100, "the global replacement expires from its own pool");

	const struct ChaosActivation notice = {.effect = CHAOS_EFFECT_KING_CRIMSON, .noticeRemainingMS = CHAOS_ACTIVATION_NOTICE_MS};
	Chaos_EnqueueInstantNotice(&notice, 2);
	CTR_TEST_CHECK(Chaos_UsesTimePalette(), "world palette queries search instant notices in every pool");
	Chaos_EnqueueInstantNotice(&notice, 3);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[2].numInstantNotices, 0, "a global notice is deduplicated across pools");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[3].numInstantNotices, 1, "the new global notice belongs to its triggering pool");
}

static void TestPoolTimersAndRestore(void)
{
	CTR_TEST_SECTION("per-player timer storage and pool-local compaction survive resident snapshots");
	SetupPersonalWorld(0, 4);
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_RANDOM;
	Chaos_Init(12345);
	Chaos_BeginRace(&s_tracker);
	for (int player = 0; player < 4; player++)
	{
		const s32 gap = sdata->chaosRuntime.players[player].intervalRemainingMS;
		CTR_TEST_CHECK(gap >= 1500 && gap <= 5000, "each pool initializes its own bounded Random gap");
	}
	CTR_TEST_CHECK(sdata->chaosRuntime.players[0].intervalRemainingMS != sdata->chaosRuntime.players[1].intervalRemainingMS,
	               "Random initializes separate clocks rather than copying the first pool's sample");
	// Personal trigger scheduling remains a later implementation step.
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_PICKUP;
	for (int player = 0; player < 4; player++)
	{
		CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 100 + player * 100, player),
		               "every pool has its own activation before capture");
	}
	static struct sData snapshot;
	memcpy(&snapshot, &sdata_static, sizeof(snapshot));
	Tick(1000);
	memcpy(&sdata_static, &snapshot, sizeof(snapshot));
	for (int player = 0; player < 4; player++)
	{
		CTR_TEST_CHECK(memcmp(&sdata->chaosRuntime.players[player], &snapshot.chaosRuntime.players[player],
		                      sizeof(struct ChaosPlayerRuntime)) == 0, "restoration captures each complete player pool including its trigger clock");
	}
	Tick(100);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[0].numActive, 0, "compaction empties just the first player's expired pool");
	for (int player = 1; player < 4; player++)
	{
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].active[0].remainingMS, player * 100,
		                  "other pools retain their indices and independent timers after compaction");
	}
}

static void TestSharedPoolSelection(void)
{
	CTR_TEST_SECTION("shared and Adventure rules use only the first pool");
	EnableInputEffects();
	sdata->chaosSettings.distribution = CHAOS_DISTRIBUTION_PER_PLAYER;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 1000), "Adventure keeps shared activation even with stale personal settings");
	CTR_TEST_CHECK_EQ(Chaos_PlayerPoolCount(), 1, "Adventure always selects the single shared pool");
	for (int player = 0; player < s_tracker.numPlyrCurrGame; player++)
	{
		CTR_TEST_CHECK(Chaos_IsActiveForDriver(CHAOS_EFFECT_MIRROR_MIRROR, s_tracker.drivers[player]),
		               "the first pool represents every human in shared mode");
	}
	CTR_TEST_CHECK(!Chaos_ActivateForPlayer(CHAOS_EFFECT_GROUNDED, 1000, 1), "a shared event cannot activate a second pool");
	// Unused pools cannot influence a shared event, even in a dirty snapshot.
	sdata->chaosRuntime.players[3].active[0] = (struct ChaosActivation){.effect = CHAOS_EFFECT_ZA_WARUDO, .remainingMS = 1000};
	sdata->chaosRuntime.players[3].numActive = 1;
	CTR_TEST_CHECK(!Chaos_IsTimeStopped(), "shared world queries ignore unused personal pools");
	Chaos_BeginRace(&s_tracker);
	for (int player = 1; player < 4; player++)
	{
		const struct ChaosPlayerRuntime empty = {0};
		CTR_TEST_CHECK(memcmp(&sdata->chaosRuntime.players[player], &empty, sizeof(empty)) == 0,
		               "starting a shared race clears all unused personal pools and clocks");
	}
}

static void TestTimeStopAcrossPools(void)
{
	CTR_TEST_SECTION("world time stop suspends activation and notice clocks in every player pool");
	SetupPersonalWorld(0, 4);
	for (int player = 0; player < 4; player++)
	{
		CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 1000, player), "each pool has a duration to suspend");
		CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_RESERVE_INJECTION, 1000, player), "each pool has a notice to suspend");
	}
	// Personal Za Warudo activation is still gated. Seed its entry to verify the
	// common pool traversal independently of the pending immunity implementation.
	struct ChaosPlayerRuntime *timePool = &sdata->chaosRuntime.players[1];
	timePool->active[timePool->numActive++] = (struct ChaosActivation){.effect = CHAOS_EFFECT_ZA_WARUDO, .remainingMS = 50};
	CTR_TEST_CHECK(Chaos_IsTimeStopped(), "world time stop is detected outside the first pool");
	Tick(50);
	CTR_TEST_CHECK(!Chaos_IsTimeStopped(), "the time-stop entry expires in its containing pool");
	for (int player = 0; player < 4; player++)
	{
		const struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
		CTR_TEST_CHECK_EQ(pool->active[0].remainingMS, 1000, "the expiry frame stays frozen even for pools visited after time stop is removed");
		CTR_TEST_CHECK_EQ(pool->instantNotices[0].elapsedMS, 0, "instant notices remain suspended in every pool");
	}
	Tick(10);
	for (int player = 0; player < 4; player++)
	{
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].active[0].remainingMS, 990, "each pool resumes on the following unfrozen frame");
	}
}

int main(void)
{
	TestPersonalIsolation();
	TestPersonalConflictsAndRefresh();
	TestPersonalInputAndOneOff();
	TestPersonalCapacity();
	TestPersonalScaleAndCamera();
	TestPersonalNoticeCapacity();
	TestSlotOwnershipAndRestore();
	TestInvalidOwners();
	TestSharedEntryPoint();
	TestGlobalEffectsAcrossPools();
	TestPoolTimersAndRestore();
	TestSharedPoolSelection();
	TestTimeStopAcrossPools();
	return CtrTest_Summary();
}
