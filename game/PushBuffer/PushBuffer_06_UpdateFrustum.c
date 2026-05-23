#include <common.h>

// NOTE(aalhendi): Ghidra-assisted native port of retail 0x800430f0; required for honest 1P render lists.

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

	DECOMP_PushBuffer_SetMatrixVP(pb);

	cameraPosX = pb->pos[0];
	cameraPosY = pb->pos[1];
	cameraPosZ = pb->pos[2];

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
		posX = (s16)tx * 0x100 + cameraPosX;
		posY = (s16)ty * 0x100 + cameraPosY;
		posZ = (s16)tz * 0x100 + cameraPosZ;

		iVar19 = 0x1000;

		fcOUT->pos[0] = (s16)tx + cameraPosX;
		fcOUT->pos[1] = (s16)ty + cameraPosY;
		fcOUT->pos[2] = (s16)tz + cameraPosZ;

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

	pb->bbox.min[0] = (s16)min_X;
	pb->bbox.min[1] = (s16)min_Y;
	pb->bbox.min[2] = (s16)min_Z;

	pb->bbox.max[0] = (s16)max_X;
	pb->bbox.max[1] = (s16)max_Y;
	pb->bbox.max[2] = (s16)max_Z;

	// cameraPos (x,y,z)
	spf->camPos[0] = cameraPosX;
	spf->camPos[1] = cameraPosY;
	spf->camPos[2] = cameraPosZ;

	// PushBuffer_SetFrustumPlane (x4)
	val_Y = PushBuffer_SetFrustumPlane(&pb->frustumData[0], &spf->fc[0], &spf->camPos[0], &spf->fc[1]);
	pb->RenderListJmpIndex[0] = ~val_Y & 7;

	val_Y = PushBuffer_SetFrustumPlane(&pb->frustumData[0x8], &spf->fc[1], &spf->camPos[0], &spf->fc[3]);
	pb->RenderListJmpIndex[1] = ~val_Y & 7;

	val_Y = PushBuffer_SetFrustumPlane(&pb->frustumData[0x10], &spf->fc[3], &spf->camPos[0], &spf->fc[2]);
	pb->RenderListJmpIndex[2] = ~val_Y & 7;

	val_Y = PushBuffer_SetFrustumPlane(&pb->frustumData[0x18], &spf->fc[2], &spf->camPos[0], &spf->fc[0]);
	pb->RenderListJmpIndex[3] = ~val_Y & 7;

	PushBuffer_UpdateFrustum_LoadV0(0, 0x1000);
	gte_llv0();

	int retX;
	int retY;
	int retZ;
	PushBuffer_UpdateFrustum_ReadMAC(&retX, &retY, &retZ);

	*(s16 *)&pb->frustumData[0x20] = -(s16)retX;
	*(s16 *)&pb->frustumData[0x22] = -(s16)retY;
	*(s16 *)&pb->frustumData[0x24] = -(s16)retZ;


	int distToScreen = pb->distanceToScreen_PREV;

	int iVar9 = distToScreen;
	if (distToScreen < 0)
	{
		iVar9 = distToScreen + 3;
	}

	*(s16 *)&pb->frustumData[0x26] = (s16)(-(cameraPosX * (s16)retX + cameraPosY * (s16)retY + cameraPosZ * (s16)retZ) >> 0xd) - (s16)(iVar9 >> 2);

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

	*(s16 *)&pb->data6[0] = (s16)retX + cameraPosX;
	*(s16 *)&pb->data6[2] = (s16)retY + cameraPosY;
	*(s16 *)&pb->data6[4] = (s16)retZ + cameraPosZ;
	return;
}
