#include <common.h>

#ifndef CTR_NATIVE
// NOTE(aalhendi): These text expansions share retail scratch saves and the
// five-word store/return tail without adding calls or changing instructions.
#define MATH_MATRIX_SAVE_SCRATCH \
	"lui $at,0x1f80\n"           \
	"sw $s0,0($at)\n"            \
	"sw $s1,4($at)\n"            \
	"sw $s2,8($at)\n"            \
	"sw $s3,12($at)\n"           \
	"sw $s4,16($at)\n"           \
	"sw $ra,44($at)\n"

#define MATH_MATRIX_STORE_AND_RETURN \
	"sw $t3,0($a0)\n"                \
	"sw $t4,4($a0)\n"                \
	"sw $t5,8($a0)\n"                \
	"sw $t6,12($a0)\n"               \
	"sw $t7,16($a0)\n"               \
	"lw $ra,44($at)\n"               \
	"lw $s4,16($at)\n"               \
	"lw $s3,12($at)\n"               \
	"lw $s2,8($at)\n"                \
	"lw $s1,4($at)\n"                \
	"lw $s0,0($at)\n"                \
	"jr $ra\n"                       \
	"nop\n"
#endif

static inline u32 MATH_Matrix_ReadWord(const void *ptr, s32 offset)
{
	return CTR_ReadU32LE((u8 *)ptr + offset);
}

static inline void MATH_Matrix_WriteWord(void *ptr, s32 offset, u32 value)
{
	CTR_WriteU32LE((u8 *)ptr + offset, value);
}

static inline u32 MATH_Matrix_NegLowWord(u32 value)
{
	return (0u - value) & 0xffff;
}

static inline u32 MATH_Matrix_NegHighWord(u32 value)
{
	return MATH_Matrix_NegLowWord(value) << 0x10;
}

#ifdef CTR_NATIVE
void Unknown_8006c600(u32 r0, u32 r1, u32 r2, u32 r3, u32 r4)
{
	// NOTE(aalhendi): Retail inputs are t3/t4/t5/t6/t7 and writes GTE regs 8-12.
	CTC2(r0, 8);
	CTC2(r1, 9);
	CTC2(r2, 10);
	CTC2(r3, 11);
	CTC2(r4, 12);
}
#else
// NOTE(aalhendi): The LIGHT loader receives t3-t7, not C ABI arguments.
__asm__(".section .Unknown_8006c600,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent Unknown_8006c600\n"
        ".set noreorder\n"
        ".globl Unknown_8006c600\n"
        "Unknown_8006c600:\n"
        "ctc2 $t3,$8\n"
        "ctc2 $t4,$9\n"
        "ctc2 $t5,$10\n"
        "ctc2 $t6,$11\n"
        "jr $ra\n"
        "ctc2 $t7,$12\n"
        ".end Unknown_8006c600\n"
        ".set reorder\n"
        ".text\n");
#endif

static void MATH_Matrix_StoreWords(void *m, u32 r0, u32 r1, u32 r2, u32 r3, u32 r4)
{
	MATH_Matrix_WriteWord(m, 0x0, r0);
	MATH_Matrix_WriteWord(m, 0x4, r1);
	MATH_Matrix_WriteWord(m, 0x8, r2);
	MATH_Matrix_WriteWord(m, 0xc, r3);
	MATH_Matrix_WriteWord(m, 0x10, r4);
}

#ifdef CTR_NATIVE
static void MATH_Matrix_MulIfNonZero(s32 angle, u32 *r0, u32 *r1, u32 *r2, u32 *r3, u32 *r4, s32 axis)
{
	u32 sine;
	u32 cosine;

	if (angle == 0)
	{
		return;
	}

	TRIG_AngleSinCos_r16r17r18_duplicate(angle, &sine, &cosine);

	if (axis == 0)
	{
		*r0 = FP_ONE;
		*r1 = 0;
		*r2 = MATH_Matrix_NegHighWord(sine) | cosine;
		*r3 = sine << 0x10;
		*r4 = cosine;
	}
	else if (axis == 1)
	{
		*r0 = cosine;
		*r1 = sine;
		*r2 = FP_ONE;
		*r3 = MATH_Matrix_NegLowWord(sine);
		*r4 = cosine;
	}
	else
	{
		*r0 = MATH_Matrix_NegHighWord(sine) | cosine;
		*r1 = sine << 0x10;
		*r2 = cosine;
		*r3 = 0;
		*r4 = FP_ONE;
	}

	Unknown_8006c49c(r0, r1, r2, r3, r4);
}

