#include <common.h>


void PushBuffer_Init(struct PushBuffer *pb, int id, int total)
{
#define SIZEY_TOP 0x6a
#define SIZEY_1P  0xD8
#define STARTY_2P 0x6e

	*(u8 *)&pb->cameraID = (u8)id;

	pb->fadeFromBlack_currentValue = 0x1000;
	pb->fadeFromBlack_desiredResult = 0x1000;

	pb->fade_step = 0x88;
	pb->matrix_Proj.m[0][0] = 0x1c71;

	pb->matrix_Proj.m[0][1] = 0;
	pb->matrix_Proj.m[0][2] = 0;
	pb->matrix_Proj.m[1][0] = 0;
	pb->matrix_Proj.m[1][1] = 0x1000;
	pb->matrix_Proj.m[1][2] = 0;
	pb->matrix_Proj.m[2][0] = 0;
	pb->matrix_Proj.m[2][1] = 0;
	pb->matrix_Proj.m[2][2] = 0x1000;
	pb->matrix_Proj.t[0] = 0;
	pb->matrix_Proj.t[1] = 0;
	pb->matrix_Proj.t[2] = 0;

	switch (total)
	{
	case 1:
		pb->rect.w = 0x200;
		pb->rect.h = SIZEY_1P;

		pb->distanceToScreen_PREV = 0x100;
		pb->distanceToScreen_CURR = 0x100;

		pb->aspectX = 4;
		pb->rect.x = 0;
		pb->rect.y = 0;
		pb->aspectY = 3;
		return;

	case 2:
		switch (id)
		{
		case 0:
			pb->rect.w = 0x200;
			pb->rect.h = SIZEY_TOP;

			pb->distanceToScreen_PREV = 0x100;
			pb->distanceToScreen_CURR = 0x100;

			pb->aspectX = 8;
			pb->rect.x = 0;
			pb->rect.y = 0;
			pb->aspectY = 3;
			return;

		case 1:
			pb->rect.y = STARTY_2P;
			pb->rect.w = 0x200;
			pb->rect.h = SIZEY_TOP;

			pb->distanceToScreen_PREV = 0x100;
			pb->distanceToScreen_CURR = 0x100;

			pb->aspectX = 8;
			pb->rect.x = 0;
			pb->aspectY = 3;
			return;

		default:
			return;
		}

	case 3:
	case 4:
		switch (id)
		{
		case 0:
			pb->rect.w = 0xfd;
			pb->rect.h = SIZEY_TOP;

			pb->distanceToScreen_PREV = 0x80;
			pb->distanceToScreen_CURR = 0x80;

			pb->aspectX = 4;
			pb->rect.x = 0;
			pb->rect.y = 0;
			pb->aspectY = 3;
			return;

		case 1:
			pb->rect.x = 0x103;
			pb->rect.w = 0xfd;
			pb->rect.h = SIZEY_TOP;

			pb->distanceToScreen_PREV = 0x80;
			pb->distanceToScreen_CURR = 0x80;

			pb->aspectX = 4;
			pb->rect.y = 0;
			pb->aspectY = 3;
			return;

		case 2:
			pb->rect.y = STARTY_2P;
			pb->rect.w = 0xfd;
			pb->rect.h = SIZEY_TOP;

			pb->distanceToScreen_PREV = 0x80;
			pb->distanceToScreen_CURR = 0x80;

			pb->aspectX = 4;
			pb->rect.x = 0;
			pb->aspectY = 3;
			return;

		case 3:
			pb->rect.x = 0x103;
			pb->rect.y = STARTY_2P;
			pb->rect.w = 0xfd;
			pb->rect.h = SIZEY_TOP;

			pb->distanceToScreen_PREV = 0x80;
			pb->distanceToScreen_CURR = 0x80;

			pb->aspectX = 4;
			pb->aspectY = 3;
			return;

		default:
			return;
		}

	default:
		return;
	}
}


void PushBuffer_SetPsyqGeom(struct PushBuffer *pb)
{
	SetGeomOffset(pb->rect.w / 2, pb->rect.h / 2);
	SetGeomScreen(pb->distanceToScreen_PREV);
	return;
}


