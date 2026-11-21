#include <chaos/chaos_fixture.h>

static void EnableGiftEffects(void)
{
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	s_externalCommand = NULL;
	s_maskGrants = 0;
	s_shieldGrants = 0;
	for (int id = 0; id < CHAOS_EFFECT_COUNT; id++)
	{
		sdata->chaosSettings.effects[id] = true;
	}
	sdata->chaosSettings.enabledCount = CHAOS_EFFECT_COUNT;
}

static void TestInstantGifts(void)
{
	CTR_TEST_SECTION("instant rewards bypass timed capacity and never dock");
	EnableGiftEffects();
	const ChaosEffectId timed[] = {
		CHAOS_EFFECT_FLY_ME_TO_THE_MOON, CHAOS_EFFECT_GROUNDED,
		CHAOS_EFFECT_FAIR_MODE, CHAOS_EFFECT_MIRROR_MIRROR,
	};
	for (size_t i = 0; i < len(timed); i++)
	{
		CTR_TEST_CHECK(Chaos_Activate(timed[i], 10000), "the timed effect pool can fill independently");
	}
	s_defaultDrivers[1].actionsFlagSet = ACTION_RACE_FINISHED;
	s_defaultDrivers[1].numWumpas = 2;
	struct Driver bot = {.driverID = 2, .numWumpas = 3, .actionsFlagSet = ACTION_BOT};
	s_tracker.drivers[2] = &bot;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_JUICED_UP, 1), "a one-shot reward works with all timed slots occupied");
	CTR_TEST_CHECK_EQ(s_defaultDrivers[0].numWumpas, 0, "Juiced Up leaves crediting to the ordinary gradual HUD pickup");
	CTR_TEST_CHECK_EQ(s_defaultDrivers[0].PickupWumpaHUD.numCollected, DRIVER_WUMPA_JUICED_COUNT, "Juiced Up queues the missing Wumpas");
	const int chimesBefore = s_juicedSounds;
	while (s_defaultDrivers[0].PickupWumpaHUD.numCollected > 0)
	{
		s_defaultDrivers[0].PickupWumpaHUD.numCollected--;
		RB_Player_ModifyWumpa(&s_defaultDrivers[0], 1);
	}
	CTR_TEST_CHECK_EQ(s_defaultDrivers[0].numWumpas, DRIVER_WUMPA_JUICED_COUNT, "normal pickup crediting fills the human's Wumpas");
	CTR_TEST_CHECK_EQ(s_juicedSounds, chimesBefore + 1, "reaching ten through normal crediting plays the juiced-up chime once");
	CTR_TEST_CHECK_EQ(s_defaultDrivers[1].numWumpas, 2, "finished humans receive no gift");
	CTR_TEST_CHECK_EQ(bot.numWumpas, 3, "bots receive no human gift");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[0].numActive, CHAOS_MAX_ACTIVATIONS, "instant rewards consume no timed slot");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_JUICED_UP), "an instant reward is never an ongoing modifier");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[0].numInstantNotices, 1, "the reward has its own notice");
	static struct DB buffer;
	u32 uiOT[5];
	buffer.otMem.uiOT = uiOT;
	s_tracker.backBuffer = &buffer;
	s_tracker.hudFlags = HUD_FLAG_RACE_HUD;
	const struct ChaosRuntime snapshot = sdata->chaosRuntime;
	s_tracker.gameMode1 |= PAUSE_1;
	Tick(500);
	CTR_TEST_CHECK(memcmp(&snapshot, &sdata->chaosRuntime, sizeof(snapshot)) == 0, "pause also freezes instant notices");
	s_tracker.gameMode1 &= ~PAUSE_1;
	Tick(CHAOS_ACTIVATION_NOTICE_MS - 100);
	DrawNotice();
	CTR_TEST_CHECK(strcmp(s_noticeText, LANG_GetString(LNG_EXPANDED_CHAOS_EFFECT_JUICED_UP)) == 0, "the instant notice survives the requested effect duration");
	CTR_TEST_CHECK_EQ(s_noticeFont, FONT_BIG, "the instant notice does not enter the compact docking animation");
	Tick(100);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, CHAOS_MAX_ACTIVATIONS, "only timed effects remain after the instant banner expires");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[0].numInstantNotices, 0, "expired notices leave no docked entry");
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_JUICED_UP, 1000), "Juiced Up remains eligible at ten Wumpas");
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_JUICED_UP, 1000), "one-shot rewards can repeat immediately");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[0].numInstantNotices, 1, "repeated gifts refresh rather than duplicate their banner");
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_JUICED_UP, 0), "zero-duration requests remain invalid for instant effects");
	const struct Driver before = s_defaultDrivers[0];
	Chaos_Activate(CHAOS_EFFECT_IT_IS_I, 1000);
	Chaos_Activate(CHAOS_EFFECT_PROVIDENCE, 1000);
	CTR_TEST_CHECK_EQ(s_maskGrants, 1, "It is I invokes the normal mask activation only for the unfinished human");
	CTR_TEST_CHECK_EQ(s_shieldGrants, 1, "Providence invokes the normal shield activation only for the unfinished human");
	CTR_TEST_CHECK(memcmp(&before, &s_defaultDrivers[0], sizeof(before)) == 0, "dispatch preserves held inventory and its mask choice");
	s_defaultDrivers[0].numWumpas = 0;
	Chaos_Activate(CHAOS_EFFECT_JUICED_UP, 1000);
	CTR_TEST_CHECK_EQ(s_defaultDrivers[0].PickupWumpaHUD.numCollected, DRIVER_WUMPA_JUICED_COUNT, "repeating an instant effect queues its reward again");
	Chaos_Activate(CHAOS_EFFECT_JUICED_UP, 1000);
	CTR_TEST_CHECK_EQ(s_defaultDrivers[0].PickupWumpaHUD.numCollected, DRIVER_WUMPA_JUICED_COUNT, "repeats during collection do not duplicate pending Wumpas");
	Chaos_Activate(CHAOS_EFFECT_USE_THIS, 1000);
	s_defaultDrivers[0].reserves = 1;
	s_defaultDrivers[0].fireSpeedCap = 1;
	Chaos_Activate(CHAOS_EFFECT_RESERVE_INJECTION, 1000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[0].numInstantNotices, CHAOS_MAX_INSTANT_NOTICES, "a fifth distinct reward keeps notice storage bounded");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[0].instantNotices[0].effect, CHAOS_EFFECT_PROVIDENCE, "notice overflow drops the oldest banner");
	const struct ChaosRuntime notices = sdata->chaosRuntime;
	Chaos_Reset();
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[0].numInstantNotices, 0, "reset clears instant notices too");
	sdata->chaosRuntime = notices;
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[0].numInstantNotices, CHAOS_MAX_INSTANT_NOTICES, "resident restore retains instant announcements");
	s_tracker.gameMode1 |= CRYSTAL_CHALLENGE;
	s_defaultDrivers[0].numWumpas = 3;
	s_defaultDrivers[0].PickupWumpaHUD.numCollected = 2;
	Chaos_Activate(CHAOS_EFFECT_JUICED_UP, 1000);
	CTR_TEST_CHECK_EQ(s_defaultDrivers[0].numWumpas, DRIVER_WUMPA_JUICED_COUNT, "crystal events receive the Wumpa reward through normal crediting");
	CTR_TEST_CHECK_EQ(s_defaultDrivers[0].PickupWumpaHUD.numCollected, 2, "the Wumpa reward cannot add or consume queued crystals");
}

