#include <shared/ctr_test.h>
#include <common.h>

struct sData sdata_static;

uint32_t NativeGpuLinks_FromHostPointerAt(const void *hostPtr, const char *file, int line, const char *function)
{
	(void)hostPtr;
	(void)file;
	(void)line;
	(void)function;
	CtrTest_Bail("level animation must not emit HUD primitive links");
	return 0;
}

#include <game/CTR/CTR_CycleTex.c>

int main(void)
{
	CTR_TEST_SECTION("levels without animated textures");
	CTR_CycleTex_LEV(NULL, 0);
	CTR_TEST_CHECK(1, "a missing level animation list is a no-op");

	CTR_TEST_SECTION("packed level animation list");
	static struct IconGroup4 textures[3];
	struct
	{
		struct AnimTex first;
		struct IconGroup4 *firstFrames[2];
		struct AnimTex second;
		struct IconGroup4 *secondFrames[3];
		u32 terminator;
	} list = {0};
	list.first.numFrames = len(list.firstFrames);
	list.first.frameOffset = 1;
	list.firstFrames[0] = &textures[0];
	list.firstFrames[1] = &textures[1];
	list.second.numFrames = len(list.secondFrames);
	list.second.frameSkip = 1;
	for (unsigned int i = 0; i < len(list.secondFrames); i++)
	{
		list.secondFrames[i] = &textures[i];
	}
	list.terminator = (u32)&list.first;

	CTR_CycleTex_LEV(&list.first, 4);
	CTR_TEST_CHECK(list.first.frameCurr == 1 && (void *)list.first.ptrActiveTex == &textures[1],
	               "the first animation applies its offset and loops through frames");
	CTR_TEST_CHECK(list.second.frameCurr == 2 && (void *)list.second.ptrActiveTex == &textures[2],
	               "the next packed animation applies its frame skip");

	CTR_CycleTex_LEV(&list.first, 6);
	CTR_TEST_CHECK(list.second.frameCurr == 0 && (void *)list.second.ptrActiveTex == &textures[0],
	               "the later animation wraps without treating the terminator as an entry");
	return CtrTest_Summary();
}
