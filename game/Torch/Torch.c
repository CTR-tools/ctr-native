#include <common.h>

#define TORCH_RING0 0x68
#define TORCH_RING1 0x8c
#define TORCH_RING2 0xb0

struct TorchCardRegs
{
	u32 left;
	u32 right;
	u32 top;
	u32 bottom;
};

struct TorchPointSource
{
	int ring;
	int point;
};

static u32 Torch_Ptr24(const void *ptr)
{
	return CtrGpu_PrimToOTLink24(ptr);
}

static u32 Torch_ReadWord(const void *base, int offset)
{
	return *(const u32 *)(const void *)((const char *)base + offset);
}

static s32 Torch_ReadS32(const void *base, int offset)
{
	return *(const s32 *)(const void *)((const char *)base + offset);
}

static s16 Torch_ReadS16(const void *base, int offset)
{
	return *(const s16 *)(const void *)((const char *)base + offset);
}

static u16 Torch_ReadU16(const void *base, int offset)
{
	return *(const u16 *)(const void *)((const char *)base + offset);
}

static s8 Torch_ReadS8(const void *base, int offset)
{
	return *(const s8 *)(const void *)((const char *)base + offset);
}

static u8 Torch_ReadU8(const void *base, int offset)
{
	return *(const u8 *)(const void *)((const char *)base + offset);
}

static u32 Torch_ScratchReadWord(int offset)
{
	return *CTR_SCRATCHPAD_PTR(u32, offset);
}

static s16 Torch_ScratchReadS16(int offset)
{
	return *CTR_SCRATCHPAD_PTR(s16, offset);
}

static u16 Torch_ScratchReadU16(int offset)
{
	return *CTR_SCRATCHPAD_PTR(u16, offset);
}

static void Torch_ScratchWriteWord(int offset, u32 value)
{
	*CTR_SCRATCHPAD_PTR(u32, offset) = value;
}

static void Torch_ScratchWriteS16(int offset, s16 value)
{
	*CTR_SCRATCHPAD_PTR(s16, offset) = value;
}

static void Torch_ScratchWriteU16(int offset, u16 value)
{
	*CTR_SCRATCHPAD_PTR(u16, offset) = value;
}

static void Torch_ScratchWriteU8(int offset, u8 value)
{
	*CTR_SCRATCHPAD_PTR(u8, offset) = value;
}

static u32 Torch_PackXY(s32 x, s32 y)
{
	return (u32)(u16)x | ((u32)(u16)y << 16);
}

static void Torch_LoadViewAsLightMatrix(const struct PushBuffer *pb)
{
	CTC2(Torch_ReadWord(&pb->matrix_ViewProj, 0x00), 8);
	CTC2(Torch_ReadWord(&pb->matrix_ViewProj, 0x04), 9);
	CTC2(Torch_ReadWord(&pb->matrix_ViewProj, 0x08), 10);
	CTC2(Torch_ReadWord(&pb->matrix_ViewProj, 0x0c), 11);
	CTC2(Torch_ReadWord(&pb->matrix_ViewProj, 0x10), 12);
	CTC2(Torch_ReadWord(&pb->matrix_ViewProj, 0x14), 13);
	CTC2(Torch_ReadWord(&pb->matrix_ViewProj, 0x18), 14);
	CTC2(Torch_ReadWord(&pb->matrix_ViewProj, 0x1c), 15);

	CTC2(0x1000, 0);
	CTC2(0, 1);
	CTC2(0, 3);
	CTC2(0x1000, 4);
	CTC2(0, 5);
	CTC2(0, 6);
	CTC2(0, 7);
}

static void Torch_LoadRadiusVectors(u32 radius)
{
	u32 diagonal = (radius * 0xb50u) >> 12;

	MTC2(radius, 0);
	MTC2(radius << 16, 2);
	MTC2(diagonal | (diagonal << 16), 4);
}

static s32 Torch_ClampSignedCoord(s32 value, u16 max)
{
	if (value < 0)
		return 0;

	if ((s32)(value - (s32)max) >= 0)
		return max;

	return value;
}

