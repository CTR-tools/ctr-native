#include <chaos/chaos_fixture.h>
#include <game/Vehicle/VehPhysProc_Input.c>
#include <chaos/chaos_jump_fixture.h>

static void TestGravityEffects(void)
{
	CTR_TEST_SECTION("gravity effects derive from live driver constants without accumulating or overwriting them");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_HEAVY_METAL] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_GROUNDED] = true;
	sdata->chaosSettings.enabledCount += 2;
	struct Driver human = {.driverID = 0, .const_Gravity = 1000, .const_JumpForce = 800};
	struct Driver partner = {.driverID = 1, .const_Gravity = 1500};
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT, .const_Gravity = 2000};
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 1000, "inactive Chaos preserves normal gravity");
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 2000);
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 250, "Low Gravity applies one quarter of the human driver's gravity");
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 250, "repeated physics reads do not repeatedly divide gravity");
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&partner), 375, "the co-op partner uses their own gravity baseline");
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&bot), 2000, "gravity effects leave bots unchanged");
	CTR_TEST_CHECK_EQ(human.const_Gravity, 1000, "Chaos does not overwrite the stored gravity constant");
	CTR_TEST_CHECK_EQ(human.const_JumpForce, 800, "Low Gravity preserves the jump impulse");
	human.const_Gravity = 1200;
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 300, "legitimate constant changes immediately become the new baseline");
	Tick(500);
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 1500);
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 300, "reactivating Low Gravity does not compound");
	Tick(500);
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 300, "reactivated gravity retains its replacement lifetime");
	Tick(1000);
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 1200, "final expiry immediately uses the current baseline rather than a stale snapshot");
	Chaos_Activate(CHAOS_EFFECT_HEAVY_METAL, 3000);
	Chaos_Activate(CHAOS_EFFECT_HEAVY_METAL, 3000);
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 2400, "reactivating Heavy Karts does not compound gravity");
	CTR_TEST_CHECK_EQ(Chaos_JumpForceForDriver(&human, 800), 1120, "Heavy Metal raises jump force to 140 percent");
	CTR_TEST_CHECK_EQ(human.const_JumpForce, 800, "Heavy Karts changes gravity without disabling the jump impulse");
	human.const_Gravity = 32767;
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 65534, "effective gravity remains a full integer rather than overflowing the stored halfword");
	human.const_Gravity = 1200;
	const struct ChaosRuntime saved = sdata->chaosRuntime;
	s_tracker.gameMode1 |= PAUSE_1;
	Tick(5000);
	CTR_TEST_CHECK(memcmp(&saved, &sdata->chaosRuntime, sizeof(saved)) == 0, "pause freezes gravity effect lifetimes");
	s_tracker.gameMode1 &= ~PAUSE_1;
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 2400, "unpausing retains the gravity modifier");
	human.actionsFlagSet = ACTION_RACE_FINISHED;
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 1200, "finished humans immediately use normal gravity");
	human.actionsFlagSet = 0;
	Tick(3000);
	sdata->chaosRuntime = saved;
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 2400, "resident restoration needs no separate baseline repair");
	Chaos_Reset();
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 1200, "reset leaves no modified driver constants behind");

	CTR_TEST_SECTION("opposite gravity effects deactivate each other while preserving jumping and unrelated effects");
	Chaos_BeginRace(&s_tracker);
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 10000);
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 3000);
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 2000);
	Chaos_Activate(CHAOS_EFFECT_HEAVY_METAL, 1000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[0].numActive, 2, "Heavy Karts replaces Low Gravity but preserves unrelated effects");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_FLY_ME_TO_THE_MOON), "replaced Low Gravity entries are permanently deactivated");
	CTR_TEST_CHECK(Chaos_IsActive(CHAOS_EFFECT_GROUNDED), "gravity replacement leaves No Hopping active");
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 1680, "Grounded multiplies Heavy Metal gravity by 70 percent");
	Tick(1000);
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 840, "expiry of Heavy Metal leaves Grounded gravity active");
	Chaos_Activate(CHAOS_EFFECT_HEAVY_METAL, 3000);
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 1000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_HEAVY_METAL), "Low Gravity also permanently deactivates Heavy Karts");
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 210, "Low Gravity replaces Heavy Metal and combines with Grounded");
	Tick(1000);
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 840, "expiry of Low Gravity leaves Grounded gravity active");
	Chaos_Reset();
	Chaos_BeginRace(&s_tracker);
	Chaos_Activate(CHAOS_EFFECT_HEAVY_METAL, 1000);
	CTR_TEST_CHECK((PreparedJump(&human, BTN_L1, BTN_L1, RAIN_CLOUD_EFFECT_NONE, 0) & ACTION_JUMP_BUTTON_HELD) != 0,
	    "Heavy Karts still allows manual hops");
	CTR_TEST_CHECK_EQ(human.jump_TenBuffer, 10, "Heavy Karts retains the retail hop buffer");
	s_tracker.gameMode1 |= ADVENTURE_ARENA;
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 1200, "Adventure hubs cannot receive gravity modifiers");
}

