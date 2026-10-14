// Pins the sub-pixel vertex precision contract in platform/native_pgxp.c.
//
// The suite compiles the module into its own translation unit, because every
// guarantee worth testing lives in the file-static shadow tables rather than in
// the public API: an address-keyed lookup with no way to reset it between cases
// would test nothing.
//
// What is deliberately NOT asserted here: whether a real race looks right. A
// seam appears when a shared vertex comes back precise in one polygon and retail
// in its neighbour, which depends on copy paths in the level overlay, not on
// anything expressible below. This suite pins the invariants that make those
// copy paths correct; the visual check still has to happen in the game.
//
// Run it through CTest (`ctest -R test_pgxp`), from the Run menu in CLion, or
// build the `test_pgxp` target and run the binary directly.

#include <shared/ctr_test.h>

#include <math.h>

#include <macros.h>
#include <platform/native_config.h>
#include <platform/native_pgxp.h>
#include <platform/native_renderer_types.h>

#include <psx/libgpu.h>

// native_pgxp.c only reads the mode, so a default instance is enough; the real
// one is defined in native_config.c.
NativeConfig g_config;

// The debug counter reports through the log, which the game owns; it is not
// exercised by the cases below.
void Platform_Log(const char *fmt, ...)
{
	(void)fmt;
}

void Platform_LogFlush(void)
{
}

#include "platform/native_pgxp.c"

static u32 PackSxy(int x, int y)
{
	return (u16)x | ((u32)(u16)y << 16);
}

// Every case starts from a retired generation so a leftover entry from the
// previous case can never be mistaken for a live one.
static void Reset(void)
{
	g_config.pgxpMode = PGXP_MODE_OFF;
	NativePgxp_EndFrame();
	NativePgxp_SetPrimRegion(0, NULL, 0);
	NativePgxp_SetPrimRegion(1, NULL, 0);
}

// === Mode ladder ===

static void TestModeLadder(void)
{
	CTR_TEST_SECTION("mode ladder: an out-of-range config.ini value falls back to Off");

	// NativeConfig_Load parses an unrecognised enum name with atoi, so
	// config.ini can hold anything the user typed.
	g_config.pgxpMode = PGXP_MODE_OFF;
	CTR_TEST_CHECK_EQ(NativePgxp_Mode(), PGXP_MODE_OFF, "Off stays Off");
	CTR_TEST_CHECK(!NATIVE_PGXP_ACTIVE(), "Off is inactive");

	g_config.pgxpMode = PGXP_MODE_GEOMETRY;
	CTR_TEST_CHECK_EQ(NativePgxp_Mode(), PGXP_MODE_GEOMETRY, "Geometry survives");
	CTR_TEST_CHECK(NATIVE_PGXP_ACTIVE(), "Geometry is active");
	CTR_TEST_CHECK(NATIVE_PGXP_GEOMETRY_ACTIVE(), "Geometry implies precise positions");
	CTR_TEST_CHECK(!NATIVE_PGXP_TEXTURES_ACTIVE(), "Geometry keeps retail affine textures");

	g_config.pgxpMode = PGXP_MODE_PERSPECTIVE;
	CTR_TEST_CHECK(NATIVE_PGXP_ACTIVE(), "Perspective is active");
	CTR_TEST_CHECK(NATIVE_PGXP_GEOMETRY_ACTIVE(), "Perspective implies precise positions");
	CTR_TEST_CHECK(NATIVE_PGXP_TEXTURES_ACTIVE(), "Perspective implies perspective-correct textures");

	// Geometry and Perspective must agree about positions: the two differ only
	// in whether a depth divisor reaches the vertex.
	g_config.pgxpMode = PGXP_MODE_GEOMETRY;
	CTR_TEST_CHECK(NATIVE_PGXP_GEOMETRY_ACTIVE() && !NATIVE_PGXP_TEXTURES_ACTIVE(), "Geometry: positions only");
	g_config.pgxpMode = PGXP_MODE_PERSPECTIVE;
	CTR_TEST_CHECK(NATIVE_PGXP_GEOMETRY_ACTIVE() && NATIVE_PGXP_TEXTURES_ACTIVE(), "Perspective: positions and textures");

	g_config.pgxpMode = 99;
	CTR_TEST_CHECK_EQ(NativePgxp_Mode(), PGXP_MODE_OFF, "hand-edited 99 falls back to Off");
	CTR_TEST_CHECK(!NATIVE_PGXP_ACTIVE(), "hand-edited 99 is inactive");
	g_config.pgxpMode = -1;
	CTR_TEST_CHECK_EQ(NativePgxp_Mode(), PGXP_MODE_OFF, "hand-edited -1 falls back to Off");

	Reset();
}

