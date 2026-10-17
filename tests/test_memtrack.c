// Pins the reporting behaviour in platform/native_memtrack.c.
//
// The suite compiles the tracker into its own translation unit because every
// guarantee worth testing lives in its file-static peaks: a high-water mark that
// reaches the log through a private step threshold cannot be observed from
// outside, and the arithmetic is the part reading a log cannot prove on its own.
//
// What is deliberately NOT asserted here: whether the numbers match the game.
// That a level fills more primMem at an increased draw distance than at retail
// is a measurement, and it takes a real run. This suite pins that whatever the
// game reports, the tracker keeps the highest value seen and folds the leaf
// tallies into per-frame maxima, so the number in the log means what it says.
//
// Run it through CTest (`ctest -R test_memtrack`), from the Run menu in CLion,
// or build the `test_memtrack` target and run the binary directly.

#include <shared/ctr_test.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <common.h>
#include <platform/native_memtrack.h>

// Declared extern by regionsEXE.h; the linker wants the storage even though the
// suite reads only the rendered-quadblock array and the tracker out of it.
struct sData sdata_static;

// Platform_Log lives in native_log.c, which brings the log file and the base
// directory with it. What matters here is the text, so the suite captures it.
#define TEST_MEMTRACK_LOG_SIZE 16384
static char s_log[TEST_MEMTRACK_LOG_SIZE];
static int s_logLines;

// The only renderer symbol the tracker reaches for is where the list starts.
// The suite points it at the retail array, which is the branch whose capacity is
// a count of that array rather than a constant.
static struct QuadBlock **DrawLevelOvr1P_GetRenderedOverflowBase(void)
{
	return sdata_static.quadBlocksRendered;
}

#include "platform/native_memtrack.c"

// Mirrors Platform_LogWrite exactly: it writes what it is given and adds nothing.
// An earlier version of this stub appended a newline of its own, which made the
// report's line breaks look right here and wrong in the log file.
void Platform_Log(const char *fmt, ...)
{
	va_list args;
	size_t used = strlen(s_log);

	if (used < sizeof(s_log))
	{
		va_start(args, fmt);
		vsnprintf(s_log + used, sizeof(s_log) - used, fmt, args);
		va_end(args);
	}

	s_logLines++;
}

// === Fixtures ===

// Every case starts from a cleared tracker and a cleared log. The tracker reports
// on a growth step rather than on every observation, so a case that carried the
// previous one's state in would see no report at all and pass for the wrong
// reason.
static void Reset(void)
{
	memset(s_log, 0, sizeof(s_log));
	s_logLines = 0;
	NativeMemTrack_ResetPeaks();
}

// Stands in for a frame ending: this is what folds the per-frame leaf tallies
// into the per-frame maxima and closes the report.
static void SampleFrame(void)
{
	NativeMemTrack_SampleArenas(&sdata_static.gameTracker);
}

static bool LogContains(const char *needle)
{
	return strstr(s_log, needle) != NULL;
}

static int CountLines(void)
{
	int lines = 0;

	for (size_t i = 0; i < TEST_MEMTRACK_LOG_SIZE && s_log[i] != '\0'; i++)
	{
		if (s_log[i] == '\n')
		{
			lines++;
		}
	}

	return lines;
}

// === The high-water mark ===

static void TestPeakHoldsAtTheHighestValueSeen(void)
{
	CTR_TEST_SECTION("a buffer peak is the highest value seen, not the latest");
	Reset();

	NativeMemTrack_Observe(NATIVE_MEMTRACK_PRIM_MEM_0, 4096, 8192);
	// A later frame using less is ordinary: the camera turned away from a wall.
	// Taking the latest value would make the report fall every time it did.
	NativeMemTrack_Observe(NATIVE_MEMTRACK_PRIM_MEM_0, 64, 8192);

	CTR_TEST_CHECK(LogContains("prim0 4096/8192"), "primMem0 reports the 4096 it reached, not the 64 that came after");
}

