#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800485a8-0x800485cc.
void SelectProfile_GetTrackID()
{
	data.menuGreenLoadSave.rowSelected = 1;
	sdata->advProgress.HubLevYouSavedOn = sdata->gGT->levelID;
}
