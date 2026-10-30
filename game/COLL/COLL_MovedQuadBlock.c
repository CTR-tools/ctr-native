#include <common.h>

// NOTE(aalhendi): The real providers return scratch/quad in A0/T9. One
// paired observation models that returned tuple; native keeps its C owners.
#ifdef CTR_NATIVE
#define COLL_MOVED_QUAD_CAPTURE_RETURNS() ((void)0)
#else
#define COLL_MOVED_QUAD_CAPTURE_RETURNS() __asm__ volatile("" : "=r"(context), "=r"(quadCursor))
#endif

// NOTE(aalhendi): Point's real closure kills S0-S6 and returns scratch/quad in
// A0/T9. Derive every vertex argument from the latest captured scratch owner.
#define COLL_MOVED_QUAD_CALL_POINT(first, second, third)                                                                                              \
	do                                                                                                                                                \
	{                                                                                                                                                 \
		COLL_MOVED_TRIANGL_TestPoint(context, &context->bspSearchVert[(first)], &context->bspSearchVert[(second)], &context->bspSearchVert[(third)]); \
		CTR_PSX_CLOBBER("$16");                                                                                                                       \
		CTR_PSX_CLOBBER("$17");                                                                                                                       \
		CTR_PSX_CLOBBER("$18");                                                                                                                       \
		CTR_PSX_CLOBBER("$19");                                                                                                                       \
		CTR_PSX_CLOBBER("$20");                                                                                                                       \
		CTR_PSX_CLOBBER("$21");                                                                                                                       \
		CTR_PSX_CLOBBER("$22");                                                                                                                       \
		COLL_MOVED_QUAD_CAPTURE_RETURNS();                                                                                                            \
	} while (0)

#define COLL_MOVED_QUAD_SET_TRIANGLE(id)                \
	do                                                  \
	{                                                   \
		triangleID = (id);                              \
		context->candidate.triangleID = (u8)triangleID; \
	} while (0)

