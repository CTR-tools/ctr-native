#include "platform/native_memtrack.h"

#if defined(CTR_DIAGNOSTICS)
#include <platform/native_log.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// A frame sets a slightly new maximum often, so a line per new maximum would
// bury the log. Reporting only once the combined peak has grown by an eighth
// shows the curve converging over a lap instead.
#define NATIVE_MEMTRACK_REPORT_STEP_DIVISOR 8

// The renderer's slot enum has no count member of its own.
enum
{
	NATIVE_MEMTRACK_SLOT_COUNT = RENDER_LIST_SLOT_FULL_DYNAMIC + 1,
};

static u32 s_peaks[NATIVE_MEMTRACK_BUFFER_COUNT];
static u32 s_capacities[NATIVE_MEMTRACK_BUFFER_COUNT];

// Leaf counts are tallied per frame and folded into the peaks at the end of it,
// so the peaks are per-frame maxima and stay comparable to a buffer capacity.
// The two leaf groups differ only in what indexes them, so they share one shape.
static struct
{
	u32 frame[NATIVE_MEMTRACK_SLOT_COUNT];
	u32 peaks[NATIVE_MEMTRACK_SLOT_COUNT];
} s_slots;

static struct
{
	u32 frame[NATIVE_MEMTRACK_LEAF_REJECT_COUNT];
	u32 peaks[NATIVE_MEMTRACK_LEAF_REJECT_COUNT];
} s_rejects;

static u32 s_reportedTotal;
static int s_reportedOnce;

static u32 NativeMemTrack_Sum(const u32 *values, u32 count)
{
	u32 total = 0;

	for (u32 i = 0; i < count; i++)
	{
		total += values[i];
	}

	return total;
}

// The report threshold watches every peak at once, so growing one group still
// lets the others count towards the next report.
static u32 NativeMemTrack_TotalPeak(void)
{
	return NativeMemTrack_Sum(s_peaks, NATIVE_MEMTRACK_BUFFER_COUNT) + NativeMemTrack_Sum(s_slots.peaks, NATIVE_MEMTRACK_SLOT_COUNT) +
	       NativeMemTrack_Sum(s_rejects.peaks, NATIVE_MEMTRACK_LEAF_REJECT_COUNT);
}

static void NativeMemTrack_Append(char *out, int outSize, int *used, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	*used += vsnprintf(out + *used, outSize - *used, fmt, args);
	va_end(args);
}

// The names live in the format rather than in a table looked up by enum, so the
// report is three lines a person reads at a glance and the compiler checks every
// value against its slot. An enum that gains a member without gaining a mention
// here reads as zero, which is visible rather than quietly wrong.
static void NativeMemTrack_Report(void)
{
	char report[1024];
	int used = snprintf(report, sizeof(report), "--- memtrack ---\n");

	NativeMemTrack_Append(report, sizeof(report), &used,
	                      "memory  prim0 %u/%u  prim1 %u/%u  ot0 %u/%u  ot1 %u/%u  quads %u/%u  verts %u/%u  splits %u/%u  clip %u/%u\n",
	                      s_peaks[NATIVE_MEMTRACK_PRIM_MEM_0], s_capacities[NATIVE_MEMTRACK_PRIM_MEM_0],
	                      s_peaks[NATIVE_MEMTRACK_PRIM_MEM_1], s_capacities[NATIVE_MEMTRACK_PRIM_MEM_1],
	                      s_peaks[NATIVE_MEMTRACK_OT_MEM_0], s_capacities[NATIVE_MEMTRACK_OT_MEM_0],
	                      s_peaks[NATIVE_MEMTRACK_OT_MEM_1], s_capacities[NATIVE_MEMTRACK_OT_MEM_1],
	                      s_peaks[NATIVE_MEMTRACK_RENDERED_QUADBLOCKS], s_capacities[NATIVE_MEMTRACK_RENDERED_QUADBLOCKS],
	                      s_peaks[NATIVE_MEMTRACK_FRAME_VERTICES], s_capacities[NATIVE_MEMTRACK_FRAME_VERTICES],
	                      s_peaks[NATIVE_MEMTRACK_DRAW_SPLITS], s_capacities[NATIVE_MEMTRACK_DRAW_SPLITS],
	                      s_peaks[NATIVE_MEMTRACK_CLIP_RECORDS], s_capacities[NATIVE_MEMTRACK_CLIP_RECORDS]);

	NativeMemTrack_Append(report, sizeof(report), &used, "slots   4x4 %u  subdiv %u  4x2 %u  4x1 %u  water %u  fulldyn %u\n",
	                      s_slots.peaks[RENDER_LIST_SLOT_4X4], s_slots.peaks[RENDER_LIST_SLOT_DYNAMIC_SUBDIV],
	                      s_slots.peaks[RENDER_LIST_SLOT_4X2], s_slots.peaks[RENDER_LIST_SLOT_4X1], s_slots.peaks[RENDER_LIST_SLOT_WATER],
	                      s_slots.peaks[RENDER_LIST_SLOT_FULL_DYNAMIC]);

	NativeMemTrack_Append(report, sizeof(report), &used, "dropped box %u  frustum %u  vis %u  stack %u\n",
	                      s_rejects.peaks[NATIVE_MEMTRACK_LEAF_REJECTED_BOX], s_rejects.peaks[NATIVE_MEMTRACK_LEAF_REJECTED_FRUSTUM],
	                      s_rejects.peaks[NATIVE_MEMTRACK_LEAF_REJECTED_VISIBILITY],
	                      s_rejects.peaks[NATIVE_MEMTRACK_LEAF_REJECTED_STACK]);

	// One call, with the breaks inside it: Platform_Log writes exactly the text it
	// is given and adds nothing, so a report sent a line at a time would land in
	// the log as a single run-on paragraph.
	Platform_Log("%s", report);

	s_reportedTotal = NativeMemTrack_TotalPeak();
	s_reportedOnce = 1;
}

