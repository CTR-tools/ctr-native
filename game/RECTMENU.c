#include <common.h>

#ifndef RECTMENU_TIME_BUFFER
#define RECTMENU_TIME_BUFFER      sdata->ghostStrTrackTime
#define RECTMENU_PLAYER_TAP       sdata->buttonTapPerPlayer
#define RECTMENU_PLAYER_HOLD      sdata->buttonHeldPerPlayer
#define RECTMENU_FONT_HEIGHT      data.font_charPixHeight
#define RECTMENU_QUIP_PARAMS      data.PlayerCommentBoxParams
#define RECTMENU_QUIP_Y           (data.PlayerCommentBoxParams + 4)
#define RECTMENU_BORDER_NORMAL    sdata->battleSetup_Color_UI_1
#define RECTMENU_BORDER_ALT       sdata->battleSetup_Color_UI_2
#define RECTMENU_BOX_COLOR_0      sdata->DrawSolidBoxData[0]
#define RECTMENU_BOX_COLOR_1      sdata->DrawSolidBoxData[1]
#define RECTMENU_BOX_COLOR_2      sdata->DrawSolidBoxData[2]
#define RECTMENU_HIGHLIGHT_GREEN  sdata->menuRowHighlight_Green
#define RECTMENU_ANY_TAP          sdata->AnyPlayerTap
#define RECTMENU_ANY_HOLD         sdata->AnyPlayerHold
#define RECTMENU_ACTIVE_SUBMENU   sdata->activeSubMenu
#define RECTMENU_FRAMES_REMAINING sdata->framesRemainingInMenu
#define RECTMENU_ADV_RNG          sdata->advRng
#define RECTMENU_DRAW_GT4         DecalHUD_DrawPolyGT4
#endif


void RECTMENU_DrawPolyGT4(struct Icon *icon, s32 posX, s32 posY, struct PrimMem *primMem, u32 *ot, Color color0, Color color1, Color color2, Color color3,
                          u8 transparency, s16 scale)
{
	if (!icon)
	{
		return;
	}

	RECTMENU_DRAW_GT4(icon, posX, posY, primMem, ot, color0, color1, color2, color3, (u8)transparency, (s16)scale);
}


void RECTMENU_DrawOuterRect_Edge(RECT *r, const Color *color, u32 param_3, u32 *otMem)
{
	param_3 & 0x20 ? CTR_Box_DrawClearBox(r, color, TRANS_50_DECAL, otMem, &GAME_TRACKER->backBuffer->primMem)
	               : CTR_Box_DrawSolidBox(r, color, otMem, &GAME_TRACKER->backBuffer->primMem);
}


#if defined(CTR_NATIVE)
// NOTE(aalhendi): Native does not expose EXE rdata; this mirrors 0x80011620.
static const char s_rectMenuTimeFormat[] = "%ld:%ld%ld:%ld%ld";
#define RECTMENU_TIME_FORMAT s_rectMenuTimeFormat
#else
#define RECTMENU_TIME_FORMAT rdata.s_timeString
#endif

char *RECTMENU_DrawTime(s32 ticks)
{
	char *str = &RECTMENU_TIME_BUFFER[0];
	// NOTE(aalhendi): The timer runs at 960 ticks/second. Unsigned products retain
	// MIPS word wrapping before the signed divisions, including long races.
	sprintf(str, RECTMENU_TIME_FORMAT, CTR_PRINTF_PSX_LONG(ticks / 0xe100), CTR_PRINTF_PSX_LONG((ticks / 0x2580) % 6),
	        CTR_PRINTF_PSX_LONG((ticks / 0x3c0) % 10), CTR_PRINTF_PSX_LONG(((s32)((u32)ticks * 10) / 0x3c0) % 10),
	        CTR_PRINTF_PSX_LONG(((s32)((u32)ticks * 100) / 0x3c0) % 10));

	return str;
}

#undef RECTMENU_TIME_FORMAT