void PushBuffer_SetDrawEnv_DecalMP(void *ot, struct DB *backBuffer, RECT *viewport, s16 offsetX, s16 offsetY, u8 r0, u8 g0, u8 b0, s32 dtd, s32 isbg)
{
	DRAWENV newDrawEnv;
	struct PrimMem *primMem;
	void *prim;

#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail receives PS1 RAM OT slots here. Native translates
	// 24-bit OT tokens back to host pointers, so stale DecalMP range metadata
	// must not splice a DR_ENV packet into unrelated current-frame memory.
	if (!CtrGpu_IsCurrentOTRange(backBuffer, ot, ot))
	{
		return;
	}
#endif

	primMem = &backBuffer->primMem;
	newDrawEnv = backBuffer->drawEnv;
	newDrawEnv.clip.x = viewport->x;
	newDrawEnv.clip.y = viewport->y;
	newDrawEnv.clip.w = viewport->w;
	newDrawEnv.clip.h = viewport->h;
	newDrawEnv.ofs[0] = offsetX;
	newDrawEnv.ofs[1] = offsetY;
	newDrawEnv.r0 = r0;
	newDrawEnv.g0 = g0;
	newDrawEnv.b0 = b0;
	newDrawEnv.isbg = isbg;
	newDrawEnv.dtd = dtd;

	prim = NULL;
	if (primMem->cursor <= primMem->guardEnd)
	{
		prim = primMem->cursor;
		primMem->cursor = (u8 *)prim + 0x40;
	}

	if (prim != NULL)
	{
		SetDrawEnv(prim, &newDrawEnv);
		AddPrim(ot, prim);
	}
}


void PushBuffer_SetDrawEnv_Normal(void *ot, struct PushBuffer *pb, struct DB *backBuffer, s16 *copyDrawEnvNULL, int isbg)
{
	DRAWENV newDrawEnv;
	struct PrimMem *primMem;
	void *prim;

	primMem = &backBuffer->primMem;
	newDrawEnv = backBuffer->drawEnv;
	if (copyDrawEnvNULL != NULL)
	{
		newDrawEnv.clip.x = copyDrawEnvNULL[0];
		newDrawEnv.clip.y = copyDrawEnvNULL[1];
		newDrawEnv.clip.w = copyDrawEnvNULL[2];
		newDrawEnv.clip.h = copyDrawEnvNULL[3];
		newDrawEnv.ofs[0] = copyDrawEnvNULL[0];
		newDrawEnv.ofs[1] = copyDrawEnvNULL[1];
	}
	else
	{
		newDrawEnv.clip.x += pb->rect.x;
		newDrawEnv.clip.y += pb->rect.y;
		newDrawEnv.clip.w = pb->rect.w;
		newDrawEnv.clip.h = pb->rect.h;
		newDrawEnv.ofs[0] += pb->rect.x;
		newDrawEnv.ofs[1] += pb->rect.y;
	}
	newDrawEnv.isbg = isbg;

	prim = NULL;
	if (primMem->cursor <= primMem->guardEnd)
	{
		prim = primMem->cursor;
		primMem->cursor = (u8 *)prim + 0x40;
	}

	if (prim != NULL)
	{
		SetDrawEnv(prim, &newDrawEnv);
		AddPrim(ot, prim);
	}
}

