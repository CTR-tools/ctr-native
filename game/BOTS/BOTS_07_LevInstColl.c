#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800135d8-0x8001372c.
void BOTS_LevInstColl(struct Thread *param_1)
{
	s16 currPos[6];
	s16 prevPos[3];
	struct Driver *d = (struct Driver *)param_1->object;
	struct ScratchpadStruct *sps = (struct ScratchpadStruct *)0x1f800108;

	// scratchpad stuff
	sps->ptr_mesh_info = sdata->gGT->level1->ptr_mesh_info;
	sps->Union.QuadBlockColl.searchFlags = 1;
	sps->Input1.modelID = DYNAMIC_ROBOT_CAR;
	sps->Union.QuadBlockColl.qbFlagsWanted = 0;
	sps->Union.QuadBlockColl.qbFlagsIgnored = 0;
	sps->Input1.hitRadius = 0x19;

	// grab driver stuff
	currPos[0] = (s16)(d->posCurr.x >> 8);
	currPos[1] = ((s16)(d->posCurr.y >> 8)) + 0x19;
	currPos[2] = (s16)(d->posCurr.z >> 8);
	prevPos[0] = (s16)(d->posPrev.x >> 8);
	prevPos[1] = ((s16)(d->posPrev.y >> 8)) + 0x19;
	prevPos[2] = (s16)(d->posPrev.z >> 8);

	COLL_FIXED_BotsSearch(currPos, prevPos, (s16 *)sps);

	if (sps->boolDidTouchHitbox)
	{
		sps->Union.QuadBlockColl.searchFlags &= 0xfff7;

		if ((sps->bspHitbox->flag & 0x80) != 0)
		{
			struct InstDef *instDef = sps->bspHitbox->data.hitbox.instDef;
			struct Instance *inst = instDef->ptrInstance;
			if (inst != NULL)
			{
				struct MetaDataMODEL *mdm = DECOMP_COLL_LevModelMeta(instDef->modelID);
				if (mdm != NULL)
				{
					if (mdm->LInC != NULL)
					{
						mdm->LInC(inst, param_1, sps);
					}
				}
			}
		}
	}
}
