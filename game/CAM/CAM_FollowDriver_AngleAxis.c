#include <common.h>
#include <ctr_gte_matrix.h>

static inline s32 CAM_FollowDriver_AngleAxis_Lerp256(s32 current, s32 previous, s32 ratio)
{
	s32 currentPart = CTR_MipsMulLo(0x100 - ratio, current);
	s32 previousPart = CTR_MipsMulLo(ratio, previous);
	return CTR_MipsSra(CTR_MipsAddLo(currentPart, previousPart), 8);
}

void CAM_FollowDriver_AngleAxis(struct CameraDC *cDC, struct Driver *d, struct CameraAngleAxisScratch *scratchWork, SVec3 *pushBufferPos, SVec3 *pushBufferRot)
{
	MATRIX *axisMatrix;
	Vec3 lookAt;
	s32 distanceXZ;

	if (cDC->cameraMode == 0xe)
	{
		axisMatrix = &scratchWork->camera.matrix;
		VehPhysForce_RotAxisAngle(axisMatrix, CTR_VECTOR_DATA(&(d->AxisAngle2_normalVec)), d->angle);
		CTR_GteLoadRotMatrix(axisMatrix);
	}
	else
	{
		axisMatrix = &scratchWork->camera.matrix;
		VehPhysForce_RotAxisAngle(axisMatrix, CTR_VECTOR_DATA(&(d->AxisAngle2_normalVec)), d->rotCurr.y);
		CTR_GteLoadRotMatrix(axisMatrix);
	}

	CTR_GteLoadTransMatrix(&d->instSelf->matrix);
	MTC2((u32)(u16)cDC->driverOffset_CamEyePos.x | ((u32)cDC->driverOffset_CamEyePos.y << 16), 0);
	MTC2((u16)cDC->driverOffset_CamEyePos.z, 1);
	CTR_GteLoadDelay();
	gte_rtv0tr();
	scratchWork->camera.pos.x = (s32)MFC2(25);
	scratchWork->camera.pos.y = (s32)MFC2(26);
	scratchWork->camera.pos.z = (s32)MFC2(27);
	MTC2((u32)(u16)cDC->driverOffset_CamLookAtPos.x | ((u32)cDC->driverOffset_CamLookAtPos.y << 16), 0);
	MTC2((u16)cDC->driverOffset_CamLookAtPos.z, 1);
	CTR_GteLoadDelay();
	gte_rtv0tr();
	lookAt.x = (s32)MFC2(25);
	lookAt.y = (s32)MFC2(26);
	lookAt.z = (s32)MFC2(27);

	if ((cDC->flags & CAMERA_FLAG_DIRECTION_CHANGED) != 0)
	{
		cDC->lookAtPos.x = lookAt.x;
		cDC->lookAtPos.y = lookAt.y;
		cDC->lookAtPos.z = lookAt.z;
	}
	else
	{
		scratchWork->camera.pos.x = CAM_FollowDriver_AngleAxis_Lerp256(scratchWork->camera.pos.x, pushBufferPos->x, cDC->angleAxisLerpRatio);
		scratchWork->camera.pos.y = CAM_FollowDriver_AngleAxis_Lerp256(scratchWork->camera.pos.y, pushBufferPos->y, cDC->angleAxisLerpRatio);
		scratchWork->camera.pos.z = CAM_FollowDriver_AngleAxis_Lerp256(scratchWork->camera.pos.z, pushBufferPos->z, cDC->angleAxisLerpRatio);

		cDC->lookAtPos.x = CAM_FollowDriver_AngleAxis_Lerp256(lookAt.x, cDC->lookAtPos.x, cDC->angleAxisLerpRatio);
		cDC->lookAtPos.y = CAM_FollowDriver_AngleAxis_Lerp256(lookAt.y, cDC->lookAtPos.y, cDC->angleAxisLerpRatio);
		cDC->lookAtPos.z = CAM_FollowDriver_AngleAxis_Lerp256(lookAt.z, cDC->lookAtPos.z, cDC->angleAxisLerpRatio);
	}

	scratchWork->camera.dir.x = CTR_MipsSubLo(scratchWork->camera.pos.x, cDC->lookAtPos.x);
	scratchWork->camera.dir.y = CTR_MipsSubLo(scratchWork->camera.pos.y, cDC->lookAtPos.y);
	scratchWork->camera.dir.z = CTR_MipsSubLo(scratchWork->camera.pos.z, cDC->lookAtPos.z);

	pushBufferRot->y = (s16)ratan2(scratchWork->camera.dir.x, scratchWork->camera.dir.z);
	distanceXZ = SquareRoot0_stub(CTR_MipsAddLo(CTR_MipsMulLo(scratchWork->camera.dir.x, scratchWork->camera.dir.x),
	                                            CTR_MipsMulLo(scratchWork->camera.dir.z, scratchWork->camera.dir.z)));
	pushBufferRot->x = 0x800 - (s16)ratan2(scratchWork->camera.dir.y, distanceXZ);
	pushBufferRot->z = 0;

	pushBufferPos->x = (s16)scratchWork->camera.pos.x;
	pushBufferPos->y = (s16)scratchWork->camera.pos.y;
	pushBufferPos->z = (s16)scratchWork->camera.pos.z;
}