void PushBuffer_SetMatrixVP(struct PushBuffer *pb)
{
	// CameraMatrix
	u32 uVar3;
	u32 uVar4;
	u32 uVar5;
	u32 uVar6;
	s16 sVar7;

	u32 view0;
	u32 view4;
	u32 view8;
	u32 viewC;
	SVec3 negPos;

	// originally used 556 bytes
	struct PushBufferSetMatrixVPScratch *scratch = CTR_SCRATCHPAD_PTR(struct PushBufferSetMatrixVPScratch, 0);
	MATRIX *matrixDST = &scratch->cameraMatrix;

	scratch->rot = pb->rot;
	ConvertRotToMatrix(matrixDST, &scratch->rot);

	pb->matrix_Camera.t[0] = pb->pos.x;
	pb->matrix_Camera.t[1] = pb->pos.y;
	pb->matrix_Camera.t[2] = pb->pos.z;

	negPos.x = -pb->pos.x;
	negPos.y = -pb->pos.y;
	negPos.z = -pb->pos.z;

	// load inverted camera position
#ifndef CTR_NATIVE
#define gte_ldVXY0(r0) __asm__ volatile("mtc2   %0, $0" : : "r"(r0))
#define gte_ldVZ0(r0)  __asm__ volatile("mtc2   %0, $1" : : "r"(r0))
	gte_ldVXY0(CTR_ReadU32AlignedLE(&negPos.x));
	gte_ldVZ0(negPos.z);
#else
	CTR_GteLoadSVec3V0(&negPos);
#endif

#ifndef CTR_NATIVE

// gte_SetLightMatrix
#define gte_r8(r0)  __asm__ volatile("ctc2   %0, $8" : : "r"(r0))
#define gte_r9(r0)  __asm__ volatile("ctc2   %0, $9" : : "r"(r0))
#define gte_r10(r0) __asm__ volatile("ctc2   %0, $10" : : "r"(r0))
#define gte_r11(r0) __asm__ volatile("ctc2   %0, $11" : : "r"(r0))
#define gte_r12(r0) __asm__ volatile("ctc2   %0, $12" : : "r"(r0))

#endif

	// CameraMatrix
	uVar3 = CTR_ReadU32LE(&matrixDST->m[0][0]);
	uVar4 = CTR_ReadU32LE(&matrixDST->m[0][2]);
	uVar5 = CTR_ReadU32LE(&matrixDST->m[1][1]);
	uVar6 = CTR_ReadU32LE(&matrixDST->m[2][0]);
	sVar7 = matrixDST->m[2][2];

	// CameraMatrix, for shadows, particles, and audio
	CTR_WriteU32LE(&pb->matrix_Camera.m[0][0], uVar3);
	CTR_WriteU32LE(&pb->matrix_Camera.m[0][2], uVar4);
	CTR_WriteU32LE(&pb->matrix_Camera.m[1][1], uVar5);
	CTR_WriteU32LE(&pb->matrix_Camera.m[2][0], uVar6);
	pb->matrix_Camera.m[2][2] = sVar7;

	// transpose the camera matrix
	view0 = (uVar3 & 0xffff) | (uVar4 & 0xffff0000);
	view4 = (uVar6 & 0xffff) | (uVar3 & 0xffff0000);
	view8 = (uVar5 & 0xffff) | (uVar6 & 0xffff0000);
	viewC = (uVar4 & 0xffff) | (uVar5 & 0xffff0000);

	// CameraTranspose, for lightning during Driver Warping effect
	CTR_WriteU32AlignedLE((u8 *)&pb->matrix_CameraTranspose + 0x0, view0);
	CTR_WriteU32AlignedLE((u8 *)&pb->matrix_CameraTranspose + 0x4, view4);
	CTR_WriteU32AlignedLE((u8 *)&pb->matrix_CameraTranspose + 0x8, view8);
	CTR_WriteU32AlignedLE((u8 *)&pb->matrix_CameraTranspose + 0xC, viewC);
	pb->matrix_CameraTranspose.m[2][2] = sVar7;

	// load transpose camera matrix
	// similar to gte_SetLightMatrix
#ifndef CTR_NATIVE
	gte_r8(view0);
	gte_r9(view4);
	gte_r10(view8);
	gte_r11(viewC);
	gte_r12(sVar7);
#else
	gte_SetLightMatrix(&pb->matrix_CameraTranspose);
#endif

	// multiply inverted camera position,
	// by transpose camera matrix
	gte_llv0();

	CTR_GteStoreMAC(&pb->matrix_CameraTranspose.t[0]);
	CTR_GteStoreMAC(&pb->matrix_ViewProj.t[0]);

	// start with transpose camera matrix
	CTR_WriteU32AlignedLE((u8 *)&pb->matrix_ViewProj + 0x0, view0);
	CTR_WriteU32AlignedLE((u8 *)&pb->matrix_ViewProj + 0x4, view4);
	CTR_WriteU32AlignedLE((u8 *)&pb->matrix_ViewProj + 0x8, view8);
	CTR_WriteU32AlignedLE((u8 *)&pb->matrix_ViewProj + 0xC, viewC);
	pb->matrix_ViewProj.m[2][2] = sVar7;

	// 0x360/0x600 = 9/16 aspect,
	// 9/16 * 512/216 = 4/3

	// Do NOT set to 0x480
	// to change 4/3 to 16/9,
	// it will zoom "in" instead of "out"
	// because of stretching Y instead of X

#define r360 0x360

// constant denomenator
#define r600 0x600

	// scale position
	pb->matrix_ViewProj.t[1] = pb->matrix_ViewProj.t[1] * r360 / r600;

	// scale Y axis (1)
	pb->matrix_ViewProj.m[1][0] = pb->matrix_ViewProj.m[1][0] * r360 / r600;

	// scale Y axis (2)
	pb->matrix_ViewProj.m[1][1] = pb->matrix_ViewProj.m[1][1] * r360 / r600;

	// scale Y axis (3)
	pb->matrix_ViewProj.m[1][2] = pb->matrix_ViewProj.m[1][2] * r360 / r600;

	// store camera matrix,
	// otherwise oxide intro cutscene bugs out,
	// when crash is sleeping on the grassy hill

#ifndef CTR_NATIVE
	gte_r8(uVar3);
	gte_r9(uVar4);
	gte_r10(uVar5);
	gte_r11(uVar6);
#else
	gte_SetLightMatrix(&scratch->cameraMatrix);
#endif

	return;
}