void ConvertRotToMatrix_InverseTranspose_NoRotY(MATRIX *m, const SVec3 *rot)
{
	u32 sine;
	u32 cosine;
	u32 r0;
	u32 r1;
	u32 r2;
	u32 r3 = 0;
	u32 r4 = FP_ONE;

	TRIG_AngleSinCos_r16r17r18_duplicate((s32)rot->z, &sine, &cosine);
	r0 = MATH_Matrix_NegHighWord(sine) | cosine;
	r1 = sine << 0x10;
	r2 = cosine;
	MATRIX_SET_r11r12r13r14r15(r0, r1, r2, r3, r4);

	MATH_Matrix_MulIfNonZero((s32)rot->x, &r0, &r1, &r2, &r3, &r4, 0);
	MATH_Matrix_StoreWords(m, r0, r1, r2, r3, r4);
}
#else
// NOTE(aalhendi): Retail reads X/Z before storing five packed rotation words;
// Y and translation are untouched. It saves s0-s4/RA in reserved scratchpad
// slots, not a stack frame. A nonzero full-turn X still executes the GTE path.
__asm__(".section .ConvertRotToMatrix_InverseTranspose_NoRotY,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent ConvertRotToMatrix_InverseTranspose_NoRotY\n"
        ".set\tnoreorder\n"
        ".globl ConvertRotToMatrix_InverseTranspose_NoRotY\n"
        "ConvertRotToMatrix_InverseTranspose_NoRotY:\n"
        "addiu $at,$zero,660\n" MATH_MATRIX_SAVE_SCRATCH "lh $s0,4($a1)\n"
        "lh $s3,0($a1)\n"
        "jal TRIG_AngleSinCos_r16r17r18_duplicate\n"
        "addiu $t6,$zero,0\n"
        "addiu $t7,$zero,4096\n"
        "sub $v1,$zero,$s1\n"
        "sll $v1,$v1,16\n"
        "or $t3,$v1,$s2\n"
        "sll $t4,$s1,16\n"
        "jal MATRIX_SET_r11r12r13r14r15\n"
        "addu $t5,$s2,$zero\n"
        "beq $s3,$zero,.Lmatrix_no_roty_store\n"
        "addu $s0,$s3,$zero\n"
        "jal TRIG_AngleSinCos_r16r17r18_duplicate\n"
        "addiu $t3,$zero,4096\n"
        "addiu $t4,$zero,0\n"
        "sub $v1,$zero,$s1\n"
        "sll $v1,$v1,16\n"
        "or $t5,$v1,$s2\n"
        "sll $t6,$s1,16\n"
        "jal Unknown_8006c49c\n"
        "addu $t7,$s2,$zero\n"
        ".Lmatrix_no_roty_store:\n" MATH_MATRIX_STORE_AND_RETURN ".end ConvertRotToMatrix_InverseTranspose_NoRotY\n"
        ".set\treorder\n"
        ".text\n");
#endif

#ifdef CTR_NATIVE
static void MATH_Matrix_InverseTransposeBody(MATRIX *m, s32 rotX, s32 rotZ, s32 rotY)
{
	u32 sine;
	u32 cosine;
	u32 r0;
	u32 r1;
	u32 r2;
	u32 r3 = 0;
	u32 r4 = FP_ONE;

	TRIG_AngleSinCos_r16r17r18_duplicate(rotZ, &sine, &cosine);
	r0 = MATH_Matrix_NegHighWord(sine) | cosine;
	r1 = sine << 0x10;
	r2 = cosine;
	MATRIX_SET_r11r12r13r14r15(r0, r1, r2, r3, r4);

	MATH_Matrix_MulIfNonZero(rotX, &r0, &r1, &r2, &r3, &r4, 0);
	MATH_Matrix_MulIfNonZero(rotY, &r0, &r1, &r2, &r3, &r4, 1);
	MATH_Matrix_StoreWords(m, r0, r1, r2, r3, r4);
}

