#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800abab0-0x800abbb4.
void RB_MakeInstanceReflective(struct ScratchpadStruct *sps, struct Instance *inst)
{
	u16 quadFlags;
	struct GameTracker *gGT = sdata->gGT;

	if ((sps->boolDidTouchQuadblock == 0) || (sps->boolDidTouchHitbox != 0))
	{
		inst->compressedNormalAndDriverIndex = INST_CompressNormalVector(0, FP_ONE, 0);
	}
	else
	{
		inst->compressedNormalAndDriverIndex = INST_CompressNormalVector(sps->hit.plane.normal.x, sps->hit.plane.normal.y, sps->hit.plane.normal.z);

		if (1 < gGT->numPlyrCurrGame)
		{
			return;
		}

		quadFlags = sps->hit.ptrQuadblock->quadFlags;

		if ((quadFlags & QUADBLOCK_FLAG_COLLISION_SURFACE) == 0)
		{
			if ((quadFlags & QUADBLOCK_FLAG_REFLECT_SPLIT_LINE_1) != 0)
			{
				inst->flags |= REFLECTIVE;
				inst->vertSplit = gGT->level1->splitLines[1];
				return;
			}

			if ((quadFlags & QUADBLOCK_FLAG_REFLECT_SPLIT_LINE_0) != 0)
			{
				inst->flags |= REFLECTIVE;
				inst->vertSplit = gGT->level1->splitLines[0];
				return;
			}
		}
	}

	inst->flags &= ~REFLECTIVE;
}
