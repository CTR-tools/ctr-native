#include <common.h>

// NOTE(aalhendi): CAM_SKYBOXGLOW_* macros intentionally use bare `{ ... }`
// blocks rather than `do { ... } while (0)` so GCC 2.8.1 does not emit inner
// loop notes that inflate loop_depth for GPU packet constants inside the loop.
#ifdef CTR_NATIVE
#define CAM_SKYBOXGLOW_PRIM_ADDR(prim)     CtrGpu_PrimToOTLink24(prim)
#define CAM_SKYBOXGLOW_READ_COLOR(p)       CTR_ReadU32AlignedLE(p)
#define CAM_SKYBOXGLOW_WRITE_COLOR(r, val) CtrGpu_WriteColorCode((r), (val))
#define CAM_SKYBOXGLOW_WRITE_XY(x, val)    CtrGpu_WritePackedXY((x), (val))
#else
#define CAM_SKYBOXGLOW_PRIM_ADDR(prim)     (((u32)(prim) << 8) >> 8)
#define CAM_SKYBOXGLOW_READ_COLOR(p)       (*(u32 *)(p))
#define CAM_SKYBOXGLOW_WRITE_COLOR(r, val) (*(u32 *)(r) = (val))
#define CAM_SKYBOXGLOW_WRITE_XY(x, val)    (*(u32 *)(x) = (val))
#endif

#define CAM_SKYBOXGLOW_PACK_XY(x, y) (((u32)(u16)(x)) | ((u32)(u16)(y) << 16))

#define CAM_SKYBOXGLOW_LERP(sc, dst, from, to, ratio)                                                                                                  \
	{                                                                                                                                                  \
		u8 *scBytes = (u8 *)(sc);                                                                                                                      \
		scBytes[(dst) * 4 + 0] = (u8)((s32)scBytes[(from) * 4 + 0] + ((((s32)scBytes[(to) * 4 + 0] - (s32)scBytes[(from) * 4 + 0]) * (ratio)) >> 12)); \
		scBytes[(dst) * 4 + 1] = (u8)((s32)scBytes[(from) * 4 + 1] + ((((s32)scBytes[(to) * 4 + 1] - (s32)scBytes[(from) * 4 + 1]) * (ratio)) >> 12)); \
		scBytes[(dst) * 4 + 2] = (u8)((s32)scBytes[(from) * 4 + 2] + ((((s32)scBytes[(to) * 4 + 2] - (s32)scBytes[(from) * 4 + 2]) * (ratio)) >> 12)); \
	}

#define CAM_SKYBOXGLOW_FIXED_RATIO(numerator, denominator) (CTR_MipsSll((numerator), 12) / (denominator))

#define CAM_SKYBOXGLOW_SCREEN_X(screenWidth, ratio)        (((screenWidth) * (ratio)) >> 12)

static inline s32 CAM_SkyboxGlow_CalcCenterY(struct PushBuffer *pb)
{
	s32 pitch = (pb->rot.x - 0x800) * 0x78;

	if (pitch < 0)
	{
		pitch += 0x3ff;
	}

	return (pitch >> 10) + (pb->rect.h / 2);
}

#define CAM_SKYBOXGLOW_EMIT_G3(primMem, ot, c0, xy0, c1, xy1, c2, xy2)           \
	{                                                                            \
		POLY_G3 *poly = (primMem)->cursor;                                       \
		CAM_SKYBOXGLOW_WRITE_COLOR(&poly->r0, (((c0) << 8) >> 8) | 0x30000000u); \
		CAM_SKYBOXGLOW_WRITE_COLOR(&poly->r1, (c1));                             \
		CAM_SKYBOXGLOW_WRITE_COLOR(&poly->r2, (c2));                             \
		CAM_SKYBOXGLOW_WRITE_XY(&poly->x0, (xy0));                               \
		CAM_SKYBOXGLOW_WRITE_XY(&poly->x1, (xy1));                               \
		CAM_SKYBOXGLOW_WRITE_XY(&poly->x2, (xy2));                               \
		poly->tag = *(ot) | 0x06000000u;                                         \
		*(ot) = CAM_SKYBOXGLOW_PRIM_ADDR(poly);                                  \
		(primMem)->cursor = poly + 1;                                            \
	}