void ConvertRotToMatrix_InverseTranspose(MATRIX *m, const SVec3 *rot)
{
	MATH_Matrix_InverseTransposeBody(m, (s32)rot->x, (s32)rot->z, (s32)rot->y);
}

void ConvertRotToMatrix(MATRIX *m, const SVec3 *rot)
{
	u32 sine;
	u32 cosine;
	u32 r0;
	u32 r1;
	u32 r2 = FP_ONE;
	u32 r3;
	u32 r4;

	TRIG_AngleSinCos_r16r17r18_duplicate((s32)rot->y, &sine, &cosine);
	r0 = cosine;
	r1 = sine;
	r3 = MATH_Matrix_NegLowWord(sine);
	r4 = cosine;
	MATRIX_SET_r11r12r13r14r15(r0, r1, r2, r3, r4);

	MATH_Matrix_MulIfNonZero((s32)rot->x, &r0, &r1, &r2, &r3, &r4, 0);
	MATH_Matrix_MulIfNonZero((s32)rot->z, &r0, &r1, &r2, &r3, &r4, 2);
	MATH_Matrix_StoreWords(m, r0, r1, r2, r3, r4);
}

void ConvertRotToMatrix_Transpose(MATRIX *m, const SVec3 *rot)
{
	MATH_Matrix_InverseTransposeBody(m, -(s32)rot->x, -(s32)rot->z, -(s32)rot->y);
}
#else
// NOTE(aalhendi): Retail loads all signed angles before any destination store.
// Inverse order is Rz * Rx * Ry; raw zero angles skip optional compositions,
// while nonzero full turns still update the GTE and the fifth output word.
__asm__(".section .ConvertRotToMatrix_InverseTranspose,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent ConvertRotToMatrix_InverseTranspose\n"
        ".set\tnoreorder\n"
        ".globl ConvertRotToMatrix_InverseTranspose\n"
        "ConvertRotToMatrix_InverseTranspose:\n" MATH_MATRIX_SAVE_SCRATCH "lh $s3,0($a1)\n"
        "lh $s0,4($a1)\n"
        "lh $s4,2($a1)\n"
        ".Lmatrix_inverse_body:\n"
        "jal TRIG_AngleSinCos_r16r17r18_duplicate\n"
        "addiu $t6,$zero,0\n"
        "addiu $t7,$zero,4096\n"
        "sub $v1,$zero,$s1\n"
        "sll $v1,$v1,16\n"
        "or $t3,$v1,$s2\n"
        "sll $t4,$s1,16\n"
        "jal MATRIX_SET_r11r12r13r14r15\n"
        "addu $t5,$s2,$zero\n"
        "beq $s3,$zero,.Lmatrix_inverse_y\n"
        "addu $s0,$s3,$zero\n"
        "jal TRIG_AngleSinCos_r16r17r18_duplicate\n"
        "addiu $t3,$zero,4096\n"
        "addiu $t4,$zero,0\n"
        "sub $v1,$zero,$s1\n"
        "sll $v1,$v1,16\n"
        "or $t5,$v1,$s2\n"
        "sll $t6,$s1,16\n"
        "jal Unknown_8006c49c\n"
        "addu $t7,$s2,$zero\n"
        ".Lmatrix_inverse_y:\n"
        "beq $s4,$zero,.Lmatrix_inverse_store\n"
        "addu $s0,$s4,$zero\n"
        "jal TRIG_AngleSinCos_r16r17r18_duplicate\n"
        "addiu $t5,$zero,4096\n"
        "addu $t3,$s2,$zero\n"
        "addu $t4,$s1,$zero\n"
        "sub $v1,$zero,$s1\n"
        "andi $t6,$v1,0xffff\n"
        "jal Unknown_8006c49c\n"
        "addu $t7,$s2,$zero\n"
        ".Lmatrix_inverse_store:\n" MATH_MATRIX_STORE_AND_RETURN ".end ConvertRotToMatrix_InverseTranspose\n"
        ".set\treorder\n"
        ".text\n");

