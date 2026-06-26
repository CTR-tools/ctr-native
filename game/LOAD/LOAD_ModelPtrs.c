#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031b50-0x80031bdc.
void LOAD_GlobalModelPtrs_MPK()
{
	struct GameTracker *gGT = sdata->gGT;

	for (int i = 0; i < 3; i++)
	{
		struct Model *m = data.driverModelExtras[i].model;

		if (m == NULL)
		{
			continue;
		}

		if (m->id == -1)
		{
			continue;
		}

		gGT->modelPtr[m->id] = m;
	}

	if (sdata->PLYROBJECTLIST != 0)
	{
		LibraryOfModels_Store(gGT, -1, (struct Model **)sdata->PLYROBJECTLIST);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031bdc-0x80031c1c.
void LOAD_HubSwapPtrs(struct GameTracker *gGT)
{
	struct Level *oldLev1;
	struct VisMem *oldVisMem1;
	struct VisMem *oldVisMem2;

	// if no secondary lev exists, quit
	if (gGT->level2 == 0)
	{
		return;
	}

	oldLev1 = gGT->level1;
	oldVisMem1 = gGT->visMem1;
	oldVisMem2 = gGT->visMem2;

	gGT->level1 = gGT->level2;
	gGT->boolHubSwapped = 1;

	gGT->level2 = oldLev1;
	gGT->visMem1 = oldVisMem2;
	gGT->visMem2 = oldVisMem1;
}
