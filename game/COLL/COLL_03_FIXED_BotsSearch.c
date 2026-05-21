#include <common.h>

void COLL_FIXED_BotsSearch(s16 *posCurr, s16 *posPrev, struct ScratchpadStruct *sps)
{
	char i;
	s16 radius = sps->Input1.hitRadius;
	int sqrRadius = radius * radius;
	s16 deltaCurr;
	s16 deltaPrev;

	sps->Input1.hitRadiusSquared = sqrRadius;
	sps->Union.QuadBlockColl.hitRadiusSquared = sqrRadius;
	sps->Union.QuadBlockColl.hitRadius = radius;

	for (i = 0; i < 3; i++)
	{
		sps->Input1.pos[i] = posCurr[i];
		sps->Union.QuadBlockColl.hitPos[i] = posCurr[i];
		sps->Union.QuadBlockColl.pos[i] = posPrev[i];

		deltaCurr = posCurr[i] - radius;
		deltaPrev = posPrev[i] - radius;
		sps->bbox.min[i] = (deltaCurr < deltaPrev) ? deltaCurr : deltaPrev;

		deltaCurr = posCurr[i] + radius;
		deltaPrev = posPrev[i] + radius;
		sps->bbox.max[i] = (deltaCurr > deltaPrev) ? deltaCurr : deltaPrev;
	}

	sps->unk3C = 0;
	sps->boolDidTouchHitbox = 0;

	sps->countByOne_ForWhatReason = 0x1000;
	sps->numInstHitboxesHit = 0;
	*(u32 *)&sps->dataOutput[0] = 0;

	DECOMP_COLL_SearchBSP_CallbackPARAM(sps->ptr_mesh_info->bspRoot, &sps->bbox, COLL_FIXED_BSPLEAF_TestInstance, sps);
}
