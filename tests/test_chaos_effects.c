#include <common.h>
// Keep the fixture's unexpected-projection guards for the isolated effect paths.
void TestUnexpectedCTC2(u32 value, s32 reg);
void TestUnexpectedMTC2(u32 value, s32 reg);
u32 TestUnexpectedMFC2(s32 reg);
s32 TestUnexpectedMFC2_S(s32 reg);
s32 TestUnexpectedCOP2(s32 op);
#define CTC2 TestUnexpectedCTC2
#define MTC2 TestUnexpectedMTC2
#define MFC2 TestUnexpectedMFC2
#define MFC2_S TestUnexpectedMFC2_S
#define doCOP2 TestUnexpectedCOP2
#define CHAOS_FIXTURE_REAL_LAP_STATS
#include <shared/chaos_fixture.h>
#undef CTC2
#undef MTC2
#undef MFC2
#undef MFC2_S
#undef doCOP2

#include <platform/native_pgxp.c>
#include <platform/native_gte_core.c>
#include <platform/native_inline_c.c>

// Keep the fixture's emission double, but exercise real particle lifetime/axis updates.
struct Particle *TestRealParticleInit(u32 flags, struct IconGroup *icons, struct ParticleEmitter *emitter);
#define Particle_Init TestRealParticleInit
#include <game/Particle.c>
#undef Particle_Init

u32 MixRNG_GetValue(s32 seed)
{
	(void)seed;
	CtrTest_Bail("particle axis regression must not evaluate random oscillators");
	return 0;
}
s32 MixRNG_Particles(s32 range)
{
	(void)range;
	CtrTest_Bail("particle axis regression must not randomize emitters");
	return 0;
}
void PushBuffer_SetPsyqGeom(struct PushBuffer *buffer)
{
	(void)buffer;
	CtrTest_Bail("particle axis regression must not render geometry");
}
uint32_t NativeGpuLinks_FromHostPointerAt(const void *pointer, const char *file, int line, const char *function)
{
	(void)pointer; (void)file; (void)line; (void)function;
	CtrTest_Bail("particle axis regression must not submit GPU primitives");
	return 0;
}

int SquareRoot0_stub(int value)
{
	return (int)sqrt((double)value);
}
#include <game/MATH/MATH_4_VectorLength.c>
#include <game/MATH/MATH_5_VectorNormalize.c>
#include <game/Vehicle/VehLap.c>

// Existing results-only checks isolate projection; the geometry checks enable it.
static bool s_projectLapProgress;
static int s_teleportSavedLaps;
static void TestLapProgress(struct Driver *driver)
{
	if (s_projectLapProgress)
	{
		VehLap_UpdateProgress(driver);
	}
}
void UI_SaveLapTime(int numLaps, int lapTime, s16 driverID)
{
	(void)numLaps; (void)lapTime; (void)driverID;
	s_teleportSavedLaps++;
}
b32 MainGameEnd_IsCoopRace(struct GameTracker *tracker) { (void)tracker; return false; }
b32 MainGameEnd_AdventureRaceWon(struct GameTracker *tracker) { (void)tracker; return false; }
void BOTS_Driver_Convert(struct Driver *driver) { driver->actionsFlagSet |= ACTION_BOT; }
#define VehLap_UpdateProgress TestLapProgress
#include <game/PlayLevel.c>
#undef VehLap_UpdateProgress
#include <game/Vehicle/VehPhysProc_Input.c>

static u32 PreparedDriver(struct Driver *driver, const struct GamepadBuffer *pad, int *steering, u32 actions)
{
	struct GamepadBuffer drivingPad = *pad;
	Chaos_ApplyInputRules(driver, &drivingPad);
	*steering = drivingPad.stickLX;
	return Chaos_ApplyDriver(driver, actions);
}

static int PreparedSteering(struct Driver *driver, const struct GamepadBuffer *pad)
{
	int steering = pad->stickLX;
	PreparedDriver(driver, pad, &steering, driver->actionsFlagSet);
	return steering;
}

static u32 PreparedJump(struct Driver *driver, u32 tapped, u32 held, int rainCloudEffect, u32 actions)
{
	struct GamepadBuffer pad = {.buttonsTapped = tapped, .buttonsHeldCurrFrame = held};
	driver->actionsFlagSet = actions;
	Chaos_ApplyInputRules(driver, &pad);
	actions = VehPhysProc_JumpInput(driver, pad.buttonsTapped, pad.buttonsHeldCurrFrame, rainCloudEffect, actions);
	return Chaos_ApplyDriver(driver, actions);
}

static void TestSteeringAndDriverScope(void)
{
	CTR_TEST_SECTION("steering is mirrored once around the active center for unfinished humans only");
	EnableInputEffects();
	struct Driver human = {.driverID = 0};
	struct Driver partner = {.driverID = 1};
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT};
	struct GamepadBuffer pad = {.stickLX = 0};
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 0, "inactive steering preserves the axis");
	Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 1000);
	const int axes[] = {0, 1, 64, 127, 128, 129, 192, 255};
	for (int i = 0; i < (int)len(axes); i++)
	{
		pad.stickLX = (s16)axes[i];
		CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 256 - axes[i], "analog and resolved D-pad extremes mirror without byte wrapping");
		CTR_TEST_CHECK_EQ(pad.stickLX, axes[i], "steering never overwrites the shared gamepad axis");
	}
	pad.stickLX = 64;
	CTR_TEST_CHECK_EQ(PreparedSteering(&partner, &pad), 192, "the second human receives the same effect");
	CTR_TEST_CHECK_EQ(PreparedSteering(&bot, &pad), 64, "the first bot above the human slots is unaffected");
	human.actionsFlagSet = ACTION_RACE_FINISHED;
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 64, "a finished human is unaffected while the partner continues");
	human.actionsFlagSet = 0;
	struct RacingWheelData wheel = {.gamepadCenter = 100};
	pad.rwd = &wheel;
	pad.stickLX = 60;
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 140, "calibrated wheel steering mirrors around its own center");
	pad.stickLX = 100;
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 100, "the wheel's neutral position stays neutral");
	pad.stickLX = 60;
	Tick(500);
	Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 1000);
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 140, "duplicate activations do not invert twice and cancel");
	Tick(500);
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 140, "the later activation keeps steering inverted after the first expires");
	Tick(500);
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 60, "steering returns to the live input after final expiry");
	CTR_TEST_CHECK(!Chaos_IsActiveForDriver(CHAOS_EFFECT_NONE, &human), "invalid effect IDs cannot apply to a driver");
	CTR_TEST_CHECK(!Chaos_IsActiveForDriver(CHAOS_EFFECT_MIRROR_MIRROR, NULL), "a missing driver cannot receive an effect");
}

static void EnableWobble(void)
{
	EnableInputEffects();
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_EXTERNAL;
	sdata->chaosSettings.effects[CHAOS_EFFECT_TOTALLY_SOBER] = true;
	sdata->chaosSettings.enabledCount++;
	// Cardinal samples exercise the game's real sine lookup and quadrant handling.
	data.trigApprox[0] = (struct TrigTable){ .sin = 0, .cos = FP(1.0) };
}

static void TestSabotage(void)
{
	CTR_TEST_SECTION("Sabotage filters local brake inputs and preserves controller state");
	EnableInputEffects();
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_EXTERNAL;
	sdata->chaosSettings.effects[CHAOS_EFFECT_SABOTAGE] = true;
	sdata->chaosSettings.enabledCount++;
	struct Driver driver = {.driverID = 0};
	const struct GamepadBuffer pad = {
		.buttonsHeldCurrFrame = BTN_SQUARE_one | BTN_CROSS_one,
		.buttonsTapped = BTN_SQUARE_one,
		.buttonsHeldPrevFrame = BTN_SQUARE_one,
		.stickLX = CHAOS_STICK_CENTER,
		.stickRY = 255,
		.stickLY = 255,
	};
	const struct GamepadBuffer original = pad;
	struct GamepadBuffer drivingPad = pad;
	Chaos_ApplyInputRules(&driver, &drivingPad);
	CTR_TEST_CHECK(memcmp(&drivingPad, &pad, sizeof(pad)) == 0, "inactive Sabotage preserves braking and reverse");
	Chaos_Activate(CHAOS_EFFECT_SABOTAGE, 1000);
	Chaos_ApplyInputRules(&driver, &drivingPad);
	CTR_TEST_CHECK((drivingPad.buttonsHeldCurrFrame & BTN_SQUARE_one) == 0 && drivingPad.stickRY == CHAOS_STICK_CENTER && drivingPad.stickLY == CHAOS_STICK_CENTER, "active Sabotage neutralizes digital and analog brake inputs");
	CTR_TEST_CHECK(memcmp(&pad, &original, sizeof(pad)) == 0, "filtering leaves shared controller input and button history intact");
	drivingPad.stickRY = drivingPad.stickLY = 0;
	Chaos_ApplyInputRules(&driver, &drivingPad);
	CTR_TEST_CHECK(drivingPad.stickRY == 0 && drivingPad.stickLY == 0, "forward analog input remains available");
	Tick(1000);
	drivingPad = pad;
	Chaos_ApplyInputRules(&driver, &drivingPad);
	CTR_TEST_CHECK(memcmp(&drivingPad, &pad, sizeof(pad)) == 0, "expiry restores ordinary braking and reverse");
}

static void TestHydraulics(void)
{
	CTR_TEST_SECTION("Hydraulics renews normal hop requests without bypassing physics or synthesizing shoulder input");
	EnableInputEffects();
	sdata->chaosSettings.trigger = CHAOS_TRIGGER_EXTERNAL;
	sdata->chaosSettings.effects[CHAOS_EFFECT_HYDRAULICS] = true;
	sdata->chaosSettings.enabledCount++;
	struct Driver human = {.driverID = 0, .kartState = KS_NORMAL, .const_JumpForce = 800};
	struct Driver partner = {.driverID = 1, .kartState = KS_NORMAL};
	struct Driver bot = {.driverID = 2, .kartState = KS_NORMAL, .actionsFlagSet = ACTION_BOT};
	PreparedJump(&human, 0, 0, RAIN_CLOUD_EFFECT_NONE, ACTION_TOUCH_GROUND);
	CTR_TEST_CHECK_EQ(human.jump_TenBuffer, 0, "inactive Hydraulics does not request a hop");
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 5000);
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_HYDRAULICS, 1000), "Hydraulics activates as a timed effect");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_GROUNDED), "Hydraulics replaces No Hopping");
	const u8 states[] = {KS_NORMAL, KS_DRIFTING, KS_ANTIVSHIFT};
	for (size_t i = 0; i < len(states); i++)
	{
		human.kartState = states[i];
		const u32 actions = PreparedJump(&human, 0, 0, RAIN_CLOUD_EFFECT_NONE, ACTION_TOUCH_GROUND);
		CTR_TEST_CHECK_EQ(human.jump_TenBuffer, 1, "normal driving, drifting and V-shift recovery receive a current-step hop request");
		CTR_TEST_CHECK_EQ(actions, ACTION_TOUCH_GROUND, "automatic hopping does not invent shoulder-held input or start a jump directly");
	}
	human.kartState = KS_NORMAL;
	human.jump_CooldownMS = 100;
	human.jump_CoyoteTimerMS = 0;
	human.jump_ForcedMS = 50;
	human.forcedJumpType = FORCED_JUMP_HIGH;
	PreparedJump(&human, 0, 0, RAIN_CLOUD_EFFECT_NONE, 0);
	CTR_TEST_CHECK_EQ(human.jump_TenBuffer, 1, "a request stays ready for the normal landing and cooldown checks later in the physics step");
	CTR_TEST_CHECK(human.jump_CooldownMS == 100 && human.jump_CoyoteTimerMS == 0 && human.jump_ForcedMS == 50,
	               "Hydraulics preserves normal jump timing and airborne restrictions");
	CTR_TEST_CHECK(human.const_JumpForce == 800 && human.forcedJumpType == FORCED_JUMP_HIGH,
	               "normal force and externally forced jumps are left to the existing physics");
	PreparedJump(&human, BTN_R1, BTN_R1, RAIN_CLOUD_EFFECT_NONE, 0);
	CTR_TEST_CHECK_EQ(human.jump_TenBuffer, VEH_PHYS_PROC_JUMP_BUFFER_FRAMES, "a genuine shoulder press keeps its full normal buffer");
	human.rainCloudEffect = RAIN_CLOUD_EFFECT_JUMP_LOCKOUT;
	PreparedJump(&human, 0, 0, RAIN_CLOUD_EFFECT_JUMP_LOCKOUT, 0);
	CTR_TEST_CHECK_EQ(human.jump_TenBuffer, 0, "rain-cloud jump lockout prevents automatic hop requests");
	human.rainCloudEffect = RAIN_CLOUD_EFFECT_NONE;
	human.kartState = KS_SPINNING;
	PreparedJump(&human, 0, 0, RAIN_CLOUD_EFFECT_NONE, 0);
	CTR_TEST_CHECK_EQ(human.jump_TenBuffer, 0, "damage states do not receive automatic hop requests");
	PreparedJump(&bot, 0, 0, RAIN_CLOUD_EFFECT_NONE, ACTION_BOT);
	CTR_TEST_CHECK_EQ(bot.jump_TenBuffer, 0, "bots retain ordinary jump input");
	PreparedJump(&partner, 0, 0, RAIN_CLOUD_EFFECT_NONE, 0);
	CTR_TEST_CHECK_EQ(partner.jump_TenBuffer, 1, "the second human also receives automatic hop requests");
	partner.actionsFlagSet = ACTION_RACE_FINISHED;
	PreparedJump(&partner, 0, 0, RAIN_CLOUD_EFFECT_NONE, ACTION_RACE_FINISHED);
	CTR_TEST_CHECK_EQ(partner.jump_TenBuffer, 0, "finished humans stop receiving automatic hop requests");
	Tick(1000);
	human.kartState = KS_NORMAL;
	PreparedJump(&human, 0, 0, RAIN_CLOUD_EFFECT_NONE, 0);
	CTR_TEST_CHECK_EQ(human.jump_TenBuffer, 0, "expiry leaves no automatic input held or buffered");
	Chaos_Activate(CHAOS_EFFECT_HYDRAULICS, 5000);
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 1000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_HYDRAULICS), "No Hopping also replaces Hydraulics");
}

static void TestMyPlanetNeedsMe(void)
{
	CTR_TEST_SECTION("My Planet Needs Me launches once, including in midair, without a timed slot");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	memset(sdata->chaosSettings.effects, true, sizeof(sdata->chaosSettings.effects));
	sdata->chaosSettings.enabledCount = CHAOS_EFFECT_COUNT;
	data.trigApprox[0] = (struct TrigTable){.sin = 0, .cos = FP(1.0)};
	struct Driver human = {
		.driverID = 0, .speed = 3000, .const_JumpForce = 4596, .const_Gravity = 1000,
		.actionsFlagSet = ACTION_TOUCH_GROUND, .reserves = 500, .heldItemID = HELD_ITEM_POTION,
		.jump_CooldownMS = 100, .jump_CoyoteTimerMS = 0,
	};
	struct Driver partner = {
		.driverID = 1, .speed = 1000, .axisRotationY = -ANG_HALF_PI,
		.const_JumpForce = 900, .actionsFlagSet = ACTION_AIRBORNE,
	};
	struct Driver bot = human;
	bot.driverID = 2;
	bot.actionsFlagSet = ACTION_BOT;
	s_tracker.drivers[0] = &human;
	s_tracker.drivers[1] = &partner;
	s_tracker.drivers[2] = &bot;
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 5000);
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_MY_PLANET_NEEDS_ME, 1000), "the immediate launch activates without normal jump eligibility");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_GROUNDED), "the launch replaces No Hopping");
	CTR_TEST_CHECK_EQ(s_launchConversions[0], 1, "activation commits one launch to the ground driver's canonical speed representation");
	CTR_TEST_CHECK_EQ(s_launchVelocity[0].y, 27576, "a grounded driver receives six times the real normal jump force upward");
	CTR_TEST_CHECK(s_launchVelocity[0].x == 0 && s_launchVelocity[0].z == 3000, "launch preserves horizontal momentum");
	CTR_TEST_CHECK_EQ(s_launchVelocity[1].y, 4400, "the airborne driver receives the impulse on top of existing falling momentum");
	CTR_TEST_CHECK_EQ(partner.velocity.y, 4400, "the working velocity also reflects the immediate impulse");
	CTR_TEST_CHECK_EQ(s_launchConversions[2], 0, "bots do not receive the launch");
	CTR_TEST_CHECK(human.reserves == 500 && human.heldItemID == HELD_ITEM_POTION, "launch preserves inventory and boost reserves");
	CTR_TEST_CHECK(human.jump_CooldownMS == 100 && human.jump_CoyoteTimerMS == 0, "the one-off impulse does not rewrite normal hop eligibility");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "the launch occupies no timed slot");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numInstantNotices, 1, "the launch creates an instant announcement");
	Tick(100);
	CTR_TEST_CHECK_EQ(s_launchConversions[0], 1, "later Chaos updates do not repeat the launch");
	Chaos_Activate(CHAOS_EFFECT_HYDRAULICS, 5000);
	Chaos_Activate(CHAOS_EFFECT_MY_PLANET_NEEDS_ME, 1000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_HYDRAULICS), "the launch also replaces Hydraulics");
	CTR_TEST_CHECK_EQ(s_launchConversions[0], 2, "an explicit second activation applies one new impulse");
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 5000);
	Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 5000);
	Chaos_Activate(CHAOS_EFFECT_SUNDAY_DRIVER, 5000);
	Chaos_Activate(CHAOS_EFFECT_FAIR_MODE, 5000);
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_MY_PLANET_NEEDS_ME, 1000), "launch still works at full timed capacity");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, CHAOS_MAX_ACTIVATIONS, "launch preserves unrelated timed effects");
	CTR_TEST_CHECK_EQ(s_launchVelocity[0].y, 27576, "Low Gravity does not change the launch impulse");
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 250, "Low Gravity continues to govern the flight");
	Chaos_Activate(CHAOS_EFFECT_HEAVY_METAL, 5000);
	Chaos_Activate(CHAOS_EFFECT_MY_PLANET_NEEDS_ME, 1000);
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 3000, "Heavy Karts also remains effective after launch");
	human.actionsFlagSet |= ACTION_RACE_FINISHED;
	const int launches = s_launchConversions[0];
	Chaos_Activate(CHAOS_EFFECT_MY_PLANET_NEEDS_ME, 1000);
	CTR_TEST_CHECK_EQ(s_launchConversions[0], launches, "finished humans receive no new launch");
}

static void TestLaunchSpeedLimit(void)
{
	CTR_TEST_SECTION("launches fit canonical speed without reversing at high speed or on repeated activation");
	CTR_TEST_CHECK_EQ(VehPhysForce_ClampTerminalVelocity(27000, 27576, 8000), 27000,
	    "gravity reduces upward launch speed even above terminal velocity");
	CTR_TEST_CHECK_EQ(VehPhysForce_ClampTerminalVelocity(-27000, -27576, 8000), -27000,
	    "inward acceleration also reduces downward overspeed");
	CTR_TEST_CHECK_EQ(VehPhysForce_ClampTerminalVelocity(28000, 27576, 8000), 27576,
	    "outward acceleration cannot increase existing overspeed");
	CTR_TEST_CHECK_EQ(VehPhysForce_ClampTerminalVelocity(9000, 7500, 8000), 8000,
	    "outward acceleration from within the limit stops at terminal velocity");
	CTR_TEST_CHECK_EQ(VehPhysForce_ClampTerminalVelocity(-9000, -7500, 8000), -8000,
	    "downward acceleration from within the limit stops at terminal velocity");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	for (int i = 0; i < ANG_HALF_PI; i++)
	{
		const double angle = i * atan2(1.0, 0.0) / ANG_HALF_PI;
		data.trigApprox[i] = (struct TrigTable){
		    .sin = (s16)lround(sin(angle) * FP(1.0)),
		    .cos = (s16)lround(cos(angle) * FP(1.0)),
		};
	}
	s_convertLaunchSpeed = true;
	const int speeds[] = {0, 3000, 25600, INT16_MAX, INT16_MIN};
	const int pitches[] = {0, ANG_HALF_PI / 2, -ANG_HALF_PI / 2};
	const int forces[] = {4596, 8000};
	for (int speed = 0; speed < (int)len(speeds); speed++)
	{
		for (int heading = 0; heading < ANG_TWO_PI; heading += ANG_HALF_PI / 2)
		{
			for (int pitch = 0; pitch < (int)len(pitches); pitch++)
			{
				for (int force = 0; force < (int)len(forces); force++)
				{
					struct Driver driver = {
					    .speed = speeds[speed], .axisRotationX = heading, .axisRotationY = pitches[pitch],
					    .const_JumpForce = forces[force],
					};
					driver.matrixMovingDir.m[0][0] = FP(1.0);
					driver.matrixMovingDir.m[1][1] = FP(1.0);
					driver.matrixMovingDir.m[2][2] = FP(1.0);
					for (int repeat = 0; repeat < 3; repeat++)
					{
						Vec3 before;
						VehPhysForce_ConvertSpeedToVec(&driver, &before);
						Chaos_LaunchDriver(&driver);
						const Vec3 launch = driver.velocity;
						const s64 limitSquared = (s64)INT16_MAX * INT16_MAX;
						const s64 launchSquared = (s64)launch.x * launch.x + (s64)launch.y * launch.y + (s64)launch.z * launch.z;
						CTR_TEST_CHECK(launchSquared <= limitSquared && driver.speed >= 0,
						    "the full launch vector remains representable as positive signed-16-bit speed");
						if ((s64)before.x * before.x + (s64)before.z * before.z <= limitSquared)
						{
							CTR_TEST_CHECK(launch.x == before.x && launch.z == before.z,
							    "the launch preserves representable horizontal momentum exactly");
						}
						Vec3 restored;
						VehPhysForce_ConvertSpeedToVec(&driver, &restored);
						CTR_TEST_CHECK((s64)restored.x * launch.x + (s64)restored.z * launch.z >= 0,
						    "reconstructing canonical motion never reverses horizontal direction");
						CTR_TEST_CHECK((s64)restored.y * launch.y >= 0,
						    "reconstructing canonical motion preserves the vertical direction");
					}
				}
			}
		}
	}
	s_convertLaunchSpeed = false;
}

static struct GamepadBuffer ShuffledPad(const struct Driver *driver, const struct GamepadBuffer *raw)
{
	struct GamepadBuffer pad = *raw;
	Chaos_ApplyInputRules(driver, &pad);
	return pad;
}

static void TestWhoBoundThis(void)
{
	CTR_TEST_SECTION("Who Bound This rolls independent stable nonidentity permutations for human driving");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_WHO_BOUND_THIS] = true;
	sdata->chaosSettings.enabledCount++;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_WHO_BOUND_THIS, 1000), "the control shuffle activates as a timed effect");
	for (int player = 0; player < 2; player++)
	{
		unsigned int seen = 0;
		bool changed = false;
		for (int action = 0; action < CHAOS_CONTROL_COUNT; action++)
		{
			const int destination = sdata->chaosRuntime.controls[player].destination[action];
			seen |= 1u << destination;
			changed |= destination != action;
		}
		CTR_TEST_CHECK_EQ(seen, (1u << CHAOS_CONTROL_COUNT) - 1, "each human receives every driving action exactly once");
		CTR_TEST_CHECK(changed, "a complete identity permutation is rerolled");
	}
	struct Driver human = {.driverID = 0};
	struct Driver partner = {.driverID = 1};
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT};
	// Cross -> item, Square -> R1, Circle -> L1, L1 -> throttle, R1 -> brake.
	sdata->chaosRuntime.controls[0].destination[CHAOS_CONTROL_THROTTLE] = CHAOS_CONTROL_ITEM;
	sdata->chaosRuntime.controls[0].destination[CHAOS_CONTROL_BRAKE] = CHAOS_CONTROL_HOP_RIGHT;
	sdata->chaosRuntime.controls[0].destination[CHAOS_CONTROL_ITEM] = CHAOS_CONTROL_HOP_LEFT;
	sdata->chaosRuntime.controls[0].destination[CHAOS_CONTROL_HOP_LEFT] = CHAOS_CONTROL_THROTTLE;
	sdata->chaosRuntime.controls[0].destination[CHAOS_CONTROL_HOP_RIGHT] = CHAOS_CONTROL_BRAKE;
	sdata->chaosRuntime.controls[1].destination[CHAOS_CONTROL_THROTTLE] = CHAOS_CONTROL_BRAKE;
	sdata->chaosRuntime.controls[1].destination[CHAOS_CONTROL_BRAKE] = CHAOS_CONTROL_ITEM;
	sdata->chaosRuntime.controls[1].destination[CHAOS_CONTROL_ITEM] = CHAOS_CONTROL_THROTTLE;
	sdata->chaosRuntime.controls[1].destination[CHAOS_CONTROL_HOP_LEFT] = CHAOS_CONTROL_HOP_RIGHT;
	sdata->chaosRuntime.controls[1].destination[CHAOS_CONTROL_HOP_RIGHT] = CHAOS_CONTROL_HOP_LEFT;
	const u32 inputs[] = {BTN_CROSS_one, BTN_SQUARE_one, BTN_CIRCLE, BTN_L1, BTN_R1};
	const u32 outputs[] = {BTN_CIRCLE, BTN_R1, BTN_L1, BTN_CROSS_one, BTN_SQUARE_one};
	const u32 untouched = BTN_START | BTN_SELECT | BTN_LEFT | BTN_R2;
	for (size_t i = 0; i < len(inputs); i++)
	{
		const struct GamepadBuffer raw = {
			.stickLX = 42, .stickLY = CHAOS_STICK_CENTER, .stickRY = CHAOS_STICK_CENTER,
			.buttonsHeldCurrFrame = inputs[i] | untouched,
			.buttonsTapped = inputs[i] | BTN_START,
		};
		struct GamepadBuffer pad = ShuffledPad(&human, &raw);
		CTR_TEST_CHECK_EQ((u32)pad.buttonsHeldCurrFrame, outputs[i] | untouched, "only the five gameplay buttons are permuted");
		CTR_TEST_CHECK_EQ((u32)pad.buttonsTapped, outputs[i] | BTN_START, "tap edges follow the mapped action without changing pause");
		CTR_TEST_CHECK_EQ(pad.stickLX, 42, "left-stick steering is unchanged");
		struct GamepadBuffer released = raw;
		released.buttonsHeldPrevFrame = inputs[i];
		released.buttonsHeldCurrFrame = 0;
		released.buttonsTapped = 0;
		released.buttonsReleased = inputs[i];
		pad = ShuffledPad(&human, &released);
		CTR_TEST_CHECK(pad.buttonsReleased == (int)outputs[i] && pad.buttonsHeldPrevFrame == (int)outputs[i],
		               "release edges and previous held state use the same mapping");
	}
	const struct GamepadBuffer raw = {
		.stickLX = 42, .stickLY = CHAOS_STICK_CENTER, .stickRY = CHAOS_STICK_CENTER,
		.buttonsHeldCurrFrame = BTN_CROSS_one, .buttonsTapped = BTN_CROSS_one,
	};
	const struct GamepadBuffer original = raw;
	struct GamepadBuffer pad = ShuffledPad(&partner, &raw);
	CTR_TEST_CHECK_EQ(pad.buttonsHeldCurrFrame, BTN_SQUARE_one, "player two uses their own permutation");
	pad = ShuffledPad(&bot, &raw);
	CTR_TEST_CHECK(memcmp(&pad, &raw, sizeof(pad)) == 0, "bots retain ordinary controls");
	human.actionsFlagSet = ACTION_RACE_FINISHED;
	pad = ShuffledPad(&human, &raw);
	CTR_TEST_CHECK(memcmp(&pad, &raw, sizeof(pad)) == 0, "finished humans retain ordinary controls");
	human.actionsFlagSet = 0;
	s_tracker.gameMode1 |= PAUSE_1;
	pad = ShuffledPad(&human, &raw);
	CTR_TEST_CHECK(memcmp(&pad, &raw, sizeof(pad)) == 0, "paused menus receive unmodified input");
	s_tracker.gameMode1 &= ~PAUSE_1;
	const u32 rng = sdata->chaosRng;
	pad = ShuffledPad(&human, &raw);
	const struct GamepadBuffer again = ShuffledPad(&human, &raw);
	CTR_TEST_CHECK(memcmp(&pad, &again, sizeof(pad)) == 0 && sdata->chaosRng == rng,
	               "physics and drift consumers can resolve the same frame without reshuffling or double mapping");
	CTR_TEST_CHECK(memcmp(&raw, &original, sizeof(raw)) == 0, "the shared raw pad remains intact");
	const struct ChaosRuntime saved = sdata->chaosRuntime;
	Chaos_Activate(CHAOS_EFFECT_WHO_BOUND_THIS, 1000);
	CTR_TEST_CHECK(sdata->chaosRng != rng, "reactivation rolls a fresh set of per-human mappings");
	sdata->chaosRuntime = saved;
	pad = ShuffledPad(&human, &raw);
	CTR_TEST_CHECK_EQ(pad.buttonsHeldCurrFrame, BTN_CIRCLE, "resident restoration restores the exact previous mapping");

	CTR_TEST_SECTION("analog throttle and reverse follow the shuffle with coherent press and release edges");
	s_gamepads.gamepad[0].stickRY = 0;
	Tick(32);
	pad = ShuffledPad(&human, &s_gamepads.gamepad[0]);
	CTR_TEST_CHECK(pad.buttonsHeldCurrFrame == BTN_CIRCLE && pad.buttonsTapped == BTN_CIRCLE,
	               "analog throttle fires its mapped action on entry");
	CTR_TEST_CHECK_EQ(pad.stickRY, CHAOS_STICK_CENTER, "the original analog throttle path is neutralized");
	Tick(32);
	pad = ShuffledPad(&human, &s_gamepads.gamepad[0]);
	CTR_TEST_CHECK(pad.buttonsHeldCurrFrame == BTN_CIRCLE && pad.buttonsTapped == 0, "holding an axis does not repeat item or hop taps");
	s_gamepads.gamepad[0].buttonsHeldCurrFrame = BTN_CROSS_one;
	Tick(32);
	pad = ShuffledPad(&human, &s_gamepads.gamepad[0]);
	CTR_TEST_CHECK_EQ(pad.buttonsTapped, 0, "pressing a digital source already held by its analog axis does not duplicate the action");
	s_gamepads.gamepad[0].buttonsHeldPrevFrame = BTN_CROSS_one;
	s_gamepads.gamepad[0].stickRY = CHAOS_STICK_CENTER;
	Tick(32);
	pad = ShuffledPad(&human, &s_gamepads.gamepad[0]);
	CTR_TEST_CHECK_EQ(pad.buttonsReleased, 0, "centering the axis does not release an action still held digitally");
	s_gamepads.gamepad[0].buttonsHeldCurrFrame = 0;
	s_gamepads.gamepad[0].buttonsHeldPrevFrame = 0;
	s_gamepads.gamepad[0].stickLY = 255;
	Tick(32);
	pad = ShuffledPad(&human, &s_gamepads.gamepad[0]);
	CTR_TEST_CHECK((pad.buttonsHeldCurrFrame & BTN_R1) != 0 && pad.stickLY == CHAOS_STICK_CENTER,
	               "left-stick reverse triggers the remapped brake action without an unmapped reverse bypass");
	Tick(1000);
	pad = ShuffledPad(&human, &s_gamepads.gamepad[0]);
	CTR_TEST_CHECK(memcmp(&pad, &s_gamepads.gamepad[0], sizeof(pad)) == 0, "expiry immediately returns the complete live pad");
}

