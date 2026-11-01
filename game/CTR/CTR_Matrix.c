#include <common.h>

// NOTE(aalhendi): Keep retail's row/diagonal address evaluation order.
#define CTR_MATRIX_ROW(matrix, axis)            ((s16 *)((axis) * sizeof((matrix)->m[0]) + (u8 *)(matrix)))
#define CTR_MATRIX_ELEMENT(matrix, row, column) (*((column) + CTR_MATRIX_ROW(matrix, row)))
#define CTR_MATRIX_DIAGONAL(matrix, axis)       (*(s16 *)((axis) * 8 + (u8 *)(matrix)))

// Recover Euler angles from a matrix, including the singular-axis case.
// Flags select X/Z swapping (bit 0), repeated axes (bit 1), negation (bit 2)
// and the initial axis (bits 3-4); the original flags remain in rot->pad.
void CTR_MatrixToRot(SVECTOR *rot, MATRIX *matrix, u32 flags)
{
	u32 order = flags;
	s32 repeatedAxis;
	s32 negate;
	s32 swapXZ = order & 1;
	const u8 *const firstAxes = GAME_SDATA_NAME(matrixToRotInitialAxis);
	const u8 *axisEntry;
	s32 axis;
	s32 next;
	s32 last;
	s32 reverseStep;

	order >>= 1;
	repeatedAxis = order & 1;
	order >>= 1;
	negate = order & 1;
	order >>= 1;
	order &= 3;
	// NOTE(aalhendi): A direct pointer lookup reschedules the entry loads.
	axisEntry = order + firstAxes;
	axis = GAME_SDATA_NAME(matrixToRotInitialAxis)[axisEntry - firstAxes];
	next = GAME_SDATA_NAME(matrixToRotNextAxis)[axis + negate];
	reverseStep = negate - 1;
	last = GAME_SDATA_NAME(matrixToRotNextAxis)[axis - reverseStep];

	// NOTE(aalhendi): Each signed-halfword square fits s32, but their sum
	// needs u32 for the full input range. Keep the retail matrix access order.
	// At the singularity, choose zero for the unresolved Z angle.
	if (repeatedAxis == 1)
	{
		s32 magnitude = MATH_FastSqrt((u32)(CTR_MATRIX_ELEMENT(matrix, axis, next) * CTR_MATRIX_ELEMENT(matrix, axis, next)) +
		                                  (u32)(CTR_MATRIX_ELEMENT(matrix, axis, last) * CTR_MATRIX_ELEMENT(matrix, axis, last)),
		                              24) >>
		                12;

		if (magnitude >= 17)
		{
			rot->vx = (s16)ratan2(CTR_MATRIX_ELEMENT(matrix, axis, next), CTR_MATRIX_ELEMENT(matrix, axis, last));
			rot->vy = (s16)ratan2(magnitude, CTR_MATRIX_DIAGONAL(matrix, axis));
			rot->vz = (s16)ratan2(CTR_MATRIX_ELEMENT(matrix, next, axis), -CTR_MATRIX_ELEMENT(matrix, last, axis));
		}
		else
		{
			rot->vx = (s16)ratan2(-CTR_MATRIX_ELEMENT(matrix, next, last), CTR_MATRIX_DIAGONAL(matrix, next));
			rot->vy = (s16)ratan2(magnitude, CTR_MATRIX_DIAGONAL(matrix, axis));
			rot->vz = 0;
		}
	}
	else
	{
		s32 magnitude = MATH_FastSqrt((u32)(CTR_MATRIX_DIAGONAL(matrix, axis) * CTR_MATRIX_DIAGONAL(matrix, axis)) +
		                                  (u32)(CTR_MATRIX_ELEMENT(matrix, next, axis) * CTR_MATRIX_ELEMENT(matrix, next, axis)),
		                              24) >>
		                12;

		if (magnitude >= 17)
		{
			rot->vx = (s16)ratan2(CTR_MATRIX_ELEMENT(matrix, last, next), CTR_MATRIX_DIAGONAL(matrix, last));
			rot->vy = (s16)ratan2(-CTR_MATRIX_ELEMENT(matrix, last, axis), magnitude);
			rot->vz = (s16)ratan2(CTR_MATRIX_ELEMENT(matrix, next, axis), CTR_MATRIX_DIAGONAL(matrix, axis));
		}
		else
		{
			rot->vx = (s16)ratan2(-CTR_MATRIX_ELEMENT(matrix, next, last), CTR_MATRIX_DIAGONAL(matrix, next));
			rot->vy = (s16)ratan2(-CTR_MATRIX_ELEMENT(matrix, last, axis), magnitude);
			rot->vz = 0;
		}
	}

	if (negate == 1)
	{
		rot->vx = -rot->vx;
		rot->vy = -rot->vy;
		rot->vz = -rot->vz;
	}
	if (swapXZ == 1)
	{
		s32 x = rot->vx;
		rot->vx = rot->vz;
		rot->vz = x;
	}
	rot->pad = (s16)flags;
}

#undef CTR_MATRIX_ROW
#undef CTR_MATRIX_ELEMENT
#undef CTR_MATRIX_DIAGONAL
