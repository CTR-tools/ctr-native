#include <common.h>

// NOTE(aalhendi): On MIPS this setup falls directly into PARAM with the original
// SP/RA. Link the real PARAM body immediately after this entry.
CTR_PSX_ADJACENT_TAIL("COLL_SearchBSP_CallbackPARAM")
void COLL_SearchBSP_CallbackQUADBLK(const SVec3 *top, const SVec3 *bottom, struct ScratchpadStruct *sps, s32 hitRadius)
{
	register struct ScratchpadStruct *context CTR_PSX_REGISTER("$6") = sps;
	register struct ScratchpadStruct *queryOut CTR_PSX_REGISTER("$7");
	register struct BoundingBox *boundsOut CTR_PSX_REGISTER("$5");
	// NOTE(aalhendi): Reuse retail's caller-saved work bank across disjoint values.
	// These local choices guide allocation, not additional argument ABI rules.
	register u32 xy CTR_PSX_REGISTER("$8");
	register s32 z CTR_PSX_REGISTER("$9");
	register s32 hi CTR_PSX_REGISTER("$3");
	register s32 lo CTR_PSX_REGISTER("$8");
	register s32 hiY CTR_PSX_REGISTER("$9");
	register s32 loY CTR_PSX_REGISTER("$10");
	register s32 swap CTR_PSX_REGISTER("$7");
	register s32 delta CTR_PSX_REGISTER("$7");
	register s32 squared CTR_PSX_REGISTER("lo");
	register s32 squaredWord CTR_PSX_REGISTER("$3");
	register struct mesh_info *mesh CTR_PSX_REGISTER("$3");
	struct BSP *root;

	squared = CTR_MipsMulLo(hitRadius, hitRadius);
	context->Input1.hitRadius = hitRadius;
	context->Union.QuadBlockColl.hitRadius = hitRadius;
	context->boolDidTouchHitbox = 0;
	context->numTrianglesTested = 0;
	context->boolDidTouchQuadblock = 0;
	context->collision.stepFlags = 0;
	mesh = context->ptr_mesh_info;
	context->numBspHitboxesHit = 0;

	xy = CTR_ReadU32AlignedLE(&top->x);
	z = top->z;
	root = mesh->bspRoot;
	CTR_WriteU32AlignedLE(&context->Input1.pos.x, xy);
	context->Input1.pos.z = z;
	CTR_WriteU32AlignedLE(&context->Union.QuadBlockColl.hitPos.x, xy);
	context->Union.QuadBlockColl.hitPos.z = z;
	xy = CTR_ReadU32AlignedLE(&bottom->x);
	z = bottom->z;
	CTR_WriteU32AlignedLE(&context->Union.QuadBlockColl.pos.x, xy);
	context->Union.QuadBlockColl.pos.z = z;

	// NOTE(aalhendi): Keep the stored LO result materialized before fraction writes.
	squaredWord = squared;
#ifndef CTR_NATIVE
	__asm__ volatile("" : : "r"(squaredWord));
#endif
	// NOTE(aalhendi): The endpoint-word carrier is dead and can hold this constant.
	xy = COLL_FRACTION_ONE;
	context->hitFraction = xy;
	context->Input1.hitRadiusSquared = squaredWord;
	context->Union.QuadBlockColl.hitRadiusSquared = squaredWord;

	hi = context->Input1.pos.x;
	lo = context->Union.QuadBlockColl.pos.x;
	hiY = context->Input1.pos.y;
	loY = context->Union.QuadBlockColl.pos.y;
	delta = hi - lo;
	if (delta < 0)
	{
		swap = hi;
		hi = lo;
		lo = swap;
	}
	context->bbox.max.x = hi;
	context->bbox.min.x = lo;
	hi = context->Input1.pos.z;
	lo = context->Union.QuadBlockColl.pos.z;
	delta = hiY - loY;
	if (delta < 0)
	{
		swap = hiY;
		hiY = loY;
		loY = swap;
	}
	context->bbox.max.y = hiY;
	context->bbox.min.y = loY;
	// NOTE(aalhendi): Preserve the Y/Z boundary for the real old-high swap slot.
#ifndef CTR_NATIVE
	__asm__ volatile("" : : "r"(hi), "r"(lo));
#endif
	delta = hi - lo;
	if (delta < 0)
	{
		swap = hi;
		hi = lo;
		lo = swap;
	}
	context->bbox.max.z = hi;
	context->bbox.min.z = lo;
	// NOTE(aalhendi): Finish bounds writes before staging the outgoing arguments.
#ifndef CTR_NATIVE
	__asm__ volatile("" : : "r"(context));
#endif

	boundsOut = &context->bbox;
	queryOut = context;
	COLL_SearchBSP_CallbackPARAM(root, boundsOut, COLL_FIXED_BSPLEAF_TestQuadblocks, queryOut);
}
