#ifndef CTR_NATIVE
// NOTE(aalhendi): The loader/normal provider retain A0 and this T9 quad;
// T8 supplies the loader's vertex array.
register struct QuadBlock *sCollFixedLoadScratchpadVertsQuad __asm__("$25");
#endif

#include <common.h>

#ifdef CTR_NATIVE

global_variable struct LevVertex *sCollFixedLoadScratchpadVertsVertexArray;
global_variable struct QuadBlock *sCollFixedLoadScratchpadVertsQuad;

internal void COLL_FIXED_QUADBLK_SetLoadScratchpadVertsContext(struct ScratchpadStruct *sps, struct QuadBlock *quad)
{
	sCollFixedLoadScratchpadVertsVertexArray = sps->ptr_mesh_info->ptrVertexArray;
	sCollFixedLoadScratchpadVertsQuad = quad;
}

#endif

void COLL_FIXED_QUADBLK_GetNormVecs_LoLOD(struct ScratchpadStruct *sps, struct QuadBlock *quad)
{
	register struct ScratchpadStruct *context CTR_PSX_REGISTER("$4") = sps;
#ifndef CTR_NATIVE
	register struct mesh_info *mesh CTR_PSX_REGISTER("$24");
	register struct LevVertex *vertexArray CTR_PSX_REGISTER("$24");
#endif
	// NOTE(aalhendi): These local work registers shape code, not the calling ABI.
	register s32 normalShift CTR_PSX_REGISTER("$8");
	register s32 normalDividend CTR_PSX_REGISTER("$3");
	register u32 indexA CTR_PSX_REGISTER("$9");
	register u32 indexB CTR_PSX_REGISTER("$10");

#ifdef CTR_NATIVE
	COLL_FIXED_QUADBLK_SetLoadScratchpadVertsContext(context, quad);
#else
	mesh = context->ptr_mesh_info;
	vertexArray = mesh->ptrVertexArray;
	CTR_PSX_OBSERVE_VALUE(vertexArray);
	sCollFixedLoadScratchpadVertsQuad = quad;
#endif
	COLL_FIXED_QUADBLK_LoadScratchpadVerts(context);

	// NOTE(aalhendi): LOD setup starts after the loader; signed wide observations
	// preserve the real LB/LH reads before their SB/SH truncation.
	normalDividend = 2;
	normalShift = (s32)(s8)sCollFixedLoadScratchpadVertsQuad->triNormalVecBitShift;
	CTR_PSX_OBSERVE_VALUE(normalShift);
	context->collision.triNormalLodShift = (u8)normalDividend;
	context->collision.triNormalVecBitShift = (u8)normalShift;

	indexA = context->quadSecondTriIndexA;
	indexB = context->quadSecondTriIndexB;
	if (indexA != indexB)
	{
		normalDividend = (s32)sCollFixedLoadScratchpadVertsQuad->triNormalVecDividend[QUADBLOCK_TRI_NORMAL_DIVIDEND_LO_1];
		CTR_PSX_OBSERVE_VALUE(normalDividend);
		context->collision.triNormalVecDividend = (s16)normalDividend;
		COLL_FIXED_TRIANGL_GetNormVec(context, &context->bspSearchVert[1], &context->bspSearchVert[3], &context->bspSearchVert[2]); // 1, 3, 2
	}

	normalDividend = (s32)sCollFixedLoadScratchpadVertsQuad->triNormalVecDividend[QUADBLOCK_TRI_NORMAL_DIVIDEND_LO_0];
	CTR_PSX_OBSERVE_VALUE(normalDividend);
	context->collision.triNormalVecDividend = (s16)normalDividend;
	COLL_FIXED_TRIANGL_GetNormVec(context, &context->bspSearchVert[0], &context->bspSearchVert[1], &context->bspSearchVert[2]); // 0, 1, 2
}

