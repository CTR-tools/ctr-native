typedef int s32;
typedef unsigned int u32;

s32 HighProvider(s32 value, u32 *calls)
{
    *calls += 1u;
    return value / 2;
}
