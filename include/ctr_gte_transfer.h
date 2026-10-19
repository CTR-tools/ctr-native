/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * Includes GTE code adapted from PSn00bSDK's inline_c.h.
 * (C) 2019 Lameguy64
 * (C) 2021-2022 Soapy (tweaked by spicyjpeg)
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef CTR_GTE_TRANSFER_H
#define CTR_GTE_TRANSFER_H

#include <ctr_gte.h>

// NOTE(aalhendi): Compose five packed words through the current GTE ROT matrix
// and leave the product loaded. Retail's kernel consumes and returns t3-t7;
// native keeps its callable C implementation behind the same pointer API.
#ifdef CTR_NATIVE
#define CTR_GteComposeRotationWords(r0, r1, r2, r3, r4) Unknown_8006c49c(r0, r1, r2, r3, r4)
#else
static inline void CTR_GteComposeRotationWords(u32 *r0, u32 *r1, u32 *r2, u32 *r3, u32 *r4)
{
	register u32 t3 CTR_PSX_REGISTER("$11") = *r0;
	register u32 t4 CTR_PSX_REGISTER("$12") = *r1;
	register u32 t5 CTR_PSX_REGISTER("$13") = *r2;
	register u32 t6 CTR_PSX_REGISTER("$14") = *r3;
	register u32 t7 CTR_PSX_REGISTER("$15") = *r4;
	__asm__ volatile(".set\tnoreorder\n\tjal Unknown_8006c49c\n\tnop\n\t.set\treorder"
	                 : "+r"(t3), "+r"(t4), "+r"(t5), "+r"(t6), "+r"(t7)
	                 :
	                 : "$3", "$8", "$9", "$31", "memory");
	*r0 = t3;
	*r1 = t4;
	*r2 = t5;
	*r3 = t6;
	*r4 = t7;
}
#endif

// NOTE(aalhendi): Word-aligned data-register input with a literal reg; caller owns the load delay.
#ifdef CTR_NATIVE
#define CTR_GteLoadDataWord(address, reg) MTC2(CTR_ReadU32LE(address), reg)
#else
#define CTR_GteLoadDataWord(address, reg) __asm__ volatile("lwc2 $" CTR_GTE_STRINGIFY(reg) ",0(%0)" : : "r"(address) : "memory")
#endif

// NOTE(aalhendi): Keep the read delay inside the transfer when writing directly
// to a signed C destination. The compiler owns the register and final store.
#ifdef CTR_NATIVE
#define CTR_GteReadDataDelayed(dst, reg) ((dst) = MFC2_S(reg))
#else
#define CTR_GteReadDataDelayed(dst, reg) __asm__ volatile("mfc2 %0,$" CTR_GTE_STRINGIFY(reg) "\n\tnop" : "=r"(dst))
#endif

// The MAC2 read fills MAC1's CPU load delay. A caller using MAC2 must delay too.
#ifdef CTR_NATIVE
#define CTR_GteReadMAC12(mac1, mac2) \
	do                               \
	{                                \
		(mac1) = MFC2_S(25);         \
		(mac2) = MFC2_S(26);         \
	} while (0)
#else
#define CTR_GteReadMAC12(mac1, mac2) __asm__ volatile("mfc2 %0,$25\n\tmfc2 %1,$26" : "=r"(mac1), "=r"(mac2))
#endif

// NOTE(aalhendi): Three word-aligned SDK vectors; native does not read VZ padding.
#ifdef CTR_NATIVE
#define CTR_GteLoadPositionsV0V1V2(first, second, third) CTR_GteLoadSV3(first, second, third)
#define CTR_GteStorePositionsXY(first, second, third)    CTR_GteStoreSXY3(first, second, third)
#define CTR_GteSetGeomOffset(x, y)                       gte_SetGeomOffset(x, y)
#else
#define CTR_GteLoadPositionsV0V1V2(first, second, third)  \
	__asm__ volatile("lwc2 $0,0(%0)\n\tlwc2 $1,4(%0)\n\t" \
	                 "lwc2 $2,0(%1)\n\tlwc2 $3,4(%1)\n\t" \
	                 "lwc2 $4,0(%2)\n\tlwc2 $5,4(%2)\n\t" \
	                 "nop\n\tnop"                         \
	                 :                                    \
	                 : "r"(first), "r"(second), "r"(third), "m"(*(first)), "m"(*(second)), "m"(*(third)))
