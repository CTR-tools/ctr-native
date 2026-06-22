#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80044ef8-0x80044f90.
void RECTMENU_DrawPolyGT4(struct Icon *icon, s16 posX, s16 posY, struct PrimMem *primMem, u_long *ot, u32 color0, u32 color1, u32 color2, u32 color3,
                          char transparency, s16 scale)
{
	if (!icon)
		return;

	DecalHUD_DrawPolyGT4(icon, posX, posY, primMem, ot, color0, color1, color2, color3, transparency, scale);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80044f90-0x80044ff8.
void RECTMENU_DrawOuterRect_Edge(RECT *r, Color color, u32 param_3, u_long *otMem)
{
	param_3 & 0x20 ? CTR_Box_DrawClearBox(r, &color, TRANS_50_DECAL, otMem) : CTR_Box_DrawSolidBox(r, color, otMem);
}


#if defined(CTR_NATIVE)
// NOTE(aalhendi): Native does not expose EXE rdata; this mirrors 0x80011620.
static const char s_rectMenuTimeFormat[] = "%ld:%ld%ld:%ld%ld";
#define RECTMENU_TIME_FORMAT s_rectMenuTimeFormat
#else
#define RECTMENU_TIME_FORMAT rdata.s_timeString
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80044ff8-0x80045134.
u8 *RECTMENU_DrawTime(int milliseconds)
{
	// 32 is added to milliseconds every frame,
	// 960 per second, the rest is basic math

	char *str = &sdata->ghostStrTrackTime[0];

	// build a string
	sprintf(

	    str,

	    // Format
	    // Minute:Seconds:Milliseconds
	    RECTMENU_TIME_FORMAT,

	    milliseconds / 0xe100,              // minutes
	    (milliseconds / 0x2580) % 6,        // seconds / 10
	    (milliseconds / 0x3c0) % 10,        // seconds
	    ((milliseconds * 10) / 0x3c0) % 10, // milliseconds / 10
	    ((milliseconds * 100) / 0x3c0) % 10 // milliseconds
	);

	return (u8 *)str;
}

#undef RECTMENU_TIME_FORMAT


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80045134-0x80045254.
void RECTMENU_DrawRwdBlueRect_Subset(s16 *pos, int *color, u_long *ot, struct PrimMem *primMem)
{
	POLY_G4 *p = (POLY_G4 *)primMem->cursor;

	if ((u32)p <= (u32)primMem->guardEnd)
	{
		primMem->cursor = p + 1;

		*(int *)&p->r0 = color[0] & 0xffffff;
		*(int *)&p->r1 = color[1] & 0xffffff;
		*(int *)&p->r2 = color[2] & 0xffffff;
		*(int *)&p->r3 = color[3] & 0xffffff;

		p->code = 0x38;

		*(int *)&p->x0 = *(int *)&pos[0];
		*(int *)&p->x1 = (pos[0] + pos[2]) | (pos[1] << 16);
		*(int *)&p->x2 = pos[0] | ((pos[1] + pos[3]) << 16);
		*(int *)&p->x3 = (pos[0] + pos[2]) | ((pos[1] + pos[3]) << 16);

		*(int *)p = CtrGpu_PackOTTag(*ot, 0x8000000);
		*(int *)ot = (int)CtrGpu_PrimToOTLink24(p);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80045254-0x800453e8.
void RECTMENU_DrawRwdBlueRect(RECT *rect, char *metas, u_long *ot, struct PrimMem *primMem)
{
	s16 pos[4];
	int gradient[2];
	int colors[4];

	pos[0] = rect->x;
	pos[2] = rect->w;

	for (int i = 0; (u8)metas[i * 4 + 3] != 0x64; i++)
	{
		u8 *meta = (u8 *)&metas[i * 4];
		gradient[0] = *(int *)&meta[0];
		gradient[1] = *(int *)&meta[4];
		colors[0] = gradient[0];
		colors[1] = gradient[0];
		colors[2] = gradient[1];
		colors[3] = gradient[1];
		pos[1] = rect->y + (s16)(meta[3] * rect->h / 100);
		pos[3] = rect->y + (s16)(meta[7] * rect->h / 100) - pos[1] + 1;

		RECTMENU_DrawRwdBlueRect_Subset(pos, colors, ot, primMem);
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800453e8-0x80045534.
void RECTMENU_DrawRwdTriangle(s16 *position, char *color, u_long *otMem, struct PrimMem *primMem)
{
	POLY_G4 *p;
	void *primmemCurr;

	primmemCurr = primMem->cursor;
	p = 0;

	if (primmemCurr <= primMem->guardEnd)
	{
		p = primmemCurr;
		primMem->cursor = p + 1;
	}

	if (p != 0)
	{
		// RGB
		p->r0 = (u8)color[0x0];
		p->g0 = (u8)color[0x1];
		p->b0 = (u8)color[0x2];

		p->r1 = (u8)color[0x4];
		p->g1 = (u8)color[0x5];
		p->b1 = (u8)color[0x6];

		p->r2 = (u8)color[0x0];
		p->g2 = (u8)color[0x1];
		p->b2 = (u8)color[0x2];

		p->r3 = (u8)color[0x8];
		p->g3 = (u8)color[0x9];
		p->b3 = (u8)color[0xa];

		// rest of the primitive (four xy)
		p->x0 = position[0];
		p->y0 = position[1] - 1;

		p->x1 = position[2];
		p->y1 = position[3];

		p->x2 = position[0];
		p->y2 = position[1];

		p->x3 = position[4];
		p->y3 = position[5];

		setPolyG4(p);
		AddPrim(otMem, p);
	}
	return;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80045534-0x80045650.
void RECTMENU_DrawOuterRect_LowLevel(RECT *p, s16 xOffset, u16 yOffset, Color color, s16 param_5, u_long *otMem)
{
	int iVar1;
	RECT r;

	r.x = p->x;
	iVar1 = (int)param_5;
	r.y = p->y;
	r.w = p->w;
	r.h = yOffset;
	RECTMENU_DrawOuterRect_Edge(&r, color, iVar1, otMem);

	r.y += (p->h - yOffset);
	RECTMENU_DrawOuterRect_Edge(&r, color, iVar1, otMem);

	r.y = p->y + yOffset;
	r.h = p->h - (s16)((int)((u32)yOffset << 0x10) >> 0xf);
	r.w = xOffset;
	RECTMENU_DrawOuterRect_Edge(&r, color, iVar1, otMem);

	r.x += (p->w - xOffset);
	RECTMENU_DrawOuterRect_Edge(&r, color, iVar1, otMem);
	return;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80045650-0x8004568c.
void RECTMENU_DrawOuterRect_HighLevel(RECT *r, Color color, s16 param_3, u_long *otMem)
{
	RECTMENU_DrawOuterRect_LowLevel(r, 3, 2, color, param_3, otMem);
	return;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004568c-0x800457b0.
void RECTMENU_DrawQuip(char *comment, s16 startX, int startY, u32 sizeX, s16 fontType, int textFlag, s16 boxFlag)
{
	int posX = startX;
	int width;
	u32 sizeY;

	if ((sizeX & 0xffff) == 0)
	{
		width = DecalFont_GetLineWidth(comment, fontType);
		sizeX = width + 0xc;
	}

	// if text is not centered
	if ((textFlag & 0x8000) != 0)
	{
		// posX with text un-centered
		posX = startX - (sizeX >> 1);
	}

	sizeY = (u32)data.PlayerCommentBoxParams[fontType];

	// Draw string
	DecalFont_DrawLine(comment, startX, (data.PlayerCommentBoxParams[4 + fontType] + startY), fontType, textFlag);

	RECT r;
	r.x = posX;
	r.y = startY;
	r.w = sizeX;
	r.h = sizeY;
	RECTMENU_DrawInnerRect(&r, boxFlag, sdata->gGT->backBuffer->otMem.uiOT);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800457b0-0x800459ec.
void RECTMENU_DrawInnerRect(RECT *r, int type, u_long *ot)
{
	int *colorDataNormal;
	int *colorDataSpecial;
	int drawMode;
	RECT adjustedRect;

	colorDataNormal = &sdata->battleSetup_Color_UI_1;
	if ((type & 0x10) != 0)
	{
		colorDataNormal = &sdata->battleSetup_Color_UI_2;
	}

	if ((type & 2) == 0)
	{
		Color color;
		color.self = *colorDataNormal;
		RECTMENU_DrawOuterRect_HighLevel(r, color, (int)(s16)(type | 0x20), ot);
	}

	adjustedRect.x = r->x;
	adjustedRect.y = r->y;
	adjustedRect.w = r->w;
	adjustedRect.h = r->h;

	if ((type & 8) == 0)
	{
		if ((type & 2) == 0)
		{
			adjustedRect.x += 3;
			adjustedRect.y += 2;
			adjustedRect.w -= 6;
			adjustedRect.h -= 4;
		}

		if ((type & 1) == 0)
		{
			drawMode = ((type & 0x100) != 0) ? 2 : 0;
			colorDataSpecial = ((type & 0x100) != 0) ? &sdata->DrawSolidBoxData[1] : &sdata->DrawSolidBoxData[2];

			CTR_Box_DrawClearBox(&adjustedRect, (Color *)colorDataSpecial, drawMode, ot);
		}
		else
		{
			Color *color = (Color *)&sdata->DrawSolidBoxData[0];
			CTR_Box_DrawSolidBox(&adjustedRect, *color, ot);
		}
	}

	if ((type & 4) == 0)
	{
		s16 horizontalOffset = ((type & 0x80) != 0) ? 4 : 0xc;
		s16 verticalOffset = ((type & 0x40) != 0) ? 2 : 6;

		adjustedRect.x = r->x + r->w;
		adjustedRect.y = r->y + verticalOffset;
		adjustedRect.w = horizontalOffset;
		adjustedRect.h = r->h;

		int *color = &sdata->DrawSolidBoxData[0];
		CTR_Box_DrawClearBox(&adjustedRect, (Color *)color, 0, ot);

		adjustedRect.x = r->x + horizontalOffset;
		adjustedRect.y = r->y + r->h;
		adjustedRect.w = r->w - horizontalOffset;
		adjustedRect.h = verticalOffset;
		CTR_Box_DrawClearBox(&adjustedRect, (Color *)color, 0, ot);
	}

	return;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800459ec-0x80045b1c.
void RECTMENU_DrawFullRect(struct RectMenu *menu, RECT *inner)
{
	u32 *rgb;
	RECT outer;
	struct GameTracker *gGT = sdata->gGT;

	// if title text exists
	if ((-1 < menu->stringIndexTitle) && ((menu->state & 4) == 0))
	{
		rgb = (menu->drawStyle & 0x10) ? &sdata->battleSetup_Color_UI_2 : &sdata->battleSetup_Color_UI_1;

		outer.x = inner->x + 3;
		outer.y = inner->y + 6;

		// pixel-height of non-title menu rows
		if ((menu->state & 0x80) == 0)
		{
			outer.y = inner->y + 9 + data.font_charPixHeight[1];
		}
		else if ((menu->state & 0x4000) == 0)
		{
			outer.y += data.font_charPixHeight[2];
		}
		else
		{
			outer.y += data.font_charPixHeight[1];
		}

		outer.h = 2;
		outer.w = inner->w - 6;

		Color color;
		color.self = *rgb;
		RECTMENU_DrawOuterRect_Edge(&outer, color, (menu->drawStyle | 0x20), gGT->backBuffer->otMem.uiOT);
	}
	RECTMENU_DrawInnerRect(inner, menu->drawStyle, gGT->backBuffer->otMem.uiOT);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80045b1c-0x80045c50.
void RECTMENU_GetHeight(struct RectMenu *m, s16 *height, int boolCheckSubmenu)
{
	int lineHeight;
	struct MenuRow *row;

	// heighth of small line
	lineHeight = data.font_charPixHeight[FONT_SMALL];

	// if small text disabled
	if ((m->state & USE_SMALL_FONT) == 0)
	{
		// height of big line
		lineHeight = data.font_charPixHeight[FONT_BIG] + 3;
	}

	// if not showing only highlighted row
	if ((m->state & SHOW_ONLY_HIGHLIT_ROW) == 0)
	{
		// if not only drawing title bar
		if ((m->state & ONLY_DRAW_TITLE) == 0)
		{
			// add rows
			for (row = m->rows; row->stringIndex != -1; row++)
			{
				*height += lineHeight;
			}
		}

		// only drawing title bar
		else
		{
			*height += lineHeight - 6;
		}
	}

	// only showing row highlighted
	else
	{
		*height += lineHeight;
	}

	// handle menu title
	if (m->stringIndexTitle >= 0)
	{
		// if not drawing title big
		if ((m->state & BIG_TEXT_IN_TITLE) == 0)
		{
			*height += lineHeight + 6;
		}

		// if drawing title big,
		// this overrides title to big, even if rest of menu is small
		else
		{
			*height += data.font_charPixHeight[FONT_BIG] + 9;
		}
	}

	// if submenu needs to be drawn
	if ((m->state & DRAW_NEXT_MENU_IN_HIERARCHY) != 0)
	{
		if ((boolCheckSubmenu & 0xffff) != 0)
		{
			// recursively check height for more submenus
			RECTMENU_GetHeight(m->ptrNextBox_InHierarchy, height, 1);
		}
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80045c50-0x80045db0.
void RECTMENU_GetWidth(struct RectMenu *m, s16 *width, int boolCheckSubmenu)
{
	int fontType;
	struct MenuRow *row;
	int lineWidth;

	fontType = FONT_BIG;

	// if menu should have tiny text
	if ((m->state & 0x80) != 0)
	{
		fontType = FONT_SMALL;
	}

	// handle rows
	for (row = m->rows; row->stringIndex != -1; row++)
	{
		// width of string in each row
		lineWidth = DecalFont_GetLineWidth(sdata->lngStrings[row->stringIndex & 0x7fff], fontType);

		// set new width if new max is found
		if (*width < (lineWidth + 1))
		{
			*width = lineWidth + 1;
		}
	}

	// handle menu title
	if (m->stringIndexTitle >= 0)
	{
		// if force title to be big
		if ((m->state & 0x4000) != 0)
		{
			// override
			fontType = FONT_BIG;
		}

		// width of string in each row
		lineWidth = DecalFont_GetLineWidth(sdata->lngStrings[m->stringIndexTitle & 0x7fff], fontType);

		// set new width if new max is found
		if (*width < (lineWidth + 1))
		{
			*width = lineWidth + 1;
		}
	}

	// if submenu needs to be drawn
	if ((m->state & 0x10) != 0)
	{
		if ((boolCheckSubmenu & 0xffff) != 0)
		{
			// recursively check height for more submenus
			RECTMENU_GetWidth(m->ptrNextBox_InHierarchy, width, 1);
		}
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80045db0-0x80046404.
void RECTMENU_DrawSelf(struct RectMenu *menu, int posX, s16 posY, s16 menuWidth)
{
	u16 textFlags;
	u32 state;
	int index;
	char *titleString;
	s16 offsetX;
	s16 sVar4;
	Color *rgb;
	u16 uVar5;
	struct MenuRow *row;
	s16 sVar6;
	s16 sVar7;
	u16 uVar8;
	RECT background;
	RECT borders;
	s16 local_60;
	s16 menuHeight;
	s16 offsetY;
	s16 local_50;
	s16 local_48;
	s16 local_40;
	s16 local_38;
	int local_30;
	int local_2c;
	s16 posX_prev;
	s16 posY_prev;
	struct GameTracker *gGT = sdata->gGT;

	uVar8 = 0;
	local_40 = 0;
	local_38 = 0;
	if ((menu->drawStyle & 0x10U) != 0)
	{
		uVar8 = 0x1d;
	}
	offsetY = posY;
	if ((menu->state & 0x60000) == 0x60000)
	{
		menu->unk1e = 2;
		if (menu->funcPtr != NULL)
		{
			menu->funcPtr(menu);
		}
	}
	posX_prev = 2;
	if ((menu->state & 0x80) == 0)
	{
		posX_prev = 1;
		local_50 = 2;
		sVar7 = data.font_charPixHeight[1] + 3;
	}
	else
	{
		local_50 = 0;
		sVar7 = data.font_charPixHeight[2];
		if ((menu->state & 0x4000) == 0)
		{
			local_48 = data.font_charPixHeight[2];
			goto LAB_80045e94;
		}
	}
	local_48 = data.font_charPixHeight[1] + 3;
LAB_80045e94:

	local_60 = 0;
	menu->posX_prev = menu->posX_curr;
	menu->posY_prev = menu->posY_curr;
	RECTMENU_GetHeight(menu, &local_60, 0);

	state = menu->state;

	menu->width = menuWidth;
	menu->state &= 0xfffffff7;
	menu->height = local_60;

	if ((state & 2) != 0)
	{
		menuHeight = 0;
		RECTMENU_GetHeight(menu, &menuHeight, 1);
		local_38 = (s16)(-menuHeight / 2);
	}
	if ((state & 1) != 0)
	{
		local_40 = (s16)(-menuWidth / 2);
	}
	sVar6 = 0;
	row = &menu->rows[0];
	index = menu->stringIndexTitle;
	posY_prev = local_50 + local_38 + offsetY + menu->posY_prev;
	if ((-1 < index) && ((state & 4) == 0))
	{
		sVar4 = 1;
		if ((state & 0x4000) == 0)
		{
			sVar4 = posX_prev;
		}
		if ((state & 0x200) == 0)
		{
			offsetX = (s16)(posX + menu->posX_prev);
			uVar5 = uVar8;
			if ((state & 1) != 0)
			{
				uVar5 = uVar8 | 0x8000;
			}
			titleString = sdata->lngStrings[index];
		}
		else
		{
			uVar5 = uVar8 | 0x8000;
			titleString = sdata->lngStrings[index];
			offsetX = (s16)(posX + menu->posX_prev + (menuWidth / 2));
		}
		DecalFont_DrawLine(titleString, offsetX, posY_prev, sVar4, uVar5);
		posY_prev = local_48 + posY_prev + 6;
	}

	if (row->stringIndex != -1)
	{
		local_30 = (menuWidth / 2) + 1;
		local_2c = posX_prev;
		do
		{
			state = menu->state;
			if (((state & 0x44) == 0) || (sVar6 == menu->rowSelected))
			{
				uVar5 = row->stringIndex;
				textFlags = 0x17;
				if ((uVar5 & 0x8000) == 0)
				{
					textFlags = uVar8;
				}
				if ((uVar5 & 0x7fff) != 0)
				{
					if ((state & 0x200) == 0)
					{
						sVar4 = (s16)(posX + menu->posX_prev + 1);
						if ((state & 1) != 0)
						{
							textFlags |= 0x8000;
						}
						titleString = sdata->lngStrings[uVar5 & 0x7fff];
						index = local_2c;
					}
					else
					{
						textFlags |= 0x8000;
						titleString = sdata->lngStrings[uVar5 & 0x7fff];
						sVar4 = (s16)(posX + menu->posX_prev + local_30);
						index = posX_prev;
					}
					DecalFont_DrawLine(titleString, sVar4, posY_prev, index, textFlags);
				}
				posY_prev += sVar7;
			}
			row++;
			sVar6++;
		} while (row->stringIndex != -1);
	}
	if ((menu->state & 0x104) == 0)
	{
		background.x = local_40 + posX + menu->posX_prev;
		background.y = offsetY + menu->posY_prev + local_38;
		if ((menu->state & 0x40) == 0)
		{
			background.y += menu->rowSelected * sVar7 + local_50 + -1;
		}
		else
		{
			background.y += local_50 + -1;
		}
		if ((menu->state & 0x80) == 0)
		{
			background.h = -3;
		}
		else
		{
			background.h = 1;
		}
		background.h = sVar7 + background.h;
		if (-1 < menu->stringIndexTitle)
		{
			background.y += local_48 + 6;
		}
		rgb = &sdata->menuRowHighlight_Normal;
		if ((menu->drawStyle & 0x10U) != 0)
		{
			rgb = &sdata->menuRowHighlight_Green;
		}
		background.w = menuWidth;

		CTR_Box_DrawClearBox(&background, rgb, 1, gGT->backBuffer->otMem.uiOT);
	}
	if ((menu->state & 0x10) != 0)
	{
		RECTMENU_DrawSelf(menu->ptrNextBox_InHierarchy, posX + menu->posX_prev, local_38 + offsetY + menu->posY_prev + sVar7 + 0xc, menuWidth);
	}
	posX_prev = menu->posX_prev;
	posY_prev = menu->posY_prev;
	if ((menu->state & 4) == 0)
	{
		borders.h = (local_60 + 8) - (*(u8 *)&menu->state >> 7);
	}
	else
	{
		borders.h = sVar7 + 8;
	}
	borders.w = menuWidth + 0xc;
	borders.y = local_38 + offsetY + posY_prev - 4;
	borders.x = local_40 + posX + posX_prev - 6;
	RECTMENU_DrawFullRect(menu, &borders);
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80046404-0x80046458.
void RECTMENU_ClearInput()
{
	int i;

	sdata->AnyPlayerTap = 0;
	sdata->AnyPlayerHold = 0;

	for (i = 0; i < 4; i++)
	{
		sdata->buttonTapPerPlayer[i] = 0;
		sdata->buttonHeldPerPlayer[i] = 0;
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80046458-0x80046534.
void RECTMENU_CollectInput()
{
	int i;
	int numListen;
	struct RectMenu *activeSub;

	sdata->AnyPlayerTap = 0;
	sdata->AnyPlayerHold = 0;
	activeSub = sdata->activeSubMenu;

	numListen = sdata->gGT->numPlyrNextGame;

	if ((activeSub != NULL) && ((activeSub->state & ALL_PLAYERS_USE_MENU) != 0))
	{
		numListen = 4;
	}

	struct GamepadBuffer *gb = &sdata->gGamepads->gamepad[0];

	for (i = 0; i < numListen; i++)
	{
		sdata->buttonTapPerPlayer[i] = gb->buttonsTapped;
		sdata->buttonHeldPerPlayer[i] = gb->buttonsHeldCurrFrame;
		gb++;

		sdata->AnyPlayerTap |= sdata->buttonTapPerPlayer[i];
		sdata->AnyPlayerHold |= sdata->buttonHeldPerPlayer[i];
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80046534-0x8004680c
int RECTMENU_ProcessInput(struct RectMenu *m)
{
	struct MenuRow *currMenuRow;
	int i;
	int button;
	int oldRow;
	int newRow;

	int returnVal = 0;

	RngDeadCoed(&sdata->const_0x30215400);

	// button from any player
	button = sdata->AnyPlayerTap;

	// if only P1 can use menu
	if ((m->state & ALL_PLAYERS_USE_MENU) == 0)
	{
		// get button from P1
		button = sdata->buttonTapPerPlayer[0];
	}

	if (

	    // if not drawing only title bar,
	    // therefore this is the bottom of hierarchy
	    ((m->state & ONLY_DRAW_TITLE) == 0) &&

	    // must be both 0x20000 and 0x40000
	    ((m->state & 0x60000) != 0x60000) &&

	    // D-pad or Cross/Triangle
	    ((button & 0x4007f) != 0) &&

	    // No cheat code entering
	    ((sdata->buttonHeldPerPlayer[0] & (BTN_L1 | BTN_R1)) == 0))
	{
		oldRow = m->rowSelected;
		newRow = oldRow;

		currMenuRow = &m->rows[oldRow];

		if (sdata->activeSubMenu != m)
		{
			sdata->activeSubMenu = m;

			// if input should clear upon opening
			if ((m->state & KEEP_INPUTS_IN_SUBMENU) == 0)
			{
				RECTMENU_ClearInput();
			}
		}

		// optimized way to check all four button presses:
		// up, down, left, right, and get new row
		for (i = 0; i < 4; i++)
		{
			if (((button >> i) & 1) != 0)
			{
				newRow = *(char *)((char *)&currMenuRow->rowOnPressUp + i);
				break;
			}
		}

		// check if row has changed
		if (oldRow != newRow)
		{
			// if cursor moving sound is not muted
			if ((m->state & MUTE_SOUND_OF_MOVING_CURSOR) == 0)
			{
				OtherFX_Play(0, 1);
			}
		}

		if ((button & (BTN_CROSS | BTN_CIRCLE)) == 0)
		{
			if (
			    // if Triangle or Square
			    ((button & (BTN_TRIANGLE | BTN_SQUARE)) != 0) &&

			    // if this is not the top of the menu
			    ((m->state & MENU_CANT_GO_BACK) == 0))
			{
				// process GO BACK

				// if menu is not muted
				if ((m->state & MUTE_SOUND_OF_MOVING_CURSOR) == 0)
				{
					OtherFX_Play(2, 1);
				}

				returnVal = -1;

				m->unk1e = 0;

				m->rowSelected = -1;

				if (m->funcPtr != 0)
				{
					RECTMENU_ClearInput();
					m->funcPtr(m);
				}

				// Save row
				m->rowSelected = newRow;
			}
		}

		// if Cross or Circle
		else
		{
			// unlocked row
			if ((m->rows[m->rowSelected].stringIndex & 0x8000) == 0)
			{
				if ((m->state & MUTE_SOUND_OF_MOVING_CURSOR) == 0)
				{
					OtherFX_Play(1, 1);
				}

				m->unk1e = 0;

				// Save row BEFORE processing the Cross button,
				// this is why you can glitch into 3P VS with
				// only 2 controllers, by pressing DOWN+X same frame
				m->rowSelected = newRow;

				returnVal = 1;

				if (m->funcPtr != 0)
				{
					RECTMENU_ClearInput();
					m->funcPtr(m);
				}
			}
			else if ((m->state & MUTE_SOUND_OF_MOVING_CURSOR) == 0)
			{
				// "womp" sound for LOCKED row
				OtherFX_Play(5, 1);
			}
		}

		RECTMENU_ClearInput();

		m->rowSelected = newRow;
	}

	// if "next" hierarchy level exists
	if ((m->state & DRAW_NEXT_MENU_IN_HIERARCHY) != 0)
	{
		// store self in the next
		m->ptrNextBox_InHierarchy->ptrPrevBox_InHierarchy = m;

		// keep going till the bottom hierarchy level is hit,
		// where m->state&4==0, cause not drawing "only title"
		returnVal = RECTMENU_ProcessInput(m->ptrNextBox_InHierarchy);
	}

	return returnVal;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004680c-0x80046990.
void RECTMENU_ProcessState()
{
	struct RectMenu *currMenu;
	int currState;
	s16 width;
	int state;

	// check for curr box
	currMenu = sdata->ptrDesiredMenu;

	// unused
	if (sdata->framesRemainingInMenu != 0)
		sdata->framesRemainingInMenu--;

	// if you want to change the Menu
	if (currMenu != 0)
	{
		sdata->ptrDesiredMenu = 0;

		// show menu
		sdata->ptrActiveMenu = currMenu;
		currMenu->state &= ~NEEDS_TO_CLOSE;

		// get menu at end of hierarchy, if there is hierarchy
		while ((currMenu->state & DRAW_NEXT_MENU_IN_HIERARCHY) != 0)
		{
			currMenu = (struct RectMenu *)currMenu->ptrNextBox_InHierarchy;
		}

		// remove "draw only title bar" from lowest hierarchy,
		// so that rows in this menu draw properly
		currMenu->state &= ~ONLY_DRAW_TITLE;
	}

	currMenu = sdata->ptrActiveMenu;
	state = currMenu->state;

	// run funcPtr if it exists
	if ((state & (EXECUTE_FUNCPTR | DISABLE_INPUT_ALLOW_FUNCPTRS)) != 0)
	{
		currMenu->unk1e = 1;
		currMenu->funcPtr(currMenu);

		// check if funcPtr changed "state"
		state = currMenu->state;
	}

	// if not character selection
	if ((state & DISABLE_INPUT_ALLOW_FUNCPTRS) == 0)
	{
		// process button input for menu
		RECTMENU_ProcessInput(currMenu);

		// check if ProcessInput changed "state"
		state = currMenu->state;

		// if Menu border is not invisible
		if ((state & INVISIBLE) == 0)
		{
			// clear width, then get width
			width = 0;
			RECTMENU_GetWidth(currMenu, &width, 1);

			// draw
			RECTMENU_DrawSelf(currMenu, 0, 0, (int)width);
		}
	}

	// not sure what this is
	if ((state & 0x800) == 0)
	{
		if (RaceFlag_GetCanDraw() == 0)
		{
			RaceFlag_SetCanDraw(1);
		}

		sdata->gGT->renderFlags |= 0x20;
	}

	// if menu needs to close
	if ((state & NEEDS_TO_CLOSE) != 0)
	{
		// deactivate
		sdata->ptrActiveMenu = 0;
	}
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80046990-0x800469c8.
void RECTMENU_Show(struct RectMenu *m)
{
	RECTMENU_ClearInput();

	sdata->ptrActiveMenu = m;

	m->state &= ~NEEDS_TO_CLOSE;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800469c8-0x800469dc.
void RECTMENU_Hide(struct RectMenu *m)
{
	m->state |= 0x1000;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800469dc-0x800469f0.
int RECTMENU_BoolHidden(struct RectMenu *m)
{
	return ((m->state & NEEDS_TO_CLOSE) != 0);
}