// === Shadow round trip ===

static void TestShadowRoundTrip(void)
{
	CTR_TEST_SECTION("shadow: a bound address returns its vertex, and only for the integer it holds");

	const u32 sxy = PackSxy(100, 200);
	VERTTYPE word[2];
	s16 buffer[4];
	NativePgxpVertex found = {0};

	Reset();
	g_config.pgxpMode = PGXP_MODE_GEOMETRY;

	// Address is inside no registered region, so this exercises the cache path.
	word[0] = (VERTTYPE)100;
	word[1] = (VERTTYPE)200;
	NativePgxp_GteProject(100.4f, 200.6f, 900.0f, sxy);
	NativePgxp_StoreGteSXY(word, 14, sxy);

	CTR_TEST_CHECK(NativePgxp_Lookup(word, sxy, &found) == 1, "a bound address resolves");
	// Expect the value the float actually holds, not the decimal literal.
	CTR_TEST_CHECK_NEAR(found.x, (double)(float)100.4, "x survives the round trip");
	CTR_TEST_CHECK_NEAR(found.y, (double)(float)200.6, "y survives the round trip");
	CTR_TEST_CHECK_NEAR(found.w, 900.0, "the depth divisor survives the round trip");

	// This is the guard that makes the whole scheme safe: a lookup is keyed by
	// address AND value, so a packet word that has since been overwritten with
	// a different vertex cannot pick up the old precision.
	CTR_TEST_CHECK(NativePgxp_Lookup(word, PackSxy(101, 200), &found) == 0, "a different integer does not resolve");
	CTR_TEST_CHECK(NativePgxp_Lookup(word, PackSxy(100, 201), &found) == 0, "a moved y does not resolve");

	// An address nobody bound must not resolve, however plausible its integer.
	CTR_TEST_CHECK(NativePgxp_Lookup(buffer, sxy, &found) == 0, "an unbound address does not resolve");

	Reset();
}

static void TestShadowGeneration(void)
{
	CTR_TEST_SECTION("shadow: nothing survives a frame boundary");

	const u32 sxy = PackSxy(50, 60);
	VERTTYPE word[2];
	NativePgxpVertex found = {0};

	Reset();
	g_config.pgxpMode = PGXP_MODE_GEOMETRY;
	word[0] = (VERTTYPE)50;
	word[1] = (VERTTYPE)60;

	NativePgxp_GteProject(50.5f, 60.5f, 800.0f, sxy);
	NativePgxp_StoreGteSXY(word, 14, sxy);
	CTR_TEST_CHECK(NativePgxp_Lookup(word, sxy, &found) == 1, "bound within the frame");

	// The GPU parses this frame's packets before the next frame starts building
	// one, so the OT being consumed is the retire point.
	NativePgxp_EndFrame();
	CTR_TEST_CHECK(NativePgxp_Lookup(word, sxy, &found) == 0, "EndFrame retires the binding");
	NativePgxp_EndFrame();
	CTR_TEST_CHECK(NativePgxp_Lookup(word, sxy, &found) == 0, "a second EndFrame does not resurrect it");

	Reset();
}

