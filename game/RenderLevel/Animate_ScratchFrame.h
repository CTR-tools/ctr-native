/* Intentionally re-included; each entry undefines these local frame adapters. */
// NOTE(aalhendi): Source owns balanced S0-S7/RA saves at 0x00..0x1c/0x28;
// 0x20/0x24 stay untouched. The provider preserves AT while this image is live.
#ifdef CTR_NATIVE
#define ANIMATE_SCRATCH_DECLARE()
#define ANIMATE_SCRATCH_SAVE()    ((void)0)
#define ANIMATE_SCRATCH_RESTORE() ((void)0)
#else
#define ANIMATE_SCRATCH_DECLARE()                                                              \
	register u32 *animateFrame CTR_PSX_REGISTER("$1");                                         \
	register u32 animateSaved0 CTR_PSX_REGISTER("$16"), animateSaved1 CTR_PSX_REGISTER("$17"); \
	register u32 animateSaved2 CTR_PSX_REGISTER("$18"), animateSaved3 CTR_PSX_REGISTER("$19"); \
	register u32 animateSaved4 CTR_PSX_REGISTER("$20"), animateSaved5 CTR_PSX_REGISTER("$21"); \
	register u32 animateSaved6 CTR_PSX_REGISTER("$22"), animateSaved7 CTR_PSX_REGISTER("$23"); \
	register u32 animateSavedRa CTR_PSX_REGISTER("$31")
#define ANIMATE_SCRATCH_SAVE()                                                                                                                           \
	do                                                                                                                                                   \
	{                                                                                                                                                    \
		__asm__ volatile(""                                                                                                                              \
		                 : "=r"(animateSaved0), "=r"(animateSaved1), "=r"(animateSaved2), "=r"(animateSaved3), "=r"(animateSaved4), "=r"(animateSaved5), \
		                   "=r"(animateSaved6), "=r"(animateSaved7), "=r"(animateSavedRa));                                                              \
		animateFrame = (u32 *)0x1f800000u;                                                                                                               \
		animateFrame[0] = animateSaved0;                                                                                                                 \
		animateFrame[1] = animateSaved1;                                                                                                                 \
		animateFrame[2] = animateSaved2;                                                                                                                 \
		animateFrame[3] = animateSaved3;                                                                                                                 \
		animateFrame[4] = animateSaved4;                                                                                                                 \
		animateFrame[5] = animateSaved5;                                                                                                                 \
		animateFrame[6] = animateSaved6;                                                                                                                 \
		animateFrame[7] = animateSaved7;                                                                                                                 \
		animateFrame[10] = animateSavedRa;                                                                                                               \
	} while (0)
#define ANIMATE_SCRATCH_RESTORE()                                                                                                                  \
	do                                                                                                                                             \
	{                                                                                                                                              \
		animateSavedRa = animateFrame[10];                                                                                                         \
		animateSaved7 = animateFrame[7];                                                                                                           \
		animateSaved6 = animateFrame[6];                                                                                                           \
		animateSaved5 = animateFrame[5];                                                                                                           \
		animateSaved4 = animateFrame[4];                                                                                                           \
		animateSaved3 = animateFrame[3];                                                                                                           \
		animateSaved2 = animateFrame[2];                                                                                                           \
		animateSaved1 = animateFrame[1];                                                                                                           \
		animateSaved0 = animateFrame[0];                                                                                                           \
		__asm__ volatile(""                                                                                                                        \
		                 :                                                                                                                         \
		                 : "r"(animateSaved0), "r"(animateSaved1), "r"(animateSaved2), "r"(animateSaved3), "r"(animateSaved4), "r"(animateSaved5), \
		                   "r"(animateSaved6), "r"(animateSaved7), "r"(animateSavedRa));                                                           \
	} while (0)
#endif
