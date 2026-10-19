#ifndef MACROS_H
#define MACROS_H

#include <ctr_types.h>

#if defined(CTR_NATIVE)
#include <stdbool.h>
#include <stddef.h>
#else
typedef unsigned int size_t;
#define NULL                   ((void *)0)
#define false                  0
#define true                   1
#define offsetof(type, member) ((size_t)&(((type *)0)->member))
#endif

#define CTR_JOIN_IMPL(a, b) a##b
#define CTR_JOIN(a, b)      CTR_JOIN_IMPL(a, b)

#if defined(__GNUC__) && (__GNUC__ < 3)
#define CTR_STATIC_ASSERT(expr)
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define CTR_STATIC_ASSERT(expr) _Static_assert((expr), #expr)
#else
#define CTR_STATIC_ASSERT(expr) extern char CTR_JOIN(ctr_static_assert_, __LINE__)[(expr) ? 1 : -1]
#endif

CTR_STATIC_ASSERT(sizeof(u8) == 1);
CTR_STATIC_ASSERT(sizeof(s8) == 1);
CTR_STATIC_ASSERT(sizeof(u16) == 2);
CTR_STATIC_ASSERT(sizeof(s16) == 2);
CTR_STATIC_ASSERT(sizeof(u32) == 4);
CTR_STATIC_ASSERT(sizeof(s32) == 4);
CTR_STATIC_ASSERT(sizeof(u64) == 8);
CTR_STATIC_ASSERT(sizeof(s64) == 8);
CTR_STATIC_ASSERT(sizeof(void *) == 4);

#define SCREEN_HEIGHT                            216
#define FPS                                      30
#define ELAPSED_MS                               32
#define SCREEN_WIDTH                             512
#define SECOND                                   (FPS * ELAPSED_MS)
#define MINUTE                                   (SECOND * 60)
#define HOUR                                     (MINUTE * 60)

#define CTR_FRAMES_PER_SECOND                    30
#define CTR_SECONDS_TO_FRAMES(sec)               ((s32)((sec) * CTR_FRAMES_PER_SECOND))

#define SECONDS(x)                               ((s32)(((f32)(x)) * SECOND))
#define MINUTES(x)                               ((s32)(((f32)(x)) * MINUTE))
#define HOURS(x)                                 ((s32)(((f32)(x)) * HOUR))

#define nullptr                                  ((void *)0)

#define internal                                 static
#define local_persist                            static
#define global_variable                          static

#define len(arr)                                 (sizeof(arr) / sizeof(arr[0]))
#define OFFSETOF(TYPE, ELEMENT)                  ((u32)offsetof(TYPE, ELEMENT))
#define CTR_OFFSET_OF_ARRAY(TYPE, MEMBER, INDEX) (offsetof(TYPE, MEMBER) + (u32)(INDEX) * sizeof(((TYPE *)0)->MEMBER[0]))
#define CTR_OFFSET_OF_2D_ARRAY(TYPE, MEMBER, ROW, COLUMN) \
	(offsetof(TYPE, MEMBER) + (u32)(ROW) * sizeof(((TYPE *)0)->MEMBER[0]) + (u32)(COLUMN) * sizeof(((TYPE *)0)->MEMBER[0][0]))

static inline u16 CTR_ReadU16LE(const void *src)
{
#ifdef CTR_NATIVE
	const u8 *bytes = (const u8 *)src;

	return (u16)((u16)bytes[0] | ((u16)bytes[1] << 8));
#else
	return *(const CtrPackedU16 *)src;
#endif
}

static inline s16 CTR_ReadS16LE(const void *src)
{
#ifdef CTR_NATIVE
	return (s16)CTR_ReadU16LE(src);
#else
	return *(const CtrPackedS16 *)src;
#endif
}

static inline u32 CTR_ReadU32LE(const void *src)
{
#ifdef CTR_NATIVE
	const u8 *bytes = (const u8 *)src;

	return (u32)bytes[0] | ((u32)bytes[1] << 8) | ((u32)bytes[2] << 16) | ((u32)bytes[3] << 24);
#else
	return *(const CtrPackedU32 *)src;
#endif
}

static inline u32 CTR_ReadU32AlignedLE(const void *src)
{
#ifdef CTR_NATIVE
	return CTR_ReadU32LE(src);
#else
	return *(const u32 *)src;
#endif
}

static inline u16 CTR_ReadU16AlignedLE(const void *src)
{
#ifdef CTR_NATIVE
	return CTR_ReadU16LE(src);
#else
	return *(const u16 *)src;
#endif
}

static inline void CTR_WriteU16LE(void *dst, u16 value)
{
	u8 *bytes = (u8 *)dst;

	bytes[0] = (u8)value;
	bytes[1] = (u8)(value >> 8);
}

static inline void CTR_WriteU32LE(void *dst, u32 value)
{
	u8 *bytes = (u8 *)dst;

	bytes[0] = (u8)value;
	bytes[1] = (u8)(value >> 8);
	bytes[2] = (u8)(value >> 16);
	bytes[3] = (u8)(value >> 24);
}

static inline void CTR_WriteU32AlignedLE(void *dst, u32 value)
{
#ifdef CTR_NATIVE
	CTR_WriteU32LE(dst, value);
#else
	*(u32 *)dst = value;
#endif
}

// Raw [3] vector array helpers. Arguments must be side-effect-free lvalues.
#define CTR_COPY_VEC3(DST, SRC) \
	do                          \
	{                           \
		(DST)[0] = (SRC)[0];    \
		(DST)[1] = (SRC)[1];    \
		(DST)[2] = (SRC)[2];    \
	} while (0)

#define CTR_SET_VEC3(DST, X, Y, Z) \
	do                             \
	{                              \
		(DST)[0] = (X);            \
		(DST)[1] = (Y);            \
		(DST)[2] = (Z);            \
	} while (0)

// Retail format strings use PsyQ `%ld` for 32-bit values. Keep call sites on
// project-width types while satisfying host printf varargs for the literal.
#define CTR_PRINTF_PSX_LONG(value) ((long)(s32)(value))

#define RGBtoBGR(color)            ((color & 0xFF0000) >> 16) | (color & 0xFF00) | ((color & 0xFF) << 16)

#define GetRed(color)              (color & 0xFF)

#define GetGreen(color)            (color & 0xFF00) >> 8

#define GetBlue(color)             (color & 0xFF0000) >> 16

#define aspectratioupsample(int)   (int   *7) / 4

#define aspectratiodownsample(int) (int * 4) / 7

#endif