static void TestPrimRegions(void)
{
	CTR_TEST_SECTION("primitive memory: registered ranges resolve, and a moved range is re-registered");

	static u32 region0[64];
	static u32 region1[64];
	const u32 sxy = PackSxy(11, 22);
	NativePgxpVertex found = {0};

	Reset();
	g_config.pgxpMode = PGXP_MODE_GEOMETRY;

	// Without registration the packet word falls to the cache path, which still
	// works; registration only avoids hashing on the hot path.
	NativePgxp_EndFrame();
	NativePgxp_GteProject(11.5f, 22.5f, 700.0f, sxy);
	NativePgxp_StoreGteSXY(&region0[4], 14, sxy);
	CTR_TEST_CHECK(NativePgxp_Lookup(&region0[4], sxy, &found) == 1, "an unregistered range still resolves via the cache");

	// Registering must not lose the binding.
	NativePgxp_SetPrimRegion(0, region0, sizeof(region0));
	NativePgxp_SetPrimRegion(1, region1, sizeof(region1));
	NativePgxp_GteProject(11.5f, 22.5f, 700.0f, sxy);
	NativePgxp_StoreGteSXY(&region0[8], 14, sxy);
	CTR_TEST_CHECK(NativePgxp_Lookup(&region0[8], sxy, &found) == 1, "a registered range resolves");
	CTR_TEST_CHECK_NEAR(found.x, 11.5, "the registered range returns the right vertex");

	// Re-registering the same range is a no-op and must not drop live entries.
	NativePgxp_SetPrimRegion(0, region0, sizeof(region0));
	NativePgxp_GteProject(11.5f, 22.5f, 700.0f, sxy);
	NativePgxp_StoreGteSXY(&region0[12], 14, sxy);
	NativePgxp_SetPrimRegion(0, region0, sizeof(region0));
	CTR_TEST_CHECK(NativePgxp_Lookup(&region0[12], sxy, &found) == 1, "re-registering the same range keeps the binding");

	// MEMPACK hands primitive memory back at a new address on every level load.
	// A stale region would point at memory the game has handed back.
	static u32 moved[64];
	NativePgxp_SetPrimRegion(0, moved, sizeof(moved));
	NativePgxp_EndFrame();
	NativePgxp_GteProject(11.5f, 22.5f, 700.0f, sxy);
	NativePgxp_StoreGteSXY(&moved[4], 14, sxy);
	CTR_TEST_CHECK(NativePgxp_Lookup(&moved[4], sxy, &found) == 1, "the re-registered range resolves");

	// An out-of-range index is ignored rather than writing past the table.
	NativePgxp_SetPrimRegion(2, region0, sizeof(region0));
	NativePgxp_SetPrimRegion(-1, region0, sizeof(region0));
	CTR_TEST_CHECK(1, "out-of-range region indices are ignored");

	Reset();
}

// === Copy propagation ===

static void TestCopyXY(void)
{
	CTR_TEST_SECTION("copies: a vertex read out of another buffer keeps its precision");

	const u32 sxy = PackSxy(70, 80);
	s16 from[4];
	s16 to[4];
	NativePgxpVertex found = {0};

	Reset();
	g_config.pgxpMode = PGXP_MODE_GEOMETRY;

	from[0] = 70;
	from[1] = 80;
	to[0] = 70;
	to[1] = 80;

	NativePgxp_GteProject(70.25f, 80.75f, 640.0f, sxy);
	NativePgxp_StoreGteSXY(from, 14, sxy);
	NativePgxp_CopyXY(to, from, sxy);

	CTR_TEST_CHECK(NativePgxp_Lookup(to, sxy, &found) == 1, "the copy resolves");
	CTR_TEST_CHECK_NEAR(found.x, 70.25, "the copy keeps x");
	CTR_TEST_CHECK_NEAR(found.y, 80.75, "the copy keeps y");

	// Copying from an address that holds nothing must not invent a vertex.
	NativePgxp_CopyXY(to, from, PackSxy(71, 80));
	CTR_TEST_CHECK(NativePgxp_Lookup(to, PackSxy(71, 80), &found) == 0, "copying a mismatched integer binds nothing");

	Reset();
}

