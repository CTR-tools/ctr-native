#include <common.h>
#include <ctr_scratchpad.h>

struct MetaDataMODEL *COLL_LevModelMeta(u32 id)
{
	// NOTE(aalhendi): Invalid unsigned IDs, including -1, select the fallback row.
	if (id >= NUM_MDM)
	{
		return &data.MetaDataModels[0];
	}

	return &data.MetaDataModels[id];
}


#include "COLL/COLL_CallbackQuadBlock.c"


#include "COLL/COLL_BspSearch.c"


#include "COLL/COLL_FixedTriangleShared.h"


#include "COLL/COLL_Barycentrics.c"


#include "COLL/COLL_InstancePoint.c"


#include "COLL/COLL_TestPoint.c"


#include "COLL/COLL_GetNormVec.c"


#include "COLL/COLL_QuadBlockNormals.c"
#include "COLL/COLL_LoadScratchpadVerts.c"


#include "COLL/COLL_TestTriangles.c"


#include "COLL/COLL_BspLeaf.c"


#include "COLL/COLL_FixedPlayer.c"


internal void CollMoved_GteRTV0(void)
{
	doCOP2(0x0486012);
}

internal void CollMoved_GteGPF12(void)
{
	doCOP2(0x0198003d);
}

internal void CollMoved_SelectProjection(CollNormalAxis normalAxis, struct BspSearchResult *candidate, struct BspSearchVertex *v1, struct BspSearchVertex **v2,
                                         struct BspSearchVertex **v3, struct CollTriangleProjection *projection)
{
	if (normalAxis == COLL_NORMAL_AXIS_Y)
	{
		s32 origin = v1->pos.z;
		projection->firstA = (*v2)->pos.z - origin;
		projection->firstB = (*v3)->pos.z - origin;
		projection->firstHit = candidate->pushOut.z - origin;

		if (Coll_MipsAbsS32(projection->firstA) < Coll_MipsAbsS32(projection->firstB))
		{
			s32 tmpValue = projection->firstA;
			projection->firstA = projection->firstB;
			projection->firstB = tmpValue;
			struct BspSearchVertex *tmp = *v2;
			*v2 = *v3;
			*v3 = tmp;
		}

		origin = v1->pos.x;
		projection->secondA = (*v2)->pos.x - origin;
		projection->secondB = (*v3)->pos.x - origin;
		projection->secondHit = candidate->pushOut.x - origin;
	}
	else if (normalAxis == COLL_NORMAL_AXIS_Z)
	{
		s32 origin = v1->pos.x;
		projection->firstA = (*v2)->pos.x - origin;
		projection->firstB = (*v3)->pos.x - origin;
		projection->firstHit = candidate->pushOut.x - origin;

		if (Coll_MipsAbsS32(projection->firstA) < Coll_MipsAbsS32(projection->firstB))
		{
			s32 tmpValue = projection->firstA;
			projection->firstA = projection->firstB;
			projection->firstB = tmpValue;
			struct BspSearchVertex *tmp = *v2;
			*v2 = *v3;
			*v3 = tmp;
		}

		origin = v1->pos.y;
		projection->secondA = (*v2)->pos.y - origin;
		projection->secondB = (*v3)->pos.y - origin;
		projection->secondHit = candidate->pushOut.y - origin;
	}
	else
	{
		s32 origin = v1->pos.y;
		projection->firstA = (*v2)->pos.y - origin;
		projection->firstB = (*v3)->pos.y - origin;
		projection->firstHit = candidate->pushOut.y - origin;

		if (Coll_MipsAbsS32(projection->firstA) < Coll_MipsAbsS32(projection->firstB))
		{
			s32 tmpValue = projection->firstA;
			projection->firstA = projection->firstB;
			projection->firstB = tmpValue;
			struct BspSearchVertex *tmp = *v2;
			*v2 = *v3;
			*v3 = tmp;
		}

		origin = v1->pos.z;
		projection->secondA = (*v2)->pos.z - origin;
		projection->secondB = (*v3)->pos.z - origin;
		projection->secondHit = candidate->pushOut.z - origin;
	}
}

#include "COLL/COLL_MovedReorderNormals.c"

#include "COLL/COLL_MovedTrianglePoint.c"

#include "COLL/COLL_MovedQuadBlock.c"


#include "COLL/COLL_MovedBspLeaf.c"


#include "COLL/COLL_FindScrub.c"


#include "COLL/COLL_MovedPlayerSearch.c"


#include "COLL/COLL_MovedScrubImpact.c"
