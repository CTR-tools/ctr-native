#include <common.h>

// NOTE(aalhendi): ASM-verified against NTSC-U 926 overlay 231 0x800b4fe4-0x800b5090.

void RB_Default_LInB(struct Instance *inst)
{
	SVec3 *probeTop = CTR_SCRATCHPAD_PTR(SVec3, 0x108);
	SVec3 *probeBottom = CTR_SCRATCHPAD_PTR(SVec3, 0x110);
	struct ScratchpadStruct *sps = CTR_SCRATCHPAD_PTR(struct ScratchpadStruct, 0x118);

	// high-LOD coll (8 triangles)
	sps->Union.QuadBlockColl.searchFlags = COLL_SEARCH_HIGH_LOD;

	sps->Union.QuadBlockColl.quadFlagsWanted = QUADBLOCK_FLAG_GROUND | QUADBLOCK_FLAG_COLLISION_SURFACE;
	sps->Union.QuadBlockColl.quadFlagsIgnored = 0;
	sps->ptr_mesh_info = sdata->gGT->level1->ptr_mesh_info;

	// Make a hitbox
	probeTop->x = inst->matrix.t[0];
	probeBottom->x = inst->matrix.t[0];
	probeTop->z = inst->matrix.t[2];
	probeBottom->z = inst->matrix.t[2];
	probeTop->y = inst->matrix.t[1] - 0x180;
	probeBottom->y = inst->matrix.t[1] + 0x80;

	COLL_SearchBSP_CallbackQUADBLK(probeTop, probeBottom, sps, 0);

	RB_MakeInstanceReflective(sps, inst);
}
