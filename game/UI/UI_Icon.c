#include <common.h>

enum
{
	UI_ICON_FIXED_SHIFT = 0xc,
	UI_ICON_QUAD_COUNT = 4,
	UI_ICON_TPAGE_BLEND_MASK = 0x60,
	UI_ICON_TPAGE_BLEND_STEP = 0x20,
	UI_ICON_SEMI_TRANS_CODE_BIT = 2,
	UI_WEAPON_SHINE_RESULT_SHIFT = 0xd,
	UI_WEAPON_SHINE_BRIGHT_YELLOW_BASE = 0x7f,
	UI_WEAPON_SHINE_ORANGE_GREEN_BASE = 0x32,
	UI_WEAPON_SHINE_DARK_RED_BASE = 0x21,
	UI_WEAPON_SHINE_DARK_GREEN_BASE = 0x10,
	UI_WEAPON_SHINE_GRAY_BASE = 0x5f,
	UI_WEAPON_SHINE_RESULT_SCALE = 0xff,
	UI_WEAPON_SHINE_RESULT_BASE = 0x80,
	UI_WEAPON_SHINE_COLOR_BRIGHT_ROW = 0,
	UI_WEAPON_SHINE_COLOR_MID_ROW = 1,
	UI_WEAPON_SHINE_COLOR_DARK_ROW = 2,
	UI_WEAPON_SHINE_SECOND_COLOR_TRANSPARENCY = 3,
	UI_TRACKER_BG_SHINE_THETA_STEP = 0x100,
	UI_DRIVER_ICON_NTSC_CLIP_LIMIT = 166,
	UI_DRIVER_ICON_NTSC_CLIP_MAX = 165,
	UI_DRIVER_ICON_EUR_CLIP_LIMIT = 176,
	UI_DRIVER_ICON_EUR_CLIP_MAX = 175,
	UI_DRIVER_ICON_FT4_CODE = 0x2c,
};

void UI_WeaponBG_AnimateShine(void)
{
	int sine;
	u32 brightYellow;
	u32 orangeGreen;
	u32 gray;

	sine = MATH_Sin((int)sdata->wumpaShineTheta);
	sine = (sine < 0) ? -sine : sine;

	brightYellow = ((sine * UI_WEAPON_SHINE_BRIGHT_YELLOW_BASE) >> UI_ICON_FIXED_SHIFT) + UI_WEAPON_SHINE_BRIGHT_YELLOW_BASE;
	sdata->wumpaShineColor1[0][0] = brightYellow;
	sdata->wumpaShineColor1[0][1] = brightYellow;
	CTR_WriteU16LE(&sdata->wumpaShineColor1[0][2], 0);

	orangeGreen = ((sine * UI_WEAPON_SHINE_ORANGE_GREEN_BASE) >> UI_ICON_FIXED_SHIFT) + UI_WEAPON_SHINE_ORANGE_GREEN_BASE;
	sdata->wumpaShineColor1[1][0] = brightYellow;
	sdata->wumpaShineColor1[1][1] = orangeGreen;
	CTR_WriteU16LE(&sdata->wumpaShineColor1[1][2], 0);

	sdata->wumpaShineColor1[2][0] = ((sine * UI_WEAPON_SHINE_DARK_RED_BASE) >> UI_ICON_FIXED_SHIFT) + UI_WEAPON_SHINE_DARK_RED_BASE;
	sdata->wumpaShineColor1[2][1] = ((sine * UI_WEAPON_SHINE_DARK_GREEN_BASE) >> UI_ICON_FIXED_SHIFT) + UI_WEAPON_SHINE_DARK_GREEN_BASE;
	CTR_WriteU16LE(&sdata->wumpaShineColor1[2][2], 0);

	gray = ((sine * UI_WEAPON_SHINE_GRAY_BASE) >> UI_ICON_FIXED_SHIFT) + UI_WEAPON_SHINE_GRAY_BASE;
	sdata->wumpaShineColor2[0][0] = gray;
	sdata->wumpaShineColor2[0][1] = gray;
	CTR_WriteU16LE(&sdata->wumpaShineColor2[0][2], (u16)gray);

	gray = CTR_ReadU32LE(&sdata->wumpaShineColor2[0][0]);
	CTR_WriteU32LE(&sdata->wumpaShineColor2[1][0], gray);
	CTR_WriteU32LE(&sdata->wumpaShineColor2[2][0], gray);

	sdata->wumpaShineResult = (sine * UI_WEAPON_SHINE_RESULT_SCALE >> UI_WEAPON_SHINE_RESULT_SHIFT) + UI_WEAPON_SHINE_RESULT_BASE;
	return;
}

