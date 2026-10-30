// NOTE(aalhendi): FIXED and MOVED instantiate one traversal with their Quad callee.
// Each owner defines and undefines COLL_BSPLEAF_TEST_QUAD around this body.
	s32 numQuads;
	register u32 flags CTR_PSX_REGISTER("$3");
	struct ScratchpadStruct *scratch;
	register struct ScratchpadStruct *argument CTR_PSX_REGISTER("$5") = sps;
	register struct QuadBlock *ptrQuad CTR_PSX_REGISTER("$25");

	// NOTE(aalhendi): V1/A1 shape the measured work/copy schedule; the T9
	// cursor is the provider-preserved continuation value.
	flags = CTR_ReadU32LE(node);
	CTR_PSX_COPY_VALUE(scratch, argument);
	flags &= BSP_LEAF_FLAG_WATER;
#ifdef CTR_NATIVE
	if (flags != 0)
	{
		flags = scratch->collision.stepFlags;
		flags |= COLL_STEP_FLAG_WATER_BSP;
		scratch->collision.stepFlags = flags;
	}
#else
	// NOTE(aalhendi): The slot reads mapped RAM on both paths and consumes
	// flags as dead scratch. The join must start with an independent word
	// (the quad-count load); this is not a reusable returning primitive.
	__asm__ volatile(".set\tnoreorder\n\t"
	                 "beq %0,$0,.Lleaf_water_done_%=\n\t"
	                 "lw %0,%1\n\t"
	                 "nop\n\t"
	                 "ori %0,%0,%2\n\t"
	                 "sw %0,%1\n\t"
	                 ".Lleaf_water_done_%=:\n\t.set\treorder"
	                 : "+&r"(flags), "+m"(scratch->collision.stepFlags)
	                 : "K"(COLL_STEP_FLAG_WATER_BSP));
#endif

	numQuads = node->data.leaf.numQuads;
	ptrQuad = node->data.leaf.ptrQuadBlockArray;
	do
	{
		argument = scratch;
		COLL_BSPLEAF_TEST_QUAD(ptrQuad, argument);
		numQuads--;
		ptrQuad++;
	} while (numQuads > 0);

	flags = (u32)CTR_ReadS16LE(&scratch->Union.QuadBlockColl.searchFlags);
	if ((flags & COLL_SEARCH_TEST_INSTANCES) != 0)
	{
		argument = scratch;
		COLL_FIXED_BSPLEAF_TestInstance(node, argument);
	}
