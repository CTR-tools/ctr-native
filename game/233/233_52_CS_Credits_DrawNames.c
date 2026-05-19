#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800b88c8-0x800b8bd0
void CS_Credits_DrawNames(struct CreditsObj *co)
{
	if (co->credits_topString == 0)
		return;

	co->credits_posY--;

	if (co->credits_posY < -20)
	{
		co->credits_topString = CS_Credits_GetNextString(co->credits_topString);
		co->credits_posY += 20;
	}

	int posY = co->credits_posY;
	char *str = co->credits_topString;
	int charId = 0;

	while (posY < 0x114)
	{
		unsigned short textFlags = 0;

		if (*str == '~')
		{
			char *p = str + 2;

			do
			{
				int digit1 = (unsigned char)p[-1] - 0x30;
				int digit2 = (unsigned char)p[0];
				p += 3;
				int value = digit2 + digit1 * 10 - 0x30;
				str += 3;

				if (value < 0x32)
				{
					charId = value;
				}
				else if (value == 0x34)
				{
					textFlags |= 0x2000;
				}
				else if (value == 0x35)
				{
					textFlags |= 0x1000;
				}
			} while (*str == '~');
		}

		char *nextStr = CS_Credits_GetNextString(str);
		short strLen;

		if (nextStr == 0)
			strLen = strlen(str);
		else
			strLen = (short)(nextStr - str) - 1;

		int clampedY = posY;
		int fadeAmount = 20;

		if (clampedY < 0x83)
		{
			if (clampedY < 0x14)
				fadeAmount = clampedY;
		}
		else
		{
			fadeAmount = 0x96 - clampedY;
		}

		int colorSlot = charId;

		if ((fadeAmount < 20) && (creditsBSS.boolAllBlue != 0))
		{
			if (fadeAmount < 1)
			{
				colorSlot = -1;
			}
			else
			{
				int fade8 = (fadeAmount << 8) / 20;
				char *src = (char *)data.ptrColor[charId];
				char *dst = (char *)&data.colors[31];

				for (int i = 0; i < 4; i++)
				{
					int stride = i * 4;
					dst[stride + 0] = (char)((unsigned char)src[stride + 0] * fade8 >> 8);
					dst[stride + 1] = (char)((unsigned char)src[stride + 1] * fade8 >> 8);
					dst[stride + 2] = (char)((unsigned char)src[stride + 2] * fade8 >> 8);
				}
			}
		}

		if (colorSlot >= 0)
		{
			DECOMP_DecalFont_DrawLineStrlen((unsigned char *)str, strLen, creditsBSS.creditText_PosX, posY, 3, colorSlot | textFlags);
		}

		posY += 20;

		if (nextStr == 0)
			return;

		str = nextStr;
	}
}
