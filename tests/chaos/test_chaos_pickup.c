#include <shared/ctr_test.h>
#include <common.h>

struct sData sdata_static;
struct Data data;
NativeConfig g_config;
static struct GameTracker s_tracker;
static struct Instance s_explosion;
static int s_pickups;
static struct Driver *s_collector;
static bool s_hardCrates;

#include <game/231/RB_Crate.c>
#include <game/231/RB_Fruit.c>
#include <game/231/RB_Crystal.c>
#include <game/231/RB_CtrLetter.c>

bool Chaos_OnPickup(struct Driver *driver)
{
	s_pickups++;
	s_collector = driver;
	return true;
}
bool Chaos_IsHardCrateForDriver(const struct Instance *inst, const struct Thread *thread, const struct ScratchpadStruct *sps)
{
	(void)thread;
	return s_hardCrates && inst->scale.x == 0x1000 && sps->Input1.modelID == DYNAMIC_PLAYER;
}
void Chaos_ApplyCrateRules(struct Driver *driver, struct Instance *inst)
{
	(void)driver;
	(void)inst;
}
struct Instance *INSTANCE_BirthWithThread_Stack(const struct InstanceBirthParams *params)
{
	CTR_TEST_CHECK_EQ(params->modelID, STATIC_CRATE_EXPLOSION, "breaking a crate retains its retail visual");
	return &s_explosion;
}
struct Thread *PROC_BirthWithObject(ThreadFlags flags, void *tick, const char *name, struct Thread *relative)
{
	(void)flags; (void)tick; (void)name; (void)relative;
	CtrTest_Bail("collision fixtures supply their pickup threads");
	return NULL;
}
s32 INSTANCE_GetNumAnimFrames(struct Instance *inst, s32 animIndex)
{
	(void)inst; (void)animIndex;
	return 1;
}
void INSTANCE_Death(struct Instance *inst) { (void)inst; }
void ThTick_FastRET(struct Thread *thread) { (void)thread; }
void RB_Default_LInB(struct Instance *inst) { (void)inst; }
void ConvertRotToMatrix(MATRIX *matrix, const SVec3 *rot) { (void)matrix; (void)rot; }
void MatrixRotate(void *dst, MATRIX *src, MATRIX *rot) { (void)dst; (void)src; (void)rot; }
void Vector_SpecLightSpin3D(struct Instance *inst, const SVec3 *rot, const SVec3 *light)
{
	(void)inst; (void)rot; (void)light;
}
void PlaySound3D(u32 sound, struct Instance *inst) { (void)sound; (void)inst; }
int OtherFX_Play(u32 sound, int flags) { (void)sound; (void)flags; return 0; }
void Voiceline_RequestPlay(u32 sound, u32 character, u32 flags) { (void)sound; (void)character; (void)flags; }
int MixRNG_Scramble(void) { return 0; }
int MATH_Sin(u32 angle) { (void)angle; return 0; }
void CTC2(u32 value, s32 reg) { (void)value; (void)reg; }
void MTC2(u32 value, s32 reg) { (void)value; (void)reg; }
u32 MFC2(int reg) { (void)reg; return 0; }
s32 MFC2_S(s32 reg) { (void)reg; return 0; }
s32 doCOP2(s32 op) { (void)op; return 0; }
void NativePgxp_LoadTransform(const void *key, const s16 *rotation, const s32 *translation, int bank)
{
	(void)key; (void)rotation; (void)translation; (void)bank;
}