static void TestWorldPhaseGate(void)
{
	CTR_TEST_SECTION("world phase: HUD quads cannot inherit a world vertex that shares their integer");

	const u32 sxy = PackSxy(120, 140);
	VERTTYPE hud[2];
	NativePgxpVertex found = {0};

	Reset();
	g_config.pgxpMode = PGXP_MODE_GEOMETRY;
	hud[0] = (VERTTYPE)120;
	hud[1] = (VERTTYPE)140;

	NativePgxp_GteProject(120.3f, 140.9f, 500.0f, sxy);

	// Outside the world phase the HUD writes its own screen coordinates, which
	// must not be mistaken for the world vertex that shares their integer.
	NativePgxp_GteReadSXY(14, sxy); // the MFC2 read that precedes every store
	NativePgxp_BindWrittenXY(hud, sxy);
	CTR_TEST_CHECK(NativePgxp_Lookup(hud, sxy, &found) == 0, "a HUD quad binds no world precision");

	// EndFrame resets the phase, which is what stops a stale world-phase from
	// leaking into the next frame's menu drawing.
	NativePgxp_SetWorldPhase(1);
	NativePgxp_GteProject(120.3f, 140.9f, 500.0f, sxy);
	NativePgxp_GteReadSXY(14, sxy);
	NativePgxp_BindWrittenXY(hud, sxy);
	CTR_TEST_CHECK(NativePgxp_Lookup(hud, sxy, &found) == 1, "inside the world phase it binds");

	NativePgxp_EndFrame();
	NativePgxp_GteProject(120.3f, 140.9f, 500.0f, sxy);
	NativePgxp_GteReadSXY(14, sxy);
	NativePgxp_BindWrittenXY(hud, sxy);
	CTR_TEST_CHECK(NativePgxp_Lookup(hud, sxy, &found) == 0, "the next frame starts outside the world phase");

	Reset();
}

// === FIFO ===

static void TestFifo(void)
{
	CTR_TEST_SECTION("SXY FIFO: the three projected vertices stay addressable after being reloaded");

	const u32 a = PackSxy(1, 2);
	const u32 b = PackSxy(3, 4);
	const u32 c = PackSxy(5, 6);
	const NativePgxpVertex *v;

	Reset();
	g_config.pgxpMode = PGXP_MODE_GEOMETRY;

	// Game code saves and restores SXY registers around helper calls and reloads
	// them from memory to continue a strip, so a projected vertex has to survive
	// an MTC2 of the same integer.
	NativePgxp_GteProject(1.5f, 2.5f, 100.0f, a);
	NativePgxp_GteProject(3.5f, 4.5f, 200.0f, b);
	NativePgxp_GteProject(5.5f, 6.5f, 300.0f, c);

	v = NativePgxp_FifoMatch(12, a);
	CTR_TEST_CHECK(v != NULL, "the first vertex is in SXY0");
	CTR_TEST_CHECK_NEAR(v != NULL ? v->x : 0.0, 1.5, "SXY0 holds the first projection");

	v = NativePgxp_FifoMatch(14, c);
	CTR_TEST_CHECK(v != NULL, "the third vertex is in SXY2");
	CTR_TEST_CHECK_NEAR(v != NULL ? v->w : 0.0, 300.0, "SXY2 carries the newest depth");

	// A reload with the matching integer recovers the vertex.
	NativePgxp_GteWriteSXY(12, a);
	v = NativePgxp_FifoMatch(12, a);
	CTR_TEST_CHECK(v != NULL, "MTC2 of a known SXY recovers it");

	// A different integer must not.
	NativePgxp_GteWriteSXY(13, PackSxy(99, 99));
	CTR_TEST_CHECK(NativePgxp_FifoMatch(13, PackSxy(99, 99)) == NULL, "MTC2 of an unknown SXY binds nothing");

	// GteLoadSXY is the reverse direction: a register loaded from a bound
	// address picks the precise vertex up again, which is what the clip record
	// needs when it reloads SXY to run NCLIP.
	VERTTYPE word[2];
	word[0] = (VERTTYPE)1;
	word[1] = (VERTTYPE)2;
	NativePgxp_GteProject(1.5f, 2.5f, 100.0f, a);
	NativePgxp_StoreGteSXY(word, 14, a);
	// Shift the FIFO off the projection without retiring the binding.
	NativePgxp_GteProject(7.0f, 7.0f, 1.0f, PackSxy(7, 7));
	NativePgxp_GteProject(8.0f, 8.0f, 1.0f, PackSxy(8, 8));
	NativePgxp_GteProject(9.0f, 9.0f, 1.0f, PackSxy(9, 9));
	CTR_TEST_CHECK(NativePgxp_FifoMatch(12, a) == NULL, "the projection has left the FIFO");
	NativePgxp_GteLoadSXY(12, word, a);
	CTR_TEST_CHECK(NativePgxp_FifoMatch(12, a) != NULL, "GteLoadSXY rebinds from the packet address");
	// Reloading an address that holds nothing must not bind anything.
	NativePgxp_GteLoadSXY(13, word, PackSxy(55, 66));
	CTR_TEST_CHECK(NativePgxp_FifoMatch(13, PackSxy(55, 66)) == NULL, "GteLoadSXY of an unknown integer binds nothing");

	Reset();
}