static void TestHeldItemRemoval(void)
{
	CTR_TEST_SECTION("shared item removal preserves caller-owned timers and multiplayer bookkeeping");
	EnableGiftEffects();
	struct Driver *driver = &s_defaultDrivers[0];
	s_tracker.numPlyrCurrGame = 3;
	s_tracker.numPlayersWith3Missiles = 1;
	driver->heldItemID = HELD_ITEM_MISSILE_3X;
	driver->noItemTimer = 1;
	VehPhysProc_RemoveHeldItem(driver);
	CTR_TEST_CHECK_EQ(driver->heldItemID, HELD_ITEM_NONE, "ordinary removal empties the item slot");
	CTR_TEST_CHECK_EQ(s_tracker.numPlayersWith3Missiles, 0, "removing a triple missile releases its multiplayer holder count");
	CTR_TEST_CHECK_EQ(driver->noItemTimer, 1, "the ordinary caller retains control of cooldown decrementing");
	VehPhysProc_RemoveHeldItem(driver);
	CTR_TEST_CHECK_EQ(s_tracker.numPlayersWith3Missiles, 0, "removing an already empty slot does not decrement again");
	s_tracker.gameMode1 |= BATTLE_MODE;
	s_tracker.numPlayersWith3Missiles = 1;
	driver->heldItemID = HELD_ITEM_MISSILE_3X;
	VehPhysProc_RemoveHeldItem(driver);
	CTR_TEST_CHECK_EQ(s_tracker.numPlayersWith3Missiles, 1, "battle removal leaves the race-only missile counter untouched");
}

