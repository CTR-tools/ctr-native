#include <common.h>

CTR_STATIC_ASSERT(offsetof(struct Data, trigApprox) == 0x3c00);

#ifdef CTR_NATIVE
static void TRIG_AngleSinCos_Common(u32 angle, s32 *sine, s32 *cosine)
{
#include "TRIG_AngleSinCos.inc"
}

void TRIG_AngleSinCos_r19r17r18(u32 angle, s32 *sine, s32 *cosine)
{
	TRIG_AngleSinCos_Common(angle, sine, cosine);
}

void TRIG_AngleSinCos_r15r16r17(u32 angle, s32 *sine, s32 *cosine)
{
	TRIG_AngleSinCos_Common(angle, sine, cosine);
}

void TRIG_AngleSinCos_r16r17r18(u32 angle, s32 *sine, s32 *cosine)
{
	TRIG_AngleSinCos_Common(angle, sine, cosine);
}

void TRIG_AngleSinCos_r9r8r10(u32 angle, s32 *sine, s32 *cosine)
{
	TRIG_AngleSinCos_Common(angle, sine, cosine);
}
#endif