static void TestPeaksAreKeptPerBuffer(void)
{
	CTR_TEST_SECTION("buffers are tracked independently");
	Reset();

	NativeMemTrack_Observe(NATIVE_MEMTRACK_OT_MEM_0, 128, 512);

	CTR_TEST_CHECK(LogContains("ot0 128/512"), "otMem0 carries its own value");
	// A tracker writing every line from one running total would print the same
	// number seven times.
	CTR_TEST_CHECK(!LogContains("ot1 128/512"), "otMem1 is untouched by an observation of otMem0");
}

static void TestAnUnknownCapacityKeepsTheLastKnownOne(void)
{
	CTR_TEST_SECTION("an observation with no capacity does not erase the one already known");
	Reset();

	NativeMemTrack_Observe(NATIVE_MEMTRACK_FRAME_VERTICES, 100, 524288);
	NativeMemTrack_Observe(NATIVE_MEMTRACK_FRAME_VERTICES, 777, 0);

	// Every caller passes a real capacity, so the zero case is a guard rather than
	// a path. What it must not do is turn the line into a division by nothing,
	// which reads as a buffer filled past its end.
	CTR_TEST_CHECK(LogContains("verts 777/524288"), "the capacity stays at the last one a caller supplied");
}

// === Leaf tallies ===

static void TestLeafTalliesArePerFrameMaximaNotTotals(void)
{
	CTR_TEST_SECTION("a leaf tally is the busiest frame, not the running total");
	Reset();

	for (int i = 0; i < 5; i++)
	{
		NativeMemTrack_CountLeafSlot(RENDER_LIST_SLOT_WATER);
	}

	SampleFrame();

	for (int i = 0; i < 2; i++)
	{
		NativeMemTrack_CountLeafSlot(RENDER_LIST_SLOT_WATER);
	}

	SampleFrame();

	// Summing the frames would read 7, which is a claim about the whole run and
	// cannot be compared against a buffer capacity the way these numbers can.
	CTR_TEST_CHECK(LogContains("water 5"), "leafWater reports the 5-leaf frame, not the 7 counted across both");
}

static void TestEachRejectIsCountedSeparately(void)
{
	CTR_TEST_SECTION("the culls are counted apart, so a full traversal stack stays visible");
	Reset();

	NativeMemTrack_CountLeafReject(NATIVE_MEMTRACK_LEAF_REJECTED_STACK);
	NativeMemTrack_CountLeafReject(NATIVE_MEMTRACK_LEAF_REJECTED_BOX);
	SampleFrame();

	// Both reach the same report, so folding them into one counter would bury the
	// case that costs geometry: a drop that is not a cull at all.
	CTR_TEST_CHECK(LogContains("stack 1"), "the stack drop is counted on its own");
	CTR_TEST_CHECK(LogContains("box 1"), "the box cull is counted on its own");
	CTR_TEST_CHECK(LogContains("frustum 0"), "a cull that did not fire reads zero rather than being absent");
}

// === The list walk ===

static void TestListCountMeasuresFromTheStartOfTheArray(void)
{
	CTR_TEST_SECTION("a cursor walk is counted from where it started");
	Reset();

	// The cursor has to sit inside the array the tracker measures against: the
	// difference between two unrelated addresses is not a distance.
	struct QuadBlock **base = DrawLevelOvr1P_GetRenderedOverflowBase();
	NativeMemTrack_ObserveRenderedQuadblocks(&base[9]);

	CTR_TEST_CHECK(LogContains("quads 9/256"), "nine entries are reported against the 256 the retail array holds");
}

static void TestACursorBeforeTheStartIsRejected(void)
{
	CTR_TEST_SECTION("a cursor before the start reports nothing rather than wrapping");
	Reset();

	struct QuadBlock **base = DrawLevelOvr1P_GetRenderedOverflowBase();
	NativeMemTrack_ObserveRenderedQuadblocks(&base[0]);
	NativeMemTrack_ObserveRenderedQuadblocks(&base[4] - 8);
	// Zero entries and the rejected call both leave the peak where it was, so the
	// report has to be provoked by an unrelated buffer before it can be read.
	NativeMemTrack_Observe(NATIVE_MEMTRACK_DRAW_SPLITS, 1, 4096);

	// Subtracting a cursor that sits before the base yields a huge number, and a
	// huge peak is exactly what a reader would take for a buffer overflow.
	CTR_TEST_CHECK(LogContains("quads 0/256"), "the out-of-range cursor is ignored, leaving the peak where it was");
}

