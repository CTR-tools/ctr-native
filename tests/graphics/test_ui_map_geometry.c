#include <ui_map_geometry.h>
#include <shared/ctr_test.h>

int main(void)
{
	CTR_TEST_SECTION("map coordinates");
	CTR_TEST_CHECK(UI_Map_ScaleX(400, 500, 1000) == 400, "4:3 preserves map coordinates");
	CTR_TEST_CHECK(UI_Map_ScaleX(400, 500, 750) == 425, "16:9 compresses the map around its right edge");
	CTR_TEST_CHECK(UI_Map_ScaleX(500, 500, 750) == 500, "the map right edge stays anchored");
	CTR_TEST_CHECK(UI_Map_ScaleX(350, 440, 563) == 390, "three-player coordinates use the shifted anchor");
	CTR_TEST_CHECK(UI_Map_ScaleX(450, 500, 750) == 463, "markers use the same transform as the map");
	return CtrTest_Summary();
}