static void NativeMemTrack_FoldTallies(u32 *frame, u32 *peaks, u32 count)
{
	for (u32 i = 0; i < count; i++)
	{
		if (frame[i] > peaks[i])
		{
			peaks[i] = frame[i];
		}

		frame[i] = 0;
	}
}

// Both the buffer and the leaf paths report off the same curve, so the decision
// to log lives here rather than being written out at each of them.
static void NativeMemTrack_ReportIfStepped(void)
{
	const u32 total = NativeMemTrack_TotalPeak();

	if (!s_reportedOnce || (total > s_reportedTotal + (s_reportedTotal / NATIVE_MEMTRACK_REPORT_STEP_DIVISOR)))
	{
		NativeMemTrack_Report();
	}
}

void NativeMemTrack_Observe(enum NativeMemTrackBuffer buffer, u32 used, u32 capacity)
{
	if ((buffer < 0) || (buffer >= NATIVE_MEMTRACK_BUFFER_COUNT))
	{
		return;
	}

	if (capacity != 0)
	{
		s_capacities[buffer] = capacity;
	}

	if (used <= s_peaks[buffer])
	{
		return;
	}

	s_peaks[buffer] = used;

	NativeMemTrack_ReportIfStepped();
}

// The rendered list is filled by a bare cursor walk with no end of its own, so
// the count is however far the cursor has got from the start of the array.
//
// The two facts below are also what 226 needs for its own bounds check, and they
// are worked out here again rather than borrowed from there on purpose: the
// tracker is committed on its own and must keep building against the renderer's
// retail surface, which stops at DrawLevelOvr1P_GetRenderedOverflowBase.
void NativeMemTrack_ObserveRenderedQuadblocks(const void *cursor)
{
	struct QuadBlock **base = DrawLevelOvr1P_GetRenderedOverflowBase();
	const struct QuadBlock *const *from = (const struct QuadBlock *const *)base;
	const struct QuadBlock *const *to = (const struct QuadBlock *const *)cursor;
	u32 capacity = NATIVE_HIGH_DETAIL_RENDERED_QUADBLOCK_CAPACITY;

	// One of two static arrays, depending on the level.
	if (base == sdata_static.quadBlocksRendered)
	{
		capacity = len(sdata_static.quadBlocksRendered);
	}

	if ((base == NULL) || (cursor == NULL) || (to < from))
	{
		return;
	}

	NativeMemTrack_Observe(NATIVE_MEMTRACK_RENDERED_QUADBLOCKS, (u32)(to - from), capacity);
}

void NativeMemTrack_CountLeafSlot(int renderListSlot)
{
	if ((renderListSlot < 0) || (renderListSlot >= NATIVE_MEMTRACK_SLOT_COUNT))
	{
		return;
	}

	s_slots.frame[renderListSlot]++;
}

void NativeMemTrack_CountLeafReject(enum NativeMemTrackLeafReject reject)
{
	if ((reject < 0) || (reject >= NATIVE_MEMTRACK_LEAF_REJECT_COUNT))
	{
		return;
	}

	s_rejects.frame[reject]++;
}

void NativeMemTrack_SampleArenas(struct GameTracker *gGT)
{
	// The DB about to be reset is the one the frame just finished filling, and
	// its cursor is a lower bound on that frame's peak within it.
	const struct DB *db = &gGT->db[gGT->swapchainIndex];

	NativeMemTrack_Observe((gGT->swapchainIndex == 0) ? NATIVE_MEMTRACK_PRIM_MEM_0 : NATIVE_MEMTRACK_PRIM_MEM_1,
	                       (u32)((u8 *)db->primMem.cursor - (u8 *)db->primMem.start), db->primMem.capacityBytes);
	NativeMemTrack_Observe((gGT->swapchainIndex == 0) ? NATIVE_MEMTRACK_OT_MEM_0 : NATIVE_MEMTRACK_OT_MEM_1,
	                       (u32)((u8 *)db->otMem.cursor - (u8 *)db->otMem.start), db->otMem.capacityBytes);

	NativeMemTrack_FoldTallies(s_slots.frame, s_slots.peaks, NATIVE_MEMTRACK_SLOT_COUNT);
	NativeMemTrack_FoldTallies(s_rejects.frame, s_rejects.peaks, NATIVE_MEMTRACK_LEAF_REJECT_COUNT);

	NativeMemTrack_ReportIfStepped();
}

void NativeMemTrack_ResetPeaks(void)
{
	memset(s_peaks, 0, sizeof(s_peaks));
	memset(s_capacities, 0, sizeof(s_capacities));
	memset(&s_slots, 0, sizeof(s_slots));
	memset(&s_rejects, 0, sizeof(s_rejects));
	s_reportedTotal = 0;
	s_reportedOnce = 0;
}

#endif