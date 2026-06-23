#ifndef CTR_MATH_H
#define CTR_MATH_H

#include <macros.h>

// vectors //

typedef union SVec2
{
	struct
	{
		s16 x;
		s16 y;
	};
	s16 v[2];
} SVec2;

typedef union SVec3
{
	struct
	{
		s16 x;
		s16 y;
		s16 z;
	};
	s16 v[3];
} SVec3;

typedef union SVec4
{
	struct
	{
		s16 x;
		s16 y;
		s16 z;
		s16 w;
	};
	s16 v[4];
} SVec4;

typedef union SVec3Slot
{
	struct
	{
		SVec3 vec;
		s16 pad;
	};
	struct
	{
		s16 x;
		s16 y;
		s16 z;
		s16 w;
	};
	s16 v[4];
} SVec3Slot;

CTR_STATIC_ASSERT_LAYOUT(sizeof(SVec3Slot) == 0x8);
CTR_STATIC_ASSERT_LAYOUT(offsetof(SVec3Slot, vec) == 0x0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(SVec3Slot, pad) == 0x6);
CTR_STATIC_ASSERT_LAYOUT(offsetof(SVec3Slot, x) == 0x0);
CTR_STATIC_ASSERT_LAYOUT(offsetof(SVec3Slot, y) == 0x2);
CTR_STATIC_ASSERT_LAYOUT(offsetof(SVec3Slot, z) == 0x4);
CTR_STATIC_ASSERT_LAYOUT(offsetof(SVec3Slot, w) == 0x6);

typedef union Vec2
{
	struct
	{
		s32 x;
		s32 y;
	};
	s32 v[2];
} Vec2;

typedef union Vec3
{
	struct
	{
		s32 x;
		s32 y;
		s32 z;
	};
	s32 v[3];
} Vec3;

typedef union Vec4
{
	struct
	{
		s32 x;
		s32 y;
		s32 z;
		s32 w;
	};
	s32 v[4];
} Vec4;

typedef struct Matrix
{
	s16 m[3][3];
	Vec3 t;
} Matrix;

// trigonometry //

struct TrigTable
{
	s16 sin;
	s16 cos;
};

#define ANG_TWO_PI                         0x1000           // 360
#define ANG_PI                             (ANG_TWO_PI / 2) // 180
#define ANG_HALF_PI                        (ANG_PI / 2)     // 90

#define ANG_MODULO_TWO_PI(x)               ((x) & (ANG_TWO_PI - 1))  // ang % 360
#define ANG_MODULO_PI(x)                   ((x) & (ANG_PI - 1))      // ang % 180
#define ANG_MODULO_HALF_PI(x)              ((x) & (ANG_HALF_PI - 1)) // ang % 90

#define ANG(x)                             ANG_MODULO_TWO_PI(((s16)((((float)x) * ANG_TWO_PI) / 360))) // works for any float, pos or neg

#define IS_ANG_FIRST_OR_THIRD_QUADRANT(x)  (((x) & ANG_HALF_PI) == 0) // [0, 90[ \/ [180, 270[
#define IS_ANG_THIRD_OR_FOURTH_QUADRANT(x) ((x) & ANG_PI)             // [180, 360[

// fixed point //

#define FRACTIONAL_BITS_8                  8
#define FP8_ONE                            (1 << FRACTIONAL_BITS_8)
#define FP8_INT(x)                         ((x) >> FRACTIONAL_BITS_8)
#define FP8_MULT(x, y)                     (((x) * (y)) >> FRACTIONAL_BITS_8)
#define FP8(x)                             ((int)(((float)x) * FP8_ONE))

#define FRACTIONAL_BITS                    12
#define FP_ONE                             (1 << FRACTIONAL_BITS)
#define FP_INT(x)                          ((x) >> FRACTIONAL_BITS)
#define FP_MULT(x, y)                      (((x) * (y)) >> FRACTIONAL_BITS)
#define FP(x)                              ((int)(((float)x) * FP_ONE))

// at least one of the operands needs to be a fixed point value converted to integer form
// e.g. FP_Mult(0x1000, 0x2000) or FP_Mult(FP(1.0), FP(2.0)) or FP_Mult(3, FP(0.75))
static inline int FP_Mult(int x, int y)
{
	return (x * y) >> FRACTIONAL_BITS;
}

// MIPS R3000 integer operations used in ASM-audited code paths. These keep
// overflow, shifts, multiply-low, divide traps, and truncation points explicit.
static inline s32 CTR_MipsSll(s32 value, u32 shift)
{
	return (s32)((u32)value << (shift & 0x1f));
}

static inline s32 CTR_MipsSra(s32 value, u32 shift)
{
	shift &= 0x1f;

	return (s32)(((u32)value >> shift) | ((value < 0) ? ~(0xffffffffu >> shift) : 0));
}

static inline u32 CTR_MipsSrl(s32 value, u32 shift)
{
	return (u32)value >> (shift & 0x1f);
}

static inline s32 CTR_MipsMulLo(s32 lhs, s32 rhs)
{
	return (s32)((u32)lhs * (u32)rhs);
}

static inline u32 CTR_MipsMulHiU(u32 lhs, u32 rhs)
{
	return (u32)(((u64)lhs * (u64)rhs) >> 32);
}

static inline s32 CTR_MipsAddLo(s32 lhs, s32 rhs)
{
	return (s32)((u32)lhs + (u32)rhs);
}

static inline s32 CTR_MipsSubLo(s32 lhs, s32 rhs)
{
	return (s32)((u32)lhs - (u32)rhs);
}

static inline s32 CTR_MipsNegLo(s32 value)
{
	return CTR_MipsSubLo(0, value);
}

static inline u32 CTR_PackS16Pair(s32 lo, s32 hi)
{
	return (u32)(u16)lo | ((u32)(u16)hi << 16);
}

static inline s32 CTR_MipsDiv(s32 dividend, s32 divisor)
{
	const s32 minS32 = (-2147483647 - 1);

	if ((divisor == 0) || ((divisor == -1) && (dividend == minS32)))
	{
		__builtin_trap();
	}

	return dividend / divisor;
}

static inline u32 CTR_MipsDivU(u32 dividend, u32 divisor)
{
	if (divisor == 0)
	{
		__builtin_trap();
	}

	return dividend / divisor;
}

// misc //

#ifndef CTR_NATIVE
int abs(int value);
#endif

int clamp(int n, int lo, int hi);
int max(int a, int b);
int min(int a, int b);
s32 MATH_VectorLength(SVec3 *vector);
s32 MATH_VectorNormalize(SVec3 *vector);

#endif