static void TestJumpRebalance(void)
{
	CTR_TEST_SECTION("jump and gravity modifiers combine without changing stored constants");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_SUNDAY_DRIVER] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_GROUNDED] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_HEAVY_METAL] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_SMALL_KART_ENERGY] = true;
	sdata->chaosSettings.enabledCount += 4;
	struct Driver human = {.driverID = 0, .const_Gravity = 1000, .const_JumpForce = 1000};
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT, .const_Gravity = 1000};
	Chaos_Activate(CHAOS_EFFECT_SUNDAY_DRIVER, 5000);
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 800, "Sunday Driver reduces gravity to 80 percent");
	CTR_TEST_CHECK_EQ(Chaos_JumpForceForDriver(&human, 1000), 1200, "Sunday Driver raises jump force to 120 percent");
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 5000);
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 560, "Grounded and Sunday Driver multiply gravity");
	Chaos_Activate(CHAOS_EFFECT_HEAVY_METAL, 5000);
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 1120, "Heavy Metal combines with both gravity reductions");
	CTR_TEST_CHECK_EQ(Chaos_JumpForceForDriver(&human, 1000), 1680, "Heavy Metal and Sunday Driver multiply jump force");
	Chaos_Activate(CHAOS_EFFECT_SMALL_KART_ENERGY, 5000);
	CTR_TEST_CHECK_EQ(Chaos_JumpForceForDriver(&human, 1000), 1680, "Small Kart Energy does not reduce jump force");
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&bot), 1000, "combined effects do not change bot gravity");
	CTR_TEST_CHECK_EQ(Chaos_JumpForceForDriver(&bot, 1000), 1000, "combined effects do not change bot jump force");
	CTR_TEST_CHECK(human.const_Gravity == 1000 && human.const_JumpForce == 1000, "modifiers leave driver constants intact");
	Chaos_Reset();
	CTR_TEST_CHECK_EQ(Chaos_JumpForceForDriver(&human, 1000), 1000, "reset restores normal jump force");
}