#define CTR_GteStorePositionsXY(first, second, third) \
	__asm__ volatile("swc2 $12,0(%0)\n\tswc2 $13,0(%1)\n\tswc2 $14,0(%2)" : : "r"(first), "r"(second), "r"(third) : "memory")
#define CTR_GteSetGeomOffset(x, y) __asm__ volatile("sll $12,%0,16\n\tsll $13,%1,16\n\tctc2 $12,$24\n\tctc2 $13,$25" : : "r"(x), "r"(y) : "$12", "$13")
#endif

// Word-aligned vectors. LH also sign-extends the row's Z into R21's upper half.
static inline void CTR_GteLoadDotProduct(const SVec3 *row, const SVec3 *vector)
{
#ifdef CTR_NATIVE
	CTC2(CTR_PackS16Pair(row->x, row->y), 0);
	CTC2((u32)(s32)row->z, 1);
	MTC2(CTR_PackS16Pair(vector->x, vector->y), 0);
	MTC2((u32)(s32)vector->z, 1);
#else
	__asm__ volatile("lw $12,0(%0)\n\tlh $13,4(%0)\n\t"
	                 "ctc2 $12,$0\n\tctc2 $13,$1\n\t"
	                 "lw $12,0(%1)\n\tlh $13,4(%1)\n\t"
	                 "mtc2 $12,$0\n\tmtc2 $13,$1"
	                 :
	                 : "r"(row), "r"(vector), "m"(*row), "m"(*vector)
	                 : "$12", "$13");
#endif
}

// Adapted from PSn00bSDK's gte_mvlvtr; see THIRD_PARTY_NOTICES.md.
// NOTE(aalhendi): Preserve full MAC words; three reads fill the CPU load delays before TR writes.
static inline void CTR_GteMoveMACToTranslation(void)
{
#ifdef CTR_NATIVE
	s32 mac1 = MFC2_S(25);
	s32 mac2 = MFC2_S(26);
	s32 mac3 = MFC2_S(27);

	gte_ldtr(mac1, mac2, mac3);
#else
	__asm__ volatile("mfc2 $12,$25\n\t"
	                 "mfc2 $13,$26\n\t"
	                 "mfc2 $14,$27\n\t"
	                 "ctc2 $12,$5\n\t"
	                 "ctc2 $13,$6\n\t"
	                 "ctc2 $14,$7"
	                 :
	                 :
	                 : "$12", "$13", "$14");
#endif
}

// NOTE(aalhendi): These transfers retain the PsyQ instruction schedule;
// native builds use the equivalent GTE interface.
static inline void CTR_GteSetTransVector(const VECTOR *translation)
{
#ifdef CTR_NATIVE
	gte_SetTransVector(translation);
#else
	__asm__ volatile("lw $12,0(%0)\n\t"
	                 "lw $13,4(%0)\n\t"
	                 "lw $14,8(%0)\n\t"
	                 "ctc2 $12,$5\n\t"
	                 "ctc2 $13,$6\n\t"
	                 "ctc2 $14,$7"
	                 :
	                 : "r"(translation)
	                 : "$12", "$13", "$14", "memory");
#endif
}

static inline void CTR_GteSetRotMatrix(const MATRIX *matrix)
{
#ifdef CTR_NATIVE
	gte_SetRotMatrix(matrix);
#else
	__asm__ volatile("lw $12,0(%0)\n\t"
	                 "lw $13,4(%0)\n\t"
	                 "ctc2 $12,$0\n\t"
	                 "ctc2 $13,$1\n\t"
	                 "lw $12,8(%0)\n\t"
	                 "lw $13,12(%0)\n\t"
	                 "lw $14,16(%0)\n\t"
	                 "ctc2 $12,$2\n\t"
	                 "ctc2 $13,$3\n\t"
	                 "ctc2 $14,$4"
	                 :
	                 : "r"(matrix)
	                 : "$12", "$13", "$14", "memory");
#endif
}

