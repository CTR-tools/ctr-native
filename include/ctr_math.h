#ifndef CTR_MATH_H
#define CTR_MATH_H

#include <macros.h>

// vectors //

typedef struct SVec2
{
	s16 x;
	s16 y;
} SVec2;

typedef struct SVec3
{
	s16 x;
	s16 y;
	s16 z;
} SVec3;

typedef struct SVec4
{
	s16 x;
	s16 y;
	s16 z;
	s16 w;
} SVec4;

typedef struct SVec3Slot
{
	s16 x;
	s16 y;
	s16 z;
	s16 w;
} SVec3Slot;

CTR_STATIC_ASSERT(sizeof(SVec3Slot) == 0x8);
CTR_STATIC_ASSERT(offsetof(SVec3Slot, x) == 0x0);
CTR_STATIC_ASSERT(offsetof(SVec3Slot, y) == 0x2);
CTR_STATIC_ASSERT(offsetof(SVec3Slot, z) == 0x4);
CTR_STATIC_ASSERT(offsetof(SVec3Slot, w) == 0x6);

static inline SVec3 *SVec3Slot_AsVec3(SVec3Slot *slot)
{
	return (SVec3 *)slot;
}

static inline const SVec3 *SVec3Slot_AsConstVec3(const SVec3Slot *slot)
{
	return (const SVec3 *)slot;
}

typedef struct Vec2
{
	s32 x;
	s32 y;
} Vec2;

// NOTE(aalhendi): Vec3 can also view the SDK's three contiguous translation words.
typedef struct CTR_MAY_ALIAS Vec3
{
	s32 x;
	s32 y;
	s32 z;
} Vec3;

typedef struct Vec4
{
	s32 x;
	s32 y;
	s32 z;
	s32 w;
} Vec4;

typedef s32 VecElement CTR_MAY_ALIAS;

#define CTR_VECTOR_DATA(VECTOR) (&(VECTOR)->x)

// trigonometry //

struct TrigTable
{
	s16 sin;
	s16 cos;
};

struct TrigPair
{
	s32 sin;
	s32 cos;
};

#define ANG_TWO_PI                         0x1000           // 360
#define ANG_PI                             (ANG_TWO_PI / 2) // 180
#define ANG_HALF_PI                        (ANG_PI / 2)     // 90
#define ANG_QUADRANT_MASK                  (ANG_HALF_PI - 1)
#define ANG_QUADRANT_BIT                   ANG_HALF_PI
#define ANG_SIGN_BIT                       ANG_PI
#define ANG_QUADRANT_BITS                  (ANG_QUADRANT_BIT | ANG_SIGN_BIT)

#define ANG_MODULO_TWO_PI(x)               ((x) & (ANG_TWO_PI - 1))  // ang % 360
#define ANG_MODULO_PI(x)                   ((x) & (ANG_PI - 1))      // ang % 180
#define ANG_MODULO_HALF_PI(x)              ((x) & ANG_QUADRANT_MASK) // ang % 90

#define ANG(x)                             ANG_MODULO_TWO_PI(((s16)((((float)x) * ANG_TWO_PI) / 360))) // works for any float, pos or neg

#define IS_ANG_FIRST_OR_THIRD_QUADRANT(x)  (((x) & ANG_QUADRANT_BIT) == 0) // [0, 90[ \/ [180, 270[
#define IS_ANG_THIRD_OR_FOURTH_QUADRANT(x) ((x) & ANG_SIGN_BIT)            // [180, 360[

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

// MIPS R3000 integer helpers keep overflow, shifts, multiply-low, divide traps,
// and truncation points explicit. Native-only masks and guards avoid ISO C
// undefined behavior; retail expressions remain visible to GCC 2.8.1 codegen.
static inline s32 CTR_MipsSll(s32 value, u32 shift)
{
#if defined(CTR_NATIVE)
	shift &= 0x1f;
#endif

	return (s32)((u32)value << shift);
}

static inline s32 CTR_MipsSra(s32 value, u32 shift)
{
#if defined(CTR_NATIVE)
	shift &= 0x1f;
	return (s32)(((u32)value >> shift) | ((value < 0) ? ~(0xffffffffu >> shift) : 0));
#else
	return value >> shift;
#endif
}

static inline u32 CTR_MipsSrl(s32 value, u32 shift)
{
#if defined(CTR_NATIVE)
	shift &= 0x1f;
#endif

	return (u32)value >> shift;
}

static inline s32 CTR_MipsMulLo(s32 lhs, s32 rhs)
{
	return (s32)((u32)lhs * (u32)rhs);
}

static inline u32 CTR_MipsMulHiU(u32 lhs, u32 rhs)
{
	return (u32)(((u64)lhs * (u64)rhs) >> 32);
}

#ifdef CTR_NATIVE
// NOTE(aalhendi): Pure signed high half; no emulated HI/LO state is changed.
static inline s32 CTR_MipsMulHiS(s32 lhs, s32 rhs)
{
	return (s32)((u64)((s64)lhs * (s64)rhs) >> 32);
}
#endif

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

// NOTE(aalhendi): Ordinary signed negation requires value != INT_MIN. The
// PSX compiler primitive selects SUB without changing other C operators.
#ifdef CTR_NATIVE
static inline s32 CTR_NegateS32(s32 value)
{
	return -value;
}
#endif

static inline u32 CTR_PackS16Pair(s32 lo, s32 hi)
{
	return (u32)(u16)lo | ((u32)(u16)hi << 16);
}

static inline s32 CTR_MipsDiv(s32 dividend, s32 divisor)
{
#if defined(CTR_NATIVE)
	const s32 minS32 = (-2147483647 - 1);

	if ((divisor == 0) || ((divisor == -1) && (dividend == minS32)))
	{
		CTR_TRAP();
	}
#endif

	return dividend / divisor;
}

static inline u32 CTR_MipsDivU(u32 dividend, u32 divisor)
{
#if defined(CTR_NATIVE)
	if (divisor == 0)
	{
		CTR_TRAP();
	}
#endif

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
