#include <common.h>

#define PB_Divide1536(numerator, reciprocal) CTR_MipsSubLo(CTR_MipsSra(CTR_MipsMulHiS((numerator), (reciprocal)), 8), CTR_MipsSra((numerator), 31))

void PushBuffer_SetMatrixVP(struct PushBuffer *pb)
{
	// NOTE(aalhendi): These bindings constrain internal allocation, not the caller ABI.
	// The empty KEEP below retains the scratch pointer across the matrix conversion.
	register u32 camera0 CTR_PSX_REGISTER("$13");
	u32 camera4, camera8, cameraC;
	register s32 cameraLast;
	register u32 view0 CTR_PSX_REGISTER("$9");
	u32 view4, view8;
	u32 view4Low;
	register u32 viewC CTR_PSX_REGISTER("$8");
	u32 upperMask;
	s32 negX, negY, negZ;
	s32 scaledY, scaled10, scaled11, scaled12;
	s32 numerator, high, shift, row3input;
	s32 upperRow12;
	s32 aspectReciprocal;
	struct PushBufferSetMatrixVPScratch *scratch = CTR_SCRATCHPAD_PTR(struct PushBufferSetMatrixVPScratch, 0);
	register struct PushBufferMatrixVPWork *work CTR_PSX_REGISTER("$17") = &scratch->matrixVP;
	CTR_PSX_KEEP_VALUE_RELAXED(work);

	work->rot.x = pb->rot.x;
	work->rot.y = pb->rot.y;
	work->rot.z = pb->rot.z;
	ConvertRotToMatrix(&scratch->matrixVP.cameraMatrix, &scratch->matrixVP.rot);

	// Camera translation used by shadows, particles and audio.
	negX = pb->pos.x;
	negY = pb->pos.y;
	negZ = pb->pos.z;
	pb->matrix_Camera.t[0] = negX;
	negX = -negX;
	pb->matrix_Camera.t[1] = negY;
	negY = -negY;
	pb->matrix_Camera.t[2] = negZ;
	negZ = -negZ;

	cameraC = CTR_ReadU32AlignedLE(&work->cameraMatrix.m[2][0]);
	camera0 = CTR_ReadU32AlignedLE(&work->cameraMatrix.m[0][0]);
	camera8 = CTR_ReadU32AlignedLE(&work->cameraMatrix.m[1][1]);
	camera4 = CTR_ReadU32AlignedLE(&work->cameraMatrix.m[0][2]);
	cameraLast = work->cameraMatrix.m[2][2];
	upperMask = 0xffff0000U;

	CTR_WriteU32AlignedLE(&pb->matrix_Camera.m[0][0], camera0);
	CTR_WriteU32AlignedLE(&pb->matrix_Camera.m[0][2], camera4);
	CTR_WriteU32AlignedLE(&pb->matrix_Camera.m[1][1], camera8);
	CTR_WriteU32AlignedLE(&pb->matrix_Camera.m[2][0], cameraC);

	// Transpose packed pairs; lightning uses this unscaled camera matrix.
	view4 = (cameraC & 0xffff) | (camera0 & upperMask);
	view8 = (camera8 & 0xffff) | (cameraC & upperMask);
	view0 = camera0 & 0xffff;
	view0 |= camera4 & upperMask;
	viewC = (camera4 & 0xffff) | (camera8 & upperMask);
	CTR_WriteU32AlignedLE(&pb->matrix_CameraTranspose.m[0][0], view0);
	CTR_WriteU32AlignedLE(&pb->matrix_CameraTranspose.m[0][2], view4);
	CTR_WriteU32AlignedLE(&pb->matrix_CameraTranspose.m[1][1], view8);
	CTR_WriteU32AlignedLE(&pb->matrix_CameraTranspose.m[2][0], viewC);
	pb->matrix_Camera.m[2][2] = cameraLast;
	pb->matrix_CameraTranspose.m[2][2] = cameraLast;

	// Rotate the inverse camera position into camera space.
	CTC2(view0, 8);
	CTC2(view4, 9);
	CTC2(view8, 10);
	CTC2(viewC, 11);
	CTC2((u32)(s32)cameraLast, 12);
	MTC2(CTR_PackS16Pair(negX, negY), 0);
	MTC2((u32)negZ, 1);
	CTR_GteLoadDelay();
	gte_llv0();
	CTR_GteReadMAC123(negX, negY, negZ);

	// Scale Y by 864/1536: with the 512x216 viewport this gives 4:3.
	pb->matrix_CameraTranspose.t[1] = negY;
	aspectReciprocal = 0x2aaaaaab;
	row3input = (s32)(((u32)negY) * 864U);
	scaled10 = ((s16)(view4 >> 16)) * 0x360;
	shift = row3input;
	high = CTR_MipsMulHiS(row3input, aspectReciprocal);
	scaledY = CTR_MipsSra(high, 8);
	view4Low = view4 & 0xffff;
	scaled10 = PB_Divide1536(scaled10, aspectReciprocal);
	row3input = PB_Divide1536(((s16)view8) * 0x360, aspectReciprocal);
	scaled11 = (u32)row3input & 0xffff;
	shift = CTR_MipsSra(shift, 31);
	upperRow12 = (s16)(view8 >> 16);
	// NOTE(aalhendi): Keep this copy behind high's multiply for retail scheduling.
	// Native performs the same assignment without the artificial dependency.
	CTR_PSX_DEPEND_COPY(row3input, upperRow12, high);
	pb->matrix_CameraTranspose.t[0] = negX;
	pb->matrix_ViewProj.t[0] = negX;
	numerator = row3input * 864;
	negY = (s32)(((u32)scaledY) - ((u32)shift));
	scaled12 = PB_Divide1536(numerator, aspectReciprocal);
	pb->matrix_CameraTranspose.t[2] = negZ;
	pb->matrix_ViewProj.t[2] = negZ;
	CTR_WriteU32AlignedLE(&pb->matrix_ViewProj.m[0][0], view0);
	CTR_WriteU32AlignedLE(&pb->matrix_ViewProj.m[2][0], viewC);
	view4 = view4Low | (((u32)scaled10) << 16);
	view8 = (u32)scaled11 | (((u32)scaled12) << 16);
	pb->matrix_ViewProj.m[2][2] = cameraLast;
	CTR_WriteU32AlignedLE(&pb->matrix_ViewProj.m[1][1], view8);
	pb->matrix_ViewProj.t[1] = (s32)((u32)negY);
	CTR_WriteU32AlignedLE(&pb->matrix_ViewProj.m[0][2], view4);

	// Restore the camera lighting rows for the character intro cutscene.
	CTC2(camera0, 8);
	CTC2(camera4, 9);
	CTC2(camera8, 10);
	CTC2(cameraC, 11);
}

#undef PB_Divide1536