static void TestSpeedEffects(void)
{
	CTR_TEST_SECTION("speed effects derive targets and acceleration from live values without overwriting kart constants");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_SUNDAY_DRIVER] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_GOTTA_GO_FAST] = true;
	sdata->chaosSettings.enabledCount += 2;
	struct Driver human = {
		.driverID = 0, .baseSpeed = 12000, .const_Speed_ClassStat = 12000,
		.const_Accel_ClassStat = 100, .const_Accel_Reserves = 200,
		.reserves = 3000, .turbo_outsideTimer = 1000, .fireSpeedCap = 5000,
	};
	struct Driver partner = {.driverID = 1, .baseSpeed = 14000, .const_Speed_ClassStat = 14000};
	struct Driver bot = {.driverID = 2, .baseSpeed = 12000, .const_Speed_ClassStat = 12000, .actionsFlagSet = ACTION_BOT};
	const struct Driver original = human;
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 15000), 15000, "inactive Chaos preserves the resolved speed target");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&human, 100), 100, "inactive Chaos preserves acceleration");
	Chaos_Activate(CHAOS_EFFECT_SUNDAY_DRIVER, 2000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 12000), 8400, "Sunday Driver scales the resolved forward target to 70 percent");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 20000), 14000, "boosted targets retain 70 percent without a class-based ceiling");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 4000), 2800, "damage and partial-speed targets receive the same multiplier");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 1001), 700, "fractional target speeds truncate only after applying the percentage");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 0), 0, "Sunday Driver does not introduce motion at rest");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, -3000), -3000, "Sunday Driver leaves reverse targets unchanged");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&partner, 18000), 12600, "the co-op partner receives the same forward-speed multiplier");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&bot, 18000), 18000, "Sunday Driver does not affect bots");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&human, 100), 100, "Sunday Driver preserves normal acceleration");
	CTR_TEST_CHECK(memcmp(&human, &original, sizeof(human)) == 0, "speed queries do not overwrite constants, motion or boost fuel");
	human.const_Speed_ClassStat = 14000;
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 20000), 14000, "class constants do not impose an extra ceiling on the current target");
	Tick(500);
	Chaos_Activate(CHAOS_EFFECT_SUNDAY_DRIVER, 1500);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 20000), 14000, "reactivating Sunday Driver does not compound the multiplier");
	Tick(500);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 20000), 14000, "the replacement multiplier remains active until its own expiry");
	Tick(1000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 20000), 20000, "Sunday Driver expiry returns the current retail target");

	Chaos_Activate(CHAOS_EFFECT_GOTTA_GO_FAST, 2000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 12000), 24000, "Super Speed doubles the resolved forward target before the retail hard limit");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 4000), 8000, "Super Speed scales the current damaged target rather than restoring an old one");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 0), 0, "Super Speed preserves a zero target");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, -3000), -3000, "Super Speed leaves reverse targets unchanged");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&human, 100), 200, "Super Speed doubles ordinary forward acceleration");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&human, 200), 400, "Super Speed also doubles reserve acceleration");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&human, 500), 1000, "Super Speed also scales turbo-pad acceleration");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&human, 0), 0, "Super Speed does not create acceleration from zero");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&human, -100), -100, "Super Speed preserves negative acceleration");
	human.baseSpeed = -3000;
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&human, 100), 100, "Super Speed preserves reverse acceleration");
	human.baseSpeed = 12000;
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&partner, 14000), 28000, "Super Speed applies to the co-op partner before the retail clamp");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&partner, 100), 200, "the co-op partner also gets increased acceleration");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&bot, 12000), 12000, "Super Speed preserves bot targets");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&bot, 100), 100, "Super Speed preserves bot acceleration");
	Tick(500);
	Chaos_Activate(CHAOS_EFFECT_GOTTA_GO_FAST, 1500);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 12000), 24000, "reactivating Super Speed does not compound speed");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&human, 100), 200, "reactivating Super Speed does not compound acceleration");
	Tick(500);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 12000), 24000, "the refreshed speed effect uses its replacement lifetime");
	const struct ChaosRuntime saved = sdata->chaosRuntime;
	s_tracker.gameMode1 |= PAUSE_1;
	Tick(5000);
	CTR_TEST_CHECK(memcmp(&saved, &sdata->chaosRuntime, sizeof(saved)) == 0, "pause freezes speed effect lifetimes");
	s_tracker.gameMode1 &= ~PAUSE_1;
	human.actionsFlagSet |= ACTION_RACE_FINISHED;
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 12000), 12000, "finished humans immediately use normal targets");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&human, 100), 100, "finished humans immediately use normal acceleration");
	human.actionsFlagSet &= ~ACTION_RACE_FINISHED;
	Tick(1000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 12000), 12000, "final Super Speed expiry restores the current retail target");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&human, 100), 100, "final Super Speed expiry restores retail acceleration");
	sdata->chaosRuntime = saved;
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 12000), 24000, "resident restoration restores speed rules without baseline repair");
	Chaos_Reset();
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 12000), 12000, "reset restores normal speed rules");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&human, 100), 100, "reset restores normal acceleration rules");

	CTR_TEST_SECTION("opposite speed effects permanently replace each other and preserve unrelated activations");
	Chaos_BeginRace(&s_tracker);
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 10000);
	Chaos_Activate(CHAOS_EFFECT_SUNDAY_DRIVER, 3000);
	Chaos_Activate(CHAOS_EFFECT_SUNDAY_DRIVER, 3000);
	Chaos_Activate(CHAOS_EFFECT_SUNDAY_DRIVER, 3000);
	const struct ChaosActivation gravity = sdata->chaosRuntime.players[0].active[0];
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_GOTTA_GO_FAST, 1000), "Super Speed can replace a refreshed cap");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.players[0].numActive, 2, "replacement removes the cap but keeps the unrelated gravity effect");
	CTR_TEST_CHECK(memcmp(&gravity, &sdata->chaosRuntime.players[0].active[0], sizeof(gravity)) == 0, "replacement preserves unrelated timers and order");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_SUNDAY_DRIVER), "replaced caps are deactivated immediately");
	Tick(1000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 12000), 12000, "Super Speed expiry cannot revive replaced caps");
	Chaos_Activate(CHAOS_EFFECT_GOTTA_GO_FAST, 3000);
	Chaos_Activate(CHAOS_EFFECT_GOTTA_GO_FAST, 3000);
	Chaos_Activate(CHAOS_EFFECT_SUNDAY_DRIVER, 1000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_GOTTA_GO_FAST), "Speed Capped removes all Super Speed copies");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 12000), 8400, "Sunday Driver replaces Gotta Go Fast with the 70-percent target");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&human, 100), 100, "the replacement cap removes increased acceleration immediately");
	Tick(1000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 12000), 12000, "cap expiry cannot revive replaced Super Speed");
	Chaos_Activate(CHAOS_EFFECT_GOTTA_GO_FAST, 1000);
	s_tracker.gameMode1 |= ADVENTURE_ARENA;
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 12000), 12000, "Adventure hubs preserve ordinary speed targets");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&human, 100), 100, "Adventure hubs preserve ordinary acceleration");
}

int main(void)
{
	TestGravityEffects();
	TestJumpRebalance();
	TestSpeedEffects();
	return CtrTest_Summary();
}
