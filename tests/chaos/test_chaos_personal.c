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
	CTR_TEST_CHECK(!Chaos_ActivateForPlayer(CHAOS_EFFECT_ARENA_CHAMPIONS, 2000, 3), "another player cannot refresh a running bot effect");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[2].numActive, 1, "rejection preserves the original bot activation");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[3].numActive, 0, "rejection consumes no slot in another pool");
	Tick(1000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&bot, 100), 100, "the bot effect expires at its original deadline");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.botCooldownMS, CHAOS_BOT_COOLDOWN_MS, "expiry starts the shared bot cooldown");

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
	// Isolate activation expiry from automatic trigger scheduling.
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
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_ZA_WARUDO, 50, 1), "a real personal activation starts time stop outside the first pool");
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

static void SelectPersonalEffect(ChaosEffectId effect)
{
	memset(sdata->chaosSettings.effects, 0, sizeof(sdata->chaosSettings.effects));
	sdata->chaosSettings.effects[effect] = true;
	sdata->chaosSettings.enabledCount = 1;
}

static void TestPersonalTimeStopContention(void)
{
	CTR_TEST_SECTION("personal time stop rejects concurrent activations and resumes exact clocks after expiry");
	for (int humans = 1; humans <= CHAOS_MAX_HUMAN_PLAYERS; humans++)
	{
		for (int owner = 0; owner < humans; owner++)
		{
			SetupPersonalWorld(ARCADE_MODE, humans);
			for (int player = 0; player < humans; player++)
			{
				Chaos_ActivateForPlayer(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 2000, player);
				Chaos_ActivateForPlayer(CHAOS_EFFECT_RESERVE_INJECTION, 1000, player);
				sdata->chaosRuntime.players[player].intervalRemainingMS = 100 + player;
			}
			sdata->chaosRuntime.botCooldownMS = 700;
			sdata->chaosRuntime.botEffectCooldownMS[CHAOS_EFFECT_ARENA_CHAMPIONS] = 900;
			CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_ZA_WARUDO, 100, owner), "any available controller can receive personal time stop");
			for (int player = 0; player < humans; player++)
			{
				CTR_TEST_CHECK(!Chaos_ActivateForPlayer(CHAOS_EFFECT_ZA_WARUDO, 1000, player), "neither the owner nor another controller can refresh or overlap time stop");
				CTR_TEST_CHECK(!Chaos_OnPickup(&s_humans[player]), "pickups cannot activate effects during another time stop");
			}
			const struct ChaosRuntime saved = sdata->chaosRuntime;
			Tick(50);
			const struct ChaosRuntime advanced = sdata->chaosRuntime;
			sdata->chaosRuntime = saved;
			Tick(50);
			CTR_TEST_CHECK(memcmp(&advanced, &sdata->chaosRuntime, sizeof(advanced)) == 0, "restored time-stop state advances deterministically");
			Tick(100);
			CTR_TEST_CHECK(!Chaos_IsTimeStopped(), "an overshooting frame expires only the time-stop activation");
			for (int player = 0; player < humans; player++)
			{
				const struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
				CTR_TEST_CHECK_EQ(pool->active[0].remainingMS, 2000, "all other effects, including the recipient's, retain their full duration through expiry");
				CTR_TEST_CHECK_EQ(pool->instantNotices[0].elapsedMS, 0, "instant notices remain suspended through time-stop overshoot");
				CTR_TEST_CHECK_EQ(pool->intervalRemainingMS, 100 + player, "personal trigger clocks cannot catch up suspended time");
			}
			CTR_TEST_CHECK_EQ(sdata->chaosRuntime.botCooldownMS, 700, "the global bot cooldown remains suspended through expiry");
			CTR_TEST_CHECK_EQ(sdata->chaosRuntime.botEffectCooldownMS[CHAOS_EFFECT_ARENA_CHAMPIONS], 900, "per-effect bot cooldowns remain suspended through expiry");
			Tick(10);
			CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[owner].active[0].remainingMS, 1990, "the recipient's other effect resumes on the next ordinary frame");
			CTR_TEST_CHECK_EQ(sdata->chaosRuntime.botCooldownMS, 690, "bot cooldowns resume without catch-up");
			CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_ZA_WARUDO, 100, (owner + 1) % humans), "a new recipient can stop time after the previous activation ends");
			Chaos_Reset();
			CTR_TEST_CHECK(!Chaos_IsTimeStopped() && s_pausedVoices == 0 && !s_pausedXA, "reset clears the personal time stop and backend audio suspension");
		}
	}
	const int triggers[] = {CHAOS_TRIGGER_INTERVAL, CHAOS_TRIGGER_RANDOM, CHAOS_TRIGGER_PICKUP};
	for (int i = 0; i < (int)len(triggers); i++)
	{
		SetupPersonalWorld(ARCADE_MODE, 4);
		SelectPersonalEffect(CHAOS_EFFECT_ZA_WARUDO);
		sdata->chaosSettings.trigger = triggers[i];
		for (int player = 0; player < 4; player++)
		{
			sdata->chaosRuntime.players[player].intervalRemainingMS = 50;
		}
		if (triggers[i] == CHAOS_TRIGGER_PICKUP)
		{
			for (int player = 0; player < 4; player++)
			{
				CTR_TEST_CHECK_EQ(Chaos_OnPickup(&s_humans[player]), player == 0, "same-frame pickups select only the first time-stop recipient");
			}
		}
		else
		{
			Tick(50);
		}
		for (int player = 0; player < 4; player++)
		{
			CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].numActive, player == 0, "simultaneously due personal triggers cannot create concurrent time stops");
		}
	}
}

