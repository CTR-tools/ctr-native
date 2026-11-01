typedef unsigned int u32;

u32 EmptyAlu(u32 a, u32 b)
{
	u32 x = a + 17U;
	__asm__("" : "=r"(x) : "0"(x));
	return x ^ b;
}

u32 EmptyLoad(const u32 *p, u32 a)
{
	u32 x = *p;
	__asm__("" : "=r"(x) : "0"(x));
	return x ^ a;
}

u32 EmptyMultiply(u32 a, u32 b)
{
	u32 x = a * b;
	__asm__("" : "=l"(x) : "0"(x));
	return x ^ 0x13579bdfU;
}