static void TestTrySomethingNew(void)
{
	CTR_TEST_SECTION("Try Something New changes class constants until race end without changing character or race state");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_TRY_SOMETHING_NEW] = true;
	sdata->chaosSettings.enabledCount++;
	data.characterIDs[0] = CRASH_BANDICOOT;
	data.characterIDs[1] = COCO_BANDICOOT;
	data.MetaDataCharacters[CRASH_BANDICOOT].engineID = BALANCED;
	data.MetaDataCharacters[COCO_BANDICOOT].engineID = ACCEL;
	const struct MetaDataCHAR crashMetadata = data.MetaDataCharacters[CRASH_BANDICOOT];
	const struct MetaDataCHAR cocoMetadata = data.MetaDataCharacters[COCO_BANDICOOT];
	data.metaPhys[0] = (struct MetaPhys){.offset = SPEED_CLASS_STAT_OFFSET, .size = 2, .value = {10000, 12000, 14000, 8000}};
	data.metaPhys[1] = (struct MetaPhys){.offset = TURN_RATE_OFFSET, .size = 1, .value = {28, 26, 24, 48}};
	data.metaPhys[2] = (struct MetaPhys){.offset = GRAVITY_OFFSET, .size = 2, .value = {900, 900, 900, 900}};
	data.metaPhys[3] = (struct MetaPhys){.offset = JUMP_OFFSET, .size = 2, .value = {4596, 4596, 4596, 4596}};
	data.metaPhys[4] = (struct MetaPhys){.offset = TURBO_FULL_BAR_RESERVE_GAIN_OFFSET, .size = 1, .value = {10, 20, 30, 40}};
	g_config.speedMultiplier = 150;
	g_config.turnMultiplier = 100;
	g_config.gravityMultiplier = 200;
	g_config.jumpMultiplier = 50;
	g_config.reserveMultiplier = 100;
	struct Driver human = {
		.driverID = 0, .reserves = 500, .fireSpeedCap = 4096, .heldItemID = HELD_ITEM_POTION,
		.posCurr = {10, 20, 30}, .speed = 10000,
	};
	struct Driver partner = {.driverID = 1};
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT, .const_Speed_ClassStat = 1234};
	s_tracker.drivers[0] = &human;
	s_tracker.drivers[1] = &partner;
	s_tracker.drivers[2] = &bot;
	VehBirth_SetConsts(&human);
	VehBirth_SetConsts(&partner);
	CTR_TEST_CHECK_EQ(human.const_Speed_ClassStat, 15000, "ordinary vehicle birth retains character-based class and configured multipliers");
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 10000);
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_TRY_SOMETHING_NEW, 1000), "the class change activates as a one-off effect");
	int current = sdata->chaosRuntime.engineClass[0];
	CTR_TEST_CHECK_EQ(s_engineStopped[0], BALANCED * 4, "the old engine loop stops at the class change");
	CTR_TEST_CHECK_EQ(s_engineStarted[0], (u32)(current * 4), "the new class engine loop starts immediately");
	CTR_TEST_CHECK_EQ(Chaos_EngineClassForDriver(&human, BALANCED), current, "ongoing engine updates resolve the selected class");
	CTR_TEST_CHECK_EQ(s_engineStarted[1], (u32)(sdata->chaosRuntime.engineClass[1] * 4 + 1), "each human uses their own engine audio slot");
	struct Driver ghost = {.driverID = 0};
	CTR_TEST_CHECK_EQ(Chaos_EngineClassForDriver(&ghost, BALANCED), BALANCED, "a ghost sharing a driver ID retains its own engine class");
	CTR_TEST_CHECK(current != BALANCED && current < NUM_CLASSES, "the first class change excludes the character's current standard class");
	CTR_TEST_CHECK(sdata->chaosRuntime.engineClass[1] != ACCEL && sdata->chaosRuntime.engineClassChanged[1],
	               "the second human receives a class different from their own baseline");
	CTR_TEST_CHECK_EQ(human.const_Speed_ClassStat, data.metaPhys[0].value[current] * 150 / 100, "new class speed retains the configured multiplier");
	CTR_TEST_CHECK_EQ(human.const_TurnRate, data.metaPhys[1].value[current], "byte-sized handling stats switch with the class");
	CTR_TEST_CHECK_EQ(human.const_turboFullBarReserveGain, data.metaPhys[4].value[current], "reserve-gain constants switch through the shared table loader");
	CTR_TEST_CHECK(human.const_Gravity == 1800 && human.const_JumpForce == 2298, "class reload preserves configured gravity and jump multipliers");
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 450, "existing continuous Chaos modifiers still use the new constants");
	CTR_TEST_CHECK(human.reserves == 500 && human.fireSpeedCap == 4096 && human.heldItemID == HELD_ITEM_POTION,
	               "class changes preserve current boost and inventory");
	CTR_TEST_CHECK(human.posCurr.x == 10 && human.posCurr.y == 20 && human.posCurr.z == 30 && human.speed == 10000,
	               "class changes do not reset position or momentum");
	CTR_TEST_CHECK_EQ(bot.const_Speed_ClassStat, 1234, "bots retain their existing class constants");
	CTR_TEST_CHECK(data.characterIDs[0] == CRASH_BANDICOOT && data.characterIDs[1] == COCO_BANDICOOT &&
	               memcmp(&crashMetadata, &data.MetaDataCharacters[CRASH_BANDICOOT], sizeof(crashMetadata)) == 0 &&
	               memcmp(&cocoMetadata, &data.MetaDataCharacters[COCO_BANDICOOT], sizeof(cocoMetadata)) == 0,
	               "characters and shared character metadata remain unchanged");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 1, "the class change takes no timed slot and preserves Low Gravity");
	Tick(4000);
	CTR_TEST_CHECK_EQ(human.const_Speed_ClassStat, data.metaPhys[0].value[current] * 150 / 100, "notice expiry does not restore the old class");
	for (int i = 0; i < 20; i++)
	{
		Chaos_Activate(CHAOS_EFFECT_TRY_SOMETHING_NEW, 1000);
		const int next = sdata->chaosRuntime.engineClass[0];
		CTR_TEST_CHECK(next != current && next < NUM_CLASSES, "repeat activations exclude the live Chaos class rather than the character baseline");
		CTR_TEST_CHECK_EQ(s_engineStopped[0], (u32)(current * 4), "repeat changes stop the currently playing class loop");
		CTR_TEST_CHECK_EQ(s_engineStarted[0], (u32)(next * 4), "repeat changes start the newly selected engine loop");
		current = next;
	}
	const struct ChaosRuntime saved = sdata->chaosRuntime;
	Chaos_Reset();
	CTR_TEST_CHECK_EQ(s_engineStopped[0], (u32)(current * 4), "teardown stops the changed engine loop");
	CTR_TEST_CHECK_EQ(s_engineStarted[0], BALANCED * 4, "teardown restores the character engine for post-race updates");
	CTR_TEST_CHECK_EQ(Chaos_EngineClassForDriver(&human, BALANCED), BALANCED, "teardown clears the audio class override");
	CTR_TEST_CHECK(!sdata->chaosRuntime.engineClassChanged[0], "race teardown clears class-selection bookkeeping");
	sdata->chaosRuntime = saved;
	const int partnerClass = sdata->chaosRuntime.engineClass[1];
	partner.actionsFlagSet = ACTION_RACE_FINISHED;
	const int partnerStarts = s_engineStarts[1];
	Chaos_Activate(CHAOS_EFFECT_TRY_SOMETHING_NEW, 1000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.engineClass[1], partnerClass, "finished humans receive no further class changes");
	CTR_TEST_CHECK_EQ(s_engineStarts[1], partnerStarts, "finished humans receive no engine restart on later activations");
	Chaos_BeginRace(&s_tracker);
	VehBirth_SetConsts(&human);
	CTR_TEST_CHECK_EQ(human.const_Speed_ClassStat, 15000, "the next race starts with the character's ordinary class");
}

static void TestIdentityCrisis(void)
{
	CTR_TEST_SECTION("Identity Crisis prepares resident models only for eligible races without consuming Chaos RNG");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	Chaos_LoadCharacterModels(&s_tracker, NULL);
	CTR_TEST_CHECK_EQ(s_characterModelReads, 0, "disabled Identity Crisis does not load extra models");
	sdata->chaosSettings.effects[CHAOS_EFFECT_IDENTITY_CRISIS] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_TRY_SOMETHING_NEW] = true;
	sdata->chaosSettings.enabledCount += 2;
	s_tracker.gameMode1 |= ADVENTURE_ARENA;
	Chaos_LoadCharacterModels(&s_tracker, NULL);
	CTR_TEST_CHECK_EQ(s_characterModelReads, 0, "Adventure hubs do not load transformation models");
	s_tracker.gameMode1 &= ~ADVENTURE_ARENA;
	for (int character = 0; character < GAME_CHARACTER_COUNT; character++)
	{
		snprintf(s_characterModelFiles[character].model.name, sizeof(s_characterModelFiles[character].model.name), "racer%d", character);
		data.MetaDataCharacters[character].name_Debug = s_characterModelFiles[character].model.name;
	}
	s_residentCharacterModels[CRASH_BANDICOOT] = &s_characterModelFiles[CRASH_BANDICOOT].model;
	const u32 rng = sdata->chaosRng;
	Chaos_LoadCharacterModels(&s_tracker, NULL);
	CTR_TEST_CHECK_EQ(s_characterModelReads, GAME_CHARACTER_COUNT - 1, "only missing playable models are loaded regardless of unlocks");
	CTR_TEST_CHECK(sdata->chaosCharacterModels[CRASH_BANDICOOT] == s_residentCharacterModels[CRASH_BANDICOOT],
	               "an already resident character model is reused");
	CTR_TEST_CHECK_EQ(sdata->chaosRng, rng, "resource preparation leaves effect selection deterministic");
	for (int i = 0; i < GAME_CHARACTER_COUNT; i++)
	{
		CTR_TEST_CHECK(sdata->chaosCharacterModels[i] == &s_characterModelFiles[i].model,
		               "the resident model excludes the relocatable file header");
		data.MetaDataCharacters[i].engineID = i % NUM_CLASSES;
	}
	data.metaPhys[0] = (struct MetaPhys){.offset = SPEED_CLASS_STAT_OFFSET, .size = 2, .value = {10000, 12000, 14000, 8000}};
	g_config.speedMultiplier = 100;
	struct Model originalModels[2] = {0};
	struct Instance instances[2] = {{.model = &originalModels[0], .animFrame = 100}, {.model = &originalModels[1]}};
	struct Driver human = {
		.driverID = 0, .instSelf = &instances[0], .heldItemID = HELD_ITEM_POTION,
		.reserves = 500, .speed = 10000, .posCurr = {10, 20, 30}, .kartState = KS_SPINNING,
	};
	struct Driver partner = {.driverID = 1, .instSelf = &instances[1]};
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT};
	s_tracker.drivers[0] = &human;
	s_tracker.drivers[1] = &partner;
	s_tracker.drivers[2] = &bot;
	data.characterIDs[0] = CRASH_BANDICOOT;
	data.characterIDs[1] = COCO_BANDICOOT;
	data.characterIDs[2] = NITROS_OXIDE;
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 10000);
	for (int repeat = 0; repeat < 20; repeat++)
	{
		const int beforeHuman = data.characterIDs[0];
		const int beforePartner = data.characterIDs[1];
		const int previousClass = Chaos_EngineClassForDriver(&human, data.MetaDataCharacters[beforeHuman].engineID);
		CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_IDENTITY_CRISIS, 1000), "Identity Crisis activates as an instant effect");
		CTR_TEST_CHECK_EQ(s_identitySounds, repeat + 1, "each transformation plays one invisibility sound for both humans together");
		CTR_TEST_CHECK(data.characterIDs[0] == beforeHuman && data.characterIDs[1] == beforePartner,
		               "the original models remain while the transformation smoke expands");
		Tick(CHAOS_TRANSFORM_DELAY_MS);
		const int character = data.characterIDs[0];
		const int engine = data.MetaDataCharacters[character].engineID;
		CTR_TEST_CHECK(character != beforeHuman && character != beforePartner && character != NITROS_OXIDE,
		               "the first human excludes every character present before their change");
		CTR_TEST_CHECK(data.characterIDs[1] != character && data.characterIDs[1] != beforePartner && data.characterIDs[1] != NITROS_OXIDE,
		               "the partner excludes their current character, the transformed human and bots");
		CTR_TEST_CHECK(instances[0].model == sdata->chaosCharacterModels[character], "the existing instance uses the selected character model");
		CTR_TEST_CHECK_EQ(human.const_Speed_ClassStat, data.metaPhys[0].value[engine], "the selected character supplies the driving class");
		CTR_TEST_CHECK_EQ(s_engineStopped[0], (u32)(previousClass * 4), "transformation stops the live class engine");
		CTR_TEST_CHECK_EQ(s_engineStarted[0], (u32)(engine * 4), "transformation starts the selected character engine");
		CTR_TEST_CHECK(sdata->chaosRuntime.identity[0].originalCharacter == CRASH_BANDICOOT &&
		               sdata->chaosRuntime.identity[0].originalModel == &originalModels[0],
		               "repeated transformations preserve the original race identity");
		Chaos_Activate(CHAOS_EFFECT_TRY_SOMETHING_NEW, 1000);
		CTR_TEST_CHECK(sdata->chaosRuntime.engineClass[0] != engine && data.characterIDs[0] == character,
		               "a later Try Something New replaces the class without replacing the identity");
	}
	CTR_TEST_CHECK(human.heldItemID == HELD_ITEM_POTION && human.reserves == 500 && human.speed == 10000 &&
	               human.posCurr.x == 10 && human.posCurr.y == 20 && human.posCurr.z == 30 && human.kartState == KS_SPINNING,
	               "identity and class changes preserve inventory, motion, reserves and damage state");
	CTR_TEST_CHECK_EQ(s_characterModelReads, GAME_CHARACTER_COUNT - 1, "activations perform no further asset reads");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 1, "transformations preserve the timed effect without taking a slot");
	CTR_TEST_CHECK_EQ(s_transformationSmokeCount, 20 * CHAOS_TRANSFORM_SMOKE_COUNT * 2, "each human receives a complete smoke cloud");
	human.kartScale = 2.0f;
	partner.kartScale = 0.5f;
	Chaos_Activate(CHAOS_EFFECT_IDENTITY_CRISIS, 1000);
	CTR_TEST_CHECK(s_transformationSmoke[0].owner.driverInst == &instances[0] &&
	               s_transformationSmoke[CHAOS_TRANSFORM_SMOKE_COUNT].owner.driverInst == &instances[1],
	               "each cloud stays attached to its own moving kart");
	CTR_TEST_CHECK(s_transformationSmoke[0].driverID == -1 &&
	               (s_transformationSmoke[0].flagsSetColor & PARTICLE_SET_COLOR_FLAG_DRIVER_LOCAL) != 0,
	               "attached smoke is visible from every split-screen camera");
	CTR_TEST_CHECK_EQ(s_transformationSmoke[0].axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal,
	                  CHAOS_TRANSFORM_SMOKE_SCALE * 2, "giant karts receive proportionally larger smoke");
	CTR_TEST_CHECK_EQ(s_transformationSmoke[CHAOS_TRANSFORM_SMOKE_COUNT].axis[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE].startVal,
	                  CHAOS_TRANSFORM_SMOKE_SCALE / 2, "small karts receive proportionally smaller smoke");
	s_tracker.gameMode1 |= PAUSE_1;
	Tick(500);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.identity[0].transformRemainingMS, CHAOS_TRANSFORM_DELAY_MS,
	                  "pause freezes the pending model change together with its visual effect");
	s_tracker.gameMode1 &= ~PAUSE_1;
	Chaos_Activate(CHAOS_EFFECT_TRY_SOMETHING_NEW, 1000);
	const int newerClass = sdata->chaosRuntime.engineClass[0];
	Tick(CHAOS_TRANSFORM_DELAY_MS - 1);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.identity[0].transformRemainingMS, 1, "the change waits for the full smoke expansion delay");
	Tick(1);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.engineClass[0], newerClass, "a pending model change preserves a more recent class activation");
	s_transformationSmokeExhausted = true;
	const int finishedCharacter = data.characterIDs[1];
	partner.actionsFlagSet = ACTION_RACE_FINISHED;
	Chaos_Activate(CHAOS_EFFECT_IDENTITY_CRISIS, 1000);
	CTR_TEST_CHECK_EQ(data.characterIDs[1], finishedCharacter, "finished humans receive no further transformations");
	CTR_TEST_CHECK_EQ(data.characterIDs[2], NITROS_OXIDE, "bots keep their identity");
	Tick(4000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.identity[0].transformRemainingMS, 0, "particle pool exhaustion does not block transformation completion");
	CTR_TEST_CHECK_EQ(data.characterIDs[1], finishedCharacter, "announcement expiry does not undo a transformation");
	Chaos_Reset();
	CTR_TEST_CHECK(data.characterIDs[0] == CRASH_BANDICOOT && data.characterIDs[1] == COCO_BANDICOOT &&
	               instances[0].model == &originalModels[0] && instances[1].model == &originalModels[1],
	               "race teardown restores both original identities, including finished humans");
	CTR_TEST_CHECK_EQ(s_engineStarted[0], (u32)(data.MetaDataCharacters[CRASH_BANDICOOT].engineID * 4),
	                  "race teardown restores the original character engine after mixed class changes");
}

struct BodySwapWorld
{
	struct Driver drivers[LOAD_CHARACTER_ID_COUNT];
	struct Instance instances[LOAD_CHARACTER_ID_COUNT];
	struct Thread threads[LOAD_CHARACTER_ID_COUNT];
};

static void SetupBodySwap(struct BodySwapWorld *world, int humans, int bots)
{
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	memset(world, 0, sizeof(*world));
	memset(s_tracker.drivers, 0, sizeof(s_tracker.drivers));
	s_tracker.numPlyrCurrGame = humans;
	s_tracker.numLaps = 3;
	sdata->chaosSettings.effects[CHAOS_EFFECT_BODY_SWAP] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_TRY_SOMETHING_NEW] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_SMALL_KART_ENERGY] = true;
	sdata->chaosSettings.enabledCount += 3;
	for (int i = 0; i < humans + bots; i++)
	{
		const bool human = i < humans;
		const int bucket = human ? PLAYER : ROBOT;
		world->drivers[i] = (struct Driver){
			.driverID = i, .instSelf = &world->instances[i], .kartScale = 1.0f,
			.actionsFlagSet = human ? 0 : ACTION_BOT, .heldItemID = HELD_ITEM_POTION,
			.reserves = 100 + i, .lapIndex = i % 2, .distanceToFinish_curr = 10000,
			.posCurr = {1000 * i, 0, 2000 * i}, .speed = 1000 + i,
		};
		world->threads[i] = (struct Thread){
			.object = &world->drivers[i], .inst = &world->instances[i], .flags = bucket,
			.modelIndex = human ? DYNAMIC_PLAYER : DYNAMIC_ROBOT_CAR,
			.funcThTick = human ? NULL : TestBotTick,
			.siblingThread = s_tracker.threadBuckets[bucket].thread,
		};
		world->instances[i].thread = &world->threads[i];
		s_tracker.threadBuckets[bucket].thread = &world->threads[i];
		s_tracker.drivers[i] = &world->drivers[i];
		data.characterIDs[i] = i;
		data.MetaDataCharacters[i].engineID = i % NUM_CLASSES;
		if (!human)
		{
			LIST_AddFront(&sdata->navBotList[0], &world->drivers[i].botData.item);
		}
	}
}

static void TestBodySwap(void)
{
	CTR_TEST_SECTION("bot takeover replaces spawn-only anti-cut references and carries actual world motion into human physics");
	struct BodySwapWorld handover;
	SetupBodySwap(&handover, 1, 1);
	struct Driver *borrowed = &handover.drivers[1];
	struct QuadBlock spawnQuad = {0};
	borrowed->lastValid = &spawnQuad;
	borrowed->underDriver = &spawnQuad;
	borrowed->currBlockTouching = &spawnQuad;
	borrowed->distanceToFinish_checkpoint = 80000;
	borrowed->distanceToFinish_curr = 12000;
	borrowed->checkpoint.currentIndex = 0;
	borrowed->botData.ai_quadblock_checkpointIndex = 27;
	borrowed->collisionFlags = DRIVER_COLL_FLAG_MASK_GRAB_REQUEST;
	borrowed->stepFlagSet = COLL_STEP_FLAG_KILL_PLANE;
	borrowed->xSpeed = 300;
	borrowed->ySpeed = -400;
	borrowed->zSpeed = 1200;
	borrowed->rotCurr.w = -0xf80;
	borrowed->rotPrev.w = 8;
	s_tracker.cameraDC[0].flags = CAMERA_FLAG_MASK_GRAB;
	s_tracker.cameraDC[0].BlastedLerp.boolLerpPending = true;
	s_tracker.cameraDC[0].BlastedLerp.framesRemaining = 7;
	s_tracker.cameraDC[0].BlastedLerp.desiredPos.x = 2000;
	s_tracker.cameraDC[0].heightSmoothing.currentOffset = 500;
	s_tracker.cameraDC[0].heightSmoothing.framesRemaining = 6;
	s_tracker.cameraDC[0].nearOrFar = 1;
	const Vec3 position = borrowed->posCurr;
	Chaos_Activate(CHAOS_EFFECT_BODY_SWAP, 1000);
	CTR_TEST_CHECK(s_tracker.drivers[0] == borrowed && borrowed->distanceToFinish_checkpoint == 12000 &&
	               borrowed->checkpoint.currentIndex == 27,
	               "the human anti-cut baseline starts at the bot's live progression rather than its spawn checkpoint");
	CTR_TEST_CHECK(borrowed->rotCurr.w == 0 && borrowed->rotPrev.w == 0,
	               "AI wrapped camera lag does not become a nearly full-turn human recovery offset");
	CTR_TEST_CHECK((s_tracker.cameraDC[0].flags & CAMERA_FLAG_MASK_GRAB) == 0 &&
	               (s_tracker.cameraDC[0].flags & CAMERA_FLAG_DIRECTION_CHANGED) != 0 &&
	               s_tracker.cameraDC[0].BlastedLerp.framesRemaining == 0 &&
	               !s_tracker.cameraDC[0].BlastedLerp.boolLerpPending &&
	               s_tracker.cameraDC[0].heightSmoothing.currentOffset == 0 &&
	               s_tracker.cameraDC[0].heightSmoothing.framesRemaining == 0,
	               "camera handover drops the previous kart's damage, rescue and height-recovery state");
	CTR_TEST_CHECK_EQ(s_tracker.cameraDC[0].nearOrFar, 1, "camera handover retains the player's zoom preference");
	CTR_TEST_CHECK(Chaos_ConsumeCameraBlend(&s_tracker.cameraDC[0]) &&
	               !Chaos_ConsumeCameraBlend(&s_tracker.cameraDC[0]),
	               "the camera blends to the possessed kart once without requiring a Fresh Start countdown");
	CTR_TEST_CHECK(borrowed->lastValid == NULL && borrowed->underDriver == NULL && borrowed->currBlockTouching == NULL,
	               "the first human collision acquires the current surface without retaining a stale spawn quad");
	CTR_TEST_CHECK(borrowed->collisionFlags == 0 && borrowed->stepFlagSet == 0,
	               "stale collision/rescue flags cannot trigger a fresh rescue on takeover");
	CTR_TEST_CHECK(memcmp(&borrowed->posCurr, &position, sizeof(position)) == 0 &&
	               memcmp(&borrowed->posPrev, &position, sizeof(position)) == 0,
	               "taking control does not reposition the kart or sweep collisions from a stale previous position");
	Vec3 motion;
	VehPhysForce_ConvertSpeedToVec(borrowed, &motion);
	CTR_TEST_CHECK(abs(motion.x - 300) < 10 && abs(motion.y + 400) < 10 && abs(motion.z - 1200) < 10,
	               "real polar conversion preserves the bot's horizontal and vertical motion");

	CTR_TEST_SECTION("ownership changes during an existing rescue without replacing its mask or waiting for landing");
	SetupBodySwap(&handover, 1, 1);
	struct MaskHeadWeapon rescueMask = {0};
	borrowed = &handover.drivers[1];
	borrowed->kartState = KS_MASK_GRABBED;
	borrowed->botData.maskObj = &rescueMask;
	handover.threads[1].funcThTick = BOTS_ThTick_RevEngine;
	Chaos_Activate(CHAOS_EFFECT_BODY_SWAP, 1000);
	CTR_TEST_CHECK(s_tracker.drivers[0] == borrowed && borrowed->driverID == 0 &&
	               (borrowed->actionsFlagSet & ACTION_BOT) == 0 && s_tracker.cameraDC[0].driverToFollow == borrowed,
	               "input ownership and camera transfer immediately to the kart already under rescue");
	CTR_TEST_CHECK(borrowed->kartState == KS_MASK_GRABBED && borrowed->botData.maskObj == &rescueMask &&
	               handover.threads[1].funcThTick == BOTS_ThTick_RevEngine,
	               "the existing rescue mask and descent continue without another rescue activation");
	Tick(1000);
	CTR_TEST_CHECK(s_tracker.drivers[0] == &handover.drivers[0] && sdata->chaosRuntime.bodySwapPartner[0] == 0,
	               "expiry does not wait for the possessed kart's rescue to finish");

	CTR_TEST_SECTION("Body Swap exchanges controller ownership while complete kart state and class stay on the kart");
	struct BodySwapWorld world;
	SetupBodySwap(&world, 2, 0);
	world.drivers[0].rotCurr.w = 120;
	world.drivers[1].rotCurr.w = -160;
	s_tracker.cameraDC[0].BlastedLerp.boolLerpPending = true;
	s_tracker.cameraDC[1].BlastedLerp.framesRemaining = 4;
	sdata->chaosRuntime.engineClass[0] = SPEED;
	sdata->chaosRuntime.engineClassChanged[0] = true;
	world.drivers[1].kartState = KS_SPINNING;
	world.drivers[1].heldItemID = HELD_ITEM_MISSILE_3X;
	world.drivers[1].checkpoint.currentIndex = 7;
	Chaos_Activate(CHAOS_EFFECT_SMALL_KART_ENERGY, 5000);
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_BODY_SWAP, 1000), "Body Swap occupies a timed effect slot");
	CTR_TEST_CHECK(s_tracker.drivers[0] == &world.drivers[1] && s_tracker.drivers[1] == &world.drivers[0],
	               "each human controls the other complete driver object");
	CTR_TEST_CHECK(data.characterIDs[0] == 1 && data.characterIDs[1] == 0 &&
	               world.drivers[1].driverID == 0 && world.drivers[0].driverID == 1,
	               "character and input identities agree with controller slots");
	CTR_TEST_CHECK(s_tracker.drivers[0]->kartState == KS_SPINNING && s_tracker.drivers[0]->heldItemID == HELD_ITEM_MISSILE_3X &&
	               s_tracker.drivers[0]->checkpoint.currentIndex == 7 && s_tracker.drivers[0]->reserves == 101 &&
	               s_tracker.drivers[0]->lapIndex == 1 && s_tracker.drivers[0]->posCurr.x == 1000,
	               "damage, inventory, reserves, position and progression remain on the possessed kart");
	CTR_TEST_CHECK_EQ(Chaos_EngineClassForDriver(s_tracker.drivers[0], 1), 1, "the player takes the target kart class");
	CTR_TEST_CHECK_EQ(Chaos_EngineClassForDriver(s_tracker.drivers[1], 0), SPEED, "an existing class override remains with its original kart");
	CTR_TEST_CHECK(s_tracker.cameraDC[0].driverToFollow == s_tracker.drivers[0] &&
	               s_tracker.cameraDC[1].driverToFollow == s_tracker.drivers[1], "both cameras follow their newly possessed kart");
	CTR_TEST_CHECK(!s_tracker.cameraDC[0].BlastedLerp.boolLerpPending &&
	               s_tracker.cameraDC[1].BlastedLerp.framesRemaining == 0 &&
	               world.drivers[0].rotCurr.w == 120 && world.drivers[1].rotCurr.w == -160,
	               "both viewports reset old camera transitions while human kart drift offsets stay intact");
	Chaos_Activate(CHAOS_EFFECT_TRY_SOMETHING_NEW, 1000);
	const int borrowedClass = sdata->chaosRuntime.engineClass[0];
	Tick(1000);
	CTR_TEST_CHECK(s_tracker.drivers[0] == &world.drivers[0] && s_tracker.drivers[1] == &world.drivers[1],
	               "expiry returns both controllers to their original kart");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.engineClass[1], borrowedClass, "a class change during possession stays on the borrowed kart after return");
	CTR_TEST_CHECK(world.drivers[0].kartScale == 0.5f && world.drivers[1].kartScale == 0.5f,
	               "timed human size modifiers survive controller return");

	CTR_TEST_SECTION("human and bot possession changes physics buckets without replacing instances or navigation-list members");
	SetupBodySwap(&world, 1, 1);
	Chaos_Activate(CHAOS_EFFECT_SMALL_KART_ENERGY, 5000);
	Chaos_Activate(CHAOS_EFFECT_BODY_SWAP, 1000);
	CTR_TEST_CHECK(s_tracker.drivers[0] == &world.drivers[1] &&
	               (world.drivers[1].actionsFlagSet & ACTION_BOT) == 0 &&
	               (world.drivers[0].actionsFlagSet & ACTION_BOT) != 0,
	               "human control and AI control exchange physical karts");
	CTR_TEST_CHECK(s_tracker.threadBuckets[PLAYER].thread == &world.threads[1] &&
	               s_tracker.threadBuckets[ROBOT].thread == &world.threads[0], "the correct physics pipeline owns each thread");
	CTR_TEST_CHECK(world.threads[1].modelIndex == DYNAMIC_PLAYER && world.threads[1].funcThTick == NULL &&
	               world.threads[0].modelIndex == DYNAMIC_ROBOT_CAR && world.threads[0].funcThTick == TestBotTick,
	               "model identity and execution mode match the new controller");
	CTR_TEST_CHECK(sdata->navBotList[0].count == 1 && sdata->navBotList[0].first == &world.drivers[0].botData.item,
	               "the navigation list replaces the former bot with the unattended original kart");
	CTR_TEST_CHECK(world.drivers[1].kartScale == 0.5f && world.drivers[0].kartScale == 1.0f,
	               "the human size modifier follows the player rather than the old kart");
	const struct ChaosRuntime saved = sdata->chaosRuntime;
	s_tracker.gameMode1 |= PAUSE_1;
	Tick(5000);
	CTR_TEST_CHECK(memcmp(&saved, &sdata->chaosRuntime, sizeof(saved)) == 0, "pause freezes possession and its expiry");
	s_tracker.gameMode1 &= ~PAUSE_1;
	Chaos_Activate(CHAOS_EFFECT_BODY_SWAP, 2000);
	CTR_TEST_CHECK(s_tracker.drivers[0] == &world.drivers[1] && sdata->navBotList[0].count == 1,
	               "reactivation returns the old pair before forming a fresh pair without duplicating navigation membership");
	Tick(2000);
	CTR_TEST_CHECK(s_tracker.drivers[0] == &world.drivers[0] && s_tracker.drivers[1] == &world.drivers[1] &&
	               s_tracker.threadBuckets[PLAYER].thread == &world.threads[0] && sdata->navBotList[0].first == &world.drivers[1].botData.item,
	               "expiry restores both role pipelines and the original navigation-list owner");
	Chaos_Activate(CHAOS_EFFECT_BODY_SWAP, 1000);
	s_tracker.drivers[0]->kartState = KS_MASK_GRABBED;
	Tick(1000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_BODY_SWAP) && s_tracker.drivers[0] == &world.drivers[0],
	               "expiry returns ownership immediately even while a kart is being rescued");
	Chaos_Activate(CHAOS_EFFECT_BODY_SWAP, 2000);
	s_tracker.drivers[0]->lapIndex = 2;
	s_tracker.drivers[0]->distanceToFinish_curr = 500;
	Chaos_ReturnBodySwapsNearFinish(&s_tracker, 1200);
	CTR_TEST_CHECK(s_tracker.drivers[0] == &world.drivers[0] && sdata->chaosRuntime.bodySwapPartner[0] == 0 &&
	               (world.drivers[1].actionsFlagSet & ACTION_RACE_FINISHED) == 0,
	               "the pair returns on the final finish-line approach before either kart records a finish");
	Chaos_Activate(CHAOS_EFFECT_BODY_SWAP, 2000);
	Chaos_Reset();
	CTR_TEST_CHECK(s_tracker.drivers[0] == &world.drivers[0] && data.characterIDs[0] == 0 && world.drivers[0].driverID == 0,
	               "race teardown restores original ownership before clearing class and identity state");

	CTR_TEST_SECTION("Body Swap forms disjoint pairs involving humans and skips unavailable controllers");
	SetupBodySwap(&world, 4, 4);
	Chaos_Activate(CHAOS_EFFECT_BODY_SWAP, 1000);
	for (int slot = 0; slot < LOAD_CHARACTER_ID_COUNT; slot++)
	{
		const int partner = sdata->chaosRuntime.bodySwapPartner[slot] - 1;
		if (partner >= 0)
		{
			CTR_TEST_CHECK(partner != slot && sdata->chaosRuntime.bodySwapPartner[partner] == slot + 1,
			               "pair membership is symmetric and a controller cannot possess itself");
			CTR_TEST_CHECK(slot < 4 || partner < 4, "every pair involves a human controller");
		}
		CTR_TEST_CHECK_EQ(s_tracker.drivers[slot]->driverID, slot, "every live kart ID agrees with its controller slot");
	}
	Chaos_Reset();
	SetupBodySwap(&world, 1, 0);
	Chaos_Activate(CHAOS_EFFECT_BODY_SWAP, 1000);
	CTR_TEST_CHECK(s_tracker.drivers[0] == &world.drivers[0] && sdata->chaosRuntime.bodySwapPartner[0] == 0,
	               "a solo event without another kart leaves ownership intact");
	SetupBodySwap(&world, 1, 1);
	world.drivers[1].actionsFlagSet |= ACTION_RACE_FINISHED;
	Chaos_Activate(CHAOS_EFFECT_BODY_SWAP, 1000);
	CTR_TEST_CHECK(sdata->chaosRuntime.bodySwapPartner[0] == 0, "finished targets cannot be possessed");
	SetupBodySwap(&world, 2, 0);
	world.drivers[0].kartState = KS_ENGINE_REVVING;
	world.drivers[1].kartState = KS_WARP_PAD;
	sdata->chaosRuntime.restart[0].active = true;
	sdata->chaosRuntime.restart[1].pending = true;
	sdata->chaosRuntime.identity[1].transformRemainingMS = 100;
	Chaos_Activate(CHAOS_EFFECT_BODY_SWAP, 1000);
	CTR_TEST_CHECK(s_tracker.drivers[0] == &world.drivers[1] && s_tracker.drivers[1] == &world.drivers[0],
	               "scripted states, start holds and pending transformations do not gate controller exchange");
}

struct TeleportWorld
{
	struct BodySwapWorld racers;
	struct Level level;
	struct mesh_info mesh;
	struct BSP bsp[3];
	struct CheckpointNode nodes[13];
};

static void SetupTeleport(struct TeleportWorld *world, int humans, int bots)
{
	memset(world, 0, sizeof(*world));
	SetupBodySwap(&world->racers, humans, bots);
	sdata->chaosSettings.effects[CHAOS_EFFECT_YARDRAT_TRAINING] = true;
	sdata->chaosSettings.enabledCount++;
	world->level.ptr_restart_points = world->nodes;
	world->level.cnt_restart_points = 10;
	world->level.ptr_mesh_info = &world->mesh;
	world->mesh.bspRoot = world->bsp;
	world->mesh.numBspNodes = 1;
	world->bsp[0] = (struct BSP){
		.flag = BSP_NODE_FLAG_LEAF,
		.box = {.min = {-10000, -512, -10000}, .max = {10000, 512, 10000}},
		.data.leaf.numQuads = 1,
	};
	s_tracker.level1 = &world->level;
	for (int i = 0; i < 10; i++)
	{
		world->nodes[i] = (struct CheckpointNode){
			.pos = {i * 1000, 32, 0}, .distToFinish = (10 - i) * 1000,
			.nextIndex_forward = (i + 1) % 10, .nextIndex_backward = (i + 9) % 10,
			.nextIndex_left = 0xff, .nextIndex_right = 0xff,
		};
	}
	for (int i = 0; i < humans + bots; i++)
	{
		struct Driver *driver = &world->racers.drivers[i];
		driver->posCurr = (Vec3){500 * 256, 32 * 256, 0};
		driver->distanceToFinish_curr = 76000;
		driver->checkpoint.currentIndex = 0;
		driver->checkpoint.branchChoiceIndex = 0xff;
		driver->speed = 1000;
		driver->speedApprox = 1000;
		driver->baseSpeed = 900;
		driver->fireSpeed = 100;
		driver->reserves = 2345;
		driver->turbo_outsideTimer = 400;
		driver->heldItemID = HELD_ITEM_MISSILE_3X;
		driver->numHeldItems = 3;
	}
	s_teleportGround = (struct QuadBlock){.checkpointIndex = 1, .quadFlags = QUADBLOCK_FLAG_GROUND};
	s_teleportGroundMissing = false;
	s_teleportProbes = 0;
	s_teleportSavedLaps = 0;
	s_teleportRaceEnds = 0;
	s_allowRaceEnd = true;
	s_projectLapProgress = false;
}