static void TestCrates(void)
{
	CTR_TEST_SECTION("Consumed crates dispatch once, even without a reward; solid/hidden crates and weapon owners do not");
	ThreadScratchCollideFunc callbacks[] = {RB_CrateWeapon_ThCollide, RB_CrateFruit_ThCollide, RB_CrateTime_ThCollide};
	typedef int (*CrateContact)(struct Instance *, struct Thread *, struct ScratchpadStruct *);
	CrateContact contacts[] = {RB_CrateWeapon_LInC, RB_CrateFruit_LInC, RB_CrateTime_LInC};
	const int models[] = {PU_RANDOM_CRATE, PU_FRUIT_CRATE, STATIC_TIME_CRATE_01};
	for (unsigned int i = 0; i < len(contacts); i++)
	{
		struct Model model = {.id = models[i]};
		struct Crate object = {0};
		struct Thread crateThread = {.object = &object, .funcThCollide = (void *)callbacks[i]};
		struct Instance crate = {.scale = {0x1000, 0x1000, 0x1000}, .thread = &crateThread, .model = &model};
		crateThread.inst = &crate;
		struct Instance kart = {0};
		struct Driver driver = {.driverID = 0, .instSelf = &kart, .heldItemID = HELD_ITEM_MASK};
		struct Thread driverThread = {.object = &driver};
		struct ScratchpadStruct sps = {0};
		sps.Input1.modelID = DYNAMIC_PLAYER;
		s_pickups = 0;
		crate.scale.x = 0x800;
		contacts[i](&crate, &driverThread, &sps);
		CTR_TEST_CHECK_EQ(s_pickups, 0, "a partly regrown crate is not yet a pickup");
		crate.scale.x = 0x1000;
		s_hardCrates = true;
		CTR_TEST_CHECK_EQ(contacts[i](&crate, &driverThread, &sps), COLL_HITBOX_FORCE_SOLID, "Hard Crates retains the solid collision response");
		CTR_TEST_CHECK_EQ(s_pickups, 0, "an intact solid crate does not count as a pickup");
		s_hardCrates = false;
		CTR_TEST_CHECK_EQ(contacts[i](&crate, &driverThread, &sps), 1, "a normal crate can be consumed");
		CTR_TEST_CHECK_EQ(s_pickups, 1, "consuming each crate type dispatches exactly one pickup");
		CTR_TEST_CHECK(s_collector == &driver, "the pickup belongs to the colliding driver");
		contacts[i](&crate, &driverThread, &sps);
		CTR_TEST_CHECK_EQ(s_pickups, 1, "continued contact with a hidden crate cannot retrigger");
		crate.scale = (SVec3){0x1000, 0x1000, 0x1000};
		object.cooldown = 0;
		contacts[i](&crate, &driverThread, &sps);
		CTR_TEST_CHECK_EQ(s_pickups, 2, "a regrown crate can trigger another pickup");
		crate.scale = (SVec3){0x1000, 0x1000, 0x1000};
		object.cooldown = 0;
		struct TrackerWeapon weapon = {.driverParent = &driver};
		struct Thread weaponThread = {.object = &weapon};
		sps.Input1.modelID = DYNAMIC_BOMB;
		contacts[i](&crate, &weaponThread, &sps);
		CTR_TEST_CHECK_EQ(s_pickups, 2, "a weapon-owned crate reward is not direct pickup contact");
	}
}

static void TestCollectibles(void)
{
	CTR_TEST_SECTION("Fruit, crystals and CTR letters dispatch on collection, without waiting for HUD credit");
	ThreadScratchCollideFunc callbacks[] = {RB_Fruit_ThCollide, RB_Crystal_ThCollide, RB_CtrLetter_ThCollide};
	for (unsigned int i = 0; i < len(callbacks); i++)
	{
		struct Fruit object = {0};
		struct Model model = {0};
		struct Thread pickupThread = {.object = &object};
		struct Instance pickup = {.scale = {0x1000, 0x1000, 0x1000}, .model = &model, .thread = &pickupThread};
		pickupThread.inst = &pickup;
		struct Driver driver = {.driverID = 1, .numWumpas = 10};
		struct Thread driverThread = {.object = &driver};
		struct ScratchpadStruct sps = {0};
		sps.Input1.modelID = DYNAMIC_PLAYER;
		s_pickups = 0;
		CTR_TEST_CHECK_EQ(callbacks[i](&pickupThread, &driverThread, NULL, &sps), 1, "a visible collectible accepts human contact");
		CTR_TEST_CHECK_EQ(s_pickups, 1, "each collectible dispatches one pickup immediately");
		CTR_TEST_CHECK(s_collector == &driver, "the co-op collector is passed to Chaos");
		CTR_TEST_CHECK_EQ(driver.numWumpas, 10, "the trigger does not wait for delayed HUD awards");
		CTR_TEST_CHECK_EQ(callbacks[i](&pickupThread, &driverThread, NULL, &sps), 0, "a consumed collectible rejects repeated contact");
		CTR_TEST_CHECK_EQ(s_pickups, 1, "repeated collectible contact cannot refresh the effect");
		pickup.scale.x = 0x1000;
		sps.Input1.modelID = DYNAMIC_ROBOT_CAR;
		driver.driverID = 2;
		callbacks[i](&pickupThread, &driverThread, NULL, &sps);
		CTR_TEST_CHECK_EQ(s_pickups, 1, "bot collection cannot dispatch a human pickup trigger");
	}
}

int main(void)
{
	sdata->gGT = &s_tracker;
	TestCrates();
	TestCollectibles();
	return CtrTest_Summary();
}
