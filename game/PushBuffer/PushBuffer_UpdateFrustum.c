#include <common.h>

#if defined(CTR_NATIVE)
// NOTE(aalhendi): Overlay 221 consumes this value through retail stack residue.
// Native retains it explicitly instead of reading unrelated stack storage.
global_variable s32 s_pushBufferFrustumSavedCameraZ;

s32 PushBuffer_GetFrustumSavedCameraZ(void)
{
	return s_pushBufferFrustumSavedCameraZ;
}
#endif

void PushBuffer_UpdateFrustum(struct PushBuffer *pb)
{
	s32 cameraPosX;
	s32 cameraPosY;
	s32 cameraPosZ;

	// Retail packs screen-space corner x/y into a single GTE VXY word.
	u32 cornerXY;
	s16 screenWidth, screenHeight;

	s32 clipFraction;
	s32 clipBound;

	s32 posX;
	s32 posY;
	s32 posZ;

	s32 distToScreen;
	s32 max_X;
	s32 max_Y;
	s32 max_Z;
	s32 min_X;
	s32 min_Y;
	s32 min_Z;
	s32 i;
	s32 roundedDepth;
	s32 flags;
	s32 planeResult;

	struct ScratchpadFrustum *spf = CTR_SCRATCHPAD_PTR(struct ScratchpadFrustum, 0);

	PushBuffer_SetMatrixVP(pb);

	max_X = pb->pos.x;
	max_Y = pb->pos.y;
	max_Z = pb->pos.z;
	min_X = max_X;
	min_Y = max_Y;
	min_Z = max_Z;
	cameraPosX = max_X;
	cameraPosY = max_Y;
	cameraPosZ = max_Z;
#if defined(CTR_NATIVE)
	s_pushBufferFrustumSavedCameraZ = cameraPosZ;
#endif

	screenWidth = pb->rect.w;
	screenHeight = (s16)CTR_MipsSra((s32)((u32)(s32)pb->rect.h * 0x600000u) / 0x360, 12);
	cornerXY = CTR_PackS16Pair(screenWidth / 2, screenHeight / 2);
	distToScreen = pb->distanceToScreen_PREV;
	MTC2((u32)distToScreen, 1);


	for (i = 3; i >= 0; i--)
	{
		// Rotate each screen-space ray into world space.
		MTC2(cornerXY, 0);
		CTR_GteLoadDelay();
		gte_llv0();

		switch (i)
		{
		case 3:
		{
			u32 lowNegX;
			u32 positiveY;

			lowNegX = CTR_PackS16Pair(-screenWidth / 2, 0);
			positiveY = CTR_PackS16Pair(0, screenHeight / 2);
			// NOTE(aalhendi): This non-emitting use preserves GCC's packed-half
			// allocation across the corner loop; it is not a GTE input.
			CTR_PSX_OBSERVE_VALUE(positiveY);
			cornerXY = lowNegX | positiveY;
			break;
		}
		case 2:
		{
			u32 lowPositiveX;
			u32 highNegativeY;

			lowPositiveX = CTR_PackS16Pair(screenWidth / 2, 0);
			highNegativeY = CTR_PackS16Pair(0, -screenHeight / 2);
			cornerXY = lowPositiveX | highNegativeY;
			break;
		}
		case 1:
			cornerXY = CTR_PackS16Pair(-screenWidth / 2, -screenHeight / 2);
			break;
		}

		// SetMatrixVP restores the camera rotation in the GTE for these rays.
		posX = MFC2_S(25);
		posY = MFC2_S(26);
		posZ = MFC2_S(27);

		spf->fc[i].pos.x = posX + cameraPosX;
		spf->fc[i].pos.y = posY + cameraPosY;
		spf->fc[i].pos.z = posZ + cameraPosZ;

		posX = (s32)((u32)posX * 0x100u);
		posY = (s32)((u32)posY * 0x100u);
		posZ = (s32)((u32)posZ * 0x100u);
		posX = (s32)((u32)posX + (u32)cameraPosX);
		posY = (s32)((u32)posY + (u32)cameraPosY);
		posZ = (s32)((u32)posZ + (u32)cameraPosZ);
		clipFraction = 0x1000;

		// Extend the ray by 256 before clipping it to world-space bounds.
		spf->clippedFarPos.x = posX;
		spf->clippedFarPos.y = posY;
		spf->clippedFarPos.z = posZ;

		clipBound = -0x8000;
		// === X Axis ===
		if (((cameraPosX < clipBound) && (clipBound < posX)) || ((clipBound < cameraPosX && (posX < clipBound))))
		{
			s32 numerator, denominator, fraction;
			numerator = (clipBound - cameraPosX) * 0x1000;
			denominator = posX - cameraPosX;
			fraction = numerator / denominator;

			if (fraction < clipFraction)
			{
				spf->clippedFarPos.x = clipBound;
				spf->clippedFarPos.y = cameraPosY + CTR_MipsSra((s32)((u32)fraction * ((u32)posY - (u32)cameraPosY)), 12);
				spf->clippedFarPos.z = cameraPosZ + CTR_MipsSra((s32)((u32)fraction * ((u32)posZ - (u32)cameraPosZ)), 12);
				clipFraction = fraction;
			}
		}

		// === Y Axis ===
		if (((cameraPosY < clipBound) && (clipBound < posY)) || ((clipBound < cameraPosY && (posY < clipBound))))
		{
			s32 numerator, denominator, fraction;
			numerator = (clipBound - cameraPosY) * 0x1000;
			denominator = posY - cameraPosY;
			fraction = numerator / denominator;

			if (fraction < clipFraction)
			{
				spf->clippedFarPos.y = clipBound;
				spf->clippedFarPos.x = cameraPosX + CTR_MipsSra((s32)((u32)fraction * ((u32)posX - (u32)cameraPosX)), 12);
				spf->clippedFarPos.z = cameraPosZ + CTR_MipsSra((s32)((u32)fraction * ((u32)posZ - (u32)cameraPosZ)), 12);
				clipFraction = fraction;
			}
		}

		// === Z Axis ===
		if (((cameraPosZ < clipBound) && (clipBound < posZ)) || ((clipBound < cameraPosZ && (posZ < clipBound))))
		{
			s32 numerator, denominator, fraction;
			numerator = (clipBound - cameraPosZ) * 0x1000;
			denominator = posZ - cameraPosZ;
			fraction = numerator / denominator;

			if (fraction < clipFraction)
			{
				spf->clippedFarPos.z = clipBound;
				spf->clippedFarPos.x = cameraPosX + CTR_MipsSra((s32)((u32)fraction * ((u32)posX - (u32)cameraPosX)), 12);
				spf->clippedFarPos.y = cameraPosY + CTR_MipsSra((s32)((u32)fraction * ((u32)posY - (u32)cameraPosY)), 12);
				clipFraction = fraction;
			}
		}

		clipBound = 0x7fff;
		// === X Axis ===
		if (((cameraPosX < clipBound) && (clipBound < posX)) || ((clipBound < cameraPosX && (posX < clipBound))))
		{
			s32 numerator, denominator, fraction;
			numerator = (clipBound - cameraPosX) * 0x1000;
			denominator = posX - cameraPosX;
			fraction = numerator / denominator;

			if (fraction < clipFraction)
			{
				spf->clippedFarPos.x = clipBound;
				spf->clippedFarPos.y = cameraPosY + CTR_MipsSra((s32)((u32)fraction * ((u32)posY - (u32)cameraPosY)), 12);
				spf->clippedFarPos.z = cameraPosZ + CTR_MipsSra((s32)((u32)fraction * ((u32)posZ - (u32)cameraPosZ)), 12);
				clipFraction = fraction;
			}
		}

		// === Y Axis ===
		if (((cameraPosY < clipBound) && (clipBound < posY)) || ((clipBound < cameraPosY && (posY < clipBound))))
		{
			s32 numerator, denominator, fraction;
			numerator = (clipBound - cameraPosY) * 0x1000;
			denominator = posY - cameraPosY;
			fraction = numerator / denominator;

			if (fraction < clipFraction)
			{
				spf->clippedFarPos.y = clipBound;
				spf->clippedFarPos.x = cameraPosX + CTR_MipsSra((s32)((u32)fraction * ((u32)posX - (u32)cameraPosX)), 12);
				spf->clippedFarPos.z = cameraPosZ + CTR_MipsSra((s32)((u32)fraction * ((u32)posZ - (u32)cameraPosZ)), 12);
				clipFraction = fraction;
			}
		}

		// === Z Axis ===
		if (((cameraPosZ < clipBound) && (clipBound < posZ)) || ((clipBound < cameraPosZ && (posZ < clipBound))))
		{
			s32 zDenominator;
			s32 numerator, fraction;
			numerator = (clipBound - cameraPosZ) * 0x1000;
			zDenominator = posZ - cameraPosZ;
			fraction = numerator / zDenominator;

			if (fraction < clipFraction)
			{
				spf->clippedFarPos.z = clipBound;
				spf->clippedFarPos.x = cameraPosX + CTR_MipsSra((s32)((u32)fraction * ((u32)posX - (u32)cameraPosX)), 12);
				spf->clippedFarPos.y = cameraPosY + CTR_MipsSra((s32)((u32)fraction * ((u32)posY - (u32)cameraPosY)), 12);
			}
		}

		posX = spf->clippedFarPos.x;
		posY = spf->clippedFarPos.y;
		posZ = spf->clippedFarPos.z;

		// Expand each axis of the world-space bounds.
		if (min_X > posX)
		{
			min_X = posX;
		}
		if (max_X < posX)
		{
			max_X = posX;
		}
		if (min_Y > posY)
		{
			min_Y = posY;
		}
		if (max_Y < posY)
		{
			max_Y = posY;
		}
		if (min_Z > posZ)
		{
			min_Z = posZ;
		}
		if (max_Z < posZ)
		{
			max_Z = posZ;
		}
	}

	pb->bbox.min.x = (s16)min_X;
	pb->bbox.max.x = (s16)max_X;
	pb->bbox.min.y = (s16)min_Y;
	pb->bbox.max.y = (s16)max_Y;
	pb->bbox.min.z = (s16)min_Z;
	pb->bbox.max.z = (s16)max_Z;

	// cameraPos (x,y,z)
	spf->camPos.x = cameraPosX;
	spf->camPos.y = cameraPosY;
	spf->camPos.z = cameraPosZ;

	// PushBuffer_SetFrustumPlane (x4)
	pb->RenderListJmpIndex[0] = ~PushBuffer_SetFrustumPlane(&pb->frustumPlanes[0], &spf->fc[0], &spf->camPos, &spf->fc[1]) & 7;

	pb->RenderListJmpIndex[1] = ~PushBuffer_SetFrustumPlane(&pb->frustumPlanes[1], &spf->fc[1], &spf->camPos, &spf->fc[3]) & 7;

	pb->RenderListJmpIndex[2] = ~PushBuffer_SetFrustumPlane(&pb->frustumPlanes[2], &spf->fc[3], &spf->camPos, &spf->fc[2]) & 7;

	pb->RenderListJmpIndex[3] = ~PushBuffer_SetFrustumPlane(&pb->frustumPlanes[3], &spf->fc[2], &spf->camPos, &spf->fc[0]) & 7;

	MTC2((u32)0, 0);
	MTC2((u32)(s32)0x1000, 1);
	CTR_GteLoadDelay();
	gte_llv0();

	posX = MFC2_S(25);
	posY = MFC2_S(26);
	posZ = MFC2_S(27);

	pb->frustumPlanes[4].normal.x = -posX;
	pb->frustumPlanes[4].normal.y = -posY;
	pb->frustumPlanes[4].normal.z = -posZ;

	planeResult = CTR_MipsSra((s32)(0u - ((u32)cameraPosX * (u32)posX + (u32)cameraPosY * (u32)posY + (u32)cameraPosZ * (u32)posZ)), 13);
	roundedDepth = distToScreen;
	if (distToScreen < 0)
	{
		roundedDepth = distToScreen + 3;
	}

	planeResult = (s32)((u32)planeResult - (u32)CTR_MipsSra(roundedDepth, 2));
	pb->frustumPlanes[4].halfDistance = (s16)planeResult;

	// Negation Flags
	flags = (u32)posX >> 0x1f;
	if (posY < 0)
	{
		flags = flags | 2;
	}
	planeResult = flags;
	if (posZ < 0)
	{
		planeResult = planeResult | 4;
	}

	// 0xE0, 0xE4
	pb->RenderListJmpIndex[4] = ~planeResult & 7;
	pb->RenderListJmpIndex[5] = planeResult;

	MTC2(0, 0);
	MTC2((u32)(distToScreen / 2), 1);
	CTR_GteLoadDelay();
	gte_llv0();

	posX = MFC2_S(25);
	posY = MFC2_S(26);
	posZ = MFC2_S(27);

	pb->frustumReferencePoint.x = posX + cameraPosX;
	pb->frustumReferencePoint.y = posY + cameraPosY;
	pb->frustumReferencePoint.z = posZ + cameraPosZ;
	return;
}
