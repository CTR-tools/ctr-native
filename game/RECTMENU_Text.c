#include <common.h>

void RECTMENU_GetWidth(struct RectMenu *m, s16 *width, b32 boolCheckSubmenu)
{
	int fontType = FONT_BIG;
	if ((m->state & USE_SMALL_FONT) != 0)
	{
		fontType = FONT_SMALL;
	}

	for (struct MenuRow *row = m->rows; row->stringIndex != RECTMENU_STRING_NONE; row++)
	{
		const int lineWidth = DecalFont_GetLineWidth((char *)LANG_GetString(row->stringIndex & MENU_ROW_LNG_MASK), fontType);
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
		const int lineWidth = DecalFont_GetLineWidth((char *)LANG_GetString(m->stringIndexTitle & MENU_ROW_LNG_MASK), fontType);
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
