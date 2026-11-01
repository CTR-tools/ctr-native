typedef unsigned int u32;

u32 CoalesceComputed(u32 a, u32 b)
{
	u32 value = a * 17U + 5U;
	u32 dependency = (b ^ 0x5a5a5a5aU) + 3U;
	u32 result;
	__asm__("" : "=r"(result) : "0"(value), "r"(dependency));
	return result ^ 0x13579bdfU;
}

u32 CoalesceComputedReverse(u32 a, u32 b)
{
	u32 value = (a ^ 0x13579bdfU) + 7U;
	u32 dependency = b * 9U + 11U;
	u32 result;
	__asm__("" : "=r"(result) : "r"(dependency), "0"(value));
	return (result << 1) | (result >> 31);
}