void RECTMENU_DrawRwdBlueRect_Subset(s16 *pos, const Color *color, u32 *ot, struct PrimMem *primMem)
{
	POLY_G4 *next = primMem->cursor;
	POLY_G4 *p = NULL;
	if (next <= (POLY_G4 *)primMem->guardEnd)
	{
		p = next;
		primMem->cursor = p + 1;
	}
	if (p)
	{
		CtrGpu_WriteColorCode(&p->r0, (ColorCode_GetPacked(&color[0]) & 0xffffff) | 0x38000000);
		CtrGpu_WriteColorCode(&p->r1, ColorCode_GetPacked(&color[1]) & 0xffffff);
		CtrGpu_WriteColorCode(&p->r2, ColorCode_GetPacked(&color[2]) & 0xffffff);
		CtrGpu_WriteColorCode(&p->r3, ColorCode_GetPacked(&color[3]) & 0xffffff);

		CtrGpu_WritePackedXY(&p->x0, ((u16)pos[0] | ((u32)pos[1] << 16)));
		CtrGpu_WritePackedXY(&p->x1, (((pos[0] + pos[2]) & 0xffff) | ((u32)pos[1] << 16)));
		CtrGpu_WritePackedXY(&p->x2, ((u16)pos[0] | ((u32)(pos[1] + pos[3]) << 16)));
		CtrGpu_WritePackedXY(&p->x3, (((pos[0] + pos[2]) & 0xffff) | ((u32)(pos[1] + pos[3]) << 16)));

		p->tag = CtrGpu_PackOTTag(*ot, 0x8000000);
		*ot = CtrGpu_PrimToOTLink24(p);
	}
}


void RECTMENU_DrawRwdBlueRect(RECT *rect, char *metas, u32 *ot, struct PrimMem *primMem)
{
	s16 pos[4];
	Color colors[4];
	s16 i;
	pos[0] = rect->x;
	pos[2] = rect->w;
	// NOTE(aalhendi): Each palette record is RGB plus a vertical percentage;
	// the 100% record ends the strips and supplies the final bottom color.
	for (i = 0; (u8)metas[i * 4 + 3] != 100; ++i)
	{
		colors[1] = ColorCode_Load(&((Color *)metas)[i]);
		colors[0] = colors[1];
		colors[3] = ColorCode_Load(&((Color *)metas)[i + 1]);
		colors[2] = colors[3];
		pos[1] = rect->y + ((u8)metas[i * 4 + 3] * rect->h / 100);
		pos[3] = rect->y + ((u8)metas[i * 4 + 7] * rect->h / 100) - pos[1] + 1;
		RECTMENU_DrawRwdBlueRect_Subset(pos, colors, ot, primMem);
	}
}

void RECTMENU_DrawRwdTriangle(s16 *position, char *color, u32 *otMem, struct PrimMem *primMem)
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
		setPolyG4(p);
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

		AddPrim(otMem, p);
	}
	return;
}


void RECTMENU_DrawOuterRect_LowLevel(RECT *p, s16 xOffset, u16 yOffset, const Color *color, s16 param_5, u32 *otMem)
{
	s32 borderFlags;
	RECT r;

	r.x = p->x;
	borderFlags = param_5;
	r.y = p->y;
	r.w = p->w;
	r.h = yOffset;
	RECTMENU_DrawOuterRect_Edge(&r, color, borderFlags, otMem);

	r.y += (p->h - yOffset);
	RECTMENU_DrawOuterRect_Edge(&r, color, borderFlags, otMem);

	r.y = p->y + yOffset;
	r.w = xOffset;
	// NOTE(aalhendi): Double the signed short inset, retaining retail's shift/truncate order.
	r.h = p->h - (s16)((s32)((u32)yOffset << 16) >> 15);
	RECTMENU_DrawOuterRect_Edge(&r, color, borderFlags, otMem);

	r.x += (p->w - xOffset);
	RECTMENU_DrawOuterRect_Edge(&r, color, borderFlags, otMem);
	return;
}


void RECTMENU_DrawOuterRect_HighLevel(RECT *r, const Color *color, s16 param_3, u32 *otMem)
{
	RECTMENU_DrawOuterRect_LowLevel(r, 3, 2, color, param_3, otMem);
	return;
}


void RECTMENU_DrawQuip(char *comment, s16 startX, s16 startY, s16 sizeX, s16 fontType, s16 textFlag, s16 boxFlag)
{
	s32 width;

	RECT r;

	if (sizeX == 0)
	{
		width = DecalFont_GetLineWidth(comment, fontType);
		sizeX = width + 0xc;
	}

	r.x = startX;
	if (textFlag & 0x8000)
	{
		r.x = startX - sizeX / 2;
	}
	r.y = startY;
	r.w = sizeX;
	r.h = RECTMENU_QUIP_PARAMS[fontType];
	DecalFont_DrawLine(comment, startX, startY + RECTMENU_QUIP_Y[fontType], fontType, textFlag);
	RECTMENU_DrawInnerRect(&r, boxFlag, GAME_TRACKER->backBuffer->otMem.uiOT);
}

