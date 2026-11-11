#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common.h>

struct sData sdata_static;

static unsigned copyCalls;

static void *VisibilityCheckedMemcpy(void *dst, const void *src, size_t size)
{
	if (dst == NULL || src == NULL)
	{
		fprintf(stderr, "visibility memcpy nonnull contract: dst=%s src=%s bytes=%zu prior_copies=%u\n", dst == NULL ? "NULL" : "valid",
		        src == NULL ? "NULL" : "valid", size, copyCalls);
		exit(EXIT_FAILURE);
	}
	copyCalls++;
	return memcpy(dst, src, size);
}

// NOTE(aalhendi): Check libc's nonnull contract through the real game
// providers, with one native header/global owner as in the unity build.
#define memcpy VisibilityCheckedMemcpy
#include "../game/CTR/CTR_Visibility.c"
#include "../game/MAIN/MainVisibility.c"
#undef memcpy

#define CHECK(condition)                                                 \
	do                                                                   \
	{                                                                    \
		if (!(condition))                                                \
		{                                                                \
			fprintf(stderr, "failed: %s at %d\n", #condition, __LINE__); \
			return 1;                                                    \
		}                                                                \
	} while (0)

struct VisibilityGuardedWord
{
	int before;
	int value;
	int after;
};

int main(void)
{
	static struct GameTracker tracker;
	static struct Level level;
	static struct mesh_info mesh;
	static struct VisMem visibility;
	static struct Driver driver;
	static struct BSP bsp[1];
	static struct QuadBlock quads[1];
	static struct WaterVert water[1];
	static struct TextureLayout texture;
	static int leafSource = 0x01020304, faceSource = 0x05060708, waterSource = 0x090a0b0c;
	struct VisibilityGuardedWord leaf = {0x11121314, 0, 0x15161718};
	struct VisibilityGuardedWord face = {0x21222324, 0, 0x25262728};
	struct VisibilityGuardedWord optional = {0x31323334, 0, 0x35363738};
	int count;

	tracker.level1 = &level;
	tracker.visMem1 = &visibility;
	tracker.numPlyrCurrGame = 1;
	tracker.drivers[0] = &driver;
	tracker.cameraDC[0].visLeafSrc = &leafSource;
	tracker.cameraDC[0].visFaceSrc = &faceSource;
	level.ptr_mesh_info = &mesh;
	level.visMem = &visibility;
	level.visOVertSrc = &waterSource;
	level.ptr_water = water;
	level.ptr_tex_waterEnvMap = &texture;
	mesh.numBspNodes = 1;
	mesh.bspRoot = bsp;
	mesh.numQuadBlock = 1;
	mesh.ptrQuadBlockArray = quads;
	visibility.visLeafList[0] = &leaf.value;
	visibility.visFaceList[0] = &face.value;
	CHECK(((u32)&leafSource & 3) == 0 && ((u32)&faceSource & 3) == 0);
	CHECK(((mesh.numBspNodes + 31) >> 5) * 4 == 4 && ((mesh.numQuadBlock + 31) >> 5) * 4 == 4);

	for (count = 0; count <= 1; count++)
	{
		level.numWaterVertices = count;
		visibility.visOVertList[0] = count ? &optional.value : NULL;
		visibility.visLeafSrc[0] = visibility.visFaceSrc[0] = NULL;
		leaf.value = face.value = optional.value = 0;
		copyCalls = 0;
		CHECK(level.configFlags == 0 && visibility.visOVertSrc[0] == NULL && tracker.cameraDC[0].visOVertSrc == NULL);
		MainFrame_VisMemFullFrame(&tracker, tracker.level1);
		CHECK(copyCalls == (unsigned)(2 + count));
		CHECK(leaf.value == leafSource && face.value == faceSource && optional.value == (count ? waterSource : 0));
		CHECK(leaf.before == 0x11121314 && leaf.after == 0x15161718);
		CHECK(face.before == 0x21222324 && face.after == 0x25262728);
		CHECK(optional.before == 0x31323334 && optional.after == 0x35363738);
		CHECK(visibility.visLeafSrc[0] == &leafSource && visibility.visFaceSrc[0] == &faceSource);
		CHECK(visibility.visOVertList[0] == (count ? &optional.value : NULL) && visibility.visOVertSrc[0] == NULL);
		CHECK(tracker.cameraDC[0].visOVertSrc == NULL && level.visOVertSrc == &waterSource);
		CHECK(tracker.level1 == &level && tracker.visMem1 == &visibility && level.ptr_mesh_info == &mesh);
		CHECK(leafSource == 0x01020304 && faceSource == 0x05060708 && waterSource == 0x090a0b0c);
	}
	puts("visibility contract PASS: zero-water and positive-water fallback, copies/canaries/owners");
	return 0;
}
