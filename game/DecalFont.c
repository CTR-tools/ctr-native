#include <common.h>

#define FONT_ICON_GROUP        data.font_IconGroupID
#define FONT_CHAR_WIDTH        data.font_charPixWidth
#define FONT_CHAR_HEIGHT       data.font_charPixHeight
#define FONT_PUNCT_WIDTH       data.font_puncPixWidth
#define FONT_CHARACTER_ICONS   data.font_characterIconID
#define FONT_BUTTON_SCALE      data.font_buttonScale
#define FONT_BUTTON_WIDTH      data.font_buttonPixWidth
#define FONT_BUTTON_HEIGHT     data.font_buttonPixHeight
#define FONT_INDENT_ICONS      data.font_indentIconID
#define FONT_INDENT_DIMENSIONS data.font_indentPixDimensions
#define FONT_COLORS            data.ptrColor

// PAL character semantics adapted from CTR-ModSDK's ExpandedFont module.
enum
{
	FONT_PAL_TILDE = 3,
	FONT_APOSTROPHE_ICON = 0x24,
	FONT_EXCLAMATION_ICON = 0x25,
	FONT_PERIOD_ICON = 0x2c,
	FONT_QUESTION_ICON = 0x2e,
	FONT_UNDERSCORE_ICON = 0x2f,
};

// SCES_021.05 PAL data at 0x80082948-0x8008299f.
static const s16 s_palTilde[FONT_NUM][3] = {{0, 0, 0}, {2, -11, 0xc00}, {2, -5, 0xc00}, {2, -11, 0xc00}};
static const s16 s_palQuoteOffset[FONT_NUM] = {0, -3, -2, -3};
static const s16 s_palQuoteSpacing[FONT_NUM] = {0, 6, 4, 5};
static const s16 s_palInvertedOffset[FONT_NUM][2] = {{0, 0}, {8, 7}, {5, 3}, {8, 7}};
static const s16 s_palOrdinalOffset[FONT_NUM][2] = {{0, 0}, {-2, -8}, {-2, -3}, {0, -8}};

static int DecalFont_InlineColor(const u8 *text, s16 len)
{
	if ((len >= 0 && len < 3) || text[0] != '~' || text[1] == 0 || text[2] == 0 ||
	    text[1] < '0' || text[1] > '9' || text[2] < '0' || text[2] > '9')
	{
		return -1;
	}
	int color = (text[1] - '0') * 10 + text[2] - '0';
	return color < NUM_COLORS ? color : -1;
}

#define FONT_DRAW_POLY_GT4(icon, x, y, primMem, ot, c0, c1, c2, c3, transparency, scale)                                                                \
	DecalHUD_DrawPolyGT4((icon), (x), (y), (primMem), (ot), ColorCode_Load(&(c0)), ColorCode_Load(&(c1)), ColorCode_Load(&(c2)), ColorCode_Load(&(c3)), \
	                     (transparency), (scale))

s32 DecalFont_GetLineWidthStrlen(char *str, s16 len, s16 fontType)
{
	s16 width = 0;
	while (*str != 0 && len != 0)
	{
		u8 c = *str;
		if (DecalFont_InlineColor((const u8 *)str, len) >= 0)
		{
			str += 3;
			len -= 3;
			continue;
		}
		if (c == '@' || c == '[' || c == '^' || c == '*')
		{
			// NOTE(aalhendi): @, [, ^ and * encode circle, square, triangle and cross.
			width += FONT_CHAR_WIDTH[fontType] + FONT_BUTTON_WIDTH[fontType];
		}
		else if (c == ':' || c == '.')
		{
			width += FONT_PUNCT_WIDTH[fontType];
		}
		else if (c > FONT_PAL_TILDE)
		{
			width += FONT_CHAR_WIDTH[fontType];
		}
		str++;
		len--;
	}
	return width;
}


s32 DecalFont_GetLineWidth(char *str, s16 fontType)
{
	return (s16)DecalFont_GetLineWidthStrlen(str, -1, fontType);
}


