#include <shared/ctr_test.h>
#include <track_text.h>

struct sData sdata_static;

s32 DecalFont_GetLineWidth(char *text, s16 font)
{
	(void)font;
	return (s32)strlen(text);
}

int main(void)
{
	CTR_TEST_SECTION("Track metadata text");
	char text[32];
	TrackText_Copy(text, sizeof(text), "A\xc3\xa9 @[^*\nZ");
	CTR_TEST_CHECK(strcmp(text, "A? ?????Z") == 0, "unsupported glyphs are replaced and UTF-8 continuation bytes skipped");
	TrackText_Trim(text, 4, FONT_SMALL);
	CTR_TEST_CHECK(strcmp(text, "A? ?") == 0, "text is truncated to the available font width");
	char shortText[4];
	TrackText_Copy(shortText, sizeof(shortText), "hello");
	CTR_TEST_CHECK(strcmp(shortText, "hel") == 0, "copy leaves room for the terminator");
	char untouched = 'x';
	TrackText_Copy(&untouched, 0, "hello");
	CTR_TEST_CHECK(untouched == 'x', "zero capacity does not write to the destination");
	return CtrTest_Summary();
}