// === The report ===

static void TestTheFirstObservationReportsEverything(void)
{
	CTR_TEST_SECTION("the first observation reports, however small");
	Reset();

	NativeMemTrack_Observe(NATIVE_MEMTRACK_DRAW_SPLITS, 1, 4096);

	// Without this a run whose peaks never moved would write an empty log, and an
	// empty log is indistinguishable from a tracker that never ran.
	CTR_TEST_CHECK(LogContains("splits 1/4096"), "a single observation is enough to produce a report");

	// The whole report goes out as one call with its breaks inside, so the log
	// file shows a header and three lines rather than a run-on paragraph.
	CTR_TEST_CHECK_EQ(CountLines(), 4, "a marker line, then memory, slots and dropped");
	CTR_TEST_CHECK_EQ(s_logLines, 1, "the report is a single call, so it cannot be split by other output");
	CTR_TEST_CHECK(!LogContains("(null)"), "no tracked value is left without a name");
}

static void TestTheLogStepsInsteadOfRepeatingEveryFrame(void)
{
	CTR_TEST_SECTION("the log is written on growth, not on every frame");
	Reset();

	NativeMemTrack_Observe(NATIVE_MEMTRACK_PRIM_MEM_1, 1000, 8192);
	CTR_TEST_CHECK_EQ(s_logLines, 1, "the first observation writes one report");

	// A camera moving inside one level keeps setting a slightly higher peak. One
	// report per new maximum buries everything else in the log file.
	NativeMemTrack_Observe(NATIVE_MEMTRACK_PRIM_MEM_1, 1001, 8192);
	NativeMemTrack_Observe(NATIVE_MEMTRACK_PRIM_MEM_1, 1002, 8192);

	CTR_TEST_CHECK_EQ(s_logLines, 1, "growth too small to matter does not add another report");
}

static void TestResetPeaksStartsOver(void)
{
	CTR_TEST_SECTION("a new level or configuration starts from zero");
	Reset();

	NativeMemTrack_Observe(NATIVE_MEMTRACK_PRIM_MEM_0, 4096, 8192);
	NativeMemTrack_CountLeafReject(NATIVE_MEMTRACK_LEAF_REJECTED_STACK);
	SampleFrame();

	Reset();
	// Re-establishing the capacity does not raise the peak, so the line can be
	// read as cleared-but-sized rather than merely absent.
	NativeMemTrack_Observe(NATIVE_MEMTRACK_PRIM_MEM_0, 0, 8192);
	NativeMemTrack_Observe(NATIVE_MEMTRACK_DRAW_SPLITS, 1, 4096);

	// The arenas are sized from these peaks, so a level that inherited the
	// previous one's would size the next arena from a level it never saw.
	CTR_TEST_CHECK(LogContains("prim0 0/8192"), "the buffer peak starts again from zero");
	CTR_TEST_CHECK(LogContains("stack 0"), "the leaf tallies start again from zero");
}

int main(void)
{
	TestPeakHoldsAtTheHighestValueSeen();
	TestPeaksAreKeptPerBuffer();
	TestAnUnknownCapacityKeepsTheLastKnownOne();
	TestLeafTalliesArePerFrameMaximaNotTotals();
	TestEachRejectIsCountedSeparately();
	TestListCountMeasuresFromTheStartOfTheArray();
	TestACursorBeforeTheStartIsRejected();
	TestTheFirstObservationReportsEverything();
	TestTheLogStepsInsteadOfRepeatingEveryFrame();
	TestResetPeaksStartsOver();

	return CtrTest_Summary();
}