static u16 Torch_ClampPackedCoord(int pointOffset, int axisOffset)
{
	u32 value = Torch_ScratchReadU16(pointOffset + axisOffset);
	u16 max = Torch_ScratchReadU16(0x54 + axisOffset);

	if ((s32)(value - max) >= 0)
		value = max;

	return (u16)value;
}

static void Torch_WriteUvPair(int uvOffset, int pointOffset)
{
	u16 x = Torch_ClampPackedCoord(pointOffset, 0);
	u16 y = Torch_ClampPackedCoord(pointOffset, 2);
	s32 u = (s32)x - (s32)(u8)Torch_ScratchReadU16(0x58) + Torch_ScratchReadS16(0x50);
	s32 v = (s32)y + Torch_ScratchReadS16(0x52);

	Torch_ScratchWriteU8(uvOffset, (u8)u);
	Torch_ScratchWriteU8(uvOffset + 1, (u8)v);
}

static void Torch_LinkPrimitive(u32 *tagWord, const void *packet, u_long *ot, u32 tag)
{
	*tagWord = CtrGpu_PackOTTag(*ot, tag);
	*ot = (u_long)Torch_Ptr24(packet);
}

static u32 *Torch_EmitFT3(u32 *prim, u_long *ot, struct TorchPointSource uv0, struct TorchPointSource uv1, struct TorchPointSource uv2,
                          struct TorchPointSource xy0, struct TorchPointSource xy1, struct TorchPointSource xy2)
{
	POLY_FT3 *poly = (POLY_FT3 *)prim;

	Torch_WriteUvPair(0x5c, uv0.ring + uv0.point);
	Torch_WriteUvPair(0x60, uv1.ring + uv1.point);
	Torch_WriteUvPair(0x64, uv2.ring + uv2.point);

	CtrGpu_WriteColorCode(&poly->r0, Torch_ScratchReadWord(0x44) | 0x24000000);
	CtrGpu_WritePackedXY(&poly->x0, Torch_ScratchReadWord(xy0.ring + xy0.point));
	CtrGpu_WritePackedUVWord(&poly->u0, Torch_ScratchReadWord(0x5c));
	CtrGpu_WritePackedXY(&poly->x1, Torch_ScratchReadWord(xy1.ring + xy1.point));
	CtrGpu_WritePackedUVWord(&poly->u1, Torch_ScratchReadWord(0x60));
	CtrGpu_WritePackedXY(&poly->x2, Torch_ScratchReadWord(xy2.ring + xy2.point));
	CtrGpu_WritePackedUVWord(&poly->u2, Torch_ScratchReadWord(0x64));
	Torch_LinkPrimitive(&poly->tag, poly, ot, 0x07000000);

	return (u32 *)(poly + 1);
}

static u32 *Torch_EmitFT4(u32 *prim, u_long *ot, struct TorchPointSource uv0, struct TorchPointSource uv1, struct TorchPointSource uv2,
                          struct TorchPointSource uv3, struct TorchPointSource xy0, struct TorchPointSource xy1, struct TorchPointSource xy2,
                          struct TorchPointSource xy3)
{
	POLY_FT4 *poly = (POLY_FT4 *)prim;
	u32 uv23;

	Torch_WriteUvPair(0x5c, uv0.ring + uv0.point);
	Torch_WriteUvPair(0x60, uv1.ring + uv1.point);
	Torch_WriteUvPair(0x64, uv2.ring + uv2.point);
	Torch_WriteUvPair(0x66, uv3.ring + uv3.point);
	uv23 = Torch_ScratchReadWord(0x64);

