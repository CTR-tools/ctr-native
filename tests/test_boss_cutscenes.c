#include <shared/ctr_test.h>
#include <common.h>

struct sData sdata_static;
struct Data data;
NativeConfig g_config;
static struct GameTracker s_tracker;
static struct GamepadSystem s_pads;
static int s_stops;
static int s_resumes;
static int s_loads;
static int s_prompts;
static int s_levelRequests;
static s16 s_requestedLevel;
static s16 s_promptX;
static s16 s_promptY;

const char *LANG_GetString(s32 index)
{
	(void)index;
	return "PRESS START TO SKIP";
}

// Use the real scene catalogue and decoder: the skip path must find each
// script's terminal action past animation, audio, and subtitle operands.
#include <game/233/R233.c>
#include <game/233/CS_ScriptCmd.c>
#include <game/233/CS_Camera.c>

char *CS_OVR233_TranslateRetailOpcodePointer(char *ptr)
{
	return ptr;
}

void CDSYS_XAPauseForce(void)
{
	s_stops++;
}

void CS_DestroyPodium_StartDriving(void)
{
	s_resumes++;
}

void CS_LoadBoss(const struct BossCutsceneData *scene)
{
	(void)scene;
	s_loads++;
}

void MEMPACK_SwapPacks(int index)
{
	(void)index;
}

struct Thread *CS_Thread_Init(s32 model, const char *name, struct CsThreadInitData *init, s16 yaw, struct Thread *parent)
{
	(void)model;
	(void)name;
	(void)init;
	(void)yaw;
	(void)parent;
	return NULL;
}
void DecalFont_DrawLine(char *text, s16 x, s16 y, s16 font, s16 flags)
{
	(void)text;
	(void)font;
	(void)flags;
	s_prompts++;
	s_promptX = x;
	s_promptY = y;
}

void VehStuckProc_RIP_Init(struct Thread *thread, struct Driver *driver)
{
	(void)thread;
	(void)driver;
}

void TakeCupProgress_Activate(s16 stringIndex)
{
	(void)stringIndex;
}

int CAM_Path_GetNumPoints(void)
{
	return 0;
}

u8 CAM_Path_Move(int frame, s16 *pos, s16 *rot, s16 *flags)
{
	(void)frame;
	(void)pos;
	(void)rot;
	(void)flags;
	return 0;
}
void MainRaceTrack_RequestLoad(s16 level)
{
	s_levelRequests++;
	s_requestedLevel = level;
}

b32 VehPickupItem_MaskBoolGoodGuy(struct Driver *driver)
{
	(void)driver;
	return 1;
}

int CDSYS_XAPlay(int category, int index)
{
	(void)category;
	(void)index;
	return 0;
}

void RaceFlag_SetDrawOrder(b32 after)
{
	(void)after;
}

static void Reset(int mode, int index, CutscenePhase phase)
{
	memset(&sdata_static, 0, sizeof(sdata_static));
	memset(&s_tracker, 0, sizeof(s_tracker));
	memset(&s_pads, 0, sizeof(s_pads));
	memset(&g_config, 0, sizeof(g_config));
	CS_OVR233_ResetData();
	sdata->gGT = &s_tracker;
	sdata->gGamepads = &s_pads;
	g_config.bossCutscenes = mode;
	CS_BOSS_INDEX = index;
	CS_PHASE = phase;
	s_tracker.gameMode2 = VEH_FREEZE_PODIUM;
	s_tracker.podiumRewardID = STATIC_KEY;
	s_stops = s_resumes = s_loads = s_prompts = 0;
	s_levelRequests = 0;
}

static void TestPodiumExitRequestsLoadOnce(void)
{
	CTR_TEST_SECTION("podium exit requests the menu once");
	for (int skip = 0; skip <= 1; skip++)
	{
		struct CsPodiumCameraThreadObj podium = {0};
		struct Thread camera = {0};
		struct Driver driver = {0};
		Reset(BOSS_CUTSCENES_UNSKIPPABLE, ROO_START, CS_WAIT_INPUT);
		g_config.skipPodiumCeremonies = skip;
		g_config.loadingScreen = LOADING_SCREEN_FADE;
		s_tracker.drivers[0] = &driver;
		camera.object = &podium;
		if (!skip)
		{
			s_pads.gamepad[0].buttonsTapped = BTN_CROSS_one;
		}

		CS_Camera_ThTick_Podium(&camera);
		CTR_TEST_CHECK(s_levelRequests == 1 && s_requestedLevel == MAIN_MENU_LEVEL, "podium exit requests the main menu");
		CTR_TEST_CHECK((camera.flags & THREAD_FLAG_DEAD) != 0, "the camera retires so it cannot restart the loading fade each frame");
	}
}