static void TestPersonalInterval(void)
{
	CTR_TEST_SECTION("Interval shares cadence but targets every eligible player's pool separately");
	const int modes[] = {ARCADE_MODE, 0, BATTLE_MODE};
	for (int mode = 0; mode < (int)len(modes); mode++)
	{
		for (int players = 1; players <= 4; players++)
		{
			SetupPersonalWorld(modes[mode], players);
			SelectPersonalEffect(CHAOS_EFFECT_FLY_ME_TO_THE_MOON);
			sdata->chaosSettings.trigger = CHAOS_TRIGGER_INTERVAL;
			Chaos_BeginRace(&s_tracker);
			Tick(4999);
			for (int player = 0; player < players; player++)
			{
				CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].numActive, 0, "every pool waits for the common first boundary");
			}
			Tick(33);
			for (int player = 0; player < players; player++)
			{
				const struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
				CTR_TEST_CHECK_EQ(pool->numActive, 1, "each human receives a coincident independent activation");
				CTR_TEST_CHECK_EQ(pool->active[0].remainingMS, 30000, "each new activation receives its full duration");
				CTR_TEST_CHECK_EQ(pool->intervalRemainingMS, 4968, "all pools retain the same cadence and frame overshoot");
			}
			// The first player's clock still drives the others after finishing.
			s_humans[0].actionsFlagSet |= ACTION_RACE_FINISHED;
			Tick(4968);
			CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[0].active[0].remainingMS, 25032, "a finished player is not refreshed by Interval");
			for (int player = 1; player < players; player++)
			{
				CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].active[0].remainingMS, 30000, "remaining players continue after player one finishes");
			}
		}
	}
}

