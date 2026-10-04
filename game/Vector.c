#include <common.h>
#include <ctr_gte_transfer.h>


void Vector_SpecLightSpin2D(struct Instance *inst, const SVec3 *rot, const SVec3 *lightDir)
{
	MATRIX rotMatrix;
	VECTOR lightMac;
	VECTOR viewMac;
	SVec3 halfVector;
	struct GameTracker *gGT;
	s32 i;

	// NOTE(aalhendi): Transpose leaves ROT loaded; the view scratch then becomes the half-vector.
	ConvertRotToMatrix_Transpose(&rotMatrix, rot);
	CTR_GteLoadSVec3V0(lightDir);
	CTR_GteLoadDelay();
	gte_rtv0();
	CTR_GteStoreMAC(&lightMac.vx);

	inst->specLightX = (s8)lightMac.vx;
	inst->reflectionRGBA = (u32)lightMac.vz;

	halfVector.x = 0;
	halfVector.y = 0;
	halfVector.z = 0x1000;
	CTR_GteLoadSVec3V0(&halfVector);
	CTR_GteLoadDelay();
	gte_rtv0();
	CTR_GteStoreMAC(&viewMac.vx);

	halfVector.x = (s16)lightMac.vx + (s16)viewMac.vx;
	halfVector.y = (s16)lightMac.vy + (s16)viewMac.vy;
	halfVector.z = (s16)lightMac.vz + (s16)viewMac.vz;
	MATH_VectorNormalize(&halfVector);

	gGT = GAME_TRACKER;
	for (i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		inst->idpp[i].halfVector.x = halfVector.x;
		inst->idpp[i].halfVector.y = halfVector.y;
		inst->idpp[i].halfVector.z = halfVector.z;
	}
}