	CtrGpu_WriteColorCode(&poly->r0, Torch_ScratchReadWord(0x44) | 0x2c000000);
	CtrGpu_WritePackedXY(&poly->x0, Torch_ScratchReadWord(xy0.ring + xy0.point));
	CtrGpu_WritePackedUVWord(&poly->u0, Torch_ScratchReadWord(0x5c));
	CtrGpu_WritePackedXY(&poly->x1, Torch_ScratchReadWord(xy1.ring + xy1.point));
	CtrGpu_WritePackedUVWord(&poly->u1, Torch_ScratchReadWord(0x60));
	CtrGpu_WritePackedXY(&poly->x2, Torch_ScratchReadWord(xy2.ring + xy2.point));
	CtrGpu_WritePackedUVWord(&poly->u2, uv23);
	CtrGpu_WritePackedXY(&poly->x3, Torch_ScratchReadWord(xy3.ring + xy3.point));
	CtrGpu_WritePackedUVWord(&poly->u3, uv23 >> 16);
	Torch_LinkPrimitive(&poly->tag, poly, ot, 0x09000000);

	return (u32 *)(poly + 1);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004b914-0x8004b94c
static struct TorchCardRegs Torch_Subset1_BuildCard(s32 centerX, s32 centerY, u32 sxy0, u32 sxy1)
{
	struct TorchCardRegs regs;
	s32 xRadius = (u16)sxy0;
	s32 yRadius = (s32)sxy1 >> 16;
	u32 packedCenterY = (u32)centerY << 16;

	regs.left = ((u32)(centerX - xRadius) & 0xffff) | packedCenterY;
	regs.right = ((u32)(centerX + xRadius) & 0xffff) | packedCenterY;
	regs.top = ((u32)(centerY - yRadius) << 16) | (u32)(u16)centerX;
	regs.bottom = ((u32)(centerY + yRadius) << 16) | (u32)(u16)centerX;