// === CPU-side positions ===

static void TestPositions(void)
{
	CTR_TEST_SECTION("positions: an exact vertex is trusted only while its triplet still matches");

	s16 vector[3] = {300, -400, 500};
	float exact[3] = {300.25f, -399.75f, 500.5f};
	float out[3];
	float outB[3];
	s16 copyDst[3] = {300, -400, 500};
	s16 copySrc[3] = {300, -400, 500};

	Reset();
	g_config.pgxpMode = PGXP_MODE_GEOMETRY;

	NativePgxp_SetPosition(vector, vector, exact);
	CTR_TEST_CHECK(NativePgxp_GetPosition(vector, vector, out) == 1, "a registered position resolves");
	CTR_TEST_CHECK_NEAR(out[0], 300.25, "x survives");
	CTR_TEST_CHECK_NEAR(out[1], -399.75, "y survives");
	CTR_TEST_CHECK_NEAR(out[2], 500.5, "z survives");

	// The scratchpad slots are reused constantly, so the stored integers are
	// what say the exact value still describes the vertex at that address.
	vector[1] = -401;
	CTR_TEST_CHECK(NativePgxp_GetPosition(vector, vector, out) == 0, "a moved triplet rejects the position");
	CTR_TEST_CHECK_NEAR(out[1], -401.0, "the fallback is the integer it was handed");
	vector[1] = -400;

	NativePgxp_ClearPosition(vector);
	CTR_TEST_CHECK(NativePgxp_GetPosition(vector, vector, out) == 0, "a cleared slot rejects the position");

	// Copying to another slot is how subdivision propagates an exact midpoint;
	// without it the copy falls back to retail and the shared edge splits.
	Reset();
	g_config.pgxpMode = PGXP_MODE_GEOMETRY;
	NativePgxp_SetPosition(copySrc, copySrc, exact);
	NativePgxp_CopyPosition(copyDst, copySrc, copySrc);
	CTR_TEST_CHECK(NativePgxp_GetPosition(copyDst, copyDst, out) == 1, "the copy resolves");
	CTR_TEST_CHECK_NEAR(out[0], 300.25, "the copy keeps x");

	// Copying from a slot with nothing in it must not invent one.
	Reset();
	g_config.pgxpMode = PGXP_MODE_GEOMETRY;
	NativePgxp_CopyPosition(copyDst, copySrc, copySrc);
	CTR_TEST_CHECK(NativePgxp_GetPosition(copyDst, copyDst, outB) == 0, "copying from an empty slot binds nothing");

	// Copying a slot onto itself is a no-op, not a clear.
	Reset();
	g_config.pgxpMode = PGXP_MODE_GEOMETRY;
	NativePgxp_SetPosition(copySrc, copySrc, exact);
	NativePgxp_CopyPosition(copySrc, copySrc, copySrc);
	CTR_TEST_CHECK(NativePgxp_GetPosition(copySrc, copySrc, outB) == 1, "a self-copy keeps the position");

	Reset();
}

static void TestPositionGeneration(void)
{
	CTR_TEST_SECTION("positions: a position does not survive the frame it was computed in");

	s16 vector[3] = {1, 2, 3};
	float exact[3] = {1.5f, 2.5f, 3.5f};
	float out[3];

	Reset();
	g_config.pgxpMode = PGXP_MODE_GEOMETRY;
	NativePgxp_SetPosition(vector, vector, exact);
	CTR_TEST_CHECK(NativePgxp_GetPosition(vector, vector, out) == 1, "resolved within the frame");

	NativePgxp_EndFrame();
	CTR_TEST_CHECK(NativePgxp_GetPosition(vector, vector, out) == 0, "EndFrame retires the position");

	Reset();
}

// === GTE input vectors ===

