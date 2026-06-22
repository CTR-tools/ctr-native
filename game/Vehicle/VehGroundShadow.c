#include <common.h>

/// @brief Copies texture layout data from icon to arbitrary mem address. Particularly used to copy kart shadow textures to scratchpad.
/// @param dst - destination texture layout
/// @param iconIndex - icon index to take data from
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005b6b8-0x8005b720.
bool VehGroundShadow_Subset1(struct TextureLayout *pDst, int iconIndex)
{
	// get pointer to icon
	struct Icon *pIcon = sdata->gGT->ptrIcons[iconIndex];

	// validate icon pointer
	if (!pIcon)
		return false;

	// copy entire struct
	memcpy(pDst, &(pIcon->texLayout), sizeof(pIcon->texLayout));

	// fix blending mode
	pDst->tpage = pDst->tpage & 0xff9f | 0x40;

	return true;
}

#define VEH_GROUND_SHADOW_MAX_DRIVERS 8
#define VEH_GROUND_SHADOW_MAX_PLAYERS 4
#define VEH_GROUND_SHADOW_NUM_POINTS  9
#define VEH_GROUND_SHADOW_NUM_QUADS   4

struct VehGroundShadowEntry
{
	s16 local[3][3];
	s8 state;
	s8 depthBias;
	struct Driver *driver;
	struct Instance *inst;
	u8 idppFlags[VEH_GROUND_SHADOW_MAX_PLAYERS];
	SVec3 pos;
	u16 instFlags;
};

static u32 VehGroundShadow_Ptr24(const void *ptr)
{
	return CtrGpu_PrimToOTLink24(ptr);
}

static u32 VehGroundShadow_ReadWord(const void *base, int offset)
{
	u32 value;
	memcpy(&value, (const char *)base + offset, sizeof(value));
	return value;
}

static u16 VehGroundShadow_ReadHalf(const void *base, int offset)
{
	u16 value;
	memcpy(&value, (const char *)base + offset, sizeof(value));
	return value;
}

static u32 VehGroundShadow_PackXY(s32 x, s32 y)
{
	return (u32)(u16)x | ((u32)(u16)y << 16);
}

static s32 VehGroundShadow_DiffS16(s32 lhs, s32 rhs)
{
	return (s16)((u16)lhs - (u16)rhs);
}

static struct InstDrawPerPlayer *VehGroundShadow_GetIdpp(struct Instance *inst, int playerIndex)
{
	return (struct InstDrawPerPlayer *)((char *)INST_GETIDPP(inst) + (playerIndex * sizeof(struct InstDrawPerPlayer)));
}

static void VehGroundShadow_LoadGteRotMatrix(const MATRIX *m)
{
	CTC2(VehGroundShadow_ReadWord(m, 0x00), 0);
	CTC2(VehGroundShadow_ReadWord(m, 0x04), 1);
	CTC2(VehGroundShadow_ReadWord(m, 0x08), 2);
	CTC2(VehGroundShadow_ReadWord(m, 0x0c), 3);
	CTC2(VehGroundShadow_ReadWord(m, 0x10), 4);
}

static void VehGroundShadow_LoadGteLightMatrix(const MATRIX *m)
{
	CTC2(VehGroundShadow_ReadWord(m, 0x00), 8);
	CTC2(VehGroundShadow_ReadWord(m, 0x04), 9);
	CTC2(VehGroundShadow_ReadWord(m, 0x08), 10);
	CTC2(VehGroundShadow_ReadWord(m, 0x0c), 11);
	CTC2(VehGroundShadow_ReadWord(m, 0x10), 12);
}

static void VehGroundShadow_LoadV0(s32 x, s32 y, s32 z)
{
	MTC2(VehGroundShadow_PackXY(x, y), 0);
	MTC2((u32)z, 1);
}

static void VehGroundShadow_LoadRtptVectors(s16 points[VEH_GROUND_SHADOW_NUM_POINTS][3], int start)
{
	MTC2(VehGroundShadow_PackXY(points[start + 0][0], points[start + 0][1]), 0);
	MTC2((u32)(s32)points[start + 0][2], 1);
	MTC2(VehGroundShadow_PackXY(points[start + 1][0], points[start + 1][1]), 2);
	MTC2((u32)(s32)points[start + 1][2], 3);
	MTC2(VehGroundShadow_PackXY(points[start + 2][0], points[start + 2][1]), 4);
	MTC2((u32)(s32)points[start + 2][2], 5);
}

