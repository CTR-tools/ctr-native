#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800511c0-0x800516ac.
void UI_DrawSpeedNeedle(s16 posX, s16 posY, struct Driver *driver)
{
	int minScale = 0;
	int maxScale = FP8_INT(driver->const_AccelSpeed_ClassStat) + FP8_INT(driver->const_SacredFireSpeed);
	int speed = driver->speedometerNeedleValue;
	int minAngle, maxAngle;
	int accelSpeedInt = FP8_INT(driver->const_AccelSpeed_ClassStat);
	if (driver->const_AccelSpeed_ClassStat < speed)
	{
		maxAngle = ANG(157.5);
		minAngle = ANG(213.75);
		minScale = (accelSpeedInt * 108000) / 64000;
	}
	else
	{
		maxAngle = ANG(213.75);
		minAngle = ANG(305.20);
		maxScale = accelSpeedInt;
	}

	int speedScale = (FP8_INT(speed) * 108000) / 64000;
	maxScale = (maxScale * 108000) / 64000;
	int angle1 = VehCalc_MapToRange(speedScale, minScale, maxScale, minAngle, maxAngle);
	int angle2 = angle1 + ANG(90);

	int cos[2] = {MATH_Cos(angle1), MATH_Cos(angle2)};
	int sin[2] = {MATH_Sin(angle1), MATH_Sin(angle2)};

	PolyG3 *p;
	GetPrimMem(p);
	if (p == nullptr)
	{
		return;
	}

	const PrimCode primCode = {.poly = {.gouraud = 1, .renderCode = RenderCode_Polygon}};

	p->v[0].color = MakeColorCode(91, 91, 0, primCode);
	p->v[1].color = MakeColorCode(50, 43, 1, primCode);
	p->v[2].color = MakeColorCode(255, 187, 0, primCode);

	/* Needle is distorted in the y axis by a factor of 0.625 */
	const int needleHeight = 60;
	const int needleWidth = 6;
	const int needleCenterX = 65;
	const int needleCenterY = 41;

	int yLen = FP_INT(cos[1] * (needleWidth + 2)) * FP8(1.25);
	if (yLen < 0)
	{
		yLen += FP8(2) - 1;
	}
	yLen /= FP8(2);
	p->v[2].pos.x = posX + (FP_INT(sin[1] * needleWidth) + needleCenterX);
	p->v[2].pos.y = posY + (yLen + needleCenterY);

	yLen = FP_INT(cos[0] * needleWidth) * FP8(1.25);
	if (yLen < 0)
	{
		yLen += FP8(2) - 1;
	}
	yLen /= FP8(2);
	p->v[1].pos.x = posX - (FP_INT(sin[0] * needleWidth) - needleCenterX);
	p->v[1].pos.y = posY - (yLen - needleCenterY);

	yLen = FP_INT(cos[0] * needleHeight) * FP8(1.25);
	if (yLen < 0)
	{
		yLen += FP8(2) - 1;
	}
	yLen /= FP8(2);
	p->v[0].pos.x = posX + (FP_INT(sin[0] * needleHeight) + needleCenterX);
	p->v[0].pos.y = posY + (yLen + needleCenterY);

	AddPrimitive(p, sdata->gGT->pushBuffer_UI.ptrOT);
	GetPrimMem(p);
	if (p == nullptr)
	{
		return;
	}

	p->v[0].color = MakeColorCode(255, 255, 255, primCode);
	p->v[1].color = MakeColorCode(156, 105, 0, primCode);
	p->v[2].color = MakeColorCode(255, 255, 0, primCode);

	yLen = FP_INT(cos[1] * (needleWidth + 2)) * FP8(1.25);
	if (yLen < 0)
	{
		yLen += FP8(2) - 1;
	}
	yLen /= FP8(2);
	p->v[2].pos.x = posX - (FP_INT(sin[1] * needleWidth) - needleCenterX);
	p->v[2].pos.y = posY - (yLen - needleCenterY);

	yLen = FP_INT(cos[0] * needleWidth) * FP8(1.25);
	if (yLen < 0)
	{
		yLen += FP8(2) - 1;
	}
	yLen /= FP8(2);
	p->v[1].pos.x = posX - (FP_INT(sin[0] * needleWidth) - needleCenterX);
	p->v[1].pos.y = posY - (yLen - needleCenterY);

	yLen = FP_INT(cos[0] * needleHeight) * FP8(1.25);
	if (yLen < 0)
	{
		yLen += FP8(2) - 1;
	}
	yLen /= FP8(2);
	p->v[0].pos.x = posX + (FP_INT(sin[0] * needleHeight) + needleCenterX);
	p->v[0].pos.y = posY + (yLen + needleCenterY);

	AddPrimitive(p, sdata->gGT->pushBuffer_UI.ptrOT);
}

#ifdef _MSC_VER

#define SPEEDO_GREEN  0xb500
#define SPEEDO_YELLOW 0xffd1
#define SPEEDO_RED    0xdb

const Color DrawSpeedBG_Colors[7] = {
    [0].self = SPEEDO_GREEN,  [1].self = SPEEDO_GREEN, [2].self = SPEEDO_GREEN, [3].self = SPEEDO_GREEN,
    [4].self = SPEEDO_YELLOW, [5].self = SPEEDO_RED,   [6].self = SPEEDO_RED,
};

#else

#define SPEEDO_GREEN  MakeColor(0, 0xb5, 0)
#define SPEEDO_YELLOW MakeColor(0xff, 0xd1, 0)
#define SPEEDO_RED    MakeColor(0xdb, 0, 0)