// NOTE(aalhendi): Forward order is Ry * Rx * Rz. Without a composition, the
// fifth stored word keeps the unsigned trig cosine even though GTE control 4
// sign-extends it; do not normalize the destination padding independently.
__asm__(".section .ConvertRotToMatrix,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent ConvertRotToMatrix\n"
        ".set\tnoreorder\n"
        ".globl ConvertRotToMatrix\n"
        "ConvertRotToMatrix:\n" MATH_MATRIX_SAVE_SCRATCH "lh $s3,0($a1)\n"
        "lh $s0,2($a1)\n"
        "lh $s4,4($a1)\n"
        "jal TRIG_AngleSinCos_r16r17r18_duplicate\n"
        "addiu $t5,$zero,4096\n"
        "addu $t3,$s2,$zero\n"
        "addu $t4,$s1,$zero\n"
        "sub $v1,$zero,$s1\n"
        "andi $t6,$v1,0xffff\n"
        "jal MATRIX_SET_r11r12r13r14r15\n"
        "addu $t7,$s2,$zero\n"
        "beq $s3,$zero,.Lmatrix_forward_z\n"
        "addu $s0,$s3,$zero\n"
        "jal TRIG_AngleSinCos_r16r17r18_duplicate\n"
        "addiu $t3,$zero,4096\n"
        "addiu $t4,$zero,0\n"
        "sub $v1,$zero,$s1\n"
        "sll $v1,$v1,16\n"
        "or $t5,$v1,$s2\n"
        "sll $t6,$s1,16\n"
        "jal Unknown_8006c49c\n"
        "addu $t7,$s2,$zero\n"
        ".Lmatrix_forward_z:\n"
        "beq $s4,$zero,.Lmatrix_forward_store\n"
        "addu $s0,$s4,$zero\n"
        "jal TRIG_AngleSinCos_r16r17r18_duplicate\n"
        "addiu $t6,$zero,0\n"
        "addiu $t7,$zero,4096\n"
        "sub $v1,$zero,$s1\n"
        "sll $v1,$v1,16\n"
        "or $t3,$v1,$s2\n"
        "sll $t4,$s1,16\n"
        "jal Unknown_8006c49c\n"
        "addu $t5,$s2,$zero\n"
        ".Lmatrix_forward_store:\n" MATH_MATRIX_STORE_AND_RETURN ".end ConvertRotToMatrix\n"
        ".set\treorder\n"
        ".text\n");

// NOTE(aalhendi): Transpose negates signed angles in 32 bits and enters the
// inverse body after its scratch saves and loads, reusing this entry's saves.
// The same-section label delta names that body without a fixed byte offset.
__asm__(".section .ConvertRotToMatrix_Transpose,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent ConvertRotToMatrix_Transpose\n"
        ".set\tnoreorder\n"
        ".globl ConvertRotToMatrix_Transpose\n"
        "ConvertRotToMatrix_Transpose:\n" MATH_MATRIX_SAVE_SCRATCH "lh $s3,0($a1)\n"
        "lh $s0,4($a1)\n"
        "lh $s4,2($a1)\n"
        "sub $s3,$zero,$s3\n"
        "sub $s0,$zero,$s0\n"
        "bgez $zero,ConvertRotToMatrix_InverseTranspose+(.Lmatrix_inverse_body-ConvertRotToMatrix_InverseTranspose)\n"
        "sub $s4,$zero,$s4\n"
        ".end ConvertRotToMatrix_Transpose\n"
        ".set\treorder\n"
        ".text\n");
#endif

