#include <common.h>

static u32 Particle_RenderList_ReadWord(const void *base, int offset)
{
	return *(const u32 *)(const void *)((const char *)base + offset);
}

static u8 Particle_RenderList_ReadByte(const void *base, int offset)
{
	return *(const u8 *)(const void *)((const char *)base + offset);
}

static u32 Particle_RenderList_Ptr24(const void *ptr)
{
	return CtrGpu_PrimToOTLink24(ptr);
}

static s32 Particle_RenderList_MulLo(s32 left, s32 right)
{
	return (s32)(u32)((s64)left * (s64)right);
}

static s32 Particle_RenderList_MulShift(s32 left, s32 right, int shift)
{
	return Particle_RenderList_MulLo(left, right) >> shift;
}

static u32 Particle_RenderList_PackXY(s32 x, s32 y)
{
	return ((u32)x & 0xffff) | ((u32)y << 16);
}

static int Particle_RenderList_IsNearCamera(s32 value)
{
	if (value < 0)
		value = -value;

	return value < 30001;
}

static struct InstDrawPerPlayer *Particle_RenderList_GetIdpp(struct Instance *inst, int cameraID)
{
	return (struct InstDrawPerPlayer *)((char *)inst + sizeof(struct Instance) + (cameraID * sizeof(struct InstDrawPerPlayer)));
}

struct ParticleRenderListTrig
{
	s32 sin;
	s32 cos;
};

struct ParticleRenderListMatrix
{
	u32 r11r12;
	u32 r13r21;
	u32 r22r23;
	u32 r31r32;
	u32 r33;
};

static struct ParticleRenderListTrig Particle_RenderList_ReadTrig(s32 angle)
{
	struct TrigTable trigApprox = data.trigApprox[angle & 0x3ff];
	struct ParticleRenderListTrig trig;

	if ((angle & 0x400) == 0)
	{
		trig.sin = trigApprox.sin;
		trig.cos = trigApprox.cos;

		if ((angle & 0x800) != 0)
		{
			trig.sin = -trig.sin;
			trig.cos = -trig.cos;
		}
	}
	else
	{
		trig.sin = trigApprox.cos;

		if ((angle & 0x800) == 0)
		{
			trig.cos = -trigApprox.sin;
		}
		else
		{
			trig.sin = -trig.sin;
			trig.cos = trigApprox.sin;
		}
	}

	return trig;
}

static void Particle_RenderList_LinkPrimitive(u32 *prim, u_long *ot, u32 tag)
{
	prim[0] = (u32)*ot | tag;
	*ot = (u_long)Particle_RenderList_Ptr24(prim);
}

static void Particle_RenderList_LinkAndAdvance(u32 **primCursor, u32 **payloadCursor, struct Particle *particle, struct InstDrawPerPlayer *idpp,
                                               u16 flagsSetColor, s32 depth, u_long *defaultOT)
{
	u32 *prim = *primCursor;
	u_long *otBase;
	s32 otIndex;

	if (idpp != NULL)
	{
		otIndex = depth >> 5;

		if (otIndex < (u16)idpp->depthOffset[0])
			otIndex = (u16)idpp->depthOffset[0];

		if ((u16)idpp->depthOffset[1] < otIndex)
			otIndex = (u16)idpp->depthOffset[1];

		otBase = (u_long *)(uintptr_t)idpp->unkE4;
	}
	else
	{
		otIndex = (depth >> 8) + (s8)particle->unk18;

		if (otIndex < 0)
			otIndex = 0;

		if (otIndex >= 0x400)
			otIndex = 0x3ff;

		otBase = defaultOT;
	}

	if ((flagsSetColor & 0x1000) != 0)
	{
		Particle_RenderList_LinkPrimitive(prim, &otBase[otIndex], 0x06000000);
		*primCursor = prim + 7;
		*payloadCursor += 7;
	}
	else
	{
		Particle_RenderList_LinkPrimitive(prim, &otBase[otIndex], 0x09000000);
		*primCursor = prim + 10;
		*payloadCursor += 10;
	}
}