#include "PushBuffer/PushBuffer_SetFrustumPlane.c"


static void PushBuffer_UpdateFrustum_LoadV0(int xy, int z)
{
	MTC2((u32)xy, 0);
	MTC2((u32)(s32)z, 1);
}

static void PushBuffer_UpdateFrustum_ReadMAC(s32 *x, s32 *y, s32 *z)
{
	*x = MFC2_S(25);
	*y = MFC2_S(26);
	*z = MFC2_S(27);
}

#if defined(CTR_NATIVE)
global_variable s32 s_pushBufferFrustumSavedCameraZ;

s32 PushBuffer_GetFrustumSavedCameraZ(void)
{
	return s_pushBufferFrustumSavedCameraZ;
}
#endif

void PushBuffer_UpdateFrustum(struct PushBuffer *pb)
{
	int cameraPosX;
	int cameraPosY;
	int cameraPosZ;

	int val_X;
	int val_Y;

	// Retail packs screen-space corner x/y into a single GTE VXY word.
	struct FrustumCornerIN frustumCorner[4];

	int iVar19;

	int tx;
	int ty;
	int tz;

	int posX;
	int posY;
	int posZ;

	int min_X;
	int min_Y;
	int min_Z;
	int max_X;
	int max_Y;
	int max_Z;
	struct FrustumCornerOUT *fcOUT;
	int i;
	int retX;
	int retY;
	int retZ;
	int distToScreen;
	int iVar9;
	int flags;

	struct ScratchpadFrustum *spf = CTR_SCRATCHPAD_PTR(struct ScratchpadFrustum, 0);

#if 0
  // TRAP checks removed
  // assume no divide by zero
#endif

	PushBuffer_SetMatrixVP(pb);

	cameraPosX = pb->pos.x;
	cameraPosY = pb->pos.y;
	cameraPosZ = pb->pos.z;
#if defined(CTR_NATIVE)
	s_pushBufferFrustumSavedCameraZ = cameraPosZ;
#endif

	val_X = pb->rect.w;
	val_X = val_X / 2;

	val_Y = ((pb->rect.h * 0x600) / 0x360);
	val_Y = val_Y / 2;

	frustumCorner[0].x = val_X;
	frustumCorner[0].y = val_Y;

	frustumCorner[1].x = -val_X;
	frustumCorner[1].y = val_Y;

	frustumCorner[2].x = val_X;
	frustumCorner[2].y = -val_Y;

	frustumCorner[3].x = -val_X;
	frustumCorner[3].y = -val_Y;

	min_X = cameraPosX;
	min_Y = cameraPosY;
	min_Z = cameraPosZ;

	max_X = cameraPosX;
	max_Y = cameraPosY;
	max_Z = cameraPosZ;

	fcOUT = &spf->fc[3];

	for (i = 0; i < 4; i++)
	{
		// multiply corner of screen,
		// by view-projection matrix,
		// to get frustum plane world-pos
		PushBuffer_UpdateFrustum_LoadV0(CTR_PackS16Pair(frustumCorner[i].x, frustumCorner[i].y), pb->distanceToScreen_PREV);
		gte_llv0();

		// this is ViewProj matrix, loaded into GTE
		// from end of PushBuffer_SetMatrixVP (called earlier)
		PushBuffer_UpdateFrustum_ReadMAC(&tx, &ty, &tz);

		// far clip: pos + dir*100
		posX = tx * 0x100 + cameraPosX;
		posY = ty * 0x100 + cameraPosY;
		posZ = tz * 0x100 + cameraPosZ;

		iVar19 = 0x1000;

		fcOUT->pos.x = tx + cameraPosX;
		fcOUT->pos.y = ty + cameraPosY;
		fcOUT->pos.z = tz + cameraPosZ;

		// far clip: pos + dir*100
		spf->clippedFarPos.x = posX;
		spf->clippedFarPos.y = posY;
		spf->clippedFarPos.z = posZ;

		// === X Axis ===
		if (((cameraPosX < -0x8000) && (-0x8000 < posX)) || ((-0x8000 < cameraPosX && (posX < -0x8000))))
		{
			ty = (-0x8000 - cameraPosX) * 0x1000;
			tx = posX - cameraPosX;
			tz = ty / tx;

			if (tz < 0x1000)
			{
				spf->clippedFarPos.x = -0x8000;
				spf->clippedFarPos.y = cameraPosY + (tz * (posY - cameraPosY) >> 0xc);
				spf->clippedFarPos.z = cameraPosZ + (tz * (posZ - cameraPosZ) >> 0xc);
				iVar19 = tz;
			}
		}

		// === Y Axis ===
		if (((cameraPosY < -0x8000) && (-0x8000 < posY)) || ((-0x8000 < cameraPosY && (posY < -0x8000))))
		{
			ty = (-0x8000 - cameraPosY) * 0x1000;
			tx = posY - cameraPosY;
			tz = ty / tx;

			if (tz < iVar19)
			{
				spf->clippedFarPos.y = -0x8000;
				spf->clippedFarPos.x = cameraPosX + (tz * (posX - cameraPosX) >> 0xc);
				spf->clippedFarPos.z = cameraPosZ + (tz * (posZ - cameraPosZ) >> 0xc);
				iVar19 = tz;
			}
		}

		// === Z Axis ===
		if (((cameraPosZ < -0x8000) && (-0x8000 < posZ)) || ((-0x8000 < cameraPosZ && (posZ < -0x8000))))
		{
			ty = (-0x8000 - cameraPosZ) * 0x1000;
			tx = posZ - cameraPosZ;
			tz = ty / tx;

			if (tz < iVar19)
			{
				spf->clippedFarPos.z = -0x8000;
				spf->clippedFarPos.x = cameraPosX + (tz * (posX - cameraPosX) >> 0xc);
				spf->clippedFarPos.y = cameraPosY + (tz * (posY - cameraPosY) >> 0xc);
				iVar19 = tz;
			}
		}

		// === X Axis ===
		if (((cameraPosX < 0x7fff) && (0x7fff < posX)) || ((0x7fff < cameraPosX && (posX < 0x7fff))))
		{
			ty = (0x7fff - cameraPosX) * 0x1000;
			tx = posX - cameraPosX;
			tz = ty / tx;

			if (tz < iVar19)
			{
				spf->clippedFarPos.x = 0x7fff;
				spf->clippedFarPos.y = cameraPosY + (tz * (posY - cameraPosY) >> 0xc);
				spf->clippedFarPos.z = cameraPosZ + (tz * (posZ - cameraPosZ) >> 0xc);
				iVar19 = tz;
			}
		}

		// === Y Axis ===
		if (((cameraPosY < 0x7fff) && (0x7fff < posY)) || ((0x7fff < cameraPosY && (posY < 0x7fff))))
		{
			ty = (0x7fff - cameraPosY) * 0x1000;
			tx = posY - cameraPosY;
			tz = ty / tx;

			if (tz < iVar19)
			{
				spf->clippedFarPos.y = 0x7fff;
				spf->clippedFarPos.x = cameraPosX + (tz * (posX - cameraPosX) >> 0xc);
				spf->clippedFarPos.z = cameraPosZ + (tz * (posZ - cameraPosZ) >> 0xc);
				iVar19 = tz;
			}
		}

		// === Z Axis ===
		if (((cameraPosZ < 0x7fff) && (0x7fff < posZ)) || ((0x7fff < cameraPosZ && (posZ < 0x7fff))))
		{
			tx = (0x7fff - cameraPosZ) * 0x1000;
			posZ = posZ - cameraPosZ;
			ty = tx / posZ;

			if (ty < iVar19)
			{
				spf->clippedFarPos.z = 0x7fff;
				spf->clippedFarPos.x = cameraPosX + (ty * (posX - cameraPosX) >> 0xc);
				spf->clippedFarPos.y = cameraPosY + (ty * (posY - cameraPosY) >> 0xc);
			}
		}

		// === Set 6 Min/Max X,Y,Z variables ===

		if (min_X > spf->clippedFarPos.x)
		{
			min_X = spf->clippedFarPos.x;
		}
		if (min_Y > spf->clippedFarPos.y)
		{
			min_Y = spf->clippedFarPos.y;
		}
		if (min_Z > spf->clippedFarPos.z)
		{
			min_Z = spf->clippedFarPos.z;
		}

		if (max_X < spf->clippedFarPos.x)
		{
			max_X = spf->clippedFarPos.x;
		}
		if (max_Y < spf->clippedFarPos.y)
		{
			max_Y = spf->clippedFarPos.y;
		}
		if (max_Z < spf->clippedFarPos.z)
		{
			max_Z = spf->clippedFarPos.z;
		}

		// next corner to write
		fcOUT--;
	}

	pb->bbox.min.x = (s16)min_X;
	pb->bbox.min.y = (s16)min_Y;
	pb->bbox.min.z = (s16)min_Z;

	pb->bbox.max.x = (s16)max_X;
	pb->bbox.max.y = (s16)max_Y;
	pb->bbox.max.z = (s16)max_Z;

	// cameraPos (x,y,z)
	spf->camPos.x = cameraPosX;
	spf->camPos.y = cameraPosY;
	spf->camPos.z = cameraPosZ;

	// PushBuffer_SetFrustumPlane (x4)
	val_Y = PushBuffer_SetFrustumPlane(&pb->frustumPlanes[0], &spf->fc[0], &spf->camPos, &spf->fc[1]);
	pb->RenderListJmpIndex[0] = ~val_Y & 7;

	val_Y = PushBuffer_SetFrustumPlane(&pb->frustumPlanes[1], &spf->fc[1], &spf->camPos, &spf->fc[3]);
	pb->RenderListJmpIndex[1] = ~val_Y & 7;

	val_Y = PushBuffer_SetFrustumPlane(&pb->frustumPlanes[2], &spf->fc[3], &spf->camPos, &spf->fc[2]);
	pb->RenderListJmpIndex[2] = ~val_Y & 7;

	val_Y = PushBuffer_SetFrustumPlane(&pb->frustumPlanes[3], &spf->fc[2], &spf->camPos, &spf->fc[0]);
	pb->RenderListJmpIndex[3] = ~val_Y & 7;

	PushBuffer_UpdateFrustum_LoadV0(0, 0x1000);
	gte_llv0();

	PushBuffer_UpdateFrustum_ReadMAC(&retX, &retY, &retZ);

	pb->frustumPlanes[4].normal.x = -retX;
	pb->frustumPlanes[4].normal.y = -retY;
	pb->frustumPlanes[4].normal.z = -retZ;


	distToScreen = pb->distanceToScreen_PREV;

	iVar9 = distToScreen;
	if (distToScreen < 0)
	{
		iVar9 = distToScreen + 3;
	}

	pb->frustumPlanes[4].halfDistance = (s16)(-(cameraPosX * retX + cameraPosY * retY + cameraPosZ * retZ) >> 0xd) - (s16)(iVar9 >> 2);

	// Negation Flags
	flags = (u32)retX >> 0x1f;
	if (retY < 0)
	{
		flags = flags | 2;
	}
	if (retZ < 0)
	{
		flags = flags | 4;
	}

	// 0xE0, 0xE4
	pb->RenderListJmpIndex[4] = ~flags & 7;
	pb->RenderListJmpIndex[5] = flags;

	PushBuffer_UpdateFrustum_LoadV0(0, distToScreen / 2);
	gte_llv0();

	PushBuffer_UpdateFrustum_ReadMAC(&retX, &retY, &retZ);

	pb->frustumReferencePoint.x = retX + cameraPosX;
	pb->frustumReferencePoint.y = retY + cameraPosY;
	pb->frustumReferencePoint.z = retZ + cameraPosZ;
	return;
}


