#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800314c0-0x800314e0.
void LibraryOfModels_Clear(struct GameTracker *gGT)
{
	int i;
	for (i = 0; i < 0xe2; i++)
	{
		gGT->modelPtr[i] = 0;
	}
}
