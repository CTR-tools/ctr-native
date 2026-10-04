#include <common.h>

// NOTE(aalhendi): Both dispatch windows contain eight label words. Keep the
// copy shared, with no runtime call in the retail register entry.
static inline void RenderBucket_CopyDispatchWindow(const u32 *sourceBegin, u32 *destinationBegin)
{
	register s32 remaining CTR_PSX_REGISTER("$3");
	register const u32 *source CTR_PSX_REGISTER("$8");
	register u32 *destination CTR_PSX_REGISTER("$9");

	remaining = 7;
	CTR_PSX_KEEP_VALUE(remaining);
	source = sourceBegin;
	CTR_PSX_KEEP_VALUE(source);
	destination = destinationBegin;
	CTR_PSX_KEEP_VALUE(destination);

	goto copyBody;
copyAgain:
	remaining--;
copyBody:
{
	register u32 value CTR_PSX_REGISTER("$10") = *source++;
	CTR_PSX_KEEP_VALUE(value);
	*destination++ = value;
}
	// NOTE(aalhendi): Leave the branch slot for the backedge countdown rather
	// than the completed destination advance. Native observes ordinary C.
	CTR_PSX_OBSERVE_VALUE(destination);
	if (remaining > 0)
		goto copyAgain;
}

#ifndef CTR_NATIVE
extern const u32 RenderBucket_SetupDispatchWindow_Import[8];
extern const u32 RenderBucket_PrimitiveDispatchWindow_Import[8];

CTR_PSX_MATCH_SECTION(".RenderBucket_CopyDispatchTables")
void RenderBucket_CopyDispatchTables(void)
{
	// NOTE(aalhendi): Queue callers provide AT=scratch and keep a0/a1/a2 live.
	// Capture that real input; only the four documented working lanes change.
	register u8 *scratchBase CTR_PSX_REGISTER("$1");

	CTR_PSX_CAPTURE_REGISTER(scratchBase, CTR_SCRATCHPAD_PTR(u8, 0));
	RenderBucket_CopyDispatchWindow(RenderBucket_SetupDispatchWindow_Import, (u32 *)(scratchBase + 0x94));
	RenderBucket_CopyDispatchWindow(RenderBucket_PrimitiveDispatchWindow_Import, (u32 *)(scratchBase + 0xc4));
}
#endif