static void VehGroundShadow_ProjectPoints(s16 points[VEH_GROUND_SHADOW_NUM_POINTS][3], u32 sxy[VEH_GROUND_SHADOW_NUM_POINTS],
                                          s32 depth[VEH_GROUND_SHADOW_NUM_POINTS])
{
	for (int i = 0; i < VEH_GROUND_SHADOW_NUM_POINTS; i += 3)
	{
		VehGroundShadow_LoadRtptVectors(points, i);
		gte_rtpt_b();

		sxy[i + 0] = MFC2(12);
		sxy[i + 1] = MFC2(13);
		sxy[i + 2] = MFC2(14);
		depth[i + 0] = (s32)MFC2(17);
		depth[i + 1] = (s32)MFC2(18);
		depth[i + 2] = (s32)MFC2(19);
	}
}

static void VehGroundShadow_BuildEntry(struct VehGroundShadowEntry *entry, struct Driver *driver, int numPlayers)
{
	struct Instance *inst = driver->instSelf;

	memset(entry, 0, sizeof(*entry));
	entry->driver = driver;
	entry->inst = inst;
	entry->instFlags = (u16)inst->flags;

	for (int playerIndex = numPlayers - 1; playerIndex >= 0; playerIndex--)
		entry->idppFlags[playerIndex] = (u8)VehGroundShadow_GetIdpp(inst, playerIndex)->instFlags;

	entry->pos.x = (s16)CTR_MipsSra(driver->posCurr.x, 8);
	entry->pos.y = (s16)CTR_MipsAddLo(CTR_MipsSra(driver->quadBlockHeight, 8), 3);
	entry->pos.z = (s16)CTR_MipsSra(driver->posCurr.z, 8);
	entry->depthBias = (s8)((entry->instFlags & SPLIT_LINE) != 0 ? inst->depthBiasSecondary : inst->depthBiasNormal) + 1;
}

static void VehGroundShadow_TransformLocalAxes(struct VehGroundShadowEntry *entry)
{
	struct Driver *driver = entry->driver;
	MATRIX axisMatrix;
	s16 local[4];
	int height = CTR_MipsSubLo(0x100, CTR_MipsSra(CTR_MipsSubLo(driver->posCurr.y, driver->quadBlockHeight), 8));
	int localX;
	int localZ0;
	int localZ1;

	if (height < 1 || height >= 0x10a)
	{
		entry->state = -1;
		return;
	}

	if (height > 0x100)
		height = 0x100;

	VehPhysForce_RotAxisAngle(&axisMatrix, driver->AxisAngle3_normalVec.v, driver->rotCurr.y);
	VehGroundShadow_LoadGteLightMatrix(&axisMatrix);

	localX = (height * 0x28) >> 6;
	localZ0 = (height * 0x29) >> 6;
	localZ1 = (height * 0x34) >> 6;
	local[0] = (s16)localX;
	local[1] = 0;
	local[2] = 0;
	local[3] = 0;

	for (int i = 0; i < 3; i++)
	{
		CTR_GteLoadS16TripletV0(local);
		gte_llv0_b();

		if (i == 0)
		{
			local[0] = 0;
			local[2] = (s16)localZ0;
		}
		else if (i == 1)
		{
			local[2] = (s16)localZ1;
		}

		entry->local[i][0] = (s16)MFC2(25);
		entry->local[i][1] = (s16)MFC2(26);
		entry->local[i][2] = (s16)MFC2(27);
	}

	entry->state = 1;
}

static void VehGroundShadow_BuildProjectionPoints(struct VehGroundShadowEntry *entry, s32 baseX, s32 baseY, s32 baseZ,
                                                  s16 points[VEH_GROUND_SHADOW_NUM_POINTS][3])
{
	s16(*local)[3] = entry->local;

	points[0][0] = (s16)baseX;
	points[0][1] = (s16)baseY;
	points[0][2] = (s16)baseZ;

	points[1][0] = (s16)((baseX - local[0][0]) - local[1][0]);
	points[1][1] = (s16)((baseY - local[0][1]) - local[1][1]);
	points[1][2] = (s16)((baseZ - local[0][2]) - local[1][2]);

	points[2][0] = (s16)(baseX - local[1][0]);
	points[2][1] = (s16)(baseY - local[1][1]);
	points[2][2] = (s16)(baseZ - local[1][2]);

	points[3][0] = (s16)((baseX + local[0][0]) - local[1][0]);
	points[3][1] = (s16)((baseY + local[0][1]) - local[1][1]);
	points[3][2] = (s16)((baseZ + local[0][2]) - local[1][2]);

	points[4][0] = (s16)(baseX + local[0][0]);
	points[4][1] = (s16)(baseY + local[0][1]);
	points[4][2] = (s16)(baseZ + local[0][2]);

	points[5][0] = (s16)(baseX + local[2][0] + local[0][0]);
	points[5][1] = (s16)(baseY + local[2][1] + local[0][1]);
	points[5][2] = (s16)(baseZ + local[2][2] + local[0][2]);

	points[6][0] = (s16)(baseX + local[2][0]);
	points[6][1] = (s16)(baseY + local[2][1]);
	points[6][2] = (s16)(baseZ + local[2][2]);

	points[7][0] = (s16)(baseX + local[2][0] - local[0][0]);
	points[7][1] = (s16)(baseY + local[2][1] - local[0][1]);
	points[7][2] = (s16)(baseZ + local[2][2] - local[0][2]);

	points[8][0] = (s16)(baseX - local[0][0]);
	points[8][1] = (s16)(baseY - local[0][1]);
	points[8][2] = (s16)(baseZ - local[0][2]);
}

