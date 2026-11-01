#include <common.h>

enum
{
	UI_ICON_FIXED_SHIFT = 0xc,
	UI_ICON_QUAD_COUNT = 4,
	UI_ICON_TPAGE_BLEND_MASK = 0x60,
	UI_ICON_TPAGE_BLEND_STEP = 0x20,
	UI_ICON_SEMI_TRANS_CODE_BIT = 2,
	UI_WEAPON_SHINE_RESULT_SHIFT = 0xd,
	UI_WEAPON_SHINE_BRIGHT_BASE = 0x00007f7f,
	UI_WEAPON_SHINE_ORANGE_BASE = 0x0000327f,
	UI_WEAPON_SHINE_DARK_BASE = 0x00001021,
	UI_WEAPON_SHINE_GRAY_BASE = 0x005f5f5f,
	UI_WEAPON_SHINE_RESULT_SCALE = 0xff,
	UI_WEAPON_SHINE_RESULT_BASE = 0x80,
	UI_WEAPON_SHINE_COLOR_BRIGHT_ROW = 0,
	UI_WEAPON_SHINE_COLOR_MID_ROW = 1,
	UI_WEAPON_SHINE_COLOR_DARK_ROW = 2,
	UI_WEAPON_SHINE_SECOND_COLOR_TRANSPARENCY = 3,
	UI_WEAPON_SHINE_GT4_CODE = 0x3c,
	UI_TRACKER_BG_SHINE_THETA_STEP = 0x100,
	UI_DRIVER_ICON_CLIP_LIMIT = 166,
	UI_DRIVER_ICON_CLIP_MAX = 165,
	UI_DRIVER_ICON_FT4_CODE = 0x2c,
};

#define UI_ICON_PULSE_CHANNEL(channel, sine) ((channel) += ((channel) * (sine)) >> UI_ICON_FIXED_SHIFT)
static inline void UI_Icon_PulseColor(u32 *color, s32 sine)
{
	s32 red = *(u8 *)color;
	s32 product = red * sine;
	u8 *rgb = (u8 *)color;
	rgb[0] = red + (product >> UI_ICON_FIXED_SHIFT);
	UI_ICON_PULSE_CHANNEL(rgb[1], sine);
	UI_ICON_PULSE_CHANNEL(rgb[2], sine);
}

void UI_WeaponBG_AnimateShine(void)
{
	s32 sine;
	u32 bright;
	u32 orange;
	u32 dark;
	s32 red;
	u8 *rgb;
	u32 grayBase;

	// NOTE(aalhendi): Retail reads only the phase word's signed low half.
	sine = MATH_Sin(CTR_ReadS16LE(&GAME_SDATA_NAME(wumpaShineTheta)));
	bright = UI_WEAPON_SHINE_BRIGHT_BASE;
	// NOTE(aalhendi): The first channel is read before abs to retain the packed-color schedule.
	red = *(u8 *)&bright;
	sine = abs(sine);
	GAME_SDATA_NAME(wumpaShineResult) = ((sine * UI_WEAPON_SHINE_RESULT_SCALE) >> UI_WEAPON_SHINE_RESULT_SHIFT) + UI_WEAPON_SHINE_RESULT_BASE;
	orange = UI_WEAPON_SHINE_ORANGE_BASE;
	dark = UI_WEAPON_SHINE_DARK_BASE;
	rgb = (u8 *)&bright;
	rgb[0] = red + ((red * sine) >> UI_ICON_FIXED_SHIFT);
	UI_ICON_PULSE_CHANNEL(rgb[1], sine);
	UI_ICON_PULSE_CHANNEL(rgb[2], sine);
	UI_Icon_PulseColor(&orange, sine);
	UI_Icon_PulseColor(&dark, sine);
	grayBase = UI_WEAPON_SHINE_GRAY_BASE;
	CtrGpu_WriteColorCode(&GAME_SDATA_NAME(wumpaShineColor1)[UI_WEAPON_SHINE_COLOR_BRIGHT_ROW][0], bright);
	CtrGpu_WriteColorCode(&GAME_SDATA_NAME(wumpaShineColor1)[UI_WEAPON_SHINE_COLOR_MID_ROW][0], orange);
	CtrGpu_WriteColorCode(&GAME_SDATA_NAME(wumpaShineColor1)[UI_WEAPON_SHINE_COLOR_DARK_ROW][0], dark);
	bright = grayBase;
	orange = grayBase;
	dark = grayBase;
	UI_Icon_PulseColor(&bright, sine);
	UI_Icon_PulseColor(&orange, sine);
	UI_Icon_PulseColor(&dark, sine);
	CtrGpu_WriteColorCode(&GAME_SDATA_NAME(wumpaShineColor2)[UI_WEAPON_SHINE_COLOR_BRIGHT_ROW][0], bright);
	CtrGpu_WriteColorCode(&GAME_SDATA_NAME(wumpaShineColor2)[UI_WEAPON_SHINE_COLOR_MID_ROW][0], orange);
	CtrGpu_WriteColorCode(&GAME_SDATA_NAME(wumpaShineColor2)[UI_WEAPON_SHINE_COLOR_DARK_ROW][0], dark);
}

