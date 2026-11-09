#include <common.h>

static const char *const s_menuPortStrings[LNG_PORT_STRING_COUNT] = {
	[LNG_PORT_MODE - LNG_PORT_STRING_BASE] = "Mode",
	[LNG_PORT_CHAOS - LNG_PORT_STRING_BASE] = "Chaos",
	[LNG_PORT_REGULAR - LNG_PORT_STRING_BASE] = "Regular",
	[LNG_PORT_CHAOS_RULES - LNG_PORT_STRING_BASE] = "Chaos Rules",
	[LNG_PORT_SUPER_HARD - LNG_PORT_STRING_BASE] = "Super Hard",
	[LNG_PORT_ULTRA_HARD - LNG_PORT_STRING_BASE] = "Ultra Hard",
};

CTR_STATIC_ASSERT(LNG_PORT_STRING_BASE + LNG_PORT_STRING_COUNT <= MENU_ROW_LNG_MASK);

const char *LANG_GetString(s32 index)
{
	if (index < 0)
	{
		return "";
	}
	if (index >= LNG_PORT_STRING_BASE)
	{
		const s32 portIndex = index - LNG_PORT_STRING_BASE;
		return portIndex < LNG_PORT_STRING_COUNT ? s_menuPortStrings[portIndex] : "";
	}
	if (sdata->lngStrings == NULL || index >= sdata->numLngStrings)
	{
		return "";
	}
	return sdata->lngStrings[index] != NULL ? sdata->lngStrings[index] : "";
}

const char *RECTMENU_GetString(s32 stringIndex)
{
	return LANG_GetString(stringIndex & MENU_ROW_LNG_MASK);
}

void RECTMENU_GetWidth(struct RectMenu *m, s16 *width, b32 boolCheckSubmenu)
{
	int fontType = FONT_BIG;
	if ((m->state & USE_SMALL_FONT) != 0)
	{
		fontType = FONT_SMALL;
	}

	for (struct MenuRow *row = m->rows; row->stringIndex != RECTMENU_STRING_NONE; row++)
	{
		const int lineWidth = DecalFont_GetLineWidth((char *)RECTMENU_GetString(row->stringIndex), fontType);
		if (*width < lineWidth + 1)
		{
			*width = lineWidth + 1;
		}
	}

	if (m->stringIndexTitle >= 0)
	{
		if ((m->state & BIG_TEXT_IN_TITLE) != 0)
		{
			fontType = FONT_BIG;
		}
		const int lineWidth = DecalFont_GetLineWidth((char *)RECTMENU_GetString(m->stringIndexTitle), fontType);
		if (*width < lineWidth + 1)
		{
			*width = lineWidth + 1;
		}
	}

	if ((m->state & DRAW_NEXT_MENU_IN_HIERARCHY) != 0 && (boolCheckSubmenu & 0xffff) != 0)
	{
		RECTMENU_GetWidth(m->ptrNextBox_InHierarchy, width, 1);
	}
}