	return regs;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004b94c-0x8004b9cc
static void Torch_Subset2_StoreCard(const struct TorchCardRegs *regs, s32 centerX, s32 centerY, int ringOffset)
{
	u32 sxy2 = MFC2(14);
	s32 radiusX = (u16)sxy2;
	s32 radiusY = (s32)sxy2 >> 16;

	Torch_ScratchWriteWord(ringOffset + 0x1c, regs->left);
	Torch_ScratchWriteWord(ringOffset + 0x0c, regs->right);
	Torch_ScratchWriteWord(ringOffset + 0x04, regs->top);
	Torch_ScratchWriteWord(ringOffset + 0x14, regs->bottom);
	Torch_ScratchWriteWord(ringOffset + 0x20, Torch_PackXY(centerX - radiusX, centerY - radiusY));
	Torch_ScratchWriteWord(ringOffset + 0x08, Torch_PackXY(centerX + radiusX, centerY - radiusY));
	Torch_ScratchWriteWord(ringOffset + 0x18, Torch_PackXY(centerX - radiusX, centerY + radiusY));
	Torch_ScratchWriteWord(ringOffset + 0x10, Torch_PackXY(centerX + radiusX, centerY + radiusY));
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004b9cc-0x8004ba4c
static void Torch_Subset3_SetTpage(s32 x, s32 y)
{
	u16 maxX = Torch_ScratchReadU16(0x54);
	u16 maxY = Torch_ScratchReadU16(0x56);
	u32 tile;

	x = Torch_ClampSignedCoord(x, maxX);
	x += Torch_ScratchReadS16(0x50);

	y = Torch_ClampSignedCoord(y, maxY);
	y += Torch_ScratchReadS16(0x52);

	tile = ((u32)x & 0x3ff) >> 6;
	Torch_ScratchWriteU16(0x58, (u16)(tile << 6));
	Torch_ScratchWriteU16(0x62, (u16)(tile | (((u32)y & 0x100) >> 4) | 0x100));
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004ba4c-0x8004bbe8
static u32 *Torch_Subset4_EmitFT3(u32 *prim, u_long *ot, int pointA, int pointB)
{
	return Torch_EmitFT3(prim, ot, (struct TorchPointSource){TORCH_RING0, 0}, (struct TorchPointSource){TORCH_RING2, pointA},
	                     (struct TorchPointSource){TORCH_RING2, pointB}, (struct TorchPointSource){TORCH_RING0, 0},
	                     (struct TorchPointSource){TORCH_RING1, pointA}, (struct TorchPointSource){TORCH_RING1, pointB});
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004bbe8-0x8004bd84
static u32 *Torch_Subset5_EmitFT3(u32 *prim, u_long *ot, int pointA, int pointB)
{
	return Torch_EmitFT3(prim, ot, (struct TorchPointSource){TORCH_RING0, 0}, (struct TorchPointSource){TORCH_RING2, pointA},
	                     (struct TorchPointSource){TORCH_RING1, pointB}, (struct TorchPointSource){TORCH_RING0, 0},
	                     (struct TorchPointSource){TORCH_RING1, pointA}, (struct TorchPointSource){TORCH_RING2, pointB});
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004bd84-0x8004bf20
static u32 *Torch_Subset6_EmitFT3(u32 *prim, u_long *ot, int pointA, int pointB)
{
	return Torch_EmitFT3(prim, ot, (struct TorchPointSource){TORCH_RING0, 0}, (struct TorchPointSource){TORCH_RING1, pointA},
	                     (struct TorchPointSource){TORCH_RING1, pointB}, (struct TorchPointSource){TORCH_RING0, 0},
	                     (struct TorchPointSource){TORCH_RING2, pointA}, (struct TorchPointSource){TORCH_RING2, pointB});
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004bf20-0x8004c134
static u32 *Torch_Subset7_EmitFT4(u32 *prim, u_long *ot, int pointA, int pointB)
{
	return Torch_EmitFT4(prim, ot, (struct TorchPointSource){TORCH_RING0, pointA}, (struct TorchPointSource){TORCH_RING0, pointB},
	                     (struct TorchPointSource){TORCH_RING2, pointA}, (struct TorchPointSource){TORCH_RING2, pointB},
	                     (struct TorchPointSource){TORCH_RING0, pointA}, (struct TorchPointSource){TORCH_RING0, pointB},
	                     (struct TorchPointSource){TORCH_RING1, pointA}, (struct TorchPointSource){TORCH_RING1, pointB});
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004c134-0x8004c348
static u32 *Torch_Subset8_EmitFT4(u32 *prim, u_long *ot, int pointA, int pointB)
{
	return Torch_EmitFT4(prim, ot, (struct TorchPointSource){TORCH_RING0, pointA}, (struct TorchPointSource){TORCH_RING0, pointB},
	                     (struct TorchPointSource){TORCH_RING2, pointA}, (struct TorchPointSource){TORCH_RING1, pointB},
	                     (struct TorchPointSource){TORCH_RING0, pointA}, (struct TorchPointSource){TORCH_RING0, pointB},
	                     (struct TorchPointSource){TORCH_RING1, pointA}, (struct TorchPointSource){TORCH_RING2, pointB});
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004c348-0x8004c55c
static u32 *Torch_Subset9_EmitFT4(u32 *prim, u_long *ot, int pointA, int pointB)
{
	return Torch_EmitFT4(prim, ot, (struct TorchPointSource){TORCH_RING1, pointA}, (struct TorchPointSource){TORCH_RING1, pointB},
	                     (struct TorchPointSource){TORCH_RING0, pointA}, (struct TorchPointSource){TORCH_RING0, pointB},
	                     (struct TorchPointSource){TORCH_RING2, pointA}, (struct TorchPointSource){TORCH_RING2, pointB},
	                     (struct TorchPointSource){TORCH_RING0, pointA}, (struct TorchPointSource){TORCH_RING0, pointB});
}

static int Torch_IsCardVisible(const struct TorchCardRegs *regs, u32 screenSize)
{
	u32 bounds;

	if ((((s32)regs->top >> 16) - 0x18) <= 0)
		return 0;

	bounds = ~((regs->left - screenSize) | (regs->right - screenSize) | (regs->top - screenSize) | (regs->bottom - screenSize)) |
	         (regs->left & regs->right & regs->top & regs->bottom);

	if ((s32)bounds < 0)
		return 0;

	return (s32)(bounds << 16) >= 0;
}

static u32 *Torch_EmitParticle(u32 *prim, u_long *ot)
{
	Torch_Subset3_SetTpage(Torch_ScratchReadS16(0x6c), Torch_ScratchReadS16(0x6e));
	prim = Torch_Subset6_EmitFT3(prim, ot, 0x08, 0x04);
	prim = Torch_Subset9_EmitFT4(prim, ot, 0x04, 0x08);

	Torch_Subset3_SetTpage(Torch_ScratchReadS16(0x68), Torch_ScratchReadS16(0x72));
	prim = Torch_Subset6_EmitFT3(prim, ot, 0x08, 0x0c);
	prim = Torch_Subset9_EmitFT4(prim, ot, 0x0c, 0x08);

	Torch_Subset3_SetTpage(Torch_ScratchReadS16(0x68), Torch_ScratchReadS16(0x6a));
	prim = Torch_Subset6_EmitFT3(prim, ot, 0x0c, 0x10);
	prim = Torch_Subset9_EmitFT4(prim, ot, 0x0c, 0x10);
	prim = Torch_Subset6_EmitFT3(prim, ot, 0x10, 0x14);
	prim = Torch_Subset9_EmitFT4(prim, ot, 0x14, 0x10);

	Torch_Subset3_SetTpage(Torch_ScratchReadS16(0x80), Torch_ScratchReadS16(0x6a));
	prim = Torch_Subset5_EmitFT3(prim, ot, 0x18, 0x14);
	prim = Torch_Subset8_EmitFT4(prim, ot, 0x18, 0x14);

	Torch_Subset3_SetTpage(Torch_ScratchReadS16(0x84), Torch_ScratchReadS16(0x86));
	prim = Torch_Subset4_EmitFT3(prim, ot, 0x1c, 0x18);
	prim = Torch_Subset7_EmitFT4(prim, ot, 0x1c, 0x18);

	Torch_Subset3_SetTpage(Torch_ScratchReadS16(0x84), Torch_ScratchReadS16(0x8a));
	prim = Torch_Subset4_EmitFT3(prim, ot, 0x20, 0x1c);
	prim = Torch_Subset7_EmitFT4(prim, ot, 0x20, 0x1c);

	Torch_Subset3_SetTpage(Torch_ScratchReadS16(0x88), Torch_ScratchReadS16(0x6e));
	prim = Torch_Subset5_EmitFT3(prim, ot, 0x20, 0x04);
	prim = Torch_Subset8_EmitFT4(prim, ot, 0x20, 0x04);

	return prim;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004b470-0x8004b914
void Torch_Main(void *particleList_heatWarp, struct PushBuffer *pb, struct PrimMem *primMem, char numPlyr, int swapchainIndex)
{
	struct Particle *firstParticle = particleList_heatWarp;
	u32 *prim = (u32 *)primMem->cursor;

	// NOTE(aalhendi): PSX-backfeed blocker: retail saves callee registers and pointer cursors in scratchpad 0x00-0x38.
	// Native C keeps host-width pointers as locals, while preserving retail data temporaries from 0x44 up.
	if (firstParticle != NULL)
	{
		int playerPassesLeft = (s32)(s8)numPlyr - 1;
		int particlesLeft = 12;

		Torch_ScratchWriteWord(0x30, (u32)(uintptr_t)firstParticle);
		Torch_ScratchWriteWord(0x38, (u32)swapchainIndex);
		Torch_ScratchWriteU16(0x5e, 0);

		while (1)
		{
			struct Particle *particle;
			u32 screenSize;
			u_long *otBase;

			Torch_LoadViewAsLightMatrix(pb);

			CTC2((u32)(s32)pb->rect.w << 15, 24);
			CTC2((u32)(s32)pb->rect.h << 15, 25);
			CTC2((u32)pb->distanceToScreen_PREV, 26);

			Torch_ScratchWriteWord(0x48, (u32)(s32)pb->rect.w << 15);
			Torch_ScratchWriteWord(0x4c, (u32)(s32)pb->rect.h << 15);
			Torch_ScratchWriteS16(0x50, pb->rect.x);
			Torch_ScratchWriteS16(0x52, (s16)(pb->rect.y + Torch_ScratchReadWord(0x38)));
			Torch_ScratchWriteS16(0x54, (s16)(pb->rect.w - 1));
			Torch_ScratchWriteS16(0x56, (s16)(pb->rect.h - 1));

			screenSize = Torch_ReadWord(pb, 0x20);
			otBase = pb->ptrOT;
			particle = firstParticle;

			while (particle != NULL)
			{
				u32 color;
				u32 radius0;
				u32 radius1;
				u32 radius2;
				s32 viewZ;
				u32 centerSxy;
				u32 gteFlag;
				s32 centerX;
				s32 centerY;
				struct TorchCardRegs card;

				MTC2(Torch_PackXY(Torch_ReadS32(particle, 0x24) >> 8, Torch_ReadS32(particle, 0x2c) >> 8), 0);
				MTC2((u32)(Torch_ReadS32(particle, 0x34) >> 8), 1);

				radius0 = Torch_ReadU8(particle, 0x3d);
				radius1 = Torch_ReadU8(particle, 0x45);
				radius2 = Torch_ReadU8(particle, 0x4d);

				gte_llv0bk_b();
				color = (u32)Torch_ReadU8(particle, 0x5d) | ((u32)Torch_ReadU8(particle, 0x65) << 8) | ((u32)Torch_ReadU8(particle, 0x6d) << 16);
				Torch_ScratchWriteWord(0x44, color);

				viewZ = (s32)MFC2(27);
				MTC2((u32)viewZ, 1);
				MTC2((u32)viewZ, 3);
				MTC2((u32)viewZ, 5);
				CTC2(0x1000, 2);
				CTC2(Torch_ScratchReadWord(0x48), 24);
				CTC2(Torch_ScratchReadWord(0x4c), 25);
				MTC2(Torch_PackXY((u16)MFC2(25), MFC2(26)), 0);

				gte_rtps_b();

				centerSxy = MFC2(14);
				gteFlag = CFC2(31);
				centerX = (u16)centerSxy;
				centerY = (s32)centerSxy >> 16;

				if ((s32)(gteFlag << 14) >= 0)
				{
					u32 sxy0;
					u32 sxy1;

					Torch_ScratchWriteWord(TORCH_RING0, centerSxy);
					CTC2(0x0a00, 2);
					CTC2(0, 24);
					CTC2(0, 25);
					Torch_LoadRadiusVectors(radius0);
					gte_rtpt_b();

					sxy0 = MFC2(12);
					sxy1 = MFC2(13);

					if ((s32)(sxy0 - 0x80) < 0)
					{
						Torch_LoadRadiusVectors(radius1);
						card = Torch_Subset1_BuildCard(centerX, centerY, sxy0, sxy1);

						if (Torch_IsCardVisible(&card, screenSize))
						{
							s32 otIndex;
							u_long *ot;

							Torch_Subset2_StoreCard(&card, centerX, centerY, TORCH_RING0);
							gte_rtpt_b();

							sxy0 = MFC2(12);
							sxy1 = MFC2(13);
							Torch_LoadRadiusVectors(radius2);
							card = Torch_Subset1_BuildCard(centerX, centerY, sxy0, sxy1);
							Torch_Subset2_StoreCard(&card, centerX, centerY, TORCH_RING1);
							gte_rtpt_b();

							otIndex = (viewZ >> 6) + Torch_ReadS8(particle, 0x18);
							if (otIndex < 0)
								otIndex = 0;
							else if ((otIndex - 0x400) >= 0)
								otIndex = 0x3ff;

							ot = (u_long *)(void *)((char *)otBase + (otIndex << 2));
							sxy0 = MFC2(12);
							sxy1 = MFC2(13);
							card = Torch_Subset1_BuildCard(centerX, centerY, sxy0, sxy1);
							Torch_Subset2_StoreCard(&card, centerX, centerY, TORCH_RING2);
							prim = Torch_EmitParticle(prim, ot);
						}
					}
				}

				particlesLeft--;
				particle = particle->next;
				if (particlesLeft < 1)
					goto done;
			}

			if (playerPassesLeft <= 0)
				break;

			playerPassesLeft--;
			pb++;
		}
	}

done:
	primMem->cursor = prim;
}
