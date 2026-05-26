#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80059070-0x80059100.
u32 VehCalc_FastSqrt(u32 n, u32 shift)
{
	u32 result = 0;
	u32 rootBitIndex = 1;
	u32 lastApproximation = 0;
	u32 testBit = 1 << (shift & 0x1f);

	while ((testBit < n) && ((testBit << 2) != 0))
	{
		rootBitIndex++;
		testBit <<= 2;
	}

	u32 addBit = 1 << ((rootBitIndex + (shift - 1)) & 0x1f);

	while (testBit != 0)
	{
		u32 shiftedResult = result << (rootBitIndex & 0x1f);

		if ((s32)rootBitIndex < 0)
		{
			shiftedResult = result >> ((-rootBitIndex) & 0x1f);
		}

		u32 approximation = shiftedResult + lastApproximation + testBit;

		if (approximation <= n)
		{
			result += addBit;
			lastApproximation = approximation;
		}

		addBit >>= 1;
		testBit >>= 2;
		rootBitIndex--;
	}

	return result;
}