static void TestPersonalRandom(void)
{
	CTR_TEST_SECTION("Random consumes independent trigger clocks and restores their RNG sequence");
	for (int players = 1; players <= 4; players++)
	{
		SetupPersonalWorld(ARCADE_MODE, players);
		SelectPersonalEffect(CHAOS_EFFECT_FLY_ME_TO_THE_MOON);
		sdata->chaosSettings.trigger = CHAOS_TRIGGER_RANDOM;
		Chaos_Init(12345);
		Chaos_BeginRace(&s_tracker);
		for (int player = 0; player < players; player++)
		{
			sdata->chaosRuntime.players[player].intervalRemainingMS = 100 + player * 100;
		}
		Tick(100);
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[0].numActive, 1, "the first Random clock triggers independently");
		for (int player = 1; player < players; player++)
		{
			CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].numActive, 0, "later Random deadlines do not trigger early");
			CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].intervalRemainingMS, player * 100, "later clocks retain their own remaining gaps");
		}
		const struct ChaosRuntime saved = sdata->chaosRuntime;
		const u32 rng = sdata->chaosRng;
		Tick(300);
		const struct ChaosRuntime expected = sdata->chaosRuntime;
		const u32 expectedRng = sdata->chaosRng;
		sdata->chaosRuntime = saved;
		sdata->chaosRng = rng;
		Tick(300);
		CTR_TEST_CHECK(memcmp(&expected, &sdata->chaosRuntime, sizeof(expected)) == 0, "restoring clocks reproduces every Random pool");
		CTR_TEST_CHECK_EQ(sdata->chaosRng, expectedRng, "restoring the RNG reproduces future independent gaps");
		for (int player = 0; player < players; player++)
		{
			CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].numActive, 1, "every due Random pool eventually activates");
		}
	}
}

static void TestPersonalPickupAndReroll(void)
{
	CTR_TEST_SECTION("Pickup targets its source and bounded rerolls preserve useful triggers");
	for (int players = 1; players <= 4; players++)
	{
		SetupPersonalWorld(BATTLE_MODE, players);
		SelectPersonalEffect(CHAOS_EFFECT_FLY_ME_TO_THE_MOON);
		for (int player = players - 1; player >= 0; player--)
		{
			CTR_TEST_CHECK(Chaos_OnPickup(&s_humans[player]), "every human can trigger a personal pickup");
			CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].numActive, 1, "Pickup inserts the activation in its source's pool");
			if (player > 0)
			{
				CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player - 1].numActive, 0, "Pickup leaves untriggered players untouched");
			}
		}
	}
	SetupPersonalWorld(BATTLE_MODE, 2);
	SelectPersonalEffect(CHAOS_EFFECT_YARDRAT_TRAINING);
	CTR_TEST_CHECK(!Chaos_OnPickup(&s_humans[1]), "a completely incompatible pool returns without looping");
	sdata->chaosSettings.effects[CHAOS_EFFECT_BODY_SWAP] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_ZA_WARUDO] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_RESERVE_INJECTION] = true;
	sdata->chaosSettings.enabledCount = 4;
	CTR_TEST_CHECK(Chaos_OnPickup(&s_humans[1]), "unavailable special effects and incompatible candidates do not waste a usable pickup");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[1].numInstantNotices, 1, "the fallback gift belongs only to the source player");
	s_humans[1].actionsFlagSet |= ACTION_RACE_FINISHED;
	CTR_TEST_CHECK(!Chaos_OnPickup(&s_humans[1]), "finished players cannot trigger personal pickups");
}

