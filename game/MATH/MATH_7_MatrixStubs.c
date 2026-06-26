#include <common.h>

static inline u32 MATH_Matrix_ReadWord(const void *ptr, s32 offset)
{
	return *(u32 *)(void *)((u8 *)ptr + offset);
}

static inline void MATH_Matrix_WriteWord(void *ptr, s32 offset, u32 value)
{
	*(u32 *)(void *)((u8 *)ptr + offset) = value;
}

static inline u32 MATH_Matrix_NegLowWord(u32 value)
{
	return (0u - value) & 0xffff;
}

static inline u32 MATH_Matrix_NegHighWord(u32 value)
{
	return MATH_Matrix_NegLowWord(value) << 0x10;
}

static void MATH_Matrix_TrigSinCos(u32 angle, u32 *sinOut, u32 *cosOut)
{
	u32 trig = *(u32 *)&data.trigApprox[angle & 0x3ff];
	u32 quadrant = angle & 0xc00;

	if (quadrant == 0)
	{
		*sinOut = trig & 0xffff;
		*cosOut = trig >> 0x10;
	}
	else if (quadrant == 0x400)
	{
		*sinOut = trig >> 0x10;
		*cosOut = MATH_Matrix_NegLowWord(trig);
	}
	else if (quadrant == 0x800)
	{
		*sinOut = MATH_Matrix_NegLowWord(trig);
		*cosOut = MATH_Matrix_NegLowWord(trig >> 0x10);
	}
	else
	{
		*sinOut = MATH_Matrix_NegLowWord(trig >> 0x10);
		*cosOut = trig & 0xffff;
	}
}

static void MATH_Matrix_LoadRotWords(u32 r0, u32 r1, u32 r2, u32 r3, u32 r4)
{
	CTC2(r0, 0);
	CTC2(r1, 1);
	CTC2(r2, 2);
	CTC2(r3, 3);
	CTC2(r4, 4);
}

void MATRIX_SET_r11r12r13r14r15(u32 r0, u32 r1, u32 r2, u32 r3, u32 r4)
{
	// NOTE(aalhendi): Retail inputs are t3/t4/t5/t6/t7 and writes GTE regs 0-4.
	MATH_Matrix_LoadRotWords(r0, r1, r2, r3, r4);
}

void Unknown_8006c600(u32 r0, u32 r1, u32 r2, u32 r3, u32 r4)
{
	// NOTE(aalhendi): Retail inputs are t3/t4/t5/t6/t7 and writes GTE regs 8-12.
	CTC2(r0, 8);
	CTC2(r1, 9);
	CTC2(r2, 10);
	CTC2(r3, 11);
	CTC2(r4, 12);
}

static void MATH_Matrix_MulRotWords(u32 *r0, u32 *r1, u32 *r2, u32 *r3, u32 *r4)
{
	u32 t3 = *r0;
	u32 t4 = *r1;
	u32 t5 = *r2;
	u32 t6 = *r3;
	u32 t7 = *r4;

	u32 v0 = (t3 & 0xffff) | (t4 & 0xffff0000);
	MTC2(v0, 0);
	MTC2(t6, 1);

	v0 = (t3 >> 0x10) | (t5 << 0x10);
	gte_rtv0_b();
	MTC2(v0, 2);
	MTC2(t6 >> 0x10, 3);

	v0 = (t4 & 0xffff) | (t5 & 0xffff0000);
	t3 = MFC2(9);
	t4 = MFC2(10);
	t6 = MFC2(11);
	gte_rtv1_b();
	MTC2(v0, 4);
	MTC2(t7, 5);

	t3 &= 0xffff;
	t4 <<= 0x10;
	t6 &= 0xffff;

	v0 = MFC2(9);
	t5 = MFC2(10);
	u32 nextT6 = MFC2(11);
	gte_rtv2_b();

	t3 |= v0 << 0x10;
	t5 &= 0xffff;
	t6 |= nextT6 << 0x10;

	v0 = MFC2(9) & 0xffff;
	u32 nextT5 = MFC2(10);
	t7 = MFC2(11);

	t4 |= v0;
	t5 |= nextT5 << 0x10;

	*r0 = t3;
	*r1 = t4;
	*r2 = t5;
	*r3 = t6;
	*r4 = t7;

	MATH_Matrix_LoadRotWords(t3, t4, t5, t6, t7);
}

void Unknown_8006c49c(u32 *r0, u32 *r1, u32 *r2, u32 *r3, u32 *r4)
{
	// NOTE(aalhendi): Retail transforms and reloads caller-owned t3/t4/t5/t6/t7.
	MATH_Matrix_MulRotWords(r0, r1, r2, r3, r4);
}