#undef UI_ICON_PULSE_CHANNEL

void UI_WeaponBG_DrawShine(struct Icon *icon, s32 posX, s32 posY, struct PrimMem *primMem, u32 *ot, char transparency, s16 scaleX, s16 scaleY, u32 unusedColor)
{
	u32 bright;
	// NOTE(aalhendi): These palette snapshots retain retail's callee-saved lifetimes under GCC 2.8.1.
	register u32 middle CTR_PSX_REGISTER("$16");
	register u32 dark CTR_PSX_REGISTER("$23");
	s32 pixelWidth;
	s32 pixelHeight;
	s32 zeroX;
	s32 width;
	s32 height;
	s32 rightX;
	s32 bottomY;
	s32 leftX;
	s32 topY;
	s32 stepX;
	s32 stepY;
	s16 startX;
	s16 startY;
	s32 quadIndex;
	register u32 *callOT CTR_PSX_REGISTER("$4");
	POLY_GT4 *p;

	(void)unusedColor;
	pixelWidth = icon->texLayout.u1 - icon->texLayout.u0;
	pixelHeight = icon->texLayout.v2 - icon->texLayout.v0;
	if ((u8)transparency != UI_WEAPON_SHINE_SECOND_COLOR_TRANSPARENCY)
	{
		bright = CTR_ReadU32AlignedLE(&GAME_SDATA_NAME(wumpaShineColor1)[UI_WEAPON_SHINE_COLOR_BRIGHT_ROW][0]);
		middle = CTR_ReadU32AlignedLE(&GAME_SDATA_NAME(wumpaShineColor1)[UI_WEAPON_SHINE_COLOR_MID_ROW][0]);
		dark = CTR_ReadU32AlignedLE(&GAME_SDATA_NAME(wumpaShineColor1)[UI_WEAPON_SHINE_COLOR_DARK_ROW][0]);
	}
	else
	{
		bright = CTR_ReadU32AlignedLE(&GAME_SDATA_NAME(wumpaShineColor2)[UI_WEAPON_SHINE_COLOR_BRIGHT_ROW][0]);
		middle = CTR_ReadU32AlignedLE(&GAME_SDATA_NAME(wumpaShineColor2)[UI_WEAPON_SHINE_COLOR_MID_ROW][0]);
		dark = CTR_ReadU32AlignedLE(&GAME_SDATA_NAME(wumpaShineColor2)[UI_WEAPON_SHINE_COLOR_DARK_ROW][0]);
	}
	width = (pixelWidth * scaleX) >> UI_ICON_FIXED_SHIFT;
	height = (pixelHeight * scaleY) >> UI_ICON_FIXED_SHIFT;
	stepX = scaleX >> UI_ICON_FIXED_SHIFT;
	stepY = scaleY >> UI_ICON_FIXED_SHIFT;
	rightX = posX + width;
	leftX = rightX - stepX;
	bottomY = posY + height;
	topY = bottomY - stepY;
	// NOTE(aalhendi): Keep the zero-translation operand in T9; GCC owns its preservation across AddPrim.
	zeroX = 0;
	CTR_PSX_T9_OPERAND(zeroX);
	quadIndex = 0;
	startX = posX;
	startY = posY;
	do
	{
		p = primMem->cursor;
		CtrGpu_WritePackedUVWord(&p->u0, CTR_ReadU32AlignedLE(&icon->texLayout.u0));
		CtrGpu_WritePackedUVWord(&p->u1, CTR_ReadU32AlignedLE(&icon->texLayout.u1));
		CtrGpu_WritePackedUVWord(&p->u2, CTR_ReadU32AlignedLE(&icon->texLayout.u2));
		CtrGpu_WritePackedUV(&p->u3, CTR_ReadU16AlignedLE(&icon->texLayout.u3));
		if (quadIndex == 0)
		{
			setXY4(p, startX, startY, rightX, startY, startX, bottomY, rightX, bottomY);
		}
		else if (quadIndex == 1)
		{
			s32 altX = zeroX + ((posX + width * 2) - stepX);
			setXY4(p, altX, startY, leftX, startY, altX, bottomY, leftX, bottomY);
		}
		else if (quadIndex == 2)
		{
			s32 altY = (posY + height * 2) - stepY;
			// NOTE(aalhendi): Retail retains this zero-first coordinate copy; native folds the ordinary addition.
			CTR_PSX_ADD_U32(altY, 0, altY);
			setXY4(p, startX, altY, rightX, altY, startX, topY, rightX, topY);
		}
		else
		{
			s32 altX = zeroX + ((posX + width * 2) - stepX);
			s32 altY;
			// NOTE(aalhendi): Finish reflected X before forming reflected Y.
			CTR_PSX_OBSERVE_VALUE(altX);
			altY = (posY + height * 2) - stepY;
			CTR_PSX_ADD_U32(altY, 0, altY);
			setXY4(p, altX, altY, leftX, altY, altX, topY, leftX, topY);
		}
		CTR_PSX_KEEP_VALUE(bright);
		setInt32RGB4(p, dark, middle, middle, bright);
		// NOTE(aalhendi): Complete color words before writing the packet header.
		CTR_PSX_OBSERVE_MEMORY(*p);
		((u8 *)&p->tag)[3] = (sizeof(*p) - sizeof(p->tag)) / sizeof(u32);
		p->code = UI_WEAPON_SHINE_GT4_CODE;
		if ((u8)transparency != 0)
		{
			p->tpage = (p->tpage & ~UI_ICON_TPAGE_BLEND_MASK) | (((u8)transparency - 1) << 5);
			p->code |= UI_ICON_SEMI_TRANS_CODE_BIT;
		}
		// NOTE(aalhendi): This short A0 argument retains GCC's OT spill lifetime and automatic caller-save layout.
		callOT = ot;
		CTR_PSX_OBSERVE_VALUE(ot);
		AddPrim(callOT, p);
		primMem->cursor = (POLY_GT4 *)primMem->cursor + 1;
		quadIndex++;
	} while (quadIndex < UI_ICON_QUAD_COUNT);
}