static void TestBotCooldowns(void)
{
	CTR_TEST_SECTION("bot cooldowns are shared across pools and begin at effect completion");
	SetupPersonalWorld(ARCADE_MODE, 4);
	g_config.extendedMultiplayer = true;
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_ARENA_CHAMPIONS, 1000, 2), "a later pool may start the first bot effect");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.botCooldownMS, 0, "a running effect has not started its completion cooldown");
	CTR_TEST_CHECK(!Chaos_ActivateForPlayer(CHAOS_EFFECT_KING_CRIMSON, 1000, 0), "another bot effect cannot overlap the running one");
	Tick(1100);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.botCooldownMS, 29900, "global cooldown starts at expiry and retains frame overshoot");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.botEffectCooldownMS[CHAOS_EFFECT_ARENA_CHAMPIONS], 59900, "per-effect cooldown starts at expiry too");
	Tick(29899);
	CTR_TEST_CHECK(!Chaos_ActivateForPlayer(CHAOS_EFFECT_KING_CRIMSON, 1000, 1), "global cooldown rejects another effect until the exact boundary");
	Tick(1);
	CTR_TEST_CHECK(!Chaos_ActivateForPlayer(CHAOS_EFFECT_ARENA_CHAMPIONS, 1000, 1), "the longer per-effect cooldown survives the global boundary");
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_KING_CRIMSON, 1000, 1), "a different bot effect becomes available at the global boundary");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.botCooldownMS, 30000, "an instant bot effect starts cooldown at execution");
	CTR_TEST_CHECK(!Chaos_ActivateForPlayer(CHAOS_EFFECT_KING_CRIMSON, 1000, 3), "same-frame triggers in another pool cannot bypass cooldowns");
	SelectPersonalEffect(CHAOS_EFFECT_ARENA_CHAMPIONS);
	CTR_TEST_CHECK(!Chaos_OnPickup(&s_humans[3]), "an entirely cooling pool returns without an activation");
	sdata->chaosSettings.effects[CHAOS_EFFECT_FLY_ME_TO_THE_MOON] = true;
	sdata->chaosSettings.enabledCount = 2;
	CTR_TEST_CHECK(Chaos_OnPickup(&s_humans[3]), "a blocked bot candidate is rerolled to a usable human effect");
	Tick(30000);
	CTR_TEST_CHECK(Chaos_ActivateForPlayer(CHAOS_EFFECT_ARENA_CHAMPIONS, 1000, 0), "the original bot effect returns at its sixty-second boundary");
	Chaos_BeginRace(&s_tracker);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.botCooldownMS, 0, "race restart clears global cooldown");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.botEffectCooldownMS[CHAOS_EFFECT_KING_CRIMSON], 0, "race restart clears per-effect cooldowns");
}

static void TestPersonalTimerSuspensionAndExternal(void)
{
	CTR_TEST_SECTION("Za Warudo suspends personal clocks and cooldowns; External remains shared-only");
	SetupPersonalWorld(ARCADE_MODE, 4);
	SelectPersonalEffect(CHAOS_EFFECT_FLY_ME_TO_THE_MOON);
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_RANDOM;
	Chaos_BeginRace(&s_tracker);
	sdata->chaosRuntime.botCooldownMS = 1000;
	sdata->chaosRuntime.botEffectCooldownMS[CHAOS_EFFECT_ARENA_CHAMPIONS] = 2000;
	for (int player = 0; player < 4; player++)
	{
		sdata->chaosRuntime.players[player].intervalRemainingMS = 100 + player;
	}
	struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[2];
	pool->active[pool->numActive++] = (struct ChaosActivation){.effect = CHAOS_EFFECT_ZA_WARUDO, .remainingMS = 50};
	Tick(50);
	for (int player = 0; player < 4; player++)
	{
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].intervalRemainingMS, 100 + player, "every trigger clock stays frozen through time-stop expiry");
	}
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.botCooldownMS, 1000, "global bot cooldown stays frozen");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.botEffectCooldownMS[CHAOS_EFFECT_ARENA_CHAMPIONS], 2000, "per-effect bot cooldown stays frozen");
	Tick(10);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.botCooldownMS, 990, "cooldowns resume only on the next unfrozen frame");
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_EXTERNAL;
	s_externalCommand = "fly-me-to-the-moon";
	const int reads = s_externalReads;
	Tick(5000);
	CTR_TEST_CHECK_EQ(s_externalReads, reads, "personal mode does not consume shared external commands");
	Chaos_PublishEffects();
	CTR_TEST_CHECK_EQ(strlen(s_externalCatalogue), 0, "personal mode publishes no shared-provider catalogue");
	s_externalCommand = NULL;
}

