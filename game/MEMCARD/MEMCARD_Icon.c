#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003d4e4-0x8003d540.
void MEMCARD_SetIcon(int icon)
{
	int *src;
	int *dst;

	if ((icon & 0xffff) == 0)
	{
		src = &data.memcardIcon_Ghost[0];
	}
	else
	{
		src = &data.memcardIcon_CrashHead[0];
	}

	dst = &data.memcardIcon_PsyqHand[0];

	for (int i = 0; i < 0x40; i += 4)
	{
		dst[i + 0] = src[i + 0];
		dst[i + 1] = src[i + 1];
		dst[i + 2] = src[i + 2];
		dst[i + 3] = src[i + 3];
	}
}
