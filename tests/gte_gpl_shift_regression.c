#include <macros.h>
#include <psx/gtereg.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                 \
	do                                                                   \
	{                                                                    \
		if (!(condition))                                                \
		{                                                                \
			fprintf(stderr, "failed: %s at %d\n", #condition, __LINE__); \
			return 1;                                                    \
		}                                                                \
	} while (0)

struct GplCase
{
	s32 mac;
	s16 ir0;
	s16 ir;
	s32 result[2];
	s16 saturated[2][2];
	u8 color[2];
	b8 colorClamped[2];
};

static int RunCase(const struct GplCase *test, int lane, int sf, int lm)
{
	GTERegisters expected;
	u32 saturationFlags;

	memset(&gteRegs, 0xa5, sizeof(gteRegs));
	C2_IR0 = test->ir0;
	C2_IR1 = C2_IR2 = C2_IR3 = 0;
	C2_MAC1 = C2_MAC2 = C2_MAC3 = 0;
	gteRegs.CP2D.p[9 + lane].sw.l = test->ir;
	gteRegs.CP2D.p[25 + lane].sd = test->mac;
	C2_CODE = 0x3e;
	expected = gteRegs;
	expected.CP2D.p[25 + lane].sd = test->result[sf];
	expected.CP2D.p[9 + lane].sw.l = test->saturated[sf][lm];
	expected.CP2D.r[20] = gteRegs.CP2D.r[21];
	expected.CP2D.r[21] = gteRegs.CP2D.r[22];
	expected.CP2D.r[22] = 0x3e000000u | ((u32)test->color[sf] << (8 * lane));

	CHECK(GTE_operator(0x3e | (sf << 19) | (lm << 10)) == 1);
	saturationFlags = (test->saturated[sf][lm] != test->result[sf] ? 1u << (24 - lane) : 0) | (test->colorClamped[sf] ? 1u << (21 - lane) : 0);
	CHECK((C2_FLAG & 0x01f80000u) == saturationFlags);
	// NOTE(aalhendi): Overflow flags retain the core's existing behavior; this test checks GPL scaling and saturation, not hardware accuracy.
	expected.CP2C.r[31] = C2_FLAG;
	CHECK(memcmp(&gteRegs, &expected, sizeof(gteRegs)) == 0);
	return 0;
}

struct MvmvaCase
{
	s32 value;
	s32 mac[2];
	s16 ir[2];
};

static int RunMvmvaCase(const struct MvmvaCase *test, int cv, int lane, int sf)
{
	GTERegisters expected;
	u32 saturationFlag;

	memset(&gteRegs, 0xa5, sizeof(gteRegs));
	for (int i = 0; i < 5; i++)
	{
		gteRegs.CP2C.r[i] = 0;
	}
	if (cv < 3)
	{
		for (int i = 0; i < 3; i++)
		{
			gteRegs.CP2C.p[(cv << 3) + 5 + i].sd = 0;
		}
		gteRegs.CP2C.p[(cv << 3) + 5 + lane].sd = test->value;
	}
	expected = gteRegs;
	for (int i = 0; i < 3; i++)
	{
		expected.CP2D.p[25 + i].sd = 0;
		expected.CP2D.p[9 + i].sw.l = 0;
	}
	if (cv < 2)
	{
		expected.CP2D.p[25 + lane].sd = test->mac[sf];
		expected.CP2D.p[9 + lane].sw.l = test->ir[sf];
	}
	CHECK(GTE_operator(0x12 | (cv << 13) | (sf << 19)) == 1);
	saturationFlag = cv == 3 || test->ir[sf] == test->mac[sf] ? 0 : 1u << (24 - lane);
	CHECK((C2_FLAG & 0x01c00000u) == saturationFlag);
	// NOTE(aalhendi): CV2 discards translation results but retains their saturation flags. Overflow flags are not a hardware-accuracy oracle.
	expected.CP2C.r[31] = C2_FLAG;
	CHECK(memcmp(&gteRegs, &expected, sizeof(gteRegs)) == 0);
	return 0;
}

int main(void)
{
	static const struct GplCase cases[] = {
	    {-5659, 0, 0, {-5659, -5659}, {{-5659, 0}, {-5659, 0}}, {0, 0}, {1, 1}},
	    {-8755, 0, 0, {-8755, -8755}, {{-8755, 0}, {-8755, 0}}, {0, 0}, {1, 1}},
	    {INT32_MIN, 0, 0, {INT32_MIN, INT32_MIN}, {{-32768, 0}, {-32768, 0}}, {0, 0}, {1, 1}},
	    {INT32_MAX, 0, 0, {INT32_MAX, INT32_MAX}, {{32767, 32767}, {32767, 32767}}, {255, 255}, {1, 1}},
	    {0, 0, 0, {0, 0}, {{0, 0}, {0, 0}}, {0, 0}, {0, 0}},
	    {-1, 0, 0, {-1, -1}, {{-1, 0}, {-1, 0}}, {0, 0}, {1, 1}},
	    {1, 0, 0, {1, 1}, {{1, 1}, {1, 1}}, {0, 0}, {0, 0}},
	    {-5659, 4096, 3, {6629, -5656}, {{6629, 6629}, {-5656, 0}}, {255, 0}, {1, 1}},
	    {-8755, -4096, 2, {-16947, -8757}, {{-16947, 0}, {-8757, 0}}, {0, 0}, {1, 1}},
	    {32760, 4096, 16, {98296, 32776}, {{32767, 32767}, {32767, 32767}}, {255, 255}, {1, 1}},
	};
	static const struct MvmvaCase translations[] = {
	    {-656, {-2686976, -656}, {-32768, -656}},
	    {INT32_MIN, {0, INT32_MIN}, {0, -32768}},
	    {INT32_MAX, {-4096, INT32_MAX}, {-4096, 32767}},
	    {0, {0, 0}, {0, 0}},
	};
	int count = 0;
	int translationCount = 0;

	for (unsigned int i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
	{
		for (int lane = 0; lane < 3; lane++)
		{
			for (int sf = 0; sf < 2; sf++)
			{
				for (int lm = 0; lm < 2; lm++)
				{
					if (RunCase(&cases[i], lane, sf, lm))
					{
						fprintf(stderr, "GPL case: MAC=%d IR0=%d IR=%d lane=%d sf=%d lm=%d\n", cases[i].mac, cases[i].ir0, cases[i].ir, lane + 1, sf, lm);
						return 1;
					}
					count++;
				}
			}
		}
	}
	printf("GPL signed-MAC regression passed: %d cases\n", count);
	for (unsigned int i = 0; i < sizeof(translations) / sizeof(translations[0]); i++)
	{
		for (int cv = 0; cv < 4; cv++)
			for (int lane = 0; lane < 3; lane++)
				for (int sf = 0; sf < 2; sf++)
				{
					CHECK(RunMvmvaCase(&translations[i], cv, lane, sf) == 0);
					translationCount++;
				}
	}
	printf("MVMVA signed-translation regression passed: %d cases\n", translationCount);
	return 0;
}
