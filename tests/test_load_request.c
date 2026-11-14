#include <shared/ctr_test.h>
#include <common.h>

struct sData sdata_static;
NativeConfig g_config;
static struct GameTracker s_tracker;
static int s_transitions;
static int s_offscreen;

#include <game/MAIN/MainRaceTrack.c>

void howl_StopAudio(int a, int b, int c) { (void)a; (void)b; (void)c; }
void ElimBG_Deactivate(struct GameTracker *tracker) { (void)tracker; }
void LOAD_LevelFile(int level) { (void)level; }
b32 RaceFlag_IsFullyOffScreen(void) { return s_offscreen; }
void RaceFlag_BeginTransition(s32 direction) { (void)direction; s_transitions++; }
void RaceFlag_ResetTextAnim(void) {}

int main(void)
{
	CTR_TEST_SECTION("repeated load requests preserve the transition");
	for (int mode = LOADING_SCREEN_FADE; mode >= 0; mode--)
	{
		memset(&sdata_static, 0, sizeof(sdata_static));
		memset(&s_tracker, 0, sizeof(s_tracker));
		sdata->gGT = &s_tracker;
		sdata->Loading.stage = LOAD_IDLE;
		g_config.loadingScreen = mode;
		s_offscreen = 1;
		s_transitions = 0;
		MainRaceTrack_RequestLoad(1);
		for (int frame = 0; frame < 10; frame++)
		{
			MainRaceTrack_RequestLoad(1);
		}
		CTR_TEST_CHECK(s_transitions == 1, "pending requests do not restart the transition");
		MainRaceTrack_RequestLoad(2);
		CTR_TEST_CHECK(sdata->Loading.Lev_ID_To_Load == 2 && s_transitions == 1, "a pending destination can change without restarting the transition");
		sdata->Loading.stage = LOAD_IDLE;
		MainRaceTrack_RequestLoad(1);
		CTR_TEST_CHECK(s_transitions == 2, "a later load starts a new transition");
	}
	return CtrTest_Summary();
}
