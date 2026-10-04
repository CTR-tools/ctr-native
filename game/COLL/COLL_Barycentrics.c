#include <common.h>

// NOTE(aalhendi): The shared algorithm uses bounded CPU seams for retail's
// delay-slot zero, trapping shift negation, and raw divide/clamp sequence.
#ifdef CTR_NATIVE
#define BARY_READ_DATA(dst, reg) ((dst) = MFC2_S(reg))
#define BARY_CLAMP_SHIFT(shift, scratch) \
	do                                   \
	{                                    \
		(scratch) = (shift) - 12;        \
		if ((shift) < 0)                 \
			(shift) = 0;                 \
		else if ((scratch) > 0)          \
			(shift) = 12;                \
	} while (0)
#define BARY_RESCALE_DENOMINATOR(denominator, shift, negative)      \
	do                                                              \
	{                                                               \
		(negative) = -(shift);                                      \
		if ((shift) < 0)                                            \
			(denominator) = CTR_MipsSra((denominator), (negative)); \
	} while (0)
#define BARY_CLAMP_QUOTIENT(factor, numerator, denominator)               \
	do                                                                    \
	{                                                                     \
		s32 baryPastOne;                                                  \
		register s32 baryQuotient CTR_PSX_REGISTER("$10");                \
		(factor) = 0;                                                     \
		if ((denominator) != 0)                                           \
		{                                                                 \
			baryQuotient = CTR_MipsDiv((numerator), (denominator));       \
			baryPastOne = CTR_MipsSubLo(baryQuotient, COLL_FRACTION_ONE); \
			if (baryQuotient >= 0)                                        \
			{                                                             \
				(factor) = baryQuotient;                                  \
				if (baryPastOne > 0)                                      \
					(factor) = COLL_FRACTION_ONE;                         \
			}                                                             \
		}                                                                 \
	} while (0)
#else
#define BARY_READ_DATA(dst, reg) __asm__ volatile("mfc2 %0,$" CTR_GTE_STRINGIFY(reg) : "=r"(dst))
#define BARY_CLAMP_SHIFT(shift, scratch)                       \
	__asm__ volatile(".set\tnoreorder\n\t"                     \
	                 "bgez %0,.Lbary_shift_nonnegative_%=\n\t" \
	                 "addiu %1,%0,-12\n\t"                     \
	                 "bgez $zero,.Lbary_shift_done_%=\n\t"     \
	                 "addiu %0,$zero,0\n\t"                    \
	                 ".Lbary_shift_nonnegative_%=:\n\t"        \
	                 "blez %1,.Lbary_shift_done_%=\n\t"        \
	                 "nop\n\t"                                 \
	                 "addiu %0,$zero,12\n\t"                   \
	                 ".Lbary_shift_done_%=:\n\t.set\treorder"  \
	                 : "+r"(shift), "=&r"(scratch))
#define BARY_RESCALE_DENOMINATOR(denominator, shift, negative)  \
	__asm__ volatile(".set\tnoreorder\n\t"                      \
	                 "bgez %2,.Lbary_rescale_done_%=\n\t"       \
	                 "sub %1,$zero,%2\n\t"                      \
	                 "srav %0,%0,%1\n\t"                        \
	                 ".Lbary_rescale_done_%=:\n\t.set\treorder" \
	                 : "+r"(denominator), "=r"(negative)        \
	                 : "r"(shift))
#define BARY_CLAMP_QUOTIENT(factor, numerator, denominator)                     \
	do                                                                          \
	{                                                                           \
		s32 baryPastOne;                                                        \
		register s32 baryQuotient CTR_PSX_REGISTER("$10");                      \
		__asm__ volatile(".set\tnoreorder\n\t"                                  \
		                 "beq %3,$zero,.Lbary_quotient_done_%=\n\t"             \
		                 "addiu %0,$zero,0\n\t"                                 \
		                 "div $zero,%1,%3\n\t"                                  \
		                 "mflo %2\n\t"                                          \
		                 "bltz %2,.Lbary_quotient_done_%=\n\t"                  \
		                 "addiu %1,%2,-4096\n\t"                                \
		                 "blez %1,.Lbary_quotient_done_%=\n\t"                  \
		                 "addu %0,%2,$zero\n\t"                                 \
		                 "addiu %0,$zero,4096\n\t"                              \
		                 ".Lbary_quotient_done_%=:\n\t.set\treorder"            \
		                 : "=&r"(factor), "=r"(baryPastOne), "=r"(baryQuotient) \
		                 : "r"(denominator), "1"(numerator)                     \
		                 : "hi", "lo");                                         \
	} while (0)
#endif