void RECTMENU_DrawInnerRect(RECT *r, s16 type, u32 *ot)
{
	u32 *colorDataNormal;
	RECT adjustedRect;

	colorDataNormal = &RECTMENU_BORDER_NORMAL;
	if ((type & 0x10) != 0)
	{
		colorDataNormal = &RECTMENU_BORDER_ALT;
	}

	if ((type & 2) == 0)
	{
		RECTMENU_DrawOuterRect_HighLevel(r, (Color *)colorDataNormal, (int)(s16)(type | 0x20), ot);
	}

	if ((type & 8) == 0)
	{
		adjustedRect = *r;
		if ((type & 2) == 0)
		{
			adjustedRect.x += 3;
			adjustedRect.y += 2;
			adjustedRect.w -= 6;
			adjustedRect.h -= 4;
		}

		if (type & 1)
		{
			CTR_Box_DrawSolidBox(&adjustedRect, &RECTMENU_BOX_COLOR_0, ot, &GAME_TRACKER->backBuffer->primMem);
		}
		else if (type & 0x100)
		{
			CTR_Box_DrawClearBox(&adjustedRect, &RECTMENU_BOX_COLOR_1, 2, ot, &GAME_TRACKER->backBuffer->primMem);
		}
		else
		{
			CTR_Box_DrawClearBox(&adjustedRect, &RECTMENU_BOX_COLOR_2, 0, ot, &GAME_TRACKER->backBuffer->primMem);
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


		CTR_Box_DrawClearBox(&adjustedRect, &RECTMENU_BOX_COLOR_0, 0, ot, &GAME_TRACKER->backBuffer->primMem);

		adjustedRect.x = r->x + horizontalOffset;
		adjustedRect.y = r->y + r->h;
		adjustedRect.w = r->w - horizontalOffset;
		adjustedRect.h = verticalOffset;
		CTR_Box_DrawClearBox(&adjustedRect, &RECTMENU_BOX_COLOR_0, 0, ot, &GAME_TRACKER->backBuffer->primMem);
	}

	return;
}


void RECTMENU_DrawFullRect(struct RectMenu *menu, RECT *inner)
{
	u32 *rgb;
	RECT outer;
	s32 y;

	// if title text exists
	if ((-1 < menu->stringIndexTitle) && ((menu->state & ONLY_DRAW_TITLE) == 0))
	{
		rgb = (menu->drawStyle & 0x10) ? &RECTMENU_BORDER_ALT : &RECTMENU_BORDER_NORMAL;

		outer.x = inner->x + 3;
		{
			s32 baseY = inner->y;
			s32 titleY = baseY + 6;
			// NOTE(aalhendi): The large-font path is baseY + 9 + font height;
			// retaining this grouping preserves GCC 2.8's addition order.
			y = (menu->state & USE_SMALL_FONT)
			        ? ((menu->state & BIG_TEXT_IN_TITLE) ? titleY + data.font_charPixHeight[FONT_BIG] : titleY + data.font_charPixHeight[FONT_SMALL])
			        : baseY - (-9 - data.font_charPixHeight[FONT_BIG]);
			outer.y = y;
		}
		outer.w = inner->w - 6;
		outer.h = 2;

		RECTMENU_DrawOuterRect_Edge(&outer, (Color *)rgb, (s16)(menu->drawStyle | 0x20), GAME_TRACKER->backBuffer->otMem.uiOT);
	}
	RECTMENU_DrawInnerRect(inner, menu->drawStyle, GAME_TRACKER->backBuffer->otMem.uiOT);
}


void RECTMENU_GetHeight(struct RectMenu *m, s16 *height, b16 boolCheckSubmenu)
{
	s16 lineHeight;
	struct MenuRow *row;
	lineHeight = (m->state & USE_SMALL_FONT) ? data.font_charPixHeight[FONT_SMALL] : data.font_charPixHeight[FONT_BIG] + 3;
	if (m->state & SHOW_ONLY_HIGHLIT_ROW)
	{
		*height += lineHeight;
	}
	else if (m->state & ONLY_DRAW_TITLE)
	{
		*height += lineHeight - 6;
	}
	else
	{
		for (row = m->rows; row->stringIndex != -1; ++row)
		{
			*height += lineHeight;
		}
	}
	if (m->stringIndexTitle >= 0)
	{
		*height += (m->state & BIG_TEXT_IN_TITLE) ? data.font_charPixHeight[FONT_BIG] + 9 : lineHeight + 6;
	}
	if (boolCheckSubmenu && (m->state & DRAW_NEXT_MENU_IN_HIERARCHY))
	{
		RECTMENU_GetHeight(m->ptrNextBox_InHierarchy, height, 1);
	}
}

void RECTMENU_GetWidth(struct RectMenu *m, s16 *width, b16 boolCheckSubmenu)
{
	s16 fontType;
	struct MenuRow *row;
	s16 lineWidth;

	fontType = FONT_BIG;

	// if menu should have tiny text
	if ((m->state & USE_SMALL_FONT) != 0)
	{
		fontType = FONT_SMALL;
	}

	// handle rows
	for (row = m->rows; row->stringIndex != -1; row++)
	{
		// width of string in each row
		lineWidth = DecalFont_GetLineWidth(GAME_LANGUAGE_STRINGS[row->stringIndex & 0x7fff], fontType) + 1;

		// set new width if new max is found
		if (*width < (lineWidth))
		{
			*width = lineWidth;
		}
	}

	// handle menu title
	if (m->stringIndexTitle >= 0)
	{
		lineWidth = DecalFont_GetLineWidth(GAME_LANGUAGE_STRINGS[m->stringIndexTitle], (m->state & BIG_TEXT_IN_TITLE) ? FONT_BIG : fontType) + 1;

		// set new width if new max is found
		if (*width < (lineWidth))
		{
			*width = lineWidth;
		}
	}

	if (boolCheckSubmenu && (m->state & DRAW_NEXT_MENU_IN_HIERARCHY))
	{
		RECTMENU_GetWidth(m->ptrNextBox_InHierarchy, width, 1);
	}
}

void RECTMENU_DrawSelf(struct RectMenu *menu, s16 posX, s16 posY, s16 menuWidth)
{
	s16 baseFlags = 0;
	s16 fontType;
	s16 rowHeight;
	s16 rowIndex;
	s16 rowY;
	struct MenuRow *row;
	RECT background;
	RECT borders;
	s16 height;
	s16 hierarchyHeight;
	s16 offsetY = posY;
	s16 rowTopMargin;
	s16 titleHeight;
	s16 centerX = 0;
	s16 centerY = 0;
	Color *highlight;

	if (menu->drawStyle & 0x10)
	{
		baseFlags = 0x1d;
	}
	if ((menu->state & RECTMENU_DRAW_CALLBACK_FLAGS) == RECTMENU_DRAW_CALLBACK_FLAGS)
	{
		menu->funcState = RECTMENU_FUNC_STATE_DRAW;
		if (menu->funcPtr)
		{
			menu->funcPtr(menu);
		}
	}
	fontType = FONT_SMALL;
	if (menu->state & USE_SMALL_FONT)
	{
		rowHeight = RECTMENU_FONT_HEIGHT[FONT_SMALL];
		rowTopMargin = 0;
		if (menu->state & BIG_TEXT_IN_TITLE)
		{
			titleHeight = RECTMENU_FONT_HEIGHT[FONT_BIG] + 3;
		}
		else
		{
			titleHeight = rowHeight;
		}
	}
	else
	{
		fontType = FONT_BIG;
		rowTopMargin = 2;
		rowHeight = RECTMENU_FONT_HEIGHT[FONT_BIG] + 3;
		titleHeight = rowHeight;
	}
	height = 0;
	menu->posX_prev = menu->posX_curr;
	menu->posY_prev = menu->posY_curr;
	RECTMENU_GetHeight(menu, &height, 0);
	menu->width = menuWidth;
	menu->state &= ~RECTMENU_CLOSE_TRANSIENT;
	menu->height = height;
	// NOTE(aalhendi): Keep the stored dimensions as the local layout snapshot.
	// This round trip also preserves GCC 2.8's register lifetimes without a load.
	height = menu->height;
	menuWidth = menu->width;
	if (menu->state & CENTER_ON_Y)
	{
		hierarchyHeight = 0;
		RECTMENU_GetHeight(menu, &hierarchyHeight, 1);
		centerY = -hierarchyHeight / 2;
	}
	if (menu->state & CENTER_ON_X)
	{
		centerX = -menuWidth / 2;
	}
	rowIndex = 0;
	row = menu->rows;
	rowY = rowTopMargin + (centerY + (offsetY + menu->posY_prev));
	if (menu->stringIndexTitle >= 0 && !(menu->state & ONLY_DRAW_TITLE))
	{
		s16 titleFont = (menu->state & BIG_TEXT_IN_TITLE) ? FONT_BIG : fontType;
		if (menu->state & CENTER_MENU_TEXT)
		{
			DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[menu->stringIndexTitle], posX + menu->posX_prev + menuWidth / 2, rowY, titleFont, baseFlags | 0x8000);
		}
		else
		{
			DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[menu->stringIndexTitle], posX + menu->posX_prev, rowY, titleFont,
			                   (menu->state & CENTER_ON_X) ? baseFlags | 0x8000 : baseFlags);
		}
		rowY = titleHeight + (s16)(rowY + 6);
	}
	if (row->stringIndex != -1)
	{
		s32 centeredRowOffset = menuWidth / 2 + 1;
		do
		{
			if (!(menu->state & (ONLY_DRAW_TITLE | SHOW_ONLY_HIGHLIT_ROW)) || rowIndex == menu->rowSelected)
			{
				s16 textIndex = row->stringIndex & 0x7fff;
				u16 textFlags = (row->stringIndex & 0x8000) ? 0x17 : baseFlags;
				// NOTE(aalhendi): The complemented mask sets JUSTIFY_CENTER with a
				// signed result; a direct OR changes GCC 2.8's loop constant folding.
				if (textIndex > 0)
				{
					if (menu->state & CENTER_MENU_TEXT)
					{
						DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[textIndex], posX + menu->posX_prev + centeredRowOffset, rowY, fontType,
						                   ~(~textFlags & 0x7fff));
					}
					else
					{
						DecalFont_DrawLine(GAME_LANGUAGE_STRINGS[textIndex], posX + menu->posX_prev + 1, rowY, fontType,
						                   (menu->state & CENTER_ON_X) ? ~(~textFlags & 0x7fff) : textFlags);
					}
				}
				rowY += rowHeight;
			}
			++row;
			++rowIndex;
		} while (row->stringIndex != -1);
	}
	if (!(menu->state & (HIDE_ROW_HIGHLIGHT | ONLY_DRAW_TITLE)))
	{
		s32 baseY;
		s32 highlightY;
		s32 margin;
		s32 highlightHeight;
		background.x = centerX + (posX + menu->posX_prev);
		baseY = offsetY + menu->posY_prev + centerY;
		margin = rowTopMargin;
		if (!(menu->state & SHOW_ONLY_HIGHLIT_ROW))
		{
			highlightY = baseY + menu->rowSelected * rowHeight + (s16)(margin - 1);
		}
		else
		{
			highlightY = baseY + (s16)(margin - 1);
		}
		background.y = highlightY;
		background.w = menuWidth;
		{
			s32 lineHeight = rowHeight;
			highlightHeight = (menu->state & USE_SMALL_FONT) ? lineHeight + 1 : lineHeight - 3;
			// NOTE(aalhendi): Keep the unadjusted height live so GCC 2.8 uses a
			// separate result register. This constraint emits no instruction.
			CTR_PSX_OBSERVE_VALUE(lineHeight);
		}
		background.h = highlightHeight;
		if (menu->stringIndexTitle >= 0)
		{
			background.y = titleHeight + (s16)(background.y + 6);
		}
		highlight = &GAME_MENU_HIGHLIGHT;
		if (menu->drawStyle & 0x10)
		{
			highlight = &RECTMENU_HIGHLIGHT_GREEN;
		}
		CTR_Box_DrawClearBox(&background, highlight, 1, GAME_TRACKER->backBuffer->otMem.uiOT, &GAME_TRACKER->backBuffer->primMem);
	}
	if (menu->state & DRAW_NEXT_MENU_IN_HIERARCHY)
	{
		RECTMENU_DrawSelf(menu->ptrNextBox_InHierarchy, (s16)(posX + menu->posX_prev), centerY + (offsetY + menu->posY_prev) + (s16)(rowHeight + 12),
		                  menuWidth);
	}
	if (menu->state & ONLY_DRAW_TITLE)
	{
		borders.x = centerX + (posX + menu->posX_prev) - 6;
		borders.y = centerY + (offsetY + menu->posY_prev) - 4;
		borders.w = menuWidth + 12;
		borders.h = rowHeight + 8;
		RECTMENU_DrawFullRect(menu, &borders);
	}
	else
	{
		borders.x = centerX + (posX + menu->posX_prev) - 6;
		borders.y = centerY + (offsetY + menu->posY_prev) - 4;
		borders.w = menuWidth + 12;
		{
			s32 borderHeight = (*(u8 *)&menu->state >> 7);
			s32 paddedHeight = height + 8;
			borderHeight = paddedHeight - borderHeight;
			borders.h = borderHeight;
		}
		RECTMENU_DrawFullRect(menu, &borders);
	}
}

