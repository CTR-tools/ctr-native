#include <common.h>

struct AHSignScratch
{
	SVec3Slot probeTop;
	SVec3Slot probeBottom;
	SVec3Slot normal;
	struct ScratchpadStruct sps;
};

CTR_STATIC_ASSERT_LAYOUT(offsetof(struct AHSignScratch, probeTop) == 0x00);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct AHSignScratch, probeBottom) == 0x08);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct AHSignScratch, normal) == 0x10);
CTR_STATIC_ASSERT_LAYOUT(offsetof(struct AHSignScratch, sps) == 0x18);

// NOTE(aalhendi): ASM-verified NTSC-U 926 overlay 232 0x800b4c80-0x800b4ddc.
void AH_Sign_LInB(struct Instance *inst)
{
	struct AHSignScratch *scratch = CTR_SCRATCHPAD_PTR(struct AHSignScratch, 0x108);
	SVec3Slot *probeTop = &scratch->probeTop;
	SVec3Slot *probeBottom = &scratch->probeBottom;
	SVec3Slot *normal = &scratch->normal;
	struct ScratchpadStruct *sps = &scratch->sps;

	normal->x = inst->matrix.m[0][2] >> 6;
	normal->y = inst->matrix.m[1][2] >> 6;
	normal->z = inst->matrix.m[2][2] >> 6;

	sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;
	sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND | QUADBLOCK_FLAG_COLLISION_SURFACE;
	sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
	sps->ptr_mesh_info = sdata->gGT->level1->ptr_mesh_info;

	probeTop->x = inst->matrix.t[0] + normal->x * 2;
	probeBottom->x = probeTop->x - normal->x * 4;

	probeTop->y = inst->matrix.t[1] + normal->y * 2;
	probeBottom->y = probeTop->y - normal->y * 4;

	probeTop->z = inst->matrix.t[2] + normal->z * 2;
	probeBottom->z = probeTop->z - normal->z * 4;

	COLL_SearchBSP_CallbackQUADBLK(&probeTop->vec, &probeBottom->vec, sps, 0);

	if (sps->boolDidTouchQuadblock != 0)
	{
		normal->x = -normal->x;
		normal->y = -normal->y;
		normal->z = -normal->z;
	}

	inst->bitCompressed_NormalVector_AndDriverIndex = ((u16)normal->x & 0xff) | (((u16)normal->y & 0xff) << 8) | (((u16)normal->z & 0xff) << 0x10);
}
