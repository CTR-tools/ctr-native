#ifndef CTR_NATIVE
// NOTE(aalhendi): This TU owns both FUNC extents instead of GCC's size
// directives; keep the GTE schedule intact before the first section too.
__asm__(".set noreorder\n");
#endif

#include <ctr_types.h>

// NOTE(aalhendi): Retail consumes and returns t3-t7, with v1/t0/t1 scratch.
// Declare the tuple before common.h's inline definitions in this isolated TU.
#ifndef CTR_NATIVE
register u32 rotT3 CTR_PSX_REGISTER("$11");
register u32 rotT4 CTR_PSX_REGISTER("$12");
register u32 rotT5 CTR_PSX_REGISTER("$13");
register u32 rotT6 CTR_PSX_REGISTER("$14");
register u32 rotT7 CTR_PSX_REGISTER("$15");
#endif

#include <common.h>
#include <ctr_gte_transfer.h>

#ifdef CTR_NATIVE
#define MATH_ROT_ARGUMENTS u32 *r0, u32 *r1, u32 *r2, u32 *r3, u32 *r4

void MATRIX_SET_r11r12r13r14r15(u32 r0, u32 r1, u32 r2, u32 r3, u32 r4)
{
	CTR_GteWriteControl(r0, 0);
	CTR_GteWriteControl(r1, 1);
	CTR_GteWriteControl(r2, 2);
	CTR_GteWriteControl(r3, 3);
	CTR_GteWriteControl(r4, 4);
}
#else
#define MATH_ROT_ARGUMENTS void
#endif

// Compose three columns through the current ROT matrix and load the product.
void Unknown_8006c49c(MATH_ROT_ARGUMENTS)
{
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Snapshot all inputs before publishing possibly aliased outputs.
	u32 rotT3 = *r0;
	u32 rotT4 = *r1;
	u32 rotT5 = *r2;
	u32 rotT6 = *r3;
	u32 rotT7 = *r4;
#endif
	register u32 packed CTR_PSX_REGISTER("$3");
	register u32 work CTR_PSX_REGISTER("$8");
	register u32 highMask CTR_PSX_REGISTER("$9");

	// NOTE(aalhendi): Keep the shared mask materialization ahead of column zero.
	highMask = 0xffff0000u;
	CTR_PSX_OBSERVE_VALUE(highMask);
	packed = rotT3 & 0xffffu;
	work = rotT4 & highMask;
	packed |= work;
	MTC2(packed, 0);
	MTC2(rotT6, 1);
	packed = rotT3 >> 16;
	work = rotT5 << 16;
	gte_rtv0_b();
	packed |= work;
	work = rotT6 >> 16;
	MTC2(packed, 2);
	MTC2(work, 3);
	packed = rotT4 & 0xffffu;
	work = rotT5 & highMask;
	packed |= work;
	rotT3 = MFC2(9);
	rotT4 = MFC2(10);
	rotT6 = MFC2(11);
	gte_rtv1_b();
	MTC2(packed, 4);
	MTC2(rotT7, 5);
	rotT3 &= 0xffffu;
	rotT4 <<= 16;
	rotT6 &= 0xffffu;
	packed = MFC2(9);
	rotT5 = MFC2(10);
	work = MFC2(11);
	gte_rtv2_b();
	packed <<= 16;
	rotT3 |= packed;
	rotT5 &= 0xffffu;
	work <<= 16;
	rotT6 |= work;
	packed = MFC2(9);
	work = MFC2(10);
	rotT7 = MFC2(11);
	packed &= 0xffffu;
	rotT4 |= packed;
	work <<= 16;
	rotT5 |= work;

#ifdef CTR_NATIVE
	*r0 = rotT3;
	*r1 = rotT4;
	*r2 = rotT5;
	*r3 = rotT6;
	*r4 = rotT7;
#else
	// NOTE(aalhendi): Composition falls into a separately callable loader.
	// Reacquire its real input tuple; measure both FUNCs from actual section ends.
	__asm__ volatile(".set noreorder\n"
	                 ".Lrot_compose_end:\n"
	                 ".type Unknown_8006c49c,@function\n"
	                 ".size Unknown_8006c49c,.Lrot_compose_end-Unknown_8006c49c\n"
	                 ".section .MATRIX_SET_r11r12r13r14r15,\"ax\",@progbits\n"
	                 ".globl MATRIX_SET_r11r12r13r14r15\n"
	                 ".type MATRIX_SET_r11r12r13r14r15,@function\n"
	                 "MATRIX_SET_r11r12r13r14r15:\n"
	                 : "+r"(rotT3), "+r"(rotT4), "+r"(rotT5), "+r"(rotT6), "+r"(rotT7)
	                 :
	                 : "memory");
#endif
	// NOTE(aalhendi): Load computed locals, not aliased output pointers. Keep
	// these writes in place: GCC 2.8.1 copies inline-helper arguments to new GPRs.
	CTR_GteWriteControl(rotT3, 0);
	CTR_GteWriteControl(rotT4, 1);
	CTR_GteWriteControl(rotT5, 2);
	CTR_GteWriteControl(rotT6, 3);
	CTR_GteWriteControl(rotT7, 4);
}

#ifndef CTR_NATIVE
__asm__(".section .MATRIX_SET_r11r12r13r14r15,\"ax\",@progbits\n"
        ".Lrot_loader_end:\n"
        ".size MATRIX_SET_r11r12r13r14r15,.Lrot_loader_end-MATRIX_SET_r11r12r13r14r15\n"
        ".set reorder\n"
        ".text\n");
#endif

#undef MATH_ROT_ARGUMENTS
