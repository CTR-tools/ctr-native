/* Compile-only unsafe ABI controls. These providers are declarations, not hardware tests. */
typedef unsigned int u32;
#define MANUAL __attribute__((mips_unsafe_manual_at_frame))
#if defined(TEST_ORDINARY_CALL)
#define PROVIDER
#elif defined(TEST_UNASSERTED_CALL)
#define PROVIDER __attribute__((mips_regcall_s012(1u << 14)))
#else
#define PROVIDER __attribute__((mips_regcall_s012(1u << 14), mips_preserves_at24))
#endif
extern void ScratchProvider(void) PROVIDER;
volatile u32 scratch_publication, scratch_before, scratch_after;

/* The source owns the actual AT window and balanced incoming register tuple. */
#define DECLARE_FRAME()                                            \
	register u32 *frame __asm__("$1");                             \
	register u32 saved_ra __asm__("$31"), saved_s0 __asm__("$16"); \
	register u32 saved_s1 __asm__("$17"), saved_s2 __asm__("$18")
#define SAVE_FRAME()                                                                           \
	do                                                                                         \
	{                                                                                          \
		__asm__ volatile("" : "=r"(saved_ra), "=r"(saved_s0), "=r"(saved_s1), "=r"(saved_s2)); \
		frame = (u32 *)0x1f800000u;                                                            \
		frame[0] = saved_ra;                                                                   \
		frame[1] = saved_s0;                                                                   \
		frame[2] = saved_s1;                                                                   \
		frame[3] = saved_s2;                                                                   \
	} while (0)
#define RESTORE_FRAME()      \
	do                       \
	{                        \
		saved_ra = frame[0]; \
		saved_s0 = frame[1]; \
		saved_s1 = frame[2]; \
		saved_s2 = frame[3]; \
	} while (0)
#define OBSERVE_RETURN()                                                                                \
	do                                                                                                  \
	{                                                                                                   \
		__asm__ volatile("" : : "r"(saved_ra), "r"(saved_s0), "r"(saved_s1), "r"(saved_s2) : "memory"); \
		return;                                                                                         \
	} while (0)

void ScratchRelay(u32 value);
void ScratchCallerBefore(u32 value)
{
	ScratchRelay(value);
	scratch_before++;
}
void MANUAL ScratchRelay(u32 value)
{
	DECLARE_FRAME();
#if defined(TEST_STACK)
	volatile u32 local;
	local = value;
#endif
#if defined(TEST_EXTRA_SAVE)
	register u32 extra __asm__("$19");
	__asm__ volatile("" : "=r"(extra));
#endif
	SAVE_FRAME();
	frame[4] = value;
	ScratchProvider();
#if defined(TEST_EXTRA_SAVE)
	__asm__ volatile("" : : "r"(extra));
#endif
	RESTORE_FRAME();
	scratch_publication = frame[4];
	OBSERVE_RETURN();
}
void ScratchRelay(u32 value);
void ScratchCallerAfter(u32 value)
{
	ScratchRelay(value);
	scratch_after++;
}

#if defined(TEST_INLINE_EARLIER)
static inline void ScratchTiny(void);
#else
static void ScratchTiny(void);
#endif
#if defined(TEST_INLINE_INITIAL)
static inline void MANUAL ScratchTiny(void)
#else
static void MANUAL ScratchTiny(void)
#endif
{
	DECLARE_FRAME();
	SAVE_FRAME();
	RESTORE_FRAME();
	OBSERVE_RETURN();
}
#if defined(TEST_INLINE_LATER)
static inline void ScratchTiny(void);
#else
static void ScratchTiny(void);
#endif
void (*volatile scratch_indirect)(void) = ScratchTiny;
void ScratchDirect(void)
{
	ScratchTiny();
	scratch_after++;
}
void ScratchIndirect(void)
{
	scratch_indirect();
	scratch_after++;
}
static u32 ScratchOrdinaryTiny(u32 value)
{
	return value + 7u;
}
void ScratchOrdinary(void)
{
	scratch_publication = ScratchOrdinaryTiny(scratch_before);
}