// NOTE(aalhendi): The destination is five packed GTE rotation words, not a
// complete MATRIX. Cutscene tables store this compact form without translation.
#ifdef CTR_NATIVE
void MatrixRotate(void *dst, MATRIX *src, MATRIX *rot)
{
	u32 r0 = MATH_Matrix_ReadWord(src, 0x0);
	u32 r1 = MATH_Matrix_ReadWord(src, 0x4);
	u32 r2 = MATH_Matrix_ReadWord(src, 0x8);
	u32 r3 = MATH_Matrix_ReadWord(src, 0xc);
	u32 r4 = MATH_Matrix_ReadWord(src, 0x10);

	MATRIX_SET_r11r12r13r14r15(r0, r1, r2, r3, r4);

	r0 = MATH_Matrix_ReadWord(rot, 0x0);
	r1 = MATH_Matrix_ReadWord(rot, 0x4);
	r2 = MATH_Matrix_ReadWord(rot, 0x8);
	r3 = MATH_Matrix_ReadWord(rot, 0xc);
	r4 = MATH_Matrix_ReadWord(rot, 0x10);

	Unknown_8006c49c(&r0, &r1, &r2, &r3, &r4);
	MATH_Matrix_StoreWords(dst, r0, r1, r2, r3, r4);
}
#else
// NOTE(aalhendi): Retail saves s0-s4 and RA in reserved scratchpad slots, not
// on the C stack. Both register-ABI callees preserve at and the argument
// pointers; loading both inputs before the stores permits destination aliases.
__asm__(".section .MatrixRotate,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent MatrixRotate\n"
        ".set\tnoreorder\n"
        ".globl MatrixRotate\n"
        "MatrixRotate:\n" MATH_MATRIX_SAVE_SCRATCH "lw $t3,0($a1)\n"
        "lw $t4,4($a1)\n"
        "lw $t5,8($a1)\n"
        "lw $t6,12($a1)\n"
        "jal MATRIX_SET_r11r12r13r14r15\n"
        "lw $t7,16($a1)\n"
        "lw $t3,0($a2)\n"
        "lw $t4,4($a2)\n"
        "lw $t5,8($a2)\n"
        "lw $t6,12($a2)\n"
        "jal Unknown_8006c49c\n"
        "lw $t7,16($a2)\n" MATH_MATRIX_STORE_AND_RETURN ".end MatrixRotate\n"
        ".set\treorder\n"
        ".text\n");
#endif

#undef MATH_MATRIX_SAVE_SCRATCH
#undef MATH_MATRIX_STORE_AND_RETURN

#ifdef CTR_NATIVE
s32 SquareRoot0_stub(s32 value)
{
	u32 shifted;
	s32 leading;
	s32 bit;
	u32 remainder;
	u32 root;

	MTC2((u32)value, 30);
	if (value == 0)
	{
		return 0;
	}

	shifted = (u32)value;
	leading = MFC2(31) & 0x1e;
	shifted <<= leading;

	bit = leading ^ 0x1e;
	remainder = 0;
	root = 0;

	do
	{
		u32 trial;
		u32 nextRemainder;

		remainder |= shifted >> 0x1e;
		trial = (root << 2) + 1;
		root <<= 1;

		nextRemainder = remainder - trial;
		shifted <<= 2;
		if ((s32)nextRemainder >= 0)
		{
			root++;
			remainder = nextRemainder << 2;
		}
		else
		{
			remainder <<= 2;
		}

		bit -= 2;
	} while (bit >= 0);

	return (s32)root;
}
#else
// NOTE(aalhendi): Retail carries the result in v0 and uses the GTE leading-zero
// count in t1. GCC's C loop adds a second live result register and a branch,
// so keep this small PSX routine in its original register/branch shape.
__asm__(".section .SquareRoot0_stub,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent SquareRoot0_stub\n"
        ".set\tnoreorder\n"
        ".globl SquareRoot0_stub\n"
        "SquareRoot0_stub:\n"
        "mtc2 $a0,$30\n"
        "beq $a0,$zero,.Lsqrt_zero\n"
        "addiu $v0,$zero,0\n"
        "mfc2 $t1,$31\n"
        "addiu $v1,$zero,0\n"
        "andi $t1,$t1,0x1e\n"
        "sllv $a0,$a0,$t1\n"
        "bgez $zero,.Lsqrt_test\n"
        "xori $t1,$t1,0x1e\n"
        ".Lsqrt_loop:\n"
        "srl $t0,$a0,0x1e\n"
        "or $v1,$v1,$t0\n"
        "sll $t0,$v0,2\n"
        "addiu $t0,$t0,1\n"
        "sll $v0,$v0,1\n"
        "subu $t0,$v1,$t0\n"
        "bltz $t0,.Lsqrt_subtract_failed\n"
        "sll $a0,$a0,2\n"
        "addiu $v0,$v0,1\n"
        "sll $v1,$t0,2\n"
        ".Lsqrt_test:\n"
        "bgez $t1,.Lsqrt_loop\n"
        "addiu $t1,$t1,-2\n"
        "jr $ra\n"
        ".Lsqrt_subtract_failed:\n"
        "sll $v1,$v1,2\n"
        "bgez $t1,.Lsqrt_loop\n"
        "addiu $t1,$t1,-2\n"
        ".Lsqrt_zero:\n"
        "jr $ra\n"
        "nop\n"
        ".end SquareRoot0_stub\n"
        ".set\treorder\n"
        ".text\n");