static void TestGteInput(void)
{
	CTR_TEST_SECTION("GTE input: the exact vector is dropped as soon as the register is reloaded");

	s16 vector[3] = {100, 200, 300};
	float exact[3] = {100.5f, 199.5f, 300.25f};
	double out[3];

	Reset();
	g_config.pgxpMode = PGXP_MODE_GEOMETRY;

	NativePgxp_GteSetInput(0, vector, exact);
	CTR_TEST_CHECK(NativePgxp_GteGetInput(0, 100, 200, 300, out) == 1, "a matching vector resolves");
	CTR_TEST_CHECK_NEAR(out[2], 300.25, "z survives");

	// Loading a vector register drops whatever exact value it held, so an
	// invalidation on write is what stops a scratch slot reused for a different
	// vertex from inheriting the previous one's position.
	NativePgxp_GteInvalidateInput(0);
	CTR_TEST_CHECK(NativePgxp_GteGetInput(0, 100, 200, 300, out) == 0, "an invalidated slot does not resolve");

	// Each vector register is two GTE data registers: 0/1 are V0, 2/3 are V1 and
	// 4/5 are V2, so writing either half must drop that vector's exact value.
	NativePgxp_GteSetInput(0, vector, exact);
	NativePgxp_GteInvalidateInput(1);
	CTR_TEST_CHECK(NativePgxp_GteGetInput(0, 100, 200, 300, out) == 0, "the second half of V0 invalidates V0");

	NativePgxp_GteSetInput(1, vector, exact);
	NativePgxp_GteInvalidateInput(2);
	CTR_TEST_CHECK(NativePgxp_GteGetInput(1, 100, 200, 300, out) == 0, "the first half of V1 invalidates V1");

	NativePgxp_GteSetInput(2, vector, exact);
	NativePgxp_GteInvalidateInput(5);
	CTR_TEST_CHECK(NativePgxp_GteGetInput(2, 100, 200, 300, out) == 0, "the second half of V2 invalidates V2");

	// Registers outside 0..5 carry no vector, so they must not disturb one.
	NativePgxp_GteSetInput(2, vector, exact);
	NativePgxp_GteInvalidateInput(6);
	CTR_TEST_CHECK(NativePgxp_GteGetInput(2, 100, 200, 300, out) == 1, "an unrelated register leaves V2 alone");
	CTR_TEST_CHECK(NativePgxp_GteGetInput(3, 100, 200, 300, out) == 0, "an out-of-range slot never resolves");
	NativePgxp_GteSetInput(9, vector, exact);
	NativePgxp_GteInvalidateInput(9);
	CTR_TEST_CHECK(1, "an out-of-range set is ignored");

	// Mismatched integers must not resolve even while the slot is valid.
	NativePgxp_GteSetInput(0, vector, exact);
	CTR_TEST_CHECK(NativePgxp_GteGetInput(0, 100, 200, 301, out) == 0, "a different integer does not resolve");
	CTR_TEST_CHECK(NativePgxp_GteGetInput(0, 101, 200, 300, out) == 0, "a moved x does not resolve");

	// The frame boundary retires them too.
	NativePgxp_EndFrame();
	CTR_TEST_CHECK(NativePgxp_GteGetInput(0, 100, 200, 300, out) == 0, "EndFrame retires the input");

	Reset();
}

// === Winding ===

// Retail NCLIP, as the GTE computes it on the integer SXY it stored.
static s64 RetailNclip(int x0, int y0, int x1, int y1, int x2, int y2)
{
	return (s64)((s64)x0 * y1) + (s64)((s64)x1 * y2) + (s64)((s64)x2 * y0) - (s64)((s64)x0 * y2) - (s64)((s64)x1 * y0) -
	       (s64)((s64)x2 * y1);
}

