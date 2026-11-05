#ifndef CTR_TRACK_TEXT_H
#define CTR_TRACK_TEXT_H

#include <common.h>

// Track metadata is UTF-8, but the retail font only accepts its own glyph set.
static inline void TrackText_Copy(char *destination, size_t capacity, const char *source)
{
	if (capacity == 0)
	{
		return;
	}
	size_t length = 0;
	while (*source != '\0' && length + 1 < capacity)
	{
		unsigned char c = (unsigned char)*source++;
		if (c >= 0x80 && c < 0xc0)
		{
			continue;
		}
		if (c < 0x20 || c >= 0x7f || c == '@' || c == '[' || c == '^' || c == '*')
		{
			c = '?';
		}
		destination[length++] = (char)c;
	}
	destination[length] = '\0';
}

static inline void TrackText_Trim(char *text, int width, s16 font)
{
	size_t length = strlen(text);
	while (length > 0 && DecalFont_GetLineWidth(text, font) > width)
	{
		text[--length] = '\0';
	}
}

#endif
