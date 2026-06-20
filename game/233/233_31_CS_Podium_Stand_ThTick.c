#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b021c-0x800b0248
void CS_Podium_Stand_ThTick(struct Thread *t)
{
	if (D233.isCutsceneOver != 0)
		t->flags |= THREAD_FLAG_DEAD;
}