void Unknown_8006c558(u32 *r0, u32 *r1, u32 *r2, u32 *r3, u32 *r4)
{
	u32 t3 = *r0;
	u32 t4 = *r1;
	u32 t5 = *r2;
	u32 t6 = *r3;
	u32 t7 = *r4;

	u32 v0 = (t3 & 0xffff) | (t4 & 0xffff0000);
	MTC2(v0, 0);
	MTC2(t6, 1);

	v0 = (t3 >> 0x10) | (t5 << 0x10);
	gte_llv0_b();
	MTC2(v0, 2);
	MTC2(t6 >> 0x10, 3);

	v0 = (t4 & 0xffff) | (t5 & 0xffff0000);
	t3 = MFC2(9);
	t4 = MFC2(10);
	t6 = MFC2(11);
	gte_llv1_b();
	MTC2(v0, 4);
	MTC2(t7, 5);

	t3 &= 0xffff;
	t4 <<= 0x10;
	t6 &= 0xffff;

	v0 = MFC2(9);
	t5 = MFC2(10);
	u32 nextT6 = MFC2(11);
	gte_llv2_b();

	t3 |= v0 << 0x10;
	t5 &= 0xffff;
	t6 |= nextT6 << 0x10;

	v0 = MFC2(9) & 0xffff;
	u32 nextT5 = MFC2(10);
	t7 = MFC2(11);

	t4 |= v0;
	t5 |= nextT5 << 0x10;

	*r0 = t3;
	*r1 = t4;
	*r2 = t5;
	*r3 = t6;
	*r4 = t7;
}

static void MATH_Matrix_StoreWords(MATRIX *m, u32 r0, u32 r1, u32 r2, u32 r3, u32 r4)
{
	MATH_Matrix_WriteWord(m, 0x0, r0);
	MATH_Matrix_WriteWord(m, 0x4, r1);
	MATH_Matrix_WriteWord(m, 0x8, r2);
	MATH_Matrix_WriteWord(m, 0xc, r3);
	MATH_Matrix_WriteWord(m, 0x10, r4);
}