static void Particle_RenderList_WriteSpecialPrimitive(u32 *prim, struct Particle *particle, u16 flagsAxis, u16 flagsSetColor, u32 color, u32 *scratch,
                                                      s32 *scratchDepth)
{
	CTC2(scratch[0], 0);
	CTC2(scratch[1], 1);
	CTC2(scratch[2], 2);
	CTC2(scratch[3], 3);
	CTC2(scratch[4], 4);

	MTC2(0, 0);
	MTC2(0, 1);

	if ((flagsAxis & 0x20) != 0)
	{
		s32 scale = particle->axis[5].startVal;
		s32 deltaX = Particle_RenderList_MulLo((particle->axis[3].startVal - particle->axis[0].startVal) >> 6, scale);
		s32 deltaY = Particle_RenderList_MulLo((particle->axis[6].startVal - particle->axis[1].startVal) >> 6, scale);
		s32 deltaZ = Particle_RenderList_MulLo((particle->axis[4].startVal - particle->axis[2].startVal) >> 6, scale);

		MTC2(((u32)deltaX >> 16) | ((u32)(deltaY >> 16) << 16), 2);
		MTC2((u32)(deltaZ >> 16), 3);
	}
	else
	{
		s32 deltaX = (particle->axis[3].startVal - particle->axis[0].startVal) >> 6;
		s32 deltaY = (particle->axis[6].startVal - particle->axis[1].startVal) >> 6;
		s32 deltaZ = (particle->axis[4].startVal - particle->axis[2].startVal) >> 6;

		MTC2(Particle_RenderList_PackXY(deltaX, deltaY), 2);
		MTC2((u32)deltaZ, 3);
	}

	gte_rtpt_b();

	color |= 0x50000000;

	if ((flagsSetColor & 0x2000) != 0)
	{
		prim[5] = color;
		prim[3] = particle->axis[10].startVal;
	}
	else
	{
		prim[3] = color;
		prim[5] = particle->axis[10].startVal;
	}

	*(u32 *)(void *)&particle->axis[10].velocity = color;
	prim[1] = 0xe1000a00 | (flagsSetColor & 0x60);
	prim[2] = 0;
	prim[4] = MFC2(12);
	prim[6] = MFC2(13);
	*scratchDepth = (s32)MFC2(17);
}

static struct ParticleRenderListMatrix Particle_RenderList_BuildNormalMatrix(struct Particle *particle, u16 flagsAxis)
{
	struct ParticleRenderListMatrix matrix;

	matrix.r11r12 = 0x2000;
	matrix.r13r21 = 0;
	matrix.r22r23 = 0x1000;
	matrix.r31r32 = 0;
	matrix.r33 = 0x1000;

	if ((flagsAxis & 0x8) == 0)
	{
		if ((flagsAxis & 0x10) == 0)
		{
			if ((flagsAxis & 0x20) != 0)
			{
				matrix.r11r12 = (u32)particle->axis[5].startVal << 1;
				matrix.r22r23 = (s32)matrix.r11r12 >> 1;
			}

			if ((flagsAxis & 0x40) != 0)
				matrix.r22r23 = particle->axis[6].startVal;

			return matrix;
		}

		struct ParticleRenderListTrig rotY = Particle_RenderList_ReadTrig(particle->axis[4].startVal);

		if ((flagsAxis & 0x20) == 0)
		{
			matrix.r11r12 = (((u32)rotY.cos & 0x7fff) << 1) | ((u32)rotY.sin << 17);

			if ((flagsAxis & 0x40) != 0)
			{
				s32 scaleY = particle->axis[6].startVal;

				matrix.r13r21 = (u32)(Particle_RenderList_MulLo(-rotY.sin, scaleY) >> 12) << 16;
				matrix.r22r23 = (u32)Particle_RenderList_MulShift(rotY.cos, scaleY, 12) & 0xffff;
			}
			else
			{
				matrix.r13r21 = (u32)-rotY.sin << 16;
				matrix.r22r23 = (u32)rotY.cos & 0xffff;
			}
		}
		else
		{
			s32 scaleX = particle->axis[5].startVal;

			if ((flagsAxis & 0x40) != 0)
			{
				s32 scaleY = particle->axis[6].startVal;

				matrix.r11r12 =
				    ((u32)Particle_RenderList_MulShift(rotY.cos, scaleX, 11) & 0xffff) | ((u32)Particle_RenderList_MulShift(rotY.sin, scaleX, 11) << 16);
				matrix.r13r21 = (u32)Particle_RenderList_MulShift(-rotY.sin, scaleY, 12) << 16;
				matrix.r22r23 = (u32)Particle_RenderList_MulShift(rotY.cos, scaleY, 12) & 0xffff;
			}
			else
			{
				s32 scaledCos = Particle_RenderList_MulShift(rotY.cos, scaleX, 12);
				s32 scaledSin = Particle_RenderList_MulShift(rotY.sin, scaleX, 12);

				matrix.r11r12 = (((u32)scaledCos & 0x7fff) << 1) | ((u32)scaledSin << 17);
				matrix.r13r21 = (u32)-scaledSin << 16;
				matrix.r22r23 = (u32)scaledCos & 0xffff;
			}
		}

		return matrix;
	}