void COLL_FIXED_TRIANGL_Barycentrics(SVec3 *out, const SVec3 *v1, const SVec3 *v2, const SVec3 *point)
{
	// NOTE(aalhendi): Local bindings anchor operands; native keeps ordinary C values.
	register SVec3 *output CTR_PSX_REGISTER("$4") = out;
	register const SVec3 *origin CTR_PSX_REGISTER("$5") = v1;
	register const SVec3 *endpoint CTR_PSX_REGISTER("$6") = v2;
	register const SVec3 *sample CTR_PSX_REGISTER("$7") = point;
	s32 v1x;
	s32 v1y;
	s32 v1z;
	s32 edgeX;
	s32 edgeY;
	s32 edgeZ;
	register s32 pointDot CTR_PSX_REGISTER("$3");
	register s32 edgeDot CTR_PSX_REGISTER("$8");
	// NOTE(aalhendi): Scale holds the bit shift, then the clamped 4.12 fraction.
	register s32 scale CTR_PSX_REGISTER("$9");
	register s32 shiftPastTwelve CTR_PSX_REGISTER("$10");
	register s32 negativeShift CTR_PSX_REGISTER("$10");

	CTR_PSX_KEEP_VALUE(output);
	{
		register s32 pointX CTR_PSX_REGISTER("$1");
		register s32 pointY CTR_PSX_REGISTER("$2");
		register s32 pointZ CTR_PSX_REGISTER("$25");
		register u32 edgeXY CTR_PSX_REGISTER("$3");
		register u32 edgePoint CTR_PSX_REGISTER("$8");
		register u32 pointYZ CTR_PSX_REGISTER("$9");
		register u32 pointZHigh CTR_PSX_REGISTER("$10");

		// Read every input before publishing output; projection can be in place.
		v1x = origin->x;
		v1y = origin->y;
		v1z = origin->z;
		CTR_PSX_MEMORY_BARRIER();
		edgeX = endpoint->x;
		edgeY = endpoint->y;
		edgeZ = endpoint->z;
		CTR_PSX_MEMORY_BARRIER();
		pointX = (s32)sample->x;
		pointY = sample->y;
		pointZ = sample->z;
		CTR_PSX_ORDER_VALUES(pointX, pointY);
		CTR_PSX_ORDER_VALUES(pointX, pointZ);
		edgeX -= v1x;
		edgeY -= v1y;
		edgeZ -= v1z;
		// NOTE(aalhendi): Finish the edge vector before forming the point offset.
		CTR_PSX_ORDER_VALUES(edgeX, edgeY);
		pointX -= v1x;
		pointY -= v1y;
		pointZ -= v1z;
		CTR_PSX_ORDER_VALUES(pointY, pointZ);
		// NOTE(aalhendi): Keep each packed half in its retail register through the merge.
		edgeXY = (u32)(u16)edgeX;
		edgePoint = (u32)edgeY << 16;
		CTR_PSX_ORDER_VALUES(edgeXY, edgePoint);
		edgeXY |= edgePoint;
		edgePoint = (u32)(u16)edgeZ;
		pointYZ = (u32)pointX << 16;
		CTR_PSX_ORDER_VALUES(edgePoint, pointYZ);
		edgePoint |= pointYZ;
		pointYZ = (u32)(u16)pointY;
		pointZHigh = (u32)pointZ << 16;
		CTR_PSX_ORDER_VALUES(pointYZ, pointZHigh);
		pointYZ |= pointZHigh;

		CTC2(edgeXY, 0);
		CTC2(edgePoint, 1);
		CTC2(pointYZ, 2);
		MTC2(edgeXY, 0);
		MTC2_S(edgeZ, 1);
	}
	CTR_PSX_GTE_PIPELINE_DELAY();
	doCOP2(0x0406012);
	BARY_READ_DATA(pointDot, 26);
	BARY_READ_DATA(edgeDot, 25);
	MTC2_S(pointDot, 30);
	CTR_PSX_GTE_PIPELINE_DELAY();
	BARY_READ_DATA(scale, 31);
	CTR_PSX_GTE_READ_DELAY();
	scale -= 2;

	BARY_CLAMP_SHIFT(scale, shiftPastTwelve);
	pointDot = CTR_MipsSll(pointDot, scale);
	scale -= 12;
	BARY_RESCALE_DENOMINATOR(edgeDot, scale, negativeShift);

	// NOTE(aalhendi): A negative wrapped signed-16 sum of squares is at most
	// -2^30; shifting by at most 12 cannot produce the overflowing divisor -1.
	BARY_CLAMP_QUOTIENT(scale, pointDot, edgeDot);

	MTC2_S(scale, 8);
	MTC2_S(v1x, 25);
	MTC2_S(v1y, 26);
	MTC2_S(v1z, 27);
	MTC2_S(edgeX, 9);
	MTC2_S(edgeY, 10);
	MTC2_S(edgeZ, 11);
	CTR_PSX_GTE_PIPELINE_DELAY();
	doCOP2(0x01a8003e);
	CTR_PSX_OBSERVE_VALUE(output);
	CTR_PSX_OBSERVE_VALUE(origin);
	CTR_PSX_OBSERVE_VALUE(endpoint);
	CTR_PSX_OBSERVE_VALUE(sample);
	BARY_READ_DATA(edgeX, 25);
	BARY_READ_DATA(edgeY, 26);
	BARY_READ_DATA(edgeZ, 27);
	output->x = (s16)edgeX;
	output->y = (s16)edgeY;
	output->z = (s16)edgeZ;
}

#undef BARY_CLAMP_SHIFT
#undef BARY_RESCALE_DENOMINATOR
#undef BARY_CLAMP_QUOTIENT
#undef BARY_READ_DATA