#endif

// NOTE(aalhendi): The payload is three aligned words, including MATRIX's
// translation arrays. Local t0-t5 bindings retain the two GTE passes' retail
// schedule without reserving registers outside this function.
VECTOR *ApplyMatrixLV_stub(VECTOR *input, VECTOR *output)
{
	register u32 x CTR_PSX_REGISTER("$8") = CTR_ReadU32LE((const u8 *)input + 0);
	register u32 y CTR_PSX_REGISTER("$9") = CTR_ReadU32LE((const u8 *)input + 4);
	register u32 z CTR_PSX_REGISTER("$10") = CTR_ReadU32LE((const u8 *)input + 8);
	register u32 highX CTR_PSX_REGISTER("$11");
	register u32 highY CTR_PSX_REGISTER("$12");
	register u32 highZ CTR_PSX_REGISTER("$13");
	VECTOR *destination = output;

	highX = (u32)((s32)x >> 0xf);
	highY = (u32)((s32)y >> 0xf);
	highZ = (u32)((s32)z >> 0xf);
	MTC2(highX, 9);
	MTC2(highY, 10);
	MTC2(highZ, 11);

	x &= 0x7fffu;
	gte_rtir_sf0_b();
	y &= 0x7fffu;
	z &= 0x7fffu;

	highX = MFC2(25);
	highY = MFC2(26);
	highZ = MFC2(27);

	MTC2(x, 9);
	MTC2(y, 10);
	MTC2(z, 11);

	highX <<= 3;
	gte_rtir_b();
	highY <<= 3;
	highZ <<= 3;

	x = MFC2(25);
	y = MFC2(26);
	z = MFC2(27);
	x += highX;
	y += highY;
	z += highZ;
	CTR_WriteU32AlignedLE((u8 *)destination + 0, x);
	CTR_WriteU32AlignedLE((u8 *)destination + 4, y);
	CTR_WriteU32AlignedLE((u8 *)destination + 8, z);
	// NOTE(aalhendi): Delay the return copy so GCC can fill JR's slot with it.
	CTR_PSX_CLOBBER("$2");

	return destination;
}

#ifdef CTR_NATIVE
VECTOR *Unknown_8006c6c8(VECTOR *input, VECTOR *output, MATRIX *matrix)
{
	MATRIX_SET_r11r12r13r14r15(MATH_Matrix_ReadWord(matrix, 0x0), MATH_Matrix_ReadWord(matrix, 0x4), MATH_Matrix_ReadWord(matrix, 0x8),
	                           MATH_Matrix_ReadWord(matrix, 0xc), MATH_Matrix_ReadWord(matrix, 0x10));

	return ApplyMatrixLV_stub(input, output);
}
#else
// NOTE(aalhendi): Retail's matrix-loading entry has no branch or return; it
// falls through into ApplyMatrixLV_stub. The final EXE link must keep these
// two sections adjacent in this order.
__asm__(".section .Unknown_8006c6c8,\"ax\",@progbits\n"
        ".align 2\n"
        ".ent Unknown_8006c6c8\n"
        ".set\tnoreorder\n"
        ".globl Unknown_8006c6c8\n"
        "Unknown_8006c6c8:\n"
        "lw $t0,0($a2)\n"
        "lw $t1,4($a2)\n"
        "lw $t2,8($a2)\n"
        "lw $t3,12($a2)\n"
        "lw $t4,16($a2)\n"
        "ctc2 $t0,$0\n"
        "ctc2 $t1,$1\n"
        "ctc2 $t2,$2\n"
        "ctc2 $t3,$3\n"
        "ctc2 $t4,$4\n"
        ".end Unknown_8006c6c8\n"
        ".set\treorder\n"
        ".text\n");

#endif
