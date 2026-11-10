#include <shared/ctr_test.h>
#include <common.h>

struct sData sdata_static;
NativeConfig g_config;
int LOAD_RetailDiscActive(void) { return 0; }

#include <platform/native_text_runtime.h>
void NativeTextRuntime_Reload(int slot) { (void)slot; }
static const char *const s_testExpanded[] = {"Mode", "Chaos", "Regular", "Chaos Rules", "Super Hard", "Ultra Hard",
                                             "PRESS START TO SKIP", "NO PREVIEW", "CHAOS SETUP", "CHAOS EFFECTS",
	                                             "Duration", "Trigger", "Interval",
	                                             "Effects", "Confirm", "Active Effects", "Random", "Pickup", "External",
	                                             "Automatic", "Disabled", "SELECT DEVICE", "DEVICE", "Vibration",
	                                             "Deadzone", "Restore Defaults", "None"};
const char *NativeTextRuntime_Get(enum NativeTextSource source, int key)
{
	int index = key - LNG_EXPANDED_STRING_BASE;
	return source == NATIVE_TEXT_ENGLISH_SHIPPED && index >= 0 && index < (int)len(s_testExpanded)
	           ? s_testExpanded[index] : NULL;
}
const char *NativeTextCatalog_KeyName(int key) { (void)key; return ""; }

#include <game/LANG.c>
#include <game/RECTMENU_Text.c>

// Keep font metrics deterministic while exercising the actual width traversal.
s32 DecalFont_GetLineWidth(char *text, s16 fontType)
{
	return (s32)strlen(text) * (fontType == FONT_BIG ? 2 : 1);
}

static void TestCustomStringsWithoutLanguageAsset(void)
{
	CTR_TEST_SECTION("custom menu IDs resolve before any language asset is loaded");
	memset(&sdata_static, 0, sizeof(sdata_static));
	LANG_Rebuild();
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_MODE), "Mode") == 0, "the prompt title needs no LNG allocation");
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_CHAOS), "Chaos") == 0, "the Chaos label is available at startup");
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_REGULAR), "Regular") == 0, "the Regular label is available at startup");
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_CHAOS_RULES), "Chaos Rules") == 0, "the pause action uses a stable custom ID");
	CTR_TEST_CHECK(strcmp(LANG_GetString((s16)(LNG_EXPANDED_CHAOS_RULES | MENU_ROW_LOCKED) & MENU_ROW_LNG_MASK), "Chaos Rules") == 0,
	               "a signed locked row resolves the same custom text");
	CTR_TEST_CHECK(strcmp(LANG_GetString(RECTMENU_STRING_NONE), "") == 0, "the terminator does not resolve as a custom string");
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_STRING_BASE + LNG_EXPANDED_STRING_COUNT), "") == 0,
	               "unknown custom IDs do not index beyond the catalogue");
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_RESUME), "") == 0, "retail text waits for the language asset");
	CTR_TEST_CHECK(strcmp(LANG_GetString(-1), "") == 0, "negative string identifiers are rejected");
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_CHAOS), "Chaos") == 0, "the central resolver handles Expanded identifiers");
}

static void TestRetailStringsAndLanguageChanges(void)
{
	CTR_TEST_SECTION("retail language changes leave custom IDs and the asset table intact");
	char *english[] = {"Paused", "Restart", "Resume"};
	char *italian[] = {"Pausa", "Ricomincia", "Riprendi"};
	sdata->lngStrings = english;
	sdata->numLngStrings = len(english);
	LANG_Rebuild();
	CTR_TEST_CHECK(LANG_GetString(LNG_RESUME) == english[LNG_RESUME], "retail IDs still resolve through the loaded asset");
	CTR_TEST_CHECK(LANG_GetString((s16)(LNG_RESTART | MENU_ROW_LOCKED) & MENU_ROW_LNG_MASK) == english[LNG_RESTART],
	               "locked retail rows retain their language text");
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_QUIT), "") == 0, "an out-of-range retail ID cannot access the table");
	sdata->lngStrings = italian;
	LANG_Rebuild();
	CTR_TEST_CHECK(LANG_GetString(LNG_RESUME) == italian[LNG_RESUME], "retail lookup follows a language change");
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_CHAOS_RULES), "Chaos Rules") == 0, "custom text is independent of language-table size and location");
	CTR_TEST_CHECK(sdata->lngStrings == italian && sdata->numLngStrings == 3, "lookup does not replace or extend the retail table");
}

static void TestMenuWidths(void)
{
	CTR_TEST_SECTION("menu widths use the same resolver for rows, titles and child menus");
	char *retail[] = {"Paused", "Restart", "Resume"};
	sdata->lngStrings = retail;
	sdata->numLngStrings = len(retail);
	LANG_Rebuild();
	struct MenuRow rows[] = {
		{LNG_RESUME, 0, 1, 0, 0},
		{(s16)(LNG_EXPANDED_CHAOS_RULES | MENU_ROW_LOCKED), 0, 0, 1, 1},
		{RECTMENU_STRING_NONE, 0, 0, 0, 0},
	};
	struct RectMenu menu = {.rows = rows, .stringIndexTitle = RECTMENU_STRING_NONE, .state = USE_SMALL_FONT};
	s16 width = 0;
	RECTMENU_GetWidth(&menu, &width, false);
	CTR_TEST_CHECK_EQ(width, 12, "a locked custom row contributes its visible text width");
	menu.stringIndexTitle = LNG_EXPANDED_REGULAR;
	menu.state |= BIG_TEXT_IN_TITLE;
	width = 0;
	RECTMENU_GetWidth(&menu, &width, false);
	CTR_TEST_CHECK_EQ(width, 15, "a big custom title can determine the menu width");
	struct RectMenu parent = {
		.rows = &rows[2], .stringIndexTitle = LNG_EXPANDED_MODE,
		.state = DRAW_NEXT_MENU_IN_HIERARCHY | USE_SMALL_FONT,
		.ptrNextBox_InHierarchy = &menu,
	};
	width = 0;
	RECTMENU_GetWidth(&parent, &width, false);
	CTR_TEST_CHECK_EQ(width, 5, "the parent-only measurement resolves its own custom title");
	width = 0;
	RECTMENU_GetWidth(&parent, &width, true);
	CTR_TEST_CHECK_EQ(width, 15, "recursive measurement resolves custom text in the child too");
}

static void TestRetailExpandedBoundary(void)
{
	CTR_TEST_SECTION("Expanded strings follow the fixed retail range");
	char *retail[LNG_RETAIL_STRING_COUNT] = {0};
	retail[LNG_RETAIL_STRING_COUNT - 1] = "Last retail";
	sdata->lngStrings = retail;
	sdata->numLngStrings = LNG_RETAIL_STRING_COUNT;
	LANG_Rebuild();
	CTR_TEST_CHECK_EQ(LNG_EXPANDED_MODE, LNG_RETAIL_STRING_COUNT, "the first Expanded ID immediately follows retail");
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_RETAIL_STRING_COUNT - 1), "Last retail") == 0, "the last retail string remains accessible");
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_MODE), "Mode") == 0, "the next ID resolves the first Expanded string");
	CTR_TEST_CHECK(sdata->lngStrings == retail && sdata->numLngStrings == LNG_RETAIL_STRING_COUNT,
	               "the resident retail table is not replaced or extended");
}

int main(void)
{
	TestCustomStringsWithoutLanguageAsset();
	TestRetailStringsAndLanguageChanges();
	TestMenuWidths();
	TestRetailExpandedBoundary();
	return CtrTest_Summary();
}
