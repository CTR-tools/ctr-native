#include <shared/ctr_test.h>
#include <water_visibility.h>

int main(void)
{
	CTR_TEST_SECTION("water visibility defaults");
	uint32_t result[2] = {0, 0};
	WaterVisibility_CopyDefault(result, NULL, sizeof(result));
	CTR_TEST_CHECK(result[0] == UINT32_MAX && result[1] == UINT32_MAX,
	               "a missing mask makes vertices visible across word boundaries");
	const uint32_t authored[] = {0x10001, 0x40};
	WaterVisibility_CopyDefault(result, authored, sizeof(result));
	CTR_TEST_CHECK(result[0] == authored[0] && result[1] == authored[1], "an authored mask is preserved");
	WaterVisibility_CopyDefault(NULL, NULL, 0);
	const uint32_t *cursor = NULL;
	CTR_TEST_CHECK(WaterVisibility_NextWord(&cursor) == UINT32_MAX && WaterVisibility_NextWord(&cursor) == UINT32_MAX,
	               "missing animation masks provide visible bits for every word");
	cursor = authored;
	CTR_TEST_CHECK(WaterVisibility_NextWord(&cursor) == authored[0] && WaterVisibility_NextWord(&cursor) == authored[1],
	               "authored animation masks advance normally");
	return CtrTest_Summary();
}
