#include <common.h>

struct CAMSkyboxGlowGradient
{
	s16 pointFrom;
	s16 pointTo;
	u32 colorFrom;
	u32 colorTo;
};

static u32 CAM_SkyboxGlow_PrimAddr(const void *prim)
{
	return (u32)((uintptr_t)prim & 0xffffff);
}

static u32 CAM_SkyboxGlow_PackXY(int x, int y)
{
	return ((u32)(u16)x) | ((u32)(u16)y << 16);
}

static u32 CAM_SkyboxGlow_LerpColor(u32 from, u32 to, int ratio)
{
	int r = (u8)from + ((((int)(u8)to - (int)(u8)from) * ratio) >> 12);
	int g = (u8)(from >> 8) + ((((int)(u8)(to >> 8) - (int)(u8)(from >> 8)) * ratio) >> 12);
	int b = (u8)(from >> 16) + ((((int)(u8)(to >> 16) - (int)(u8)(from >> 16)) * ratio) >> 12);

	return ((u32)(u8)r) | ((u32)(u8)g << 8) | ((u32)(u8)b << 16);
}

static int CAM_SkyboxGlow_FixedRatio(int numerator, int denominator)
{
	return (numerator << 12) / denominator;
}

static int CAM_SkyboxGlow_ScreenX(int screenWidth, int ratio)
{
	return (screenWidth * ratio) >> 12;
}

static int CAM_SkyboxGlow_Div2TowardZero(int value)
{
	return (value + ((u32)value >> 31)) >> 1;
}

static int CAM_SkyboxGlow_CalcCenterY(struct PushBuffer *pb)
{
	int pitch = (pb->rot[0] - 0x800) * 0x78;
	int height = (s16)pb->rect.h;

	if (pitch < 0)
		pitch += 0x3ff;

	return (pitch >> 10) + CAM_SkyboxGlow_Div2TowardZero(height);
}

static int CAM_SkyboxGlow_CalcTilt(struct PushBuffer *pb)
{
	int sine = DECOMP_MATH_Sin(pb->rot[2]);
	int cosine = DECOMP_MATH_Cos(pb->rot[2]);
	int ratio;
	int shifted;

	if (cosine == 0)
		cosine = 1;

	ratio = (sine << 12) / cosine;
	shifted = (s32)((u32)ratio << 8);
	shifted >>= 12;

	return CAM_SkyboxGlow_Div2TowardZero(-shifted);
}

static void CAM_SkyboxGlow_EmitG3(struct PrimMem *primMem, u_long *ot, u32 color0, u32 xy0, u32 color1, u32 xy1, u32 color2, u32 xy2)
{
	u32 *prim = primMem->curr;

	prim[1] = (color0 & 0xffffff) | 0x30000000;
	prim[2] = xy0;
	prim[3] = color1 & 0xffffff;
	prim[4] = xy1;
	prim[5] = color2 & 0xffffff;
	prim[6] = xy2;
	prim[0] = *ot | 0x06000000;
	*ot = CAM_SkyboxGlow_PrimAddr(prim);

	primMem->curr = prim + 7;
}

static void CAM_SkyboxGlow_EmitG4(struct PrimMem *primMem, u_long *ot, u32 color0, u32 xy0, u32 color1, u32 xy1, u32 color2, u32 xy2, u32 color3, u32 xy3)
{
	u32 *prim = primMem->curr;

	prim[1] = (color0 & 0xffffff) | 0x38000000;
	prim[2] = xy0;
	prim[3] = color1 & 0xffffff;
	prim[4] = xy1;
	prim[5] = color2 & 0xffffff;
	prim[6] = xy2;
	prim[7] = color3 & 0xffffff;
	prim[8] = xy3;
	prim[0] = *ot | 0x08000000;
	*ot = CAM_SkyboxGlow_PrimAddr(prim);

	primMem->curr = prim + 9;
}

static void CAM_SkyboxGlow_EmitF3(struct PrimMem *primMem, u_long *ot, u32 color, u32 xy0, u32 xy1, u32 xy2)
{
	u32 *prim = primMem->curr;

	prim[1] = (color & 0xffffff) | 0x20000000;
	prim[2] = xy0;
	prim[3] = xy1;
	prim[4] = xy2;
	prim[0] = *ot | 0x04000000;
	*ot = CAM_SkyboxGlow_PrimAddr(prim);

	primMem->curr = prim + 5;
}

static void CAM_SkyboxGlow_EmitF4(struct PrimMem *primMem, u_long *ot, u32 color, u32 xy0, u32 xy1, u32 xy2, u32 xy3)
{
	u32 *prim = primMem->curr;

	prim[1] = (color & 0xffffff) | 0x28000000;
	prim[2] = xy0;
	prim[3] = xy1;
	prim[4] = xy2;
	prim[5] = xy3;
	prim[0] = *ot | 0x05000000;
	*ot = CAM_SkyboxGlow_PrimAddr(prim);

	primMem->curr = prim + 6;
}

static int CAM_SkyboxGlow_HasClearGradient(int gradientIndex)
{
	return gradientIndex == 0 && sdata->gGT->level1->clearColor[2].enable != 0;
}