static void VehGroundShadow_WriteUv(POLY_FT4 *poly, const struct TextureLayout *tex)
{
	CtrGpu_WritePackedUVWord(&poly->u0, VehGroundShadow_ReadWord(tex, 0x00));
	CtrGpu_WritePackedUVWord(&poly->u1, VehGroundShadow_ReadWord(tex, 0x04));
	CtrGpu_WritePackedUV(&poly->u2, VehGroundShadow_ReadHalf(tex, 0x08));
	CtrGpu_WritePackedUV(&poly->u3, VehGroundShadow_ReadHalf(tex, 0x0a));
}

static void VehGroundShadow_EmitQuad(u32 **primCursor, u_long *otBase, const struct TextureLayout *texture, u32 color, u32 sxy[VEH_GROUND_SHADOW_NUM_POINTS],
                                     s32 depth, s8 depthBias, int quadIndex)
{
	static const u8 quadPointIndex[VEH_GROUND_SHADOW_NUM_QUADS][4] = {
	    {8, 0, 1, 2},
	    {8, 0, 7, 6},
	    {4, 0, 3, 2},
	    {4, 0, 5, 6},
	};
	POLY_FT4 *poly = (POLY_FT4 *)*primCursor;
	int depthIndex = (depth >> 8) + depthBias;
	u_long *ot;

	if (depthIndex < 0)
		depthIndex = 0;
	else if (depthIndex > 0x3ff)
		depthIndex = 0x3ff;

	CtrGpu_WriteColorCode(&poly->r0, color);
	CtrGpu_WritePackedXY(&poly->x0, sxy[quadPointIndex[quadIndex][0]]);
	CtrGpu_WritePackedXY(&poly->x1, sxy[quadPointIndex[quadIndex][1]]);
	CtrGpu_WritePackedXY(&poly->x2, sxy[quadPointIndex[quadIndex][2]]);
	CtrGpu_WritePackedXY(&poly->x3, sxy[quadPointIndex[quadIndex][3]]);
	VehGroundShadow_WriteUv(poly, texture);

	ot = &otBase[depthIndex];
	poly->tag = CtrGpu_PackOTTag(*ot, 0x09000000);
	*ot = (u_long)VehGroundShadow_Ptr24(poly);

	*primCursor = (u32 *)(poly + 1);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005b720-0x8005c120.
void VehGroundShadow_Main(void)
{
	struct GameTracker *gGT = sdata->gGT;
	struct PrimMem *primMem;
	u32 *prim;
	struct TextureLayout *shadowTex0 = CTR_SCRATCHPAD_PTR(struct TextureLayout, 0x224);
	struct TextureLayout *shadowTex1 = CTR_SCRATCHPAD_PTR(struct TextureLayout, 0x230);
	struct VehGroundShadowEntry entries[VEH_GROUND_SHADOW_MAX_DRIVERS + 1];
	int numPlayers;

	// NOTE(aalhendi): PSX-backfeed blocker: retail stages driver pointers, instance pointers, and per-driver shadow state in scratchpad
	// 0x1f8000a4-0x1f800208. Native C keeps pointer-bearing entries as host locals; restore the exact scratchpad layout before PSX backfeed.
	if (!VehGroundShadow_Subset1(shadowTex0, 0))
		return;

	if (!VehGroundShadow_Subset1(shadowTex1, 1))
		return;

	primMem = &gGT->backBuffer->primMem;
	prim = (u32 *)primMem->cursor;
	if (prim + 0x140 >= (u32 *)primMem->guardEnd)
		return;

	CTC2(0, 5);
	CTC2(0, 6);
	CTC2(0, 7);

	numPlayers = gGT->numPlyrCurrGame;
	memset(entries, 0, sizeof(entries));

	for (int driverIndex = 0; driverIndex < VEH_GROUND_SHADOW_MAX_DRIVERS; driverIndex++)
	{
		struct Driver *driver = gGT->drivers[driverIndex];

		if (driver != NULL)
			VehGroundShadow_BuildEntry(&entries[driverIndex], driver, numPlayers);
	}

	for (int playerIndex = numPlayers - 1; playerIndex >= 0; playerIndex--)
	{
		struct PushBuffer *pb = &gGT->pushBuffer[playerIndex];
		s32 camX = pb->matrix_Camera.t[0];
		s32 camY = pb->matrix_Camera.t[1];
		s32 camZ = pb->matrix_Camera.t[2];
		u_long *otBase = pb->ptrOT;
		int isLargeGeomScreen;

		CTC2((u32)(s32)pb->rect.w << 15, 24);
		CTC2((u32)(s32)pb->rect.h << 15, 25);
		CTC2((u32)pb->distanceToScreen_PREV, 26);
		VehGroundShadow_LoadGteRotMatrix(&pb->matrix_ViewProj);
		isLargeGeomScreen = pb->distanceToScreen_PREV > 0x100;

		for (int driverIndex = 0; driverIndex < VEH_GROUND_SHADOW_MAX_DRIVERS; driverIndex++)
		{
			struct VehGroundShadowEntry *entry = &entries[driverIndex];
			s32 diffX;
			s32 diffY;
			s32 diffZ;
			s32 scaledX;
			s32 scaledY;
			s32 scaledZ;
			s16 scaled[4];
			s32 groundDistance;
			u32 color;
			s16 points[VEH_GROUND_SHADOW_NUM_POINTS][3];
			u32 sxy[VEH_GROUND_SHADOW_NUM_POINTS];
			s32 depth[VEH_GROUND_SHADOW_NUM_POINTS];

			if (entry->driver == NULL)
				break;

			if (entry->state == -1)
				continue;

			if ((entry->instFlags & HIDE_MODEL) != 0)
			{
				entry->state = -1;
				continue;
			}

			if ((entry->idppFlags[playerIndex] & DRAW_SUCCESSFUL) == 0)
				continue;

			diffX = VehGroundShadow_DiffS16(entry->pos.x, camX);
			diffY = VehGroundShadow_DiffS16(entry->pos.y, camY);
			diffZ = VehGroundShadow_DiffS16(entry->pos.z, camZ);
			scaledX = (s16)(diffX * 4);
			scaledY = (s16)(diffY * 4);
			scaledZ = (s16)(diffZ * 4);

			if (!isLargeGeomScreen)
			{
				if (scaledX >= 0x1771 || scaledY >= 0x1771 || scaledZ >= 0x1771 || scaledX < -0x1770 || scaledY < -0x1770 || scaledZ < -0x1770)
					continue;
			}

			scaled[0] = (s16)scaledX;
			scaled[1] = (s16)scaledY;
			scaled[2] = (s16)scaledZ;
			scaled[3] = 0;
			CTR_GteLoadS16TripletV0(scaled);
			gte_rtv0_b();
			groundDistance = (s32)MFC2(27) >> 2;

			if (groundDistance < -0x34)
				continue;

			if (isLargeGeomScreen)
			{
				color = 0x2e1f1f1f;
			}
			else
			{
				s32 intensity;

				if (groundDistance < 0x180)
				{
					intensity = 0x1f;
				}
				else
				{
					int fade = (0x200 - groundDistance) * 0x1f;

					if (fade < 0)
						fade += 0x7f;

					intensity = fade >> 7;
					if (intensity < 1)
						continue;
				}

				color = 0x2e000000u | (u32)intensity | ((u32)intensity << 8) | ((u32)intensity << 16);
			}

			if (entry->state == 0)
			{
				VehGroundShadow_TransformLocalAxes(entry);
				VehGroundShadow_LoadGteRotMatrix(&pb->matrix_ViewProj);

				if (entry->state == -1)
					continue;
			}

			VehGroundShadow_BuildProjectionPoints(entry, scaledX, scaledY, scaledZ, points);
			VehGroundShadow_ProjectPoints(points, sxy, depth);

			for (int quadIndex = 0; quadIndex < VEH_GROUND_SHADOW_NUM_QUADS; quadIndex++)
			{
				const struct TextureLayout *texture = (quadIndex & 1) != 0 ? shadowTex1 : shadowTex0;

				VehGroundShadow_EmitQuad(&prim, otBase, texture, color, sxy, depth[0], entry->depthBias, quadIndex);
			}
		}
	}

	primMem->cursor = prim;
}