static void SetupProjectedTeleport(struct TeleportWorld *world)
{
	SetupTeleport(world, 1, 0);
	const SVec3 positions[10] = {
		{0, 32, 0}, {800, 32, 0}, {1600, 32, 0}, {2400, 32, 0}, {2400, 32, 800},
		{2400, 32, 1600}, {1600, 32, 1600}, {800, 32, 1600}, {0, 32, 1600}, {0, 32, 800},
	};
	for (int i = 0; i < 10; i++)
	{
		world->nodes[i].pos = positions[i];
		world->nodes[i].distToFinish = (10 - i) * 100;
	}
	world->racers.drivers[0].posCurr = (Vec3){400 * FP8(1), 32 * FP8(1), 0};
	world->racers.drivers[0].distanceToFinish_curr = 7600;
	// Retail projects from the two nodes after the surface's checkpoint.
	s_teleportGround.checkpointIndex = 0;
	s_projectLapProgress = true;
}

static void TestKingCrimson(void)
{
	CTR_TEST_SECTION("King Crimson skips bot paths and the race clock without advancing other race state");
	struct TeleportWorld world;
	SetupTeleport(&world, 2, 2);
	sdata->chaosSettings.effects[CHAOS_EFFECT_KING_CRIMSON] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_ZA_WARUDO] = true;
	sdata->chaosSettings.enabledCount += 2;
	struct NavFrame navigation[2] = {0};
	sdata->NavPath_ptrNavFrameArray[0] = navigation;
	struct Driver *bot = &world.racers.drivers[2];
	bot->botData.aiPhysics.speedLinear = 8192;
	bot->botData.botPath = 0;
	bot->clockReceive = 4567;
	bot->squishTimer = 987;
	world.racers.drivers[3].botData.aiPhysics.speedLinear = 0;
	const struct Driver humans[2] = {world.racers.drivers[0], world.racers.drivers[1]};
	const struct Driver stopped = world.racers.drivers[3];
	const struct ChaosActivation active = {.effect = CHAOS_EFFECT_FLY_ME_TO_THE_MOON, .remainingMS = 5432, .elapsedMS = 876};
	sdata->chaosRuntime.active[0] = active;
	sdata->chaosRuntime.numActive = 1;
	s_tracker.elapsedEventTime = 12000;
	s_tracker.frozenTimeRemaining = 3000;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_KING_CRIMSON, 1000), "King Crimson activates as an instant effect");
	CTR_TEST_CHECK_EQ(s_tracker.elapsedEventTime, 22000, "the race clock skips ten seconds even with frozen relic-clock time");
	CTR_TEST_CHECK_EQ(bot->distanceToFinish_curr, 66000, "bot travel uses the existing speed-to-navigation units");
	CTR_TEST_CHECK(bot->posCurr.x == 1750 * FP8(1) && bot->botData.positionBackup.x == bot->posCurr.x &&
	               bot->botData.botNavFrame == navigation && (bot->botData.botFlags & BOT_FLAG_ESTIMATE_NAV) != 0,
	               "the shared track interpolation places the bot and reanchors navigation at its destination");
	CTR_TEST_CHECK(memcmp(humans, world.racers.drivers, sizeof(humans)) == 0,
	               "both human karts retain every field across the skip");
	CTR_TEST_CHECK(memcmp(&stopped, &world.racers.drivers[3], sizeof(stopped)) == 0,
	               "a stationary bot remains stationary");
	CTR_TEST_CHECK(bot->reserves == 2345 && bot->turbo_outsideTimer == 400 && bot->clockReceive == 4567 &&
	               bot->squishTimer == 987 && bot->heldItemID == HELD_ITEM_MISSILE_3X && s_tracker.frozenTimeRemaining == 3000,
	               "boost fuel, damage, inventory and frozen race-clock duration do not consume skipped time");
	CTR_TEST_CHECK(sdata->chaosRuntime.numActive == 1 && memcmp(&active, sdata->chaosRuntime.active, sizeof(active)) == 0,
	               "instant activation preserves existing Chaos timers and uses no timed slot");
	CTR_TEST_CHECK(Chaos_UsesTimePalette() && !Chaos_IsTimeStopped(), "the short palette treatment does not freeze simulation");
	CTR_TEST_CHECK_EQ(s_clockSounds, 1, "the shared clock cue plays once");
	Tick(200);
	CTR_TEST_CHECK(s_timeStopCrateSounds == 1 && s_timeStopTickSounds == 0,
	               "the delayed crate break reuses the cue scheduler without time-stop ticks");
	Tick(299);
	CTR_TEST_CHECK(Chaos_UsesTimePalette(), "the palette remains enabled until half a second");
	Tick(1);
	CTR_TEST_CHECK(!Chaos_UsesTimePalette(), "the palette expires at half a second independently of the instant banner");
	Chaos_Activate(CHAOS_EFFECT_ZA_WARUDO, 1000);
	Chaos_Activate(CHAOS_EFFECT_KING_CRIMSON, 1000);
	CTR_TEST_CHECK(!Chaos_IsTimeStopped() && s_pausedVoices == 0 && Chaos_UsesTimePalette(),
	               "King Crimson replaces time stop and releases its audio suspension");
	Chaos_Activate(CHAOS_EFFECT_ZA_WARUDO, 1000);
	CTR_TEST_CHECK(sdata->chaosRuntime.numInstantNotices == 0,
	               "Za Warudo replaces the entire King Crimson notice and its pending presentation");
	Tick(1000);
	CTR_TEST_CHECK(!Chaos_UsesTimePalette(), "replacing King Crimson with time stop cancels its unfinished presentation");
	Chaos_Reset();

	CTR_TEST_SECTION("skipped bot finish crossings use retail laps and chronological finish ranks");
	SetupTeleport(&world, 1, 2);
	sdata->chaosSettings.effects[CHAOS_EFFECT_KING_CRIMSON] = true;
	sdata->chaosSettings.enabledCount++;
	sdata->NavPath_ptrNavFrameArray[0] = navigation;
	s_tracker.elapsedEventTime = 12000;
	for (int slot = 1; slot <= 2; slot++)
	{
		bot = &world.racers.drivers[slot];
		bot->botData.botPath = 0;
		bot->botData.aiPhysics.speedLinear = 8192;
		bot->distanceToFinish_curr = slot == 1 ? 8000 : 4000;
		bot->checkpoint.currentIndex = 9;
		bot->lapIndex = 2;
	}
	Chaos_Activate(CHAOS_EFFECT_KING_CRIMSON, 1000);
	CTR_TEST_CHECK(world.racers.drivers[2].driverRank == 0 && world.racers.drivers[1].driverRank == 1 &&
	               sdata->numPlayersFinishedRace == 2,
	               "the earlier finish ranks first even when its slot is processed later");
	CTR_TEST_CHECK(world.racers.drivers[2].timeElapsedInRace == 16000 && world.racers.drivers[1].timeElapsedInRace == 20000,
	               "each bot finish retains its time within the skip");
	CTR_TEST_CHECK(world.racers.drivers[1].lapIndex == 3 && world.racers.drivers[2].lapIndex == 3 &&
	               world.racers.drivers[1].heldItemID == HELD_ITEM_NONE && world.racers.drivers[2].heldItemID == HELD_ITEM_NONE,
	               "retail finish handling completes the final laps and clears finished inventories");
	PlayLevel_UpdateLapStats();
	CTR_TEST_CHECK(sdata->numPlayersFinishedRace == 2 && s_teleportRaceEnds == 0,
	               "ordinary results sorting does not repeat skipped finishes or end a solo race before its human finishes");
	Chaos_Reset();

	CTR_TEST_SECTION("multi-lap skips stay consistent with real bot checkpoint projection");
	SetupTeleport(&world, 1, 1);
	sdata->chaosSettings.effects[CHAOS_EFFECT_KING_CRIMSON] = true;
	sdata->chaosSettings.enabledCount++;
	sdata->NavPath_ptrNavFrameArray[0] = navigation;
	s_tracker.numLaps = 7;
	const SVec3 positions[10] = {
		{0, 32, 0}, {800, 32, 0}, {1600, 32, 0}, {2400, 32, 0}, {2400, 32, 800},
		{2400, 32, 1600}, {1600, 32, 1600}, {800, 32, 1600}, {0, 32, 1600}, {0, 32, 800},
	};
	for (int i = 0; i < 10; i++)
	{
		world.nodes[i].pos = positions[i];
		world.nodes[i].distToFinish = (10 - i) * 100;
	}
	bot = &world.racers.drivers[1];
	bot->distanceToFinish_curr = 7600;
	bot->lapIndex = 0;
	bot->botData.aiPhysics.speedLinear = 24576;
	bot->botData.botPath = 0;
	Chaos_Activate(CHAOS_EFFECT_KING_CRIMSON, 1000);
	CTR_TEST_CHECK(bot->lapIndex == 3 && bot->distanceToFinish_curr == 1600,
	               "a short circuit counts every skipped lap instead of collapsing crossings into a single flag");
	VehLap_UpdateProgress(bot);
	CTR_TEST_CHECK_EQ(bot->distanceToFinish_curr, 1600, "real bot projection agrees with the destination surface checkpoint");
	PlayLevel_UpdateLapStats();
	CTR_TEST_CHECK(bot->lapIndex == 3 && bot->distanceDrivenBackwards == 0,
	               "ordinary lap statistics do not repeat crossings or penalize the skipped forward distance");
	Chaos_Reset();
	const int excluded[] = {BATTLE_MODE, CRYSTAL_CHALLENGE, RELIC_RACE, TIME_TRIAL};
	for (int i = 0; i < (int)len(excluded); i++)
	{
		SetupTeleport(&world, 1, 0);
		sdata->chaosSettings.effects[CHAOS_EFFECT_KING_CRIMSON] = true;
		s_tracker.gameMode1 |= excluded[i];
		CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_KING_CRIMSON, 1000), "events without racing bots reject King Crimson");
	}
	sdata->NavPath_ptrNavFrameArray[0] = NULL;
}

static void TestTeleportProgressProjection(void)
{
	CTR_TEST_SECTION("Teleport geometry agrees with retail progress projection on placement and the following frame");
	struct TeleportWorld world;
	SetupProjectedTeleport(&world);
	struct Driver *driver = &world.racers.drivers[0];
	Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000);
	CTR_TEST_CHECK_EQ(driver->posCurr.x, 1200 * FP8(1), "one tenth of an 8000-unit circuit moves 800 world units");
	PlayLevel_UpdateLapStats();
	CTR_TEST_CHECK_EQ(driver->distanceToFinish_curr, 6800, "projected distance matches the main-route destination");
	CTR_TEST_CHECK(driver->distanceToFinish_curr == 6800 && driver->distanceToFinish_checkpoint == 6800 && driver->lapIndex == 0,
	               "real progress projection agrees with the interpolated destination and anti-cut baseline");
	PlayLevel_UpdateLapStats();
	CTR_TEST_CHECK(driver->distanceToFinish_curr == 6800 && driver->distanceDrivenBackwards == 0 && driver->lapIndex == 0,
	               "the next frame leaves progress stable without a false reverse penalty or lap");

	CTR_TEST_SECTION("a teleported finish crossing remains stable after real progress projection");
	SetupProjectedTeleport(&world);
	driver->checkpoint.currentIndex = 9;
	driver->distanceToFinish_curr = 400;
	s_teleportGround.checkpointIndex = 9;
	Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000);
	PlayLevel_UpdateLapStats();
	CTR_TEST_CHECK_EQ(driver->distanceToFinish_curr, 7600, "projected distance matches the wrapped destination");
	CTR_TEST_CHECK(driver->distanceToFinish_curr == 7600 && driver->lapIndex == 1,
	               "the ground checkpoint projects across the seam and records exactly one lap");
	PlayLevel_UpdateLapStats();
	CTR_TEST_CHECK(driver->distanceToFinish_curr == 7600 && driver->lapIndex == 1 && driver->distanceDrivenBackwards == 0,
	               "a second projected frame does not repeat the skipped finish crossing");

	CTR_TEST_SECTION("selected branch surfaces preserve progress under the real retail projection");
	SetupProjectedTeleport(&world);
	world.level.cnt_restart_points = 13;
	world.nodes[1].nextIndex_left = 10;
	world.nodes[10] = (struct CheckpointNode){
		.pos = {800, 32, 800}, .distToFinish = 800, .nextIndex_forward = 11,
		.nextIndex_backward = 1, .nextIndex_left = 0xff, .nextIndex_right = 0xff,
	};
	world.nodes[11] = (struct CheckpointNode){
		.pos = {1600, 32, 800}, .distToFinish = 700, .nextIndex_forward = 4,
		.nextIndex_backward = 10, .nextIndex_left = 0xff, .nextIndex_right = 0xff,
	};
	world.nodes[12] = (struct CheckpointNode){
		.pos = world.nodes[1].pos, .distToFinish = 900, .nextIndex_forward = 10,
		.nextIndex_backward = 0, .nextIndex_left = 0xff, .nextIndex_right = 0xff,
	};
	driver->distanceToFinish_curr = 6800;
	driver->checkpoint.branchChoiceIndex = 11;
	s_teleportGround.checkpointIndex = 12;
	Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000);
	CTR_TEST_CHECK(driver->posCurr.x == 1200 * FP8(1) && driver->posCurr.z == 800 * FP8(1),
	               "the selected alternate route receives the interpolated destination");
	PlayLevel_UpdateLapStats();
	PlayLevel_UpdateLapStats();
	CTR_TEST_CHECK_EQ(driver->distanceToFinish_curr, 6000, "projected distance matches the alternate-route destination");
	CTR_TEST_CHECK(driver->distanceToFinish_curr == 6000 && driver->distanceToFinish_checkpoint == 6000 &&
	               driver->checkpoint.currentIndex == 12 && driver->checkpoint.branchChoiceIndex == 11 && driver->lapIndex == 0,
	               "the alternate surface, branch selection and progress remain coherent on subsequent frames");
	s_projectLapProgress = false;
}

static void TestTeleport(void)
{
	CTR_TEST_SECTION("Teleport moves each unfinished human forward one tenth of a lap, preserving motion and inventory");
	struct TeleportWorld world;
	SetupTeleport(&world, 2, 1);
	world.racers.drivers[1].axisRotationY = 0x100;
	const struct Driver bot = world.racers.drivers[2];
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000), "Teleport activates as an instant effect");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "Teleport does not occupy a timed slot");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numInstantNotices, 1, "Teleport announces once across both humans");
	for (int i = 0; i < 2; i++)
	{
		const struct Driver *driver = &world.racers.drivers[i];
		CTR_TEST_CHECK(driver->posCurr.x == 1500 * 256 && driver->posCurr.y == 32 * 256 && driver->posCurr.z == 0,
		               "the destination interpolates along the path instead of selecting a whole checkpoint");
		CTR_TEST_CHECK(driver->distanceToFinish_curr == 68000 && driver->distanceToFinish_checkpoint == 68000 &&
		               driver->checkpoint.currentIndex == 1 && driver->lastValid == &s_teleportGround &&
		               driver->underDriver == &s_teleportGround && driver->terrainMeta1 == driver->terrainMeta2,
		               "the current terrain and anti-cut references describe the destination");
		CTR_TEST_CHECK(driver->posPrev.x == driver->posCurr.x && driver->angle == ANG_HALF_PI &&
		               driver->axisRotationX == ANG_HALF_PI && driver->rotCurr.w == 0,
		               "orientation faces forward along the destination segment without sweeping the skipped world");
		CTR_TEST_CHECK(driver->speed == 1000 && driver->speedApprox == 1000 && driver->baseSpeed == 900 &&
		               driver->fireSpeed == 100 && driver->reserves == 2345 && driver->turbo_outsideTimer == 400 &&
		               driver->heldItemID == HELD_ITEM_MISSILE_3X && driver->numHeldItems == 3,
		               "speed, turbo state and held inventory survive repositioning");
		CTR_TEST_CHECK(Chaos_ConsumeCameraBlend(&s_tracker.cameraDC[i]), "each viewport requests a destination camera blend");
		CTR_TEST_CHECK(!Chaos_ConsumeTeleportLapCrossing(driver), "moving within the lap does not invent a finish crossing");
	}
	CTR_TEST_CHECK(world.racers.drivers[1].ySpeed > 0, "airborne vertical motion survives the yaw realignment");
	CTR_TEST_CHECK(memcmp(&bot, &world.racers.drivers[2], sizeof(bot)) == 0, "Teleport leaves bot karts untouched");
	SetupTeleport(&world, 1, 0);
	const struct ChaosActivation fullSlots[CHAOS_MAX_ACTIVATIONS] = {
		{.effect = CHAOS_EFFECT_MIRROR_MIRROR, .remainingMS = 5000},
		{.effect = CHAOS_EFFECT_FLY_ME_TO_THE_MOON, .remainingMS = 5000},
		{.effect = CHAOS_EFFECT_SUNDAY_DRIVER, .remainingMS = 5000},
		{.effect = CHAOS_EFFECT_FAIR_MODE, .remainingMS = 5000},
	};
	memcpy(sdata->chaosRuntime.active, fullSlots, sizeof(fullSlots));
	sdata->chaosRuntime.numActive = CHAOS_MAX_ACTIVATIONS;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000) &&
	               memcmp(sdata->chaosRuntime.active, fullSlots, sizeof(fullSlots)) == 0,
	               "Teleport works at full timed capacity without changing existing activations");

	CTR_TEST_SECTION("Teleport follows the retail branch selection and otherwise takes the main route");
	SetupTeleport(&world, 1, 0);
	world.level.cnt_restart_points = 12;
	world.nodes[1].nextIndex_left = 10;
	world.nodes[10] = (struct CheckpointNode){
		.pos = {1500, 32, 500}, .distToFinish = 8500, .nextIndex_forward = 11,
		.nextIndex_backward = 1, .nextIndex_left = 0xff, .nextIndex_right = 0xff,
	};
	world.nodes[11] = (struct CheckpointNode){
		.pos = {1800, 32, 500}, .distToFinish = 8200, .nextIndex_forward = 2,
		.nextIndex_backward = 10, .nextIndex_left = 0xff, .nextIndex_right = 0xff,
	};
	world.racers.drivers[0].checkpoint.branchChoiceIndex = 11;
	Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000);
	CTR_TEST_CHECK(world.racers.drivers[0].posCurr.x == 1500 * 256 && world.racers.drivers[0].posCurr.z == 500 * 256,
	               "an already selected alternate branch is resolved by the real retail path helper");
	SetupTeleport(&world, 1, 0);
	Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000);
	CTR_TEST_CHECK_EQ(world.racers.drivers[0].posCurr.z, 0, "without a branch selection the main path remains the default");

	CTR_TEST_SECTION("finish crossings are queued for retail lap/results handling and travel with the kart through Body Swap");
	SetupTeleport(&world, 1, 1);
	world.racers.drivers[0].distanceToFinish_curr = 4000;
	world.racers.drivers[0].checkpoint.currentIndex = 9;
	s_teleportGround.checkpointIndex = 0;
	Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000);
	CTR_TEST_CHECK_EQ(world.racers.drivers[0].distanceToFinish_curr, 76000, "forward movement wraps across the finish line");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.teleportForwardCrossings[0], 1, "the crossing survives until lap statistics run");
	const struct ChaosRuntime checkpoint = sdata->chaosRuntime;
	CTR_TEST_CHECK(Chaos_ConsumeTeleportLapCrossing(&world.racers.drivers[0]), "a pending crossing can be consumed before restore");
	sdata->chaosRuntime = checkpoint;
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.teleportForwardCrossings[0], 1, "resident checkpoint restoration restores the pending crossing");
	Chaos_Activate(CHAOS_EFFECT_BODY_SWAP, 1000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.teleportForwardCrossings[1], 1, "an unconsumed crossing stays with the physical kart");
	CTR_TEST_CHECK(Chaos_ConsumeTeleportLapCrossing(&world.racers.drivers[0]) &&
	               !Chaos_ConsumeTeleportLapCrossing(&world.racers.drivers[0]), "lap processing consumes each crossing exactly once");
	Chaos_Reset();
	SetupTeleport(&world, 1, 1);
	Chaos_Activate(CHAOS_EFFECT_BODY_SWAP, 1000);
	s_tracker.drivers[0]->lapIndex = 2;
	s_tracker.drivers[0]->checkpoint.currentIndex = 9;
	s_tracker.drivers[0]->distanceToFinish_curr = 4000;
	Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000);
	Chaos_ReturnBodySwapsNearFinish(&s_tracker, 1200);
	CTR_TEST_CHECK(s_tracker.drivers[0] == &world.racers.drivers[0] && sdata->chaosRuntime.bodySwapPartner[0] == 0 &&
	               sdata->chaosRuntime.teleportForwardCrossings[1] == 1,
	               "a final-lap teleport returns the pair before the possessed kart records its finish");
	Chaos_Reset();
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.teleportForwardCrossings[0] + sdata->chaosRuntime.teleportForwardCrossings[1], 0,
	                  "reset discards pending crossings");

	CTR_TEST_SECTION("Teleport finish crossings use the real lap statistics, lap times and race completion exactly once");
	SetupTeleport(&world, 1, 0);
	s_tracker.gameMode1 |= ARCADE_MODE;
	s_tracker.elapsedEventTime = 10000;
	world.racers.drivers[0].lapTime = 1000;
	world.racers.drivers[0].checkpoint.currentIndex = 9;
	world.racers.drivers[0].distanceToFinish_curr = 4000;
	Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000);
	PlayLevel_UpdateLapStats();
	CTR_TEST_CHECK(world.racers.drivers[0].lapIndex == 1 && world.racers.drivers[0].lapTime == 10000 &&
	               s_teleportSavedLaps == 1 && s_teleportRaceEnds == 0,
	               "a skipped finish crossing records the lap through the retail results path");
	PlayLevel_UpdateLapStats();
	CTR_TEST_CHECK(world.racers.drivers[0].lapIndex == 1 && s_teleportSavedLaps == 1,
	               "the next frame does not count the same teleport as another lap");
	SetupTeleport(&world, 1, 0);
	s_tracker.gameMode1 |= ARCADE_MODE;
	world.racers.drivers[0].checkpoint.currentIndex = 9;
	world.racers.drivers[0].distanceToFinish_curr = 4000;
	world.racers.drivers[0].lapIndex = 2;
	Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000);
	PlayLevel_UpdateLapStats();
	CTR_TEST_CHECK(world.racers.drivers[0].lapIndex == 3 &&
	               (world.racers.drivers[0].actionsFlagSet & ACTION_RACE_FINISHED) != 0 &&
	               sdata->numPlayersFinishedRace == 1 && s_teleportRaceEnds == 1 &&
	               world.racers.drivers[0].heldItemID == HELD_ITEM_NONE,
	               "a final-lap teleport records a normal finish and completes the event");
	SetupTeleport(&world, 1, 0);
	world.racers.drivers[0].checkpoint.currentIndex = 9;
	world.racers.drivers[0].distanceToFinish_curr = 4000;
	world.racers.drivers[0].actionsFlagSet |= ACTION_BEHIND_START_LINE;
	Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000);
	PlayLevel_UpdateLapStats();
	CTR_TEST_CHECK(world.racers.drivers[0].lapIndex == 0 &&
	               (world.racers.drivers[0].actionsFlagSet & ACTION_BEHIND_START_LINE) == 0,
	               "teleporting across the initial starting line begins the lap instead of awarding a completed lap");

	CTR_TEST_SECTION("repositioning ends existing rescues/start holds and missing ground leaves the complete kart unchanged");
	SetupTeleport(&world, 1, 0);
	struct MaskHeadWeapon mask = {.duration = 1000};
	world.racers.drivers[0].kartState = KS_MASK_GRABBED;
	world.racers.drivers[0].KartStates.MaskGrab.maskObj = &mask;
	world.racers.drivers[0].collisionFlags = DRIVER_COLL_FLAG_MASK_GRAB_REQUEST;
	sdata->chaosRuntime.restart[0].pending = true;
	Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000);
	CTR_TEST_CHECK(world.racers.drivers[0].kartState == KS_NORMAL && mask.duration == 0 &&
	               world.racers.drivers[0].collisionFlags == 0 && !sdata->chaosRuntime.restart[0].pending,
	               "the destination releases rescue state and prevents a deferred restart from moving the kart again");
	SetupTeleport(&world, 1, 0);
	s_teleportGroundMissing = true;
	const struct Driver original = world.racers.drivers[0];
	Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000);
	CTR_TEST_CHECK(memcmp(&original, &world.racers.drivers[0], sizeof(original)) == 0,
	               "failure to find ground does not partially change inventory, motion or progression");
	for (int mode = 0; mode < 2; mode++)
	{
		SetupTeleport(&world, 1, 0);
		s_tracker.gameMode1 |= mode == 0 ? BATTLE_MODE : CRYSTAL_CHALLENGE;
		CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000), "non-lap events reject Teleport before activation");
		CTR_TEST_CHECK_EQ(s_teleportProbes, 0, "unavailable events never search world geometry");
	}
	SetupTeleport(&world, 1, 0);
	s_tracker.gameMode1 |= PAUSE_1;
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000) && s_teleportProbes == 0,
	               "pause prevents instant movement");
	SetupTeleport(&world, 1, 0);
	world.racers.drivers[0].actionsFlagSet |= ACTION_RACE_FINISHED;
	Chaos_Activate(CHAOS_EFFECT_YARDRAT_TRAINING, 1000);
	CTR_TEST_CHECK_EQ(s_teleportProbes, 0, "finished humans are not repositioned");
}

static void EnableDefectiveTeleport(void)
{
	sdata->chaosSettings.effects[CHAOS_EFFECT_DEFECTIVE_TELEPORT] = true;
	sdata->chaosSettings.enabledCount++;
}

static void TestDefectiveTeleport(void)
{
	CTR_TEST_SECTION("Defective Teleport samples a rear 3D hemisphere within one fifth of the lap without grounding");
	struct TeleportWorld world;
	bool above = false;
	bool below = false;
	bool left = false;
	bool right = false;
	for (int sample = 0; sample < 128; sample++)
	{
		SetupProjectedTeleport(&world);
		EnableDefectiveTeleport();
		sdata->chaosRng = (sample + 1) * 2654435761u;
		struct Driver *driver = &world.racers.drivers[0];
		driver->angle = sample * ANG_TWO_PI / 128;
		const Vec3 origin = driver->posCurr;
		const int yaw = driver->angle;
		const int sounds = s_identitySounds;
		s_teleportGroundMissing = true;
		CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_DEFECTIVE_TELEPORT, 1000), "Defective Teleport activates without requiring ground");
		const s64 x = driver->posCurr.x - origin.x;
		const s64 y = driver->posCurr.y - origin.y;
		const s64 z = driver->posCurr.z - origin.z;
		const s64 radius = 1600 * FP8(1);
		CTR_TEST_CHECK(x * x + y * y + z * z <= radius * radius,
		               "the full 3D displacement stays within twenty percent of the circuit length");
		CTR_TEST_CHECK(llabs(y) <= 256 * FP8(1), "vertical displacement stays within the fixed height cap rather than scaling with lap length");
		CTR_TEST_CHECK(x * MATH_Sin(yaw) + z * MATH_Sin(yaw + ANG_HALF_PI) <= FP_ONE,
		               "the destination is behind the kart at every heading, including diagonal headings");
		const s64 lateral = x * MATH_Sin(yaw + ANG_HALF_PI) - z * MATH_Sin(yaw);
		above |= y > 0;
		below |= y < 0;
		left |= lateral < 0;
		right |= lateral > 0;
		CTR_TEST_CHECK(driver->angle == yaw && driver->posPrev.x == driver->posCurr.x &&
		               driver->posPrev.y == driver->posCurr.y && driver->posPrev.z == driver->posCurr.z,
		               "free teleport keeps the heading and never sweeps through the skipped world");
		CTR_TEST_CHECK(driver->speed == 1000 && driver->baseSpeed == 900 && driver->fireSpeed == 100 &&
		               driver->reserves == 2345 && driver->turbo_outsideTimer == 400 &&
		               driver->heldItemID == HELD_ITEM_MISSILE_3X && driver->numHeldItems == 3,
		               "free placement preserves canonical motion, turbo and inventory before normal physics or rescue");
		CTR_TEST_CHECK(s_teleportProbes == 0 && s_identitySounds == sounds + 1 &&
		               sdata->chaosRuntime.numActive == 0 && sdata->chaosRuntime.numInstantNotices == 1 &&
		               Chaos_ConsumeCameraBlend(&s_tracker.cameraDC[0]),
		               "an instant free teleport plays invisibility audio and realigns the camera without a terrain probe or timed slot");
	}
	CTR_TEST_CHECK(above && below && left && right, "random destinations cover both vertical and lateral directions");
	SetupProjectedTeleport(&world);
	EnableDefectiveTeleport();
	world.bsp[0].box.min.x = INT16_MIN;
	world.bsp[0].box.min.z = INT16_MIN;
	world.bsp[0].box.max.x = INT16_MAX;
	world.bsp[0].box.max.z = INT16_MAX;
	world.racers.drivers[0].posCurr = (Vec3){INT16_MIN * FP8(1), 0, INT16_MIN * FP8(1)};
	world.racers.drivers[0].angle = ANG_HALF_PI / 2;
	Chaos_Activate(CHAOS_EFFECT_DEFECTIVE_TELEPORT, 1000);
	CTR_TEST_CHECK(world.racers.drivers[0].posCurr.x == INT16_MIN * FP8(1) &&
	               world.racers.drivers[0].posCurr.z == INT16_MIN * FP8(1) &&
	               abs(world.racers.drivers[0].posCurr.y) <= 256 * FP8(1),
	               "an outward rear hemisphere at the world corner falls back vertically without an unbounded sample loop");

	CTR_TEST_SECTION("Defective Teleport stays in the level footprint and near geometry, including unsafe water leaves");
	SetupProjectedTeleport(&world);
	EnableDefectiveTeleport();
	world.mesh.numBspNodes = 3;
	world.bsp[0] = (struct BSP){.box = {.min = {-2000, -128, -2000}, .max = {800, 64, 800}}};
	world.bsp[1] = (struct BSP){
		.flag = BSP_NODE_FLAG_LEAF,
		.box = {.min = {0, 0, -400}, .max = {800, 64, 400}},
		.data.leaf.numQuads = 1,
	};
	world.bsp[2] = (struct BSP){
		.flag = BSP_NODE_FLAG_LEAF | BSP_LEAF_FLAG_WATER,
		.box = {.min = {-1600, -128, -1600}, .max = {-800, 0, -800}},
		.data.leaf.numQuads = 1,
	};
	const Vec3 water = {-1200 * FP8(1), -64 * FP8(1), -1200 * FP8(1)};
	const Vec3 empty = {-1200 * FP8(1), 32 * FP8(1), 400 * FP8(1)};
	const Vec3 outside = {900 * FP8(1), 32 * FP8(1), 0};
	const Vec3 high = {400 * FP8(1), 1000 * FP8(1), 0};
	CTR_TEST_CHECK(Chaos_IsTeleportNearGeometry(&world.mesh, &water), "water geometry remains a valid unsafe destination");
	CTR_TEST_CHECK(!Chaos_IsTeleportNearGeometry(&world.mesh, &empty), "an empty gap inside the root bounds is not treated as rendered geometry");
	CTR_TEST_CHECK(!Chaos_IsTeleportNearGeometry(&world.mesh, &outside), "the leaf margin cannot extend a destination beyond the level footprint");
	CTR_TEST_CHECK(!Chaos_IsTeleportNearGeometry(&world.mesh, &high), "destinations high above local geometry are rejected");
	for (int sample = 0; sample < 128; sample++)
	{
		struct Driver *kart = &world.racers.drivers[0];
		kart->posCurr = (Vec3){400 * FP8(1), 32 * FP8(1), 0};
		kart->angle = sample * ANG_TWO_PI / 128;
		sdata->chaosRng = (sample + 1) * 2654435761u;
		Chaos_Activate(CHAOS_EFFECT_DEFECTIVE_TELEPORT, 1000);
		const int x = kart->posCurr.x >> 8;
		const int y = kart->posCurr.y >> 8;
		const int z = kart->posCurr.z >> 8;
		CTR_TEST_CHECK(x >= -2000 && x <= 800 && z >= -2000 && z <= 800 &&
		               ((x >= -256 && z >= -656 && z <= 656 && y >= -256 && y <= 320) ||
		                (x >= -1856 && x <= -544 && z >= -1856 && z <= -544 && y >= -384 && y <= 256)),
		               "sampled destinations and fallbacks remain close to actual leaf geometry within the level footprint");
	}
	SetupProjectedTeleport(&world);
	EnableDefectiveTeleport();
	world.bsp[0].box.min.x = 10000;
	world.bsp[0].box.max.x = 11000;
	const struct Driver outsideGeometry = world.racers.drivers[0];
	Chaos_Activate(CHAOS_EFFECT_DEFECTIVE_TELEPORT, 1000);
	CTR_TEST_CHECK(memcmp(&outsideGeometry, &world.racers.drivers[0], sizeof(outsideGeometry)) == 0,
	               "an invalid fallback cannot move the kart when no destination is close to level geometry");

	CTR_TEST_SECTION("free teleport retains the rescue anchor and uses ordinary retail progress projection");
	SetupProjectedTeleport(&world);
	EnableDefectiveTeleport();
	struct Driver *driver = &world.racers.drivers[0];
	struct QuadBlock rescueGround = {.checkpointIndex = 9};
	driver->lastValid = &rescueGround;
	driver->underDriver = &rescueGround;
	driver->currBlockTouching = &rescueGround;
	driver->actionsFlagSet |= ACTION_TOUCH_GROUND;
	driver->collisionFlags = DRIVER_COLL_FLAG_GROUNDED;
	Chaos_Activate(CHAOS_EFFECT_DEFECTIVE_TELEPORT, 1000);
	CTR_TEST_CHECK(driver->lastValid == &rescueGround && driver->underDriver == NULL && driver->currBlockTouching == NULL &&
	               (driver->actionsFlagSet & ACTION_TOUCH_GROUND) == 0 && (driver->actionsFlagSet & ACTION_AIRBORNE) != 0 &&
	               driver->collisionFlags == 0 && driver->terrainMeta1 == VehAfterColl_GetTerrain(TERRAIN_NONE),
	               "stale contacts are discarded while lastValid remains usable by the normal mask save");
	driver->lastValid = &s_teleportGround;
	driver->posCurr = (Vec3){1200 * FP8(1), 32 * FP8(1), 0};
	PlayLevel_UpdateLapStats();
	CTR_TEST_CHECK(driver->distanceToFinish_curr == 6800 && driver->lapIndex == 0,
	               "the real retail projection runs normally even while the teleported kart is airborne");

	CTR_TEST_SECTION("free teleports restore deterministically and release existing rescue holds");
	SetupProjectedTeleport(&world);
	EnableDefectiveTeleport();
	const struct Driver original = *driver;
	const u32 rng = sdata->chaosRng;
	Chaos_Activate(CHAOS_EFFECT_DEFECTIVE_TELEPORT, 1000);
	const Vec3 destination = driver->posCurr;
	const u32 nextRng = sdata->chaosRng;
	*driver = original;
	sdata->chaosRng = rng;
	Chaos_Activate(CHAOS_EFFECT_DEFECTIVE_TELEPORT, 1000);
	CTR_TEST_CHECK(driver->posCurr.x == destination.x && driver->posCurr.y == destination.y && driver->posCurr.z == destination.z &&
	               sdata->chaosRng == nextRng, "restoring the Chaos RNG and original kart reproduces all three destination coordinates");
	SetupProjectedTeleport(&world);
	EnableDefectiveTeleport();
	struct MaskHeadWeapon mask = {.duration = 1000};
	driver->kartState = KS_MASK_GRABBED;
	driver->KartStates.MaskGrab.maskObj = &mask;
	driver->lastValid = &rescueGround;
	sdata->chaosRuntime.restart[0].pending = true;
	Chaos_Activate(CHAOS_EFFECT_DEFECTIVE_TELEPORT, 1000);
	CTR_TEST_CHECK(driver->kartState == KS_NORMAL && mask.duration == 0 && driver->lastValid == &rescueGround &&
	               !sdata->chaosRuntime.restart[0].pending,
	               "free teleport releases an existing rescue or deferred start hold while retaining its safe respawn anchor");

	CTR_TEST_SECTION("free teleport moves both humans independently and leaves bots and finished humans untouched");
	SetupTeleport(&world, 1, 1);
	EnableDefectiveTeleport();
	const struct Driver bot = world.racers.drivers[1];
	Chaos_Activate(CHAOS_EFFECT_DEFECTIVE_TELEPORT, 1000);
	CTR_TEST_CHECK(memcmp(&bot, &world.racers.drivers[1], sizeof(bot)) == 0, "free teleport never moves a bot");
	SetupTeleport(&world, 2, 0);
	EnableDefectiveTeleport();
	Chaos_Activate(CHAOS_EFFECT_DEFECTIVE_TELEPORT, 1000);
	CTR_TEST_CHECK(memcmp(&world.racers.drivers[0].posCurr, &world.racers.drivers[1].posCurr, sizeof(Vec3)) != 0,
	               "both humans receive independent random destinations");
	for (int mode = 0; mode < 3; mode++)
	{
		SetupTeleport(&world, 1, 0);
		EnableDefectiveTeleport();
		s_tracker.gameMode1 |= mode == 0 ? BATTLE_MODE : mode == 1 ? CRYSTAL_CHALLENGE : PAUSE_1;
		const u32 beforeRng = sdata->chaosRng;
		CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_DEFECTIVE_TELEPORT, 1000) && sdata->chaosRng == beforeRng,
		               "non-lap events and pause reject Defective Teleport without consuming destination RNG");
	}
	SetupTeleport(&world, 1, 0);
	EnableDefectiveTeleport();
	world.racers.drivers[0].actionsFlagSet |= ACTION_RACE_FINISHED;
	const u32 beforeRng = sdata->chaosRng;
	Chaos_Activate(CHAOS_EFFECT_DEFECTIVE_TELEPORT, 1000);
	CTR_TEST_CHECK(sdata->chaosRng == beforeRng,
	               "finished humans do not receive a random destination");
}