static void TestPersonalSelectionAndEligibility(void)
{
	CTR_TEST_SECTION("personal selection is independent and temporary eligibility cannot stall scheduling");
	SetupPersonalWorld(ARCADE_MODE, 4);
	SelectPersonalEffect(CHAOS_EFFECT_FLY_ME_TO_THE_MOON);
	sdata->chaosSettings.effects[CHAOS_EFFECT_GROUNDED] = true;
	sdata->chaosSettings.enabledCount = 2;
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_INTERVAL;
	Chaos_Init(12345);
	Chaos_BeginRace(&s_tracker);
	bool differentSelections = false;
	for (int boundary = 0; boundary < 10; boundary++)
	{
		Tick(5000);
		const struct ChaosPlayerRuntime *first = &sdata->chaosRuntime.players[0];
		for (int player = 1; player < 4; player++)
		{
			const struct ChaosPlayerRuntime *pool = &sdata->chaosRuntime.players[player];
			differentSelections |= pool->active[pool->numActive - 1].effect != first->active[first->numActive - 1].effect;
		}
	}
	CTR_TEST_CHECK(differentSelections, "common Interval cadence does not copy one selection into every pool");
	SelectPersonalEffect(CHAOS_EFFECT_ARENA_CHAMPIONS);
	sdata->chaosRuntime.botCooldownMS = 200000;
	Tick(100000);
	for (int player = 0; player < 4; player++)
	{
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].numActive, 0, "a temporarily empty pool neither loops nor creates unavailable effects");
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].intervalRemainingMS, 5000, "empty pools retain common cadence through multiple missed boundaries");
	}
	SelectPersonalEffect(CHAOS_EFFECT_FLY_ME_TO_THE_MOON);
	Tick(5000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[3].numActive, 1, "a usable pool resumes without restarting the race");

	SetupPersonalWorld(ARCADE_MODE, 4);
	SelectPersonalEffect(CHAOS_EFFECT_FLY_ME_TO_THE_MOON);
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_RANDOM;
	Chaos_BeginRace(&s_tracker);
	s_humans[0].actionsFlagSet |= ACTION_RACE_FINISHED;
	s_humans[1].actionsFlagSet |= ACTION_BOT;
	s_tracker.drivers[2] = NULL;
	for (int player = 0; player < 4; player++)
	{
		sdata->chaosRuntime.players[player].intervalRemainingMS = 50;
	}
	Tick(50);
	for (int player = 0; player < 3; player++)
	{
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].numActive, 0, "Random skips finished, bot and missing recipients");
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].intervalRemainingMS, 50, "Random does not consume an ineligible recipient's clock");
	}
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[3].numActive, 1, "ineligible earlier slots do not block another player's Random deadline");
}

static void TestBotTriggerContention(void)
{
	CTR_TEST_SECTION("automatic personal triggers cannot duplicate bot effects in the same frame");
	SetupPersonalWorld(ARCADE_MODE, 4);
	g_config.extendedMultiplayer = true;
	SelectPersonalEffect(CHAOS_EFFECT_ARENA_CHAMPIONS);
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_INTERVAL;
	Chaos_BeginRace(&s_tracker);
	Tick(5000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[0].numActive, 1, "the first due recipient owns the sole bot activation");
	for (int player = 1; player < 4; player++)
	{
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].numActive, 0, "later same-frame draws cannot duplicate the running bot effect");
	}
	sdata->chaosSettings.effects[CHAOS_EFFECT_RESERVE_INJECTION] = true;
	sdata->chaosSettings.enabledCount = 2;
	Tick(5000);
	for (int player = 0; player < 4; player++)
	{
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[player].numInstantNotices, 1, "every due recipient rerolls a blocked bot candidate to a personal gift");
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
	TestPersonalTimeStopContention();
	TestPersonalInterval();
	TestPersonalRandom();
	TestPersonalPickupAndReroll();
	TestBotCooldowns();
	TestPersonalTimerSuspensionAndExternal();
	TestPersonalSelectionAndEligibility();
	TestBotTriggerContention();
	return CtrTest_Summary();
}