void RECTMENU_ClearInput(void)
{
	s16 i;

	sdata->AnyPlayerTap = 0;
	sdata->AnyPlayerHold = 0;

	for (i = 0; i < 4; i++)
	{
		RECTMENU_PLAYER_TAP[i] = 0;
		RECTMENU_PLAYER_HOLD[i] = 0;
	}
}


void RECTMENU_CollectInput(void)
{
	s16 i;
	s16 numListen;
	struct RectMenu *activeSub = sdata->activeSubMenu;
	RECTMENU_ANY_TAP = 0;
	RECTMENU_ANY_HOLD = 0;
	numListen = (activeSub && (activeSub->state & ALL_PLAYERS_USE_MENU)) ? 4 : GAME_TRACKER->numPlyrNextGame;
	for (i = 0; i < numListen; ++i)
	{
		RECTMENU_PLAYER_TAP[i] = GAMEPADS->gamepad[i].buttonsTapped;
		RECTMENU_PLAYER_HOLD[i] = GAMEPADS->gamepad[i].buttonsHeldCurrFrame;
		RECTMENU_ANY_TAP |= RECTMENU_PLAYER_TAP[i];
		RECTMENU_ANY_HOLD |= RECTMENU_PLAYER_HOLD[i];
	}
}

s32 RECTMENU_ProcessInput(struct RectMenu *m)
{
	struct MenuRow *row;
	s16 selected;
	s16 oldRow;
	s32 button;
	s16 result;
	RngDeadCoed(&RECTMENU_ADV_RNG);
	result = 0;
	if (!(m->state & ONLY_DRAW_TITLE) && (m->state & RECTMENU_DRAW_CALLBACK_FLAGS) != RECTMENU_DRAW_CALLBACK_FLAGS)
	{
		selected = m->rowSelected;
		row = &m->rows[m->rowSelected];
		if (RECTMENU_ACTIVE_SUBMENU != m)
		{
			RECTMENU_ACTIVE_SUBMENU = m;
			if (!(m->state & KEEP_INPUTS_IN_SUBMENU))
			{
				RECTMENU_ClearInput();
			}
		}
		if (!(m->state & ALL_PLAYERS_USE_MENU))
		{
			button = RECTMENU_PLAYER_TAP[0];
		}
		else
		{
			button = sdata->AnyPlayerTap;
		}
		if (!(RECTMENU_PLAYER_HOLD[0] & (BTN_L1 | BTN_R1)))
		{
			if (button & RECTMENU_INPUT_MENU)
			{
				oldRow = selected;
				if (button & BTN_UP)
				{
					selected = (u8)row->rowOnPressUp;
				}
				else if (button & BTN_DOWN)
				{
					selected = (u8)row->rowOnPressDown;
				}
				else if (button & BTN_LEFT)
				{
					selected = (u8)row->rowOnPressLeft;
				}
				else if (button & BTN_RIGHT)
				{
					selected = (u8)row->rowOnPressRight;
				}
				if (oldRow != selected && !(m->state & MUTE_SOUND_OF_MOVING_CURSOR))
				{
					OtherFX_Play(0, 1);
				}
				if (button & (BTN_CROSS_one | BTN_CIRCLE))
				{
					// NOTE(aalhendi): Retail checks the old row's lock before confirming
					// the new selection when direction and confirm arrive together.
					if (m->rows[m->rowSelected].stringIndex & 0x8000)
					{
						if (!(m->state & MUTE_SOUND_OF_MOVING_CURSOR))
						{
							OtherFX_Play(5, 1);
						}
					}
					else
					{
						if (!(m->state & MUTE_SOUND_OF_MOVING_CURSOR))
						{
							OtherFX_Play(1, 1);
						}
						m->funcState = RECTMENU_FUNC_STATE_INPUT;
						m->rowSelected = selected;
						result = 1;
						if (m->funcPtr)
						{
							RECTMENU_ClearInput();
							m->funcPtr(m);
						}
					}
				}
				else if (!(m->state & MENU_CANT_GO_BACK) && (button & (BTN_TRIANGLE | BTN_SQUARE_one)))
				{
					if (!(m->state & MUTE_SOUND_OF_MOVING_CURSOR))
					{
						OtherFX_Play(2, 1);
					}
					m->funcState = RECTMENU_FUNC_STATE_INPUT;
					result = -1;
					m->rowSelected = result;
					// NOTE(aalhendi): Back is reported as row -1 only during the callback.
					if (m->funcPtr)
					{
						RECTMENU_ClearInput();
						m->funcPtr(m);
					}
					m->rowSelected = selected;
				}
				RECTMENU_ClearInput();
			}
			m->rowSelected = selected;
		}
	}
	if (m->state & DRAW_NEXT_MENU_IN_HIERARCHY)
	{
		m->ptrNextBox_InHierarchy->ptrPrevBox_InHierarchy = m;
		result = RECTMENU_ProcessInput(m->ptrNextBox_InHierarchy);
	}
	return result;
}

