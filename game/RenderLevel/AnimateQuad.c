#include <ctr_types.h>

#ifndef CTR_NATIVE
// NOTE(aalhendi): Actual entry/loop lanes; the provider changes T5 and preserves
// the other five. These declarations must precede common.h's inline functions.
register u32 quadTimer CTR_PSX_REGISTER("$4");
register u32 quadRemaining CTR_PSX_REGISTER("$5");
register u32 quadVertexAddress CTR_PSX_REGISTER("$6");
register const u32 *quadVisList CTR_PSX_REGISTER("$7");
register u32 quadVisBits CTR_PSX_REGISTER("$13");
register u32 quadBitsLeft CTR_PSX_REGISTER("$14");
#endif

#include <common.h>
#include <ctr_gte_transfer.h>
#include "Animate_ScratchFrame.h"

#define ANIMATE_QUAD_OWNER CTR_PSX_UNSAFE_MANUAL_AT44_RETURN(AnimateQuad_Return)

void ANIMATE_QUAD_OWNER AnimateQuad(s32 timer, s32 numSCVert, struct SCVert *scVert, s32 *visSCVertList)
{
#ifdef CTR_NATIVE
	u32 quadRemaining = (u32)numSCVert;
	struct SCVert *quadVertex = scVert;
	const u32 *quadVisList = (const u32 *)visSCVertList;
	u32 quadVisBits;
	u32 quadBitsLeft;
#else
	ANIMATE_SCRATCH_DECLARE();
#endif
	ANIMATE_SCRATCH_SAVE();
	CTR_GteWriteControl(0, 21);
	CTR_GteWriteControl(0, 22);
	CTR_GteWriteControl(0, 23);
#ifndef CTR_NATIVE
	// NOTE(aalhendi): Prebias only the integer lane, never a native array pointer.
	quadVertexAddress -= sizeof(struct SCVert);
#endif
	quadVisBits = *quadVisList++;
	quadBitsLeft = 32;
	goto test_remaining;
decrement_then_call:
	quadBitsLeft--;
vertex_call:
#ifdef CTR_NATIVE
	AnimateQuadVertex(timer, quadVertex, &quadVisBits);
	quadVertex++;
#else
	quadVertexAddress += sizeof(struct SCVert);
	AnimateQuadVertex();
#endif
test_remaining:
	if (quadRemaining == 0)
		goto restore_frame;
	quadRemaining--;
	if (quadBitsLeft != 0)
		goto decrement_then_call;
	quadVisBits = *quadVisList++;
	quadBitsLeft = 31;
	goto vertex_call;
restore_frame:
	quadRemaining--;
	ANIMATE_SCRATCH_RESTORE();
}

#undef ANIMATE_QUAD_OWNER
#undef ANIMATE_SCRATCH_DECLARE
#undef ANIMATE_SCRATCH_SAVE
#undef ANIMATE_SCRATCH_RESTORE