#ifdef CTR_NATIVE
static inline void CTR_GteLoadPositionV0(const SVec4 *worldPosition)
{
	MTC2(CTR_PackS16Pair(worldPosition->x, worldPosition->y), 0);
	MTC2(CTR_PackS16Pair(worldPosition->z, 0), 1);
}
#else
// NOTE(aalhendi): Keep the address expression at the SDK transfer site; an
// inline-function parameter makes GCC 2.8 hoist stack addresses out of loops.
#define CTR_GteLoadPositionV0(worldPosition)                                                              \
	do                                                                                                    \
	{                                                                                                     \
		__asm__ volatile("lwc2 $0,0(%0)\n\tlwc2 $1,4(%0)" : : "r"(worldPosition), "m"(*(worldPosition))); \
		CTR_PSX_GTE_PIPELINE_DELAY();                                                                     \
	} while (0)
#endif

static inline void CTR_GteStorePositionXY(s16 *screenPosition)
{
	CTR_PSX_STORE_COP2_WORD(screenPosition, 14);
}

// Three packed words: V0 XY, V1 XY, and their shared signed Z coordinate.
#ifdef CTR_NATIVE
#define CTR_GteLoadLineV0V1(line)                       \
	do                                                  \
	{                                                   \
		MTC2(CTR_ReadU32LE((const u8 *)(line)), 0);     \
		MTC2(CTR_ReadU32LE((const u8 *)(line) + 8), 1); \
		MTC2(CTR_ReadU32LE((const u8 *)(line) + 4), 2); \
		MTC2(CTR_ReadU32LE((const u8 *)(line) + 8), 3); \
	} while (0)
#else
// NOTE(aalhendi): The transfer reads all three words, independently of the
// caller's pointer type. Keep those reads visible to the memory scheduler.
#define CTR_GteLoadLineV0V1(line)                             \
	do                                                        \
	{                                                         \
		__asm__ volatile("lwc2 $0,0(%0)\n\tlwc2 $1,8(%0)\n\t" \
		                 "lwc2 $2,4(%0)\n\tlwc2 $3,8(%0)\n\t" \
		                 "nop\n\tnop"                         \
		                 :                                    \
		                 : "r"(line)                          \
		                 : "memory");                         \
	} while (0)
#endif

static inline void CTR_GteStoreLineXY(void *screenPositions)
{
#ifdef CTR_NATIVE
	CTR_WriteU32LE(screenPositions, (u32)MFC2(12));
	CTR_WriteU32LE((u8 *)screenPositions + 4, (u32)MFC2(13));
#else
	__asm__ volatile("swc2 $12,0(%0)\n\tswc2 $13,4(%0)" : : "r"(screenPositions) : "memory");
#endif
}

// Word-aligned XYZ vectors; the GTE ignores the upper half of each Z word.
#ifdef CTR_NATIVE
#define CTR_GteLoadPositionsV0V1(first, second) \
	do                                          \
	{                                           \
		CTR_GteLoadSVec3V0(first);              \
		CTR_GteLoadSVec3V1(second);             \
	} while (0)
#else
#define CTR_GteLoadPositionsV0V1(first, second)           \
	__asm__ volatile("lwc2 $0,0(%0)\n\tlwc2 $1,4(%0)\n\t" \
	                 "lwc2 $2,0(%1)\n\tlwc2 $3,4(%1)\n\t" \
	                 "nop\n\tnop"                         \
	                 :                                    \
	                 : "r"(first), "r"(second)            \
	                 : "memory")
#endif

static inline s32 CTR_GteReadDepthZ1(void)
{
#ifdef CTR_NATIVE
	return (s32)MFC2(17);
#else
	s32 depth;
	__asm__ volatile("mfc2 %0,$17\n\tnop" : "=r"(depth));
	return depth;
#endif
}

#ifdef CTR_NATIVE
#define CTR_GteSetTransMatrix(matrix) gte_SetTransMatrix(matrix)
#else
#define CTR_GteSetTransMatrix(matrix)    \
	__asm__ volatile("lw $12,20(%0)\n\t" \
	                 "lw $13,24(%0)\n\t" \
	                 "ctc2 $12,$5\n\t"   \
	                 "lw $14,28(%0)\n\t" \
	                 "ctc2 $13,$6\n\t"   \
	                 "ctc2 $14,$7"       \
	                 :                   \
	                 : "r"(matrix)       \
	                 : "$12", "$13", "$14", "memory")
#endif

#endif