void UI_WeaponBG_DrawShine(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u32 *ot, char transparency, s16 angleX, s16 angleY, int unusedColor)
{
	s16 rightX;
	s16 bottomY;
	s16 widthOffset;
	s16 heightOffset;
	s16 altX;
	s16 altY;
	POLY_GT4 *p;
	int quadIndex;
	s16 topY;
	s16 leftX;

	(void)unusedColor;

	u8 *shineColors = &sdata->wumpaShineColor1[0][0];

	if (transparency == UI_WEAPON_SHINE_SECOND_COLOR_TRANSPARENCY)
	{
		shineColors = &sdata->wumpaShineColor2[0][0];
	}

	widthOffset = (s16)(((icon->texLayout.u1 - icon->texLayout.u0) * (int)angleX) >> UI_ICON_FIXED_SHIFT);
	rightX = posX + widthOffset;
	angleX = angleX >> UI_ICON_FIXED_SHIFT;
	leftX = rightX - angleX;

	heightOffset = (s16)(((icon->texLayout.v2 - icon->texLayout.v0) * (int)angleY) >> UI_ICON_FIXED_SHIFT);
	bottomY = posY + heightOffset;
	angleY = angleY >> UI_ICON_FIXED_SHIFT;
	topY = bottomY - angleY;

	for (quadIndex = 0; quadIndex < UI_ICON_QUAD_COUNT; quadIndex++)
	{
		p = primMem->cursor;
		CtrGpu_WritePackedUVWord(&p->u0, CTR_ReadU32LE(&icon->texLayout.u0));
		CtrGpu_WritePackedUVWord(&p->u1, CTR_ReadU32LE(&icon->texLayout.u1));
		CtrGpu_WritePackedUVWord(&p->u2, CTR_ReadU32LE(&icon->texLayout.u2));
		CtrGpu_WritePackedUV(&p->u3, CTR_ReadU16LE(&icon->texLayout.u3));

		switch (quadIndex)
		{
		case 0:
			p->x0 = posX;
			p->y0 = posY;

			p->x1 = rightX;
			p->y1 = posY;
			p->x2 = posX;
			p->y2 = bottomY;
			p->x3 = rightX;
			p->y3 = bottomY;
			break;

		case 1:
			altX = (posX + widthOffset * 2) - angleX;
			p->x0 = altX;
			p->y0 = posY;

			p->x1 = leftX;
			p->y1 = posY;
			p->x2 = altX;
			p->y2 = bottomY;
			p->x3 = leftX;
			p->y3 = bottomY;

			break;

		case 2:
			altY = (posY + heightOffset * 2) - angleY;
			p->x0 = posX;
			p->y0 = altY;

			p->x1 = rightX;
			p->y1 = altY;
			p->x2 = posX;
			p->y2 = topY;
			p->x3 = rightX;
			p->y3 = topY;

			break;

		case 3:
			altX = (posX + widthOffset * 2) - angleX;
			altY = (posY + heightOffset * 2) - angleY;
			p->x0 = altX;
			p->y0 = altY;

			p->x1 = leftX;
			p->y1 = altY;
			p->x2 = altX;
			p->y2 = topY;
			p->x3 = leftX;
			p->y3 = topY;

			break;
		}

		// All four mirrored quadrants share one center; compressing each
		// separately pulls their inner edges apart.
		p->x0 = Widescreen_ScaleXFromAnchor(p->x0, rightX);
		p->x1 = Widescreen_ScaleXFromAnchor(p->x1, rightX);
		p->x2 = Widescreen_ScaleXFromAnchor(p->x2, rightX);
		p->x3 = Widescreen_ScaleXFromAnchor(p->x3, rightX);

		CtrGpu_WriteColorCode(&p->r0, CTR_ReadU32LE(&shineColors[UI_WEAPON_SHINE_COLOR_DARK_ROW * sizeof(u32)]));
		CtrGpu_WriteColorCode(&p->r1, CTR_ReadU32LE(&shineColors[UI_WEAPON_SHINE_COLOR_MID_ROW * sizeof(u32)]));
		CtrGpu_WriteColorCode(&p->r2, CTR_ReadU32LE(&shineColors[UI_WEAPON_SHINE_COLOR_MID_ROW * sizeof(u32)]));
		CtrGpu_WriteColorCode(&p->r3, CTR_ReadU32LE(&shineColors[UI_WEAPON_SHINE_COLOR_BRIGHT_ROW * sizeof(u32)]));

		setPolyGT4(p);

		if (transparency != 0)
		{
			p->tpage = (p->tpage & ~UI_ICON_TPAGE_BLEND_MASK) | (((u16)transparency - 1) * UI_ICON_TPAGE_BLEND_STEP);
			p->code |= UI_ICON_SEMI_TRANS_CODE_BIT;
		}

		AddPrim(ot, p);

		primMem->cursor = p + 1;
	}
}

