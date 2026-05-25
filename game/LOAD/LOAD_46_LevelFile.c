#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80034874-0x800348e8.
void LOAD_LevelFile(int levelID)
{
	struct GameTracker *gGT = sdata->gGT;

	// why here?
	sdata->modelMaskHints3D = 0;

	gGT->hudFlags &= 0xfe;

	gGT->prevLEV = gGT->levelID;
	gGT->levelID = levelID;

	// disable all rendering except checkeredFlag
	gGT->renderFlags &= 0x1000;

	if (RaceFlag_IsFullyOffScreen() == 1)
	{
		RaceFlag_BeginTransition(1);
	}

	// start loading
	sdata->Loading.stage = 0;
}
