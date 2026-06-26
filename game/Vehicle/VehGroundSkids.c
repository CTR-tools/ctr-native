#include <common.h>

static u32 VehGroundSkids_ReadTexWord(const struct TextureLayout *layout, int offset)
{
	u32 word;
	memcpy(&word, (const u8 *)layout + offset, sizeof(word));
	return word;
}

static u16 VehGroundSkids_ReadTexHalf(const struct TextureLayout *layout, int offset)
{
	u16 half;
	memcpy(&half, (const u8 *)layout + offset, sizeof(half));
	return half;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005c120-0x8005c278.
void VehGroundSkids_Subset1(u32 *currXY, u32 *prevXY, int depth, struct VehGroundSkidsScratch *scratch)
{
	struct GameTracker *gGT = sdata->gGT;
	struct DB *backBuffer = gGT->backBuffer;
	POLY_GT4 *poly = backBuffer->primMem.cursor;
	POLY_GT4 *nextPrim = poly + 1;

	if ((char *)backBuffer->primMem.guardEnd < (char *)nextPrim)
	{
		return;
	}

	backBuffer->primMem.cursor = nextPrim;

	CtrGpu_WriteColorCode(&poly->r0, scratch->colorNear);
	CtrGpu_WriteColorCode(&poly->r1, scratch->colorNear);
	CtrGpu_WriteColorCode(&poly->r2, scratch->colorFar);
	CtrGpu_WriteColorCode(&poly->r3, scratch->colorFar);

	CtrGpu_WritePackedXY(&poly->x0, currXY[0]);
	CtrGpu_WritePackedXY(&poly->x1, currXY[1]);
	CtrGpu_WritePackedXY(&poly->x2, prevXY[0]);
	CtrGpu_WritePackedXY(&poly->x3, prevXY[1]);

	struct Icon *icon = gGT->ptrIcons[0x2f];
	CtrGpu_WritePackedUVWord(&poly->u0, VehGroundSkids_ReadTexWord(&icon->texLayout, 0x0));

	u32 tpage = VehGroundSkids_ReadTexWord(&icon->texLayout, 0x4);
	if ((scratch->segmentFlags & 1) != 0)
	{
		tpage = (tpage & 0xff9fffff) | 0x00600000;
	}
	else
	{
		tpage = (tpage & 0xff9fffff) | 0x00400000;
	}
	CtrGpu_WritePackedUVWord(&poly->u1, tpage);

	CtrGpu_WritePackedUV(&poly->u2, VehGroundSkids_ReadTexHalf(&icon->texLayout, 0x8));
	CtrGpu_WritePackedUV(&poly->u3, VehGroundSkids_ReadTexHalf(&icon->texLayout, 0xa));

	struct PushBuffer *pb = scratch->pushBuffer;
	uint32_t *ot = pb->ptrOT + ((s32)depth >> 6);
	CtrGpu_LinkPacket24(ot, &poly->tag, poly, 0x0c000000);
}

static s16 VehGroundSkids_ScaleRelative(u16 value, u16 origin)
{
	// NOTE(aalhendi): Retail uses lhu/subu/sll/sh, so preserve unsigned halfword wraparound.
	return (s16)(u16)(((u32)value - (u32)origin) << 2);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005c278-0x8005c354.
void VehGroundSkids_Subset2(struct VehGroundSkidsScratch *scratch, const SVECTOR *v1, const SVECTOR *v2, const SVECTOR *v3)
{
	u16 originX = (u16)scratch->origin.x;
	u16 originY = (u16)scratch->origin.y;
	u16 originZ = (u16)scratch->origin.z;

	scratch->projected[0].vx = VehGroundSkids_ScaleRelative((u16)v1->vx, originX);
	scratch->projected[0].vy = VehGroundSkids_ScaleRelative((u16)v1->vy, originY);
	scratch->projected[0].vz = VehGroundSkids_ScaleRelative((u16)v1->vz, originZ);

	scratch->projected[1].vx = VehGroundSkids_ScaleRelative((u16)v2->vx, originX);
	scratch->projected[1].vy = VehGroundSkids_ScaleRelative((u16)v2->vy, originY);
	scratch->projected[1].vz = VehGroundSkids_ScaleRelative((u16)v2->vz, originZ);

	scratch->projected[2].vx = VehGroundSkids_ScaleRelative((u16)v3->vx, originX);
	scratch->projected[2].vy = VehGroundSkids_ScaleRelative((u16)v3->vy, originY);
	scratch->projected[2].vz = VehGroundSkids_ScaleRelative((u16)v3->vz, originZ);
}

static u32 VehGroundSkids_ColorWord(int value)
{
	return 0x3e000000 | ((u32)value << 16) | ((u32)value << 8) | (u32)value;
}

static s32 VehGroundSkids_Abs(s32 value)
{
	return value < 0 ? -value : value;
}

static int VehGroundSkids_InitPoint(SVECTOR *scratch, const SVECTOR *point, const s32 *origin)
{
	// NOTE(aalhendi): Retail uses lh/lw/subu/sll here; preserve unsigned 32-bit wraparound.
	s32 x = (s32)(((u32)(s32)point->vx - (u32)origin[0]) << 2);
	if (VehGroundSkids_Abs(x) >= 0x1771)
	{
		return 0;
	}

	s32 y = (s32)(((u32)(s32)point->vy - (u32)origin[1]) << 2);
	if (VehGroundSkids_Abs(y) >= 0x1771)
	{
		return 0;
	}

	s32 z = (s32)(((u32)(s32)point->vz - (u32)origin[2]) << 2);
	if (VehGroundSkids_Abs(z) >= 0x1771)
	{
		return 0;
	}

	scratch[0].vx = x;
	scratch[0].vy = y;
	scratch[0].vz = z;
	return 1;
}

static int VehGroundSkids_IntensityFromDepth(int depth)
{
	depth >>= 2;
	if (depth < 0x180)
	{
		return 0x7f;
	}

	MTC2(depth - 0x180, 30);
	int shift = 0x1a - MFC2(31);
	if (shift < 0)
	{
		shift = 0;
	}

	int intensity = 0x7f >> (shift & 0x1f);
	if (intensity < 0x10)
	{
		return -1;
	}

	return intensity;
}

force_inline void VehGroundSkids_ProjectTriplet(struct VehGroundSkidsScratch *scratch, const SVECTOR *frame, u32 *sxy, s32 *depth)
{
	VehGroundSkids_Subset2(scratch, &frame[0], &frame[1], &frame[2]);
	CTR_GteLoadSV3(&scratch->projected[0], &scratch->projected[1], &scratch->projected[2]);
	gte_rtpt();
	CTR_GteStoreSXY3(&sxy[0], &sxy[1], &sxy[2]);
	gte_stsz3(&depth[0], &depth[1], &depth[2]);
}

static void VehGroundSkids_ProjectFrame(struct VehGroundSkidsScratch *scratch, const SVECTOR *frame, u32 *sxy, s32 *depth)
{
	VehGroundSkids_ProjectTriplet(scratch, &frame[0], &sxy[0], &depth[0]);
	VehGroundSkids_ProjectTriplet(scratch, &frame[3], &sxy[3], &depth[3]);

	VehGroundSkids_Subset2(scratch, &frame[6], &frame[7], &frame[0]);
	CTR_GteLoadSV3(&scratch->projected[0], &scratch->projected[1], &scratch->projected[2]);
	gte_rtpt();
	CTR_GteStoreSXY3(&sxy[6], &sxy[7], &sxy[8]);
	gte_stsz3(&depth[6], &depth[7], &depth[8]);
}

static void VehGroundSkids_TryEmitSegment(struct VehGroundSkidsScratch *scratch, u32 *currXY, u32 *prevXY, s32 *currDepth, s32 *prevDepth, u32 flags,
                                          u32 prevFlags, int bit, const union VehEmitterSkidmark *mark, int pointIndex)
{
	if ((flags & prevFlags & bit) == 0)
	{
		return;
	}
	if (currDepth[pointIndex] <= 0x20 || currDepth[pointIndex + 1] <= 0x20)
	{
		return;
	}
	if (prevDepth[pointIndex] <= 0x20 || prevDepth[pointIndex + 1] <= 0x20)
	{
		return;
	}

	scratch->segmentFlagsLow = mark->flags;
	int depth = (currDepth[pointIndex] >> 2) + (mark->color << 6);
	VehGroundSkids_Subset1(&currXY[pointIndex], &prevXY[pointIndex], depth, scratch);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005c354-0x8005ca24.
void VehGroundSkids_Main(struct Thread *thread, struct PushBuffer *pb)
{
	gte_SetGeomOffset(pb->rect.w >> 1, pb->rect.h >> 1);
	gte_SetGeomScreen(pb->distanceToScreen_PREV);

	struct VehGroundSkidsScratch *scratch = CTR_SCRATCHPAD_PTR(struct VehGroundSkidsScratch, 0x0);

	scratch->pushBuffer = pb;
	scratch->origin.x = 0;
	scratch->origin.y = 0;
	scratch->origin.z = 0;

	gte_SetRotMatrix(&pb->matrix_ViewProj);
	gte_SetTransVector((VECTOR *)&scratch->origin);

	scratch->origin.x = pb->matrix_Camera.t[0];
	scratch->origin.y = pb->matrix_Camera.t[1];
	scratch->origin.z = pb->matrix_Camera.t[2];

	while (thread != NULL)
	{
		struct Driver *d = thread->object;
		u32 flags = d->skidmarkEnableFlags;

		if (flags > 0xf)
		{
			int frameIndex = ((u8)d->skidmarkFrameIndex - 1) & (DRIVER_SKIDMARK_FRAME_COUNT - 1);
			union VehEmitterSkidmark *frame = d->skidmarks[frameIndex];
			SVECTOR *framePoints = &frame[0].edge[0];

			if (VehGroundSkids_InitPoint(scratch->projected, &framePoints[0], scratch->origin.v))
			{
				CTR_GteLoadSV0(&scratch->projected[0]);
				gte_rtv0();

				int intensity = VehGroundSkids_IntensityFromDepth(MFC2_S(27));
				if (intensity >= 0)
				{
					u32 *currXY = scratch->currXY;
					u32 *prevXY = scratch->prevXY;
					s32 *currDepth = scratch->currDepth;
					s32 *prevDepth = scratch->prevDepth;

					scratch->colorNear = VehGroundSkids_ColorWord(intensity);
					scratch->colorFar = 0xffffffff;

					u32 prevFlags = 0;
					while (flags != 0)
					{
						u32 currFlags = flags;

						if ((currFlags & 0xf) != 0)
						{
							frame = d->skidmarks[frameIndex];
							framePoints = &frame[0].edge[0];
							VehGroundSkids_ProjectFrame(scratch, framePoints, currXY, currDepth);

							VehGroundSkids_TryEmitSegment(scratch, currXY, prevXY, currDepth, prevDepth, currFlags, prevFlags, 1, &frame[0], 0);
							VehGroundSkids_TryEmitSegment(scratch, currXY, prevXY, currDepth, prevDepth, currFlags, prevFlags, 2, &frame[1], 2);
							VehGroundSkids_TryEmitSegment(scratch, currXY, prevXY, currDepth, prevDepth, currFlags, prevFlags, 4, &frame[2], 4);
							VehGroundSkids_TryEmitSegment(scratch, currXY, prevXY, currDepth, prevDepth, currFlags, prevFlags, 8, &frame[3], 6);
						}

						u32 *tmpXY = currXY;
						currXY = prevXY;
						prevXY = tmpXY;

						s32 *tmpDepth = currDepth;
						currDepth = prevDepth;
						prevDepth = tmpDepth;

						frameIndex = (frameIndex + 1) & (DRIVER_SKIDMARK_FRAME_COUNT - 1);

						if (scratch->colorFar == 0xffffffff)
						{
							scratch->colorFar = scratch->colorNear;
							prevFlags = 0xf;
						}
						else
						{
							prevFlags = currFlags;
							flags = currFlags >> 4;

							int faded = (scratch->colorNear & 0xff) >> 1;
							scratch->colorFar = scratch->colorNear;
							scratch->colorNear = VehGroundSkids_ColorWord(faded);
						}

						if (scratch->colorFar == 0)
						{
							break;
						}
					}
				}
			}
		}

		thread = thread->siblingThread;
	}
}
