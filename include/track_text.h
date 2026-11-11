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

enum
{
	TRACK_TEXT_SCROLL_PAUSE_FRAMES = 45,
	TRACK_TEXT_SCROLL_STEP_FRAMES = 8,
};

// Scroll long text horizontally and clip it to the available pixel width.
static inline void TrackText_Scroll(char *text, int available, s16 font, u32 frame)
{
	if (DecalFont_GetLineWidth(text, font) <= available)
	{
		return;
	}
	size_t length = strlen(text);
	size_t lastStart = 0;
	while (lastStart < length && DecalFont_GetLineWidth(text + lastStart, font) > available)
	{
		lastStart++;
	}
	u32 travelFrames = (u32)lastStart * TRACK_TEXT_SCROLL_STEP_FRAMES;
	u32 cycle = travelFrames + 2 * TRACK_TEXT_SCROLL_PAUSE_FRAMES;
	u32 position = frame % cycle;
	size_t start = 0;
	if (position >= TRACK_TEXT_SCROLL_PAUSE_FRAMES)
	{
		start = (position - TRACK_TEXT_SCROLL_PAUSE_FRAMES) / TRACK_TEXT_SCROLL_STEP_FRAMES;
		if (start > lastStart)
		{
			start = lastStart;
		}
	}
	memmove(text, text + start, length - start + 1);
	TrackText_Trim(text, available, font);
}

#endif
