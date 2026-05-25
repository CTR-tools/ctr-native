#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80031c1c-0x80031c58.
void LOAD_StringToUpper(char *path)
{
	for (u8 *letter = (u8 *)path; *letter != 0; letter++)
	{
		// if lowercase letter
		if ((u32)(*letter - 0x61) < 0x1a)
		{
			// uppercase
			*letter -= 0x20;
		}
	}
}