static void TestModesAndPrompt(void)
{
	CTR_TEST_SECTION("default playback and Start-to-skip");
	struct Thread camera = {0};
	Reset(BOSS_CUTSCENES_UNSKIPPABLE, ROO_START, CS_WAIT_END);
	s_pads.gamepad[0].buttonsTapped = BTN_START;
	CS_Camera_ThTick_Boss(&camera);
	CTR_TEST_CHECK(s_stops == 0 && s_resumes == 0 && camera.flags == 0, "Start cannot skip vanilla scenes");
	CTR_TEST_CHECK(s_prompts == 0, "vanilla scenes have no skip prompt");

	Reset(BOSS_CUTSCENES_SKIPPABLE, ROO_START, CS_WAIT_END);
	CS_Camera_ThTick_Boss(&camera);
	CTR_TEST_CHECK(s_prompts == 1 && s_promptX == 256 && s_promptY < 166, "skip prompt is centered above the subtitle box");
	CTR_TEST_CHECK(s_resumes == 0, "skippable scenes wait for input");
	s_pads.gamepad[0].buttonsTapped = BTN_START | BTN_CROSS_one;
	sdata->AnyPlayerTap = BTN_START | BTN_CROSS_one;
	CS_Camera_ThTick_Boss(&camera);
	CTR_TEST_CHECK(s_stops == 1 && s_resumes == 1, "Start stops dialogue and resumes driving");
	CTR_TEST_CHECK(s_pads.gamepad[0].buttonsTapped == BTN_CROSS_one, "skip consumes Start before the per-player pause check without clearing other buttons");
	CTR_TEST_CHECK(sdata->AnyPlayerTap == BTN_CROSS_one, "skip also consumes the aggregate Start tap");
	CTR_TEST_CHECK((camera.flags & THREAD_FLAG_DEAD) != 0 && CS_FINISHED == 1, "skipped scene and camera finish together");
	CTR_TEST_CHECK(s_tracker.podiumRewardID == NOFUNC && (s_tracker.gameMode2 & VEH_FREEZE_PODIUM) == 0, "skip clears the reward handoff and freeze");
	CTR_TEST_CHECK(s_tracker.overlayTransition == 3 && s_tracker.pushBuffer_UI.fadeFromBlack_desiredResult == FP_ONE, "skip schedules overlay restoration and clears a pending fade");
}

static void TestAllAutomaticSkips(void)
{
	CTR_TEST_SECTION("every real boss script can be bypassed before model loading");
	for (int index = 0; index < BOSS_CUTSCENE_COUNT; index++)
	{
		struct Thread camera = {0};
		struct Thread actor = {0};
		Reset(BOSS_CUTSCENES_SKIP, index, CS_WAIT_INPUT);
		s_tracker.threadBuckets[OTHER].thread = &actor;
		CS_Camera_ThTick_Boss(&camera);
		CTR_TEST_CHECK(s_loads == 0 && s_prompts == 0, "automatic skip needs neither boss assets nor a prompt");
		CTR_TEST_CHECK((actor.flags & THREAD_FLAG_DEAD) != 0, "skip retires remaining ceremony actors");
		if (index == PINSTRIPE_BEAT)
		{
			CTR_TEST_CHECK(CS_BOSS_INDEX == OXIDE_TROPHIES && s_resumes == 0, "Pinstripe hands off to Oxide without restoring gameplay early");
			CS_Camera_ThTick_Boss(&camera);
		}
		CTR_TEST_CHECK(CS_FINISHED == 1 && s_resumes == 1 && (camera.flags & THREAD_FLAG_DEAD) != 0, "automatic skip returns to gameplay");
	}

	struct Thread camera = {0};
	Reset(BOSS_CUTSCENES_SKIPPABLE, PINSTRIPE_BEAT, CS_WAIT_END);
	s_tracker.currAdvProfile.numRelics = ADV_OXIDE_FINAL_RELIC_COUNT;
	s_pads.gamepad[0].buttonsTapped = BTN_START;
	CS_Camera_ThTick_Boss(&camera);
	CTR_TEST_CHECK(CS_BOSS_INDEX == OXIDE_RELICS_GEMSTONE && CS_PHASE == CS_WAIT_INPUT, "Pinstripe uses the relic challenge when all relics are owned");
	CTR_TEST_CHECK(s_resumes == 0 && camera.flags == 0, "a manual skip leaves the chained scene available to watch");
	CTR_TEST_CHECK((s_pads.gamepad[0].buttonsTapped & BTN_START) == 0, "a chained scene skip also consumes the pause input");
}

int main(void)
{
	TestModesAndPrompt();
	TestAllAutomaticSkips();
	TestPodiumExitRequestsLoadOnce();
	return CtrTest_Summary();
}