void UI_TrackerBG(struct Icon *targetIcon, s16 centerX, s16 centerY, struct PrimMem *primMem, u32 *ot, char transparency, s16 angleX, s16 angleY, int color, int anchorX)
{
	s16 rightX;
	s16 bottomY;
	s16 heightOffset;
	POLY_FT4 *p;
	int widthOffset;
	int quadIndex;
	s16 topY;
	s16 leftX;

	sdata->wumpaShineTheta += UI_TRACKER_BG_SHINE_THETA_STEP;

	widthOffset = ((targetIcon->texLayout.u1 - targetIcon->texLayout.u0) * angleX) >> UI_ICON_FIXED_SHIFT;
	heightOffset = ((targetIcon->texLayout.v2 - targetIcon->texLayout.v0) * angleY) >> UI_ICON_FIXED_SHIFT;

	rightX = centerX + widthOffset;
	angleX >>= UI_ICON_FIXED_SHIFT;
	leftX = rightX - angleX;

	bottomY = centerY + heightOffset;
	angleY >>= UI_ICON_FIXED_SHIFT;
	topY = bottomY - angleY;

	int altX = (centerX + (widthOffset * 2)) - angleX;
	int altY = (centerY + (heightOffset * 2)) - angleY;

	for (quadIndex = 0; quadIndex < UI_ICON_QUAD_COUNT; quadIndex++)
	{
		p = primMem->cursor;
		primMem->cursor = (p + 1);

		CtrGpu_WriteColorCode(&p->r0, (u32)color);
		CtrGpu_WritePackedUVWord(&p->u0, CTR_ReadU32LE(&targetIcon->texLayout.u0));
		CtrGpu_WritePackedUVWord(&p->u1, CTR_ReadU32LE(&targetIcon->texLayout.u1));
		CtrGpu_WritePackedUV(&p->u2, CTR_ReadU16LE(&targetIcon->texLayout.u2));
		CtrGpu_WritePackedUV(&p->u3, CTR_ReadU16LE(&targetIcon->texLayout.u3));

		setPolyFT4(p);

		if (transparency != 0)
		{
			p->tpage = (p->tpage & ~UI_ICON_TPAGE_BLEND_MASK) | (((u16)transparency - 1) * UI_ICON_TPAGE_BLEND_STEP);
			p->code |= UI_ICON_SEMI_TRANS_CODE_BIT;
		}

		p->x0 = centerX;
		p->x1 = rightX;
		p->y0 = centerY;
		p->y2 = bottomY;

		switch (quadIndex)
		{
		case 1:
			p->x0 = altX;
			p->x1 = leftX;
			break;

		case 2:
			p->y0 = altY;
			p->y2 = topY;
			break;

		case 3:
			p->x0 = altX;
			p->y0 = altY;

			p->x1 = leftX;
			p->y2 = topY;
			break;
		}

		p->x0 = anchorX + Widescreen_ScaleWidth(p->x0 - anchorX);
		p->x1 = anchorX + Widescreen_ScaleWidth(p->x1 - anchorX);
		p->x2 = p->x0;
		p->y1 = p->y0;

		p->x3 = p->x1;
		p->y3 = p->y2;

		AddPrim(ot, p);
	}
	return;
}