	if ((flagsAxis & 0x10) == 0)
	{
		struct ParticleRenderListTrig rotX = Particle_RenderList_ReadTrig(particle->axis[3].startVal);

		if ((flagsAxis & 0x20) != 0)
		{
			s32 scaleX = particle->axis[5].startVal;

			matrix.r11r12 = ((u32)scaleX << 1) & 0xffff;

			if ((flagsAxis & 0x40) != 0)
			{
				s32 scaleY = particle->axis[6].startVal;

				matrix.r31r32 = (u32)-rotX.sin << 16;
				matrix.r33 = (u32)rotX.cos & 0xffff;
				matrix.r22r23 =
				    ((u32)Particle_RenderList_MulShift(rotX.cos, scaleY, 12) & 0xffff) | ((u32)Particle_RenderList_MulShift(rotX.sin, scaleY, 12) << 16);
			}
			else
			{
				s32 scaledSin = Particle_RenderList_MulShift(rotX.sin, scaleX, 12);
				s32 scaledCos = Particle_RenderList_MulShift(rotX.cos, scaleX, 12);

				matrix.r33 = (u32)scaledCos & 0xffff;
				matrix.r22r23 = matrix.r33 | ((u32)scaledSin << 16);
				matrix.r31r32 = (u32)-scaledSin << 16;
			}
		}
		else
		{
			matrix.r33 = (u32)rotX.cos & 0xffff;

			if ((flagsAxis & 0x40) != 0)
			{
				s32 scaleY = particle->axis[6].startVal;

				matrix.r31r32 = (u32)-rotX.sin << 16;
				matrix.r22r23 =
				    ((u32)Particle_RenderList_MulShift(rotX.cos, scaleY, 12) & 0xffff) | ((u32)Particle_RenderList_MulShift(rotX.sin, scaleY, 12) << 16);
			}
			else
			{
				matrix.r22r23 = ((u32)rotX.cos & 0xffff) | ((u32)rotX.sin << 16);
				matrix.r31r32 = (u32)-rotX.sin << 16;
			}
		}

		return matrix;
	}

	struct ParticleRenderListTrig rotX = Particle_RenderList_ReadTrig(particle->axis[3].startVal);
	struct ParticleRenderListTrig rotY = Particle_RenderList_ReadTrig(particle->axis[4].startVal);