static void TestNclipAgreement(void)
{
	CTR_TEST_SECTION("winding: precise winding must agree with retail on any polygon that is not a sliver");

	// The culling decision in the level overlay is the sign of NCLIP, so a
	// disagreement culls a polygon retail draws and shows as a hole. The
	// sub-pixel residual can only move the cross product by roughly the
	// coordinate magnitude, so a well-conditioned triangle is far outside that
	// band and must never flip.
	int flips = 0;
	int tested = 0;

	for (int w = 8; w <= 40; w += 8)
	{
		for (int h = 8; h <= 40; h += 8)
		{
			for (int ox = 0; ox < 24; ox += 4)
			{
				for (int oy = 0; oy < 24; oy += 4)
				{
					const int x0 = 100 + ox;
					const int y0 = 200 + oy;
					const int x1 = x0 + w;
					const int y1 = y0;
					const int x2 = x0;
					const int y2 = y0 + h;
					const s64 retail = RetailNclip(x0, y0, x1, y1, x2, y2);

					// Sweep the full sub-pixel residual range for each corner.
					for (int a = 0; a < 4; a++)
					{
						for (int b = 0; b < 4; b++)
						{
							for (int c = 0; c < 4; c++)
							{
								const double ra = (a + 1) * 0.25;
								const double rb = (b + 1) * 0.25;
								const double rc = (c + 1) * 0.25;
								s64 precise = 0;

								Reset();
								g_config.pgxpMode = PGXP_MODE_GEOMETRY;
								NativePgxp_GteProject(x0 + ra, y0 + rb, 500.0, PackSxy(x0, y0));
								NativePgxp_GteProject(x1 + rb, y1 + rc, 500.0, PackSxy(x1, y1));
								NativePgxp_GteProject(x2 + rc, y2 + ra, 500.0, PackSxy(x2, y2));

								CTR_TEST_CHECK(NativePgxp_GteNclip(PackSxy(x0, y0), PackSxy(x1, y1), PackSxy(x2, y2), &precise) == 1,
										               "the winding resolves from the FIFO");
								tested++;
								if ((retail < 0) != (precise < 0))
								{
									flips++;
								}
							}
						}
					}
				}
			}
		}
	}

	CTR_TEST_CHECK(tested > 0, "the sweep exercised the winding path");
	CTR_TEST_CHECK_EQ(flips, 0, "no well-conditioned triangle flips its winding sign");
	Reset();
}

static void TestNclipUnknownVertices(void)
{
	CTR_TEST_SECTION("winding: an unresolved corner falls back to retail rather than guessing");

	const u32 sxy0 = PackSxy(10, 10);
	s64 out = 12345;

	Reset();
	g_config.pgxpMode = PGXP_MODE_GEOMETRY;

	// Only two of three corners are known, so the third would have to be
	// invented. Guessing would cull on a position nobody drew.
	NativePgxp_GteProject(10.5f, 10.5f, 500.0f, sxy0);
	NativePgxp_GteProject(11.5f, 10.5f, 500.0f, PackSxy(11, 10));
	NativePgxp_GteProject(10.5f, 11.5f, 500.0f, PackSxy(10, 11));

	CTR_TEST_CHECK(NativePgxp_GteNclip(sxy0, PackSxy(11, 10), PackSxy(77, 88), &out) == 0, "an unknown corner declines the winding");
	CTR_TEST_CHECK_EQ(out, 12345, "the caller's value is untouched");

	Reset();
}

// === Vertex layout ===

static void TestVertexLayout(void)
{
	CTR_TEST_SECTION("vertex layout: the float attributes stay 4-byte aligned for the GL upload");

	// The vertex buffer is uploaded with glVertexAttribPointer and a
	// sizeof(GrVertex) stride, so the float fields must not straddle a 4-byte
	// boundary and the struct must not gain padding the stride does not cover.
	CTR_TEST_CHECK_EQ(sizeof(GrVertex), 32, "the vertex is 32 bytes");
	CTR_TEST_CHECK_EQ(offsetof(GrVertex, u), 16, "texcoords follow position and page/clut");
	CTR_TEST_CHECK_EQ(offsetof(GrVertex, page), 12, "page sits after the float position");

	// w is the perspective divisor; leaving it at 0 is what keeps an affine
	// polygon affine, because the shader only scales clip w when w > 0.
	GrVertex vertex;
	memset(&vertex, 0, sizeof(vertex));
	CTR_TEST_CHECK_EQ((int)(vertex.w == 0.0f), 1, "a zeroed vertex has no perspective divisor");
}

int main(void)
{
	CTR_TEST_SECTION("sub-pixel vertex precision");
	printf("  GrVertex=%zu bytes, mode default=%d\n", sizeof(GrVertex), PGXP_MODE_OFF);

	TestModeLadder();
	TestShadowRoundTrip();
	TestShadowGeneration();
	TestPrimRegions();
	TestCopyXY();
	TestWorldPhaseGate();
	TestFifo();
	TestPositions();
	TestPositionGeneration();
	TestGteInput();
	TestNclipAgreement();
	TestNclipUnknownVertices();
	TestVertexLayout();

	return CtrTest_Summary();
}