void UI_TrackerBG(struct Icon *icon, s32 centerX, s32 centerY, struct PrimMem *primMem, u32 *ot, char transparency, s16 scaleX, s16 scaleY, u32 color)
{
	POLY_FT4 *p;
	s32 width;
	s32 height;
	s32 rightX;
	s32 bottomY;
	s32 leftX;
	s32 topY;
	s32 stepX;
	s32 stepY;
	s32 quadIndex;
	u16 phase;

	width = ((icon->texLayout.u1 - icon->texLayout.u0) * scaleX) >> UI_ICON_FIXED_SHIFT;
	height = ((icon->texLayout.v2 - icon->texLayout.v0) * scaleY) >> UI_ICON_FIXED_SHIFT;
	stepX = scaleX >> UI_ICON_FIXED_SHIFT;
	stepY = scaleY >> UI_ICON_FIXED_SHIFT;
	rightX = centerX + width;
	leftX = rightX - stepX;
	bottomY = centerY + height;
	topY = bottomY - stepY;
	// NOTE(aalhendi): Retail advances only the phase word's low half, without carrying into its upper half.
	phase = CTR_ReadU16LE(&sdata->wumpaShineTheta);
	phase += UI_TRACKER_BG_SHINE_THETA_STEP;
	*(CtrPackedU16 *)&sdata->wumpaShineTheta = phase;

	for (quadIndex = 0; quadIndex < UI_ICON_QUAD_COUNT; quadIndex++)
	{
		p = primMem->cursor;
		CtrGpu_WriteColorCode(&p->r0, color);
		CtrGpu_WritePackedUVWord(&p->u0, CTR_ReadU32AlignedLE(&icon->texLayout.u0));
		CtrGpu_WritePackedUVWord(&p->u1, CTR_ReadU32AlignedLE(&icon->texLayout.u1));
		// NOTE(aalhendi): The full UV2 word also initializes packet padding from the following UV pair.
		CtrGpu_WritePackedUVWord(&p->u2, CTR_ReadU32AlignedLE(&icon->texLayout.u2));
		CtrGpu_WritePackedUV(&p->u3, CTR_ReadU16AlignedLE(&icon->texLayout.u3));
		if (quadIndex == 0)
		{
			setXY4(p, centerX, centerY, rightX, centerY, centerX, bottomY, rightX, bottomY);
		}
		else if (quadIndex == 1)
		{
			// NOTE(aalhendi): Arm-local mirrored corners retain GCC 2.8.1's short register lifetimes.
			s32 altX = (centerX + width * 2) - stepX;
			setXY4(p, altX, centerY, leftX, centerY, altX, bottomY, leftX, bottomY);
		}
		else if (quadIndex == 2)
		{
			s32 altY = (centerY + height * 2) - stepY;
			setXY4(p, centerX, altY, rightX, altY, centerX, topY, rightX, topY);
		}
		else
		{
			s32 altX = (centerX + width * 2) - stepX;
			s32 altY = (centerY + height * 2) - stepY;
			setXY4(p, altX, altY, leftX, altY, altX, topY, leftX, topY);
		}
		((u8 *)&p->tag)[3] = (sizeof(*p) - sizeof(p->tag)) / sizeof(u32);
		p->code = UI_DRIVER_ICON_FT4_CODE;
		if ((u8)transparency != 0)
		{
			p->tpage = (p->tpage & ~UI_ICON_TPAGE_BLEND_MASK) | (((u8)transparency - 1) << 5);
			p->code |= UI_ICON_SEMI_TRANS_CODE_BIT;
		}
		AddPrim(ot, p);
		primMem->cursor = (POLY_FT4 *)primMem->cursor + 1;
	}
}

