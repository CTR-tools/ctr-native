#ifndef NATIVE_DRAW_DISTANCE_H
#define NATIVE_DRAW_DISTANCE_H

#include <stdbool.h>

// The increase_draw_distance option (g_config.increaseDrawDistance, declared in
// include/platform/native_config.h) lets the renderer submit geometry up to
// NATIVE_DRAW_DISTANCE_SCALE times farther than retail, so distant geometry is
// still drawn and a model keeps its high-poly LOD header out to the distance it
// was authored for.
//
// Each helper is pure: it takes retail's own value plus the flag, reads no
// global, and returns the value the caller should use. That is what lets them be
// tested off-line — see tests/test_draw_distance.c.

#define NATIVE_DRAW_DISTANCE_SCALE 4

// Retail's far clip: how far the frustum corner direction is extrapolated to
// land on the plane. A length in world units, not a ratio, so it is the one
// number here that starts from a constant instead of a caller-supplied value.
#define NATIVE_DRAW_DISTANCE_RETAIL_FAR_CLIP 0x100

static inline int NativeDrawDistance_FarClip(bool extended)
{
	return NATIVE_DRAW_DISTANCE_RETAIL_FAR_CLIP * (extended ? NATIVE_DRAW_DISTANCE_SCALE : 1);
}

// GTE H/2, the depth past which a projected bounds test would overflow the
// divide. Dividing rather than shifting keeps the expression valid for any
// factor; the argument is a non-negative register read, so both agree.
static inline int NativeDrawDistance_GeomScreenHalf(int geomScreenHalf, bool extended)
{
	return extended ? geomScreenHalf / NATIVE_DRAW_DISTANCE_SCALE : geomScreenHalf;
}

// A model's projected depth, used to pick its LOD header. Left unscaled the walk
// exhausts every header on a distant instance and drops it entirely, which is
// why this one is a divide: it delays that exhaustion rather than moving the
// cutoff.
static inline int NativeDrawDistance_ProjectedLodDistance(int projectedDistance, bool extended)
{
	return extended ? projectedDistance / NATIVE_DRAW_DISTANCE_SCALE : projectedDistance;
}

// The projected depth past which a BSP leaf draws through the full-dynamic
// emitter instead of the subdivided grids.
static inline int NativeDrawDistance_BspLodThreshold(int retailThreshold, bool extended)
{
	return extended ? retailThreshold * NATIVE_DRAW_DISTANCE_SCALE : retailThreshold;
}

#endif