void Vector_SpecLightSpin3D(struct Instance *inst, const SVec3 *rot, const SVec3 *lightDir)
{
	MATRIX rotMatrix;
	VECTOR lightMac;
	VECTOR viewMac;
	SVec3 halfVector;
	SVec3 lightCamera;
	s32 i;

	ConvertRotToMatrix_Transpose(&rotMatrix, rot);
	for (i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
	{
		// NOTE(aalhendi): Retail advances camera positions by one PushBuffer stride,
		// but the LIGHT matrix address by two, including the later tracker-byte windows.
		u8 *pbBytes = (u8 *)GAME_TRACKER + OFFSETOF(struct GameTracker, pushBuffer) + i * sizeof(struct PushBuffer);
		struct PushBuffer *pb = (struct PushBuffer *)pbBytes;
		MATRIX *cameraMatrix = (MATRIX *)(pbBytes + i * sizeof(struct PushBuffer) + OFFSETOF(struct PushBuffer, matrix_Camera));

		CTR_GteLoadLightMatrix(cameraMatrix);
		CTR_GteLoadSVec3V0(lightDir);
		CTR_GteLoadDelay();
		gte_llv0();
		CTR_GteStoreMAC(&lightMac.vx);

		lightCamera.x = (s16)lightMac.vx;
		lightCamera.y = (s16)lightMac.vy;
		lightCamera.z = (s16)lightMac.vz;
		CTR_GteLoadLightMatrix(&rotMatrix);
		CTR_GteLoadSVec3V0(&lightCamera);
		CTR_GteLoadDelay();
		gte_llv0();
		CTR_GteStoreMAC(&lightMac.vx);

		inst->specLightX = (s8)lightMac.vx;
		inst->reflectionRGBA = (u16)lightMac.vz;

		halfVector.x = (s16)inst->matrix.t[0] - pb->pos.x;
		halfVector.y = (s16)inst->matrix.t[1] - pb->pos.y;
		halfVector.z = (s16)inst->matrix.t[2] - pb->pos.z;
		MATH_VectorNormalize(&halfVector);
		CTR_GteLoadLightMatrix(&rotMatrix);
		CTR_GteLoadSVec3V0(&halfVector);
		CTR_GteLoadDelay();
		gte_llv0();
		CTR_GteStoreMAC(&viewMac.vx);

		halfVector.x = (s16)lightMac.vx + (s16)viewMac.vx;
		halfVector.y = (s16)lightMac.vy + (s16)viewMac.vy;
		halfVector.z = (s16)lightMac.vz + (s16)viewMac.vz;
		MATH_VectorNormalize(&halfVector);

		inst->idpp[i].halfVector.x = halfVector.x;
		inst->idpp[i].halfVector.y = halfVector.y;
		inst->idpp[i].halfVector.z = halfVector.z;
	}
}

void Vector_SpecLightNoSpin3D(struct Instance *inst, const SVec3 *rot, const SVec3 *lightDir)
{
	MATRIX lightMatrix;
	VECTOR lightMac;
	VECTOR viewMac;
	SVec3 halfVector;
	s32 i;

	ConvertRotToMatrix_Transpose(&lightMatrix, rot);
	// NOTE(aalhendi): Normalize changes ROT but preserves LIGHT, so load this matrix only once.
	CTR_GteLoadLightMatrix(&lightMatrix);
	CTR_GteLoadSVec3V0(lightDir);
	CTR_GteLoadDelay();
	gte_llv0();
	CTR_GteStoreMAC(&lightMac.vx);

	inst->specLightX = (s8)lightMac.vx;
	inst->reflectionRGBA = (u16)lightMac.vz;

	for (i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
	{
		struct PushBuffer *pb = &GAME_TRACKER->pushBuffer[i];
		halfVector.x = (s16)inst->matrix.t[0] - pb->pos.x;
		halfVector.y = (s16)inst->matrix.t[1] - pb->pos.y;
		halfVector.z = (s16)inst->matrix.t[2] - pb->pos.z;
		MATH_VectorNormalize(&halfVector);
		CTR_GteLoadSVec3V0(&halfVector);
		CTR_GteLoadDelay();
		gte_llv0();
		CTR_GteStoreMAC(&viewMac.vx);

		halfVector.x = (s16)lightMac.vx + (s16)viewMac.vx;
		halfVector.y = (s16)lightMac.vy + (s16)viewMac.vy;
		halfVector.z = (s16)lightMac.vz + (s16)viewMac.vz;
		MATH_VectorNormalize(&halfVector);

		inst->idpp[i].halfVector.x = halfVector.x;
		inst->idpp[i].halfVector.y = halfVector.y;
		inst->idpp[i].halfVector.z = halfVector.z;
	}
}

void Vector_BakeMatrixTable(void)
{
	MATRIX scale;
	MATRIX rot;
	VECTOR translation;
	s32 i;
	s32 j;
	struct BakedGteMatrixTable *table;
	struct MatrixND *entry;
	MatrixNDAuthored *authored;
	void *matrixDst;
	s32 angle2000;
	s32 x;
	s32 y;
	s32 z;
	s32 vectorZ;
	u32 vectorXY;
	s32 translationY;
	VECTOR *translationPtr;

	if (GAME_MATRIX_TABLE_BAKED != 0)
	{
		return;
	}
	GAME_MATRIX_TABLE_BAKED = 1;

	// NOTE(aalhendi): Generate blasted rotation/scale values before the
	// in-place matrix bake replaces those authored slots.
	table = &data.bakedGteMath[BAKED_GTE_MATRIX_BLASTED];
	for (i = 0; i < table->numEntries; i++)
	{
		entry = &table->physEntry[i];
		authored = MatrixND_GetAuthored(entry);
		angle2000 = i << 0xd;

		authored->authoredRot.z = angle2000 / table->numEntries;
		authored->authoredRot.x = -MATH_Sin((i * 0x3000) / table->numEntries) / 7;
		// NOTE(aalhendi): Retail evaluates each sine separately and overwrites
		// this first X scale; preserving the order also preserves count reloads.
		authored->authoredScale.x = MATH_Sin(angle2000 / table->numEntries) * 6 / 0x28 + 0x1000;
		authored->authoredScale.z = MATH_Sin(angle2000 / table->numEntries) * 6 / 0x28 + 0x1000;
		authored->authoredScale.x = MATH_Sin(angle2000 / table->numEntries) / 4 + 0x1000;
	}

	// NOTE(aalhendi): Clear only the 18 coefficient bytes; the GTE ignores
	// MATRIX's padding and translation here.
	CTR_WriteU32AlignedLE((u8 *)&scale + 0, 0);
	CTR_WriteU32AlignedLE((u8 *)&scale + 4, 0);
	CTR_WriteU32AlignedLE((u8 *)&scale + 8, 0);
	CTR_WriteU32AlignedLE((u8 *)&scale + 12, 0);
	scale.m[2][2] = 0;
	for (i = 0; (u32)i < BAKED_GTE_MATRIX_COUNT; i++)
	{
		table = &data.bakedGteMath[i];
		if ((table->numEntries == 0) || (table->physEntry == NULL))
		{
			continue;
		}
		for (j = 0; j < table->numEntries; j++)
		{
			entry = &table->physEntry[j];
			authored = MatrixND_GetAuthored(entry);
			matrixDst = MatrixND_GetOverlapMatrix(entry);

			ConvertRotToMatrix(&rot, SVec3Slot_AsVec3(&authored->authoredRot));
			scale.m[0][0] = authored->authoredScale.x;
			scale.m[1][1] = authored->authoredScale.y;
			scale.m[2][2] = authored->authoredScale.z;
			// NOTE(aalhendi): Only the rotation payload is written: a full MATRIX would
			// extend beyond the 0x20-byte animation entry.
			MatrixRotate(matrixDst, &scale, &rot);
		}
	}

	// NOTE(aalhendi): Rotate the negative Y pivot, then translate back.
	// Store MAC, not saturated IR, and leave the final GTE state loaded.
	table = &data.bakedGteMath[BAKED_GTE_MATRIX_BLASTED];
	i = 0;
	if (table->numEntries > 0)
	{
		translationY = 0x2000;
		translationPtr = &translation;
		vectorXY = CTR_PackS16Pair(0, -0x2000);
		// NOTE(aalhendi): Capture the initial zero index, not later frame numbers.
		vectorZ = i;
		do
		{
			entry = &table->physEntry[i];
			authored = MatrixND_GetAuthored(entry);

			translation.vx = 0;
			translation.vy = translationY;
			translation.vz = 0;
			CTR_GteSetRotMatrix((MATRIX *)MatrixND_GetOverlapMatrix(entry));
			CTR_GteSetTransVector(translationPtr);
			MTC2(vectorXY, 0);
			MTC2(vectorZ, 1);
			CTR_GteLoadDelay();
			gte_mvmva(1, 0, 0, 0, 0);
			x = MFC2_S(25);
			y = MFC2_S(26);
			z = MFC2_S(27);
			authored->bakedOffset.x = (s16)x;
			authored->bakedOffset.y = (s16)y;
			authored->bakedOffset.z = (s16)z;
		} while (++i < table->numEntries);
	}
}
