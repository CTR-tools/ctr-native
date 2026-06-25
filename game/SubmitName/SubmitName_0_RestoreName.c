#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004aa08-0x8004aa60
void SubmitName_RestoreName(s16 param_1)
{
	struct GameTracker *gGT = sdata->gGT;

	// Time Trial or Adventure
	sdata->data10_bbb[0xd] = param_1;

	// copy the last string you typed the last time you were in
	// the OSK menu, back into the menu, avoid typing a second time
	memmove(gGT->currNameEntered, gGT->prevNameEntered, 0x11);

	// "A" or "SAVE"
	s16 cursor = 0;
	if (gGT->currNameEntered[0] != 0)
	{
		cursor = 1001;
	}

	gGT->typeCursorPosition = cursor;
}