static void TestTimeStopPotionContacts(void)
{
	CTR_TEST_SECTION("deferred red and green potions use current contact positions at resumption");
	enum { TARGET_NEAR, TARGET_AT_RADIUS, TARGET_ABOVE, DIFFERENT_TARGET_NEAR, ORIGINAL_TARGET_DEAD, NUM_SCENARIOS };
	const int models[] = {STATIC_BEAKER_RED, STATIC_BEAKER_GREEN};
	for (int color = 0; color < (int)len(models); color++)
	{
		for (int scenario = 0; scenario < NUM_SCENARIOS; scenario++)
		{
			SetupRace(CHAOS_TRIGGER_EXTERNAL);
			sdata->chaosSettings.effects[CHAOS_EFFECT_ZA_WARUDO] = true;
			sdata->chaosSettings.enabledCount++;
			struct Instance karts[2] = {0};
			struct Thread drivers[2] = {0};
			for (int i = 0; i < 2; i++)
			{
				drivers[i] = (struct Thread){.modelIndex = DYNAMIC_PLAYER, .object = s_tracker.drivers[i], .inst = &karts[i]};
				karts[i].thread = &drivers[i];
				s_tracker.drivers[i]->instSelf = &karts[i];
			}
			karts[1].matrix.t[0] = 1000;
			struct Model model = {.id = models[color]};
			struct MineWeapon mine = {.instParent = &karts[0]};
			struct Instance potion = {.model = &model};
			struct Thread thread = {.flags = MINE, .modelIndex = models[color], .inst = &potion, .object = &mine};
			potion.thread = &thread;
			Chaos_Activate(CHAOS_EFFECT_ZA_WARUDO, 1000);
			CTR_TEST_CHECK(Chaos_DeferImpact(CHAOS_IMPACT_MINE_DRIVER, &thread, &drivers[0], NULL), "potion contact is retained while time is stopped");

			// Retail potion contact uses a 3D radius of 80 units (0x1900 squared).
			karts[0].matrix.t[0] = scenario == TARGET_NEAR ? 79 : 80;
			if (scenario == TARGET_ABOVE)
			{
				karts[0].matrix.t[0] = 0;
				karts[0].matrix.t[1] = 80;
			}
			if (scenario == DIFFERENT_TARGET_NEAR)
			{
				karts[1].matrix.t[0] = 0;
			}
			if (scenario == ORIGINAL_TARGET_DEAD)
			{
				drivers[0].flags |= THREAD_FLAG_DEAD;
			}
			const int shattersBefore = s_deferredPotionShatters;
			s_lastMineDriver = NULL;
			Tick(1000);
			Chaos_ResolveDeferredImpacts();
			struct Thread *expected = NULL;
			if (scenario == TARGET_NEAR || scenario == DIFFERENT_TARGET_NEAR)
			{
				expected = &drivers[scenario == TARGET_NEAR ? 0 : 1];
			}
			CTR_TEST_CHECK(s_lastMineDriver == expected, "only a kart currently inside the potion radius receives its ordinary contact effect");
			CTR_TEST_CHECK_EQ(s_deferredPotionShatters, shattersBefore + (expected == NULL), "a triggered potion still shatters when its former toucher is out of range or gone");
			CTR_TEST_CHECK((thread.flags & THREAD_FLAG_DEAD) != 0, "every triggered potion resolves once instead of remaining as a live mine");
			CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numDeferredImpacts, 0, "resumption releases the original potion contact");
			Chaos_Reset();
		}
	}
}

static void TestTimeStop(void)
{
	CTR_TEST_SECTION("Za Warudo caps its own clock and suspends other Chaos timers without catch-up");
	SetupRace(CHAOS_TRIGGER_INTERVAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_ZA_WARUDO] = true;
	sdata->chaosSettings.enabledCount++;
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 30000);
	Tick(32);
	const struct ChaosActivation gravity = sdata->chaosRuntime.active[0];
	const int interval = sdata->chaosRuntime.intervalRemainingMS;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_ZA_WARUDO, 30000), "time stop activates alongside an existing modifier");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[1].remainingMS, 10000, "time stop is capped at ten seconds");
	CTR_TEST_CHECK(Chaos_IsTimeStopped(), "time stop freezes the world immediately on activation");
	CTR_TEST_CHECK_EQ(s_clockSounds, 1, "the clock cue plays on activation");
	Tick(100);
	CTR_TEST_CHECK_EQ(s_timeStopCrateSounds, 0, "the breaking crate cue follows rather than overlaps the clock cue");
	CTR_TEST_CHECK_EQ(s_timeStopTickSounds, 0, "time-crate ticking waits for the initial clock cue");
	Tick(100);
	CTR_TEST_CHECK_EQ(s_timeStopCrateSounds, 1, "the second cue plays once at its delayed threshold");
	CTR_TEST_CHECK_EQ(s_timeStopTickSounds, 1, "time-crate ticking begins after the clock cue");
	CTR_TEST_CHECK_EQ(HowlSfx_Distortion(s_timeStopTickFlags), HOWL_SFX_DISTORTION_NONE, "the tick plays at its normal pitch");
	CTR_TEST_CHECK_EQ(s_timeStopTickChannel.timeLeft, 10, "normal-speed playback retains the sample's ordinary lifetime");
	CTR_TEST_CHECK(!Chaos_ShouldFreezeAudioChannel(&s_timeStopTickChannel), "the real world-audio filter exempts the time-stop tick channel");
	Tick(191);
	CTR_TEST_CHECK_EQ(s_timeStopTickSounds, 1, "ticking does not repeat before its normal cadence");
	Tick(1);
	CTR_TEST_CHECK_EQ(s_timeStopTickSounds, 2, "the next tick follows after 192 milliseconds");
	for (int i = 0; i < 4; i++)
	{
		Tick(191);
		CTR_TEST_CHECK_EQ(s_timeStopTickSounds, i + 2, "each remaining tick waits for the same interval");
		Tick(1);
		CTR_TEST_CHECK_EQ(s_timeStopTickSounds, i + 3, "the next tick plays once at its scheduled time");
		CTR_TEST_CHECK_EQ(HowlSfx_Distortion(s_timeStopTickFlags), HOWL_SFX_DISTORTION_NONE, "all six ticks retain normal pitch");
		CTR_TEST_CHECK_EQ(s_timeStopTickChannel.timeLeft, 10, "each tick retains its ordinary sample lifetime");
	}
	Tick(8640);
	CTR_TEST_CHECK_EQ(s_timeStopTickSounds, 6, "ticking stops after six sounds throughout the rest of time stop");
	CTR_TEST_CHECK(memcmp(&gravity, &sdata->chaosRuntime.active[0], sizeof(gravity)) == 0,
	               "other activation duration, phase and notice clocks stay unchanged");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.intervalRemainingMS, interval, "the automatic trigger clock stays frozen");
	s_tracker.gameMode1 |= PAUSE_1;
	Tick(5000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[1].remainingMS, 200, "game pause also suspends time-stop expiry");
	s_tracker.gameMode1 &= ~PAUSE_1;
	Tick(200);
	CTR_TEST_CHECK(!Chaos_IsTimeStopped(), "time stop ends on its own clock");
	CTR_TEST_CHECK_EQ(s_timeStopCrateSounds, 1, "expiry does not repeat the delayed cue");
	Tick(32);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].remainingMS, gravity.remainingMS - 32,
	                  "other effects resume by one ordinary frame without accumulated catch-up");

	CTR_TEST_SECTION("time stop freezes bots while preserving human driving and weapons");
	Chaos_Activate(CHAOS_EFFECT_ZA_WARUDO, 1000);
	struct Driver human = {.driverID = 0};
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT};
	struct Thread humanThread = {.modelIndex = DYNAMIC_PLAYER, .object = &human, .flags = PLAYER};
	struct Thread botThread = {.modelIndex = DYNAMIC_ROBOT_CAR, .object = &bot, .flags = ROBOT};
	struct Thread weapon = {.modelIndex = DYNAMIC_BOMB, .flags = TRACKING, .parentThread = &humanThread};
	struct Thread camera = {.flags = CAMERA};
	struct Thread crate = {.flags = STATIC};
	struct MaskHeadWeapon maskObject = {0};
	struct Thread mask = {.modelIndex = STATIC_AKUAKU, .flags = OTHER, .parentThread = &humanThread, .object = &maskObject};
	CTR_TEST_CHECK(!Chaos_ShouldFreezeThread(&humanThread) && !Chaos_ShouldFreezeThread(&camera), "humans and cameras keep updating");
	CTR_TEST_CHECK(Chaos_ShouldFreezeThread(&botThread) && !Chaos_ShouldFreezeThread(&weapon), "bots freeze while newly created human weapons keep moving");
	CTR_TEST_CHECK(Chaos_ShouldFreezeThread(&crate) && (crate.flags & THREAD_FLAG_DISABLE_COLLISION) == 0,
	               "crate animation freezes without disabling collection or collisions");
	CTR_TEST_CHECK(!Chaos_ShouldFreezeThread(&mask), "a human weapon mask keeps following and orbiting its kart");
	human.kartState = KS_MASK_GRABBED;
	human.KartStates.MaskGrab.maskObj = &maskObject;
	CTR_TEST_CHECK(!Chaos_ShouldFreezeThread(&mask), "a human mask rescue can complete during time stop");
	mask.parentThread = &botThread;
	CTR_TEST_CHECK(Chaos_ShouldFreezeThread(&mask), "a bot mask remains frozen");
	struct Instance humanInstance = {.thread = &humanThread};
	struct Instance botInstance = {.thread = &botThread};
	human.instSelf = &humanInstance;
	bot.instSelf = &botInstance;
	struct Turbo turbo = {.driver = &human};
	struct Thread fire = {.modelIndex = STATIC_TURBO_EFFECT, .flags = TURBO, .object = &turbo, .funcThTick = VehTurbo_ThTick};
	CTR_TEST_CHECK(!Chaos_ShouldFreezeThread(&fire), "human boost flames keep following and animating with the kart");
	turbo.driver = &bot;
	CTR_TEST_CHECK(Chaos_ShouldFreezeThread(&fire), "bot boost flames remain frozen");
	struct Thread scenery = {.modelIndex = STATIC_TURBO_EFFECT, .flags = STATIC, .object = (void *)1};
	CTR_TEST_CHECK(Chaos_ShouldFreezeThread(&scenery), "static scenery sharing the flame model is never interpreted as a Turbo object");
	s_tracker.drivers[0] = &human;
	struct Particle exhaust = {.owner.driverInst = &humanInstance, .driverID = -1};
	struct Particle botExhaust = {.owner.driverInst = &botInstance, .driverID = 0};
	struct Particle worldParticle = {.driverID = 0};
	CTR_TEST_CHECK(!Chaos_ShouldFreezeParticle(&exhaust), "human smoke advances even when visible in every viewport");
	CTR_TEST_CHECK(Chaos_ShouldFreezeParticle(&botExhaust) && Chaos_ShouldFreezeParticle(&worldParticle),
	               "bot and world particles stay frozen regardless of their camera filter");
	struct Particle weaponParticle = {.flagsAxis = PARTICLE_AXIS_FLAG_HUMAN_EFFECT};
	CTR_TEST_CHECK(!Chaos_ShouldFreezeParticle(&weaponParticle), "human weapon particles keep advancing without borrowing a kart owner pointer");

	CTR_TEST_SECTION("human weapons retain ownership and postpone external impacts until time resumes");
	humanThread.inst = &humanInstance;
	botThread.inst = &botInstance;
	struct TrackerWeapon trackerWeapon = {.instParent = &humanInstance};
	struct Instance weaponInstance = {.thread = &weapon};
	weapon.inst = &weaponInstance;
	weapon.object = &trackerWeapon;
	weapon.parentThread = NULL;
	CTR_TEST_CHECK(!Chaos_ShouldFreezeThread(&weapon), "weapons use their real owner even when they have no thread parent");
	trackerWeapon.instParent = (struct Instance *)(uintptr_t)0x0004fe6c;
	CTR_TEST_CHECK(!Chaos_IsHumanOwnedThread(&weapon), "a recycled missile owner is compared against live karts without dereferencing it");
	trackerWeapon.instParent = NULL;
	CTR_TEST_CHECK(!Chaos_IsHumanOwnedThread(&weapon), "a weapon without an initialized owner has no human exemption");
	trackerWeapon.instParent = &humanInstance;
	struct MineWeapon levelMineObject = {.instParent = &humanInstance};
	struct InstDef levelMineDefinition = {0};
	struct Thread levelMine = {.flags = MINE, .object = &levelMineObject};
	struct Instance levelMineInstance = {.thread = &levelMine, .instDef = &levelMineDefinition};
	levelMine.inst = &levelMineInstance;
	CTR_TEST_CHECK(Chaos_ShouldFreezeThread(&levelMine), "a level mine's retail driver pointer never grants human ownership");
	levelMineInstance.instDef = NULL;
	levelMineObject.instParent = (struct Instance *)(uintptr_t)0x0004fe6c;
	CTR_TEST_CHECK(!Chaos_IsHumanOwnedThread(&levelMine), "a recycled mine owner is never dereferenced during audio attribution");
	struct Model missileModel = {.id = DYNAMIC_ROCKET};
	struct TrackerWeapon enemyWeaponObject = {.instParent = &botInstance};
	struct Thread enemyWeapon = {.flags = TRACKING, .object = &enemyWeaponObject, .modelIndex = DYNAMIC_ROCKET};
	struct Instance enemyWeaponInstance = {.thread = &enemyWeapon, .model = &missileModel};
	enemyWeapon.inst = &enemyWeaponInstance;
	s_tracker.drivers[2] = &bot;
	s_tracker.drivers[1] = NULL;
	human.kartState = KS_NORMAL;
	humanInstance.matrix.t[0] = 1000;
	CTR_TEST_CHECK(RB_Hazard_CollideWithDrivers(&enemyWeaponInstance, 0, 0x2400, NULL) == NULL,
	               "collision-only checks never make a frozen weapon hit a frozen bot");
	humanInstance.matrix.t[0] = 0;
	CTR_TEST_CHECK(RB_Hazard_CollideWithDrivers(&enemyWeaponInstance, 0, 0x2400, NULL) == &humanInstance,
	               "a human can still collide with a motionless enemy missile");
	s_deferredCollisions = 0;
	CTR_TEST_CHECK(Chaos_DeferImpact(CHAOS_IMPACT_MOVING_DRIVER, &weapon, &botThread, NULL), "an impact against a frozen kart is retained");
	CTR_TEST_CHECK(Chaos_DeferImpact(CHAOS_IMPACT_MOVING_DRIVER, &weapon, &botThread, NULL), "repeated detection retains the same pending impact");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numDeferredImpacts, 1, "one contact produces one queued operation");
	CTR_TEST_CHECK(Chaos_ShouldFreezeThread(&weapon), "a weapon that has reached its target waits at the impact point");
	CTR_TEST_CHECK(Chaos_HasDeferredImpact(&weapon) && Chaos_HasDeferredImpact(&botThread), "pending collision participants cannot be recycled");
	CTR_TEST_CHECK(!Chaos_DeferImpact(CHAOS_IMPACT_MOVING_DRIVER, &weapon, &humanThread, NULL), "impacts against humans remain immediate");
	Chaos_ResolveDeferredImpacts();
	CTR_TEST_CHECK_EQ(s_deferredCollisions, 0, "frozen time never dispatches a queued collision");
	Tick(1000);
	Chaos_ResolveDeferredImpacts();
	CTR_TEST_CHECK_EQ(s_deferredCollisions, 1, "time resumption resolves the retained collision exactly once");
	Chaos_ResolveDeferredImpacts();
	CTR_TEST_CHECK_EQ(s_deferredCollisions, 1, "later frames cannot replay an old collision");
	CTR_TEST_CHECK(!Chaos_HasDeferredImpact(&weapon), "resolving the queue releases its participant references");

	Chaos_Activate(CHAOS_EFFECT_ZA_WARUDO, 1000);
	s_tracker.drivers[2] = &bot;
	CTR_TEST_SECTION("direct damage resumes through the ordinary bot state machine");
	Chaos_DeferImpact(CHAOS_IMPACT_MOVING_DRIVER, &weapon, &botThread, NULL);
	weapon.flags |= THREAD_FLAG_DEAD;
	bot.kartState = KS_NORMAL;
	RB_Hazard_HurtDriver(&bot, 2, &human, 3);
	CTR_TEST_CHECK_EQ(bot.kartState, KS_NORMAL, "human weapon damage leaves a frozen bot's state unchanged");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numDeferredImpacts, 2, "direct weapon damage also retains an operation");
	Tick(1000);
	Chaos_ResolveDeferredImpacts();
	CTR_TEST_CHECK_EQ(s_deferredCollisions, 1, "a waiting projectile destroyed by another impact cannot explode twice on resumption");
	CTR_TEST_CHECK_EQ(bot.kartState, KS_BLASTED, "the retained damage reaches the ordinary bot state machine on resumption");

	CTR_TEST_SECTION("Nitro and potion contact waits until time stop expires, regardless of ownership");
	Chaos_Activate(CHAOS_EFFECT_ZA_WARUDO, 1000);
	const int mineModels[] = {PU_EXPLOSIVE_CRATE, STATIC_BEAKER_RED, STATIC_BEAKER_GREEN};
	struct Thread mines[3] = {0};
	struct Instance mineInstances[3] = {0};
	struct MineWeapon mineObjects[3] = {0};
	struct Model mineModelData[3] = {0};
	const int collisionsBeforeMines = s_deferredCollisions;
	const int nitroBlastsBefore = s_deferredNitroBlasts;
	for (int i = 0; i < (int)len(mines); i++)
	{
		mines[i] = (struct Thread){.flags = MINE, .modelIndex = mineModels[i], .inst = &mineInstances[i], .object = &mineObjects[i]};
		mineInstances[i].thread = &mines[i];
		mineModelData[i].id = mineModels[i];
		mineInstances[i].model = &mineModelData[i];
		mineInstances[i].instDef = &levelMineDefinition;
		mineObjects[i].instParent = &botInstance;
		CTR_TEST_CHECK(Chaos_DeferImpact(CHAOS_IMPACT_MINE_DRIVER, &mines[i], &humanThread, NULL),
		               "touching a world Nitro or potion records the human contact without detonating it");
		mineInstances[i].instDef = NULL;
		mineObjects[i].instParent = &humanInstance;
		CTR_TEST_CHECK(Chaos_DeferImpact(CHAOS_IMPACT_MINE_DRIVER, &mines[i], &humanThread, NULL),
		               "human-owned Nitro and potions also defer contact with another human");
		CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numDeferredImpacts, i + 1, "repeated mine contact retains one operation per object");
		CTR_TEST_CHECK(Chaos_ShouldFreezeThread(&mines[i]), "a touched mine waits without advancing or being destroyed");
	}
	Tick(999);
	Chaos_ResolveDeferredImpacts();
	CTR_TEST_CHECK_EQ(s_deferredCollisions, collisionsBeforeMines, "Nitro and potion contacts stay pending throughout the timer");
	Tick(1);
	Chaos_ResolveDeferredImpacts();
	CTR_TEST_CHECK_EQ(s_deferredCollisions, collisionsBeforeMines + 3, "all retained Nitro and potion contacts resolve when the timer expires");
	CTR_TEST_CHECK_EQ(s_deferredNitroBlasts, nitroBlastsBefore + 1, "triggered Nitro uses its area explosion instead of replaying direct damage to the former toucher");
	CTR_TEST_CHECK(!Chaos_DeferImpact(CHAOS_IMPACT_MINE_DRIVER, &mines[0], &humanThread, NULL), "ordinary Nitro contact remains immediate outside time stop");

	Chaos_Activate(CHAOS_EFFECT_ZA_WARUDO, 1000);
	humanThread.driverHitRadius = THREAD_DRIVER_HIT_RADIUS;
	CTR_TEST_SECTION("karts are mutually collision-free during time stop");
	botThread.driverHitRadius = THREAD_DRIVER_HIT_RADIUS;
	struct DriverCollisionSearch contact = {.bucket = {.th = &botThread, .bestDistSq = 900, .dist = {-30, 0, 0}}};
	Vec3 incoming = {1000, 0, 0};
	const struct Driver frozenBot = bot;
	VehPhysCrash_AnyTwoCars(&humanThread, &contact, &incoming);
	CTR_TEST_CHECK_EQ(incoming.x, 1000, "a frozen bot cannot push back or block a human kart");
	CTR_TEST_CHECK(memcmp(&bot, &frozenBot, sizeof(bot)) == 0, "collision response never advances or changes a frozen bot");
	human.velocity = incoming;
	human.stepFlagSet = 0;
	human.collisionFlags = 0;
	VehPhysForce_CollideDrivers(&humanThread, &human);
	CTR_TEST_CHECK_EQ(human.velocity.x, 1000, "the driver collision stage skips kart searches during time stop");
	CTR_TEST_SECTION("pass-through mask contacts are retained once and displayed on resumption");
	bot.kartState = KS_NORMAL;
	bot.actionsFlagSet &= ~ACTION_MASK_WEAPON;
	human.actionsFlagSet |= ACTION_MASK_WEAPON;
	g_config.maskDamagesOthers = true;
	const struct Driver botBeforeMask = bot;
	const Vec3 velocityBeforeMask = human.velocity;
	VehPhysForce_CollideDrivers(&humanThread, &human);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numDeferredImpacts, 1, "passing through a frozen bot with a mask records its hit");
	VehPhysForce_CollideDrivers(&humanThread, &human);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numDeferredImpacts, 1, "remaining inside a kart cannot queue the same mask hit every frame");
	CTR_TEST_CHECK(memcmp(&bot, &botBeforeMask, sizeof(bot)) == 0, "mask contact leaves the frozen bot unchanged until resumption");
	CTR_TEST_CHECK(memcmp(&human.velocity, &velocityBeforeMask, sizeof(human.velocity)) == 0, "recording a mask hit never blocks or bounces the human");
	humanInstance.matrix.t[0] += 10000;
	Tick(1000);
	Chaos_ResolveDeferredImpacts();
	CTR_TEST_CHECK_EQ(bot.kartState, KS_BLASTED, "the recorded mask hit blasts the bot after time resumes even after the attacker leaves");
	humanInstance.matrix.t[0] -= 10000;
	human.actionsFlagSet &= ~ACTION_MASK_WEAPON;
	Chaos_Activate(CHAOS_EFFECT_ZA_WARUDO, 1000);
	CTR_TEST_SECTION("obstacle collisions continue without advancing their animation");
	struct Thread obstacle = {.flags = STATIC, .funcThTick = RB_Snowball_ThTick, .inst = &botInstance, .object = &bot};
	s_frozenObstacleCollisions = 0;
	Chaos_CollideFrozenThread(&obstacle);
	CTR_TEST_CHECK_EQ(s_frozenObstacleCollisions, 1, "frozen moving obstacles still receive their collision-only pass");

	CTR_TEST_SECTION("human particle metadata never advances a nonexistent axis into its neighbor");
	struct Particle particles[2] = {
		{.framesLeftInLife = 10, .flagsAxis = PARTICLE_AXIS_FLAG_HUMAN_EFFECT | PARTICLE_AXIS_FLAG_ICON_FRAME_OR_LINE_COLOR},
		{.framesLeftInLife = 10, .owner.driverInst = &botInstance},
	};
	particles[0].next = &particles[1];
	particles[0].axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR] = (struct ParticleAxis){.startVal = 100, .velocity = 7, .accel = 2};
	// The former axis-15 write landed exactly on the next particle's owner union.
	particles[1].axis[0].velocity = 123;
	const struct Particle neighbor = particles[1];
	struct Particle *head = particles;
	Particle_UpdateList(&head, head);
	CTR_TEST_CHECK_EQ(particles[0].framesLeftInLife, 9, "human weapon particle lifetime still advances during time stop");
	CTR_TEST_CHECK_EQ(particles[0].axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].startVal, 107, "the eleventh real axis keeps its ordinary integration");
	CTR_TEST_CHECK_EQ(particles[0].axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].velocity, 9, "real particle acceleration is preserved");
	CTR_TEST_CHECK(memcmp(&particles[1], &neighbor, sizeof(neighbor)) == 0, "human ownership metadata cannot overwrite the frozen neighboring particle");

	CTR_TEST_SECTION("audio ownership pauses world and music channels and survives channel reuse");
	struct ChannelStats world = {.channelID = 3, .type = HOWL_CHANNEL_TYPE_OTHER_FX, .soundID = HOWL_SFX_TNT_EXPLOSION};
	struct ChannelStats music = {.channelID = 4, .type = HOWL_CHANNEL_TYPE_MUSIC};
	Chaos_SetHumanAudio(true);
	Chaos_TagAudioChannel(&world);
	CTR_TEST_CHECK(!Chaos_ShouldFreezeAudioChannel(&world), "human sounds stay audible");
	Chaos_SetHumanAudio(false);
	Chaos_TagAudioChannel(&world);
	CTR_TEST_CHECK(Chaos_ShouldFreezeAudioChannel(&world), "reusing a human channel for a world sound removes the old exemption");
	const int menuSounds[] = {MENU_SFX_MOVE, MENU_SFX_BACK, MENU_SFX_CONFIRM, MENU_SFX_LOCKED};
	for (int i = 0; i < (int)len(menuSounds); i++)
	{
		struct ChannelStats menu = {.type = HOWL_CHANNEL_TYPE_OTHER_FX, .soundID = menuSounds[i]};
		CTR_TEST_CHECK(!Chaos_ShouldFreezeAudioChannel(&menu), "menu navigation, back, confirm and locked sounds remain audible during time stop");
		// OtherFX_Play_LowLevel packs CountSounds() into the upper half of soundID.
		menu.soundID |= 0x12340000;
		CTR_TEST_CHECK(!Chaos_ShouldFreezeAudioChannel(&menu), "real packed menu handles retain their exemption regardless of the sound-instance counter");
		s_tracker.gameMode1 |= PAUSE_1;
		CTR_TEST_CHECK(!Chaos_ShouldFreezeAudioChannel(&menu), "opening pause does not freeze menu sounds");
		CTR_TEST_CHECK(Chaos_ShouldFreezeAudioChannel(&world), "allowing pause-menu sounds does not unfreeze world audio");
		menu.channelID = 5;
		menu.link.links.next = &world;
		sdata->channelTaken.first = &menu.link.item;
		Chaos_UpdateTimeStopAudio();
		CTR_TEST_CHECK_EQ(s_pausedVoices, 1u << world.channelID, "the mixer pauses only the world channel while a packed menu sound is playing");
		sdata->channelTaken.first = NULL;
		s_tracker.gameMode1 &= ~PAUSE_1;
		CTR_TEST_CHECK(!Chaos_ShouldFreezeAudioChannel(&menu), "the back sound remains audible after closing pause");
		menu.type = HOWL_CHANNEL_TYPE_MUSIC;
		CTR_TEST_CHECK(Chaos_ShouldFreezeAudioChannel(&menu), "music sharing a menu sound number still freezes");
	}
	Chaos_SetHumanAudio(true);
	Chaos_TagAudioChannel(&music);
	Chaos_SetHumanAudio(false);
	CTR_TEST_CHECK(Chaos_ShouldFreezeAudioChannel(&music), "music cannot inherit a human channel exemption");
	CTR_TEST_SECTION("audio slider previews continue while race music remains frozen");
	sdata->OptionSlider_BoolPlay = true;
	sdata->OptionSlider_Index = HOWL_VOLUME_TYPE_MUSIC;
	sdata->songSeq[0].songPoolIndex = 1;
	sdata->songSeq[1].songPoolIndex = 0;
	sdata->songPool[0].id = CSEQ_SONG_LEVEL;
	const int previewSongs[] = {CSEQ_SONG_AKU, CSEQ_SONG_UKA};
	for (int i = 0; i < (int)len(previewSongs); i++)
	{
		sdata->songPool[1].id = previewSongs[i];
		CTR_TEST_CHECK(Chaos_IsMenuPreviewSong(&sdata->songPool[1]), "the selected mask-song preview may advance its sequencer during time stop");
		CTR_TEST_CHECK(!Chaos_ShouldFreezeAudioChannel(&music), "the music slider preview remains audible in its own song pool slot");
		struct ChannelStats background = {.type = HOWL_CHANNEL_TYPE_MUSIC, .soundID = 1};
		CTR_TEST_CHECK(Chaos_ShouldFreezeAudioChannel(&background), "the preview does not resume the race soundtrack in another song pool slot");
	}
	sdata->OptionSlider_Index = HOWL_VOLUME_TYPE_FX;
	CTR_TEST_CHECK(Chaos_ShouldFreezeAudioChannel(&music), "leaving the music slider removes its preview exemption");
	sdata->OptionSlider_BoolPlay = false;
	sdata->OptionSlider_Index = HOWL_VOLUME_TYPE_MUSIC;
	CTR_TEST_CHECK(Chaos_ShouldFreezeAudioChannel(&music), "stopping preview freezes music again even if its slider remains selected");
	sdata->channelTaken.first = &world.link.item;
	Chaos_UpdateTimeStopAudio();
	CTR_TEST_CHECK(s_pausedVoices == (1u << 3) && s_pausedXA, "the backend receives the frozen voices and XA pause state");
	sdata->chaosRuntime.humanXA = true;
	Chaos_UpdateTimeStopAudio();
	CTR_TEST_CHECK(!s_pausedXA, "human XA voices remain audible");
	s_tracker.gameMode1 |= END_OF_RACE;
	Chaos_UpdateTimeStopAudio();
	CTR_TEST_CHECK(s_pausedVoices == 0 && !s_pausedXA, "race completion releases suspended audio without waiting for effect expiry");
	Chaos_Reset();
	CTR_TEST_CHECK(!Chaos_IsTimeStopped() && s_pausedVoices == 0, "reset leaves no time stop or audio suspension behind");
}

