#ifndef CTR_TRIG_H
#define CTR_TRIG_H

#include <ctr_math.h>

// NOTE(aalhendi): One work word retains the packed lookup through half selection and sign.
static inline s32 CTR_SinFromTable(struct TrigTable *trigBase, u32 angle)
{
	u32 trig = CTR_ReadU32LE(&trigBase[ANG_MODULO_HALF_PI(angle)]);

	if ((angle & ANG_QUADRANT_BIT) == 0)
	{
		trig <<= 0x10;
	}

	trig = (u32)((s32)trig >> 0x10);
	if ((angle & ANG_SIGN_BIT) != 0)
	{
		trig = 0u - trig;
	}

	return (s32)trig;
}

#endif
