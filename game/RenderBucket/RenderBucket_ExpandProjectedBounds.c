#include <ctr_types.h>

#ifndef CTR_NATIVE
// NOTE(aalhendi): This standalone leaf receives and returns the bounds in
// t3/t4/t5/t6/t7/s0, consuming packed XY in t1 and preserving depth in t2.
// Declare the tuple before common.h so GCC treats mutable s0 as an output,
// not a callee-saved local. The context-based caller shares the C algorithm.
register u32 boundsSxy CTR_PSX_REGISTER("$9");
register s32 boundsDepth CTR_PSX_REGISTER("$10");
register s32 boundsMinX CTR_PSX_REGISTER("$11");
register s32 boundsMaxX CTR_PSX_REGISTER("$12");
register s32 boundsMinY CTR_PSX_REGISTER("$13");
register s32 boundsMaxY CTR_PSX_REGISTER("$14");
register s32 boundsMinZ CTR_PSX_REGISTER("$15");
register s32 boundsMaxZ CTR_PSX_REGISTER("$16");

#include <common.h>

#include "RenderBucket_ProjectedBounds.h"

void RenderBucket_ExpandProjectedBounds(void)
{
	RENDER_BUCKET_EXPAND_PROJECTED_BOUNDS(boundsSxy, boundsDepth, boundsMinX, boundsMaxX, boundsMinY, boundsMaxY, boundsMinZ, boundsMaxZ);
}
#endif
