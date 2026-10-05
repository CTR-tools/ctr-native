#include "psx_at44.h"
#ifdef TEST_ALIAS
register u32 modeledSxy2 __asm__("gte_sxy2");
register u32 modeledVxy0 __asm__("gte_vxy0");
#ifndef DATA_INDEX
#define DATA_INDEX 0
#endif

/* Compile separately for data0, data14 and the data15 FIFO alias. */
void Mtc2Sxy2Around(u32 value, u32 *observed)
{
    u32 before = modeledSxy2;
    __builtin_mips_mtc2(value, DATA_INDEX);
    observed[0] = before;
    observed[1] = modeledSxy2;
}

/* VXY0 supports real volatile memory loads, not an ordinary C-value read. */
void Mtc2Vxy0Loads(const volatile u32 *input, u32 value)
{
    modeledVxy0 = input[0];
    __builtin_mips_mtc2(value, DATA_INDEX);
    modeledVxy0 = input[1];
}
#else
#define RemainderHi(n,d) __builtin_mips_umod_hi((u32)(n),(u32)(d))
#define ReadHi(v) __builtin_mips_read_hi(v)
u32 PendingAcrossMultiply(u32 n, u32 m)
{
    u32 pending, product;
    pending = RemainderHi(n, 3u);
    product = n * m;
    return ReadHi(pending) ^ product;
}
u32 TwoPending(u32 n, u32 m)
{
    u32 a, b;
    a = RemainderHi(n, 3u);
    b = RemainderHi(m, 5u);
    return ReadHi(a) + ReadHi(b);
}
#endif
