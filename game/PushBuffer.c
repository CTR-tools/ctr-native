#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800426f8-0x80042910.
void PushBuffer_Init(struct PushBuffer *pb, int id, int total)
{
#if BUILD == EurRetail
#define SIZEY_TOP 0x74
#define SIZEY_1P  0xEC
#define STARTY_2P 0x78
#else
#define SIZEY_TOP 0x6a
#define SIZEY_1P  0xD8
#define STARTY_2P 0x6e
#endif

	pb->fade_step = 0x88;
	pb->matrix_Proj.m[0][0] = 0x1c71;
	*(u8 *)&pb->cameraID = (u8)id;

	pb->fadeFromBlack_currentValue = 0x1000;
	pb->fadeFromBlack_desiredResult = 0x1000;

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

	if (total == 1)
	{
		pb->rect.w = 0x200;
		pb->rect.h = SIZEY_1P;

		pb->distanceToScreen_PREV = 0x100;
		pb->distanceToScreen_CURR = 0x100;

		pb->aspectX = 4;
		pb->rect.x = 0;
		pb->rect.y = 0;
		pb->aspectY = 3;
		return;
	}

	if (total == 2)
	{
		if (id == 0)
		{
			pb->rect.w = 0x200;
			pb->rect.h = SIZEY_TOP;

			pb->distanceToScreen_PREV = 0x100;
			pb->distanceToScreen_CURR = 0x100;

			pb->aspectX = 8;
			pb->rect.x = 0;
			pb->rect.y = 0;
			pb->aspectY = 3;
			return;
		}

		if (id == 1)
		{
			pb->rect.y = STARTY_2P;
			pb->rect.w = 0x200;
			pb->rect.h = SIZEY_TOP;

			pb->distanceToScreen_PREV = 0x100;
			pb->distanceToScreen_CURR = 0x100;

			pb->aspectX = 8;
			pb->rect.x = 0;
			pb->aspectY = 3;
		}

		return;
	}

	if ((total < 3) || (total > 4))
		return;

	if (id == 0)
	{
		pb->rect.w = 0xfd;
		pb->rect.h = SIZEY_TOP;

		pb->distanceToScreen_PREV = 0x80;
		pb->distanceToScreen_CURR = 0x80;

		pb->aspectX = 4;
		pb->rect.x = 0;
		pb->rect.y = 0;
		pb->aspectY = 3;
		return;
	}

	if (id == 1)
	{
		pb->rect.x = 0x103;
		pb->rect.w = 0xfd;
		pb->rect.h = SIZEY_TOP;

		pb->distanceToScreen_PREV = 0x80;
		pb->distanceToScreen_CURR = 0x80;

		pb->aspectX = 4;
		pb->rect.y = 0;
		pb->aspectY = 3;
		return;
	}

	if (id == 2)
	{
		pb->rect.y = STARTY_2P;
		pb->rect.w = 0xfd;
		pb->rect.h = SIZEY_TOP;

		pb->distanceToScreen_PREV = 0x80;
		pb->distanceToScreen_CURR = 0x80;

		pb->aspectX = 4;
		pb->rect.x = 0;
		pb->aspectY = 3;
		return;
	}

	if (id == 3)
	{
		pb->rect.x = 0x103;
		pb->rect.y = STARTY_2P;
		pb->rect.w = 0xfd;
		pb->rect.h = SIZEY_TOP;

		pb->distanceToScreen_PREV = 0x80;
		pb->distanceToScreen_CURR = 0x80;

		pb->aspectX = 4;
		pb->aspectY = 3;
	}

	return;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80042910-0x80042974.
void PushBuffer_SetPsyqGeom(struct PushBuffer *pb)
{
	gte_SetGeomOffset(pb->rect.w / 2, pb->rect.h / 2);
	gte_SetGeomScreen(pb->distanceToScreen_PREV);
	return;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80042974-0x80042a8c.
void PushBuffer_SetDrawEnv_DecalMP(void *ot, struct DB *backBuffer, RECT *viewport, s16 offsetX, s16 offsetY, u8 dtd, u8 dfe, u8 isbg, u8 tpageUpper,
                                   u8 tpageLower)
{
	void *p;
	DRAWENV newDrawEnv;

	// Copy DrawEnv from gGT->backBuffer
	int *dst = (int *)&newDrawEnv;
	int *src = (int *)&backBuffer->drawEnv;

	for (u32 i = 0; i < sizeof(DRAWENV) / 4; i++)
		dst[i] = src[i];

	// Now modify DrawEnv...

	// RECT viewport (startX, startY, endX, endY)
	newDrawEnv.clip.x = viewport->x;
	newDrawEnv.clip.y = viewport->y;
	newDrawEnv.clip.w = viewport->w;
	newDrawEnv.clip.h = viewport->h;

	newDrawEnv.ofs[1] = offsetY;

	// tpage
	newDrawEnv.tpage = (u16)((tpageUpper << 8) | tpageLower);

	// dtd (dithering)
	newDrawEnv.dtd = dtd;

	// dfe (blocked or permitted)
	newDrawEnv.dfe = dfe;

	// isbg (always 0)
	newDrawEnv.isbg = isbg;

	p = backBuffer->primMem.cursor;
	void *prim = NULL;

	// cursor < guardEnd
	if (p <= backBuffer->primMem.guardEnd)
	{
		// advance curr
		backBuffer->primMem.cursor = (void *)((u32)backBuffer->primMem.cursor + 0x40);

		prim = p;
	}

	if (prim == NULL)
		return;

	// ofs[X]
	newDrawEnv.ofs[0] = offsetX;

	// DrawEnv just built
	SetDrawEnv(prim, &newDrawEnv);

	// This doesn't really draw a primitive,
	// it links the ptrOT from the camera,
	// into the ptrOT of backBuffer DB, allowing
	// this camera's primitives to draw
	AddPrim(ot, prim);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80042a8c-0x80042c04.
void PushBuffer_SetDrawEnv_Normal(void *ot, struct PushBuffer *pb, struct DB *backBuffer, s16 *copyDrawEnvNULL, int isbg)
{
	DRAWENV newDrawEnv;

	int *dst = (int *)&newDrawEnv;
	int *src = (int *)&backBuffer->drawEnv;

	for (u32 i = 0; i < sizeof(DRAWENV) / 4; i++)
		dst[i] = src[i];

	// always?
	if (copyDrawEnvNULL == 0)
	{
		newDrawEnv.clip.x += pb->rect.x;
		newDrawEnv.clip.y += pb->rect.y;
		newDrawEnv.clip.w = pb->rect.w;
		newDrawEnv.clip.h = pb->rect.h;
		newDrawEnv.ofs[0] += pb->rect.x;
		newDrawEnv.ofs[1] += pb->rect.y;
	}

	else
	{
		newDrawEnv.clip.x = copyDrawEnvNULL[0];
		newDrawEnv.clip.y = copyDrawEnvNULL[1];
		newDrawEnv.clip.w = copyDrawEnvNULL[2];
		newDrawEnv.clip.h = copyDrawEnvNULL[3];
		newDrawEnv.ofs[0] = copyDrawEnvNULL[0];
		newDrawEnv.ofs[1] = copyDrawEnvNULL[1];
	}

	newDrawEnv.isbg = isbg;

	void *p = backBuffer->primMem.cursor;
	if (p <= backBuffer->primMem.guardEnd)
	{
		backBuffer->primMem.cursor = (void *)((u32)backBuffer->primMem.cursor + 0x40);

		SetDrawEnv(p, &newDrawEnv);

		// This doesn't really draw a primitive,
		// it links the ptrOT from the camera,
		// into the ptrOT of backBuffer DB, allowing
		// this camera's primitives to draw
		AddPrim(ot, p);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80042c04-0x80042e50.
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

	// originally used 556 bytes
	struct PushBufferSetMatrixVPScratch *scratch = CTR_SCRATCHPAD_PTR(struct PushBufferSetMatrixVPScratch, 0);
	MATRIX *matrixDST = &scratch->cameraMatrix;

	scratch->rot = pb->rot;
	ConvertRotToMatrix(matrixDST, &scratch->rot);

	SVec3 negPos;

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
	gte_ldVXY0(*(int *)&negPos.v[0]);
	gte_ldVZ0(negPos.z);
#else
	gte_ldv0(&negPos);
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
	uVar3 = *(int *)&matrixDST->m[0][0];
	uVar4 = *(int *)&matrixDST->m[0][2];
	uVar5 = *(int *)&matrixDST->m[1][1];
	uVar6 = *(int *)&matrixDST->m[2][0];
	sVar7 = *(s16 *)&matrixDST->m[2][2];

	// CameraMatrix, for shadows, particles, and audio
	*(int *)((int)&pb->matrix_Camera + 0x0) = uVar3;
	*(int *)((int)&pb->matrix_Camera + 0x4) = uVar4;
	*(int *)((int)&pb->matrix_Camera + 0x8) = uVar5;
	*(int *)((int)&pb->matrix_Camera + 0xC) = uVar6;
	*(s16 *)((int)&pb->matrix_Camera + 0x10) = sVar7;

	// transpose the camera matrix
	view0 = uVar3 & 0xffff | uVar4 & 0xffff0000;
	view4 = uVar6 & 0xffff | uVar3 & 0xffff0000;
	view8 = uVar5 & 0xffff | uVar6 & 0xffff0000;
	viewC = uVar4 & 0xffff | uVar5 & 0xffff0000;

	// CameraTranspose, for lightning during Driver Warping effect
	*(int *)((int)&pb->matrix_CameraTranspose + 0x0) = view0;
	*(int *)((int)&pb->matrix_CameraTranspose + 0x4) = view4;
	*(int *)((int)&pb->matrix_CameraTranspose + 0x8) = view8;
	*(int *)((int)&pb->matrix_CameraTranspose + 0xC) = viewC;
	*(s16 *)((int)&pb->matrix_CameraTranspose + 0x10) = sVar7;

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

	gte_stlvnl(&pb->matrix_CameraTranspose.t[0]);
	gte_stlvnl(&pb->matrix_ViewProj.t[0]);

	// start with transpose camera matrix
	*(int *)((int)&pb->matrix_ViewProj + 0x0) = view0;
	*(int *)((int)&pb->matrix_ViewProj + 0x4) = view4;
	*(int *)((int)&pb->matrix_ViewProj + 0x8) = view8;
	*(int *)((int)&pb->matrix_ViewProj + 0xC) = viewC;
	*(s16 *)((int)&pb->matrix_ViewProj + 0x10) = sVar7;

	// NTSC:
	// 0x360/0x600 = 9/16 aspect,
	// 9/16 * 512/216 = 4/3

	// PAL:
	// 0x3B0/0x600 = 59/96 aspect,
	// 59/96 * 512/236 = 4/3

	// Do NOT set to 0x480
	// to change 4/3 to 16/9,
	// it will zoom "in" instead of "out"
	// because of stretching Y instead of X

#if BUILD == EurRetail
#define r360 0x3B0
#else
#define r360 0x360
#endif

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


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80042e50-0x800430f0.

static void PushBuffer_SetFrustumPlane_LoadAxisVector(int x, int y, int z)
{
	CTC2((u32)(s32)x, 0);
	CTC2((u32)(s32)y, 2);
	CTC2((u32)(s32)z, 4);
}

static void PushBuffer_SetFrustumPlane_LoadIRVector(int x, int y, int z)
{
	MTC2_S(x, 9);
	MTC2_S(y, 10);
	MTC2_S(z, 11);
}

static int PushBuffer_SetFrustumPlane_ReadLeadingZeroes(u32 value)
{
	MTC2(value, 30);
	return MFC2_S(31);
}

static s32 PushBuffer_SetFrustumPlane_Abs(s32 value)
{
	return (value < 0) ? -value : value;
}

int PushBuffer_SetFrustumPlane(s16 *frustumData, struct FrustumCornerOUT *fc1, s16 *camPos, struct FrustumCornerOUT *fc2)
{
	int leadingZeroBits;
	int temp;
	s32 normalX;
	s32 normalY;
	s32 normalZ;
	int cameraPosX = camPos[0];
	int cameraPosY = camPos[1];
	int cameraPosZ = camPos[2];

	PushBuffer_SetFrustumPlane_LoadAxisVector(fc2->pos.x - cameraPosX, fc2->pos.y - cameraPosY, fc2->pos.z - cameraPosZ);
	PushBuffer_SetFrustumPlane_LoadIRVector(fc1->pos.x - cameraPosX, fc1->pos.y - cameraPosY, fc1->pos.z - cameraPosZ);

	gte_op0();

	normalX = MFC2_S(25);
	normalY = MFC2_S(26);
	normalZ = MFC2_S(27);

	leadingZeroBits = PushBuffer_SetFrustumPlane_ReadLeadingZeroes((u32)PushBuffer_SetFrustumPlane_Abs(normalX));

	temp = PushBuffer_SetFrustumPlane_ReadLeadingZeroes((u32)PushBuffer_SetFrustumPlane_Abs(normalY));
	if (temp < leadingZeroBits)
		leadingZeroBits = temp;

	temp = PushBuffer_SetFrustumPlane_ReadLeadingZeroes((u32)PushBuffer_SetFrustumPlane_Abs(normalZ));
	if (temp < leadingZeroBits)
		leadingZeroBits = temp;

	if (leadingZeroBits < 0x12)
	{
		int vecBitShift = 0x12 - leadingZeroBits;
		normalX >>= vecBitShift & 0x1f;
		normalY >>= vecBitShift & 0x1f;
		normalZ >>= vecBitShift & 0x1f;
	}

	int length = SquareRoot0_stub(normalX * normalX + normalY * normalY + normalZ * normalZ);
	if (length != 0)
	{
		normalX = (normalX << 12) / length;
		normalY = (normalY << 12) / length;
		normalZ = (normalZ << 12) / length;
	}

	int planeD = (normalX * cameraPosX + normalY * cameraPosY + normalZ * cameraPosZ) >> 13;

	frustumData[0] = (s16)normalX;
	frustumData[1] = (s16)normalY;
	frustumData[2] = (s16)normalZ;
	frustumData[3] = (s16)planeD;

	u32 planeType = (u32)normalX >> 31;
	if (normalY < 0)
		planeType |= 2;
	if (normalZ < 0)
		planeType |= 4;

	return planeType;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800430f0-0x80043928.

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
	union FrustumCornerIN frustumCorner[4];

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

	struct FrustumCornerOUT *fcOUT = &spf->fc[3];

	for (int i = 0; i < 4; i++)
	{
		// multiply corner of screen,
		// by view-projection matrix,
		// to get frustum plane world-pos
		PushBuffer_UpdateFrustum_LoadV0(frustumCorner[i].self, pb->distanceToScreen_PREV);
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
		spf->pos[0] = posX;
		spf->pos[1] = posY;
		spf->pos[2] = posZ;

		// === X Axis ===
		if (((cameraPosX < -0x8000) && (-0x8000 < posX)) || ((-0x8000 < cameraPosX && (posX < -0x8000))))
		{
			ty = (-0x8000 - cameraPosX) * 0x1000;
			tx = posX - cameraPosX;
			tz = ty / tx;

			if (tz < 0x1000)
			{
				spf->pos[0] = -0x8000;
				spf->pos[1] = cameraPosY + (tz * (posY - cameraPosY) >> 0xc);
				spf->pos[2] = cameraPosZ + (tz * (posZ - cameraPosZ) >> 0xc);
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
				spf->pos[1] = -0x8000;
				spf->pos[0] = cameraPosX + (tz * (posX - cameraPosX) >> 0xc);
				spf->pos[2] = cameraPosZ + (tz * (posZ - cameraPosZ) >> 0xc);
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
				spf->pos[2] = -0x8000;
				spf->pos[0] = cameraPosX + (tz * (posX - cameraPosX) >> 0xc);
				spf->pos[1] = cameraPosY + (tz * (posY - cameraPosY) >> 0xc);
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
				spf->pos[0] = 0x7fff;
				spf->pos[1] = cameraPosY + (tz * (posY - cameraPosY) >> 0xc);
				spf->pos[2] = cameraPosZ + (tz * (posZ - cameraPosZ) >> 0xc);
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
				spf->pos[1] = 0x7fff;
				spf->pos[0] = cameraPosX + (tz * (posX - cameraPosX) >> 0xc);
				spf->pos[2] = cameraPosZ + (tz * (posZ - cameraPosZ) >> 0xc);
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
				spf->pos[2] = 0x7fff;
				spf->pos[0] = cameraPosX + (ty * (posX - cameraPosX) >> 0xc);
				spf->pos[1] = cameraPosY + (ty * (posY - cameraPosY) >> 0xc);
			}
		}

		// === Set 6 Min/Max X,Y,Z variables ===

		if (min_X > spf->pos[0])
			min_X = spf->pos[0];
		if (min_Y > spf->pos[1])
			min_Y = spf->pos[1];
		if (min_Z > spf->pos[2])
			min_Z = spf->pos[2];

		if (max_X < spf->pos[0])
			max_X = spf->pos[0];
		if (max_Y < spf->pos[1])
			max_Y = spf->pos[1];
		if (max_Z < spf->pos[2])
			max_Z = spf->pos[2];

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
	val_Y = PushBuffer_SetFrustumPlane(&pb->frustumData[0], &spf->fc[0], &spf->camPos.x, &spf->fc[1]);
	pb->RenderListJmpIndex[0] = ~val_Y & 7;

	val_Y = PushBuffer_SetFrustumPlane(&pb->frustumData[0x8], &spf->fc[1], &spf->camPos.x, &spf->fc[3]);
	pb->RenderListJmpIndex[1] = ~val_Y & 7;

	val_Y = PushBuffer_SetFrustumPlane(&pb->frustumData[0x10], &spf->fc[3], &spf->camPos.x, &spf->fc[2]);
	pb->RenderListJmpIndex[2] = ~val_Y & 7;

	val_Y = PushBuffer_SetFrustumPlane(&pb->frustumData[0x18], &spf->fc[2], &spf->camPos.x, &spf->fc[0]);
	pb->RenderListJmpIndex[3] = ~val_Y & 7;

	PushBuffer_UpdateFrustum_LoadV0(0, 0x1000);
	gte_llv0();

	int retX;
	int retY;
	int retZ;
	PushBuffer_UpdateFrustum_ReadMAC(&retX, &retY, &retZ);

	*(s16 *)&pb->frustumData[0x20] = -retX;
	*(s16 *)&pb->frustumData[0x22] = -retY;
	*(s16 *)&pb->frustumData[0x24] = -retZ;


	int distToScreen = pb->distanceToScreen_PREV;

	int iVar9 = distToScreen;
	if (distToScreen < 0)
	{
		iVar9 = distToScreen + 3;
	}

	*(s16 *)&pb->frustumData[0x26] = (s16)(-(cameraPosX * retX + cameraPosY * retY + cameraPosZ * retZ) >> 0xd) - (s16)(iVar9 >> 2);

	// Negation Flags
	int flags = (u32)retX >> 0x1f;
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

	*(s16 *)&pb->data6[0] = retX + cameraPosX;
	*(s16 *)&pb->data6[2] = retY + cameraPosY;
	*(s16 *)&pb->data6[4] = retZ + cameraPosZ;
	return;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80043928-0x80043ab8.
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

	struct DB *backBuffer = sdata->gGT->backBuffer;

	s16 currValue = pb->fadeFromBlack_currentValue;

	// if not 0x1000, which means there must be
	// some amount of fading
	if (currValue != 0x1000)
	{
		p = (multiCmdPacket *)backBuffer->primMem.cursor;

		setlen(p, 7);
		p->f4.tag = 0;
		p->f4.code = 0x2a;
		p->f4.x0 = 0;
		p->f4.y0 = 0;

		// if we are fading to black
		if (currValue < 0x1001)
		{
			p->tpage = 0xe1000a40;

			// get strength of fade (0 to 0x1000)
			fadeStrength = 0xfff - currValue;
		}
		else
		{
			// fade to white
			p->tpage = 0xe1000a20;

			// get strength of fade (0 to 0x1000)
			fadeStrength = currValue - 0x1000;
		}

#ifdef CTR_NATIVE
		// NOTE(aalhendi): Native PsyCross needs dfe=1 for this full-window fade.
		p->tpage |= 0x400; // set dfe=1
#endif

		// strength of fade
		fadeStrength = fadeStrength >> 4;

		p->f4.r0 = (u8)fadeStrength;
		p->f4.g0 = (u8)fadeStrength;
		p->f4.b0 = (u8)fadeStrength;
		p->f4.x1 = pb->rect.w;
		p->f4.y1 = 0;
		p->f4.x2 = 0;
		p->f4.y2 = pb->rect.h;
		p->f4.x3 = pb->rect.w;
		p->f4.y3 = pb->rect.h;
		AddPrim(pb->ptrOT, p);

		// move pointer after writing polygons
		backBuffer->primMem.cursor = p + 1;
	}

	// alter the fade value by the fade velocity
	currValue += pb->fade_step;

	// if fade velocity is negative
	if (pb->fade_step < 1)
	{
		// if we go lower than the desired fade
		if (currValue < pb->fadeFromBlack_desiredResult)
		{
			// set to desired fade
			currValue = pb->fadeFromBlack_desiredResult;
		}
	}

	// if fade velocity is positive
	else
	{
		// if we go higher than the desired fade value
		if (pb->fadeFromBlack_desiredResult < currValue)
		{
			// set to desired fade value
			currValue = pb->fadeFromBlack_desiredResult;
		}
	}

	// set new fade value
	pb->fadeFromBlack_currentValue = currValue;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80043ab8-0x80043b30.
void PushBuffer_FadeAllWindows()
{
	struct GameTracker *gGT = sdata->gGT;

	for (int i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		PushBuffer_FadeOneWindow(&gGT->pushBuffer[i]);
	}

	PushBuffer_FadeOneWindow(&gGT->pushBuffer_UI);
}
