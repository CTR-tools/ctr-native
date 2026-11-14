#include <shared/ctr_test.h>
#include <common.h>

NativeConfig g_config;
struct sData sdata_static;

#include "platform/native_pgxp.c"
#include "platform/native_gte_core.c"
#include "platform/native_inline_c.c"

static void LoadBank(int bank, int scale, int translation)
{
	const s16 rotation[9] = {scale, 0, 0, 0, scale, 0, 0, 0, scale};
	for (int word = 0; word < 4; word++)
	{
		CTC2(CTR_PackS16Pair(rotation[word * 2], rotation[word * 2 + 1]), bank * 8 + word);
	}
	CTC2((u32)(s32)rotation[8], bank * 8 + 4);
	for (int axis = 0; axis < 3; axis++)
	{
		CTC2((u32)(translation + axis), bank * 8 + 5 + axis);
	}
}

static void TestSelectedBanks(void)
{
	CTR_TEST_SECTION("MVMVA precision falls back to the selected register banks");
	for (int mode = PGXP_MODE_GEOMETRY; mode <= PGXP_MODE_PERSPECTIVE; mode++)
	{
		for (int mx = 0; mx < 3; mx++)
		{
			for (int cv = 0; cv < 4; cv++)
			{
				// CV=2 has the hardware's far-color bug and no PGXP counterpart.
				if (cv == 2)
				{
					continue;
				}
				NativePgxp_EndFrame();
				memset(&gteRegs, 0, sizeof(gteRegs));
				g_config.pgxpMode = mode;
				LoadBank(0, 4096, 100);
				LoadBank(1, 2048, 200);
				LoadBank(2, -4096, 300);
				const s16 input[3] = {40, 80, 120};
				CTR_GteLoadS16TripletV0(input);
				doCOP2((1 << 19) | (mx << 17) | (cv << 13) | 0x12);
				const s16 ir[3] = {C2_IR1, C2_IR2, C2_IR3};
				float precise[3];
				CTR_TEST_CHECK(NativePgxp_GteGetMvmvaResult(ir, precise), "MVMVA exposes its precise result");
				for (int axis = 0; axis < 3; axis++)
				{
					// All operations here are exact integers: PGXP must agree with IR,
					// including CV=3's zero translation, not silently use bank zero.
					CTR_TEST_CHECK_NEAR(precise[axis], ir[axis], "without shadows, PGXP and integer MVMVA use the same selected banks");
				}
			}
		}
	}
}

static void TestEvictedClipTransform(void)
{
	CTR_TEST_SECTION("near clipping survives eviction of the view-projection shadow");
	NativePgxp_EndFrame();
	g_config.pgxpMode = PGXP_MODE_GEOMETRY;
	LoadBank(0, 4096, 0);
	CTC2(0, 6);
	CTC2(0, 7);
	LoadBank(1, 2048, 200);
	const s16 rotation[9] = {2048, 0, 0, 0, 2048, 0, 0, 0, 2048};
	const s32 translation[3] = {200, 201, 202};
	const double preciseRotation[9] = {2048, 0, 0, 0, 2048, 0, 0, 0, 2048};
	const double preciseTranslation[3] = {200.25, 201.25, 202.25};
	static u32 keys[32768];
	const void *viewKey = &keys[0];
	const void *collidingKey = NULL;
	// Reproduce the address-dependent eviction caused by another model matrix.
	// Internal access is needed only to locate an actual direct-map collision.
	for (size_t i = 1; i < len(keys); i++)
	{
		if (NativePgxp_TransformSlot(viewKey) == NativePgxp_TransformSlot(&keys[i]))
		{
			collidingKey = &keys[i];
			break;
		}
	}
	CTR_TEST_CHECK(collidingKey != NULL, "the fixture contains a colliding matrix address");
	for (int evicted = 0; evicted <= 1; evicted++)
	{
		NativePgxp_SetTransform(viewKey, rotation, translation, preciseRotation, preciseTranslation);
		if (evicted)
		{
			NativePgxp_SetTransform(collidingKey, rotation, translation, preciseRotation, preciseTranslation);
		}
		NativePgxp_LoadTransform(viewKey, rotation, translation, 1);
		NativePgxp_LoadTransform(viewKey, rotation, translation, 5);
		const s16 input[3] = {40, 80, 120};
		CTR_GteLoadS16TripletV0(input);
		// LLV0BK: the terminal clipping pass transforms world positions through
		// bank 1 while bank 0 holds an identity matrix for re-projection.
		gte_llv0bk_b();
		const s16 ir[3] = {C2_IR1, C2_IR2, C2_IR3};
		float precise[3];
		CTR_TEST_CHECK(NativePgxp_GteGetMvmvaResult(ir, precise), "the clip transform has a precise result");
		for (int axis = 0; axis < 3; axis++)
		{
			CTR_TEST_CHECK_NEAR(precise[axis], ir[axis] + (evicted ? 0.0 : 0.25),
			                    "eviction drops only the fractional precision, not the camera transform");
		}
	}
}

int main(void)
{
	TestSelectedBanks();
	TestEvictedClipTransform();
	return CtrTest_Summary();
}
