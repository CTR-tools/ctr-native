#include <ctr_types.h>

// NOTE(aalhendi): Retail takes s0 and returns zero-extended s1/s2, clobbering
// only v1. Keep these register owners before common.h and in this retail TU.
#ifndef CTR_NATIVE
register u32 trigAngle CTR_PSX_REGISTER("$16");
register u32 trigSine CTR_PSX_REGISTER("$17");
register u32 trigCosine CTR_PSX_REGISTER("$18");
#endif

#include <common.h>

#ifdef CTR_NATIVE
#define TRIG_UNSIGNED_ARGUMENTS u32 angle, u32 *sine, u32 *cosine
#else
#define TRIG_UNSIGNED_ARGUMENTS void
#endif

// NOTE(aalhendi): Retail uses checked SUB, which GCC 2.8.1 does not emit for
// C negation. Native retains wrapping, including restored non-authored table words.
#ifdef CTR_NATIVE
#define TRIG_NEGATE_IN_PLACE(value) ((value) = 0u - (value))
#else
#define TRIG_NEGATE_IN_PLACE(value) __asm__ volatile("sub %0,$0,%0" : "+r"(value))
#endif

void TRIG_AngleSinCos_r16r17r18_duplicate(TRIG_UNSIGNED_ARGUMENTS)
{
#ifdef CTR_NATIVE
	u32 trigAngle = angle;
	u32 trigSine;
	u32 trigCosine;
#endif
	register u32 trigWork CTR_PSX_REGISTER("$3");

	trigWork = (u32)&data.trigApprox[0];
	trigSine = ANG_MODULO_HALF_PI(trigAngle);
	trigSine <<= 2;
	// NOTE(aalhendi): Forget the symbolic base equivalence to retain ADDU operand order.
	CTR_PSX_KEEP_VALUE(trigWork);
	trigWork += trigSine;
	trigSine = CTR_ReadU32AlignedLE((const void *)trigWork);
	trigWork = trigAngle & ANG_QUADRANT_BIT;
	if (trigWork != 0)
		goto quadrant;
	trigWork = trigAngle & ANG_SIGN_BIT;
	trigCosine = trigSine >> 16;
	if (trigWork != 0)
		goto both_negative;
	trigSine &= 0xffffu;
	goto finished;

quadrant:
	trigWork = trigAngle & ANG_SIGN_BIT;
	trigCosine = trigSine & 0xffffu;
	trigSine >>= 16;
	if (trigWork != 0)
		goto sine_negative;
	TRIG_NEGATE_IN_PLACE(trigCosine);
	trigCosine &= 0xffffu;
	goto finished;

both_negative:
	TRIG_NEGATE_IN_PLACE(trigCosine);
	TRIG_NEGATE_IN_PLACE(trigSine);
	trigCosine &= 0xffffu;
	trigSine &= 0xffffu;
	goto finished;

sine_negative:
	TRIG_NEGATE_IN_PLACE(trigSine);
	trigSine &= 0xffffu;

finished:
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Preserve the public cosine-first interface when outputs alias.
	if (sine == cosine && ((trigAngle & ANG_QUADRANT_BIT) != 0) != ((trigAngle & ANG_SIGN_BIT) != 0))
		trigSine = (0u - trigSine) & 0xffffu;
	*cosine = trigCosine;
	*sine = trigSine;
#else
	return;
#endif
}

#undef TRIG_NEGATE_IN_PLACE
#undef TRIG_UNSIGNED_ARGUMENTS