static void TestWobbleSteering(void)
{
	CTR_TEST_SECTION("Wobble oscillates in game time with calibrated limits and human-only scope");
	EnableWobble();
	struct Driver human = {.driverID = 0};
	struct Driver partner = {.driverID = 1};
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT};
	struct GamepadBuffer pad = {.stickLX = 128};
	Chaos_Activate(CHAOS_EFFECT_TOTALLY_SOBER, 6000);
	const u32 rng = sdata->chaosRng;
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 128, "Wobble begins at the live steering input");
	Tick(500);
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 224, "a quarter period produces the positive steering peak beyond the retail dead zone");
	CTR_TEST_CHECK_EQ(PreparedSteering(&partner, &pad), 224, "the co-op partner receives the same phase");
	CTR_TEST_CHECK_EQ(PreparedSteering(&bot, &pad), 128, "Wobble does not change a bot's steering");
	human.actionsFlagSet = ACTION_RACE_FINISHED;
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 128, "Wobble leaves finished humans alone");
	human.actionsFlagSet = 0;
	CTR_TEST_CHECK_EQ(pad.stickLX, 128, "Wobble does not overwrite shared controller input");
	pad.stickLX = 255;
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 256, "the positive peak clamps to the signed controller limit without wrapping");
	pad.stickLX = 128;
	struct RacingWheelData wheel = {.gamepadCenter = 100, .range = 64};
	pad.rwd = &wheel;
	pad.stickLX = 100;
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 148, "wheel oscillation scales to the calibrated range");
	pad.stickLX = 160;
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 164, "wheel steering clamps around its calibrated center");
	wheel.range = 0;
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 160, "a wheel without a usable range preserves its live input");
	pad.rwd = NULL;
	pad.stickLX = 128;
	const struct ChaosRuntime saved = sdata->chaosRuntime;
	s_tracker.gameMode1 |= PAUSE_1;
	Tick(1000);
	CTR_TEST_CHECK(memcmp(&saved, &sdata->chaosRuntime, sizeof(saved)) == 0, "pause freezes the oscillator together with effect timers");
	s_tracker.gameMode1 &= ~PAUSE_1;
	Tick(1000);
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 32, "three quarters of a period produces the opposite steering peak");
	pad.stickLX = 0;
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 0, "the negative peak clamps without reversing a full steering input");
	pad.stickLX = 128;
	sdata->chaosRuntime = saved;
	Tick(1000);
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 32, "restoring resident runtime reproduces the oscillator phase");
	Tick(500);
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 128, "a full two-second period returns to the live input");
	CTR_TEST_CHECK_EQ(sdata->chaosRng, rng, "Wobble phase consumes no random choices");
	Chaos_Reset();
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 128, "reset leaves no synthetic steering behind");

	CTR_TEST_SECTION("steering reactivation resets phase and replaces the previous lifetime");
	EnableWobble();
	Chaos_Activate(CHAOS_EFFECT_TOTALLY_SOBER, 6000);
	Tick(500);
	Chaos_Activate(CHAOS_EFFECT_TOTALLY_SOBER, 1000);
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 128, "a new Wobble activation uses its own phase instead of adding both waves");
	Tick(500);
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 224, "the latest Wobble supplies one oscillation amplitude");
	Tick(500);
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 128, "expiry of reactivated Wobble cannot reveal the replaced oscillator");
	pad.stickLX = 64;
	Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 500);
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 192, "later Inverted Steering overrides the older Wobble");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_TOTALLY_SOBER), "Inverted Steering immediately deactivates conflicting Wobble entries");
	Tick(500);
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 64, "expiry of Inverted Steering returns to live input without reviving Wobble");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "replaced effects do not remain dormant in the activation queue");
	EnableWobble();
	Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 4000);
	Chaos_Activate(CHAOS_EFFECT_TOTALLY_SOBER, 1000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_MIRROR_MIRROR), "Wobble immediately deactivates conflicting Inverted Steering");
	Tick(500);
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 160, "later Wobble overrides Inverted Steering using live input");
	Tick(500);
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 64, "expiry of Wobble returns to live input without reviving Inverted Steering");
	Tick(3000);
	CTR_TEST_CHECK_EQ(PreparedSteering(&human, &pad), 64, "final expiry restores ordinary input without a stored snapshot");

	CTR_TEST_SECTION("conflict replacement frees capacity and preserves unrelated effects");
	EnableWobble();
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 6000);
	Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 6000);
	sdata->chaosSettings.effects[CHAOS_EFFECT_FLY_ME_TO_THE_MOON] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_FAIR_MODE] = true;
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 5000);
	Chaos_Activate(CHAOS_EFFECT_FAIR_MODE, 5000);
	Tick(500);
	const struct ChaosActivation firstHop = sdata->chaosRuntime.active[0];
	const struct ChaosActivation gravity = sdata->chaosRuntime.active[2];
	const struct ChaosActivation items = sdata->chaosRuntime.active[3];
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_TOTALLY_SOBER, 0), "invalid activation cannot cancel conflicting running effects");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 4, "a rejected conflict replacement preserves all running activations");
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_TOTALLY_SOBER, 1000), "a conflicting replacement can activate even when all four slots were occupied");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 4, "replacement removes the conflicting effect before adding the new activation");
	CTR_TEST_CHECK(memcmp(&firstHop, &sdata->chaosRuntime.active[0], sizeof(firstHop)) == 0,
	    "replacement preserves the first unrelated activation and its timers");
	CTR_TEST_CHECK(memcmp(&gravity, &sdata->chaosRuntime.active[1], sizeof(gravity)) == 0,
	    "replacement preserves unrelated gravity and its ordering");
	CTR_TEST_CHECK(memcmp(&items, &sdata->chaosRuntime.active[2], sizeof(items)) == 0,
	    "replacement preserves unrelated item restrictions");
}

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
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 3600, "reactivating Heavy Karts does not compound gravity");
	CTR_TEST_CHECK_EQ(human.const_JumpForce, 800, "Heavy Karts changes gravity without disabling the jump impulse");
	human.const_Gravity = 32767;
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 98301, "effective gravity remains a full integer rather than overflowing the stored halfword");
	human.const_Gravity = 1200;
	const struct ChaosRuntime saved = sdata->chaosRuntime;
	s_tracker.gameMode1 |= PAUSE_1;
	Tick(5000);
	CTR_TEST_CHECK(memcmp(&saved, &sdata->chaosRuntime, sizeof(saved)) == 0, "pause freezes gravity effect lifetimes");
	s_tracker.gameMode1 &= ~PAUSE_1;
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 3600, "unpausing retains the gravity modifier");
	human.actionsFlagSet = ACTION_RACE_FINISHED;
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 1200, "finished humans immediately use normal gravity");
	human.actionsFlagSet = 0;
	Tick(3000);
	sdata->chaosRuntime = saved;
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 3600, "resident restoration needs no separate baseline repair");
	Chaos_Reset();
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 1200, "reset leaves no modified driver constants behind");

	CTR_TEST_SECTION("opposite gravity effects deactivate each other while preserving jumping and unrelated effects");
	Chaos_BeginRace(&s_tracker);
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 10000);
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 3000);
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 2000);
	Chaos_Activate(CHAOS_EFFECT_HEAVY_METAL, 1000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 2, "Heavy Karts replaces Low Gravity but preserves unrelated effects");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_FLY_ME_TO_THE_MOON), "replaced Low Gravity entries are permanently deactivated");
	CTR_TEST_CHECK(Chaos_IsActive(CHAOS_EFFECT_GROUNDED), "gravity replacement leaves No Hopping active");
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 3600, "the latest gravity effect takes effect immediately");
	Tick(1000);
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 1200, "expiry of Heavy Karts cannot revive replaced Low Gravity");
	Chaos_Activate(CHAOS_EFFECT_HEAVY_METAL, 3000);
	Chaos_Activate(CHAOS_EFFECT_FLY_ME_TO_THE_MOON, 1000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_HEAVY_METAL), "Low Gravity also permanently deactivates Heavy Karts");
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 300, "Low Gravity can replace Heavy Karts without multiplying modified values");
	Tick(1000);
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 1200, "expiry of Low Gravity cannot revive replaced Heavy Karts");
	Chaos_Reset();
	Chaos_BeginRace(&s_tracker);
	Chaos_Activate(CHAOS_EFFECT_HEAVY_METAL, 1000);
	CTR_TEST_CHECK((PreparedJump(&human, BTN_L1, BTN_L1, RAIN_CLOUD_EFFECT_NONE, 0) & ACTION_JUMP_BUTTON_HELD) != 0,
	    "Heavy Karts still allows manual hops");
	CTR_TEST_CHECK_EQ(human.jump_TenBuffer, 10, "Heavy Karts retains the retail hop buffer");
	s_tracker.gameMode1 |= ADVENTURE_ARENA;
	CTR_TEST_CHECK_EQ(Chaos_GravityForDriver(&human), 1200, "Adventure hubs cannot receive gravity modifiers");
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
	const struct ChaosActivation gravity = sdata->chaosRuntime.active[0];
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_GOTTA_GO_FAST, 1000), "Super Speed can replace a refreshed cap");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 2, "replacement removes the cap but keeps the unrelated gravity effect");
	CTR_TEST_CHECK(memcmp(&gravity, &sdata->chaosRuntime.active[0], sizeof(gravity)) == 0, "replacement preserves unrelated timers and order");
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

static void TestDefectiveBoosts(void)
{
	CTR_TEST_SECTION("Defective Boosts inverts fire speed while retaining ordinary reserve grants");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_BROWN_FIRE] = true;
	sdata->chaosSettings.enabledCount++;
	struct Thread thread = {.modelIndex = DYNAMIC_PLAYER};
	struct Instance inst = {.thread = &thread};
	const struct Driver initial = {
		.driverID = 0, .instSelf = &inst, .kartState = KS_NORMAL,
		.const_SingleTurboSpeed = 1000, .const_SacredFireSpeed = 2000,
	};
	const u32 types[] = {
		0, POWER_SLIDE_HANG_TIME,
		TURBO_PAD | FREEZE_RESERVES_ON_TURBO_PAD,
		TURBO_ITEM | FREEZE_RESERVES_ON_TURBO_PAD,
		TURBO_PAD | SUPER_ENGINE,
	};
	struct Driver retail[len(types)];
	for (size_t i = 0; i < len(types); i++)
	{
		retail[i] = initial;
		VehFire_Increment(&retail[i], 1000, types[i], 128);
	}
	struct Driver human = initial;
	human.reserves = 1000;
	human.fireSpeedCap = 3000;
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 15000), 15000, "inactive Defective Boosts retains the ordinary fire bonus");
	Chaos_Activate(CHAOS_EFFECT_BROWN_FIRE, 5000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 15000), 9000, "the existing three-thousand fire bonus becomes a three-thousand penalty");
	for (size_t i = 0; i < len(types); i++)
	{
		human = initial;
		VehFire_Increment(&human, 1000, types[i], 128);
		CTR_TEST_CHECK_EQ(human.reserves, retail[i].reserves, "start, drift, hang-time, pad, item and Super Engine grants retain retail reserves");
		CTR_TEST_CHECK_EQ(human.turbo_outsideTimer, retail[i].turbo_outsideTimer, "pad and item outside-reserve bookkeeping remains retail");
		CTR_TEST_CHECK_EQ(human.fireSpeedCap, retail[i].fireSpeedCap, "the stored fire level stays positive and unchanged");
		CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 12000 + human.fireSpeedCap), 12000 - human.fireSpeedCap,
		                  "each boost source subtracts its live bonus rather than adding it");
	}
	human = initial;
	VehFire_Increment(&human, 1000, POWER_SLIDE_HANG_TIME, 128);
	VehFire_Increment(&human, 1000, POWER_SLIDE_HANG_TIME, 256);
	CTR_TEST_CHECK_EQ(human.reserves, 2000, "repeated boosts accumulate reserves instead of clearing fuel");
	CTR_TEST_CHECK_EQ(human.fireSpeedCap, 2000, "a stronger boost raises the stored fire level normally");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 14000), 10000, "stronger fire produces a larger speed penalty");
	human.reserves = 1;
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 14000), 10000, "the last remaining reserve keeps the penalty active");
	human.reserves = 0;
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 12000), 12000, "empty reserves remove the penalty even if the stored fire level remains");
	human.reserves = 1000;
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 3000), 0, "an oversized penalty stops at zero without creating reverse acceleration");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, -3000), -3000, "reverse targets remain unchanged");
	CTR_TEST_CHECK_EQ(Chaos_AccelerationForDriver(&human, 200), 200, "reserve acceleration stays retail and approaches the reduced speed target");
	struct Driver partner = human;
	partner.driverID = 1;
	struct Driver bot = human;
	bot.driverID = 2;
	bot.actionsFlagSet = ACTION_BOT;
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&partner, 14000), 10000, "the co-op partner also receives defective boosts");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&bot, 14000), 14000, "bot boost bonuses remain ordinary");
	human.actionsFlagSet |= ACTION_RACE_FINISHED;
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 14000), 14000, "finished humans retain ordinary boost bonuses");
	human.actionsFlagSet &= ~ACTION_RACE_FINISHED;
	s_tracker.gameMode1 |= ADVENTURE_ARENA;
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 14000), 14000, "Adventure hubs retain ordinary boost bonuses");
	s_tracker.gameMode1 &= ~ADVENTURE_ARENA;
	Chaos_Activate(CHAOS_EFFECT_BROWN_FIRE, 5000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 14000), 10000, "duplicate activations invert the bonus only once");
	Tick(1000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 14000), 10000, "the replacement activation retains the penalty");
	Tick(4000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 14000), 14000, "final expiry restores the bonus from current reserves");
	CTR_TEST_CHECK_EQ(human.reserves, 1000, "effect expiry never restores or discards fuel");
}

static void TestThisIsFire(void)
{
	CTR_TEST_SECTION("This Is Fire upgrades existing turbo without granting fuel or a synthetic boost");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_THIS_IS_FIRE] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_BROWN_FIRE] = true;
	sdata->chaosSettings.enabledCount += 2;
	struct Thread driverThread = {.modelIndex = DYNAMIC_PLAYER};
	struct Instance driverInst = {.thread = &driverThread};
	const struct Driver initial = {
		.driverID = 0, .instSelf = &driverInst, .kartState = KS_NORMAL,
		.const_SingleTurboSpeed = 1000, .const_SacredFireSpeed = 2000,
	};
	struct Driver human = initial;
	human.reserves = 500;
	human.turbo_outsideTimer = 100;
	human.fireSpeedCap = 1500;
	human.numTurbos = 3;
	struct Driver partner = initial;
	partner.driverID = 1;
	struct Driver bot = human;
	bot.driverID = 2;
	bot.actionsFlagSet = ACTION_BOT;
	s_tracker.drivers[0] = &human;
	s_tracker.drivers[1] = &partner;
	s_tracker.drivers[2] = &bot;
	struct Turbo turbo = {.driver = &human, .fireSize = 5};
	struct Thread turboThread = {.object = &turbo};
	s_tracker.threadBuckets[TURBO].thread = &turboThread;
	Chaos_Activate(CHAOS_EFFECT_BROWN_FIRE, 5000);
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_THIS_IS_FIRE, 1000), "This Is Fire activates as a timed effect");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_BROWN_FIRE), "This Is Fire replaces Defective Boosts");
	CTR_TEST_CHECK_EQ(human.fireSpeedCap, 9000, "existing turbo immediately receives the super-pad speed tier");
	CTR_TEST_CHECK_EQ(turbo.fireSize, 8, "existing exhaust receives normal USF flame sizing");
	CTR_TEST_CHECK(human.reserves == 500 && human.turbo_outsideTimer == 100, "upgrading existing fire preserves both fuel fields");
	CTR_TEST_CHECK(human.numTurbos == 3 && human.actionsFlagSet == 0, "activation does not count or latch an extra boost");
	CTR_TEST_CHECK(partner.reserves == 0 && partner.fireSpeedCap == 0, "an unboosted human receives no free turbo or reserves");
	CTR_TEST_CHECK_EQ(bot.fireSpeedCap, 1500, "existing bot turbo remains unchanged");
	CTR_TEST_CHECK_EQ(Chaos_FireLevelForDriver(&bot, 128), 128, "new bot turbo remains ordinary");
	CTR_TEST_CHECK_EQ(Chaos_FireLevelForDriver(&partner, VEH_PHYS_FORCE_SUPER_TURBO_PAD_FIRE_LEVEL + 128), VEH_PHYS_FORCE_SUPER_TURBO_PAD_FIRE_LEVEL, "active effect selects the super-pad fire level");
	s_tracker.threadBuckets[TURBO].thread = NULL;

	const u32 types[] = {
		START_LINE_BOOST, POWER_SLIDE_HANG_TIME,
		TURBO_PAD | FREEZE_RESERVES_ON_TURBO_PAD,
		TURBO_ITEM | FREEZE_RESERVES_ON_TURBO_PAD, TURBO_PAD | SUPER_ENGINE,
	};
	for (size_t i = 0; i < len(types); i++)
	{
		partner = initial;
		partner.driverID = 1;
		VehFire_Increment(&partner, 300, types[i], 128);
		CTR_TEST_CHECK_EQ(partner.fireSpeedCap, 9000, "start, drift, hang-time, pad, item and engine boosts receive USF");
		CTR_TEST_CHECK_EQ(partner.reserves, 300, "upgraded boost sources grant only their normal reserves");
	}
	VehFire_Increment(&human, 100, POWER_SLIDE_HANG_TIME, 128);
	CTR_TEST_CHECK_EQ(human.fireSpeedCap, 9000, "a weaker boost cannot cancel USF during the effect");
	CTR_TEST_CHECK_EQ(human.reserves, 600, "a new boost adds only its earned reserves");
	Tick(1000);
	CTR_TEST_CHECK_EQ(human.fireSpeedCap, 9000, "expiry does not remove earned USF");
	VehFire_Increment(&human, 100, POWER_SLIDE_HANG_TIME, 128);
	CTR_TEST_CHECK_EQ(human.fireSpeedCap, 1500, "the next weaker boost cancels USF normally after expiry");
	Chaos_Activate(CHAOS_EFFECT_THIS_IS_FIRE, 5000);
	Chaos_Activate(CHAOS_EFFECT_BROWN_FIRE, 1000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_THIS_IS_FIRE), "Defective Boosts also replaces This Is Fire");
}

static void TestNoReserves(void)
{
	CTR_TEST_SECTION("No Reserves clears fuel at driver application while boost grants remain retail");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_HOLE_IN_THE_TANK] = true;
	sdata->chaosSettings.enabledCount++;
	struct Thread thread = {.modelIndex = DYNAMIC_PLAYER};
	struct Instance inst = {.thread = &thread};
	struct Driver human = {
		.driverID = 0, .instSelf = &inst, .reserves = 2000, .turbo_outsideTimer = 1000,
		.kartState = KS_NORMAL, .baseSpeed = 1000, .const_SingleTurboSpeed = 100,
		.const_SacredFireSpeed = 200, .rainCloudEffect = RAIN_CLOUD_EFFECT_NONE,
	};
	struct GamepadBuffer pad = {.stickLX = 128};
	int steering = pad.stickLX;
	PreparedDriver(&human, &pad, &steering, human.actionsFlagSet);
	CTR_TEST_CHECK_EQ(human.reserves, 2000, "inactive Chaos leaves accumulated reserves alone");
	CTR_TEST_CHECK_EQ(human.turbo_outsideTimer, 1000, "inactive Chaos leaves the outside turbo timer alone");
	Chaos_Activate(CHAOS_EFFECT_HOLE_IN_THE_TANK, 2000);
	const u32 actions = ACTION_JUMP_BUTTON_HELD | ACTION_MASK_WEAPON;
	CTR_TEST_CHECK_EQ(PreparedDriver(&human, &pad, &steering, actions), actions,
	                  "No Reserves preserves manual hops and mask input flags");
	CTR_TEST_CHECK_EQ(human.reserves, 0, "the existing driver hook clears accumulated reserves before physics consumers");
	CTR_TEST_CHECK_EQ(human.turbo_outsideTimer, 0, "the existing driver hook also clears outside turbo bookkeeping");
	CTR_TEST_CHECK_EQ(human.baseSpeed, 1000, "No Reserves does not overwrite base speed");
	CTR_TEST_CHECK_EQ(steering, 128, "No Reserves does not change steering");
	const u32 types[] = {
		POWER_SLIDE_HANG_TIME,
		TURBO_PAD | FREEZE_RESERVES_ON_TURBO_PAD,
		TURBO_ITEM | FREEZE_RESERVES_ON_TURBO_PAD,
		TURBO_PAD | SUPER_ENGINE,
	};
	for (int i = 0; i < (int)(sizeof(types) / sizeof(types[0])); i++)
	{
		human.reserves = 500;
		human.turbo_outsideTimer = 100;
		VehFire_Increment(&human, 1000, types[i], 100);
		CTR_TEST_CHECK(human.reserves > 0, "retail boost grants can supply fuel until the next driver application");
		PreparedDriver(&human, &pad, &steering, human.actionsFlagSet);
		CTR_TEST_CHECK_EQ(human.reserves, 0, "the next driver application clears newly granted reserves");
		CTR_TEST_CHECK_EQ(human.turbo_outsideTimer, 0, "the next driver application clears outside turbo time too");
		CTR_TEST_CHECK((human.actionsFlagSet & ACTION_NEW_BOOST) != 0, "retail boost event flags remain available for presentation");
	}
	struct Driver partner = {.driverID = 1, .reserves = 500, .turbo_outsideTimer = 200};
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT, .reserves = 700, .turbo_outsideTimer = 300};
	PreparedDriver(&partner, &pad, &steering, partner.actionsFlagSet);
	PreparedDriver(&bot, &pad, &steering, bot.actionsFlagSet);
	PreparedDriver(NULL, &pad, &steering, 0);
	CTR_TEST_CHECK_EQ(partner.reserves, 0, "No Reserves also applies to the co-op partner");
	CTR_TEST_CHECK_EQ(partner.turbo_outsideTimer, 0, "the partner's two fuel fields are cleared together");
	CTR_TEST_CHECK_EQ(bot.reserves, 700, "No Reserves preserves bot reserves");
	CTR_TEST_CHECK_EQ(bot.turbo_outsideTimer, 300, "No Reserves preserves bot outside turbo time");
	human.actionsFlagSet |= ACTION_RACE_FINISHED;
	VehFire_Increment(&human, 1000, TURBO_PAD | FREEZE_RESERVES_ON_TURBO_PAD, 100);
	PreparedDriver(&human, &pad, &steering, human.actionsFlagSet);
	CTR_TEST_CHECK_EQ(human.reserves, 1000, "finished humans can receive ordinary retail boosts");
	CTR_TEST_CHECK_EQ(human.turbo_outsideTimer, 1000, "finished humans retain outside turbo time");
	human.actionsFlagSet &= ~ACTION_RACE_FINISHED;
	human.reserves = 0;
	human.turbo_outsideTimer = 0;
	s_tracker.gameMode1 |= PAUSE_1;
	const struct ChaosRuntime saved = sdata->chaosRuntime;
	Tick(5000);
	CTR_TEST_CHECK(memcmp(&saved, &sdata->chaosRuntime, sizeof(saved)) == 0, "pause freezes No Reserves lifetimes");
	s_tracker.gameMode1 &= ~PAUSE_1;
	Tick(1000);
	Chaos_Activate(CHAOS_EFFECT_HOLE_IN_THE_TANK, 1000);
	Tick(500);
	VehFire_Increment(&human, 1000, POWER_SLIDE_HANG_TIME, 100);
	PreparedDriver(&human, &pad, &steering, human.actionsFlagSet);
	CTR_TEST_CHECK_EQ(human.reserves, 0, "the refreshed restriction still clears reserves before its own expiry");
	Tick(500);
	CTR_TEST_CHECK_EQ(human.reserves, 0, "final expiry does not restore reserves erased earlier");
	VehFire_Increment(&human, 1000, POWER_SLIDE_HANG_TIME, 100);
	PreparedDriver(&human, &pad, &steering, human.actionsFlagSet);
	CTR_TEST_CHECK_EQ(human.reserves, 1000, "new reserves can accumulate normally after final expiry");
	sdata->chaosRuntime = saved;
	PreparedDriver(&human, &pad, &steering, human.actionsFlagSet);
	CTR_TEST_CHECK_EQ(human.reserves, 0, "resident restoration restores the restriction without a separate driver snapshot");
	Chaos_Reset();
	VehFire_Increment(&human, 1000, TURBO_PAD | FREEZE_RESERVES_ON_TURBO_PAD, 100);
	PreparedDriver(&human, &pad, &steering, human.actionsFlagSet);
	CTR_TEST_CHECK_EQ(human.reserves, 1000, "reset immediately restores normal boost grants");
	CTR_TEST_CHECK_EQ(human.turbo_outsideTimer, 1000, "reset also restores ordinary outside turbo bookkeeping");
	Chaos_BeginRace(&s_tracker);
	Chaos_Activate(CHAOS_EFFECT_HOLE_IN_THE_TANK, 1000);
	s_tracker.gameMode1 |= ADVENTURE_ARENA;
	PreparedDriver(&human, &pad, &steering, human.actionsFlagSet);
	CTR_TEST_CHECK_EQ(human.reserves, 1000, "Adventure hubs do not receive reserve restrictions");
}

static void TestNoItems(void)
{
	CTR_TEST_SECTION("No Items discards inventory and pending requests before roulette and weapon processing");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_FAIR_MODE] = true;
	sdata->chaosSettings.enabledCount++;
	struct Instance shield = {0};
	struct Instance bomb = {0};
	struct Thread cloud = {0};
	struct Driver human = {
		.driverID = 0, .heldItemID = HELD_ITEM_BOMB_3X, .numHeldItems = 3,
		.noItemTimer = 20, .itemRollTimer = 10,
		.actionsFlagSet = ACTION_WEAPON_FIRE_REQUEST | ACTION_MASK_WEAPON | ACTION_JUMP_BUTTON_HELD,
		.instBubbleHold = &shield, .instBombThrow = &bomb, .thCloud = &cloud,
		.superEngineTimer = 1000, .invisibleTimer = 2000, .invincibleTimer = 3000,
		.numWumpas = 10, .reserves = 1000, .turbo_outsideTimer = 500,
	};
	struct Driver partner = {.driverID = 1, .heldItemID = HELD_ITEM_NONE};
	s_tracker.drivers[0] = &human;
	s_tracker.drivers[1] = &partner;
	const struct Driver original = human;
	Chaos_ApplyItemRules(&human);
	CTR_TEST_CHECK(memcmp(&human, &original, sizeof(human)) == 0, "inactive No Items leaves inventory and pending requests intact");
	Chaos_Activate(CHAOS_EFFECT_FAIR_MODE, 2000);
	const u8 items[] = {
		HELD_ITEM_TURBO, HELD_ITEM_BOMB_1X, HELD_ITEM_MISSILE_1X, HELD_ITEM_TNT,
		HELD_ITEM_POTION, HELD_ITEM_SPRING, HELD_ITEM_SHIELD, HELD_ITEM_MASK,
		HELD_ITEM_CLOCK, HELD_ITEM_BOMB_3X, HELD_ITEM_MISSILE_3X,
		HELD_ITEM_INVISIBILITY, HELD_ITEM_SUPER_ENGINE, HELD_ITEM_NONE, HELD_ITEM_ROULETTE,
	};
	for (size_t i = 0; i < len(items); i++)
	{
		human.heldItemID = items[i];
		human.numHeldItems = 3;
		human.itemRollTimer = 10;
		human.noItemTimer = 20;
		human.actionsFlagSet |= ACTION_WEAPON_FIRE_REQUEST;
		Chaos_ApplyItemRules(&human);
		CTR_TEST_CHECK_EQ(human.heldItemID, HELD_ITEM_NONE, "every held item or roulette sentinel is discarded");
		CTR_TEST_CHECK_EQ(human.numHeldItems, 0, "discarding inventory clears the stack count");
		CTR_TEST_CHECK_EQ(human.itemRollTimer, 0, "discarding inventory cancels pending roulette time");
		CTR_TEST_CHECK_EQ(human.noItemTimer, 0, "discarding inventory cancels the old weapon cooldown");
		CTR_TEST_CHECK_EQ(human.actionsFlagSet, ACTION_MASK_WEAPON | ACTION_JUMP_BUTTON_HELD,
		                  "pending weapon requests are removed without changing mask or jump flags");
	}
	CTR_TEST_CHECK(human.instBubbleHold == &shield && human.instBombThrow == &bomb,
	               "already deployed shields and remote bombs remain owned by the driver");
	CTR_TEST_CHECK(human.thCloud == &cloud, "No Items leaves retail rain-cloud ownership intact");
	CTR_TEST_CHECK_EQ(human.superEngineTimer, 1000, "already activated Super Engine remains active");
	CTR_TEST_CHECK_EQ(human.invisibleTimer, 2000, "already activated invisibility remains active");
	CTR_TEST_CHECK_EQ(human.invincibleTimer, 3000, "No Items preserves existing invincibility");
	CTR_TEST_CHECK_EQ(human.numWumpas, 10, "No Items does not remove collected wumpa");
	CTR_TEST_CHECK_EQ(human.reserves, 1000, "No Items does not remove existing boost reserves");
	CTR_TEST_CHECK_EQ(human.turbo_outsideTimer, 500, "No Items does not alter outside turbo time");
	human.heldItemID = HELD_ITEM_ROULETTE;
	human.itemRollTimer = 0;
	Chaos_ApplyItemRules(&human);
	CTR_TEST_CHECK_EQ(human.heldItemID, HELD_ITEM_NONE, "a roulette already at its final frame is cancelled before award selection");

	CTR_TEST_SECTION("No Items clears the warpball flag and also empties the co-op partner's inventory");
	human.heldItemID = HELD_ITEM_WARPBALL;
	human.actionsFlagSet |= ACTION_WEAPON_FIRE_REQUEST;
	s_tracker.gameMode1 |= WARPBALL_HELD;
	Chaos_ApplyItemRules(&human);
	CTR_TEST_CHECK((s_tracker.gameMode1 & WARPBALL_HELD) == 0, "discarding an unlaunched warpball releases its reservation");
	CTR_TEST_CHECK((human.actionsFlagSet & ACTION_WEAPON_FIRE_REQUEST) == 0, "discarding a requested warpball prevents its pending launch");
	s_tracker.gameMode1 |= WARPBALL_HELD;
	Chaos_ApplyItemRules(&human);
	CTR_TEST_CHECK((s_tracker.gameMode1 & WARPBALL_HELD) != 0, "empty inventory preserves an already launched warpball's reservation");
	human.heldItemID = HELD_ITEM_TURBO;
	Chaos_ApplyItemRules(&human);
	CTR_TEST_CHECK((s_tracker.gameMode1 & WARPBALL_HELD) != 0, "discarding a different item preserves another driver's warpball reservation");
	partner.heldItemID = HELD_ITEM_ROULETTE;
	partner.itemRollTimer = 30;
	Chaos_ApplyItemRules(&partner);
	CTR_TEST_CHECK_EQ(partner.heldItemID, HELD_ITEM_NONE, "No Items also empties the co-op partner's slot");
	CTR_TEST_CHECK_EQ(partner.itemRollTimer, 0, "No Items also cancels the co-op partner's roulette");

	CTR_TEST_SECTION("No Items respects scope, gates, independent lifetimes and reset without restoring discarded inventory");
	struct Driver bot = {.driverID = 2, .heldItemID = HELD_ITEM_MASK, .actionsFlagSet = ACTION_BOT | ACTION_WEAPON_FIRE_REQUEST};
	const struct Driver originalBot = bot;
	Chaos_ApplyItemRules(&bot);
	Chaos_ApplyItemRules(NULL);
	CTR_TEST_CHECK(memcmp(&bot, &originalBot, sizeof(bot)) == 0, "No Items leaves bot inventory and requests unchanged");
	human.heldItemID = HELD_ITEM_TURBO;
	human.actionsFlagSet |= ACTION_RACE_FINISHED;
	Chaos_ApplyItemRules(&human);
	CTR_TEST_CHECK_EQ(human.heldItemID, HELD_ITEM_TURBO, "finished humans keep their inventory");
	human.actionsFlagSet &= ~ACTION_RACE_FINISHED;
	s_tracker.gameMode1 |= ADVENTURE_ARENA;
	Chaos_ApplyItemRules(&human);
	CTR_TEST_CHECK_EQ(human.heldItemID, HELD_ITEM_TURBO, "Adventure hubs keep ordinary item behavior");
	s_tracker.gameMode1 &= ~ADVENTURE_ARENA;
	s_tracker.gameMode1 |= PAUSE_1;
	const struct ChaosRuntime saved = sdata->chaosRuntime;
	Tick(5000);
	Chaos_ApplyItemRules(&human);
	CTR_TEST_CHECK_EQ(human.heldItemID, HELD_ITEM_TURBO, "paused item processing cannot discard inventory");
	CTR_TEST_CHECK(memcmp(&saved, &sdata->chaosRuntime, sizeof(saved)) == 0, "pause freezes No Items lifetimes");
	s_tracker.gameMode1 &= ~PAUSE_1;
	Tick(1000);
	Chaos_Activate(CHAOS_EFFECT_FAIR_MODE, 1000);
	Tick(500);
	Chaos_ApplyItemRules(&human);
	CTR_TEST_CHECK_EQ(human.heldItemID, HELD_ITEM_NONE, "the refreshed restriction discards items until its own expiry");
	Tick(500);
	CTR_TEST_CHECK_EQ(human.heldItemID, HELD_ITEM_NONE, "final expiry does not restore discarded items");
	human.heldItemID = HELD_ITEM_ROULETTE;
	human.itemRollTimer = 90;
	Chaos_ApplyItemRules(&human);
	CTR_TEST_CHECK_EQ(human.heldItemID, HELD_ITEM_ROULETTE, "new rewards survive after final expiry");
	CTR_TEST_CHECK_EQ(human.itemRollTimer, 90, "roulette can run normally after final expiry");
	sdata->chaosRuntime = saved;
	Chaos_ApplyItemRules(&human);
	CTR_TEST_CHECK_EQ(human.heldItemID, HELD_ITEM_NONE, "resident restoration restores the restriction without an inventory snapshot");
	Chaos_Reset();
	human.heldItemID = HELD_ITEM_POTION;
	Chaos_ApplyItemRules(&human);
	CTR_TEST_CHECK_EQ(human.heldItemID, HELD_ITEM_POTION, "race reset immediately restores ordinary inventory behavior");
}

static void TickAndApplyDrivers(int elapsedMS)
{
	Tick(elapsedMS);
	struct GamepadBuffer pad = {.stickLX = 128};
	for (size_t i = 0; i < len(s_tracker.drivers); i++)
	{
		struct Driver *driver = s_tracker.drivers[i];
		if (driver != NULL)
		{
			// Retail driving physics consumes received-clock time before the Chaos hook.
			if ((s_tracker.gameMode1 & PAUSE_ALL) == 0 && driver->clockReceive > 0)
			{
				driver->clockReceive = driver->clockReceive > elapsedMS ? driver->clockReceive - elapsedMS : 0;
			}
			int steering = pad.stickLX;
			driver->actionsFlagSet = PreparedDriver(driver, &pad, &steering, driver->actionsFlagSet);
		}
	}
}

