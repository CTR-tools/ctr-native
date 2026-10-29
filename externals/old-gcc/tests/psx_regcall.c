/* Compile-only attribute controls. CASE must be selected explicitly by CPP. */
#ifndef CASE
#error Select one documented CASE
#endif

extern unsigned int ParserMask;

#if CASE == 1
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(16384u)));
#elif CASE == 2
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(1u << 14)));
#elif CASE == 3
extern void ParserLeaf(void) __attribute__((mips_regcall_s012((1u << 13) + (1u << 13))));
#elif CASE == 4
extern void ParserLeaf(void) __attribute__((mips_regcall_s012((1u << 14) | (0u << 2))));
#elif CASE == 5
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(16384u)));
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(1u << 14)));
#elif CASE == 6
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(1u << 14)));
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(16384u)));
#elif CASE == 7
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(0u)));
#elif CASE == 8
extern void ParserLeaf(void) __attribute__((mips_regcall_s012((unsigned char)(1u << 14))));
#elif CASE == 9
extern void ParserLeaf(void) __attribute__((mips_regcall_s012((3u << 2) | (3u << 8) | (3u << 14) | (3u << 24))));
#elif CASE == 20
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(1u << 14)));
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(1u << 15)));
#elif CASE == 21
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(ParserMask + 0u)));
#elif CASE == 22
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(ParserMask)));
#elif CASE == 23
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(16384.0)));
#elif CASE == 24
extern void ParserLeaf(void) __attribute__((mips_regcall_s012("16384")));
#elif CASE == 25
extern void ParserLeaf(void) __attribute__((mips_regcall_s012((void *)16384u)));
#elif CASE == 26
extern void ParserLeaf(void) __attribute__((mips_regcall_s012()));
#elif CASE == 27
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(16384u, 16384u)));
#elif CASE == 28
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(-1)));
#elif CASE == 29
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(1u)));
#elif CASE == 30
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(1u << 1)));
#elif CASE == 31
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(1u << 26)));
#elif CASE == 32
extern void ParserLeaf(void) __attribute__((mips_regcall_s012(1ULL << 40)));
#elif CASE == 33
extern void ParserLeaf(void) __attribute__((mips_regcall_s012((1u << 25) + (1u << 25))));
#else
#error Unknown CASE
#endif

void ParserRelay(void)
{
	ParserLeaf();
}

#if defined(TEST_ADDRESS)
void (*ParserAddress)(void) = ParserLeaf;
#endif
#if defined(TEST_MISSING_REDECL)
extern void ParserLeaf(void);
#endif
