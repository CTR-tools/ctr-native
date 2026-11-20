#include <shared/ctr_test.h>
#include <common.h>

struct sData sdata_static;
struct Data data;
NativeConfig g_config;
static struct GameTracker s_tracker;
static struct DB s_buffer;
static PolyF4 s_quads[2];
static int s_quadCount;
static RECT s_outline;

#include <game/widescreen.c>
#include <game/UI/UI_Meter.c>

void GetPrimitiveMem(void **primitive, u32 size)
{
	(void)size;
	*primitive = s_quadCount < 2 ? &s_quads[s_quadCount++] : NULL;
}

void AddPrimitive(void *primitive, void *ot)
{
	(void)primitive;
	(void)ot;
}

void CTR_Box_DrawWireBox(RECT *rect, const Color *color, void *ot, struct PrimMem *memory)
{
	(void)color;
	(void)ot;
	(void)memory;
	s_outline = *rect;
}

void DebugFont_DrawNumbers(int index, int x, int y)
{
	(void)index;
	(void)x;
	(void)y;
}

void VehFire_Increment(struct Driver *driver, int reserves, u32 type, int fireLevel)
{
	(void)driver;
	(void)reserves;
	(void)type;
	(void)fireLevel;
}

void Voiceline_RequestPlay(u32 voiceID, u32 characterID, u32 characterID2)
{
	(void)voiceID;
	(void)characterID;
	(void)characterID2;
}

u32 NativeGpuLinks_FromHostPointerAt(const void *pointer, const char *file, int line, const char *function)
{
	(void)pointer;
	(void)file;
	(void)line;
	(void)function;
	return 1;
}

int main(void)
{
	sdata->gGT = &s_tracker;
	s_tracker.backBuffer = &s_buffer;
	CTR_TEST_SECTION("slide meter anchors in every aspect ratio and player layout");
	const int displayWidths[] = {49, 37, 41, 29};
	for (int aspect = 0; aspect < 4; aspect++)
	{
		g_config.aspectRatio = aspect;
		for (int players = 1; players <= 4; players++)
		{
			s_tracker.numPlyrCurrGame = players;
			for (int player = 0; player < players; player++)
			{
				struct Driver driver = {0};
				driver.driverID = player;
				driver.const_turboMaxRoom = 100;
				driver.const_turboLowRoomWarning = 25;
				const int x = players < 3 ? 498 : (player & 1) ? 315 : 244;
				const int y = player < 2 ? 102 : 212;
				const int width = displayWidths[aspect];
				const int right = players >= 3 && (player & 1) ? x - 49 + width : x;
				for (int room = 0; room <= 100; room += 50)
				{
					driver.turbo_MeterRoomLeft = room * ELAPSED_MS;
					s_quadCount = 0;
					UI_DrawSlideMeter(x, y, &driver);
					CTR_TEST_CHECK(s_quadCount == 2, "both fill and empty background are emitted");
					CTR_TEST_CHECK(s_outline.x == right - width && s_outline.w == width,
					               "right viewports retain the vanilla left edge; other layouts retain the right edge");
					CTR_TEST_CHECK(s_outline.y == y - (players >= 3 ? 3 : 7), "split layouts retain their meter height");
					CTR_TEST_CHECK(s_quads[1].v[0].pos.x == s_outline.x && s_quads[1].v[1].pos.x == right,
					               "the empty background matches the anchored outline");
					const int length = room == 0 ? 0 : 49 - room * 49 / 100;
					CTR_TEST_CHECK(s_quads[0].v[0].pos.x == right - length + Widescreen_FixedEdgeShift(length) &&
					                   s_quads[0].v[1].pos.x == right,
					               "the fill follows the same anchor at every boost level");
				}
			}
		}
	}
	return CtrTest_Summary();
}