static void DecalFont_DrawLineStrlenScaled(char *str, s16 len, s16 posX, s16 posY, s16 fontType, s16 flags, s32 scale)
{
	s16 x = posX;
	u8 *character;

	if (flags & JUSTIFY_CENTER)
	{
		x = posX - ((s16)(DecalFont_GetLineWidthStrlen(str, len, fontType) * scale / FP(1.0)) >> 1);
	}
	else if (flags & JUSTIFY_RIGHT)
	{
		x = posX - DecalFont_GetLineWidthStrlen(str, len, fontType) * scale / FP(1.0);
	}

	const s16 startX = x;
	int advance = 0;
	flags &= 0xfff;
	for (character = (u8 *)str; *character != 0 && len != 0; character++, len--)
	{
		int inlineColor = DecalFont_InlineColor(character, len);
		if (inlineColor >= 0)
		{
			flags = (s16)inlineColor;
			character += 2;
			len -= 2;
			continue;
		}
		s16 extraX = 0;
		u16 iconID = 0xff;
		s16 extraY = 0;
		s16 iconGroupID = FONT_ICON_GROUP[fontType];
		Color *colors = (Color *)FONT_COLORS[flags];
		s16 charWidth = FONT_CHAR_WIDTH[fontType];
		s16 iconScale = FP(1.0);
		int copies = 1;
		int copyOffset = 0;
		bool inverted = false;
		s32 rawCode = *character;
		// NOTE(aalhendi): Keep the raw byte separate from the classifier's retail scratch register.
		register s32 c CTR_PSX_REGISTER("$3") = rawCode;

		if (c == ':' || c == '.')
		{
			charWidth = FONT_PUNCT_WIDTH[fontType];
		}
		else if (c == '@' || c == '[' || c == '^' || c == '*')
		{
			// NOTE(aalhendi): Button glyphs have their own scale, baseline and neutral palette.
			iconScale = FONT_BUTTON_SCALE[fontType];
			extraY = FONT_BUTTON_HEIGHT[fontType];
			charWidth = FONT_CHAR_WIDTH[fontType] + FONT_BUTTON_WIDTH[fontType];
			colors = (Color *)FONT_COLORS[GRAY];
		}

		c = rawCode;
		if (c < 3)
		{
			// NOTE(aalhendi): Bytes 1 and 2 select zero-advance accent placeholders.
			s32 indentBase = fontType * 2 - 1;
			s16 *dimensions = FONT_INDENT_DIMENSIONS + fontType * 2;
			iconID = (u8)FONT_INDENT_ICONS[c + indentBase];
			extraX = dimensions[0];
			extraY = dimensions[1];
			charWidth = 0;
		}
		else if (c == FONT_PAL_TILDE)
		{
			// PAL composes Ñ from a reduced underscore followed by N.
			iconID = FONT_UNDERSCORE_ICON;
			extraX = s_palTilde[fontType][0];
			extraY = s_palTilde[fontType][1];
			iconScale = s_palTilde[fontType][2];
			charWidth = 0;
		}
		else if (c == '"')
		{
			iconID = FONT_APOSTROPHE_ICON;
			extraX = s_palQuoteOffset[fontType];
			copyOffset = s_palQuoteSpacing[fontType];
			copies = 2;
		}
		else if (c == '#' || c == '$')
		{
			iconID = c == '#' ? FONT_EXCLAMATION_ICON : FONT_QUESTION_ICON;
			extraX = s_palInvertedOffset[fontType][0];
			extraY = s_palInvertedOffset[fontType][1];
			inverted = true;
		}
		else if (c == '&')
		{
			iconID = FONT_PERIOD_ICON;
			extraX = s_palOrdinalOffset[fontType][0];
			extraY = s_palOrdinalOffset[fontType][1];
		}
		else if ((u32)(c - 0x21) < 0xdf && iconID == 0xff)
		{
			iconID = FONT_CHARACTER_ICONS[c - 0x21];
		}

		if (iconID != 0xff)
		{
			struct GameTracker *gGT;
			struct IconGroup *group;
			if (iconID > 0x7f)
			{
				// NOTE(aalhendi): High IDs select the reserved kana groups, not the normal font atlas.
				s16 kanaGroup;
				iconID -= 0x80;
				kanaGroup = 15;
				if (iconGroupID == 4)
				{
					kanaGroup = 14;
				}
				iconGroupID = kanaGroup;
			}
			gGT = GAME_TRACKER;
			group = gGT->iconGroup[iconGroupID];
			// NOTE(aalhendi): Native can boot before every retail icon group is loaded.
			if (group != NULL)
			{
				if (iconID < group->numIcons)
				{
					struct Icon **icons = ICONGROUP_GETICONS(group);
					for (int i = 0; i < copies; i++)
					{
						int glyphX = x + (extraX + i * copyOffset) * scale / FP(1.0);
						int glyphY = posY + extraY * scale / FP(1.0);
						if (inverted)
						{
							DecalHUD_Arrow2D(icons[iconID], glyphX, glyphY, &gGT->backBuffer->primMem, gGT->pushBuffer_UI.ptrOT,
							                 ColorCode_Load(&colors[2]), ColorCode_Load(&colors[3]), ColorCode_Load(&colors[0]),
							                 ColorCode_Load(&colors[1]), 0, iconScale * scale / FP(1.0), 0x800);
						}
						else
						{
							FONT_DRAW_POLY_GT4(icons[iconID], glyphX, glyphY, &gGT->backBuffer->primMem,
							                   gGT->pushBuffer_UI.ptrOT, colors[0], colors[1], colors[2], colors[3],
							                   0, iconScale * scale / FP(1.0));
						}
					}
				}
			}
		}
		advance += charWidth;
		x = startX + advance * scale / FP(1.0);
	}
}