static u32 CAM_SkyboxGlow_ClearGradientColor(void)
{
	return *(u32 *)&sdata->gGT->level1->clearColor[2].rgb[0] & 0xffffff;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800175cc-0x8001861c
void CAM_SkyboxGlow(s16 *param_1, struct PushBuffer *pb, struct PrimMem *primMem, u_long *ptrOT)
{
	struct CAMSkyboxGlowGradient *grad = (struct CAMSkyboxGlowGradient *)param_1;
	int tilt = CAM_SkyboxGlow_CalcTilt(pb);
	int centerY1 = CAM_SkyboxGlow_CalcCenterY(pb);
	int centerY2 = CAM_SkyboxGlow_CalcCenterY(pb);
	int screenWidth = (s16)pb->rect.w;
	u32 screenWidthXY = (u16)screenWidth;

	for (int i = 0; i < 3; i++, grad++)
	{
		int toLeft = centerY2 - grad->pointTo - tilt;
		int toRight = centerY2 - grad->pointTo + tilt;
		int fromLeft = centerY1 - grad->pointFrom - tilt;
		int fromRight = centerY1 - grad->pointFrom + tilt;
		u32 colorFrom = grad->colorFrom & 0xffffff;
		u32 colorTo = grad->colorTo & 0xffffff;
		u32 mask = 0;

		if (toLeft >= 0)
			mask |= 1;
		if (toRight >= 0)
			mask |= 2;
		if (fromLeft >= 0)
			mask |= 4;
		if (fromRight >= 0)
			mask |= 8;

		switch (mask)
		{
		case 1:
		{
			int leftRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - toLeft);
			int rightRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 leftColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, leftRatio);
			u32 rightX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, rightRatio);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, leftColor, 0, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft), colorTo, rightX);
			break;
		}
		case 2:
		{
			int rightRatio = CAM_SkyboxGlow_FixedRatio(fromRight, fromRight - toRight);
			int leftRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 rightColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, rightRatio);
			u32 leftX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, leftRatio);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, colorTo, leftX, rightColor, screenWidthXY, colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));
			break;
		}
		case 3:
		{
			int leftRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - toLeft);
			int rightRatio = CAM_SkyboxGlow_FixedRatio(fromRight, fromRight - toRight);
			u32 leftColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, leftRatio);
			u32 rightColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, rightRatio);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, leftColor, 0, rightColor, screenWidthXY, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft), colorTo,
			                      CAM_SkyboxGlow_PackXY(screenWidth, toRight));
			break;
		}
		case 5:
		{
			int fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			int toRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 toX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, toRatio);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), colorFrom, fromX, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft),
			                      colorTo, toX);

			if (CAM_SkyboxGlow_HasClearGradient(i))
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, 0, CAM_SkyboxGlow_PackXY(0, fromLeft));
			break;
		}
		case 7:
		{
			int fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			int rightRatio = CAM_SkyboxGlow_FixedRatio(fromRight, fromRight - toRight);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 rightColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, rightRatio);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), colorFrom, fromX, rightColor, screenWidthXY);
			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), rightColor, screenWidthXY, colorTo,
			                      CAM_SkyboxGlow_PackXY(0, toLeft), colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, 0, CAM_SkyboxGlow_PackXY(0, fromLeft));
			break;
		}
		case 10:
		{
			int fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			int toRatio = CAM_SkyboxGlow_FixedRatio(toLeft, toLeft - toRight);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 toX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, toRatio);
			u32 fromRightXY = CAM_SkyboxGlow_PackXY(screenWidth, fromRight);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, fromX, colorFrom, fromRightXY, colorTo, toX, colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, screenWidthXY, fromRightXY);
			break;
		}
		case 11:
		{
			int fromRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - fromRight);
			int leftRatio = CAM_SkyboxGlow_FixedRatio(fromLeft, fromLeft - toLeft);
			u32 fromX = (u16)CAM_SkyboxGlow_ScreenX(screenWidth, fromRatio);
			u32 leftColor = CAM_SkyboxGlow_LerpColor(colorFrom, colorTo, leftRatio);
			u32 fromRightXY = CAM_SkyboxGlow_PackXY(screenWidth, fromRight);

			CAM_SkyboxGlow_EmitG3(primMem, ptrOT, leftColor, 0, colorFrom, fromX, colorFrom, fromRightXY);
			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, leftColor, 0, colorFrom, fromRightXY, colorTo, CAM_SkyboxGlow_PackXY(0, toLeft), colorTo,
			                      CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
				CAM_SkyboxGlow_EmitF3(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), fromX, screenWidthXY, fromRightXY);
			break;
		}
		case 15:
		{
			u32 fromRightXY = CAM_SkyboxGlow_PackXY(screenWidth, fromRight);

			CAM_SkyboxGlow_EmitG4(primMem, ptrOT, colorFrom, CAM_SkyboxGlow_PackXY(0, fromLeft), colorFrom, fromRightXY, colorTo,
			                      CAM_SkyboxGlow_PackXY(0, toLeft), colorTo, CAM_SkyboxGlow_PackXY(screenWidth, toRight));

			if (CAM_SkyboxGlow_HasClearGradient(i))
				CAM_SkyboxGlow_EmitF4(primMem, ptrOT, CAM_SkyboxGlow_ClearGradientColor(), 0, screenWidthXY, CAM_SkyboxGlow_PackXY(0, fromLeft), fromRightXY);
			break;
		}
		}
	}
}
