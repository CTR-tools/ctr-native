#include <common.h>

s32 MATH_VectorNormalize(SVec3 *vector)
{
	s32 length = MATH_VectorLength(vector);

	if (length != 0)
	{
		vector->x = (vector->x * 0x1000) / length;
		vector->y = (vector->y * 0x1000) / length;
		vector->z = (vector->z * 0x1000) / length;
	}

	return length;
}
