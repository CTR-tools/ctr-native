#include <common.h>
#include <ctr_trig.h>

int MATH_Sin(u32 angle)
{
	return CTR_SinFromTable(data.trigApprox, angle);
}