static void TestRandomItemGift(void)
{
	CTR_TEST_SECTION("Use This! uses Chaos RNG and respects inventory and orb ownership");
	EnableGiftEffects();
	Chaos_Activate(CHAOS_EFFECT_FAIR_MODE, 10000);
	struct Driver *human = &s_defaultDrivers[0];
	s_itemSounds = 0;
	human->heldItemID = HELD_ITEM_ROULETTE;
	human->itemRollTimer = 100;
	human->noItemTimer = 20;
	human->actionsFlagSet |= ACTION_WEAPON_FIRE_REQUEST;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_USE_THIS, 1000), "the item gift replaces No Items before assignment");
	CTR_TEST_CHECK_EQ(s_itemSounds, s_tracker.numPlyrCurrGame, "each human item grant plays the usual acquisition sound");
	CTR_TEST_CHECK_EQ(s_itemSound, 0x5e, "unjuiced item grants use the ordinary crate-item sound");
	human->numWumpas = DRIVER_WUMPA_JUICED_COUNT;
	Chaos_GiveRandomItem(human);
	CTR_TEST_CHECK_EQ(s_itemSound, 0x41, "juiced item grants use the ordinary upgraded-item sound");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_FAIR_MODE), "No Items cannot immediately erase the gift");
	CTR_TEST_CHECK_EQ(human->itemRollTimer, 0, "the previous roulette stops");
	CTR_TEST_CHECK_EQ(human->noItemTimer, 0, "the new item is immediately available");
	CTR_TEST_CHECK((human->actionsFlagSet & ACTION_WEAPON_FIRE_REQUEST) == 0, "a stale launch request cannot fire the new item");
	bool seen[HELD_ITEM_ROULETTE + 1] = {0};
	for (int sample = 0; sample < 300; sample++)
	{
		Chaos_Activate(CHAOS_EFFECT_USE_THIS, 1000);
		for (int player = 0; player < s_tracker.numPlyrCurrGame; player++)
		{
			const struct Driver *driver = s_tracker.drivers[player];
			CTR_TEST_CHECK(driver->heldItemID <= HELD_ITEM_MISSILE_3X && driver->heldItemID != HELD_ITEM_SPRING,
			               "race gifts exclude unused and battle-only items");
			seen[driver->heldItemID] = true;
			CTR_TEST_CHECK_EQ(driver->numHeldItems,
			                  driver->heldItemID == HELD_ITEM_BOMB_3X || driver->heldItemID == HELD_ITEM_MISSILE_3X ? HELD_ITEM_STACK_COUNT : 0,
			                  "multi-item rewards receive the standard stack count");
		}
	}
	CTR_TEST_CHECK(seen[HELD_ITEM_MASK] && seen[HELD_ITEM_CLOCK] && seen[HELD_ITEM_WARPBALL] &&
	               seen[HELD_ITEM_BOMB_3X] && seen[HELD_ITEM_MISSILE_3X], "the pool includes powerful and multi-item results");
	struct Thread orb = {.modelIndex = DYNAMIC_WARPBALL};
	s_tracker.threadBuckets[TRACKING].thread = &orb;
	s_tracker.gameMode1 |= WARPBALL_HELD;
	for (int sample = 0; sample < 50; sample++)
	{
		Chaos_Activate(CHAOS_EFFECT_USE_THIS, 1000);
		CTR_TEST_CHECK(human->heldItemID != HELD_ITEM_WARPBALL && s_defaultDrivers[1].heldItemID != HELD_ITEM_WARPBALL,
		               "an already deployed orb is excluded from both independent rolls");
	}
	human->heldItemID = HELD_ITEM_WARPBALL;
	Chaos_ClearItems(human);
	CTR_TEST_CHECK((s_tracker.gameMode1 & WARPBALL_HELD) != 0, "discarding a held orb retains a deployed orb's flag");
	s_tracker.threadBuckets[TRACKING].thread = NULL;
	human->heldItemID = HELD_ITEM_WARPBALL;
	s_defaultDrivers[1].heldItemID = HELD_ITEM_WARPBALL;
	Chaos_ClearItems(human);
	CTR_TEST_CHECK((s_tracker.gameMode1 & WARPBALL_HELD) != 0, "another holder retains the orb flag");
	Chaos_ClearItems(&s_defaultDrivers[1]);
	CTR_TEST_CHECK((s_tracker.gameMode1 & WARPBALL_HELD) == 0, "discarding the last undeployed orb releases the flag");
	s_tracker.gameMode1 |= RELIC_RACE;
	Chaos_PublishEffects();
	CTR_TEST_CHECK(strstr(s_externalCatalogue, "use-this\n") != NULL, "relic races publish the item gift too");
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_USE_THIS, 1000), "relic races accept the item gift");
	s_tracker.gameMode1 &= ~RELIC_RACE;
	s_tracker.gameMode1 |= CRYSTAL_CHALLENGE;
	Chaos_PublishEffects();
	CTR_TEST_CHECK(strstr(s_externalCatalogue, "use-this\n") != NULL, "crystal challenges publish the item gift too");
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_USE_THIS, 1000), "crystal challenges accept the full race item pool");
}

