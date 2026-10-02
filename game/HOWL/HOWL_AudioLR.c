#include <common.h>
#include <ctr_gte_transfer.h>

void GTE_AudioLR_Inst(MATRIX *matrix, s32 *vec)
{
	SVec4 input;

	input.x = (s16)vec[0];
	input.y = (s16)vec[1];
	input.z = (s16)vec[2];

	SetRotMatrix(matrix);
	// NOTE(aalhendi): Native transfers read only XYZ; retail ignores the upper VZ halfword.
	CTR_GteLoadPositionV0(&input);
	gte_rtv0();
	CTR_PSX_STORE_COP2_WORD(&vec[0], 25);
	CTR_PSX_STORE_COP2_WORD(&vec[1], 26);
	CTR_PSX_STORE_COP2_WORD(&vec[2], 27);
}

void GTE_AudioLR_Driver(MATRIX *matrix, struct Driver *driver, s32 *out)
{
	SVec4 input;

	input.x = (s16)(CTR_MipsSra(driver->posCurr.x, 8) - (u16)matrix->t[0]);
	input.y = (s16)(CTR_MipsSra(driver->posCurr.y, 8) - (u16)matrix->t[1]);
	input.z = (s16)(CTR_MipsSra(driver->posCurr.z, 8) - (u16)matrix->t[2]);

	SetRotMatrix(matrix);
	CTR_GteLoadPositionV0(&input);
	gte_rtv0();
	CTR_PSX_STORE_COP2_WORD(out, 25);
	CTR_PSX_STORE_COP2_WORD(out + 1, 26);
	CTR_PSX_STORE_COP2_WORD(out + 2, 27);
}

int GTE_GetSquaredLength(s32 *vec)
{
	s32 x2;
	s32 y2;
	s32 z2;

	CTR_GteLoadDataWord(&vec[0], 9);
	CTR_GteLoadDataWord(&vec[1], 10);
	CTR_GteLoadDataWord(&vec[2], 11);
	CTR_PSX_GTE_PIPELINE_DELAY();
	gte_sqr0();

	x2 = MFC2(25);
	y2 = MFC2(26);
	z2 = MFC2(27);

	return CTR_MipsAddLo(CTR_MipsAddLo(x2, y2), z2);
}