void UI_DrawDriverIcon(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u32 *ot, char transparency, s16 scale, u32 color)
{
	PolyFT4 *p = primMem->cursor;
	const PrimCode primCode = {.kind.poly = {.renderCode = RenderCode_Polygon, .quad = 1, .textured = 1}};
	ColorCode_SetPacked(&p->colorCode, color);
	p->colorCode.code = primCode;

	int width = icon->texLayout.u1 - icon->texLayout.u0;
	int height = icon->texLayout.v2 - icon->texLayout.v0;
	int scaledWidth = FP_Mult(width, scale);
	int scaledHeight = FP_Mult(height, scale);
	int topX = posX;
	int bottomX = topX + scaledWidth;
	int topY = (posY < UI_DRIVER_ICON_NTSC_CLIP_LIMIT) ? posY : UI_DRIVER_ICON_NTSC_CLIP_MAX;
	int bottomY = ((posY + scaledHeight) < UI_DRIVER_ICON_NTSC_CLIP_LIMIT) ? (posY + scaledHeight) : UI_DRIVER_ICON_NTSC_CLIP_MAX;

	p->tag.bits.size = (sizeof(*p) - sizeof(p->tag)) / sizeof(u32);
	p->colorCode.code.code = UI_DRIVER_ICON_FT4_CODE;

	p->v[0].pos.x = topX;
	p->v[0].pos.y = topY;
	p->v[1].pos.x = bottomX;
	p->v[1].pos.y = topY;
	p->v[2].pos.x = topX;
	p->v[2].pos.y = bottomY;
	p->v[3].pos.x = bottomX;
	p->v[3].pos.y = bottomY;

	Widescreen_CompressNative(p);

	p->polyClut.self = icon->texLayout.clut;
	p->polyTpage.self = icon->texLayout.tpage;
	p->v[2].page.clut.self = (icon->texLayout.v3 << 8) | icon->texLayout.u3;

	if (transparency)
	{
		p->polyTpage.bits.semiTransparency = transparency - 1;
		p->colorCode.code.kind.poly.semiTransparency = 1;
	}

	u8 bottomV = (u8)((icon->texLayout.v0 + bottomY) - posY);
	p->v[0].texCoords.u = icon->texLayout.u0;
	p->v[0].texCoords.v = icon->texLayout.v0;
	p->v[1].texCoords.u = icon->texLayout.u1;
	p->v[1].texCoords.v = icon->texLayout.v1;
	p->v[2].texCoords.u = icon->texLayout.u2;
	p->v[2].texCoords.v = bottomV;
	p->v[3].texCoords.u = icon->texLayout.u3;
	p->v[3].texCoords.v = bottomV;

	AddPrimitive(p, ot);
	primMem->cursor = p + 1;
}