void DecalFont_DrawLineStrlen(char *str, s16 len, s16 posX, s16 posY, s16 fontType, s16 flags)
{
	DecalFont_DrawLineStrlenScaled(str, len, posX, posY, fontType, flags, FP(1.0));
}

void DecalFont_DrawLineScaledOT(char *str, s16 posX, s16 posY, s16 fontType, s16 flags, s32 scale, u32 *ot)
{
	u32 *backupOT = GAME_TRACKER->pushBuffer_UI.ptrOT;
	GAME_TRACKER->pushBuffer_UI.ptrOT = ot;
	DecalFont_DrawLineStrlenScaled(str, -1, posX, posY, fontType, flags, scale);
	GAME_TRACKER->pushBuffer_UI.ptrOT = backupOT;
}


void DecalFont_DrawLine(char *str, s16 posX, s16 posY, s16 fontType, s16 flags)
{
	DecalFont_DrawLineStrlen(str, -1, (s16)posX, (s16)posY, fontType, (s16)flags);
}


void DecalFont_DrawLineOT(char *str, s16 posX, s16 posY, s16 fontType, s16 flags, u32 *ot)
{
	struct GameTracker *gGT;
	u32 *backupOT;

	gGT = GAME_TRACKER;

	backupOT = gGT->pushBuffer_UI.ptrOT;
	gGT->pushBuffer_UI.ptrOT = ot;

	DecalFont_DrawLine(str, (s16)posX, (s16)posY, fontType, (s16)flags);

	// NOTE(aalhendi): Reload the tracker after drawing before restoring its UI table.
	GAME_TRACKER->pushBuffer_UI.ptrOT = backupOT;
}


s32 DecalFont_DrawMultiLineStrlen(char *str, s16 len, s16 posX, s16 posY, s16 maxPixLen, s16 fontType, s16 flags)
{
	s16 totalHeight = 0;
	char *cursor;
	char *lineStart;
	s16 remainingAtBreak;
	s16 lineWidth;

	do
	{
		cursor = str;
		remainingAtBreak = len;
		lineStart = str;
		// NOTE(aalhendi): CTR uses carriage returns for explicit line breaks.
		if (*str != '\r')
		{
			while (1)
			{
				if (*cursor == ' ' && len != 0)
				{
					cursor++;
					len--;
				}
				if (*cursor == 0 || len == 0)
				{
					break;
				}
				if (*cursor != ' ' && *cursor != '\r')
				{
					// NOTE(aalhendi): The word scanner keeps its own terminators and
					// short budget snapshot to retain the retail loop layout.
					s32 wordSpace = ' ';
					s32 wordReturn = '\r';
					while (1)
					{
						s16 remaining = len;
						if (remaining == 0)
						{
							break;
						}
						cursor++;
						len--;
						if (*cursor == 0 || *cursor == wordSpace || *cursor == wordReturn)
						{
							break;
						}
					}
				}
				lineWidth = DecalFont_GetLineWidthStrlen(lineStart, (s16)(cursor - lineStart), fontType);
				if (maxPixLen <= lineWidth)
				{
					break;
				}
				// NOTE(aalhendi): Accept only complete words that fit; retain their remaining byte budget.
				str = cursor;
				remainingAtBreak = len;
				if (*cursor == '\r')
				{
					break;
				}
			}
		}

		DecalFont_DrawLineStrlen(lineStart, (s16)(str - lineStart), posX, posY + totalHeight, fontType, flags);
		len = remainingAtBreak;
		totalHeight += FONT_CHAR_HEIGHT[fontType];
		if (*str == 0)
		{
			break;
		}
		if (len != 0)
		{
			str++;
			len--;
		}
		if (*str == 0 || len == 0)
		{
			break;
		}
	} while (1);
	return totalHeight;
}


s32 DecalFont_DrawMultiLine(char *str, s16 posX, s16 posY, s16 maxPixLen, s16 fontType, s16 flags)
{
	return (s16)DecalFont_DrawMultiLineStrlen(str, -1, (s16)posX, (s16)posY, (s16)maxPixLen, fontType, (s16)flags);
}