	if ((flagsAxis & 0x20) != 0)
	{
		s32 scaleX = particle->axis[5].startVal;

		if ((flagsAxis & 0x40) != 0)
		{
			s32 scaleY = particle->axis[6].startVal;
			s32 r13Base = Particle_RenderList_MulShift(rotX.cos, -rotY.sin, 12);
			s32 r22Base = Particle_RenderList_MulShift(rotY.cos, rotX.cos, 12);

			matrix.r33 = (u32)rotX.cos & 0xffff;
			matrix.r11r12 =
			    ((u32)Particle_RenderList_MulShift(rotY.cos, scaleX, 11) & 0xffff) | ((u32)Particle_RenderList_MulShift(rotY.sin, scaleX, 11) << 16);
			matrix.r31r32 =
			    ((u32)Particle_RenderList_MulShift(rotX.sin, rotY.sin, 12) & 0xffff) | ((u32)Particle_RenderList_MulShift(-rotX.sin, rotY.cos, 12) << 16);
			matrix.r13r21 = (u32)Particle_RenderList_MulShift(r13Base, scaleY, 12) << 16;
			matrix.r22r23 = ((u32)Particle_RenderList_MulShift(r22Base, scaleY, 12) & 0xffff) | ((u32)Particle_RenderList_MulShift(rotX.sin, scaleY, 12) << 16);
		}
		else
		{
			s32 scaledCosX = Particle_RenderList_MulShift(rotX.cos, scaleX, 12);
			s32 scaledSinX = Particle_RenderList_MulShift(rotX.sin, scaleX, 12);
			s32 scaledCosY = Particle_RenderList_MulShift(rotY.cos, scaleX, 12);
			s32 scaledSinY = Particle_RenderList_MulShift(rotY.sin, scaleX, 12);

			matrix.r33 = (u32)scaledCosX & 0xffff;
			matrix.r11r12 = (((u32)scaledCosY << 1) & 0xffff) | ((u32)scaledSinY << 17);
			matrix.r13r21 = (u32)Particle_RenderList_MulShift(scaledCosX, -scaledSinY, 12) << 16;
			matrix.r22r23 = ((u32)Particle_RenderList_MulShift(scaledCosY, scaledCosX, 12) & 0xffff) | ((u32)scaledSinX << 16);
			matrix.r31r32 = ((u32)Particle_RenderList_MulShift(scaledSinX, scaledSinY, 12) & 0xffff) |
			                ((u32)Particle_RenderList_MulShift(-scaledSinX, scaledCosY, 12) << 16);
		}
	}
	else
	{
		if ((flagsAxis & 0x40) != 0)
		{
			s32 scaleY = particle->axis[6].startVal;
			s32 r13Base = Particle_RenderList_MulShift(rotX.cos, -rotY.sin, 12);
			s32 r22Base = Particle_RenderList_MulShift(rotY.cos, rotX.cos, 12);

			matrix.r33 = (u32)rotX.cos & 0xffff;
			matrix.r11r12 = (((u32)rotY.cos << 1) & 0xffff) | ((u32)rotY.sin << 17);
			matrix.r31r32 =
			    ((u32)Particle_RenderList_MulShift(rotX.sin, rotY.sin, 12) & 0xffff) | ((u32)Particle_RenderList_MulShift(-rotX.sin, rotY.cos, 12) << 16);
			matrix.r13r21 = (u32)Particle_RenderList_MulShift(r13Base, scaleY, 12) << 16;
			matrix.r22r23 = ((u32)Particle_RenderList_MulShift(r22Base, scaleY, 12) & 0xffff) | ((u32)Particle_RenderList_MulShift(rotX.sin, scaleY, 12) << 16);
		}
		else
		{
			matrix.r33 = (u32)rotX.cos & 0xffff;
			matrix.r11r12 = (((u32)rotY.cos << 1) & 0xffff) | ((u32)rotY.sin << 17);
			matrix.r13r21 = (u32)Particle_RenderList_MulShift(rotX.cos, -rotY.sin, 12) << 16;
			matrix.r22r23 = ((u32)Particle_RenderList_MulShift(rotY.cos, rotX.cos, 12) & 0xffff) | ((u32)rotX.sin << 16);
			matrix.r31r32 =
			    ((u32)Particle_RenderList_MulShift(rotX.sin, rotY.sin, 12) & 0xffff) | ((u32)Particle_RenderList_MulShift(-rotX.sin, rotY.cos, 12) << 16);
		}
	}

	return matrix;
}

static void Particle_RenderList_WriteNormalPrimitive(u32 *prim, struct Icon *icon, u16 flagsAxis, u16 flagsSetColor, u32 color,
                                                     struct ParticleRenderListMatrix *matrix, s32 *scratchDepth)
{
	s32 width;
	s32 height;
	s32 halfWidth;
	s32 halfHeight;
	u32 input;