static void TestWumpaDrain(void)
{
	CTR_TEST_SECTION("Wumpa Drain loses one credited fruit per game second for unfinished humans only");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_WUMPA_TAX] = true;
	sdata->chaosSettings.enabledCount++;
	struct Driver human = {.driverID = 0, .numWumpas = 10, .numTimesWumpa = 20, .reserves = 1000};
	struct Driver partner = {.driverID = 1, .numWumpas = 7};
	struct Driver bot = {.driverID = 2, .numWumpas = 10, .actionsFlagSet = ACTION_BOT};
	s_tracker.drivers[0] = &human;
	s_tracker.drivers[1] = &partner;
	s_tracker.drivers[2] = &bot;
	human.PickupWumpaHUD.numCollected = 3;
	const u32 rng = sdata->chaosRng;
	TickAndApplyDrivers(2000);
	CTR_TEST_CHECK_EQ(human.numWumpas, 10, "inactive Wumpa Drain leaves fruit intact");
	Chaos_Activate(CHAOS_EFFECT_WUMPA_TAX, 5000);
	CTR_TEST_CHECK_EQ(human.numWumpas, 10, "activation waits for the first full second");
	TickAndApplyDrivers(999);
	CTR_TEST_CHECK_EQ(human.numWumpas, 10, "partial seconds do not drain fruit early");
	TickAndApplyDrivers(1);
	CTR_TEST_CHECK_EQ(human.numWumpas, 9, "the first complete second drains one fruit");
	CTR_TEST_CHECK_EQ(partner.numWumpas, 6, "the co-op partner uses the same pulse cadence");
	CTR_TEST_CHECK_EQ(bot.numWumpas, 10, "Wumpa Drain preserves bot fruit");
	TickAndApplyDrivers(32);
	CTR_TEST_CHECK_EQ(human.numWumpas, 9, "frames after a pulse do not repeat already counted seconds");
	TickAndApplyDrivers(2468);
	CTR_TEST_CHECK_EQ(human.numWumpas, 8, "a longer frame drains only one fruit even when it crosses several seconds");
	CTR_TEST_CHECK_EQ(partner.numWumpas, 5, "the partner also loses at most one fruit per activation per frame");
	CTR_TEST_CHECK_EQ(human.PickupWumpaHUD.numCollected, 3, "fruit still in the pickup animation remains pending");
	CTR_TEST_CHECK_EQ(human.numTimesWumpa, 20, "draining fruit does not rewrite collection statistics");
	CTR_TEST_CHECK_EQ(human.reserves, 1000, "draining fruit leaves boost reserves alone");
	human.numWumpas++;
	TickAndApplyDrivers(500);
	CTR_TEST_CHECK_EQ(human.numWumpas, 8, "the next pulse uses the current count including newly credited fruit");
	TickAndApplyDrivers(1000);
	CTR_TEST_CHECK_EQ(human.numWumpas, 8, "the expiry frame applies no additional drain");
	CTR_TEST_CHECK_EQ(partner.numWumpas, 4, "an activation removed at exact expiry cannot apply another pulse");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "the drain activation expires normally");
	TickAndApplyDrivers(2000);
	CTR_TEST_CHECK_EQ(human.numWumpas, 8, "expired Wumpa Drain cannot remove more fruit");
	Chaos_Activate(CHAOS_EFFECT_WUMPA_TAX, 500);
	TickAndApplyDrivers(2000);
	CTR_TEST_CHECK_EQ(human.numWumpas, 8, "a sub-second activation cannot drain a whole second from a longer frame");
	Chaos_Activate(CHAOS_EFFECT_WUMPA_TAX, 1500);
	TickAndApplyDrivers(3000);
	CTR_TEST_CHECK_EQ(human.numWumpas, 8, "a frame extending past expiry cannot apply a removed activation");
	human.numWumpas = 1;
	partner.numWumpas = 0;
	Chaos_Activate(CHAOS_EFFECT_WUMPA_TAX, 4000);
	TickAndApplyDrivers(3000);
	CTR_TEST_CHECK_EQ(human.numWumpas, 0, "draining the last fruit stops at zero");
	CTR_TEST_CHECK_EQ(partner.numWumpas, 0, "an empty inventory cannot underflow");
	CTR_TEST_CHECK_EQ(sdata->chaosRng, rng, "drain timing consumes no Chaos RNG samples");

	CTR_TEST_SECTION("reactivating drain resets its pulse clock and replaces its expiry");
	Chaos_Reset();
	Chaos_BeginRace(&s_tracker);
	human.numWumpas = 10;
	Chaos_Activate(CHAOS_EFFECT_WUMPA_TAX, 2500);
	TickAndApplyDrivers(750);
	Chaos_Activate(CHAOS_EFFECT_WUMPA_TAX, 2500);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 1, "reactivated drain occupies a single slot");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.active[0].elapsedMS, 0, "reactivated drain resets its pulse clock");
	TickAndApplyDrivers(250);
	CTR_TEST_CHECK_EQ(human.numWumpas, 10, "reactivation cancels the old pending pulse");
	TickAndApplyDrivers(500);
	CTR_TEST_CHECK_EQ(human.numWumpas, 10, "the replacement waits a full second before draining");
	TickAndApplyDrivers(250);
	CTR_TEST_CHECK_EQ(human.numWumpas, 9, "the replacement produces its first pulse at one second");
	TickAndApplyDrivers(250);
	CTR_TEST_CHECK_EQ(human.numWumpas, 9, "the superseded clock cannot produce a second pulse");
	TickAndApplyDrivers(500);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 1, "the old deadline cannot expire the replacement");
	TickAndApplyDrivers(250);
	CTR_TEST_CHECK_EQ(human.numWumpas, 8, "the replacement produces one second pulse");
	TickAndApplyDrivers(500);
	CTR_TEST_CHECK_EQ(human.numWumpas, 8, "replacement expiry applies no extra pulse");
	Chaos_Activate(CHAOS_EFFECT_WUMPA_TAX, 2000);
	TickAndApplyDrivers(750);
	CTR_TEST_CHECK_EQ(human.numWumpas, 8, "a new activation after expiry starts with a fresh second");
	TickAndApplyDrivers(250);
	CTR_TEST_CHECK_EQ(human.numWumpas, 7, "the fresh activation drains only after its own full second");
	Chaos_Reset();
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "reset removes the activation-owned drain clock");
	CTR_TEST_CHECK_EQ(human.numWumpas, 7, "reset does not restore already drained fruit");

	CTR_TEST_SECTION("drain cadence freezes with race gates and survives resident restoration");
	Chaos_BeginRace(&s_tracker);
	human.numWumpas = 10;
	partner.numWumpas = 10;
	partner.actionsFlagSet = ACTION_RACE_FINISHED;
	Chaos_Activate(CHAOS_EFFECT_WUMPA_TAX, 5000);
	TickAndApplyDrivers(500);
	const struct ChaosRuntime saved = sdata->chaosRuntime;
	s_tracker.gameMode1 |= PAUSE_1;
	TickAndApplyDrivers(5000);
	CTR_TEST_CHECK_EQ(human.numWumpas, 10, "pause prevents drain pulses");
	CTR_TEST_CHECK(memcmp(&saved, &sdata->chaosRuntime, sizeof(saved)) == 0, "pause freezes the partial cadence and independent timers");
	s_tracker.gameMode1 &= ~PAUSE_1;
	sdata->ptrActiveMenu = &sdata->chaosMenu.menu;
	TickAndApplyDrivers(5000);
	CTR_TEST_CHECK_EQ(human.numWumpas, 10, "menus prevent drain pulses");
	sdata->ptrActiveMenu = NULL;
	s_tracker.gameMode1 |= ADVENTURE_ARENA;
	TickAndApplyDrivers(5000);
	CTR_TEST_CHECK_EQ(human.numWumpas, 10, "Adventure hubs prevent drain pulses");
	s_tracker.gameMode1 &= ~ADVENTURE_ARENA;
	TickAndApplyDrivers(500);
	CTR_TEST_CHECK_EQ(human.numWumpas, 9, "unpausing resumes the remaining half-second");
	CTR_TEST_CHECK_EQ(partner.numWumpas, 10, "finished humans are excluded from drain pulses");
	sdata->chaosRuntime = saved;
	human.numWumpas = 10;
	TickAndApplyDrivers(499);
	CTR_TEST_CHECK_EQ(human.numWumpas, 10, "resident restoration resumes the saved partial cadence");
	TickAndApplyDrivers(1);
	CTR_TEST_CHECK_EQ(human.numWumpas, 9, "the restored cadence fires at the original boundary");

	CTR_TEST_SECTION("timed scheduling gives a new drain its full first second");
	SetupRace(CHAOS_TRIGGER_INTERVAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_FLY_ME_TO_THE_MOON] = false;
	sdata->chaosSettings.effects[CHAOS_EFFECT_WUMPA_TAX] = true;
	sdata->chaosSettings.triggerIntervalSeconds = 1;
	sdata->chaosSettings.durationSeconds = 2;
	Chaos_BeginRace(&s_tracker);
	human.numWumpas = 10;
	s_tracker.drivers[0] = &human;
	TickAndApplyDrivers(1000);
	CTR_TEST_CHECK(Chaos_IsActive(CHAOS_EFFECT_WUMPA_TAX), "Interval can schedule Wumpa Drain");
	CTR_TEST_CHECK_EQ(human.numWumpas, 10, "the scheduling frame does not retroactively consume the new drain's first second");
	TickAndApplyDrivers(1000);
	CTR_TEST_CHECK_EQ(human.numWumpas, 10, "scheduled reactivation resets the drain clock before driver application");
}

static void TestAnxiousMask(void)
{
	CTR_TEST_SECTION("Anxious Mask requests rescue on grounded off-road contact before checkpoint progress");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_ANXIOUS_MASK] = true;
	sdata->chaosSettings.enabledCount++;
	struct CheckpointNode restartPoints[2] = {0};
	struct Level level = {.ptr_restart_points = restartPoints, .cnt_restart_points = len(restartPoints)};
	s_tracker.level1 = &level;
	data.MetaDataTerrain[TERRAIN_ASPHALT].speedMultiplier = 256;
	data.MetaDataTerrain[TERRAIN_ICE].speedMultiplier = 256;
	data.MetaDataTerrain[TERRAIN_GRASS].speedMultiplier = 140;
	struct QuadBlock road = {.terrain_type = TERRAIN_ASPHALT, .checkpointIndex = 0};
	struct QuadBlock offRoad = {.terrain_type = TERRAIN_GRASS, .checkpointIndex = 1};
	struct Driver human = {
		.driverID = 0, .kartState = KS_NORMAL, .currBlockTouching = &offRoad, .lastValid = &road,
		.collisionFlags = DRIVER_COLL_FLAG_GROUNDED, .currentTerrain = TERRAIN_ICE,
		.reserves = 1000, .numWumpas = 10, .actionsFlagSet = ACTION_MASK_WEAPON,
	};
	Chaos_ApplyRescueRules(&human);
	CTR_TEST_CHECK_EQ(human.collisionFlags, DRIVER_COLL_FLAG_GROUNDED, "inactive Anxious Mask leaves off-road contact alone");
	Chaos_Activate(CHAOS_EFFECT_ANXIOUS_MASK, 10000);
	Chaos_ApplyRescueRules(&human);
	CTR_TEST_CHECK((human.collisionFlags & DRIVER_COLL_FLAG_MASK_GRAB_REQUEST) != 0, "grounded grass contact requests rescue even when a cloud overrides current terrain");
	CTR_TEST_CHECK(human.lastValid == &road, "the request preserves the last valid road checkpoint");
	CTR_TEST_CHECK_EQ(human.reserves, 1000, "the request leaves reserve loss to retail rescue initialization");
	CTR_TEST_CHECK_EQ(human.numWumpas, 10, "the request leaves fruit loss to retail rescue initialization");
	Chaos_ApplyRescueRules(&human);
	CTR_TEST_CHECK_EQ(human.collisionFlags, DRIVER_COLL_FLAG_GROUNDED | DRIVER_COLL_FLAG_MASK_GRAB_REQUEST, "an existing rescue request remains unchanged");
	human.collisionFlags = DRIVER_COLL_FLAG_GROUNDED;
	Chaos_ApplyRescueRules(&human);
	CTR_TEST_CHECK((human.collisionFlags & DRIVER_COLL_FLAG_MASK_GRAB_REQUEST) != 0, "a new eligible off-road contact can request rescue immediately");
	struct Driver partner = human;
	partner.driverID = 1;
	partner.collisionFlags = DRIVER_COLL_FLAG_GROUNDED;
	Chaos_ApplyRescueRules(&partner);
	CTR_TEST_CHECK((partner.collisionFlags & DRIVER_COLL_FLAG_MASK_GRAB_REQUEST) != 0, "the co-op partner can independently request rescue");
	Chaos_Activate(CHAOS_EFFECT_ANXIOUS_MASK, 1000);
	Chaos_ApplyRescueRules(&human);
	CTR_TEST_CHECK_EQ(human.collisionFlags, DRIVER_COLL_FLAG_GROUNDED | DRIVER_COLL_FLAG_MASK_GRAB_REQUEST, "duplicate activations leave an existing rescue request unchanged");
	const struct ChaosRuntime saved = sdata->chaosRuntime;
	s_tracker.gameMode1 |= PAUSE_1;
	Tick(5000);
	CTR_TEST_CHECK(memcmp(&saved, &sdata->chaosRuntime, sizeof(saved)) == 0, "pause freezes Anxious Mask activation timers");
	s_tracker.gameMode1 &= ~PAUSE_1;
	human.collisionFlags = DRIVER_COLL_FLAG_GROUNDED;

	CTR_TEST_SECTION("Anxious Mask uses contact terrain and leaves rescue eligibility to retail");
	const u8 roadTypes[] = {TERRAIN_ASPHALT, TERRAIN_ICE};
	for (size_t i = 0; i < len(roadTypes); i++)
	{
		road.terrain_type = roadTypes[i];
		human.currBlockTouching = &road;
		Chaos_ApplyRescueRules(&human);
		CTR_TEST_CHECK_EQ(human.collisionFlags, DRIVER_COLL_FLAG_GROUNDED, "ordinary road and ice do not trigger rescue");
	}
	road.terrain_type = TERRAIN_ASPHALT;
	human.currBlockTouching = &offRoad;
	human.collisionFlags = DRIVER_COLL_FLAG_GROUNDED;
	const u8 states[] = {KS_MASK_GRABBED, KS_BLASTED, KS_SPINNING, KS_ENGINE_REVVING, KS_WARP_PAD};
	for (size_t i = 0; i < len(states); i++)
	{
		human.kartState = states[i];
		human.collisionFlags = DRIVER_COLL_FLAG_GROUNDED;
		Chaos_ApplyRescueRules(&human);
		CTR_TEST_CHECK((human.collisionFlags & DRIVER_COLL_FLAG_MASK_GRAB_REQUEST) != 0, "kart state does not prevent an off-road rescue request");
	}
	human.kartState = KS_DRIFTING;
	human.collisionFlags = DRIVER_COLL_FLAG_GROUNDED;
	human.actionsFlagSet |= ACTION_RACE_FINISHED;
	Chaos_ApplyRescueRules(&human);
	CTR_TEST_CHECK_EQ(human.collisionFlags, DRIVER_COLL_FLAG_GROUNDED, "finished humans do not receive anxious rescues");
	human.actionsFlagSet &= ~ACTION_RACE_FINISHED;
	human.driverID = 2;
	human.actionsFlagSet |= ACTION_BOT;
	Chaos_ApplyRescueRules(&human);
	CTR_TEST_CHECK_EQ(human.collisionFlags, DRIVER_COLL_FLAG_GROUNDED, "bots do not receive anxious rescues");
	human.driverID = 0;
	human.actionsFlagSet &= ~ACTION_BOT;
	s_tracker.gameMode1 |= ADVENTURE_ARENA;
	Chaos_ApplyRescueRules(&human);
	CTR_TEST_CHECK_EQ(human.collisionFlags, DRIVER_COLL_FLAG_GROUNDED, "Adventure hubs retain ordinary off-road driving");
	s_tracker.gameMode1 &= ~ADVENTURE_ARENA;
	sdata->chaosRuntime = saved;
	Chaos_ApplyRescueRules(&human);
	CTR_TEST_CHECK((human.collisionFlags & DRIVER_COLL_FLAG_MASK_GRAB_REQUEST) != 0, "restored Anxious Mask immediately permits drifting off-road rescue");
	human.collisionFlags = DRIVER_COLL_FLAG_GROUNDED;
	Tick(10000);
	Chaos_ApplyRescueRules(&human);
	CTR_TEST_CHECK_EQ(human.collisionFlags, DRIVER_COLL_FLAG_GROUNDED, "final activation expiry restores ordinary off-road driving");
	Chaos_Reset();
	human.collisionFlags = DRIVER_COLL_FLAG_GROUNDED;
	Chaos_ApplyRescueRules(&human);
	CTR_TEST_CHECK_EQ(human.collisionFlags, DRIVER_COLL_FLAG_GROUNDED, "reset disables anxious rescue requests");
}

static void TestExplosiveCrates(void)
{
	CTR_TEST_SECTION("Explosive Crates applies one blast per contact and bypasses protections");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_UNBOXING_GONE_WRONG] = true;
	sdata->chaosSettings.enabledCount++;
	struct Model model = {.id = PU_RANDOM_CRATE};
	struct Instance crate = {.model = &model};
	struct Thread thread = {0};
	struct Instance inst = {.thread = &thread};
	struct Driver human = {.driverID = 0, .instSelf = &inst, .reserves = 1000, .heldItemID = HELD_ITEM_POTION};
	Chaos_ApplyCrateRules(&human, &crate);
	CTR_TEST_CHECK_EQ(s_crateExplosions, 0, "inactive explosive crates do not create explosions");
	Chaos_Activate(CHAOS_EFFECT_UNBOXING_GONE_WRONG, 5000);
	Chaos_Activate(CHAOS_EFFECT_UNBOXING_GONE_WRONG, 1000);
	CTR_TEST_CHECK_EQ(s_crateExplosions, 0, "activation waits for a crate contact");
	Chaos_ApplyCrateRules(&human, &crate);
	CTR_TEST_CHECK_EQ(s_crateExplosions, 1, "overlapping activations cause only one explosion per contact");
	CTR_TEST_CHECK_EQ(s_crateSounds, 1, "a contact plays one explosion sound");
	CTR_TEST_CHECK(s_lastExplodingCrate == &crate, "the visual explosion uses the contacted crate");
	CTR_TEST_CHECK(human.funcPtrs[DRIVER_FUNC_INIT] == VehStuckProc_Tumble_Init, "crate contact selects retail blast damage rather than a spin");
	CTR_TEST_CHECK_EQ(human.reserves, 0, "the blast clears boost reserves through ordinary damage handling");
	CTR_TEST_CHECK_EQ(human.heldItemID, HELD_ITEM_POTION, "explosive crates do not discard the held item");

	human.funcPtrs[DRIVER_FUNC_INIT] = NULL;
	human.actionsFlagSet |= ACTION_MASK_WEAPON;
	g_config.maskProtectsFromDamage = true;
	Chaos_ApplyCrateRules(&human, &crate);
	CTR_TEST_CHECK(human.funcPtrs[DRIVER_FUNC_INIT] == VehStuckProc_Tumble_Init, "crate blast damage bypasses a protective mask");
	CTR_TEST_CHECK((human.actionsFlagSet & ACTION_MASK_WEAPON) != 0, "the blast does not remove the mask");
	CTR_TEST_CHECK_EQ(s_crateExplosions, 2, "mask protection does not suppress the crate explosion");
	human.actionsFlagSet &= ~ACTION_MASK_WEAPON;
	human.funcPtrs[DRIVER_FUNC_INIT] = NULL;
	human.invincibleTimer = 100;
	Chaos_ApplyCrateRules(&human, &crate);
	CTR_TEST_CHECK(human.funcPtrs[DRIVER_FUNC_INIT] == VehStuckProc_Tumble_Init, "crate blast damage bypasses invincibility");
	CTR_TEST_CHECK_EQ(human.invincibleTimer, 100, "the blast preserves the invincibility timer");
	human.invincibleTimer = 0;
	human.funcPtrs[DRIVER_FUNC_INIT] = NULL;
	struct Shield shield = {0};
	struct Thread shieldThread = {.object = &shield};
	struct Instance bubble = {.thread = &shieldThread};
	human.instBubbleHold = &bubble;
	Chaos_ApplyCrateRules(&human, &crate);
	CTR_TEST_CHECK(human.funcPtrs[DRIVER_FUNC_INIT] == VehStuckProc_Tumble_Init, "crate blast damage bypasses a shield");
	CTR_TEST_CHECK_EQ(shield.flags, 0, "the blast does not request a shield pop");
	CTR_TEST_CHECK(human.instBubbleHold == &bubble, "the blast leaves the shield attached");

	CTR_TEST_SECTION("Explosive Crates respects scope, race gates and independent activation expiry");
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT};
	struct Driver finished = {.driverID = 0, .actionsFlagSet = ACTION_RACE_FINISHED};
	const int explosions = s_crateExplosions;
	Chaos_ApplyCrateRules(&bot, &crate);
	Chaos_ApplyCrateRules(&finished, &crate);
	Chaos_ApplyCrateRules(NULL, &crate);
	CTR_TEST_CHECK_EQ(s_crateExplosions, explosions, "bots, finished humans and missing drivers cannot trigger explosions");
	s_tracker.gameMode1 |= PAUSE_1;
	Chaos_ApplyCrateRules(&human, &crate);
	CTR_TEST_CHECK_EQ(s_crateExplosions, explosions, "pause disables crate explosions");
	s_tracker.gameMode1 &= ~PAUSE_1;
	s_tracker.gameMode1 |= ADVENTURE_ARENA;
	Chaos_ApplyCrateRules(&human, &crate);
	CTR_TEST_CHECK_EQ(s_crateExplosions, explosions, "Adventure hubs retain ordinary crates");
	s_tracker.gameMode1 &= ~ADVENTURE_ARENA;
	Tick(1000);
	human.invincibleTimer = 0;
	Chaos_ApplyCrateRules(&human, &crate);
	CTR_TEST_CHECK_EQ(s_crateExplosions, explosions, "the shorter replacement expires without reviving the old effect");
	Tick(4000);
	Chaos_ApplyCrateRules(&human, &crate);
	CTR_TEST_CHECK_EQ(s_crateExplosions, explosions, "the superseded expiry cannot trigger more explosions");
}

static void TestHardCrates(void)
{
	CTR_TEST_SECTION("Hard Crates routes unfinished human contacts to solid crate collision");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_UNBREAKABLE] = true;
	sdata->chaosSettings.enabledCount++;
	struct Instance crate = {.scale.x = 0x1000};
	struct ScratchpadStruct sps = {0};
	sps.Input1.modelID = DYNAMIC_PLAYER;
	struct Driver human = {.driverID = 0};
	struct Thread collidingTh = {.object = &human};
	CTR_TEST_CHECK(!Chaos_IsHardCrateForDriver(&crate, &collidingTh, &sps), "inactive effect leaves crates breakable");
	Chaos_Activate(CHAOS_EFFECT_UNBREAKABLE, 5000);
	CTR_TEST_CHECK(Chaos_IsHardCrateForDriver(&crate, &collidingTh, &sps), "active effect routes an unfinished human into solid collision");
	crate.scale.x = 0;
	CTR_TEST_CHECK(!Chaos_IsHardCrateForDriver(&crate, &collidingTh, &sps), "invisible crates cannot block the kart");
	crate.scale.x = 0x1000;
	sps.Input1.modelID = DYNAMIC_BOMB;
	CTR_TEST_CHECK(!Chaos_IsHardCrateForDriver(&crate, &collidingTh, &sps), "weapon hits keep the retail crate response");
	sps.Input1.modelID = DYNAMIC_PLAYER;
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT};
	struct Driver finished = {.driverID = 0, .actionsFlagSet = ACTION_RACE_FINISHED};
	collidingTh.object = &bot;
	CTR_TEST_CHECK(!Chaos_IsHardCrateForDriver(&crate, &collidingTh, &sps), "bots keep retail crates");
	collidingTh.object = &finished;
	CTR_TEST_CHECK(!Chaos_IsHardCrateForDriver(&crate, &collidingTh, &sps), "finished humans keep retail crates");
	collidingTh.object = NULL;
	CTR_TEST_CHECK(!Chaos_IsHardCrateForDriver(&crate, &collidingTh, &sps), "missing drivers keep retail crates");
	collidingTh.object = &human;
	s_tracker.gameMode1 |= PAUSE_1;
	CTR_TEST_CHECK(!Chaos_IsHardCrateForDriver(&crate, &collidingTh, &sps), "pause disables solid crate collisions");
	s_tracker.gameMode1 &= ~PAUSE_1;
	s_tracker.gameMode1 |= ADVENTURE_ARENA;
	CTR_TEST_CHECK(!Chaos_IsHardCrateForDriver(&crate, &collidingTh, &sps), "Adventure hubs retain ordinary crates");
	s_tracker.gameMode1 &= ~ADVENTURE_ARENA;
	Chaos_Activate(CHAOS_EFFECT_UNBREAKABLE, 1000);
	Tick(1000);
	CTR_TEST_CHECK(!Chaos_IsHardCrateForDriver(&crate, &collidingTh, &sps), "the shorter replacement restores breakable crates at its expiry");
	Tick(4000);
	CTR_TEST_CHECK(!Chaos_IsHardCrateForDriver(&crate, &collidingTh, &sps), "final expiry restores ordinary crates");
	Chaos_Reset();
	CTR_TEST_CHECK(!Chaos_IsHardCrateForDriver(&crate, &collidingTh, &sps), "race reset leaves crates breakable");
}

static void TestGiantKarts(void)
{
	CTR_TEST_SECTION("Giant Karts doubles visual/contact size and raises forward speed by 30 percent");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_BIG_KART_ENERGY] = true;
	sdata->chaosSettings.enabledCount++;
	struct Instance humanInst = {.scale = {0xccc, 0xccc, 0xccc}};
	struct Driver human = {.driverID = 0, .instSelf = &humanInst, .wheelSize = 0xccc};
	struct Driver partner = {.driverID = 1};
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT, .wheelSize = 0xccc};
	s_tracker.drivers[0] = &human;
	s_tracker.drivers[1] = &partner;
	CTR_TEST_CHECK_EQ(human.kartScale, 0, "before activation the driver uses retail scale");
	CTR_TEST_CHECK_EQ(Chaos_JumpForceForDriver(&human, 1000), 1000, "inactive effect preserves ordinary jumps");
	Chaos_Activate(CHAOS_EFFECT_BIG_KART_ENERGY, 3000);
	CTR_TEST_CHECK_EQ(human.kartScale, CHAOS_GIANT_SCALE, "activation changes the driver's scale target once");
	CTR_TEST_CHECK_EQ(partner.kartScale, CHAOS_GIANT_SCALE, "co-op partner receives the same scale target");
	CTR_TEST_CHECK_EQ(humanInst.scale.x, 0xccc, "activation leaves the current scale for retail interpolation");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 10000), 13000, "forward target increases by 30 percent");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, -2000), -2000, "reverse speed stays retail");
	human.const_Speed_ClassStat = 20000;
	sdata->chaosSettings.effects[CHAOS_EFFECT_SUNDAY_DRIVER] = true;
	sdata->chaosSettings.enabledCount++;
	Chaos_Activate(CHAOS_EFFECT_SUNDAY_DRIVER, 1000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 10000), 9100, "Sunday Driver scales the giant's 130-percent forward target to 70 percent");
	CTR_TEST_CHECK_EQ(bot.kartScale, 0, "bot keeps the retail scale");
	CTR_TEST_CHECK_EQ(Chaos_JumpForceForDriver(&human, 1000), 1300, "normal, forced and spring jumps gain 30 percent impulse");
	CTR_TEST_CHECK_EQ(Chaos_JumpForceForDriver(&bot, 1000), 1000, "bots retain normal jump force");
	struct Thread botThread = {0};
	struct Instance botInst = {.thread = &botThread};
	bot.instSelf = &botInst;
	CTR_TEST_CHECK(VehPhysCrash_TrySquashSmallerKart(&human, &bot), "a successful squash skips the blocking kart collision");
	CTR_TEST_CHECK(bot.squishTimer > 0 && bot.funcPtrs[DRIVER_FUNC_INIT] == VehPhysProc_SpinFirst_Init,
	               "a giant squashes the smaller kart through retail damage");
	CTR_TEST_CHECK_EQ(human.forcedJumpType, FORCED_JUMP_HIGH, "the giant gets the Turbo-item hop over its victim");
	CTR_TEST_CHECK_EQ(s_squishSounds, 1, "squash plays its retail sound once");
	const int squishTimer = bot.squishTimer;
	human.forcedJumpType = FORCED_JUMP_NONE;
	CTR_TEST_CHECK(VehPhysCrash_TrySquashSmallerKart(&bot, &human), "a squished kart cannot block the giant on repeated contact");
	CTR_TEST_CHECK_EQ(bot.squishTimer, squishTimer, "continued contact does not restart an existing squash");
	CTR_TEST_CHECK_EQ(human.forcedJumpType, FORCED_JUMP_NONE, "continued contact does not queue repeated forced hops");
	bot.squishTimer = 0;
	bot.invincibleTimer = 100;
	bot.funcPtrs[DRIVER_FUNC_INIT] = NULL;
	CTR_TEST_CHECK(!VehPhysCrash_TrySquashSmallerKart(&human, &bot), "a protected smaller kart keeps ordinary collision response");
	CTR_TEST_CHECK_EQ(bot.squishTimer, 0, "retail invincibility protects the smaller kart");
	bot.invincibleTimer = 0;
	CTR_TEST_CHECK(!VehPhysCrash_TrySquashSmallerKart(&human, &partner), "equally giant humans keep ordinary kart collision");
	s_tracker.gameMode1 |= PAUSE_1;
	Tick(1000);
	CTR_TEST_CHECK_EQ(human.kartScale, CHAOS_GIANT_SCALE, "pause freezes the enlarged target");
	s_tracker.gameMode1 &= ~PAUSE_1;
	Chaos_Activate(CHAOS_EFFECT_BIG_KART_ENERGY, 1000);
	CTR_TEST_CHECK_EQ(human.kartScale, CHAOS_GIANT_SCALE, "duplicate activation does not compound the scale target");
	human.actionsFlagSet |= ACTION_RACE_FINISHED;
	Tick(1000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_BIG_KART_ENERGY), "the replacement expiry deactivates Giant Karts without a surviving copy");
	CTR_TEST_CHECK_EQ(human.kartScale, 1, "deactivating Giant Karts restores the scale even for a finished human");
	human.actionsFlagSet &= ~ACTION_RACE_FINISHED;
	Tick(2000);
	CTR_TEST_CHECK_EQ(human.kartScale, 1, "final expiry restores the normal target");
	CTR_TEST_CHECK_EQ(partner.kartScale, 1, "final expiry restores the co-op partner's target");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 10000), 10000, "expiry restores speed");
	CTR_TEST_CHECK_EQ(Chaos_JumpForceForDriver(&human, 1000), 1000, "expiry restores jump force");
	Chaos_Activate(CHAOS_EFFECT_BIG_KART_ENERGY, 1000);
	CTR_TEST_CHECK_EQ(human.kartScale, CHAOS_GIANT_SCALE, "a new activation changes the target again");
	Chaos_Reset();
	CTR_TEST_CHECK_EQ(human.kartScale, 1, "race reset restores the normal target");
	CTR_TEST_CHECK_EQ(partner.kartScale, 1, "race reset also restores the partner's target");

	CTR_TEST_SECTION("Scale and squash use driver dimensions independently of Chaos");
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&bot, 25), 25, "zero-initialized scale keeps retail collision size");
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, 0xccc), 0xccc, "explicit normal scale keeps the retail target");
	human.kartScale = 3;
	bot.kartScale = 2;
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, 25), 75, "collision size follows arbitrary driver scale");
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, human.wheelSize), 0x2664, "wheel size uses the same scale as the body");
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, 0), 0, "squish preserves zero vertical scale");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_BIG_KART_ENERGY), "no Chaos activation is needed for dimension-based collision");
	CTR_TEST_CHECK(VehPhysCrash_TrySquashSmallerKart(&bot, &human), "a larger kart squashes a smaller enlarged kart without Chaos");
	human.actionsFlagSet |= ACTION_RACE_FINISHED;
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, 25), 25, "finished drivers use retail dimensions");
	bot.squishTimer = 0;
	bot.kartScale = 1;
	CTR_TEST_CHECK(!VehPhysCrash_TrySquashSmallerKart(&human, &bot), "a finished enlarged driver cannot squash an equally sized kart");
}

