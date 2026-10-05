#include "psx_at44.h"
#ifndef CASE
#define CASE 1
#endif
#ifndef CTR_NATIVE
register u32 inputWord asm("$4");
register u32 *outputWords asm("$5");
register const u16 *cursor0 asm("$24");
#if CASE != 1
register const u16 *cursor1 asm("$25");
#endif
#endif
#define IncomingPointer(p) __builtin_mips_incoming_ptr(p)
#define RemainderHi(n,d) __builtin_mips_umod_hi((u32)(n),(u32)(d))
#define ReadHi(v) __builtin_mips_read_hi(v)
#define WriteGteData(v,i) __builtin_mips_mtc2((v),(i))
#if CASE == 2
#define ENTRY IncomingSix
#else
#define ENTRY IncomingFive
#endif

#ifdef TEST_SBANK
#ifdef TEST_NO_PRESERVE
#define PRESERVES
#else
#define PRESERVES __attribute__((mips_preserves_at44))
#endif
#ifdef TEST_BAD_CALL
void Provider(u32) __attribute__((mips_regcall_sbank(0x10))) PRESERVES;
#else
void Provider(void) __attribute__((mips_regcall_sbank(0x10))) PRESERVES;
#endif
#endif
#ifdef TEST_CALLERS
void IncomingFive(u32, u32 *, u32, u32, u16 *);
void OrdinaryBefore(u32 n, u32 *out, u16 *p) { IncomingFive(n, out, 0, 0, p); }
#endif

void FRAME_OWNER ENTRY(u32 n, u32 *out, u32 unused0, u32 unused1,
#if CASE == 8
    u32 input0
#else
    u16 *input0
#endif
#if CASE != 1
    , u16 *input1
#endif
#if CASE == 15
    , u16 *input2
#endif
)
{
    FRAME_DECLARE();
#ifdef CTR_NATIVE
    u32 inputWord = n;
    u32 *outputWords = out;
    const u16 *cursor0;
#if CASE != 1
    const u16 *cursor1;
#endif
#endif
    u32 pending, first, second, bits;
    (void)unused0; (void)unused1;
    FRAME_SAVE();
#if CASE == 7
    input0 = (u16 *)outputWords;
#endif
#if CASE == 3
    cursor0 = input0;
#elif CASE == 4
    cursor0 = IncomingPointer((void *)16);
#else
    cursor0 = IncomingPointer(input0);
#endif
#if CASE == 5
    cursor0 = IncomingPointer(input0);
#endif
#if CASE == 6
    outputWords[3] = (u32)&input0;
#elif CASE == 13
    { register u32 sp asm("$29") = inputWord; __asm__ volatile("" : : "r"(sp)); }
#endif
#if CASE == 14
    if (inputWord) goto after_branch;
    outputWords[3] = 0;
after_branch:
#endif
#if CASE != 1
    cursor1 = IncomingPointer(input1);
#endif
#if CASE == 9
    pending = RemainderHi(inputWord, 0u);
#elif CASE == 10
    pending = RemainderHi(inputWord, inputWord | 1u);
#else
    pending = RemainderHi(inputWord, 28u);
#endif
    bits = *cursor0++;
    first = ReadHi(pending);
#if CASE == 11
    WriteGteData(bits, 32);
#elif CASE == 12
    WriteGteData(bits, inputWord);
#else
    WriteGteData(bits, 8);
#endif
    pending = RemainderHi(first + 1u, 28u);
    bits ^= *cursor0;
#if CASE != 1
    bits |= *cursor1;
#endif
    second = ReadHi(pending);
    outputWords[0] = first;
    outputWords[1] = second;
    outputWords[2] = bits;
#ifdef TEST_SBANK
#ifdef TEST_CALL_ADDRESS
    outputWords[3] = (u32)Provider;
#else
    Provider();
#endif
#endif
    FRAME_RESTORE();
}
#ifdef TEST_CALLERS
void OrdinaryAfter(u32 n, u32 *out, u16 *p) { IncomingFive(n, out, 0, 0, p); }
#endif
