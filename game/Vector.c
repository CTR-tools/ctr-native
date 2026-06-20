#include <common.h>


static void Vector_SpecLightSpin2D_RotMatrixMul(MATRIX *matrix, const SVec3 *input, VECTOR *mac, SVec3 *output)
{
	gte_SetRotMatrix(matrix);
	gte_ldv0((SVECTOR *)input);
	gte_rtv0();
	gte_stlvnl(mac);

	output->x = (s16)mac->vx;
	output->y = (s16)mac->vy;
	output->z = (s16)mac->vz;
}

void Vector_SpecLightSpin2D(struct Instance *inst, s16 *rot, s16 *lightDir)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800572d0-0x8005741c.
	MATRIX rotMatrix;
	VECTOR lightMac;
	VECTOR viewMac;
	SVec3 light = {.x = lightDir[0], .y = lightDir[1], .z = lightDir[2]};
	SVec3 lightLocal;
	SVec3 view = {.x = 0, .y = 0, .z = 0x1000};
	SVec3 viewLocal;
	SVec3 halfVector;
	struct GameTracker *gGT = sdata->gGT;
	struct InstDrawPerPlayer *idpp = INST_GETIDPP(inst);

	ConvertRotToMatrix_Transpose(&rotMatrix, rot);
	Vector_SpecLightSpin2D_RotMatrixMul(&rotMatrix, &light, &lightMac, &lightLocal);

	inst->unk53 = (char)lightMac.vx;
	inst->reflectionRGBA = (u32)lightMac.vz;

	Vector_SpecLightSpin2D_RotMatrixMul(&rotMatrix, &view, &viewMac, &viewLocal);

	halfVector.x = lightLocal.x + viewLocal.x;
	halfVector.y = lightLocal.y + viewLocal.y;
	halfVector.z = lightLocal.z + viewLocal.z;
	MATH_VectorNormalize(&halfVector);

	for (int i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		idpp[i].specLight[0] = halfVector.x;
		idpp[i].specLight[1] = halfVector.y;
		idpp[i].specLight[2] = halfVector.z;
	}
}


static void Vector_SpecLightSpin3D_LightMatrixMul(MATRIX *matrix, const SVec3 *input, SVec3 *output)
{
	VECTOR mac;

	gte_SetLightMatrix(matrix);
	gte_ldv0((SVECTOR *)input);
	gte_llv0();
	gte_stlvnl(&mac);

	output->x = (s16)mac.vx;
	output->y = (s16)mac.vy;
	output->z = (s16)mac.vz;
}

void Vector_SpecLightSpin3D(struct Instance *inst, s16 *rot, s16 *lightDir)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005741c-0x800576b8.
	MATRIX rotMatrix;
	SVec3 light = {.x = lightDir[0], .y = lightDir[1], .z = lightDir[2]};
	struct GameTracker *gGT = sdata->gGT;
	struct InstDrawPerPlayer *idpp = INST_GETIDPP(inst);

	ConvertRotToMatrix_Transpose(&rotMatrix, rot);

	for (int i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		struct PushBuffer *pb = &gGT->pushBuffer[i];
		SVec3 lightCamera;
		SVec3 lightLocal;
		SVec3 view;
		SVec3 viewLocal;
		SVec3 halfVector;

		Vector_SpecLightSpin3D_LightMatrixMul(&pb->matrix_Camera, &light, &lightCamera);
		Vector_SpecLightSpin3D_LightMatrixMul(&rotMatrix, &lightCamera, &lightLocal);

		inst->unk53 = (char)lightLocal.x;
		inst->reflectionRGBA = (u16)lightLocal.z;

		view.x = inst->matrix.t[0] - pb->pos[0];
		view.y = inst->matrix.t[1] - pb->pos[1];
		view.z = inst->matrix.t[2] - pb->pos[2];
		MATH_VectorNormalize(&view);
		Vector_SpecLightSpin3D_LightMatrixMul(&rotMatrix, &view, &viewLocal);

		halfVector.x = lightLocal.x + viewLocal.x;
		halfVector.y = lightLocal.y + viewLocal.y;
		halfVector.z = lightLocal.z + viewLocal.z;
		MATH_VectorNormalize(&halfVector);

		idpp[i].specLight[0] = halfVector.x;
		idpp[i].specLight[1] = halfVector.y;
		idpp[i].specLight[2] = halfVector.z;
	}
}


static void Vector_LightMatrixMul(MATRIX *matrix, const SVec3 *input, SVec3 *output)
{
	VECTOR mac;

	gte_SetLightMatrix(matrix);
	gte_ldv0((SVECTOR *)input);
	gte_llv0();
	gte_stlvnl(&mac);

	output->x = (s16)mac.vx;
	output->y = (s16)mac.vy;
	output->z = (s16)mac.vz;
}

