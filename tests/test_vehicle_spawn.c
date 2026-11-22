#include <shared/ctr_test.h>
#include <common.h>

struct sData sdata_static;
struct Data data;
struct rData rdata;
NativeConfig g_config;
u8 *gCTRNativeScratchpadBase;

#include <game/Vehicle/VehBirth.c>

static bool s_testCollisionSearch;
static u32 s_searchFlags;

// Allocation, terrain and motion are doubled.
struct Terrain *VehAfterColl_GetTerrain(u32 terrainType)
{
	static struct Terrain terrain;
	CTR_TEST_CHECK_EQ(terrainType, TERRAIN_NONE, "spawn initializes neutral terrain");
	return &terrain;
}

void COLL_SearchBSP_CallbackQUADBLK(const SVec3 *top, const SVec3 *bottom, struct ScratchpadStruct *sps, s32 radius)
{
	(void)top; (void)bottom; (void)radius;
	if (!s_testCollisionSearch)
	{
		CtrTest_Bail("appearance initialization must not search level geometry");
	}
	s_searchFlags = sps->Union.QuadBlockColl.searchFlags;
	sps->boolDidTouchQuadblock = true;
	sps->hit.plane.normal.y = FP_ONE;
}
u32 VehFrameInst_GetNumAnimFrames(struct Instance *inst, int animIndex)
{
	(void)inst; (void)animIndex;
	if (!s_testCollisionSearch)
	{
		CtrTest_Bail("appearance initialization must not read model animations");
	}
	return 1;
}
int VehFrameInst_GetStartFrame(int animIndex, int numFrames)
{
	(void)animIndex; (void)numFrames;
	if (!s_testCollisionSearch)
	{
		CtrTest_Bail("appearance initialization must not read model animations");
	}
	return 0;
}
s16 *AH_WarpPad_GetSpawnPosRot(s16 *posData)
{
	(void)posData;
	CtrTest_Bail("appearance initialization must not access warp pads");
	return NULL;
}
void CAM_StartOfRace(struct CameraDC *camera)
{
	(void)camera;
	CtrTest_Bail("appearance initialization must not initialize cameras");
}
void VehPhysProc_Driving_Init(struct Thread *thread, struct Driver *driver)
{
	(void)thread; (void)driver;
	CtrTest_Bail("appearance initialization must not run driving physics");
}
void VehStuckProc_RevEngine_Init(struct Thread *thread, struct Driver *driver)
{
	(void)thread; (void)driver;
	CtrTest_Bail("appearance initialization must not run driving physics");
}
int MATH_Cos(u32 angle)
{
	(void)angle;
	CtrTest_Bail("appearance initialization must not calculate spawn positions");
	return 0;
}
int MATH_Sin(u32 angle)
{
	(void)angle;
	CtrTest_Bail("appearance initialization must not calculate spawn positions");
	return 0;
}
void BOTS_GotoStartingLine(struct Driver *driver)
{
	(void)driver;
	CtrTest_Bail("appearance initialization must not reposition bots");
}
int Chaos_EngineClassForDriver(const struct Driver *driver, int engineClass)
{
	(void)driver;
	return engineClass;
}

b32 EngineAudio_InitOnce(u32 soundID, u32 flags)
{
	(void)soundID; (void)flags;
	CtrTest_Bail("appearance initialization must not start audio");
	return false;
}
struct Instance *INSTANCE_Birth3D(struct Model *model, const char *name, struct Thread *thread)
{
	(void)model; (void)name; (void)thread;
	CtrTest_Bail("appearance initialization must not allocate model instances");
	return NULL;
}
struct Thread *PROC_BirthWithObject(ThreadFlags flags, void *tick, const char *name, struct Thread *relative)
{
	(void)flags; (void)tick; (void)name; (void)relative;
	CtrTest_Bail("appearance initialization must not allocate threads");
	return NULL;
}
struct Model *LOAD_GetExtraCharacterModelByName(char *name)
{
	(void)name;
	CtrTest_Bail("appearance initialization must not load character assets");
	return NULL;
}

