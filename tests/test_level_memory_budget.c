// Pins that every per-level arena still fits inside the mempack it is carved out
// of, and that one shared factor scales all of them.
//
// The suite compiles game/MAIN/MainDB.c into its own translation unit so it uses
// the real size tables rather than a copy: a copy would drift, and the drift is
// exactly what this suite exists to catch. Everything those tables depend on is a
// plain integer here, so the whole table can be walked without a disc, an asset
// or a window.
//
// What is deliberately NOT asserted here: that a level uses less than it was
// given. Peak usage is a measurement, it needs a real run, and it belongs in the
// memtrack log. This suite pins the part that is arithmetic — that the numbers we
// chose to allocate agree with each other and with the arena they come from.

#include <shared/ctr_test.h>

#include <common.h>
#include <platform/native_memtrack.h>

// The arena the arenas above are carved out of, and the size the budget is
// measured against. Compiled in so the number is the real one.
#include "platform/native_memory.c"

// Declared extern by regionsEXE.h; MainDB_GetClipSize reads the split-screen flag
// out of it, and the linker wants the storage regardless.
struct sData sdata_static;

// The two things MainDB.c calls but does not define. MEMPACK_AllocMem traps when
// the arena is full, which is the failure this suite is about to make impossible,
// so it is never reached: the budget is arithmetic, not an allocation.
void *MEMPACK_AllocMem(s32 size, const char *name)
{
	(void)size;
	(void)name;
	return NULL;
}

void NativePgxp_SetPrimRegion(int index, const void *start, size_t size)
{
	(void)index;
	(void)start;
	(void)size;
}

#include "game/MAIN/MainDB.c"

// The largest primitive arena the game can ask for, from the constant branches of
// MainInit_GetPrimMemSize. That function also has per-level branches reading a
// table loaded from the disc, which cannot be walked here; the constants are
// what a level falls back to without it, and the largest of them bounds the rest.
#define TEST_PRIM_MEM_RETAIL_MAX 0x25800

// What a level holds at once: both double buffers of each arena, plus the two
// per-player swap chains from MainInit_OTMem. Every one of these comes from
// MEMPACK_AllocMem, so if they do not fit, the game traps rather than degrades.
static int MempackUsedByLevel(u32 gameMode1, int numPlyrCurrGame, u32 levelID)
{
	const int primMem = TEST_PRIM_MEM_RETAIL_MAX * MAINDB_ARENA_SCALE;
	const int swapChain = ((numPlyrCurrGame << 12) | 0x18);

	return (2 * primMem) + (2 * MainDB_GetOTMemSize(gameMode1, numPlyrCurrGame)) + (2 * swapChain) + MainDB_GetClipSize(levelID, numPlyrCurrGame);
}

// The mode bits that pick an arena size, as a walkable set.
static const u32 k_gameModes[] = {0, MAIN_MENU, ADVENTURE_ARENA, BATTLE_MODE};

// Levels that name a different clip size, plus one that does not, so the default
// branch is walked too.
static const u32 k_levelIDs[] = {ADVENTURE_GARAGE, MAIN_MENU_LEVEL, SEWER_SPEEDWAY, MYSTERY_CAVES, PAPU_PYRAMID, POLAR_PASS, 0x7F};

// === The budget ===

static void TestEveryLevelFitsTheMempack(void)
{
	CTR_TEST_SECTION("every level's arenas fit the mempack arena together");
	sdata->highDetailSplitScreenLevel = 0;

	const int arenaBytes = Platform_GetMempackBackingSize();

	for (u32 m = 0; m < len(k_gameModes); m++)
	{
		for (int players = 1; players <= 4; players++)
		{
			for (u32 l = 0; l < len(k_levelIDs); l++)
			{
				CTR_TEST_CHECK(MempackUsedByLevel(k_gameModes[m], players, k_levelIDs[l]) < arenaBytes,
				               "a level's double-buffered arenas fit the mempack arena");
			}
		}
	}
}

static void TestSplitScreenDoesNotChangeTheClipSize(void)
{
	CTR_TEST_SECTION("the high-detail split-screen path asks for no more clip space");
	sdata->highDetailSplitScreenLevel = 1;

	const int split = MainDB_GetClipSize(PAPU_PYRAMID, 4);

	sdata->highDetailSplitScreenLevel = 0;

	// The flag forces the player count back to 1 rather than scaling anything, so
	// a change that multiplied instead would look like extra memory here.
	CTR_TEST_CHECK(split <= MainDB_GetClipSize(PAPU_PYRAMID, 1), "the split-screen clip size is no larger than the 1P one");
}

static void TestTheBudgetLeavesRoomForTheLevel(void)
{
	CTR_TEST_SECTION("the arenas leave most of the mempack for the level itself");
	sdata->highDetailSplitScreenLevel = 0;

	const int arenaBytes = Platform_GetMempackBackingSize();
	const int used = MempackUsedByLevel(0, 1, 0);

	// The arenas are a floor, not the whole cost: meshes, instances, particles and
	// decals come out of the same arena. This guards the opposite mistake, an
	// arena so small that a level's own allocations cannot fit.
	CTR_TEST_CHECK(used < arenaBytes / 2, "the arenas take under half the mempack");
}

// === The scale ===

static void TestEveryArenaIsScaledFromRetail(void)
{
	CTR_TEST_SECTION("every arena is retail's size times the one shared factor");

	// The bug this pins: the primitive arena grew by a bare literal while the
	// ordering tables went through the shared factor, so the two drifted apart
	// with nothing to notice.
	CTR_TEST_CHECK_EQ(MainDB_GetOTMemSize(0, 1) / 0x2000, MAINDB_ARENA_SCALE, "the 1P ordering table uses the shared factor");
	CTR_TEST_CHECK_EQ(MainDB_GetOTMemSize(BATTLE_MODE, 4) / 0x8000, MAINDB_ARENA_SCALE, "the battle ordering table uses the same factor");
	CTR_TEST_CHECK_EQ(MainDB_GetClipSize(PAPU_PYRAMID, 1) / 3000, MAINDB_ARENA_SCALE, "the clip buffer uses the same factor as the arenas");
	CTR_TEST_CHECK_EQ(MainDB_GetClipSize(MAIN_MENU_LEVEL, 1) / 16, MAINDB_ARENA_SCALE, "the menu's clip buffer is scaled like the rest");
}

static void TestTheScaleDoesNotOverflow(void)
{
	CTR_TEST_SECTION("the scaled sizes stay inside the int the allocators take");

	// MainDB_PrimMem takes a u32 and the mempack counts in s32, so a factor large
	// enough to push the largest arena past 2 GiB would silently wrap rather than
	// fail to allocate.
	const int largest = TEST_PRIM_MEM_RETAIL_MAX * MAINDB_ARENA_SCALE;

	CTR_TEST_CHECK(largest > 0, "the scaled primitive arena is positive");
	CTR_TEST_CHECK(largest < 0x40000000, "the scaled primitive arena stays under 1 GiB");
}

int main(void)
{
	TestEveryLevelFitsTheMempack();
	TestSplitScreenDoesNotChangeTheClipSize();
	TestTheBudgetLeavesRoomForTheLevel();
	TestEveryArenaIsScaledFromRetail();
	TestTheScaleDoesNotOverflow();

	return CtrTest_Summary();
}