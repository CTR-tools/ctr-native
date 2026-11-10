#include <shared/ctr_test.h>
#include <common.h>

struct sData sdata_static;
struct Data data;
NativeConfig g_config;
static struct GameTracker s_tracker;
static struct DB s_buffer;
static struct Icon s_icon;
static struct { struct IconGroup group; struct Icon *icons[48]; } s_atlas;
static POLY_GT4 s_glyphs[16];
static u32 s_originalOT[5];
static u32 s_targetOT[5];

#include <platform/native_gpu_links.c>
#include <game/widescreen.c>
#include <game/DecalHUD.c>
#include <game/DecalFont.c>

void NativePgxp_BindWrittenXY(const void *destination, u32 value)
{
	(void)destination;
	(void)value;
}

static void ResetDrawing(void)
{
	memset(s_glyphs, 0, sizeof(s_glyphs));
	s_buffer.primMem.cursor = s_glyphs;
	for (int i = 0; i < 5; i++)
	{
		s_originalOT[i] = s_targetOT[i] = NATIVE_GPU_LINK_TERMINATOR;
	}
}

static int GlyphWidth(int index)
{
	return s_glyphs[index].x1 - s_glyphs[index].x0;
}

int main(void)
{
	static u32 colors[4];
	sdata->gGT = &s_tracker;
	s_tracker.backBuffer = &s_buffer;
	s_tracker.pushBuffer_UI.ptrOT = s_originalOT;
	s_atlas.group.numIcons = 48;
	s_atlas.icons[0] = &s_icon;
	s_atlas.icons[0x2f] = &s_icon;
	s_atlas.icons[0x24] = &s_icon;
	s_atlas.icons[0x25] = &s_icon;
	s_atlas.icons[0x2c] = &s_icon;
	s_atlas.icons[0x2e] = &s_icon;
	s_tracker.iconGroup[0] = &s_atlas.group;
	s_icon.texLayout.u1 = s_icon.texLayout.u3 = 12;
	s_icon.texLayout.v2 = s_icon.texLayout.v3 = 10;
	data.ptrColor[ORANGE] = colors;
	data.font_charPixWidth[FONT_SMALL] = 13;
	data.trigApprox[0].cos = FP(1.0);
	NativeGpuLinks_RegisterRangeChecked("font test primitives", s_glyphs, sizeof(s_glyphs));

	CTR_TEST_SECTION("scaled font emits real geometry with alignment and ordering-table ownership");
	ResetDrawing();
	DecalFont_DrawLineScaledOT("AB", 100, 40, FONT_SMALL, JUSTIFY_RIGHT | ORANGE, FP(0.5), s_targetOT);
	CTR_TEST_CHECK(s_buffer.primMem.cursor == s_glyphs + 2, "scaled drawing emits both glyph packets");
	CTR_TEST_CHECK_EQ(s_glyphs[0].x0, 87, "the right-aligned origin uses the scaled total width");
	CTR_TEST_CHECK_EQ(s_glyphs[1].x0, 93, "glyph positions scale the cumulative advance");
	CTR_TEST_CHECK_EQ(s_glyphs[0].y0, 40, "scaling preserves the requested baseline");
	CTR_TEST_CHECK_EQ(GlyphWidth(0), 6, "half-scale glyph geometry has half the atlas width in 4:3");
	CTR_TEST_CHECK_EQ(s_glyphs[0].y2 - s_glyphs[0].y0, 5, "half-scale glyph geometry has half the atlas height");
	CTR_TEST_CHECK(NativeGpuLinks_ToHostPointer(s_targetOT[0]) == &s_glyphs[1], "glyphs are linked into the requested ordering table");
	CTR_TEST_CHECK(NativeGpuLinks_ToHostPointer(s_glyphs[1].tag & 0xffffffu) == &s_glyphs[0], "glyphs retain their primitive chain");
	CTR_TEST_CHECK_EQ(s_originalOT[0], NATIVE_GPU_LINK_TERMINATOR, "the caller's ordering table receives no glyph packets");
	CTR_TEST_CHECK(s_tracker.pushBuffer_UI.ptrOT == s_originalOT, "scaled drawing restores the caller's ordering table");

	ResetDrawing();
	DecalFont_DrawLineScaledOT("AB", 100, 40, FONT_SMALL, JUSTIFY_CENTER | ORANGE, FP(0.5), s_targetOT);
	CTR_TEST_CHECK_EQ(s_glyphs[0].x0, 94, "center alignment uses half the scaled width");
	ResetDrawing();
	data.font_indentPixDimensions[FONT_SMALL * 2] = 2;
	data.font_indentPixDimensions[FONT_SMALL * 2 + 1] = 4;
	DecalFont_DrawLineScaledOT("\1A", 100, 40, FONT_SMALL, ORANGE, FP(0.5), s_targetOT);
	CTR_TEST_CHECK_EQ(s_glyphs[0].x0, 101, "accent horizontal offsets scale with their glyphs");
	CTR_TEST_CHECK_EQ(s_glyphs[0].y0, 42, "accent vertical offsets scale with their glyphs");
	CTR_TEST_CHECK_EQ(s_glyphs[1].x0, 100, "accent placeholders still have zero advance");
	ResetDrawing();
	DecalFont_DrawLineScaledOT("\3N", 100, 40, FONT_SMALL, JUSTIFY_RIGHT | ORANGE, FP(1.0), s_targetOT);
	CTR_TEST_CHECK_EQ(DecalFont_GetLineWidth("\3N", FONT_SMALL), 13, "PAL tilde does not affect text width");
	CTR_TEST_CHECK(s_buffer.primMem.cursor == s_glyphs + 2, "PAL tilde and its letter both draw");
	CTR_TEST_CHECK_EQ(s_glyphs[0].x0, 89, "PAL tilde is positioned above the following letter");
	CTR_TEST_CHECK_EQ(s_glyphs[1].x0, 87, "PAL tilde does not advance the following letter");
	CTR_TEST_CHECK_EQ(s_glyphs[0].y0, 35, "the small tilde uses the PAL vertical offset");
	CTR_TEST_CHECK_EQ(GlyphWidth(0), 9, "the small tilde uses three-quarter PAL scale");
	data.font_charPixWidth[FONT_BIG] = 17;
	ResetDrawing();
	DecalFont_DrawLineOT("\3N", 100, 40, FONT_BIG, ORANGE, s_targetOT);
	CTR_TEST_CHECK_EQ(s_glyphs[0].y0, 29, "the big tilde uses its PAL vertical offset");
	CTR_TEST_CHECK_EQ(GlyphWidth(0), 9, "the big tilde uses the PAL scale");
	ResetDrawing();
	DecalFont_DrawLineOT("A\3NO", 0, 40, FONT_SMALL, ORANGE, s_targetOT);
	CTR_TEST_CHECK_EQ(s_glyphs[2].x0, 13, "the N immediately follows the preceding letter");
	CTR_TEST_CHECK_EQ(s_glyphs[3].x0, 26, "the letter after N retains normal spacing");

	CTR_TEST_SECTION("PAL punctuation and inline colors");
	data.font_charPixHeight[FONT_SMALL] = 10;
	ResetDrawing();
	DecalFont_DrawLineOT("\"A", 20, 40, FONT_SMALL, ORANGE, s_targetOT);
	CTR_TEST_CHECK(s_buffer.primMem.cursor == s_glyphs + 3, "quotation marks draw two apostrophes and one following glyph");
	CTR_TEST_CHECK_EQ(s_glyphs[1].x0 - s_glyphs[0].x0, 4, "quotation marks use small-font apostrophe spacing");
	CTR_TEST_CHECK_EQ(s_glyphs[2].x0, 33, "quotation marks advance by one character");
	ResetDrawing();
	DecalFont_DrawLineOT("#$&", 20, 40, FONT_SMALL, ORANGE, s_targetOT);
	CTR_TEST_CHECK(s_buffer.primMem.cursor == s_glyphs + 3, "Spanish punctuation and ordinal indicator draw");
	CTR_TEST_CHECK(s_glyphs[0].x0 > s_glyphs[0].x1 && s_glyphs[1].x0 > s_glyphs[1].x1,
	               "inverted punctuation rotates the glyphs by half a turn");
	CTR_TEST_CHECK_EQ(s_glyphs[2].y0, 37, "ordinal indicator uses the PAL vertical offset");
	static u32 alternateColors[4] = {0x00000012, 0x00000012, 0x00000012, 0x00000012};
	data.ptrColor[WHITE] = alternateColors;
	ResetDrawing();
	DecalFont_DrawLineOT("A~04B", 100, 40, FONT_SMALL, JUSTIFY_RIGHT | ORANGE, s_targetOT);
	CTR_TEST_CHECK_EQ(DecalFont_GetLineWidth("A~04B", FONT_SMALL), 26, "inline color markers do not add width");
	CTR_TEST_CHECK_EQ(s_glyphs[0].x0, 74, "alignment ignores inline color markers");
	CTR_TEST_CHECK(s_buffer.primMem.cursor == s_glyphs + 2 && s_glyphs[1].r0 == 0x12,
	               "inline color changes the following glyph without drawing the marker");
	CTR_TEST_CHECK_EQ(DecalFont_GetLineWidthStrlen("~04A", 2, FONT_SMALL), 26,
	                  "a truncated color marker is not consumed");
	CTR_TEST_CHECK_EQ(DecalFont_GetLineWidth("~99A", FONT_SMALL), 52,
	                  "out-of-range color markers do not index the palette");

	CTR_TEST_SECTION("small glyphs retain widescreen compression after fractional scaling");
	const int halfWidths[] = { 6, 5, 5, 3 };
	const int fractionalWidths[] = { 6, 5, 5, 4 };
	const int fullWidths[] = { 13, 11, 11, 9 };
	for (int aspect = 0; aspect < 4; aspect++)
	{
		g_config.aspectRatio = aspect;
		s_icon.texLayout.u1 = s_icon.texLayout.u3 = 12;
		ResetDrawing();
		DecalFont_DrawLineScaledOT("AB", 100, 40, FONT_SMALL, JUSTIFY_RIGHT | ORANGE, FP(0.5), s_targetOT);
		CTR_TEST_CHECK_EQ(GlyphWidth(0), halfWidths[aspect], "half-size glyphs use the requested display proportions");
		CTR_TEST_CHECK_EQ(s_glyphs[1].x0 - s_glyphs[0].x0, 6, "aspect correction preserves the text's cumulative advance");
		CTR_TEST_CHECK_EQ(s_glyphs[0].y0, 40, "aspect correction preserves the text baseline");
		CTR_TEST_CHECK_EQ(s_glyphs[0].y2 - s_glyphs[0].y0, 5, "aspect correction does not alter glyph height");
		CTR_TEST_CHECK_EQ(s_glyphs[0].x2, s_glyphs[0].x0, "the lower left edge follows the corrected upper edge");
		CTR_TEST_CHECK_EQ(s_glyphs[0].x3, s_glyphs[0].x1, "the lower right edge follows the corrected upper edge");

		// 13 * .49 truncates to 6; combining the transforms first rounds 21:9 to 4, not 3.
		s_icon.texLayout.u1 = s_icon.texLayout.u3 = 13;
		ResetDrawing();
		DecalFont_DrawLineScaledOT("A", 100, 40, FONT_SMALL, ORANGE, FP(0.49), s_targetOT);
		CTR_TEST_CHECK_EQ(GlyphWidth(0), fractionalWidths[aspect], "fractional scale and aspect are combined before rounding");

		ResetDrawing();
		DecalFont_DrawLineOT("AB", 100, 40, FONT_SMALL, JUSTIFY_RIGHT | ORANGE, s_targetOT);
		CTR_TEST_CHECK_EQ(GlyphWidth(0), fullWidths[aspect], "ordinary drawing retains its original aspect-corrected geometry");
		CTR_TEST_CHECK_EQ(s_glyphs[1].x0 - s_glyphs[0].x0, 13, "ordinary drawing retains the full character advance");
		CTR_TEST_CHECK(s_tracker.pushBuffer_UI.ptrOT == s_originalOT, "ordinary drawing restores the ordering table");
	}
	return CtrTest_Summary();
}
