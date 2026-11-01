typedef int s32;
typedef unsigned int u32;
extern s32 HighProvider(s32 value, u32 *calls);
#define H(a, b) __builtin_smul_high_si(a, b)

u32 HighMixCallBranch(s32 a, s32 b, s32 c, u32 *calls)
{
	s32 x = H(a, b);
	u32 result = (u32)HighProvider(c, calls);
	if (c < 0)
		result ^= (u32)a * (u32)b;
	else
		result ^= (u32)a / ((u32)c | 1u);
	return result ^ (u32)x;
}
