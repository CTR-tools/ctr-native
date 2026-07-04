#include <common.h>

enum CsInstanceConstants
{
	CS_ANIM_FRAME_COUNT_MASK = 0x7fff,
	CS_ANIM_BONE_AXIS_STRIDE = 3,
	CS_ANIM_BONE_DATA_OFFSET = 0x1c,
	CS_ANIM_BONE_VALUE_X_OFFSET = 0,
	CS_ANIM_BONE_VALUE_Z_OFFSET = 1,
	CS_ANIM_BONE_VALUE_Y_OFFSET = 2,
	CS_ANIM_BONE_TARGET_X_OFFSET = 3,
	CS_ANIM_BONE_TARGET_Z_OFFSET = 4,
	CS_ANIM_BONE_TARGET_Y_OFFSET = 5,
};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac320-0x800ac5a4
void CS_Instance_GetFrameData(struct Instance *inst, int animIndex, u32 animFrame, SVec3 *pos, SVec3 *rotOut, int offset)
{
	int isOdd;
	int numFrames;
	struct ModelAnim *ptrAnim;
	s16 *framePos;
	u8 *bonePtr;
	u32 boneValueX, boneValueY, boneValueZ;
	u32 boneTargetX, boneTargetY, boneTargetZ;
	struct ModelHeader *headers;
	int scaleX, scaleY, scaleZ;
	int deltaDX, deltaDY, deltaDZ;

	headers = inst->model->headers;
	ptrAnim = headers->ptrAnimations[animIndex];

	if ((int)animFrame < 0)
	{
		animFrame = 0;
	}

	numFrames = (s16)ptrAnim->numFrames;
	isOdd = 0;

	if (numFrames < 0)
	{
		numFrames = -numFrames;
		isOdd = animFrame & 1;
		animFrame = animFrame >> 1;
	}

	if ((int)animFrame >= (numFrames - 1))
	{
		isOdd = 0;
		animFrame = numFrames - 1;
	}

	framePos = (s16 *)((char *)ptrAnim + ptrAnim->frameSize * (int)animFrame + sizeof(struct ModelAnim));

	{
		int boneOff = offset * CS_ANIM_BONE_AXIS_STRIDE + CS_ANIM_BONE_DATA_OFFSET;
		bonePtr = (u8 *)framePos + boneOff;
	}

	boneValueX = (u32)bonePtr[CS_ANIM_BONE_VALUE_X_OFFSET];
	boneValueY = (u32)bonePtr[CS_ANIM_BONE_VALUE_Y_OFFSET];
	boneValueZ = (u32)bonePtr[CS_ANIM_BONE_VALUE_Z_OFFSET];
	boneTargetX = (u32)bonePtr[CS_ANIM_BONE_TARGET_X_OFFSET];
	boneTargetY = (u32)bonePtr[CS_ANIM_BONE_TARGET_Y_OFFSET];
	boneTargetZ = (u32)bonePtr[CS_ANIM_BONE_TARGET_Z_OFFSET];

	if (isOdd)
	{
		framePos = (s16 *)((char *)framePos + ptrAnim->frameSize);
		{
			int boneOff = offset * CS_ANIM_BONE_AXIS_STRIDE + CS_ANIM_BONE_DATA_OFFSET;
			bonePtr = (u8 *)framePos + boneOff;
		}

		boneValueX = (int)(boneValueX + bonePtr[CS_ANIM_BONE_VALUE_X_OFFSET]) >> 1;
		boneValueY = (int)(boneValueY + bonePtr[CS_ANIM_BONE_VALUE_Y_OFFSET]) >> 1;
		boneValueZ = (int)(boneValueZ + bonePtr[CS_ANIM_BONE_VALUE_Z_OFFSET]) >> 1;
		boneTargetX = (int)(boneTargetX + bonePtr[CS_ANIM_BONE_TARGET_X_OFFSET]) >> 1;
		boneTargetY = (int)(boneTargetY + bonePtr[CS_ANIM_BONE_TARGET_Y_OFFSET]) >> 1;
		boneTargetZ = (int)(boneTargetZ + bonePtr[CS_ANIM_BONE_TARGET_Z_OFFSET]) >> 1;
	}

	deltaDX = (int)boneValueX - (int)boneTargetX;

	{
		s16 instScale = inst->scale.x;

		scaleX = ((((int)boneValueX + (int)framePos[0]) * instScale) >> FRACTIONAL_BITS) * (int)headers->scale.x >> FRACTIONAL_BITS;
		scaleY = ((((int)boneValueY + (int)framePos[1]) * instScale) >> FRACTIONAL_BITS) * (int)headers->scale.y >> FRACTIONAL_BITS;
		scaleZ = ((((int)boneValueZ + (int)framePos[2]) * instScale) >> FRACTIONAL_BITS) * (int)headers->scale.z >> FRACTIONAL_BITS;
	}

	deltaDY = (int)boneValueY - (int)boneTargetY;
	deltaDZ = (int)boneValueZ - (int)boneTargetZ;

	gte_SetLightMatrix(&inst->matrix);

	MTC2(CTR_PackS16Pair(scaleX, scaleY), 0);
	MTC2(scaleZ, 1);
	gte_llv0();

	{
		s32 mac[3];
		CTR_GteStoreMAC(mac);

		pos->x = (s16)mac[0];
		pos->y = (s16)mac[1];
		pos->z = (s16)mac[2];
	}

	if (rotOut != NULL)
	{
		MTC2(CTR_PackS16Pair(deltaDX, deltaDY), 0);
		MTC2(deltaDZ, 1);
		gte_llv0();

		{
			s32 mac[3];
			CTR_GteStoreMAC(mac);

			int dvx = mac[0];
			int dvy = mac[1];
			int dvz = mac[2];
			int pitch = ratan2(-dvy, SquareRoot0_stub(dvx * dvx + dvz * dvz));
			rotOut->x = (s16)pitch;

			int yaw = ratan2(dvx, dvz);
			rotOut->y = (s16)yaw;
			rotOut->z = 0;
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac5a4-0x800ac638
int CS_Instance_GetNumAnimFrames(struct Instance *modelInst, int animIndex, int LOD)
{
	struct Model *model;
	struct ModelHeader *header;
	struct ModelAnim *anim;

	if (modelInst == NULL)
	{
		return 0;
	}

	model = modelInst->model;
	if (model == NULL)
	{
		return 0;
	}

	if (LOD >= model->numHeaders)
	{
		return 0;
	}

	header = &model->headers[LOD];
	if (header == NULL)
	{
		return 0;
	}

	if (animIndex >= (int)header->numAnimations)
	{
		return 0;
	}

	if (header->ptrAnimations == NULL)
	{
		return 0;
	}

	anim = header->ptrAnimations[animIndex];
	if (anim == NULL)
	{
		return 0;
	}

	return (anim->numFrames & CS_ANIM_FRAME_COUNT_MASK);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac638-0x800ac694
int CS_Instance_SafeCheckAnimFrame(struct Instance *inst, int animIndex, int LOD, int desiredFrame)
{
	// Default return value
	int animFrame = desiredFrame;

	if (inst == NULL)
	{
		return animFrame;
	}

	if (desiredFrame <= 0)
	{
		return 0;
	}

	int numFrames = CS_Instance_GetNumAnimFrames(inst, animIndex, LOD);

	// if negative
	if (numFrames < 1)
	{
		return 0;
	}

	// if more than 1 and out of bounds
	if (numFrames <= desiredFrame)
	{
		animFrame = numFrames - 1;
	}

	// Return adjusted animFrame
	return animFrame;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac694-0x800ac714
char CS_Instance_BoolPlaySound(struct CutsceneObj *cs, struct Instance *desiredInst)
{
	struct Instance **visInstSrc;
	struct InstDrawPerPlayer *idpp;

	if ((desiredInst == NULL) || ((cs->flags & CS_FLAG_SOUND_ONSCREEN_ONLY) == 0))
	{
		return 1;
	}

	// pointer to array of visible instances
	visInstSrc = sdata->gGT->cameraDC[0].visInstSrc;

#if defined(CTR_NATIVE)
	// NOTE(aalhendi): Same native low-RAM guard as AH_WarpPad_ThTick:
	// a null camera list behaves like "desired instance is not visible."
	if (visInstSrc == NULL)
	{
		return 0;
	}
#endif

	// Same code as warppad_thtick
	while (visInstSrc[0] != 0)
	{
		if (visInstSrc[0] == desiredInst)
		{
			idpp = INST_GETIDPP(desiredInst);
			return (idpp[0].instFlags & DRAW_SUCCESSFUL) != 0;
		}

		visInstSrc++;
	}

	return 0;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac214-0x800ac320
void CS_Instance_InitMatrix(void)
{
	if (D233.cs_initMatrixBool != 0)
	{
		return;
	}

	D233.cs_initMatrixBool = 1;

	MATRIX mat;
	MATRIX scale = {0};

	for (int i = 0; i < 4; i++)
	{
		struct CsInitMatrixEntry *data = D233.cs_initMatrixTable[i].data;
		int count = D233.cs_initMatrixTable[i].count;

		if (data == NULL || count <= 0)
		{
			continue;
		}

		for (int j = 0; j < count; j++)
		{
			struct CsInitMatrixEntry *entry = &data[j];

			ConvertRotToMatrix(&mat, &entry->rot);

			scale.m[0][0] = entry->scale.x;
			scale.m[1][1] = entry->scale.y;
			scale.m[2][2] = entry->scale.z;

			// NOTE(aalhendi): Retail writes the 0x14-byte rotated payload
			// directly into this entry, not a full MATRIX copy.
			void *matrixDst = &entry->rotScaleOrMatrix[0];
			MatrixRotate(matrixDst, &scale, &mat);
		}
	}
}