void COLL_MOVED_QUADBLK_TestTriangles(struct QuadBlock *quad, struct ScratchpadStruct *sps)
{
	register struct QuadBlock *quadCursor CTR_PSX_REGISTER("$25") = quad;
	register struct ScratchpadStruct *context CTR_PSX_REGISTER("$4") = sps;
	// NOTE(aalhendi): These eight internal work roles use measured register colors.
	// Reused colors constrain allocation; only the A0/T9 owners above are ABI state.
	register u32 flags CTR_PSX_REGISTER("$3");
	register u32 testMask CTR_PSX_REGISTER("$8");
	register u32 ignored CTR_PSX_REGISTER("$9");
	register u32 packed CTR_PSX_REGISTER("$9");
	register s32 separation CTR_PSX_REGISTER("$10");
	register s32 bound CTR_PSX_REGISTER("$8");
	register u32 firstIndex CTR_PSX_REGISTER("$8");
	register u32 triangleID CTR_PSX_REGISTER("$3");

	context->candidate.ptrQuadblock = quadCursor;
	flags = (u32)(s32)(s16)quadCursor->quadFlags;
	testMask = context->Union.QuadBlockColl.quadFlagsWanted;
	ignored = context->Union.QuadBlockColl.quadFlagsIgnored;
	testMask &= flags;
	if (testMask == 0)
		return;
	ignored &= flags;
	if (ignored != 0)
		return;

	// NOTE(aalhendi): Prepare each next scratch bound after consuming the
	// previous one; every read belongs to the complete mapped scratch record.
	packed = CTR_ReadU32AlignedLE(&quadCursor->bbox.min.x);
	bound = context->bbox.max.y;
	separation = (s32)(s16)(packed >> 16);
	separation -= bound;
	bound = context->bbox.max.x;
	if (separation > 0)
		return;
	separation = (s32)(packed << 16);
	separation >>= 16;
	separation -= bound;
	if (separation > 0)
		return;
	packed = CTR_ReadU32AlignedLE(&quadCursor->bbox.min.z);
	bound = context->bbox.min.x;
	separation = (s32)(s16)(packed >> 16);
	separation = bound - separation;
	bound = context->bbox.max.z;
	if (separation > 0)
		return;
	separation = (s32)(packed << 16);
	separation >>= 16;
	separation -= bound;
	if (separation > 0)
		return;
	packed = CTR_ReadU32AlignedLE(&quadCursor->bbox.max.y);
	bound = context->bbox.min.z;
	separation = (s32)(s16)(packed >> 16);
	separation = bound - separation;
	bound = context->bbox.min.y;
	if (separation > 0)
		return;
	separation = (s32)(packed << 16);
	separation >>= 16;
	separation = bound - separation;
	flags = (u32)(s32)(s16)context->Union.QuadBlockColl.searchFlags;
	if (separation > 0)
		return;

	testMask = flags & COLL_SEARCH_HIGH_LOD;
	if (testMask != 0)
	{
		testMask = flags & COLL_SEARCH_REUSE_NORMALS;
		if (testMask == 0)
		{
			COLL_FIXED_QUADBLK_GetNormVecs_HiLOD(context, quadCursor);
			COLL_MOVED_QUAD_CAPTURE_RETURNS();
		}

		// NOTE(aalhendi): Observe the completed setup tuple at the common join;
		// keep ordinal setup after the optional normals call.
		CTR_PSX_OBSERVE_VALUE(context);

		COLL_MOVED_QUAD_SET_TRIANGLE(COLL_QUAD_TRIANGLE_HI_0);
		COLL_MOVED_QUAD_CALL_POINT(0, 4, 5);
		COLL_MOVED_QUAD_SET_TRIANGLE(COLL_QUAD_TRIANGLE_HI_1);
		COLL_MOVED_QUAD_CALL_POINT(4, 6, 5);
		COLL_MOVED_QUAD_SET_TRIANGLE(COLL_QUAD_TRIANGLE_HI_2);
		COLL_MOVED_QUAD_CALL_POINT(6, 4, 1);
		// NOTE(aalhendi): Retail advances the working ordinal without publishing
		// it; the fourth Point call still reads triangle ID4 from scratch.
		triangleID = COLL_QUAD_TRIANGLE_HI_3;
		CTR_PSX_OBSERVE_VALUE(triangleID);
		COLL_MOVED_QUAD_CALL_POINT(5, 6, 2);

		packed = CTR_ReadU32AlignedLE(&quadCursor->index[2]);
		COLL_MOVED_QUAD_SET_TRIANGLE(COLL_QUAD_TRIANGLE_HI_4);
		firstIndex = packed >> 16;
		packed &= 0xffffu;
		if (firstIndex != packed)
		{
			COLL_MOVED_QUAD_CALL_POINT(8, 6, 7);
			COLL_MOVED_QUAD_SET_TRIANGLE(COLL_QUAD_TRIANGLE_HI_5);
			COLL_MOVED_QUAD_CALL_POINT(7, 3, 8);
			COLL_MOVED_QUAD_SET_TRIANGLE(COLL_QUAD_TRIANGLE_HI_6);
			COLL_MOVED_QUAD_CALL_POINT(1, 7, 6);
			COLL_MOVED_QUAD_SET_TRIANGLE(COLL_QUAD_TRIANGLE_HI_7);
			COLL_MOVED_QUAD_CALL_POINT(2, 6, 8);
		}
		return;
	}

	COLL_FIXED_QUADBLK_GetNormVecs_LoLOD(context, quadCursor);
	COLL_MOVED_QUAD_CAPTURE_RETURNS();
	context->candidate.triangleID = COLL_QUAD_TRIANGLE_LO_0;
	COLL_MOVED_QUAD_CALL_POINT(0, 1, 2);
	packed = CTR_ReadU32AlignedLE(&quadCursor->index[2]);
	COLL_MOVED_QUAD_SET_TRIANGLE(COLL_QUAD_TRIANGLE_LO_1);
	firstIndex = packed >> 16;
	packed &= 0xffffu;
	if (firstIndex != packed)
		COLL_MOVED_QUAD_CALL_POINT(1, 3, 2);
}

#undef COLL_MOVED_QUAD_SET_TRIANGLE
#undef COLL_MOVED_QUAD_CALL_POINT
#undef COLL_MOVED_QUAD_CAPTURE_RETURNS
