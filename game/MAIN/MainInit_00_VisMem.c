#include <common.h>

#ifdef CTR_NATIVE
static void MainInit_InitVisMemBspListNodes(struct VisMem *visMem, struct mesh_info *mesh)
{
	if (mesh == NULL || mesh->bspRoot == NULL)
		return;

	for (int playerIndex = 0; playerIndex < 4; playerIndex++)
	{
		struct VisMemBspListNode *bspList = visMem->bspList[playerIndex];

		if (bspList == NULL)
			continue;

		for (int bspIndex = 0; bspIndex < mesh->numBspNodes; bspIndex++)
		{
			// NOTE(aalhendi): Overlay 226 expects word two to retain the BSP pointer; RenderLists_Init* only rewrites word zero when linking visible leaves.
			bspList[bspIndex].next = NULL;
			bspList[bspIndex].bsp = &mesh->bspRoot[bspIndex];
		}
	}
}
#endif

void DECOMP_MainInit_VisMem(struct GameTracker *gGT)
{
	struct VisMem *visMem = gGT->level1->visMem;
	if (visMem == NULL)
		return;

	gGT->visMem1 = visMem;

#ifdef CTR_NATIVE
	MainInit_InitVisMemBspListNodes(visMem, gGT->level1->ptr_mesh_info);
#endif
}