void UI_DrawDriverIcon(struct Icon *icon, s32 posX, s32 posY, struct PrimMem *primMem, u32 *ot, char transparency, s16 scale, u32 color)
{
	POLY_FT4 *p;
	s32 width;
	s32 height;
	u16 scaleBits;
	s32 bottomY;

	p = primMem->cursor;
	scaleBits = scale;
	// NOTE(aalhendi): Retail keeps coordinates full-width and interprets the scale's low half as signed.
	width = icon->texLayout.u1 - icon->texLayout.u0;
	height = icon->texLayout.v2 - icon->texLayout.v0;
	CtrGpu_WriteColorCode(&p->r0, color);
	CtrGpu_WritePackedUVWord(&p->u0, CTR_ReadU32AlignedLE(&icon->texLayout.u0));
	CtrGpu_WritePackedUVWord(&p->u1, CTR_ReadU32AlignedLE(&icon->texLayout.u1));
	CtrGpu_WritePackedUVWord(&p->u2, CTR_ReadU32AlignedLE(&icon->texLayout.u2));
	CtrGpu_WritePackedUV(&p->u3, CTR_ReadU16AlignedLE(&icon->texLayout.u3));

	p->x0 = posX;
	if (posY >= UI_DRIVER_ICON_CLIP_LIMIT)
	{
		p->y0 = UI_DRIVER_ICON_CLIP_MAX;
	}
	else
	{
		p->y0 = posY;
	}
	p->x1 = posX + ((width * (s16)scaleBits) >> UI_ICON_FIXED_SHIFT);
	if (posY >= UI_DRIVER_ICON_CLIP_LIMIT)
	{
		p->y1 = UI_DRIVER_ICON_CLIP_MAX;
	}
	else
	{
		p->y1 = posY;
	}
	bottomY = posY + ((height * (s16)scaleBits) >> UI_ICON_FIXED_SHIFT);
	p->x2 = posX;
	if (bottomY >= UI_DRIVER_ICON_CLIP_LIMIT)
	{
		p->y2 = UI_DRIVER_ICON_CLIP_MAX;
	}
	else
	{
		p->y2 = bottomY;
	}
	p->x3 = posX + ((width * (s16)scaleBits) >> UI_ICON_FIXED_SHIFT);
	bottomY = posY + ((height * (s16)scaleBits) >> UI_ICON_FIXED_SHIFT);
	if (bottomY >= UI_DRIVER_ICON_CLIP_LIMIT)
	{
		p->y3 = UI_DRIVER_ICON_CLIP_MAX;
	}
	else
	{
		p->y3 = bottomY;
	}

	((u8 *)&p->tag)[3] = (sizeof(*p) - sizeof(p->tag)) / sizeof(u32);
	p->code = UI_DRIVER_ICON_FT4_CODE;
	// NOTE(aalhendi): Adjust V from each clipped packet Y's low byte, not the unclipped corner.
	p->v2 = p->v0 + (u8)p->y2 - posY;
	p->v3 = p->v0 + (u8)p->y3 - posY;
	if ((u8)transparency != 0)
	{
		p->tpage = (p->tpage & ~UI_ICON_TPAGE_BLEND_MASK) | (((u8)transparency - 1) << 5);
		p->code |= UI_ICON_SEMI_TRANS_CODE_BIT;
	}
	AddPrim(ot, p);
	primMem->cursor = (POLY_FT4 *)primMem->cursor + 1;
}