void PushBuffer_FadeOneWindow(struct PushBuffer *pb)
{
	typedef struct
	{
		u32 tag;
		u32 tpage;
		POLY_F4 f4;
	} multiCmdPacket;

	int fadeStrength;
	multiCmdPacket *p = NULL;

	if (pb->fadeFromBlack_currentValue != 0x1000)
	{
		if (pb->fadeFromBlack_currentValue > 0x1000)
		{
			p = (multiCmdPacket *)GAME_TRACKER->backBuffer->primMem.cursor;

			setlen(p, 7);
			p->f4.code = 0x2a;
			p->tpage = 0xe1000a20;
			p->f4.tag = 0;
			fadeStrength = pb->fadeFromBlack_currentValue - 0x1000;
			p->f4.x0 = 0;
			p->f4.y0 = 0;
		}
		else
		{
			p = (multiCmdPacket *)GAME_TRACKER->backBuffer->primMem.cursor;

			setlen(p, 7);
			p->f4.code = 0x2a;
			p->tpage = 0xe1000a40;
			p->f4.tag = 0;
			fadeStrength = 0xfff - pb->fadeFromBlack_currentValue;
			p->f4.x0 = 0;
			p->f4.y0 = 0;
		}

#ifdef CTR_NATIVE
		// NOTE(aalhendi): Native PsyCross needs dfe=1 for this full-window fade.
		p->tpage |= 0x400; // set dfe=1
#endif

		// strength of fade
		fadeStrength = fadeStrength >> 4;

		p->f4.r0 = p->f4.g0 = p->f4.b0 = (u8)fadeStrength;
		p->f4.x1 = pb->rect.w;
		p->f4.y1 = 0;
		p->f4.x2 = 0;
		p->f4.y2 = pb->rect.h;
		p->f4.x3 = pb->rect.w;
		p->f4.y3 = pb->rect.h;
		AddPrim(pb->ptrOT, p);

		// move pointer after writing polygons
		{
			struct DB *backBuffer = GAME_TRACKER->backBuffer;
			backBuffer->primMem.cursor = (u8 *)backBuffer->primMem.cursor + sizeof(multiCmdPacket);
		}
	}

	// NOTE(aalhendi): Narrow the updated fade state before clamping its field.
	pb->fadeFromBlack_currentValue += pb->fade_step;

	if (pb->fade_step > 0)
	{
		if (pb->fadeFromBlack_desiredResult < pb->fadeFromBlack_currentValue)
		{
			pb->fadeFromBlack_currentValue = pb->fadeFromBlack_desiredResult;
		}
	}
	else
	{
		if (pb->fadeFromBlack_currentValue < pb->fadeFromBlack_desiredResult)
		{
			pb->fadeFromBlack_currentValue = pb->fadeFromBlack_desiredResult;
		}
	}
}


void PushBuffer_FadeAllWindows()
{
	int i;

	for (i = 0; i < GAME_TRACKER->numPlyrCurrGame; i++)
	{
		PushBuffer_FadeOneWindow(&GAME_TRACKER->pushBuffer[i]);
	}

	PushBuffer_FadeOneWindow(&GAME_TRACKER->pushBuffer_UI);
}