static void TestReserveGifts(void)
{
	CTR_TEST_SECTION("reserve gifts use normal fire grants and replace incompatible rules");
	EnableGiftEffects();
	struct Thread thread = {.modelIndex = DYNAMIC_PLAYER};
	struct Instance instance = {.thread = &thread};
	struct Driver *human = &s_defaultDrivers[0];
	human->instSelf = &instance;
	human->const_SingleTurboSpeed = 1000;
	human->const_SacredFireSpeed = 2000;
	s_defaultDrivers[1].reserves = 100;
	s_defaultDrivers[1].fireSpeedCap = 3000;
	Chaos_Activate(CHAOS_EFFECT_HOLE_IN_THE_TANK, 10000);
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_RESERVE_INJECTION, 1000), "Reserve Injection ends No Reserves and grants fuel");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_HOLE_IN_THE_TANK), "No Reserves is removed before the grant");
	CTR_TEST_CHECK_EQ(human->reserves, CHAOS_INJECTION_RESERVES, "the empty kart receives ten seconds of reserves");
	CTR_TEST_CHECK_EQ(human->fireSpeedCap, 1500, "an empty kart gets the third drift boost's normal fire level");
	CTR_TEST_CHECK_EQ(s_defaultDrivers[1].fireSpeedCap, 3000, "an existing stronger fire level is preserved");
	VehFire_Increment(human, 1000, POWER_SLIDE_HANG_TIME, CHAOS_THIRD_DRIFT_FIRE_LEVEL);
	CTR_TEST_CHECK_EQ(human->reserves, CHAOS_INJECTION_RESERVES + 1000, "normal boost grants can still add to the ten-second reward");
	human->reserves = INT16_MAX;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_RESERVE_INJECTION, 1000), "the gift remains eligible when reserves are already full");
	CTR_TEST_CHECK_EQ(human->reserves, INT16_MAX, "the gift cannot reduce a larger earned reserve balance");
	human->reserves = 2000;
	Chaos_Activate(CHAOS_EFFECT_TOO_BOOSTED_TO_FAIL, 1000);
	CTR_TEST_CHECK_EQ(Chaos_ReservesForDriver(human, 1968), 2000, "natural consumption cannot lower frozen reserves");
	VehFire_Increment(human, 500, POWER_SLIDE_HANG_TIME, CHAOS_THIRD_DRIFT_FIRE_LEVEL);
	CTR_TEST_CHECK_EQ(human->reserves, 2500, "boosts can still increase frozen reserves");
	VehPickState_ForceDamage(human, VEH_PICK_DAMAGE_SPIN);
	CTR_TEST_CHECK_EQ(human->reserves, 2500, "the actual damage path cannot clear frozen reserves");
	struct Driver bot = {.driverID = 2, .reserves = 2000, .actionsFlagSet = ACTION_BOT};
	CTR_TEST_CHECK_EQ(Chaos_ReservesForDriver(&bot, 0), 0, "bot fuel remains ordinary");
	const struct ChaosRuntime snapshot = sdata->chaosRuntime;
	Tick(1000);
	CTR_TEST_CHECK_EQ(Chaos_ReservesForDriver(human, 0), 0, "expiry restores reserve losses");
	sdata->chaosRuntime = snapshot;
	CTR_TEST_CHECK_EQ(Chaos_ReservesForDriver(human, 0), 2500, "resident restore restores reserve protection");
	Chaos_Activate(CHAOS_EFFECT_HOLE_IN_THE_TANK, 1000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_TOO_BOOSTED_TO_FAIL), "No Reserves replaces reserve protection");
	struct GamepadBuffer pad = {0};
	Chaos_ApplyInputRules(human, &pad);
	Chaos_ApplyDriver(human, human->actionsFlagSet);
	CTR_TEST_CHECK_EQ(human->reserves, 0, "the replacement rule clears fuel immediately when applied");
	Chaos_Activate(CHAOS_EFFECT_TOO_BOOSTED_TO_FAIL, 1000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_HOLE_IN_THE_TANK), "reserve protection replaces No Reserves in the opposite order");
}

int main(void)
{
	TestInstantGifts();
	TestHeldItemRemoval();
	TestRandomItemGift();
	TestReserveGifts();
	return CtrTest_Summary();
}