static void MATH_Matrix_MulIfNonZero(s32 angle, u32 *r0, u32 *r1, u32 *r2, u32 *r3, u32 *r4, s32 axis)
{
	u32 sine;
	u32 cosine;

	if (angle == 0)
	{
		return;
	}

	MATH_Matrix_TrigSinCos(angle, &sine, &cosine);

	if (axis == 0)
	{
		*r0 = 0x1000;
		*r1 = 0;
		*r2 = MATH_Matrix_NegHighWord(sine) | cosine;
		*r3 = sine << 0x10;
		*r4 = cosine;
	}
	else if (axis == 1)
	{
		*r0 = cosine;
		*r1 = sine;
		*r2 = 0x1000;
		*r3 = MATH_Matrix_NegLowWord(sine);
		*r4 = cosine;
	}
	else
	{
		*r0 = MATH_Matrix_NegHighWord(sine) | cosine;
		*r1 = sine << 0x10;
		*r2 = cosine;
		*r3 = 0;
		*r4 = 0x1000;
	}

	MATH_Matrix_MulRotWords(r0, r1, r2, r3, r4);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006c124-0x8006c1d0.
void ConvertRotToMatrix_InverseTranspose_NoRotY(MATRIX *m, const SVec3 *rot)
{
	u32 sine;
	u32 cosine;
	u32 r0;
	u32 r1;
	u32 r2;
	u32 r3 = 0;
	u32 r4 = 0x1000;

	MATH_Matrix_TrigSinCos((s32)rot->z, &sine, &cosine);
	r0 = MATH_Matrix_NegHighWord(sine) | cosine;
	r1 = sine << 0x10;
	r2 = cosine;
	MATH_Matrix_LoadRotWords(r0, r1, r2, r3, r4);

	MATH_Matrix_MulIfNonZero((s32)rot->x, &r0, &r1, &r2, &r3, &r4, 0);
	MATH_Matrix_StoreWords(m, r0, r1, r2, r3, r4);
}

static void MATH_Matrix_InverseTransposeBody(MATRIX *m, s32 rotX, s32 rotZ, s32 rotY)
{
	u32 sine;
	u32 cosine;
	u32 r0;
	u32 r1;
	u32 r2;
	u32 r3 = 0;
	u32 r4 = 0x1000;

	MATH_Matrix_TrigSinCos(rotZ, &sine, &cosine);
	r0 = MATH_Matrix_NegHighWord(sine) | cosine;
	r1 = sine << 0x10;
	r2 = cosine;
	MATH_Matrix_LoadRotWords(r0, r1, r2, r3, r4);

	MATH_Matrix_MulIfNonZero(rotX, &r0, &r1, &r2, &r3, &r4, 0);
	MATH_Matrix_MulIfNonZero(rotY, &r0, &r1, &r2, &r3, &r4, 1);
	MATH_Matrix_StoreWords(m, r0, r1, r2, r3, r4);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006c1d0-0x8006c2a4.
void ConvertRotToMatrix_InverseTranspose(MATRIX *m, const SVec3 *rot)
{
	MATH_Matrix_InverseTransposeBody(m, (s32)rot->x, (s32)rot->z, (s32)rot->y);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006c2a4-0x8006c378.
void ConvertRotToMatrix(MATRIX *m, const SVec3 *rot)
{
	u32 sine;
	u32 cosine;
	u32 r0;
	u32 r1;
	u32 r2 = 0x1000;
	u32 r3;
	u32 r4;

	MATH_Matrix_TrigSinCos((s32)rot->y, &sine, &cosine);
	r0 = cosine;
	r1 = sine;
	r3 = MATH_Matrix_NegLowWord(sine);
	r4 = cosine;
	MATH_Matrix_LoadRotWords(r0, r1, r2, r3, r4);

	MATH_Matrix_MulIfNonZero((s32)rot->x, &r0, &r1, &r2, &r3, &r4, 0);
	MATH_Matrix_MulIfNonZero((s32)rot->z, &r0, &r1, &r2, &r3, &r4, 2);
	MATH_Matrix_StoreWords(m, r0, r1, r2, r3, r4);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006c378-0x8006c3b0.
void ConvertRotToMatrix_Transpose(MATRIX *m, const SVec3 *rot)
{
	MATH_Matrix_InverseTransposeBody(m, -(s32)rot->x, -(s32)rot->z, -(s32)rot->y);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006c3b0-0x8006c430.
void MatrixRotate(MATRIX *dst, MATRIX *src, MATRIX *rot)
{
	u32 r0 = MATH_Matrix_ReadWord(src, 0x0);
	u32 r1 = MATH_Matrix_ReadWord(src, 0x4);
	u32 r2 = MATH_Matrix_ReadWord(src, 0x8);
	u32 r3 = MATH_Matrix_ReadWord(src, 0xc);
	u32 r4 = MATH_Matrix_ReadWord(src, 0x10);

	MATH_Matrix_LoadRotWords(r0, r1, r2, r3, r4);

	r0 = MATH_Matrix_ReadWord(rot, 0x0);
	r1 = MATH_Matrix_ReadWord(rot, 0x4);
	r2 = MATH_Matrix_ReadWord(rot, 0x8);
	r3 = MATH_Matrix_ReadWord(rot, 0xc);
	r4 = MATH_Matrix_ReadWord(rot, 0x10);

	MATH_Matrix_MulRotWords(&r0, &r1, &r2, &r3, &r4);
	MATH_Matrix_StoreWords(dst, r0, r1, r2, r3, r4);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006c618-0x8006c684.
s32 SquareRoot0_stub(s32 value)
{
	MTC2((u32)value, 30);
	if (value == 0)
	{
		return 0;
	}

	u32 shifted = (u32)value;
	s32 leading = MFC2(31) & 0x1e;
	shifted <<= leading;

	s32 bit = leading ^ 0x1e;
	u32 remainder = 0;
	u32 root = 0;

	do
	{
		remainder |= shifted >> 0x1e;
		u32 trial = (root << 2) + 1;
		root <<= 1;

		u32 nextRemainder = remainder - trial;
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

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006c6f0-0x8006c778.
VECTOR *ApplyMatrixLV_stub(VECTOR *input, VECTOR *output)
{
	u32 x = (u32)input->vx;
	u32 y = (u32)input->vy;
	u32 z = (u32)input->vz;

	MTC2((u32)((s32)x >> 0xf), 9);
	MTC2((u32)((s32)y >> 0xf), 10);
	MTC2((u32)((s32)z >> 0xf), 11);

	x &= 0x7fff;
	gte_rtir_sf0_b();
	y &= 0x7fff;
	z &= 0x7fff;

	u32 highX = MFC2(25);
	u32 highY = MFC2(26);
	u32 highZ = MFC2(27);

	MTC2(x, 9);
	MTC2(y, 10);
	MTC2(z, 11);

	highX <<= 3;
	gte_rtir_b();
	highY <<= 3;
	highZ <<= 3;

	output->vx = (s32)(MFC2(25) + highX);
	output->vy = (s32)(MFC2(26) + highY);
	output->vz = (s32)(MFC2(27) + highZ);

	return output;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8006c6c8-0x8006c6f0.
VECTOR *Unknown_8006c6c8(VECTOR *input, VECTOR *output, MATRIX *matrix)
{
	MATH_Matrix_LoadRotWords(MATH_Matrix_ReadWord(matrix, 0x0), MATH_Matrix_ReadWord(matrix, 0x4), MATH_Matrix_ReadWord(matrix, 0x8),
	                         MATH_Matrix_ReadWord(matrix, 0xc), MATH_Matrix_ReadWord(matrix, 0x10));

	return ApplyMatrixLV_stub(input, output);
}
