/* Compile-only controls. No game routine or MMIO address is recognized. */
typedef unsigned int u32;

register const u32 *lw_words __asm__("$6");
register u32 lw_condition __asm__("$4");
register u32 lw_pending __asm__("$14");
register u32 lw_seed __asm__("$3");

#define LOAD_PENDING()  (lw_pending = *(const volatile u32 *)(lw_words + 5))
#define KEEP_PAIR(a, b) __asm__ volatile("" : : "r"(a), "r"(b))

void lw_global_data(void)
{
	register u32 following __asm__("$9");
	LOAD_PENDING();
	if (lw_condition == 0)
		return;
	following = lw_words[6];
	KEEP_PAIR(lw_pending, following);
}

void lw_local_data(void)
{
	register u32 pending __asm__("$8");
	register u32 following __asm__("$9");
	pending = *(const volatile u32 *)(lw_words + 5);
	if (lw_condition == 0)
		return;
	following = lw_words[6];
	KEEP_PAIR(pending, following);
}

void lw_fallthrough_reads_pending(void)
{
	register u32 following __asm__("$9");
	LOAD_PENDING();
	if (lw_condition == 0)
		return;
	following = lw_pending + 1;
	KEEP_PAIR(lw_pending, following);
}

void lw_fallthrough_writes_pending(void)
{
	LOAD_PENDING();
	if (lw_condition == 0)
		return;
	lw_pending = lw_seed + 1;
	KEEP_PAIR(lw_pending, lw_seed);
}

void lw_taken_reads_pending(void)
{
	register u32 following __asm__("$9");
	LOAD_PENDING();
	if (lw_condition == 0)
		goto taken;
	following = lw_words[6];
	KEEP_PAIR(lw_pending, following);
	return;
taken:
	lw_pending += 1;
	KEEP_PAIR(lw_pending, lw_seed);
}

void lw_taken_writes_pending(void)
{
	register u32 following __asm__("$9");
	LOAD_PENDING();
	if (lw_condition == 0)
		goto taken;
	following = lw_words[6];
	KEEP_PAIR(lw_pending, following);
	return;
taken:
	lw_pending = lw_seed + 1;
	KEEP_PAIR(lw_pending, lw_seed);
}

void lw_branch_reads_pending(void)
{
	register u32 following __asm__("$9");
	LOAD_PENDING();
	if (lw_pending == 0)
		return;
	following = lw_words[6];
	KEEP_PAIR(lw_pending, following);
}

void lw_address_equals_destination(void)
{
	register u32 following __asm__("$9");
	lw_pending = *(const volatile u32 *)(lw_pending + 4u);
	if (lw_condition == 0)
		return;
	following = lw_words[6];
	KEEP_PAIR(lw_pending, following);
}

void lw_opaque_asm_gap(void)
{
	register u32 following __asm__("$9");
	LOAD_PENDING();
	if (lw_condition == 0)
		return;
	__asm__ volatile("nop" : : : "memory");
	following = lw_words[6];
	KEEP_PAIR(lw_pending, following);
}

void lw_empty_asm_gap(void)
{
	register u32 following __asm__("$9");
	LOAD_PENDING();
	if (lw_condition == 0)
		return;
	__asm__ volatile("" : : : "memory");
	following = lw_words[6];
	KEEP_PAIR(lw_pending, following);
}

void lw_second_volatile_read(void)
{
	register u32 following __asm__("$9");
	LOAD_PENDING();
	if (lw_condition == 0)
		return;
	following = *(const volatile u32 *)(lw_words + 6);
	KEEP_PAIR(lw_pending, following);
}

void lw_halfword_is_not_word(void)
{
	register u32 following __asm__("$9");
	lw_pending = *(const volatile unsigned short *)((const char *)lw_words + 2);
	if (lw_condition == 0)
		return;
	following = lw_words[6];
	KEEP_PAIR(lw_pending, following);
}

void lw_unaligned_word(void)
{
	register u32 following __asm__("$9");
	lw_pending = *(const volatile u32 *)((const char *)lw_words + 2);
	if (lw_condition == 0)
		return;
	following = lw_words[6];
	KEEP_PAIR(lw_pending, following);
}

void lw_not_adjacent(void)
{
	register u32 following __asm__("$9");
	LOAD_PENDING();
	lw_seed = *(const volatile u32 *)(lw_words + 7);
	if (lw_condition == 0)
		return;
	following = lw_words[6];
	KEEP_PAIR(lw_pending, following);
}
