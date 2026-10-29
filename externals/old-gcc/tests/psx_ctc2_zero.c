typedef unsigned int u32;

void ctc_zero_literal(void)
{
	__builtin_mips_ctc2(0u, 0);
}

void ctc_zero_folded(void)
{
	__builtin_mips_ctc2(0x12345678u ^ 0x12345678u, 31);
}

void ctc_zero_repeated(void)
{
	__builtin_mips_ctc2(0u, 5);
	__builtin_mips_ctc2(0u, 6);
	__builtin_mips_ctc2(0u, 7);
}

u32 ctc_zero_effectful(volatile u32 *value)
{
	__builtin_mips_ctc2(((*value)++, 0u), 4);
	return *value;
}

void ctc_zero_runtime(u32 value)
{
	__builtin_mips_ctc2(value, 4);
}

void ctc_zero_nonzero(void)
{
	__builtin_mips_ctc2(1u, 31);
}

extern void ctc_zero_keep_frame(volatile u32 *slots);

u32 ctc_zero_fifth(u32 first, u32 second, u32 third, u32 fourth, u32 fifth)
{
	volatile u32 slots[2];

	slots[0] = first ^ second;
	slots[1] = third ^ fourth;
	ctc_zero_keep_frame(slots);
	__builtin_mips_ctc2(0u, 6);
	return fifth + slots[0] + slots[1];
}

static inline u32 Ctc2ReadOrdinaryWord(const u32 *word)
{
	return *word;
}

void Ctc2OrdinaryWord(const u32 *word)
{
	__builtin_mips_ctc2(Ctc2ReadOrdinaryWord(word), 4);
}

struct Ctc2OrdinaryFourWords
{
	u32 words[4];
};

void Ctc2OrdinaryCopy(struct Ctc2OrdinaryFourWords *dst, const struct Ctc2OrdinaryFourWords *src)
{
	*dst = *src;
}