static void TestSmallKartEnergy(void)
{
	CTR_TEST_SECTION("Small Kart Energy halves dimensions and uses normal size-based squash protection");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_SMALL_KART_ENERGY] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_BIG_KART_ENERGY] = true;
	sdata->chaosSettings.enabledCount += 2;
	struct Thread humanThread = {0};
	struct Instance humanInst = {.thread = &humanThread, .scale = {0xccc, 0xccc, 0xccc}};
	struct Driver human = {.driverID = 0, .instSelf = &humanInst, .wheelSize = 0xccc};
	struct Driver partner = {.driverID = 1};
	struct Thread botThread = {0};
	struct Instance botInst = {.thread = &botThread};
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT, .instSelf = &botInst};
	s_tracker.drivers[0] = &human;
	s_tracker.drivers[1] = &partner;
	s_tracker.drivers[2] = &bot;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_SMALL_KART_ENERGY, 3000), "Small Kart Energy activates as a timed effect");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 1, "the small size occupies a timed slot");
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, 0xccc), 0x666, "body dimensions shrink to half size");
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, human.wheelSize), 0x666, "tires shrink with the body");
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, 25), 12, "odd collision radii round toward zero at half size");
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, -25), -12, "signed scale offsets retain their direction");
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, 0), 0, "squashed zero height remains zero");
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&partner, 100), 50, "the second human also shrinks");
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&bot, 100), 100, "bots keep normal dimensions");
	CTR_TEST_CHECK_EQ(humanInst.scale.x, 0xccc, "activation preserves the existing scale interpolation");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 10000), 7000, "forward speed targets fall to seventy percent");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, -2000), -2000, "reverse speed follows the existing size-effect convention");
	CTR_TEST_CHECK_EQ(Chaos_JumpForceForDriver(&human, 4596), 3217, "jump force falls to seventy percent without altering constants");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&bot, 10000), 10000, "bots keep normal speed targets");
	CTR_TEST_CHECK_EQ(Chaos_JumpForceForDriver(&bot, 4596), 4596, "bots keep normal jump force");
	CTR_TEST_CHECK(!VehPhysCrash_TrySquashSmallerKart(&human, &partner), "equally small humans do not squash each other");
	g_config.maskProtectsFromDamage = true;
	human.actionsFlagSet |= ACTION_MASK_WEAPON;
	CTR_TEST_CHECK(!VehPhysCrash_TrySquashSmallerKart(&bot, &human), "a protective mask prevents squash by a normal bot");
	human.actionsFlagSet &= ~ACTION_MASK_WEAPON;
	struct Shield shieldObject = {0};
	struct Thread shieldThread = {.object = &shieldObject};
	struct Instance shield = {.thread = &shieldThread};
	human.instBubbleHold = &shield;
	CTR_TEST_CHECK(!VehPhysCrash_TrySquashSmallerKart(&human, &bot), "a shield absorbs the squash regardless of contact order");
	CTR_TEST_CHECK((shieldObject.flags & SHIELD_FLAG_POP_ON_DAMAGE) != 0 && human.instBubbleHold == NULL,
	               "shield protection uses normal shield consumption");
	CTR_TEST_CHECK(!VehPhysCrash_TrySquashSmallerKart(&bot, &human), "shield-granted immunity prevents an immediate repeat squash");
	human.invincibleTimer = 0;
	CTR_TEST_CHECK(VehPhysCrash_TrySquashSmallerKart(&human, &bot), "a normal bot can squash an unprotected half-sized human");
	CTR_TEST_CHECK_EQ(human.squishTimer, VEH_PICK_SQUISH_TIMER, "small karts use the existing squash duration");
	const int timer = human.squishTimer;
	bot.forcedJumpType = FORCED_JUMP_NONE;
	CTR_TEST_CHECK(VehPhysCrash_TrySquashSmallerKart(&bot, &human), "continued contact passes over an already squashed small kart");
	CTR_TEST_CHECK(human.squishTimer == timer && bot.forcedJumpType == FORCED_JUMP_NONE,
	               "continued contact does not renew damage or hop repeatedly");
	Chaos_Activate(CHAOS_EFFECT_BIG_KART_ENERGY, 3000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_SMALL_KART_ENERGY), "Giant Karts removes Small Kart Energy");
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, 100), 200, "replacement grows directly to the giant target");
	Chaos_Activate(CHAOS_EFFECT_SMALL_KART_ENERGY, 3000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_BIG_KART_ENERGY), "Small Kart Energy removes Giant Karts");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 10000), 7000, "replacement does not retain the giant speed multiplier");
	s_tracker.gameMode1 |= PAUSE_1;
	Tick(3000);
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, 100), 50, "pause freezes the small-size duration");
	s_tracker.gameMode1 &= ~PAUSE_1;
	Chaos_Activate(CHAOS_EFFECT_SMALL_KART_ENERGY, 1000);
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, 100), 50, "repeat activation does not compound shrinkage");
	human.actionsFlagSet |= ACTION_RACE_FINISHED;
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, 100), 100, "finished humans return to retail dimensions");
	Tick(1000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_SMALL_KART_ENERGY), "Small Kart Energy expires normally");
	CTR_TEST_CHECK_EQ(human.kartScale, 1, "expiry clears the small target even after finishing");
	human.actionsFlagSet &= ~ACTION_RACE_FINISHED;
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&partner, 100), 100, "expiry restores the partner's dimensions");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&human, 10000), 10000, "expiry restores speed targets");
	CTR_TEST_CHECK_EQ(Chaos_JumpForceForDriver(&human, 4596), 4596, "expiry restores jump force");
	Chaos_Activate(CHAOS_EFFECT_SMALL_KART_ENERGY, 1000);
	Chaos_Reset();
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, 100), 100, "race reset restores normal size");
	human.squishTimer = 0;
	human.kartScale = 1.5f;
	CTR_TEST_CHECK(VehPhysCrash_TrySquashSmallerKart(&human, &bot), "one-and-a-half size remains distinguishable from normal size during squash comparison");
	human.kartScale = 0.75f;
	CTR_TEST_CHECK_EQ(VehPhysForce_ScaleValue(&human, 100), 75, "arbitrary fractional scales are not restricted to half-units");
}

static void TestKartParticleScale(void)
{
	CTR_TEST_SECTION("Kart particle scaling preserves color, rotation, timing and oscillator ownership");
	struct Driver driver = {.kartScale = 0.5f};
	MATRIX matrix = {.m = {{4096, -2048, 0}, {0, 4096, 0}, {0, 0, 4096}}, .t = {100, 200, 300}};
	VehPhysForce_ScaleMatrix(&driver, &matrix);
	CTR_TEST_CHECK(matrix.m[0][0] == 2048 && matrix.m[0][1] == -1024 && matrix.m[2][2] == 2048,
	               "attached effects scale their complete rotated geometry");
	CTR_TEST_CHECK(matrix.t[0] == 100 && matrix.t[1] == 200 && matrix.t[2] == 300,
	               "scaling an attached effect does not scale its world-space position");
	struct ParticleOscillator color = {.scale = 400, .offset = 200, .period = 12};
	struct ParticleOscillator size = {.next = &color, .previousValue = 100, .scale = 200, .offset = 400, .min = -100, .max = 500, .period = 16, .phase = 8};
	struct Particle particle = {
		.oscillator = &size, .framesLeftInLife = 30,
		.flagsOscillatorAxis = PARTICLE_AXIS_FLAG_SCALE_X | PARTICLE_AXIS_FLAG_COLOR_R,
		.axis = {
			[PARTICLE_AXIS_POS_X] = {.startVal = 100, .velocity = -20, .accel = 4},
			[PARTICLE_AXIS_ROT_X_OR_LINE_PREV_X] = {.startVal = 200, .velocity = 10},
			[PARTICLE_AXIS_ROT_Y_OR_LINE_PREV_Z] = {.startVal = 300},
			[PARTICLE_AXIS_SCALE_X_OR_LINE_SCALE] = {.startVal = 1536, .velocity = 400, .accel = -8},
			[PARTICLE_AXIS_SCALE_Y_OR_LINE_PREV_Y] = {.startVal = 1024},
			[PARTICLE_AXIS_COLOR_R] = {.startVal = 16384, .velocity = -2560},
		},
	};
	const struct Particle original = particle;
	VehPhysForce_ScaleParticle(&driver, &particle);
	CTR_TEST_CHECK(particle.axis[0].startVal == 50 && particle.axis[0].velocity == -10 && particle.axis[0].accel == 2,
	               "particle spread and motion shrink together");
	CTR_TEST_CHECK(particle.axis[5].startVal == 768 && particle.axis[5].velocity == 200 && particle.axis[5].accel == -4 && particle.axis[6].startVal == 512,
	               "particle size and its growth rate shrink together");
	CTR_TEST_CHECK(memcmp(&particle.axis[3], &original.axis[3], 2 * sizeof(struct ParticleAxis)) == 0,
	               "sprite angles and angular motion keep their original values");
	CTR_TEST_CHECK(memcmp(&particle.axis[7], &original.axis[7], 4 * sizeof(struct ParticleAxis)) == 0 && particle.framesLeftInLife == 30,
	               "particle color, fade, icon animation and lifetime stay unchanged");
	CTR_TEST_CHECK(size.scale == 100 && size.offset == 200 && size.previousValue == 50 && size.min == -50 && size.max == 250,
	               "size oscillator values follow the same scale as their axis");
	CTR_TEST_CHECK(size.period == 16 && size.phase == 8 && size.next == &color && color.scale == 400 && color.offset == 200,
	               "oscillator timing, links and color oscillation remain intact");
	particle.flagsSetColor = PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE;
	VehPhysForce_ScaleParticle(&driver, &particle);
	CTR_TEST_CHECK(particle.axis[3].startVal == 100 && particle.axis[4].startVal == 150,
	               "line particles scale their previous endpoint rather than treating it as rotation");
	driver.kartScale = 2.0f;
	particle = original;
	particle.oscillator = NULL;
	VehPhysForce_ScaleParticle(&driver, &particle);
	CTR_TEST_CHECK(particle.axis[0].startVal == 200 && particle.axis[5].startVal == 3072 && particle.axis[5].velocity == 800,
	               "giant karts double particle dimensions and growth");
	driver.actionsFlagSet = ACTION_RACE_FINISHED;
	particle = original;
	particle.oscillator = NULL;
	const struct Particle finished = particle;
	VehPhysForce_ScaleParticle(&driver, &particle);
	CTR_TEST_CHECK(memcmp(&particle, &finished, sizeof(particle)) == 0, "finished drivers retain retail particles");
}

static void TestFreshStart(void)
{
	CTR_TEST_SECTION("Fresh Start replays a human-only start without resetting the race clock");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	struct Instance instances[2] = {0};
	struct Thread threads[2] = {0};
	struct Icon icon = {0};
	icon.texLayout.u1 = 24;
	icon.texLayout.v2 = 24;
	struct DB db = {0};
	s_tracker.backBuffer = &db;
	s_tracker.trafficLightsTimer = DOT_LIGHT_TIMER_SHOW_MIN - 1;
	for (int i = 0; i < 4; i++)
	{
		s_tracker.trafficLightIcon[i] = &icon;
	}
	for (int i = 0; i < 2; i++)
	{
		struct Driver *driver = s_tracker.drivers[i];
		driver->instSelf = &instances[i];
		s_tracker.cameraDC[i].driverToFollow = driver;
		instances[i].thread = &threads[i];
		driver->posCurr = (Vec3){100, 100000, 200};
		driver->velocity = (Vec3){100, 200, 300};
		driver->quadBlockHeight = -500;
		driver->reserves = 10000;
		driver->turbo_outsideTimer = 300;
		driver->jump_ForcedMS = 500;
		driver->actionsFlagSet = ACTION_AIRBORNE | ACTION_HIGH_JUMP;
		driver->heldItemID = HELD_ITEM_BOMB_3X;
		driver->numHeldItems = 3;
		driver->turnAngleCurr = 300;
		driver->simpTurnState = 40;
		driver->angle = 0x100;
		driver->rotCurr.y = 0x500;
		driver->rotCurr.w = 100;
		driver->turnWobbleAngle = 50;
	}
	s_tracker.drivers[1]->actionsFlagSet = ACTION_TOUCH_GROUND;
	struct Driver bot = {.driverID = 2, .speed = 9000, .reserves = 1000};
	s_tracker.drivers[2] = &bot;
	sdata->chaosSettings.effects[CHAOS_EFFECT_FRESH_START] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_TOO_BOOSTED_TO_FAIL] = true;
	sdata->chaosSettings.enabledCount += 2;
	Chaos_Activate(CHAOS_EFFECT_TOO_BOOSTED_TO_FAIL, 30000);
	s_restartInitializations = 0;
	s_restartFallPoses = 0;
	s_startLightDraws = 0;
	s_startCountdownSounds = 0;
	s_startGreenSounds = 0;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_FRESH_START, 1), "restart duration is the retail countdown rather than the requested Chaos duration");
	CTR_TEST_CHECK_EQ(s_restartInitializations, 2, "both unfinished humans enter the retail starting state once");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 1, "restart consumes no timed slot");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numInstantNotices, 1, "restart uses an instant notice");
	for (int i = 0; i < 2; i++)
	{
		struct Driver *driver = s_tracker.drivers[i];
		CTR_TEST_CHECK_EQ(driver->posCurr.y, 100000, "restart preserves the kart's height instead of teleporting");
		CTR_TEST_CHECK(driver->posCurr.x == 100 && driver->posCurr.z == 200 && driver->posPrev.y == driver->posCurr.y, "restart preserves horizontal progress without artificial displacement");
		CTR_TEST_CHECK(driver->velocity.x == 0 && driver->velocity.y == 0 && driver->velocity.z == 0, "restart clears motion on every axis");
		CTR_TEST_CHECK(driver->reserves == 0 && driver->turbo_outsideTimer == 0, "the explicit restart clears fire even during reserve protection");
		CTR_TEST_CHECK_EQ(driver->jump_ForcedMS, 0, "old jump state does not relaunch the kart");
		CTR_TEST_CHECK(driver->turnAngleCurr == 0 && driver->simpTurnState == 0, "restart clears the previous steering pose");
		CTR_TEST_CHECK(driver->angle == 0x500 && driver->rotCurr.y == 0x500 && driver->rotCurr.w == 0 && driver->turnWobbleAngle == 0,
		    "neutral pose synchronizes physical, visual and camera heading before the countdown");
		CTR_TEST_CHECK(driver->heldItemID == HELD_ITEM_BOMB_3X && driver->numHeldItems == 3, "restart preserves held inventory");
		CTR_TEST_CHECK_EQ(Chaos_StartTimerForDriver(driver), DOT_LIGHT_TIMER_HIDE, "the revving update waits for the full local countdown");
	}
	CTR_TEST_CHECK(sdata->chaosRuntime.restart[0].floating && sdata->chaosRuntime.restart[0].mask != NULL, "an airborne kart is held by its own rescue mask");
	CTR_TEST_CHECK_EQ(s_restartFallPoses, 1, "the suspended kart reuses the retail rescue falling pose");
	CTR_TEST_CHECK(!sdata->chaosRuntime.restart[1].floating && instances[1].animIndex == 0 && instances[1].animFrame == 20, "grounded karts return directly to the neutral forward-facing frame");
	for (int i = 0; i < 2; i++)
	{
		CTR_TEST_CHECK(Chaos_ConsumeCameraBlend(&s_tracker.cameraDC[i]), "each human's camera starts the existing recovery blend at activation");
		CTR_TEST_CHECK(!Chaos_ConsumeCameraBlend(&s_tracker.cameraDC[i]), "the camera blend is not restarted on every countdown frame");
	}
	struct CameraDC botCamera = {.driverToFollow = &bot};
	CTR_TEST_CHECK(!Chaos_ConsumeCameraBlend(&botCamera), "bot cameras do not consume a human restart transition");
	CTR_TEST_CHECK_EQ(Chaos_StartTimerForDriver(&bot), DOT_LIGHT_TIMER_SHOW_MIN - 1, "bots continue to see the completed global countdown");
	CTR_TEST_CHECK(bot.speed == 9000 && bot.reserves == 1000, "restart does not stop or drain bots");
	s_tracker.gameMode1 |= PAUSE_ALL;
	Tick(960);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.restart[0].timer, DOT_LIGHT_TIMER_HIDE, "pause freezes the local countdown");
	s_tracker.gameMode1 &= ~PAUSE_ALL;
	for (int i = 0; i < 4; i++)
	{
		Tick(960);
		Chaos_DrawStartLights(&s_tracker);
		Chaos_DrawStartLights(&s_tracker);
	}
	CTR_TEST_CHECK_EQ(s_startCountdownSounds, 3, "the three red lights sound once each across both viewports");
	CTR_TEST_CHECK_EQ(s_startGreenSounds, 1, "green sounds once even if the HUD is drawn again");
	CTR_TEST_CHECK_EQ(s_startLightDraws, 64, "the original four-light display renders independently in both human viewports");
	CTR_TEST_CHECK_EQ(Chaos_StartTimerForDriver(s_tracker.drivers[0]), 0, "green lets the normal revving update release the kart and grant its earned boost");
	CTR_TEST_CHECK_EQ(s_tracker.trafficLightsTimer, DOT_LIGHT_TIMER_SHOW_MIN - 1, "local lights never rewind the global race countdown");
	CTR_TEST_CHECK(!sdata->chaosRuntime.restart[0].floating && s_restartMasks[0].duration == 0, "green ends the suspension and fades its temporary mask");
	CTR_TEST_CHECK(s_tracker.drivers[0]->angle == 0x500 && s_tracker.drivers[0]->rotCurr.y == 0x500 &&
	    s_tracker.drivers[1]->angle == 0x500 && s_tracker.drivers[1]->rotCurr.y == 0x500,
	    "green preserves the held heading instead of returning to the old pre-drift direction");
	Tick(960);
	CTR_TEST_CHECK(!sdata->chaosRuntime.restart[0].active && !sdata->chaosRuntime.restart[1].active, "the green-light exit animation ends naturally");
	Chaos_Activate(CHAOS_EFFECT_FRESH_START, 1000);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.restart[0].timer, DOT_LIGHT_TIMER_HIDE, "repeated activation starts a fresh countdown");
	s_tracker.cameraDC[0].cameraMode = CAMERA_MODE_FREECAM;
	CTR_TEST_CHECK(!Chaos_ConsumeCameraBlend(&s_tracker.cameraDC[0]), "a non-follow camera is not redirected by the restart blend");
	s_tracker.cameraDC[0].cameraMode = 0;
	CTR_TEST_CHECK(Chaos_ConsumeCameraBlend(&s_tracker.cameraDC[0]), "repeated activation requests a fresh blend when driver follow resumes");
	s_tracker.drivers[1]->actionsFlagSet |= ACTION_RACE_FINISHED;
	Chaos_Activate(CHAOS_EFFECT_FRESH_START, 1000);
	Tick(1);
	CTR_TEST_CHECK(sdata->chaosRuntime.restart[0].active && !sdata->chaosRuntime.restart[1].active, "finished humans are excluded from the restarted sequence");
	s_startLightDraws = 0;
	Chaos_DrawStartLights(&s_tracker);
	CTR_TEST_CHECK_EQ(s_startLightDraws, 4, "only the unfinished human viewport receives restart lights");
	s_tracker.drivers[0]->kartState = KS_MASK_GRABBED;
	Chaos_Activate(CHAOS_EFFECT_FRESH_START, 1000);
	Tick(960);
	CTR_TEST_CHECK(sdata->chaosRuntime.restart[0].pending && !sdata->chaosRuntime.restart[0].active, "an ongoing rescue delays both the hold and its traffic lights");
	CTR_TEST_CHECK(!Chaos_ConsumeCameraBlend(&s_tracker.cameraDC[0]), "pending restart does not disturb the camera during an ongoing rescue");
	s_tracker.drivers[0]->kartState = KS_ENGINE_REVVING;
	s_tracker.drivers[0]->KartStates.RevEngine.boolMaskGrab = true;
	Tick(960);
	CTR_TEST_CHECK(sdata->chaosRuntime.restart[0].pending, "the descending part of a rescue also finishes before the countdown starts");
	s_tracker.drivers[0]->kartState = KS_NORMAL;
	Tick(1);
	CTR_TEST_CHECK(sdata->chaosRuntime.restart[0].active && sdata->chaosRuntime.restart[0].timer == DOT_LIGHT_TIMER_HIDE, "rescue completion starts a full independent countdown");
	CTR_TEST_CHECK(Chaos_ConsumeCameraBlend(&s_tracker.cameraDC[0]), "the deferred restart blends only after rescue completion");
	Chaos_Reset();
	CTR_TEST_CHECK_EQ(Chaos_StartTimerForDriver(s_tracker.drivers[0]), DOT_LIGHT_TIMER_SHOW_MIN - 1, "reset releases local start gating");
	s_tracker.trafficLightsTimer = DOT_LIGHT_TIMER_HIDE;
	s_tracker.gameMode1 = ADVENTURE_ARENA;
	CTR_TEST_CHECK(!Chaos_IsWaitingForStart(s_tracker.drivers[0]), "the adventure hub never waits on a positive global start timer without a local restart");
	s_tracker.drivers[2] = NULL;
}

static void TestYouveGotMail(void)
{
	CTR_TEST_SECTION("You've Got Mail launches once from the last bot at the leading human");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_YOUVE_GOT_MAIL] = true;
	sdata->chaosSettings.enabledCount++;
	s_mailLaunches = 0;
	struct Driver firstBot = {.driverID = 2, .actionsFlagSet = ACTION_BOT};
	struct Driver lastBot = {.driverID = 7, .actionsFlagSet = ACTION_BOT};
	struct Driver finishedBot = {.driverID = 6, .actionsFlagSet = ACTION_BOT | ACTION_RACE_FINISHED};
	s_tracker.driversInRaceOrder[0] = &firstBot;
	s_tracker.driversInRaceOrder[1] = s_tracker.drivers[1];
	s_tracker.driversInRaceOrder[2] = s_tracker.drivers[0];
	s_tracker.driversInRaceOrder[3] = &lastBot;
	s_tracker.driversInRaceOrder[4] = &finishedBot;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_YOUVE_GOT_MAIL, 1000), "You've Got Mail activates as an instant effect");
	CTR_TEST_CHECK_EQ(s_mailLaunches, 1, "one activation launches exactly one orb");
	CTR_TEST_CHECK(s_mailSource == &lastBot && s_mailTarget == s_tracker.drivers[1], "the last unfinished bot targets the highest-ranked human");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "mail consumes no timed slot");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numInstantNotices, 1, "mail announces through the instant banner");
	s_tracker.driversInRaceOrder[3] = NULL;
	s_tracker.drivers[1]->actionsFlagSet |= ACTION_RACE_FINISHED;
	s_tracker.drivers[0]->actionsFlagSet |= ACTION_BOT;
	Chaos_Activate(CHAOS_EFFECT_YOUVE_GOT_MAIL, 1000);
	CTR_TEST_CHECK(s_mailSource == &firstBot && s_mailTarget == s_tracker.drivers[0], "a source ahead of the target and a human on autopilot remain eligible");
	firstBot.actionsFlagSet |= ACTION_RACE_FINISHED;
	Chaos_Activate(CHAOS_EFFECT_YOUVE_GOT_MAIL, 1000);
	CTR_TEST_CHECK_EQ(s_mailLaunches, 2, "mail without an unfinished bot performs no launch");
	firstBot.actionsFlagSet &= ~ACTION_RACE_FINISHED;
	s_tracker.drivers[0]->actionsFlagSet |= ACTION_RACE_FINISHED;
	Chaos_Activate(CHAOS_EFFECT_YOUVE_GOT_MAIL, 1000);
	CTR_TEST_CHECK_EQ(s_mailLaunches, 2, "mail without an unfinished human performs no launch");
	memset(s_tracker.driversInRaceOrder, 0, sizeof(s_tracker.driversInRaceOrder));
	Chaos_Reset();
}

static void TestArenaChampions(void)
{
	CTR_TEST_SECTION("Arena Champions scales bot motion to 175 percent without compounding AI state");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_ARENA_CHAMPIONS] = true;
	sdata->chaosSettings.enabledCount++;
	Chaos_PublishEffects();
	CTR_TEST_CHECK(strstr(s_externalCatalogue, "arena-champions\n") != NULL, "Arena Champions is published even in a botless race");
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_ARENA_CHAMPIONS, 1000), "Arena Champions activates without a bot-availability requirement");
	Chaos_BeginRace(&s_tracker);
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT};
	struct Driver secondBot = {.driverID = 7, .actionsFlagSet = ACTION_BOT};
	s_tracker.drivers[2] = &bot;
	s_tracker.drivers[7] = &secondBot;
	struct Driver *human = s_tracker.drivers[0];
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&bot, 9000), 9000, "inactive Arena Champions preserves retail bot motion");
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_ARENA_CHAMPIONS, 1000), "Arena Champions activates as a timed effect when bots are present");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 1, "Arena Champions occupies a timed slot");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numInstantNotices, 0, "Arena Champions is not an instant action");
	CTR_TEST_CHECK(Chaos_IsActiveForDriver(CHAOS_EFFECT_ARENA_CHAMPIONS, &bot) &&
	               Chaos_IsActiveForDriver(CHAOS_EFFECT_ARENA_CHAMPIONS, &secondBot), "all unfinished bot slots receive the effect");
	CTR_TEST_CHECK(!Chaos_IsActiveForDriver(CHAOS_EFFECT_ARENA_CHAMPIONS, human), "human slots are excluded");
	const int nominalSpeeds[] = {0, 500, 1000, 9000, 16000, 24000, 25600};
	int previousActualSpeed = 0;
	int previousNominalSpeed = 0;
	for (int i = 0; i < (int)len(nominalSpeeds); i++)
	{
		bot.botData.aiPhysics.speedLinear = nominalSpeeds[i];
		const int actualSpeed = Chaos_SpeedForDriver(&bot, bot.botData.aiPhysics.speedLinear);
		CTR_TEST_CHECK_EQ(actualSpeed, nominalSpeeds[i] * 175 / 100, "bot motion scales to 175 percent through the full retail speed range, including boosted speeds");
		CTR_TEST_CHECK_EQ(actualSpeed - previousActualSpeed, (nominalSpeeds[i] - previousNominalSpeed) * 175 / 100,
		                  "the same AI acceleration produces 175 percent of the change in physical speed");
		CTR_TEST_CHECK_EQ(bot.botData.aiPhysics.speedLinear, nominalSpeeds[i], "motion scaling leaves persistent AI speed in retail units");
		CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&bot, bot.botData.aiPhysics.speedLinear), actualSpeed,
		                  "repeated motion queries never compound the speed multiplier");
		previousActualSpeed = actualSpeed;
		previousNominalSpeed = nominalSpeeds[i];
	}
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&bot, 501), 876, "fractional bot speeds use the full 175-percent multiplier before truncation");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(human, 9000), 9000, "human driving speed remains unchanged");
	sdata->chaosSettings.effects[CHAOS_EFFECT_GOTTA_GO_FAST] = true;
	sdata->chaosSettings.enabledCount++;
	Chaos_Activate(CHAOS_EFFECT_GOTTA_GO_FAST, 1000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(human, 9000), 18000, "human Super Speed can coexist with bot Arena Champions");
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&bot, 9000), 15750, "human Gotta Go Fast leaves the bot's independent 175-percent multiplier intact");
	human->actionsFlagSet |= ACTION_BOT;
	CTR_TEST_CHECK(!Chaos_IsActiveForDriver(CHAOS_EFFECT_ARENA_CHAMPIONS, human), "a human-owned slot on autopilot does not become an Arena Champions target");
	human->actionsFlagSet &= ~ACTION_BOT;
	secondBot.actionsFlagSet |= ACTION_RACE_FINISHED;
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&secondBot, 9000), 9000, "finished bots return to retail motion immediately");
	Tick(1000);
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&bot, 9000), 9000, "expiry restores retail motion without undoing a modified constant");
	Chaos_Activate(CHAOS_EFFECT_ARENA_CHAMPIONS, 1000);
	Chaos_Reset();
	CTR_TEST_CHECK_EQ(Chaos_SpeedForDriver(&bot, 9000), 9000, "race teardown clears the bot multiplier");
}

static void TestClassicGTA(void)
{
	CTR_TEST_SECTION("Classic GTA provides per-human top-down views and viewport-local cutaways");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_CLASSIC_GTA] = true;
	sdata->chaosSettings.enabledCount++;
	struct Driver human = {.driverID = 0, .posCurr = {100 << 8, 200 << 8, 300 << 8}, .angle = 0x100};
	struct Driver partner = {.driverID = 1, .posCurr = {3000 << 8, 400 << 8, 5000 << 8}, .angle = 0x700};
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT};
	s_tracker.drivers[0] = &human;
	s_tracker.drivers[1] = &partner;
	struct CameraDC *camera = &s_tracker.cameraDC[0];
	struct CameraDC *secondCamera = &s_tracker.cameraDC[1];
	struct PushBuffer *view = &s_tracker.pushBuffer[0];
	struct PushBuffer *secondView = &s_tracker.pushBuffer[1];
	*camera = (struct CameraDC){.driverToFollow = &human, .pushBuffer = view, .nearOrFar = 1, .cameraMode = 0};
	*secondCamera = (struct CameraDC){.driverToFollow = &partner, .pushBuffer = secondView};
	view->distanceToScreen_CURR = 256;
	view->pos = (SVec3){10, 20, 30};
	CTR_TEST_CHECK(!Chaos_ApplyCameraRules(camera), "inactive Classic GTA leaves the retail camera alone");
	CTR_TEST_CHECK_EQ(view->pos.y, 20, "the inactive hook does not move the camera");
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_CLASSIC_GTA, 3000), "Classic GTA activates as a timed effect");
	CTR_TEST_CHECK(Chaos_ApplyCameraRules(camera) && Chaos_ApplyCameraRules(secondCamera), "both human viewports receive the override");
	CTR_TEST_CHECK(view->pos.x == 100 && view->pos.y == 200 + CHAOS_TOP_DOWN_HEIGHT && view->pos.z == 300,
	               "the eye stays vertically above the current kart position");
	CTR_TEST_CHECK(view->rot.x == 0x400 && view->rot.y == 0x900 && view->rot.z == 0,
	               "the camera looks vertically down with heading derived from the kart");
	CTR_TEST_CHECK(secondView->pos.x == 3000 && secondView->pos.z == 5000 && secondView->rot.y == 0xf00,
	               "split-screen cameras follow independent positions and headings");
	CTR_TEST_CHECK(camera->nearOrFar == 1 && camera->cameraMode == 0 && view->distanceToScreen_PREV == 256,
	               "the presentation override preserves the retail camera mode, zoom toggle and projection");
	const struct LevVertex vertices[] = {
		{.pos = {0, 400, 200}}, {.pos = {200, 400, 200}},
		{.pos = {0, 400, 400}}, {.pos = {200, 400, 400}},
		{.pos = {100, 400, 200}}, {.pos = {0, 400, 300}},
		{.pos = {100, 400, 300}}, {.pos = {200, 400, 300}},
		{.pos = {100, 400, 400}},
	};
	struct QuadBlock roof = {
		.index = {0, 2, 1, 3, 5, 4, 6, 8, 7},
		.bbox = {.min = {0, 400, 200}, .max = {200, 500, 400}},
	};
	const struct QuadBlock originalRoof = roof;
	CTR_TEST_CHECK(Chaos_HideCameraQuadBlock(view, &roof, vertices), "downward-facing tunnel geometry is cut away");
	CTR_TEST_CHECK(Chaos_HideCameraQuadBlock(secondView, &roof, vertices), "ceiling removal is stable outside the kart's current block footprint");
	struct QuadBlock road = {
		.index = {0, 1, 2, 3, 4, 5, 6, 7, 8},
		.quadFlags = QUADBLOCK_FLAG_GROUND,
		.bbox = {.min = {0, 200, 200}, .max = {200, 450, 400}},
	};
	human.underDriver = &road;
	Chaos_ApplyCameraRules(camera);
	CTR_TEST_CHECK(camera->ptrQuadBlock == &road && camera->quadBlockSearchHit,
	               "tunnel visibility uses the driver's ground block instead of a roof above the camera ray");
	CTR_TEST_CHECK(!Chaos_HideCameraQuadBlock(view, &road, vertices), "sloping ground that crosses the cutaway height remains drivable and visible");
	struct QuadBlock uphill = road;
	uphill.bbox = (struct BoundingBox){.min = {0, 400, 500}, .max = {200, 700, 900}};
	CTR_TEST_CHECK(!Chaos_HideCameraQuadBlock(view, &uphill, vertices), "elevated road ahead stays visible instead of being mistaken for a roof");
	uphill.quadFlags = 0;
	CTR_TEST_CHECK(!Chaos_HideCameraQuadBlock(view, &uphill, vertices), "a ramp ahead stays visible even without the ground flag");
	struct QuadBlock downhill = road;
	downhill.bbox = (struct BoundingBox){.min = {0, -300, 500}, .max = {200, 0, 900}};
	CTR_TEST_CHECK(!Chaos_HideCameraQuadBlock(view, &downhill, vertices), "descending road ahead is retained");
	struct QuadBlock bridge = roof;
	bridge.quadFlags = QUADBLOCK_FLAG_GROUND;
	CTR_TEST_CHECK(Chaos_HideCameraQuadBlock(view, &bridge, vertices), "a downward-facing underside is cut away regardless of the ground flag");
	int leafLists[2][2] = {{0}};
	int faceLists[2][3] = {{0}};
	int packedSource = 0;
	struct VisMem visibility = {
		.visLeafList = {leafLists[0], leafLists[1]},
		.visFaceList = {faceLists[0], faceLists[1]},
		.visLeafSrc = {&packedSource, &packedSource},
		.visFaceSrc = {&packedSource, &packedSource},
	};
	const struct mesh_info mesh = {.numBspNodes = 33, .numQuadBlock = 65};
	Chaos_ApplyLevelVisibilityRules(view, &visibility, &mesh, 0);
	CTR_TEST_CHECK(leafLists[0][0] == -1 && leafLists[0][1] == -1 &&
	               faceLists[0][0] == -1 && faceLists[0][1] == -1 && faceLists[0][2] == -1,
	               "overhead geometry selection includes leaves and faces excluded by the ground-level PVS");
	CTR_TEST_CHECK(leafLists[1][0] == 0 && faceLists[1][0] == 0 && visibility.visFaceSrc[1] == &packedSource,
	               "overhead visibility changes only the requested viewport");
	CTR_TEST_CHECK(visibility.visLeafSrc[0] == NULL && visibility.visFaceSrc[0] == NULL,
	               "visibility cache invalidation forces retail PVS decoding after expiry on the same block");
	road.quadFlags = 0;
	CTR_TEST_CHECK(!Chaos_HideCameraQuadBlock(view, &road, vertices), "the driver's supporting ramp stays visible even without the ground flag");
	const struct QuadBlock unflaggedFloor = road;
	CTR_TEST_CHECK(!Chaos_HideCameraQuadBlock(view, &unflaggedFloor, vertices), "unflagged floor remains visible even when it is not the current supporting block");
	const int boundaryPositions[] = {-100, 0, 100, 200, 300};
	for (int i = 0; i < (int)len(boundaryPositions); i++)
	{
		human.posCurr.x = boundaryPositions[i] * 256;
		human.posCurr.z = (200 + boundaryPositions[i]) * 256;
		CTR_TEST_CHECK(Chaos_HideCameraQuadBlock(view, &roof, vertices), "ceiling stays hidden across both X/Z bounding-box edges");
		CTR_TEST_CHECK(!Chaos_HideCameraQuadBlock(view, &unflaggedFloor, vertices), "unflagged pavement stays visible across block edges");
	}
	human.posCurr.x = 100 << 8;
	human.posCurr.z = 300 << 8;
	struct LevVertex slopedVertices[len(vertices)];
	memcpy(slopedVertices, vertices, sizeof(vertices));
	for (int i = 0; i < (int)len(slopedVertices); i++)
	{
		slopedVertices[i].pos.y = slopedVertices[i].pos.x * 2 + slopedVertices[i].pos.z;
	}
	CTR_TEST_CHECK(!Chaos_HideCameraQuadBlock(view, &unflaggedFloor, slopedVertices), "steep unflagged slopes retain their upward-facing pavement");
	struct LevVertex foldedVertices[len(vertices)];
	memcpy(foldedVertices, vertices, sizeof(vertices));
	foldedVertices[6].pos.x = -500;
	CTR_TEST_CHECK(!Chaos_HideCameraQuadBlock(view, &roof, foldedVertices), "a mixed block containing upward-facing high-detail triangles is preserved");
	struct LevVertex wallVertices[len(vertices)];
	memcpy(wallVertices, slopedVertices, sizeof(vertices));
	for (int i = 0; i < (int)len(wallVertices); i++)
	{
		wallVertices[i].pos.x = 0;
	}
	CTR_TEST_CHECK(!Chaos_HideCameraQuadBlock(view, &unflaggedFloor, wallVertices), "vertical panels are not mistaken for downward-facing ceilings");
	struct QuadBlock triangleRoof = roof;
	triangleRoof.index[3] = triangleRoof.index[2];
	triangleRoof.index[7] = UINT16_MAX;
	triangleRoof.index[8] = UINT16_MAX;
	CTR_TEST_CHECK(Chaos_HideCameraQuadBlock(view, &triangleRoof, vertices), "triangular blocks classify only their four valid high-detail faces");
	struct InstDef definition = {0};
	struct Instance decoration = {.instDef = &definition};
	CTR_TEST_CHECK(Chaos_HideCameraInstance(view, &decoration, 100, true), "static level models in front of the kart are cut away");
	CTR_TEST_CHECK(!Chaos_HideCameraInstance(view, &decoration, 100, false), "models outside the kart's screen position remain visible");
	CTR_TEST_CHECK(!Chaos_HideCameraInstance(view, &decoration, CHAOS_TOP_DOWN_HEIGHT * 4, true), "models at road height are not overhead occluders");
	struct Thread dynamicThread = {0};
	decoration.thread = &dynamicThread;
	CTR_TEST_CHECK(!Chaos_HideCameraInstance(view, &decoration, 100, true), "dynamic hazards and gameplay objects remain visible");
	CTR_TEST_CHECK(memcmp(&roof, &originalRoof, sizeof(roof)) == 0, "cutaway queries do not modify shared level geometry");
	s_tracker.gameMode1 |= PAUSE_1;
	Tick(3000);
	CTR_TEST_CHECK(Chaos_ApplyCameraRules(camera) && Chaos_HideCameraQuadBlock(view, &roof, vertices), "pause retains the top-down presentation and tunnel cutaway");
	s_tracker.gameMode1 &= ~PAUSE_1;
	human.posCurr.y = 300 << 8;
	human.angle = 0x300;
	Chaos_ApplyCameraRules(camera);
	CTR_TEST_CHECK(view->pos.y == 300 + CHAOS_TOP_DOWN_HEIGHT && view->rot.y == 0xb00, "height changes and turns update the top-down pose immediately");
	secondCamera->driverToFollow = &bot;
	CTR_TEST_CHECK(!Chaos_ApplyCameraRules(secondCamera), "a bot-follow camera does not receive the human-only override");
	secondCamera->driverToFollow = &partner;
	partner.actionsFlagSet = ACTION_RACE_FINISHED;
	CTR_TEST_CHECK(!Chaos_ApplyCameraRules(secondCamera), "finished humans regain their end-of-race camera");
	Tick(3000);
	view->pos = (SVec3){1, 2, 3};
	CTR_TEST_CHECK(!Chaos_ApplyCameraRules(camera) && view->pos.y == 2, "expiry hands presentation back to the live retail camera");
	CTR_TEST_CHECK(!Chaos_HideCameraQuadBlock(view, &roof, vertices), "expiry restores roof visibility immediately");
	leafLists[0][0] = 0;
	faceLists[0][0] = 0;
	Chaos_ApplyLevelVisibilityRules(view, &visibility, &mesh, 0);
	CTR_TEST_CHECK(leafLists[0][0] == 0 && faceLists[0][0] == 0, "expired Classic GTA leaves retail visibility lists alone");
	Chaos_Activate(CHAOS_EFFECT_CLASSIC_GTA, 1000);
	Chaos_Reset();
	CTR_TEST_CHECK(!Chaos_ApplyCameraRules(camera), "race teardown clears the camera override without a saved pose");
}

