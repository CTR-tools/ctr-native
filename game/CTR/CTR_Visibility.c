#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80021da0-0x80021e1c.
void CTR_unknownMaybeThunk1(void *dst, void *src)
{
	u8 *out = (u8 *)dst;
	s8 *rle = (s8 *)src;

	for (;;)
	{
		int count = *rle;

		if (count == 0)
		{
			return;
		}

		if (count < 0)
		{
			int repeat = 1 - count;
			u8 value = (u8)rle[1];
			rle += 2;

			while (repeat-- != 0)
			{
				*out++ = value;
			}
		}

		else
		{
			rle++;

			while (count-- != 0)
			{
				*out++ = (u8)*rle++;
			}
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80021e1c-0x80021ea8.
void CTR_unknownMaybeThunk2(void *dst, void *src)
{
	u8 *out = (u8 *)dst;
	s8 *rle = (s8 *)src;

	for (;;)
	{
		int count = *rle;

		if (count == 0)
		{
			return;
		}

		if (count < 0)
		{
			int repeat = 1 - count;
			u8 value = (u8)rle[1];
			rle += 2;

			while (repeat-- != 0)
			{
				*out++ |= value;
			}
		}

		else
		{
			rle++;

			while (count-- != 0)
			{
				*out++ |= (u8)*rle++;
			}
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80021ea8-0x80021edc.
void CTR_unknownMaybeThunk3(void *dst, void *src, int byteCount)
{
	u32 *out = (u32 *)dst;
	u32 *in = (u32 *)src;
	u32 wordCount = (u32)(byteCount >> 2);

	while (wordCount != 0)
	{
		*out++ |= *in++;
		wordCount--;
	}
}
