#include <common.h>

// NOTE(aalhendi): Copy packed XY and Z/flags without losing flag bits. Native
// LE helpers also support the vertex type's two-byte alignment.
static inline struct LevVertex *CollFixed_CopyScratchpadVertex(struct ScratchpadStruct *context, u32 recordIndex, struct LevVertex *source)
{
	register struct LevVertex *vertex CTR_PSX_REGISTER("$3") = source;
	register u32 xy CTR_PSX_REGISTER("$11");
	register u32 zFlags CTR_PSX_REGISTER("$13");

	xy = CTR_ReadU32AlignedLE(&vertex->pos.x);
	zFlags = CTR_ReadU32AlignedLE(&vertex->pos.z);
	CTR_WriteU32AlignedLE(&context->bspSearchVert[recordIndex].pos.x, xy);
	CTR_WriteU32AlignedLE(&context->bspSearchVert[recordIndex].pos.z, zFlags);
	return vertex;
}

void COLL_FIXED_QUADBLK_LoadScratchpadVerts(struct ScratchpadStruct *sps)
{
	// NOTE(aalhendi): Keep a0/t8/t9 live; copying uses v1/t0/t1/t3/t5.
	register struct ScratchpadStruct *context CTR_PSX_REGISTER("$4") = sps;
	register struct LevVertex *vertexArray CTR_PSX_REGISTER("$24");
	register struct QuadBlock *quad CTR_PSX_REGISTER("$25");
	register u32 indicesFirst CTR_PSX_REGISTER("$8");
	register u32 indicesSecond CTR_PSX_REGISTER("$9");

	// NOTE(aalhendi): Capture actual wrapper inputs T8/T9. Native uses the
	// previously primed context globals, not a fresh read of the mesh or quad.
	CTR_PSX_CAPTURE_REGISTER(vertexArray, sCollFixedLoadScratchpadVertsVertexArray);
	CTR_PSX_CAPTURE_REGISTER(quad, sCollFixedLoadScratchpadVertsQuad);
	indicesFirst = CTR_ReadU32AlignedLE(&quad->index[0]);
	indicesSecond = CTR_ReadU32AlignedLE(&quad->index[2]);
	context->bspSearchVert[0].pLevelVertex = CollFixed_CopyScratchpadVertex(context, 0, &vertexArray[indicesFirst & 0xffffu]);
	context->bspSearchVert[1].pLevelVertex = CollFixed_CopyScratchpadVertex(context, 1, &vertexArray[indicesFirst >> 16]);
	CTR_WriteU32AlignedLE(&context->quadSecondTriIndexA, indicesSecond);
	// NOTE(aalhendi): Keep later index loads staged between record copies.
	indicesFirst = CTR_ReadU32AlignedLE(&quad->index[4]);
	CTR_PSX_KEEP_VALUE(indicesFirst);
	context->bspSearchVert[2].pLevelVertex = CollFixed_CopyScratchpadVertex(context, 2, &vertexArray[indicesSecond & 0xffffu]);
	context->bspSearchVert[3].pLevelVertex = CollFixed_CopyScratchpadVertex(context, 3, &vertexArray[indicesSecond >> 16]);
	indicesSecond = CTR_ReadU32AlignedLE(&quad->index[6]);
	CTR_PSX_KEEP_VALUE(indicesSecond);
	context->bspSearchVert[4].pLevelVertex = CollFixed_CopyScratchpadVertex(context, 4, &vertexArray[indicesFirst & 0xffffu]);
	context->bspSearchVert[5].pLevelVertex = CollFixed_CopyScratchpadVertex(context, 5, &vertexArray[indicesFirst >> 16]);
	indicesFirst = (u16)quad->index[8];
	CTR_PSX_KEEP_VALUE(indicesFirst);
	context->bspSearchVert[6].pLevelVertex = CollFixed_CopyScratchpadVertex(context, 6, &vertexArray[indicesSecond & 0xffffu]);
	context->bspSearchVert[7].pLevelVertex = CollFixed_CopyScratchpadVertex(context, 7, &vertexArray[indicesSecond >> 16]);
	// NOTE(aalhendi): Leave the final pointer store available for the return delay slot.
	context->bspSearchVert[8].pLevelVertex = CollFixed_CopyScratchpadVertex(context, 8, &vertexArray[indicesFirst]);
}