const Color DrawSpeedBG_Colors[7] = {
    SPEEDO_GREEN, SPEEDO_GREEN, SPEEDO_GREEN, SPEEDO_GREEN, SPEEDO_YELLOW, SPEEDO_RED, SPEEDO_RED,
};
#endif

// NOTE(aalhendi): PSX path ASM-verified NTSC-U 926 0x800516ac-0x80051c64.
void UI_DrawSpeedBG(void)
{
	Point *vertexes = (Point *)&data.speedometerBG_vertData[0];
	Point *vertexesExtLine = (Point *)&data.speedometerBG_vertData[1];
	int pointCount = sizeof(data.speedometerBG_vertData) / (sizeof(Point) * 2);
	const s16 xOffset = 480;
	const s16 yOffset = 190;

	/* Draw the horizontal lines - they're a bit wider than the speedometer width */
	for (int i = 0; i < pointCount; i += 2)
	{
		Point pt[2];
		pt[0] = MakePoint(vertexesExtLine[i].x + xOffset, vertexesExtLine[i].y + yOffset);
		pt[1] = MakePoint(vertexesExtLine[i + 1].x + xOffset, vertexesExtLine[i + 1].y + yOffset);
		CTR_Box_DrawWirePrims(pt[0], pt[1], MakeColor(0xff, 0xff, 0xff), sdata->gGT->pushBuffer_UI.ptrOT);
		CTR_Box_DrawWirePrims(MakePoint(pt[0].x + 1, pt[0].y + 1), MakePoint(pt[1].x + 1, pt[1].y + 1), MakeColor(0, 0, 0), sdata->gGT->pushBuffer_UI.ptrOT);
	}

	/* Draw the vertical lines and colors */
	int colorIndex = 0;
	for (int i = 0; i < pointCount - 2; i += 2)
	{
		Point pt[4];
		for (int j = 0; j < 4; j++)
		{
			pt[j] = MakePoint(vertexes[i + j].x + xOffset, vertexes[i + j].y + yOffset);
		}
		CTR_Box_DrawWirePrims(pt[0], pt[2], MakeColor(0xff, 0xff, 0xff), sdata->gGT->pushBuffer_UI.ptrOT);
		CTR_Box_DrawWirePrims(pt[1], pt[3], MakeColor(0xff, 0xff, 0xff), sdata->gGT->pushBuffer_UI.ptrOT);
		CTR_Box_DrawWirePrims(MakePoint(pt[0].x + 1, pt[0].y + 1), MakePoint(pt[2].x + 1, pt[2].y + 1), MakeColor(0, 0, 0), sdata->gGT->pushBuffer_UI.ptrOT);
		CTR_Box_DrawWirePrims(MakePoint(pt[1].x + 1, pt[1].y + 1), MakePoint(pt[3].x + 1, pt[3].y + 1), MakeColor(0, 0, 0), sdata->gGT->pushBuffer_UI.ptrOT);

		PolyG4 *p;
		GetPrimMem(p);
		if (p == nullptr)
		{
			return;
		}

		const PrimCode primCode = {.poly = {.renderCode = RenderCode_Polygon, .gouraud = 1, .quad = 1}};
		ColorCode colorBottom = DrawSpeedBG_Colors[colorIndex];
		ColorCode colorTop = DrawSpeedBG_Colors[colorIndex + 1];
		colorBottom.code = primCode;
		colorTop.code = primCode;
		++colorIndex;
		for (int j = 0; j < 2; j++)
		{
			p->v[j].pos = pt[j];
			p->v[j].color = colorBottom;
			p->v[j + 2].pos = pt[j + 2];
			p->v[j + 2].color = colorTop;
		}
		p->gPolyCode = primCode;
		AddPrimitive(p, sdata->gGT->pushBuffer_UI.ptrOT);
	}

	typedef struct TPage_PolyG3
	{
		TPage t;
		PolyG3 p;
	} TPage_PolyG3;

	/* Draw transparent background */
	s16 p2x = vertexes[pointCount - 1].x + xOffset;
	s16 p2y = vertexes[1].y + yOffset;
	for (int i = 0; i < pointCount - 2; i += 2)
	{
		TPage_PolyG3 *p;
		GetPrimMem(p);
		if (p == nullptr)
		{
			return;
		}

		p->t.texpage = (Texpage){.code = 0xE1, .dither = 1};
		p->p.tag.self = 0;

		Color color = MakeColor(0, 0, 0);
		const PrimCode primCode = {.poly = {.renderCode = RenderCode_Polygon, .gouraud = 1, .semiTransparency = 1}};

		for (int j = 0; j < 3; j++)
		{
			p->p.v[j].color = color;
		}
		p->p.gPolyCode = primCode;

		p->p.v[0].pos.x = vertexes[i + 1].x + xOffset;
		p->p.v[0].pos.y = vertexes[i + 1].y + yOffset;
		p->p.v[1].pos.x = vertexes[i + 3].x + xOffset;
		p->p.v[1].pos.y = vertexes[i + 3].y + yOffset;
		p->p.v[2].pos.x = p2x;
		p->p.v[2].pos.y = p2y;

// NOTE(aalhendi): CTR_NATIVE keeps PsyCross display-area drawing enabled.
#ifdef CTR_NATIVE
		((TPage *)p)->texpage.drawDisplayArea = 1;
#endif

		AddPrimitive(p, sdata->gGT->pushBuffer_UI.ptrOT);
	}
}