void RECTMENU_ProcessState(void)
{
	struct RectMenu *currMenu;
	s16 width;
	s16 remaining = RECTMENU_FRAMES_REMAINING;
	if (remaining)
	{
		RECTMENU_FRAMES_REMAINING = remaining - 1;
	}
	if (sdata->ptrDesiredMenu)
	{
		u32 state;
		currMenu = sdata->ptrDesiredMenu;
		state = currMenu->state;
		sdata->ptrActiveMenu = currMenu;
		sdata->ptrDesiredMenu = NULL;
		currMenu->state = state & ~NEEDS_TO_CLOSE;
		while (currMenu->state & DRAW_NEXT_MENU_IN_HIERARCHY)
		{
			currMenu = currMenu->ptrNextBox_InHierarchy;
		}
		currMenu->state &= ~ONLY_DRAW_TITLE;
	}
	currMenu = sdata->ptrActiveMenu;
	if (currMenu->state & (EXECUTE_FUNCPTR | DISABLE_INPUT_ALLOW_FUNCPTRS))
	{
		currMenu->funcState = RECTMENU_FUNC_STATE_UPDATE;
		currMenu->funcPtr(currMenu);
	}
	// NOTE(aalhendi): Callbacks may replace the active menu; do not retain the
	// old pointer across update/input or skip the reload before drawing.
	currMenu = sdata->ptrActiveMenu;
	if (!(currMenu->state & DISABLE_INPUT_ALLOW_FUNCPTRS))
	{
		RECTMENU_ProcessInput(currMenu);
		currMenu = sdata->ptrActiveMenu;
		if (!(currMenu->state & INVISIBLE))
		{
			width = 0;
			RECTMENU_GetWidth(currMenu, &width, 1);
			RECTMENU_DrawSelf(sdata->ptrActiveMenu, 0, 0, width);
		}
	}
	if (!(sdata->ptrActiveMenu->state & RECTMENU_UNKNOWN_0x800))
	{
		if (!RaceFlag_GetCanDraw())
		{
			RaceFlag_SetCanDraw(1);
		}
		GAME_TRACKER->renderFlags |= RENDER_FLAG_RENDER_BUCKET;
	}
	if (sdata->ptrActiveMenu->state & NEEDS_TO_CLOSE)
	{
		sdata->ptrActiveMenu = NULL;
	}
}

void RECTMENU_Show(struct RectMenu *m)
{
	u32 state;
	RECTMENU_ClearInput();
	state = m->state;
	sdata->ptrActiveMenu = m;
	m->state = state & ~NEEDS_TO_CLOSE;
}


void RECTMENU_Hide(struct RectMenu *m)
{
	m->state |= NEEDS_TO_CLOSE;
}


b32 RECTMENU_BoolHidden(struct RectMenu *m)
{
	return ((m->state & NEEDS_TO_CLOSE) != 0);
}
