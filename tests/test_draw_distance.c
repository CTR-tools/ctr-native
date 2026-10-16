// Pins the increase_draw_distance contract in
// include/platform/native_draw_distance.h.
//
// The header is header-only and every helper is pure, so this suite needs none
// of the platform unit and builds as a single file against the shared harness.
// Nothing here sets a config value: the flag is an argument, which is the point
// of the helpers taking it rather than reading g_config.
//
// Run it through CTest (`ctest -R draw_distance`), from the Run menu in CLion, or
// build the `test_draw_distance` target and run the binary directly.

#include <shared/ctr_test.h>

#include <platform/native_draw_distance.h>

// A value each helper is given, chosen so that dividing and multiplying it by
// the factor are distinguishable from returning it untouched.
#define TEST_DRAW_DISTANCE_VALUE 1600

int main(void)
{
	CTR_TEST_SECTION("the option off is retail, exactly");
	// The tempting mutation: making a helper always apply the factor, or applying
	// it to the wrong side of the value. Vanilla behaviour is the default and
	// every one of these four has to stay bit-identical to it.
	CTR_TEST_CHECK(NativeDrawDistance_FarClip(false) == NATIVE_DRAW_DISTANCE_RETAIL_FAR_CLIP,
	               "the off frustum clip is retail's own value");
	CTR_TEST_CHECK(NativeDrawDistance_GeomScreenHalf(TEST_DRAW_DISTANCE_VALUE, false) == TEST_DRAW_DISTANCE_VALUE,
	               "the off near-reject depth is the register read untouched");
	CTR_TEST_CHECK(NativeDrawDistance_ProjectedLodDistance(TEST_DRAW_DISTANCE_VALUE, false) == TEST_DRAW_DISTANCE_VALUE,
	               "the off model LOD depth is retail's projection untouched");
	CTR_TEST_CHECK(NativeDrawDistance_BspLodThreshold(TEST_DRAW_DISTANCE_VALUE, false) == TEST_DRAW_DISTANCE_VALUE,
	               "the off BSP LOD cutoff is retail's cutoff untouched");

	CTR_TEST_SECTION("the option on moves every threshold by the one factor");
	// The bug this pins: the four numbers had drifted apart (a 2x frustum clip, a
	// 3x BSP cutoff, a 4x near reject, a 4x model LOD), so no single factor
	// described the option. Asserting the signed ratio, not the result, is what
	// fails when one of them grows its own literal — and it pins the direction too,
	// so a helper that inverted would read as "extended" while culling harder than
	// retail rather than needing a check of its own.
	CTR_TEST_CHECK(NativeDrawDistance_FarClip(true) / NATIVE_DRAW_DISTANCE_RETAIL_FAR_CLIP == NATIVE_DRAW_DISTANCE_SCALE,
	               "the frustum clip is scaled by the shared factor");
	CTR_TEST_CHECK(TEST_DRAW_DISTANCE_VALUE / NativeDrawDistance_GeomScreenHalf(TEST_DRAW_DISTANCE_VALUE, true) ==
	                   NATIVE_DRAW_DISTANCE_SCALE,
	               "the near-reject depth is divided by the shared factor");
	CTR_TEST_CHECK(TEST_DRAW_DISTANCE_VALUE / NativeDrawDistance_ProjectedLodDistance(TEST_DRAW_DISTANCE_VALUE, true) ==
	                   NATIVE_DRAW_DISTANCE_SCALE,
	               "the model LOD depth is divided by the shared factor");
	CTR_TEST_CHECK(NativeDrawDistance_BspLodThreshold(TEST_DRAW_DISTANCE_VALUE, true) / TEST_DRAW_DISTANCE_VALUE ==
	                   NATIVE_DRAW_DISTANCE_SCALE,
	               "the BSP LOD cutoff is scaled by the shared factor");

	return CtrTest_Summary();
}