	CTC2(matrix->r11r12, 0);
	CTC2(matrix->r13r21, 1);
	CTC2(matrix->r22r23, 2);
	CTC2(matrix->r31r32, 3);
	CTC2(matrix->r33, 4);

	prim[1] = color | 0x2c000000;
	prim[3] = Particle_RenderList_ReadWord(icon, 0x14);
	prim[5] = (Particle_RenderList_ReadWord(icon, 0x18) & 0xff9fffff) | ((u32)(flagsSetColor & 0x60) << 16);
	*(u16 *)(void *)&prim[7] = *(u16 *)(void *)((char *)icon + 0x1c);
	*(u16 *)(void *)&prim[9] = *(u16 *)(void *)((char *)icon + 0x1e);

	width = (Particle_RenderList_ReadByte(icon, 0x18) - Particle_RenderList_ReadByte(icon, 0x14)) + 1;
	height = (Particle_RenderList_ReadByte(icon, 0x1d) - Particle_RenderList_ReadByte(icon, 0x15)) + 1;

	halfWidth = width << 1;
	halfHeight = height << 1;

	if ((flagsSetColor & 0x400) != 0)
	{
		halfWidth = width << 4;
		halfHeight = height << 4;
	}

	input = Particle_RenderList_PackXY(-halfWidth, -halfHeight);
	MTC2(0, 1);

	MTC2(input, 0);
	gte_rtps_b();
	prim[2] = MFC2(14);
	*scratchDepth = (s32)MFC2(19);

	input = Particle_RenderList_PackXY(halfWidth, -halfHeight);
	MTC2(input, 0);
	gte_rtps_b();
	prim[4] = MFC2(14);

	input = Particle_RenderList_PackXY(-halfWidth, halfHeight);
	MTC2(input, 0);
	gte_rtps_b();
	prim[6] = MFC2(14);