void COLL_FIXED_QUADBLK_GetNormVecs_HiLOD(struct ScratchpadStruct *sps, struct QuadBlock *quad)
{
	register struct ScratchpadStruct *context CTR_PSX_REGISTER("$4") = sps;
#ifndef CTR_NATIVE
	register struct mesh_info *mesh CTR_PSX_REGISTER("$24");
	register struct LevVertex *vertexArray CTR_PSX_REGISTER("$24");
#endif
	register s32 normalShift CTR_PSX_REGISTER("$8");
	register s32 normalDividend CTR_PSX_REGISTER("$3");
	register u32 indexA CTR_PSX_REGISTER("$9");
	register u32 indexB CTR_PSX_REGISTER("$10");

#ifdef CTR_NATIVE
	COLL_FIXED_QUADBLK_SetLoadScratchpadVertsContext(context, quad);
#else
	mesh = context->ptr_mesh_info;
	vertexArray = mesh->ptrVertexArray;
	CTR_PSX_OBSERVE_VALUE(vertexArray);
	sCollFixedLoadScratchpadVertsQuad = quad;
#endif
	COLL_FIXED_QUADBLK_LoadScratchpadVerts(context);

	normalShift = (s32)(s8)sCollFixedLoadScratchpadVertsQuad->triNormalVecBitShift;
	context->collision.triNormalLodShift = 0;
	context->collision.triNormalVecBitShift = (u8)normalShift;

	// Calculate normals for eight triangles, without testing collision here.
	indexA = context->quadSecondTriIndexA;
	indexB = context->quadSecondTriIndexB;
	if (indexA != indexB)
	{
		normalDividend = (s32)sCollFixedLoadScratchpadVertsQuad->triNormalVecDividend[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_4];
		CTR_PSX_OBSERVE_VALUE(normalDividend);
		context->collision.triNormalVecDividend = (s16)normalDividend;
		COLL_FIXED_TRIANGL_GetNormVec(context, &context->bspSearchVert[8], &context->bspSearchVert[6], &context->bspSearchVert[7]); // 8, 6, 7

		normalDividend = (s32)sCollFixedLoadScratchpadVertsQuad->triNormalVecDividend[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_5];
		CTR_PSX_OBSERVE_VALUE(normalDividend);
		context->collision.triNormalVecDividend = (s16)normalDividend;
		COLL_FIXED_TRIANGL_GetNormVec(context, &context->bspSearchVert[7], &context->bspSearchVert[3], &context->bspSearchVert[8]); // 7, 3, 8

		normalDividend = (s32)sCollFixedLoadScratchpadVertsQuad->triNormalVecDividend[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_6];
		CTR_PSX_OBSERVE_VALUE(normalDividend);
		context->collision.triNormalVecDividend = (s16)normalDividend;
		COLL_FIXED_TRIANGL_GetNormVec(context, &context->bspSearchVert[1], &context->bspSearchVert[7], &context->bspSearchVert[6]); // 1, 7, 6

		normalDividend = (s32)sCollFixedLoadScratchpadVertsQuad->triNormalVecDividend[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_7];
		CTR_PSX_OBSERVE_VALUE(normalDividend);
		context->collision.triNormalVecDividend = (s16)normalDividend;
		COLL_FIXED_TRIANGL_GetNormVec(context, &context->bspSearchVert[2], &context->bspSearchVert[6], &context->bspSearchVert[8]); // 2, 6, 8
	}

	normalDividend = (s32)sCollFixedLoadScratchpadVertsQuad->triNormalVecDividend[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_0];
	CTR_PSX_OBSERVE_VALUE(normalDividend);
	context->collision.triNormalVecDividend = (s16)normalDividend;
	COLL_FIXED_TRIANGL_GetNormVec(context, &context->bspSearchVert[0], &context->bspSearchVert[4], &context->bspSearchVert[5]); // 0, 4, 5

	normalDividend = (s32)sCollFixedLoadScratchpadVertsQuad->triNormalVecDividend[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_1];
	CTR_PSX_OBSERVE_VALUE(normalDividend);
	context->collision.triNormalVecDividend = (s16)normalDividend;
	COLL_FIXED_TRIANGL_GetNormVec(context, &context->bspSearchVert[4], &context->bspSearchVert[6], &context->bspSearchVert[5]); // 4, 6, 5

	normalDividend = (s32)sCollFixedLoadScratchpadVertsQuad->triNormalVecDividend[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_2];
	CTR_PSX_OBSERVE_VALUE(normalDividend);
	context->collision.triNormalVecDividend = (s16)normalDividend;
	COLL_FIXED_TRIANGL_GetNormVec(context, &context->bspSearchVert[6], &context->bspSearchVert[4], &context->bspSearchVert[1]); // 6, 4, 1

	normalDividend = (s32)sCollFixedLoadScratchpadVertsQuad->triNormalVecDividend[QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_3];
	CTR_PSX_OBSERVE_VALUE(normalDividend);
	context->collision.triNormalVecDividend = (s16)normalDividend;
	COLL_FIXED_TRIANGL_GetNormVec(context, &context->bspSearchVert[5], &context->bspSearchVert[6], &context->bspSearchVert[2]); // 5, 6, 2
}
