#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006925c-0x80069284.
void VehTalkMask_End()
{
	CDSYS_XAPauseRequest();

	sdata->boolIsMaskThreadAlive = 0;
	sdata->talkMask_boolDead = 1;
}
