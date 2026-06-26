#include <common.h>

#define read_mt(r0, r1, r2) \
	{                       \
		r0 = MFC2(25);      \
		r1 = MFC2(26);      \
		r2 = MFC2(27);      \
	}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800ac320-0x800ac5a4
void CS_Instance_GetFrameData(struct Instance *inst, int animIndex, u32 animFrame, u16 *pos, u16 *param_5, int offset)
{
	int isOdd;
	int numFrames;
	struct ModelAnim *ptrAnim;
	s16 *framePos;
	u8 *bonePtr;
	u32 boneValX, boneValY, boneValZ;
	u32 boneDX, boneDY, boneDZ;
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
		int boneOff = offset * 3 + 0x1c;
		bonePtr = (u8 *)framePos + boneOff;
	}

	boneValX = (u32)bonePtr[0];
	boneValY = (u32)bonePtr[2];
	boneValZ = (u32)bonePtr[1];
	boneDX = (u32)bonePtr[3];
	boneDZ = (u32)bonePtr[5];
	boneDY = (u32)bonePtr[4];

	if (isOdd)
	{
		framePos = (s16 *)((char *)framePos + ptrAnim->frameSize);
		{
			int boneOff = offset * 3 + 0x1c;
			bonePtr = (u8 *)framePos + boneOff;
		}

		boneValX = (int)(boneValX + bonePtr[0]) >> 1;
		boneValY = (int)(boneValY + bonePtr[2]) >> 1;
		boneDZ = (int)(boneDZ + bonePtr[5]) >> 1;
		boneValZ = (int)(boneValZ + bonePtr[1]) >> 1;
		boneDX = (int)(boneDX + bonePtr[3]) >> 1;
		boneDY = (int)(boneDY + bonePtr[4]) >> 1;
	}

	deltaDX = (int)boneValX - (int)boneDX;

	{
		s16 instScale = inst->scale.x;

		scaleX = ((((int)boneValX + (int)framePos[0]) * instScale) >> 0xc) * (int)headers->scale.x >> 0xc;
		scaleY = ((((int)boneValY + (int)framePos[1]) * instScale) >> 0xc) * (int)headers->scale.y >> 0xc;
		scaleZ = ((((int)boneValZ + (int)framePos[2]) * instScale) >> 0xc) * (int)headers->scale.z >> 0xc;
	}

	deltaDY = (int)boneValY - (int)boneDZ;
	deltaDZ = (int)boneValZ - (int)boneDY;

	gte_SetLightMatrix(&inst->matrix);

	MTC2((scaleX & 0xffff) | ((u32)scaleY << 0x10), 0);
	MTC2(scaleZ, 1);
	gte_llv0();

	{
		int rx, ry, rz;
		read_mt(rx, ry, rz);

		pos[0] = (s16)rx;
		pos[1] = (s16)ry;
		pos[2] = (s16)rz;
	}

	if (param_5 != NULL)
	{
		MTC2((deltaDX & 0xffff) | ((u32)deltaDY << 0x10), 0);
		MTC2(deltaDZ, 1);
		gte_llv0();

		{
			int dvx, dvy, dvz;
			read_mt(dvx, dvy, dvz);

			int pitch = ratan2(-dvy, SquareRoot0_stub(dvx * dvx + dvz * dvz));
			param_5[0] = (s16)pitch;

			int yaw = ratan2(dvx, dvz);
			param_5[1] = (s16)yaw;
			param_5[2] = 0;
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

	if (animIndex >= header->numAnimations)
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

	return (anim->numFrames & 0x7fff);
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
	struct Instance *inst;
	struct Instance **visInstSrc;
	struct InstDrawPerPlayer *idpp;

	if ((desiredInst == NULL) || ((cs->flags & 0x1000) == 0))
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
			return (idpp[0].instFlags & 0x40) != 0;
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
		char *data = (char *)D233.cs_initMatrixTable[i].data;
		int count = D233.cs_initMatrixTable[i].count;

		if (data == NULL || count <= 0)
		{
			continue;
		}

		for (int j = 0; j < count; j++)
		{
			char *entry = data + j * 0x20;

			ConvertRotToMatrix(&mat, (const SVec3 *)(entry + 8));

			scale.m[0][0] = *(s16 *)(entry + 0x10);
			scale.m[1][1] = *(s16 *)(entry + 0x12);
			scale.m[2][2] = *(s16 *)(entry + 0x14);

			MatrixRotate((MATRIX *)(entry + 8), &scale, &mat);
		}
	}
}