#define CAM_SKYBOXGLOW_EMIT_G4(primMem, ot, c0, xy0, c1, xy1, c2, xy2, c3, xy3)  \
	{                                                                            \
		POLY_G4 *poly = (primMem)->cursor;                                       \
		CAM_SKYBOXGLOW_WRITE_COLOR(&poly->r0, (((c0) << 8) >> 8) | 0x38000000u); \
		CAM_SKYBOXGLOW_WRITE_COLOR(&poly->r1, (c1));                             \
		CAM_SKYBOXGLOW_WRITE_COLOR(&poly->r2, (c2));                             \
		CAM_SKYBOXGLOW_WRITE_COLOR(&poly->r3, (c3));                             \
		CAM_SKYBOXGLOW_WRITE_XY(&poly->x0, (xy0));                               \
		CAM_SKYBOXGLOW_WRITE_XY(&poly->x1, (xy1));                               \
		CAM_SKYBOXGLOW_WRITE_XY(&poly->x2, (xy2));                               \
		CAM_SKYBOXGLOW_WRITE_XY(&poly->x3, (xy3));                               \
		poly->tag = *(ot) | 0x08000000u;                                         \
		*(ot) = CAM_SKYBOXGLOW_PRIM_ADDR(poly);                                  \
		(primMem)->cursor = poly + 1;                                            \
	}

#define CAM_SKYBOXGLOW_EMIT_F3(primMem, ot, c0, xy0, xy1, xy2)                   \
	{                                                                            \
		POLY_F3 *poly = (primMem)->cursor;                                       \
		CAM_SKYBOXGLOW_WRITE_COLOR(&poly->r0, (((c0) << 8) >> 8) | 0x20000000u); \
		CAM_SKYBOXGLOW_WRITE_XY(&poly->x0, (xy0));                               \
		CAM_SKYBOXGLOW_WRITE_XY(&poly->x1, (xy1));                               \
		CAM_SKYBOXGLOW_WRITE_XY(&poly->x2, (xy2));                               \
		poly->tag = *(ot) | 0x04000000u;                                         \
		*(ot) = CAM_SKYBOXGLOW_PRIM_ADDR(poly);                                  \
		(primMem)->cursor = poly + 1;                                            \
	}

#define CAM_SKYBOXGLOW_EMIT_F4(primMem, ot, c0, xy0, xy1, xy2, xy3)              \
	{                                                                            \
		POLY_F4 *poly = (primMem)->cursor;                                       \
		CAM_SKYBOXGLOW_WRITE_COLOR(&poly->r0, (((c0) << 8) >> 8) | 0x28000000u); \
		CAM_SKYBOXGLOW_WRITE_XY(&poly->x0, (xy0));                               \
		CAM_SKYBOXGLOW_WRITE_XY(&poly->x1, (xy1));                               \
		CAM_SKYBOXGLOW_WRITE_XY(&poly->x2, (xy2));                               \
		CAM_SKYBOXGLOW_WRITE_XY(&poly->x3, (xy3));                               \
		poly->tag = *(ot) | 0x05000000u;                                         \
		*(ot) = CAM_SKYBOXGLOW_PRIM_ADDR(poly);                                  \
		(primMem)->cursor = poly + 1;                                            \
	}