	input = Particle_RenderList_PackXY(halfWidth, halfHeight);
	MTC2(input, 0);
	gte_rtps_b();
	prim[8] = MFC2(14);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003f590-0x80040308
void Particle_RenderList(struct PushBuffer *pb, void *particleList)
{
	struct GameTracker *gGT = sdata->gGT;
	struct PrimMem *primMem = &gGT->backBuffer->primMem;
	struct Particle *particle = particleList;
	u32 *scratch = CTR_SCRATCHPAD_PTR(u32, 0x00);
	u16 *scratchR33Low = CTR_SCRATCHPAD_PTR(u16, 0x10);
	u_long **scratchOT = CTR_SCRATCHPAD_PTR(u_long *, 0x20);
	s32 *scratchCameraOffset = CTR_SCRATCHPAD_PTR(s32, 0x24);
	s32 *scratchDepth = CTR_SCRATCHPAD_PTR(s32, 0x30);
	u32 *prim = (u32 *)primMem->curr;
	u32 *primPayload = prim + 8;
	char cameraID;

	PushBuffer_SetPsyqGeom(pb);

	scratch[0] = Particle_RenderList_ReadWord(&pb->matrix_ViewProj, 0x00);
	scratch[1] = Particle_RenderList_ReadWord(&pb->matrix_ViewProj, 0x04);
	scratch[2] = Particle_RenderList_ReadWord(&pb->matrix_ViewProj, 0x08);
	scratch[3] = Particle_RenderList_ReadWord(&pb->matrix_ViewProj, 0x0c);
	*scratchR33Low = *(u16 *)(void *)((char *)&pb->matrix_ViewProj + 0x10);

	CTC2(scratch[0], 8);
	CTC2(scratch[1], 9);
	CTC2(scratch[2], 10);
	CTC2(scratch[3], 11);
	CTC2(scratch[4], 12);

	*scratchOT = pb->ptrOT;
	cameraID = (char)pb->cameraID;
	scratchCameraOffset[0] = (s32)Particle_RenderList_ReadWord(pb, 0x7c) << 2;
	scratchCameraOffset[1] = (s32)Particle_RenderList_ReadWord(pb, 0x80) << 2;
	scratchCameraOffset[2] = (s32)Particle_RenderList_ReadWord(pb, 0x84) << 2;

	if (prim + (gGT->numParticles * 10) >= (u32 *)primMem->endMin100)
		return;

	if (particle != NULL)
	{
		u32 *primCursor = prim;
		u32 *payloadCursor = primPayload;

		do
		{
			struct IconGroup *iconGroup;
			struct Icon *icon;
			struct InstDrawPerPlayer *idpp;
			u16 flagsAxis;
			u16 flagsSetColor;
			s8 driverID;
			s32 posX;
			s32 posY;
			s32 posZ;
			s32 depth;
			u32 color;

			prim = primCursor;
			driverID = (s8)particle->unk19;

			if (driverID != -1 && driverID != cameraID)
				goto next_particle;

			iconGroup = particle->ptrIconGroup;
			if (iconGroup == NULL)
				goto next_particle;

			flagsAxis = particle->flagsAxis;
			if ((flagsAxis & 0x400) == 0)
			{
				icon = particle->ptrIconArray;
			}
			else
			{
				int frame = particle->axis[10].startVal >> 8;

				if (frame < 0)
					frame = 0;

				if (iconGroup->numIcons <= frame)
					frame = iconGroup->numIcons - 1;

				if (frame < 0)
					goto next_particle;

				icon = ((struct Icon **)ICONGROUP_GETICONS(iconGroup))[frame];
				particle->ptrIconArray = icon;
			}

			if (icon == NULL)
				goto next_particle;

			idpp = NULL;
			posX = particle->axis[0].startVal >> 6;
			posY = particle->axis[1].startVal >> 6;
			posZ = particle->axis[2].startVal >> 6;
			flagsSetColor = particle->flagsSetColor;

			if ((flagsSetColor & 0x800) != 0 && particle->driverInst != NULL)
			{
				struct Instance *inst = particle->driverInst;
				u32 idppFlags;

				idpp = Particle_RenderList_GetIdpp(inst, cameraID);
				idppFlags = idpp->instFlags;

				if ((idppFlags & 0x40) == 0)
					goto next_particle;

				posX += inst->matrix.t[0] << 2;
				if ((flagsSetColor & 0x8000) == 0)
					posY += inst->matrix.t[1] << 2;
				posZ += inst->matrix.t[2] << 2;

				if ((idppFlags & 0x100) != 0)
					idpp = NULL;
			}

			posX -= scratchCameraOffset[0];
			posY -= scratchCameraOffset[1];
			posZ -= scratchCameraOffset[2];

			if (!Particle_RenderList_IsNearCamera(posX))
				goto next_particle;
			if (!Particle_RenderList_IsNearCamera(posY))
				goto next_particle;
			if (!Particle_RenderList_IsNearCamera(posZ))
				goto next_particle;

			MTC2((u32)(u16)posX | ((u32)posY << 16), 0);
			MTC2((u32)posZ, 1);
			gte_llv0_b();

			CTC2(MFC2(25), 5);
			CTC2(MFC2(26), 6);
			CTC2(MFC2(27), 7);

			depth = (s32)MFC2(27);
			if (depth < 0)
				goto next_particle;

			if ((particle->unk1A << 2) < depth)
				goto next_particle;

			color = Particle_SetColors(flagsAxis, flagsSetColor, particle);

			if ((flagsSetColor & 0x1000) != 0)
			{
				Particle_RenderList_WriteSpecialPrimitive(prim, particle, flagsAxis, flagsSetColor, color, scratch, scratchDepth);
				Particle_RenderList_LinkAndAdvance(&primCursor, &payloadCursor, particle, idpp, flagsSetColor, *scratchDepth, *scratchOT);
				prim = primCursor;
				goto next_particle;
			}

			struct ParticleRenderListMatrix matrix = Particle_RenderList_BuildNormalMatrix(particle, flagsAxis);

			Particle_RenderList_WriteNormalPrimitive(prim, icon, flagsAxis, flagsSetColor, color, &matrix, scratchDepth);
			Particle_RenderList_LinkAndAdvance(&primCursor, &payloadCursor, particle, idpp, flagsSetColor, *scratchDepth, *scratchOT);
			prim = primCursor;

		next_particle:
			particle = particle->next;
		} while (particle != NULL);
	}

	primMem->curr = prim;
}