void Vector_SpecLightNoSpin3D(struct Instance *inst, s16 *rot, s16 *lightDir)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800576b8-0x80057884.
	MATRIX lightMatrix;
	SVec3 light = {.x = lightDir[0], .y = lightDir[1], .z = lightDir[2]};
	SVec3 lightLocal;
	struct GameTracker *gGT = sdata->gGT;
	struct InstDrawPerPlayer *idpp = INST_GETIDPP(inst);

	ConvertRotToMatrix(&lightMatrix, rot);
	Vector_LightMatrixMul(&lightMatrix, &light, &lightLocal);

	inst->unk53 = (char)lightLocal.x;
	inst->reflectionRGBA = (u16)lightLocal.z;

	for (int i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		struct PushBuffer *pb = &gGT->pushBuffer[i];
		SVec3 viewLocal;
		SVec3 halfVector;
		SVec3 view = {
		    .x = inst->matrix.t[0] - pb->pos[0],
		    .y = inst->matrix.t[1] - pb->pos[1],
		    .z = inst->matrix.t[2] - pb->pos[2],
		};

		MATH_VectorNormalize(&view);
		Vector_LightMatrixMul(&lightMatrix, &view, &viewLocal);

		halfVector.x = lightLocal.x + viewLocal.x;
		halfVector.y = lightLocal.y + viewLocal.y;
		halfVector.z = lightLocal.z + viewLocal.z;
		MATH_VectorNormalize(&halfVector);

		idpp[i].specLight[0] = halfVector.x;
		idpp[i].specLight[1] = halfVector.y;
		idpp[i].specLight[2] = halfVector.z;
	}
}


static s16 Vector_BakeMatrixTable_Div4TowardZero(s32 value)
{
	if (value < 0)
		value += 3;

	return (s16)(value >> 2);
}

static void Vector_BakeMatrixTable_PrepareBlastedFrames(void)
{
	char *entries = (char *)data.bakedGteMath[6].physEntry;
	int count = data.bakedGteMath[6].numEntries;

	if ((entries == NULL) || (count <= 0))
		return;

	for (int i = 0; i < count; i++)
	{
		char *entry = entries + (i * 0x20);
		s32 angle2000 = (i << 0xd) / count;
		s32 angle3000 = (i * 0x3000) / count;
		s32 sin2000 = MATH_Sin((u32)angle2000);

		*(s16 *)(void *)(entry + 0xc) = (s16)angle2000;
		*(s16 *)(void *)(entry + 0x8) = (s16)(-MATH_Sin((u32)angle3000) / 7);
		*(s16 *)(void *)(entry + 0x10) = Vector_BakeMatrixTable_Div4TowardZero(sin2000) + 0x1000;
		*(s16 *)(void *)(entry + 0x14) = (s16)(((sin2000 * 6) / 0x28) + 0x1000);
	}
}

static void Vector_BakeMatrixTable_BakeRotScaleEntries(void)
{
	MATRIX rot;
	MATRIX scale = {0};

	for (int i = 0; i < 0x14; i++)
	{
		char *entries = (char *)data.bakedGteMath[i].physEntry;
		int count = data.bakedGteMath[i].numEntries;

		if ((entries == NULL) || (count <= 0))
			continue;

		for (int j = 0; j < count; j++)
		{
			char *entry = entries + (j * 0x20);

			ConvertRotToMatrix(&rot, (s16 *)(void *)(entry + 8));

			scale.m[0][0] = *(s16 *)(void *)(entry + 0x10);
			scale.m[1][1] = *(s16 *)(void *)(entry + 0x12);
			scale.m[2][2] = *(s16 *)(void *)(entry + 0x14);

			MatrixRotate((MATRIX *)(void *)(entry + 8), &scale, &rot);
		}
	}
}

static void Vector_BakeMatrixTable_BakeBlastedOffsets(void)
{
	char *entries = (char *)data.bakedGteMath[6].physEntry;
	int count = data.bakedGteMath[6].numEntries;

	if ((entries == NULL) || (count <= 0))
		return;

	for (int i = 0; i < count; i++)
	{
		char *entry = entries + (i * 0x20);
		MATRIX *matrix = (MATRIX *)(void *)(entry + 8);
		s32 x = (matrix->m[0][1] * -0x2000) >> 12;
		s32 y = ((matrix->m[1][1] * -0x2000) >> 12) + 0x2000;
		s32 z = (matrix->m[2][1] * -0x2000) >> 12;

		*(s16 *)(void *)(entry + 0) = (s16)x;
		*(s16 *)(void *)(entry + 2) = (s16)y;
		*(s16 *)(void *)(entry + 4) = (s16)z;
	}
}

void Vector_BakeMatrixTable(void)
{
	// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80057884-0x80057c44.
	// Retail bakes authored rot/scale vehicle-animation entries in-place before
	// VehPhysForce_TranslateMatrix consumes entry+8 as a MATRIX.
	if (sdata->matrixTableBaked != 0)
		return;

	sdata->matrixTableBaked = 1;

	Vector_BakeMatrixTable_PrepareBlastedFrames();
	Vector_BakeMatrixTable_BakeRotScaleEntries();
	Vector_BakeMatrixTable_BakeBlastedOffsets();
}