void CAM_SkyboxGlow(struct SkyboxGlowGradient *grad, struct PushBuffer *pb, struct PrimMem *primMem, u32 *ptrOT)
{
	u32 *sc;
	s32 tilt;
	s32 cosine;
	s32 screenWidth;
	u32 screenWidthXY;
	s32 i;

	tilt = CTR_MipsSll(MATH_Sin(pb->rot.z), 12);
	cosine = MATH_Cos(pb->rot.z);
	if (cosine == 0)
	{
		cosine = 1;
	}
	tilt = tilt / cosine;
	tilt = CTR_MipsSll(tilt, 8);
	tilt = tilt >> 12;
	tilt = -tilt / 2;

	sc = CTR_SCRATCHPAD_PTR(u32, 0);
	((s32 *)sc)[5] = CAM_SkyboxGlow_CalcCenterY(pb);
	// NOTE(aalhendi): Match GCC 2.8.1 global.c priority between tilt (s5) and screenWidth (s6).
	CTR_PSX_KEEP_VALUE_RELAXED(tilt);
	((s32 *)sc)[6] = CAM_SkyboxGlow_CalcCenterY(pb);
	screenWidth = (s16)pb->rect.w;
	screenWidthXY = (u16)screenWidth;

	for (i = 0; i < 3; i++)
	{
		// NOTE(aalhendi): Keep toBase in a0 ahead of shorter-lived pointFrom (a1) in local-alloc.
		register s32 toBase CTR_PSX_REGISTER("$4") = ((s32 *)sc)[6] - grad[i].pointTo;
		s32 toLeft = toBase - tilt;
		s32 toRight = toBase + tilt;
		s32 fromBase;
		s32 fromLeft;
		s32 fromRight;
		s32 ratio;
		s32 fromX;
		u32 colorFrom = grad[i].colorFrom & 0xffffff;
		u32 colorTo;
		u32 mask = 0;

		sc[0] = colorFrom;
		sc[1] = colorFrom;
		colorTo = grad[i].colorTo & 0xffffff;
		fromBase = ((s32 *)sc)[5] - grad[i].pointFrom;
		fromLeft = fromBase - tilt;
		fromRight = fromBase + tilt;
		// NOTE(aalhendi): Keep colorTo stores after fromLeft/fromRight so sc[3] fills bltz's delay slot.
		CTR_PSX_DEPEND_VALUE(colorTo, fromLeft);
		sc[2] = colorTo;
		sc[3] = colorTo;

		if (toLeft >= 0)
		{
			mask |= 1;
		}
		if (toRight >= 0)
		{
			mask |= 2;
		}
		if (fromLeft >= 0)
		{
			mask |= 4;
		}
		if (fromRight >= 0)
		{
			mask |= 8;
		}

		switch (mask)
		{
		case 1:
		{
			u32 rightX;

			ratio = CAM_SKYBOXGLOW_FIXED_RATIO(fromLeft, fromLeft - toLeft);
			CAM_SKYBOXGLOW_LERP(sc, 0, 0, 2, ratio);
			ratio = CAM_SKYBOXGLOW_FIXED_RATIO(toLeft, toLeft - toRight);
			CAM_SKYBOXGLOW_LERP(sc, 3, 2, 3, ratio);
			rightX = (u16)CAM_SKYBOXGLOW_SCREEN_X(screenWidth, ratio);

			CAM_SKYBOXGLOW_EMIT_G3(primMem, ptrOT, sc[0], 0, sc[2], CAM_SKYBOXGLOW_PACK_XY(0, toLeft), sc[3], rightX);
			break;
		}
		case 2:
		{
			u32 leftX;

			ratio = CAM_SKYBOXGLOW_FIXED_RATIO(fromRight, fromRight - toRight);
			CAM_SKYBOXGLOW_LERP(sc, 1, 1, 3, ratio);
			ratio = CAM_SKYBOXGLOW_FIXED_RATIO(toLeft, toLeft - toRight);
			CAM_SKYBOXGLOW_LERP(sc, 2, 2, 3, ratio);
			leftX = (u16)CAM_SKYBOXGLOW_SCREEN_X(screenWidth, ratio);

			CAM_SKYBOXGLOW_EMIT_G3(primMem, ptrOT, sc[2], leftX, sc[1], screenWidthXY, sc[3], screenWidthXY | ((u32)(u16)toRight << 16));
			break;
		}
		case 3:
		{
			ratio = CAM_SKYBOXGLOW_FIXED_RATIO(fromLeft, fromLeft - toLeft);
			CAM_SKYBOXGLOW_LERP(sc, 0, 0, 2, ratio);
			ratio = CAM_SKYBOXGLOW_FIXED_RATIO(fromRight, fromRight - toRight);
			CAM_SKYBOXGLOW_LERP(sc, 1, 1, 3, ratio);

			CAM_SKYBOXGLOW_EMIT_G4(primMem, ptrOT, sc[0], 0, sc[1], screenWidthXY, sc[2], CAM_SKYBOXGLOW_PACK_XY(0, toLeft), sc[3],
			                       screenWidthXY | ((u32)(u16)toRight << 16));
			break;
		}
		case 5:
		{
			s32 toX;

			ratio = CAM_SKYBOXGLOW_FIXED_RATIO(fromLeft, fromLeft - fromRight);
			fromX = CAM_SKYBOXGLOW_SCREEN_X(screenWidth, ratio);
			ratio = CAM_SKYBOXGLOW_FIXED_RATIO(toLeft, toLeft - toRight);
			toX = CAM_SKYBOXGLOW_SCREEN_X(screenWidth, ratio);

			CAM_SKYBOXGLOW_EMIT_G4(primMem, ptrOT, sc[0], CAM_SKYBOXGLOW_PACK_XY(0, fromLeft), sc[1], (u16)fromX, sc[2], CAM_SKYBOXGLOW_PACK_XY(0, toLeft),
			                       sc[3], (u16)toX);

			if ((i == 0) && (GAME_TRACKER->level1->clearColor[2].enable != 0))
			{
				CAM_SKYBOXGLOW_EMIT_F3(primMem, ptrOT, CAM_SKYBOXGLOW_READ_COLOR(&GAME_TRACKER->level1->clearColor[2].rgb[0]), (u16)fromX, 0,
				                       CAM_SKYBOXGLOW_PACK_XY(0, fromLeft));
			}
			break;
		}
		case 7:
		{
			ratio = CAM_SKYBOXGLOW_FIXED_RATIO(fromLeft, fromLeft - fromRight);
			fromX = CAM_SKYBOXGLOW_SCREEN_X(screenWidth, ratio);
			CAM_SKYBOXGLOW_LERP(sc, 4, 0, 1, ratio);
			ratio = CAM_SKYBOXGLOW_FIXED_RATIO(fromRight, fromRight - toRight);
			CAM_SKYBOXGLOW_LERP(sc, 1, 1, 3, ratio);

			CAM_SKYBOXGLOW_EMIT_G3(primMem, ptrOT, sc[0], CAM_SKYBOXGLOW_PACK_XY(0, fromLeft), sc[4], (u16)fromX, sc[1], screenWidthXY);
			CAM_SKYBOXGLOW_EMIT_G4(primMem, ptrOT, sc[0], CAM_SKYBOXGLOW_PACK_XY(0, fromLeft), sc[1], screenWidthXY, sc[2], CAM_SKYBOXGLOW_PACK_XY(0, toLeft),
			                       sc[3], screenWidthXY | ((u32)(u16)toRight << 16));

			if ((i == 0) && (GAME_TRACKER->level1->clearColor[2].enable != 0))
			{
				CAM_SKYBOXGLOW_EMIT_F3(primMem, ptrOT, CAM_SKYBOXGLOW_READ_COLOR(&GAME_TRACKER->level1->clearColor[2].rgb[0]), (u16)fromX, 0,
				                       CAM_SKYBOXGLOW_PACK_XY(0, fromLeft));
			}
			break;
		}
		case 10:
		{
			s32 toX;
			u32 fromRightXY;

			ratio = CAM_SKYBOXGLOW_FIXED_RATIO(fromLeft, fromLeft - fromRight);
			fromX = CAM_SKYBOXGLOW_SCREEN_X(screenWidth, ratio);
			ratio = CAM_SKYBOXGLOW_FIXED_RATIO(toLeft, toLeft - toRight);
			toX = CAM_SKYBOXGLOW_SCREEN_X(screenWidth, ratio);
			fromRightXY = screenWidthXY | ((u32)(u16)fromRight << 16);

			CAM_SKYBOXGLOW_EMIT_G4(primMem, ptrOT, sc[0], (u16)fromX, sc[1], fromRightXY, sc[2], (u16)toX, sc[3], screenWidthXY | ((u32)(u16)toRight << 16));

			if ((i == 0) && (GAME_TRACKER->level1->clearColor[2].enable != 0))
			{
				CAM_SKYBOXGLOW_EMIT_F3(primMem, ptrOT, CAM_SKYBOXGLOW_READ_COLOR(&GAME_TRACKER->level1->clearColor[2].rgb[0]), (u16)fromX, screenWidthXY,
				                       fromRightXY);
			}
			break;
		}
		case 11:
		{
			u32 fromRightXY;

			ratio = CAM_SKYBOXGLOW_FIXED_RATIO(fromLeft, fromLeft - fromRight);
			fromX = CAM_SKYBOXGLOW_SCREEN_X(screenWidth, ratio);
			CAM_SKYBOXGLOW_LERP(sc, 4, 0, 1, ratio);
			ratio = CAM_SKYBOXGLOW_FIXED_RATIO(fromLeft, fromLeft - toLeft);
			CAM_SKYBOXGLOW_LERP(sc, 0, 0, 2, ratio);
			fromRightXY = screenWidthXY | ((u32)(u16)fromRight << 16);

			CAM_SKYBOXGLOW_EMIT_G3(primMem, ptrOT, sc[0], 0, sc[4], (u16)fromX, sc[1], fromRightXY);
			CAM_SKYBOXGLOW_EMIT_G4(primMem, ptrOT, sc[0], 0, sc[1], fromRightXY, sc[2], CAM_SKYBOXGLOW_PACK_XY(0, toLeft), sc[3],
			                       screenWidthXY | ((u32)(u16)toRight << 16));

			if ((i == 0) && (GAME_TRACKER->level1->clearColor[2].enable != 0))
			{
				CAM_SKYBOXGLOW_EMIT_F3(primMem, ptrOT, CAM_SKYBOXGLOW_READ_COLOR(&GAME_TRACKER->level1->clearColor[2].rgb[0]), (u16)fromX, screenWidthXY,
				                       fromRightXY);
			}
			break;
		}
		case 15:
		{
			u32 fromRightXY = screenWidthXY | ((u32)(u16)fromRight << 16);

			CAM_SKYBOXGLOW_EMIT_G4(primMem, ptrOT, sc[0], CAM_SKYBOXGLOW_PACK_XY(0, fromLeft), sc[1], fromRightXY, sc[2], CAM_SKYBOXGLOW_PACK_XY(0, toLeft),
			                       sc[3], screenWidthXY | ((u32)(u16)toRight << 16));

			if ((i == 0) && (GAME_TRACKER->level1->clearColor[2].enable != 0))
			{
				CAM_SKYBOXGLOW_EMIT_F4(primMem, ptrOT, CAM_SKYBOXGLOW_READ_COLOR(&GAME_TRACKER->level1->clearColor[2].rgb[0]), 0, screenWidthXY,
				                       CAM_SKYBOXGLOW_PACK_XY(0, fromLeft), fromRightXY);
			}
			break;
		}
		}
	}
}
