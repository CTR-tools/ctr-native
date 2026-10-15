#ifndef NATIVE_MEMTRACK_H
#define NATIVE_MEMTRACK_H

#include <macros.h>

// Diagnostics for the level renderer's per-frame work. Nothing here changes
// behaviour: it records what a frame did and how much of each buffer it needed,
// so hand-sized constants and the renderer's own routing decisions can be set
// from a measurement instead of a guess.
//
// The peaks do not depend on how large the buffers currently are, so a run at
// any allocation reports the requirement for the configuration it ran.

struct GameTracker;

enum NativeMemTrackBuffer
{
	NATIVE_MEMTRACK_PRIM_MEM_0,
	NATIVE_MEMTRACK_PRIM_MEM_1,
	NATIVE_MEMTRACK_OT_MEM_0,
	NATIVE_MEMTRACK_OT_MEM_1,
	NATIVE_MEMTRACK_RENDERED_QUADBLOCKS,
	NATIVE_MEMTRACK_FRAME_VERTICES,
	NATIVE_MEMTRACK_DRAW_SPLITS,
	NATIVE_MEMTRACK_BUFFER_COUNT,
};

// A leaf the BSP walk discarded before it could be slotted: outside the push
// buffer's bounding box, rejected by a frustum plane, absent from the visibility
// list, or dropped because the traversal stack ran out. These are the culls that
// decide how much of the level a frame sees at all.
//
// The leaves that did get slotted are not named here: they are counted by
// enum RenderListsSlot1P2P, the game's own slot, which the renderer already has
// in hand when it makes the choice.
enum NativeMemTrackLeafReject
{
	NATIVE_MEMTRACK_LEAF_REJECTED_BOX,
	NATIVE_MEMTRACK_LEAF_REJECTED_FRUSTUM,
	NATIVE_MEMTRACK_LEAF_REJECTED_VISIBILITY,
	NATIVE_MEMTRACK_LEAF_REJECTED_STACK,
	NATIVE_MEMTRACK_LEAF_REJECT_COUNT,
};

#if defined(CTR_DIAGNOSTICS)

// Records the high-water mark of a fixed-size buffer.
void NativeMemTrack_Observe(enum NativeMemTrackBuffer buffer, u32 used, u32 capacity);
// Records how much of the level's rendered-quadblock list a bucket filled. The
// caller passes only the cursor: where the list starts and how big it is belong
// to the renderer, so the tracker reads both back rather than being told.
void NativeMemTrack_ObserveRenderedQuadblocks(const void *cursor);
// Tallies a leaf into the current frame's count for one render list slot. The
// value is an enum RenderListsSlot1P2P member.
void NativeMemTrack_CountLeafSlot(int renderListSlot);
// Tallies a leaf the traversal discarded instead of slotting it.
void NativeMemTrack_CountLeafReject(enum NativeMemTrackLeafReject reject);
// Records both arenas of the DB the caller is about to reset and closes the
// frame. Call once per frame, at the arena cursor rewind: that is the last
// point at which the frame's high-water mark is known.
void NativeMemTrack_SampleArenas(struct GameTracker *gGT);
// Drops every recorded peak and tally, so a new level or configuration starts
// from zero.
void NativeMemTrack_ResetPeaks(void);

#else

static inline void NativeMemTrack_Observe(enum NativeMemTrackBuffer buffer, u32 used, u32 capacity)
{
	(void)buffer;
	(void)used;
	(void)capacity;
}

static inline void NativeMemTrack_ObserveRenderedQuadblocks(const void *cursor)
{
	(void)cursor;
}

static inline void NativeMemTrack_CountLeafSlot(int renderListSlot)
{
	(void)renderListSlot;
}

static inline void NativeMemTrack_CountLeafReject(enum NativeMemTrackLeafReject reject)
{
	(void)reject;
}

static inline void NativeMemTrack_SampleArenas(struct GameTracker *gGT)
{
	(void)gGT;
}

static inline void NativeMemTrack_ResetPeaks(void)
{
}

#endif

#endif