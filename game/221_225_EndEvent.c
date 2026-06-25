#include <common.h>

void OVR_Region1(void)
{
	struct GameTracker *gGT = sdata->gGT;

	if (gGT == 0)
	{
		return;
	}

	// NOTE(aalhendi): OVR_Region1 is a native dispatcher, not a named retail function body.
	//  On PS1, overlays 221-225 reuse the same entry address; native builds keep all five compiled and select one with the loader index.
	switch (gGT->overlayIndex_EndOfRace)
	{
	case 0:
		CC_EndEvent_DrawMenu();
		return;
	case 1:
		AA_EndEvent_DrawMenu();
		return;
	case 2:
		RR_EndEvent_DrawMenu();
		return;
	case 3:
		TT_EndEvent_DrawMenu();
		return;
	case 4:
		VB_EndEvent_DrawMenu();
		return;
	default:
		return;
	}
}
