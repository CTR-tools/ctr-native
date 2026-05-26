#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80046bc0-0x80046c30.
int RefreshCard_GhostDecodeByte(int value)
{
	u8 byte = value;

	if (byte == '-')
		return 0x3e;

	if (byte == '_')
		return 0x3f;

	if (byte < ':')
		return byte - '0';

	if (byte < '[')
		return (s16)(byte - 0x37);

	return (s16)(byte - 0x3d);
}