static void TestSpawnScale(void)
{
	CTR_TEST_SECTION("Reused driver memory starts at normal scale in every mode");
	struct GameTracker tracker = {0};
	struct IconGroup tires = {0};
	struct Driver driver;
	struct Thread thread = {.object = &driver};
	const u32 modes[] = {ADVENTURE_MODE | ADVENTURE_ARENA, ARCADE_MODE};
	sdata->gGT = &tracker;
	tracker.iconGroup[0] = &tires;
	for (unsigned int i = 0; i < len(modes); i++)
	{
		tracker.gameMode1 = modes[i];
		// Birth clears only the retail prefix; the native tail may contain pool data.
		memset(&driver, 0xa5, sizeof(driver));
		driver.kartScale = 0.5f;
		memset(&driver, 0, DRIVER_NTSC_RETAIL_SIZE);
		CTR_TEST_CHECK_EQ(driver.kartScale, 0.5f, "retail initialization leaves a nonzero native-tail scale");
		VehBirth_TireSprites(&thread);
		CTR_TEST_CHECK_EQ(driver.kartScale, 1, "spawn replaces stale pool scale with normal scale");
		CTR_TEST_CHECK_EQ(driver.wheelSize, 0xccc, "spawn retains the retail wheel size");
		CTR_TEST_CHECK(driver.wheelSprites == ICONGROUP_GETICONS(&tires), "spawn uses the supplied tire sprites");
	}
	sdata->gGT = NULL;
}

static void TestCharacterWheels(void)
{
	CTR_TEST_SECTION("Character changes restore normal wheels or hide Oxide's wheels");
	struct Driver driver = {.kartScale = 2};
	const int characters[] = {CRASH_BANDICOOT, NITROS_OXIDE, NEO_CORTEX};
	const int sizes[] = {0xccc, 0, 0xccc};
	for (unsigned int i = 0; i < len(characters); i++)
	{
		VehBirth_SetWheelSizeForCharacter(&driver, characters[i]);
		CTR_TEST_CHECK_EQ(driver.wheelSize, sizes[i], "each character change replaces the previous wheel size");
		CTR_TEST_CHECK_EQ(driver.kartScale, 2, "character wheel selection preserves Chaos body scaling");
	}
}

static void TestOxideBirthWheels(void)
{
	CTR_TEST_SECTION("Oxide birth retains the menu exception until character selection");
	struct GameTracker tracker = {0};
	struct IconGroup tires = {0};
	struct Driver driver = {0};
	struct Thread thread = {.object = &driver};
	const int levels[] = {MAIN_MENU_LEVEL, CRASH_COVE};
	const int sizes[] = {0xccc, 0};
	sdata->gGT = &tracker;
	tracker.iconGroup[0] = &tires;
	GAME_CHARACTER_IDS[0] = NITROS_OXIDE;
	for (unsigned int i = 0; i < len(levels); i++)
	{
		tracker.levelID = levels[i];
		VehBirth_TireSprites(&thread);
		CTR_TEST_CHECK_EQ(driver.wheelSize, sizes[i], "only race birth hides Oxide's wheels immediately");
		VehBirth_SetWheelSizeForCharacter(&driver, NITROS_OXIDE);
		CTR_TEST_CHECK_EQ(driver.wheelSize, 0, "explicit character selection hides Oxide's wheels in either scene");
	}
	GAME_CHARACTER_IDS[0] = CRASH_BANDICOOT;
	sdata->gGT = NULL;
}

static void TestSpawnCollisionLod(void)
{
	CTR_TEST_SECTION("Spawn collision detail follows the loaded level, not just player count");
	struct mesh_info mesh = {0};
	struct Level level = {.ptr_mesh_info = &mesh};
	struct GameTracker tracker = {.level1 = &level};
	struct Instance instance = {0};
	struct Driver driver = {.instSelf = &instance};
	_Alignas(4) u8 scratchpad[0x1000] = {0};
	sdata->gGT = &tracker;
	gCTRNativeScratchpadBase = scratchpad;
	s_testCollisionSearch = true;
	for (int players = 1; players <= 4; players++)
	{
		tracker.numPlyrCurrGame = players;
		for (int highDetail = 0; highDetail <= 1; highDetail++)
		{
			sdata->highDetailSplitScreenLevel = highDetail;
			VehBirth_TeleportSelf(&driver, 0, 0);
			CTR_TEST_CHECK_EQ(s_searchFlags, players < 3 || highDetail ? COLL_SEARCH_HIGH_LOD : 0,
			                  "high-detail split-screen spawns use high-detail collision while retail 3/4P retains low detail");
			CTR_TEST_CHECK_EQ(driver.AxisAngle2_normalVec.y, FP_ONE, "spawn keeps the collision normal for kart orientation");
		}
	}
	s_testCollisionSearch = false;
	sdata->highDetailSplitScreenLevel = false;
	sdata->gGT = NULL;
	gCTRNativeScratchpadBase = NULL;
}

int main(void)
{
	TestSpawnScale();
	TestCharacterWheels();
	TestOxideBirthWheels();
	TestSpawnCollisionLod();
	return CtrTest_Summary();
}