static void TestClockCurse(void)
{
	CTR_TEST_SECTION("Clock Curse sets the received clock once and bypasses protection");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_CLOCKBLOCKED] = true;
	sdata->chaosSettings.enabledCount++;
	g_config.maskProtectsFromDamage = true;
	struct Shield shieldObject = {0};
	struct Thread shieldThread = {.object = &shieldObject};
	struct Instance shield = {.thread = &shieldThread};
	struct Thread humanThread = {0};
	struct Thread partnerThread = {0};
	struct Instance humanInst = {.thread = &humanThread};
	struct Instance partnerInst = {.thread = &partnerThread};
	struct Driver human = {
		.driverID = 0, .actionsFlagSet = ACTION_MASK_WEAPON | ACTION_JUMP_BUTTON_HELD,
		.invincibleTimer = 1000, .numWumpas = 10, .reserves = 2000, .baseSpeed = 12000,
		.instSelf = &humanInst,
	};
	struct Driver partner = {.driverID = 1, .instBubbleHold = &shield, .instSelf = &partnerInst};
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT, .clockReceive = 3000};
	s_tracker.drivers[0] = &human;
	s_tracker.drivers[1] = &partner;
	struct GamepadBuffer pad = {.stickLX = 128};
	int steering = pad.stickLX;
	TickAndApplyDrivers(32);
	CTR_TEST_CHECK_EQ(human.clockReceive, 0, "inactive Clock Curse does not introduce a received clock");
	CTR_TEST_CHECK_EQ(VehPickState_NewState(&human, 1, NULL, 0), 0, "ordinary damage still respects mask and invincibility protection");
	Chaos_Activate(CHAOS_EFFECT_CLOCKBLOCKED, 1000);
	CTR_TEST_CHECK_EQ(human.clockReceive, 1000, "activation immediately assigns the full clock duration");
	CTR_TEST_CHECK(human.funcPtrs[DRIVER_FUNC_INIT] == VehPhysProc_SpinFirst_Init, "the initial clock spin bypasses mask and invincibility");
	CTR_TEST_CHECK(partner.funcPtrs[DRIVER_FUNC_INIT] == VehPhysProc_SpinFirst_Init, "the initial clock spin bypasses shields");
	CTR_TEST_CHECK_EQ(s_clockSounds, 1, "activation plays the clock sound once for all humans");
	CTR_TEST_CHECK_EQ(s_squishSounds, 0, "Clock Curse does not use the Giant Karts squish sound");
	human.funcPtrs[DRIVER_FUNC_INIT] = NULL;
	TickAndApplyDrivers(32);
	CTR_TEST_CHECK(human.funcPtrs[DRIVER_FUNC_INIT] == NULL, "continuous slowdown does not repeat the initial hit");
	CTR_TEST_CHECK_EQ(s_clockSounds, 1, "continuous slowdown does not repeat the clock sound");
	CTR_TEST_CHECK_EQ(human.clockReceive, 968, "the curse sets the received clock to the remaining Chaos duration");
	CTR_TEST_CHECK(partner.clockReceive > 32, "the curse also reaches a co-op partner protected by a shield");
	CTR_TEST_CHECK_EQ(human.actionsFlagSet, ACTION_MASK_WEAPON | ACTION_JUMP_BUTTON_HELD, "bypassing mask protection does not remove the mask or alter jump input");
	CTR_TEST_CHECK(partner.instBubbleHold == &shield, "bypassing a shield does not consume or detach it");
	CTR_TEST_CHECK_EQ(shieldObject.flags, 0, "the initial clock hit does not request a shield pop");
	CTR_TEST_CHECK_EQ(human.invincibleTimer, 1000, "the curse bypasses invincibility without changing its timer");
	CTR_TEST_CHECK_EQ(human.numWumpas, 10, "the retail spin setup leaves fruit loss to the spin state");
	CTR_TEST_CHECK_EQ(human.reserves, 0, "the initial clock hit clears reserves through retail damage handling");
	CTR_TEST_CHECK_EQ(human.baseSpeed, 12000, "Clock Curse leaves speed calculations to the retail clock consumers");
	CTR_TEST_CHECK_EQ(human.clockFlash, 0, "continuous application does not repeat the weapon's initial flash");
	TickAndApplyDrivers(32);
	CTR_TEST_CHECK_EQ(human.clockReceive, 936, "the received clock follows the remaining effect time on subsequent frames");
	partner.clockReceive = 4000;
	TickAndApplyDrivers(32);
	CTR_TEST_CHECK_EQ(partner.clockReceive, 3968, "a later retail clock counts down without being overwritten by Chaos");
	human.clockReceive = 9000;
	TickAndApplyDrivers(1000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_CLOCKBLOCKED), "the curse expires on its independent Chaos timer");
	CTR_TEST_CHECK_EQ(human.clockReceive, 8000, "curse expiry preserves a longer clock received during the effect");
	CTR_TEST_CHECK_EQ(partner.clockReceive, 2968, "Chaos expiry leaves a later retail clock intact");

	CTR_TEST_SECTION("reactivated curses replace the clock and remove earlier timers");
	Chaos_BeginRace(&s_tracker);
	human.clockReceive = 0;
	partner.clockReceive = 0;
	Chaos_Activate(CHAOS_EFFECT_CLOCKBLOCKED, 256);
	TickAndApplyDrivers(32);
	Chaos_Activate(CHAOS_EFFECT_CLOCKBLOCKED, 64);
	TickAndApplyDrivers(32);
	CTR_TEST_CHECK_EQ(human.clockReceive, 32, "the replacement assigns its own remaining duration");
	TickAndApplyDrivers(32);
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 0, "the shorter replacement curse leaves no old entry behind");
	for (int i = 0; i < 4; i++)
	{
		TickAndApplyDrivers(32);
		CTR_TEST_CHECK_EQ(human.clockReceive, 0, "the superseded activation cannot resume the overwritten clock");
	}
	TickAndApplyDrivers(32);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_CLOCKBLOCKED), "the last curse expires without a separate restoration step");
	CTR_TEST_CHECK_EQ(human.clockReceive, 0, "the received clock reaches zero with the final curse timer");
	Chaos_Activate(CHAOS_EFFECT_CLOCKBLOCKED, 60000);
	CTR_TEST_CHECK_EQ(human.clockReceive, 60000, "a one-off activation stores the full sixty-second duration");
	TickAndApplyDrivers(32);
	CTR_TEST_CHECK_EQ(human.clockReceive, 59968, "the received clock retains durations above the signed 16-bit range");
	TickAndApplyDrivers(32000);
	CTR_TEST_CHECK_EQ(human.clockReceive, 27968, "the full clock counts down across the old signed 16-bit boundary");
	TickAndApplyDrivers(27968);
	CTR_TEST_CHECK_EQ(human.clockReceive, 0, "long curses also reach zero at final expiry");

	CTR_TEST_SECTION("Clock Curse respects race gates and driver scope without a separate baseline snapshot");
	Chaos_Activate(CHAOS_EFFECT_CLOCKBLOCKED, 5000);
	s_tracker.elapsedTimeMS = 32;
	PreparedDriver(&bot, &pad, &steering, bot.actionsFlagSet);
	CTR_TEST_CHECK_EQ(bot.clockReceive, 3000, "Clock Curse does not change bot clocks");
	human.clockReceive = 0;
	human.actionsFlagSet |= ACTION_RACE_FINISHED;
	PreparedDriver(&human, &pad, &steering, human.actionsFlagSet);
	CTR_TEST_CHECK_EQ(human.clockReceive, 0, "finished humans do not receive the curse");
	human.actionsFlagSet &= ~ACTION_RACE_FINISHED;
	s_tracker.gameMode1 |= PAUSE_1;
	const struct ChaosRuntime saved = sdata->chaosRuntime;
	TickAndApplyDrivers(5000);
	CTR_TEST_CHECK_EQ(human.clockReceive, 0, "pause cannot reapply one-off clock damage");
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_CLOCKBLOCKED, 5000), "pause blocks a new one-off activation");
	CTR_TEST_CHECK(memcmp(&saved, &sdata->chaosRuntime, sizeof(saved)) == 0, "pause freezes curse lifetimes");
	s_tracker.gameMode1 &= ~PAUSE_1;
	s_tracker.gameMode1 |= ADVENTURE_ARENA;
	TickAndApplyDrivers(5000);
	CTR_TEST_CHECK_EQ(human.clockReceive, 0, "Adventure hubs do not receive Clock Curse");
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_CLOCKBLOCKED, 5000), "Adventure hubs block a new one-off activation");
	s_tracker.gameMode1 &= ~ADVENTURE_ARENA;
	TickAndApplyDrivers(32);
	CTR_TEST_CHECK_EQ(human.clockReceive, 0, "reopening the race gate does not repeat a one-off activation");
	sdata->chaosRuntime = saved;
	human.clockReceive = 0;
	TickAndApplyDrivers(32);
	CTR_TEST_CHECK_EQ(human.clockReceive, 0, "restoring only Chaos runtime does not replay a one-off activation");
	human.clockReceive = 4000;
	const int clockBeforeReset = human.clockReceive;
	Chaos_Reset();
	TickAndApplyDrivers(32);
	CTR_TEST_CHECK_EQ(human.clockReceive, clockBeforeReset - 32, "runtime reset leaves the existing received clock to its ordinary countdown");

	CTR_TEST_SECTION("initial Clock Curse damage and audio occur only on successful activations");
	Chaos_BeginRace(&s_tracker);
	human.actionsFlagSet |= ACTION_RACE_FINISHED;
	partner.actionsFlagSet |= ACTION_RACE_FINISHED;
	human.funcPtrs[DRIVER_FUNC_INIT] = NULL;
	partner.funcPtrs[DRIVER_FUNC_INIT] = NULL;
	s_tracker.drivers[2] = &bot;
	Chaos_Activate(CHAOS_EFFECT_CLOCKBLOCKED, 1000);
	CTR_TEST_CHECK(human.funcPtrs[DRIVER_FUNC_INIT] == NULL && partner.funcPtrs[DRIVER_FUNC_INIT] == NULL, "the initial hit skips finished humans");
	CTR_TEST_CHECK(bot.funcPtrs[DRIVER_FUNC_INIT] == NULL, "the initial hit skips bots");
	human.actionsFlagSet &= ~ACTION_RACE_FINISHED;
	for (int i = 1; i < CHAOS_MAX_ACTIVATIONS; i++)
	{
		human.funcPtrs[DRIVER_FUNC_INIT] = NULL;
		Chaos_Activate(CHAOS_EFFECT_CLOCKBLOCKED, 1000);
		CTR_TEST_CHECK(human.funcPtrs[DRIVER_FUNC_INIT] == VehPhysProc_SpinFirst_Init, "each successful reactivation delivers a new initial hit");
	}
	human.funcPtrs[DRIVER_FUNC_INIT] = NULL;
	const int clockSounds = s_clockSounds;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_CLOCKBLOCKED, 1000), "reactivation accepts another curse");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, 1, "repeated curses still occupy only one slot");
	CTR_TEST_CHECK(human.funcPtrs[DRIVER_FUNC_INIT] == VehPhysProc_SpinFirst_Init, "reactivation repeats the one-off clock hit");
	CTR_TEST_CHECK_EQ(s_clockSounds, clockSounds + 1, "reactivation replays the clock sound once");
	human.funcPtrs[DRIVER_FUNC_INIT] = NULL;
	CTR_TEST_CHECK(!Chaos_Activate(CHAOS_EFFECT_CLOCKBLOCKED, 0), "invalid reactivation rejects another curse");
	CTR_TEST_CHECK(human.funcPtrs[DRIVER_FUNC_INIT] == NULL, "rejected activations cannot inflict clock damage");
	CTR_TEST_CHECK_EQ(s_clockSounds, clockSounds + 1, "rejected activations cannot play clock audio");
	CTR_TEST_CHECK_EQ(VehPickState_NewState(&partner, 1, NULL, 0), 0, "ordinary damage still respects shield protection");
	CTR_TEST_CHECK((shieldObject.flags & SHIELD_FLAG_POP_ON_DAMAGE) != 0, "ordinary damage retains retail shield consumption");
}

static void TestHopInputAndCloudOwnership(void)
{
	CTR_TEST_SECTION("No Hopping clears pending manual jumps without overwriting retail cloud effects");
	EnableInputEffects();
	struct Driver driver = {.driverID = 0, .kartState = KS_NORMAL};
	const u32 unrelated = ACTION_TOUCH_GROUND | ACTION_MASK_WEAPON;
	u32 actions = PreparedJump(&driver, BTN_L1, BTN_L1, RAIN_CLOUD_EFFECT_NONE, unrelated);
	CTR_TEST_CHECK((actions & ACTION_JUMP_BUTTON_HELD) != 0, "ordinary hop input still latches the shoulder button");
	CTR_TEST_CHECK_EQ(driver.jump_TenBuffer, 10, "ordinary hop input still fills the retail buffer");
	CTR_TEST_CHECK_EQ(driver.buttonUsedToStartDrift, BTN_L1, "a single tapped shoulder selects its drift button");
	PreparedJump(&driver, BTN_L1 | BTN_R1, BTN_L1 | BTN_R1, RAIN_CLOUD_EFFECT_NONE, unrelated);
	CTR_TEST_CHECK_EQ(driver.buttonUsedToStartDrift, BTN_R1, "simultaneous shoulder taps preserve the retail R1 preference");
	driver.jump_TenBuffer = 4;
	actions = PreparedJump(&driver, 0, BTN_R1, RAIN_CLOUD_EFFECT_NONE, unrelated | ACTION_JUMP_BUTTON_HELD);
	CTR_TEST_CHECK_EQ(driver.jump_TenBuffer, 4, "holding an already-latched shoulder does not refresh the retail buffer");
	actions = PreparedJump(&driver, 0, 0, RAIN_CLOUD_EFFECT_NONE, actions);
	CTR_TEST_CHECK_EQ(actions, unrelated, "releasing the shoulders clears the ordinary jump latch");
	CTR_TEST_CHECK_EQ(driver.jump_TenBuffer, 0, "releasing the shoulders clears the ordinary jump buffer");
	driver.rainCloudEffect = RAIN_CLOUD_EFFECT_ITEM_ROLL;
	driver.jump_TenBuffer = 4;
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 1000);
	actions = PreparedJump(&driver, BTN_R1, BTN_R1, driver.rainCloudEffect, unrelated | ACTION_JUMP_BUTTON_HELD);
	CTR_TEST_CHECK_EQ(actions, unrelated, "active No Hopping clears only the jump latch");
	CTR_TEST_CHECK_EQ(driver.jump_TenBuffer, 0, "a hop buffered before activation cannot fire later");
	CTR_TEST_CHECK_EQ(driver.rainCloudEffect, RAIN_CLOUD_EFFECT_ITEM_ROLL, "Chaos does not replace the real cloud's item effect");
	CTR_TEST_CHECK_EQ(PreparedJump(&driver, 0, BTN_L1, RAIN_CLOUD_EFFECT_NONE, unrelated), unrelated,
	                  "holding a shoulder cannot refill the buffer during No Hopping");
	Tick(500);
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 1000);
	Tick(500);
	CTR_TEST_CHECK_EQ(PreparedJump(&driver, BTN_R1, BTN_R1, RAIN_CLOUD_EFFECT_NONE, unrelated), unrelated,
	                  "the second independent activation continues blocking hops");
	Tick(500);
	actions = PreparedJump(&driver, BTN_R1, BTN_R1, driver.rainCloudEffect, unrelated);
	CTR_TEST_CHECK((actions & ACTION_JUMP_BUTTON_HELD) != 0, "hop input resumes on final expiry even while a different cloud effect exists");
	driver.jump_TenBuffer = 0;
	actions = PreparedJump(&driver, BTN_R1, BTN_R1, RAIN_CLOUD_EFFECT_JUMP_LOCKOUT, unrelated);
	CTR_TEST_CHECK_EQ(actions, unrelated, "retail cloud lockout still blocks hops after Chaos expires");
	CTR_TEST_CHECK_EQ(driver.jump_TenBuffer, 0, "Chaos expiry does not clear the retail cloud's jump lockout");

	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 1000);
	driver.driverID = 2;
	actions = PreparedJump(&driver, BTN_L1, BTN_L1, RAIN_CLOUD_EFFECT_NONE, unrelated);
	CTR_TEST_CHECK((actions & ACTION_JUMP_BUTTON_HELD) != 0, "No Hopping does not suppress input for a bot slot");
	driver.driverID = 1;
	actions = PreparedJump(&driver, BTN_L1, BTN_L1, RAIN_CLOUD_EFFECT_NONE, unrelated);
	CTR_TEST_CHECK_EQ(actions, unrelated, "No Hopping applies to the second human too");
	driver.driverID = 0;
	Chaos_Reset();
	actions = PreparedJump(&driver, BTN_L1, BTN_L1, RAIN_CLOUD_EFFECT_NONE, unrelated);
	CTR_TEST_CHECK((actions & ACTION_JUMP_BUTTON_HELD) != 0, "race reset leaves no synthetic jump lockout behind");
}

static void TestDriverApplication(void)
{
	CTR_TEST_SECTION("direct application removes resolved hops while preserving drift state and shared input");
	EnableInputEffects();
	struct Driver driver = {
		.driverID = 0, .kartState = KS_DRIFTING, .jump_TenBuffer = 4,
		.actionsFlagSet = ACTION_TOUCH_GROUND | ACTION_JUMP_BUTTON_HELD,
		.forcedJumpType = FORCED_JUMP_HIGH,
	};
	struct Driver partner = {.driverID = 1};
	struct GamepadBuffer pad = {
		.stickLX = 32,
		.buttonsHeldCurrFrame = BTN_R1 | BTN_CROSS_one,
		.buttonsTapped = BTN_L1 | BTN_CIRCLE,
	};
	const struct GamepadBuffer original = pad;
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 1000);
	Chaos_Activate(CHAOS_EFFECT_MIRROR_MIRROR, 1000);
	Chaos_Activate(CHAOS_EFFECT_GROUNDED, 1000);
	int steering = pad.stickLX;
	u32 actions = VehPhysProc_JumpInput(&driver, pad.buttonsTapped, pad.buttonsHeldCurrFrame,
	                                   RAIN_CLOUD_EFFECT_NONE, driver.actionsFlagSet);
	CTR_TEST_CHECK((actions & ACTION_JUMP_BUTTON_HELD) != 0, "retail input resolves its hop latch before Chaos application");
	actions = PreparedDriver(&driver, &pad, &steering, actions);
	CTR_TEST_CHECK_EQ(steering, 224, "steering and No Hopping are applied in the same pass");
	CTR_TEST_CHECK_EQ(driver.kartState, KS_DRIFTING, "an existing drift continues without a forced state transition");
	CTR_TEST_CHECK_EQ(actions, ACTION_TOUCH_GROUND, "application removes the resolved jump latch without clearing other actions");
	CTR_TEST_CHECK_EQ(driver.jump_TenBuffer, 0, "pending hops are removed before jump physics can consume them");
	CTR_TEST_CHECK_EQ(driver.forcedJumpType, FORCED_JUMP_HIGH, "forced jumps are not removed by application");
	CTR_TEST_CHECK(memcmp(&pad, &original, sizeof(pad)) == 0, "neither effect writes any shared gamepad field");
	CTR_TEST_CHECK((pad.buttonsHeldCurrFrame & BTN_R1) != 0, "drift still sees its held shoulder");
	CTR_TEST_CHECK((pad.buttonsTapped & BTN_L1) != 0, "drift still sees its boost tap");
	for (int frame = 0; frame < 3; frame++)
	{
		actions = VehPhysProc_JumpInput(&driver, 0, BTN_R1, RAIN_CLOUD_EFFECT_NONE, actions);
		actions = PreparedDriver(&driver, &pad, &steering, actions);
		CTR_TEST_CHECK_EQ(actions, ACTION_TOUCH_GROUND, "held shoulders cannot retain a resolved hop on later frames");
		CTR_TEST_CHECK_EQ(driver.jump_TenBuffer, 0, "held shoulders cannot retain a jump buffer on later frames");
	}
	struct GamepadBuffer partnerPad = {.stickLX = 200, .buttonsHeldCurrFrame = BTN_SQUARE_one};
	CTR_TEST_CHECK_EQ(PreparedSteering(&partner, &partnerPad), 56, "each player resolves their own steering baseline");
	CTR_TEST_CHECK_EQ(steering, 224, "applying player two preserves player one's local steering");
	driver.kartState = KS_CRASHING;
	PreparedDriver(&driver, &pad, &steering, actions);
	CTR_TEST_CHECK_EQ(driver.kartState, KS_CRASHING, "No Hopping does not replace damage states with normal driving");
	Tick(1000);
	CTR_TEST_CHECK_EQ(PreparedSteering(&driver, &pad), pad.stickLX, "expiry uses live steering without restoring a snapshot");
	actions = PreparedJump(&driver, BTN_R1, BTN_R1, RAIN_CLOUD_EFFECT_NONE, ACTION_TOUCH_GROUND);
	CTR_TEST_CHECK((actions & ACTION_JUMP_BUTTON_HELD) != 0, "expiry lets retail hop input resume without restoring a snapshot");
	CTR_TEST_CHECK_EQ(driver.jump_TenBuffer, 10, "expiry lets retail input fill the jump buffer again");
}

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
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numActive, CHAOS_MAX_ACTIVATIONS, "instant rewards consume no timed slot");
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_JUICED_UP), "an instant reward is never an ongoing modifier");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numInstantNotices, 1, "the reward has its own notice");
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
	CTR_TEST_CHECK(strcmp(s_noticeText, "Juiced Up") == 0, "the instant notice survives the requested effect duration");
	CTR_TEST_CHECK_EQ(s_noticeFont, FONT_BIG, "the instant notice does not enter the compact docking animation");
	Tick(100);
	DrawNotice();
	CTR_TEST_CHECK_EQ(s_noticeDraws, CHAOS_MAX_ACTIVATIONS, "only timed effects remain after the instant banner expires");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numInstantNotices, 0, "expired notices leave no docked entry");
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_JUICED_UP, 1000), "Juiced Up remains eligible at ten Wumpas");
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_JUICED_UP, 1000), "one-shot rewards can repeat immediately");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numInstantNotices, 1, "repeated gifts refresh rather than duplicate their banner");
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
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numInstantNotices, CHAOS_MAX_INSTANT_NOTICES, "a fifth distinct reward keeps notice storage bounded");
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.instantNotices[0].effect, CHAOS_EFFECT_PROVIDENCE, "notice overflow drops the oldest banner");
	const struct ChaosRuntime notices = sdata->chaosRuntime;
	Chaos_Reset();
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numInstantNotices, 0, "reset clears instant notices too");
	sdata->chaosRuntime = notices;
	CTR_TEST_CHECK_EQ(sdata->chaosRuntime.numInstantNotices, CHAOS_MAX_INSTANT_NOTICES, "resident restore retains instant announcements");
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
	VehPickState_ApplyDamage(human, VEH_PICK_DAMAGE_SPIN, NULL, 0, true);
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
	PreparedSteering(human, &(struct GamepadBuffer){0});
	CTR_TEST_CHECK_EQ(human->reserves, 0, "the replacement rule clears fuel immediately when applied");
	Chaos_Activate(CHAOS_EFFECT_TOO_BOOSTED_TO_FAIL, 1000);
	CTR_TEST_CHECK(!Chaos_IsActive(CHAOS_EFFECT_HOLE_IN_THE_TANK), "reserve protection replaces No Reserves in the opposite order");
}

static void TestSenileMask(void)
{
	CTR_TEST_SECTION("Senile Mask adds false warnings without changing race direction or progress");
	SetupRace(CHAOS_TRIGGER_EXTERNAL);
	sdata->chaosSettings.effects[CHAOS_EFFECT_SENILE_MASK] = true;
	sdata->boolCanPlayVoicelines = true;
	struct Driver *human = s_tracker.drivers[0];
	struct Driver *partner = s_tracker.drivers[1];
	struct Driver bot = {.driverID = 2, .actionsFlagSet = ACTION_BOT};
	s_tracker.drivers[2] = &bot;
	human->distanceDrivenBackwards = 0;
	const struct Driver original = *human;
	Chaos_DrawWrongWayWarnings(&s_tracker);
	CTR_TEST_CHECK_EQ(s_wrongWayDraws, 0, "inactive Senile Mask adds no warnings");
	CTR_TEST_CHECK(!Chaos_PlayWrongWayVoice(), "inactive Senile Mask leaves retail voice arbitration alone");
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_SENILE_MASK, 1000), "Senile Mask activates as a timed effect");
	Chaos_DrawWrongWayWarnings(&s_tracker);
	CTR_TEST_CHECK_EQ(s_wrongWayDraws, 3, "both correct-direction humans receive the shared flashing prompt, never bots");
	Voiceline_Update();
	CTR_TEST_CHECK_EQ(s_wrongWayVoiceID, 0x1e, "the first eligible human uses the ordinary good-mask voice");
	CTR_TEST_CHECK_EQ(sdata->voicelineCooldown, 45, "Senile Mask halves the ordinary track-length-plus-delay cooldown");
	CTR_TEST_CHECK(memcmp(human, &original, sizeof(original)) == 0, "false warnings leave direction flags, distance, progress and inventory intact");
	for (int i = 0; i < 44; i++)
	{
		Voiceline_Update();
	}
	CTR_TEST_CHECK_EQ(s_wrongWayVoices, 1, "the warning cannot replay before its shortened cooldown");
	Voiceline_Update();
	CTR_TEST_CHECK_EQ(s_wrongWayVoices, 1, "an occupied XA channel is not interrupted when the cooldown expires");
	sdata->XA_State = 0;
	Voiceline_Update();
	CTR_TEST_CHECK_EQ(s_wrongWayVoices, 2, "the warning repeats without needing a real wrong-way direction transition");

	human->actionsFlagSet |= ACTION_DRIVING_WRONG_WAY;
	s_wrongWayDraws = 0;
	Chaos_DrawWrongWayWarnings(&s_tracker);
	CTR_TEST_CHECK_EQ(s_wrongWayDraws, 2, "a genuinely wrong-way human receives no false warning");
	CTR_TEST_CHECK(Chaos_OverridesWrongWayWarning(human) && Chaos_OverridesWrongWayWarning(partner),
	    "Senile Mask replaces retail warnings for both directions, supplying only its correct-direction warnings");
	// A retail warning may already be armed when the effect starts or direction changes.
	s_tracker.numPlyrCurrGame = 1;
	sdata->WrongWayDirection_bool = true;
	sdata->framesDrivingSameDirection = 31;
	sdata->boolCanPlayWrongWaySFX = true;
	sdata->XA_State = 0;
	sdata->voicelineCooldown = 0;
	const int voicesBeforeWrongWay = s_wrongWayVoices;
	Voiceline_Update();
	CTR_TEST_CHECK_EQ(s_wrongWayVoices, voicesBeforeWrongWay, "an armed retail voice stays silent while the human is genuinely wrong-way");
	CTR_TEST_CHECK(sdata->boolCanPlayWrongWaySFX, "suppressing the retail voice does not consume its armed warning");
	s_tracker.numPlyrCurrGame = 2;
	sdata->XA_State = 0;
	sdata->voicelineCooldown = 0;
	Voiceline_Update();
	CTR_TEST_CHECK(s_voiceDriver == partner && s_wrongWayVoiceID == 0x3d, "2P speech uses the eligible partner's normal evil-mask voice");
	partner->actionsFlagSet |= ACTION_RACE_FINISHED;
	s_wrongWayDraws = 0;
	Chaos_DrawWrongWayWarnings(&s_tracker);
	CTR_TEST_CHECK_EQ(s_wrongWayDraws, 0, "finished humans receive no false warning");
	CTR_TEST_CHECK(!Chaos_PlayWrongWayVoice(), "no false voice is requested without an eligible human");
	partner->actionsFlagSet = 0;
	const int blockedModes[] = {PAUSE_1, DEBUG_MENU};
	for (int i = 0; i < (int)len(blockedModes); i++)
	{
		s_tracker.gameMode1 |= blockedModes[i];
		s_wrongWayDraws = 0;
		Chaos_DrawWrongWayWarnings(&s_tracker);
		CTR_TEST_CHECK(s_wrongWayDraws == 0 && !Chaos_PlayWrongWayVoice(), "pause and debug menus suppress false warnings and voices");
		s_tracker.gameMode1 &= ~blockedModes[i];
	}
	s_tracker.gameMode1 |= CRYSTAL_CHALLENGE;
	s_wrongWayDraws = 0;
	Chaos_DrawWrongWayWarnings(&s_tracker);
	CTR_TEST_CHECK_EQ(s_wrongWayDraws, 2, "false warnings also apply in crystal events without a retail wrong-way HUD path");
	s_tracker.gameMode1 &= ~CRYSTAL_CHALLENGE;
	CTR_TEST_CHECK(Chaos_Activate(CHAOS_EFFECT_SENILE_MASK, 1000) && sdata->chaosRuntime.numActive == 1,
	    "repeated activation refreshes one timed effect");
	sdata->XA_State = 0;
	sdata->voicelineCooldown = 0;
	s_voicePlayFails = true;
	Voiceline_Update();
	CTR_TEST_CHECK_EQ(sdata->voicelineCooldown, 15, "missing voice assets retain a bounded retry delay");
	s_voicePlayFails = false;
	Tick(1000);
	s_wrongWayDraws = 0;
	Chaos_DrawWrongWayWarnings(&s_tracker);
	CTR_TEST_CHECK(s_wrongWayDraws == 0 && !Chaos_PlayWrongWayVoice(), "expiry removes both false warning hooks");
	CTR_TEST_CHECK(!Chaos_OverridesWrongWayWarning(human), "expiry restores the real wrong-way warning");

	// The ordinary one-shot warning must retain its original cooldown.
	s_tracker.numPlyrCurrGame = 1;
	sdata->WrongWayDirection_bool = true;
	sdata->framesDrivingSameDirection = 31;
	sdata->boolCanPlayWrongWaySFX = true;
	sdata->voicelineCooldown = 0;
	Voiceline_Update();
	CTR_TEST_CHECK_EQ(sdata->voicelineCooldown, 90, "retail wrong-way playback keeps its full cooldown");
	CTR_TEST_CHECK(!sdata->boolCanPlayWrongWaySFX, "retail wrong-way speech remains one-shot until rearmed");
	Chaos_Activate(CHAOS_EFFECT_SENILE_MASK, 1000);
	Chaos_Reset();
	CTR_TEST_CHECK(!Chaos_PlayWrongWayVoice(), "reset leaves no Senile Mask playback pending");
}

int main(void)
{
	TestSteeringAndDriverScope();
	TestSabotage();
	TestHydraulics();
	TestMyPlanetNeedsMe();
	TestLaunchSpeedLimit();
	TestWhoBoundThis();
	TestTrySomethingNew();
	TestIdentityCrisis();
	TestBodySwap();
	TestTeleport();
	TestKingCrimson();
	TestTeleportProgressProjection();
	TestDefectiveTeleport();
	TestTimeStop();
	TestTimeStopPotionContacts();
	TestWobbleSteering();
	TestGravityEffects();
	TestSpeedEffects();
	TestNoReserves();
	TestDefectiveBoosts();
	TestThisIsFire();
	TestNoItems();
	TestWumpaDrain();
	TestClockCurse();
	TestExplosiveCrates();
	TestHardCrates();
	TestGiantKarts();
	TestSmallKartEnergy();
	TestKartParticleScale();
	TestClassicGTA();
	TestArenaChampions();
	TestYouveGotMail();
	TestFreshStart();
	TestSenileMask();
	TestAnxiousMask();
	TestHopInputAndCloudOwnership();
	TestDriverApplication();
	TestInstantGifts();
	TestHeldItemRemoval();
	TestRandomItemGift();
	TestReserveGifts();
	return CtrTest_Summary();
}
