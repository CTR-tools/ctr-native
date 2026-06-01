#include <common.h>

#define VEH_WARP_DUST_SEGMENTS 16

struct VehWarpDustProjected
{
	u32 sxy0;
	u32 sxy1;
	u32 sxy2;
	u32 depth;
};

static s16 VehWarpDust_AddHalf(s16 value, int delta)
{
	return (s16)((u16)value + delta);
}

static u32 VehWarpDust_Ptr24(const void *ptr)
{
	return CtrGpu_PrimToOTLink24(ptr);
}

static void VehWarpDust_Project(SVECTOR *point, int offsetX, int offsetY, int offsetZ, struct VehWarpDustProjected *out)
{
	SVECTOR *left = CTR_SCRATCHPAD_PTR(SVECTOR, 0x190);
	SVECTOR *right = CTR_SCRATCHPAD_PTR(SVECTOR, 0x198);

	left->vx = VehWarpDust_AddHalf(point->vx, offsetX);
	left->vy = VehWarpDust_AddHalf(point->vy, offsetY);
	left->vz = VehWarpDust_AddHalf(point->vz, offsetZ);

	right->vx = VehWarpDust_AddHalf(point->vx, -offsetX);
	right->vy = VehWarpDust_AddHalf(point->vy, -offsetY);
	right->vz = VehWarpDust_AddHalf(point->vz, -offsetZ);

	gte_ldv3(left, point, right);
	gte_rtpt_b();

	out->sxy0 = MFC2(12);
	out->sxy1 = MFC2(13);
	out->sxy2 = MFC2(14);
	out->depth = MFC2(17);
}

static void VehWarpDust_EmitSegment(u32 **primCursor, struct PushBuffer *pb, const struct VehWarpDustProjected *prev, const struct VehWarpDustProjected *curr)
{
	u32 *prim = *primCursor;
	u_long *ot = pb->ptrOT + ((s32)curr->depth >> 6);

	prim[1] = 0xe1000a20;
	prim[2] = 0x3a000000;
	prim[3] = curr->sxy0;
	prim[4] = 0x007f1f3f;
	prim[5] = curr->sxy1;
	prim[6] = 0;
	prim[7] = prev->sxy0;
	prim[8] = 0x007f1f3f;
	prim[9] = prev->sxy1;
	prim[10] = 0x3a000000;
	prim[11] = curr->sxy2;
	prim[12] = 0x007f1f3f;
	prim[13] = curr->sxy1;
	prim[14] = 0;
	prim[15] = prev->sxy2;
	prim[16] = 0x007f1f3f;
	prim[17] = prev->sxy1;

	prim[0] = (u32)*ot | 0x11000000;
	*ot = (u_long)VehWarpDust_Ptr24(prim);
	*primCursor = prim + 18;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80068644-0x80068be8.
void VehStuckProc_Warp_AddDustPuff2(struct Driver *d, int *warp)
{
	struct GameTracker *gGT = sdata->gGT;
	struct PushBuffer *pb = &gGT->pushBuffer[d->driverID];
	struct DB *backBuffer = gGT->backBuffer;
	u32 *prim = backBuffer->primMem.curr;
	SVECTOR *points = CTR_SCRATCHPAD_PTR(SVECTOR, 0x108);
	SVECTOR *endpoint = &points[VEH_WARP_DUST_SEGMENTS];
	s16 *jitterScale = CTR_SCRATCHPAD_PTR(s16, 0x1c0);
	int offsetX;
	int offsetY;
	int offsetZ;

	gte_SetRotMatrix(&pb->matrix_ViewProj);
	gte_SetTransMatrix(&pb->matrix_ViewProj);

	jitterScale[0] = (pb->matrix_CameraTranspose.m[0][0] + pb->matrix_CameraTranspose.m[0][1]) >> 5;
	jitterScale[1] = (pb->matrix_CameraTranspose.m[1][0] + pb->matrix_CameraTranspose.m[1][1]) >> 5;
	jitterScale[2] = (pb->matrix_CameraTranspose.m[2][0] + pb->matrix_CameraTranspose.m[2][1]) >> 5;

	offsetX = pb->matrix_CameraTranspose.m[0][0] >> 10;
	offsetY = pb->matrix_CameraTranspose.m[1][0] >> 10;
	offsetZ = pb->matrix_CameraTranspose.m[2][0] >> 10;

	if ((d->instSelf->flags & HIDE_MODEL) != 0)
	{
		endpoint->vx = d->posCurr.x >> 8;
		endpoint->vy = warp[4] >> 8;
		endpoint->vz = d->posCurr.z >> 8;
		VehStuckProc_Warp_AddDustPuff1((struct ScratchpadStruct *)endpoint);
	}

	for (int ring = 0; ring < 6; ring++)
	{
		int baseAngle = ((ring << 12) / 6) + warp[3];
		struct VehWarpDustProjected *prev = CTR_SCRATCHPAD_PTR(struct VehWarpDustProjected, 0x1a0);
		struct VehWarpDustProjected *curr = CTR_SCRATCHPAD_PTR(struct VehWarpDustProjected, 0x1b0);

		points[0].vx = (d->posCurr.x >> 8) - (MATH_Sin(baseAngle) >> 5);
		points[0].vy = warp[2] >> 8;
		points[0].vz = (d->posCurr.z >> 8) - (MATH_Cos(baseAngle) >> 5);

		endpoint->vx = d->posCurr.x >> 8;
		endpoint->vy = warp[4] >> 8;
		endpoint->vz = d->posCurr.z >> 8;

		if ((d->instSelf->flags & HIDE_MODEL) == 0)
		{
			points[0].vx = VehWarpDust_AddHalf(points[0].vx, -(MATH_Sin(baseAngle) >> 6));
			points[0].vz = VehWarpDust_AddHalf(points[0].vz, -(MATH_Cos(baseAngle) >> 6));
			endpoint->vx = VehWarpDust_AddHalf(endpoint->vx, MATH_Sin(baseAngle) >> 8);
			endpoint->vz = VehWarpDust_AddHalf(endpoint->vz, MATH_Cos(baseAngle) >> 8);
		}
		else
		{
			VehStuckProc_Warp_AddDustPuff1((struct ScratchpadStruct *)points);
		}

		VehStuckProc_Warp_MoveDustPuff((s16 *)points, VEH_WARP_DUST_SEGMENTS, 0x100, jitterScale);

		for (int i = 1; i < VEH_WARP_DUST_SEGMENTS; i++)
			points[i].vy = VehWarpDust_AddHalf(points[i].vy, MATH_Sin(i << 7) >> 7);

		VehWarpDust_Project(&points[0], offsetX, offsetY, offsetZ, prev);

		for (int seg = 0; seg < VEH_WARP_DUST_SEGMENTS; seg++)
		{
			struct VehWarpDustProjected *tmp;

			VehWarpDust_Project(&points[seg + 1], offsetX, offsetY, offsetZ, curr);
			VehWarpDust_EmitSegment(&prim, pb, prev, curr);

			tmp = prev;
			prev = curr;
			curr = tmp;
		}
	}

	backBuffer->primMem.curr = prim;
}
