#include <common.h>

/// @brief used for matrices generated without rotation, which need rotation, like AngleAxis matrices
///        for instance, called when player fires a bowling bomb
/// @param rot - resulting rotation vector
/// @param matrix - instance matrix
/// @param flags - 3 bits boots + 2 bits some offset value (b0 - switch XZ, b1 - ?, b2 - negate the result)
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80021edc-0x80022234.
void CTR_MatrixToRot(SVECTOR *rot, MATRIX *matrix, u32 flags)
{
	int iVar2;
	int iVar3;
	s16 *psVar5;
	s16 *psVar11;

	// bit 2 check - negate vector
	u32 uVar7 = flags >> 2 & 1;

	// TODO: update global table def and use that instead of local
	// TODO: figure out better names for whatever indices these are
	// my guess these are axes
	// there are 2 char[] tables
	// 0 1 2 0 0 0 0 0
	// 1 2 0 1 0 0 0 0

	// 8008D004
	// char unk_CTR_MatrixToRot_table[0x10];
// NOTE(aalhendi): CTR_NATIVE mirrors the retail 0x8008d004 table through sdata.
#if defined(CTR_NATIVE)
	char *table1 = &sdata->unk_CTR_MatrixToRot_table[0];
	char *table2 = &sdata->unk_CTR_MatrixToRot_table[8];
#else
	char *table1 = (char *)0x8008d004;
	char *table2 = (char *)0x8008d00C;
#endif

	// take value from the first table
	u32 t1value = (u32)table1[flags >> 3 & 3];

	// take two values from the second table
	u32 t2value1 = (u32)table2[t1value + uVar7];
	u32 t2value2 = (u32)table2[t1value - (uVar7 - 1)];

	// bit 1 check - uses flipped matrix or smth?
	if ((flags >> 1 & 1) == 1)
	{
		// TODO: there's definitely more elegant way for this
		// probably it was smth like matrix->m[val1][val2]

		psVar5 = matrix->m[t1value] + t2value1;
		iVar2 = (int)*psVar5;

		psVar11 = matrix->m[t1value] + t2value2;
		iVar3 = (int)*psVar11;

		iVar2 = MATH_FastSqrt(iVar2 * iVar2 + iVar3 * iVar3, 0x18) >> FRACTIONAL_BITS;

		// based on sqrt result, calculate vector
		if (iVar2 < 0x11)
		{
			rot->vx = (s16)ratan2(-(int)matrix->m[t2value1][t2value2], (int)*(s16 *)((uintptr_t)matrix->m + t2value1 * 8));
			rot->vy = (s16)ratan2(iVar2, (s32) * (s16 *)((uintptr_t)matrix->m + t1value * 8));
			rot->vz = (s16)0;
		}
		else
		{
			rot->vx = (s16)ratan2((int)*psVar5, (int)*psVar11);
			rot->vy = (s16)ratan2(iVar2, (int)*(s16 *)((uintptr_t)matrix->m + t1value * 8));
			rot->vz = (s16)ratan2((int)matrix->m[t2value1][t1value], -(int)matrix->m[t2value2][t1value]);
		}
	}
	else
	{
		// TODO: there's definitely more elegant way for this
		// probably it was smth like matrix->m[val1][val2]

		psVar5 = (s16 *)((uintptr_t)matrix->m + t1value * 8);
		iVar2 = (int)*psVar5;

		psVar11 = matrix->m[t2value1] + t1value;
		iVar3 = (int)*psVar11;

		iVar2 = MATH_FastSqrt(iVar2 * iVar2 + iVar3 * iVar3, 0x18) >> FRACTIONAL_BITS;

		// based on sqrt result, calculate vector
		if (iVar2 < 0x11)
		{
			rot->vx = (s16)ratan2(-(int)matrix->m[t2value1][t2value2], (int)*(s16 *)((uintptr_t)matrix->m + t2value1 * 8));
			rot->vy = (s16)ratan2(-(int)matrix->m[t2value2][t1value], iVar2);
			rot->vz = (s16)0;
		}
		else
		{
			rot->vx = (s16)ratan2((int)matrix->m[t2value2][t2value1], (int)*(s16 *)((uintptr_t)matrix->m + t2value2 * 8));
			rot->vy = (s16)ratan2(-(int)matrix->m[t2value2][t1value], iVar2);
			rot->vz = (s16)ratan2((int)*psVar11, (int)*psVar5);
		}
	}

	// bit 2 check - negate vector
	// flags >> 2 & 1;
	if (uVar7 == 1)
	{
		rot->vx = -rot->vx;
		rot->vz = -rot->vz;
		rot->vy = -rot->vy;
	}

	// bit 0 check - switch XZ axis
	if ((flags & 1) == 1)
	{
		// can use pad for the swap
		rot->pad = rot->vx;
		rot->vx = rot->vz;
		rot->vz = rot->pad;
	}

	// copy flags to pad
	rot->pad = (s16)flags;
}
