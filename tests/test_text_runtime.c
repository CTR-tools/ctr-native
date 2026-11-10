#include <shared/ctr_test.h>
#include <common.h>
#include <platform/native_text_runtime.h>
#include <platform/native_language_disc.h>

struct sData sdata_static;
NativeConfig g_config;
static int s_discActive;
int LOAD_RetailDiscActive(void) { return s_discActive; }

static const char *s_english;
static const char *s_italian;
static const char *s_englishOverride;
static const char *s_italianOverride;
static const char *s_italianRetail;

static FILE *TextTest_File(const char *text)
{
	if (text == NULL)
	{
		return NULL;
	}
	FILE *file = tmpfile();
	if (file != NULL)
	{
		fwrite(text, 1, strlen(text), file);
		rewind(file);
	}
	return file;
}

const char *NativeAssets_GetExecutableDir(void) { return "fixture"; }

static FILE *TextTest_OpenShipped(const char *path, const char *mode)
{
	(void)mode;
	if (strcmp(path, "fixture/resources/text/en/expanded.toml") == 0)
	{
		return TextTest_File(s_english);
	}
	if (strcmp(path, "fixture/resources/text/it/expanded.toml") == 0)
	{
		return TextTest_File(s_italian);
	}
	if (strcmp(path, "fixture/resources/text/it/retail.toml") == 0)
	{
		return TextTest_File(s_italianRetail);
	}
	return NULL;
}

FILE *NativeAssets_OpenHost(const char *path, const char *mode)
{
	(void)mode;
	const char *text = NULL;
	if (strcmp(path, "text/en/expanded.toml") == 0)
	{
		text = s_englishOverride;
	}
	else if (strcmp(path, "text/it/expanded.toml") == 0)
	{
		text = s_italianOverride;
	}
	return TextTest_File(text);
}

#include <platform/native_text_catalog.c>
#define fopen TextTest_OpenShipped
#include <platform/native_text_runtime.c>
#undef fopen
#include <game/LANG.c>
#include <game/RECTMENU_Text.c>

s32 DecalFont_GetLineWidth(char *text, s16 fontType)
{
	(void)fontType;
	return (s32)strlen(text);
}

static void TextTest_Reload(int slot)
{
	LANG_Refresh(slot);
}

int main(void)
{
	CTR_TEST_SECTION("Selected and English catalogs");
	s_english = "MODE = 'Mode'\nCHAOS = 'Chaos'\n";
	s_italian = "MODE = 'Modalita'\nCHAOS = 'Caos'\n";
	s_italianRetail = "PAUSED = 'PAUSA'\nRESTART = '\xc3\xa0'\nDRIVERS_CHUMP = 'TEST %s'\n";
	CTR_TEST_CHECK(NativeTextRuntime_HasRetail(NATIVE_LANGUAGE_ITALIAN), "a usable shipped retail catalog makes Italian available without a disc");
	CTR_TEST_CHECK(!NativeTextRuntime_HasRetail(NATIVE_LANGUAGE_FRENCH), "a missing retail catalog does not make a language available");
	char *retailTable[LNG_RETAIL_STRING_COUNT] = {0};
	retailTable[LNG_PAUSED] = "PAUSED";
	retailTable[LNG_RESTART] = "RESTART";
	retailTable[LNG_DRIVERS_CHUMP] = "%s CHUMP";
	sdata->lngStrings = retailTable;
	sdata->numLngStrings = LNG_RETAIL_STRING_COUNT;
	TextTest_Reload(NATIVE_LANGUAGE_ITALIAN);
	CTR_TEST_CHECK(strcmp(retailTable[LNG_PAUSED], "PAUSA") == 0 && strcmp(LANG_GetString(LNG_PAUSED), "PAUSA") == 0,
	               "retail table and resolver use the selected catalog");
	CTR_TEST_CHECK(strcmp(retailTable[LNG_RESTART], "RESTART") == 0 && strcmp(retailTable[LNG_DRIVERS_CHUMP], "TEST %s") == 0,
	               "unsupported text falls back and matching format strings are applied");
	CTR_TEST_CHECK(!LANG_FormatsMatch("%s %d", "%d %s") && !LANG_FormatsMatch("%s", "%n") &&
	               !LANG_FormatsMatch("%s", "%s %d") && !LANG_FormatsMatch("%s", "%"),
	               "changed, dangerous, extra and incomplete format directives are rejected");
	CTR_TEST_CHECK(strcmp(NativeTextRuntime_Get(NATIVE_TEXT_SELECTED_SHIPPED, LNG_PAUSED), "PAUSA") == 0,
	               "selected retail catalog uses the shared TOML loader");
	CTR_TEST_CHECK(NativeTextRuntime_Get(NATIVE_TEXT_SELECTED_SHIPPED, LNG_RESTART) == NULL,
	               "unsupported retail text is not published");
	s_discActive = 1;
	g_config.retailTextPriority = RETAIL_TEXT_PRIORITY_ISO_EU;
	TextTest_Reload(NATIVE_LANGUAGE_ITALIAN);
	CTR_TEST_CHECK(strcmp(retailTable[LNG_PAUSED], "PAUSED") == 0, "disc preference restores disc text");
	s_discActive = 0;
	TextTest_Reload(NATIVE_LANGUAGE_ITALIAN);
	CTR_TEST_CHECK(strcmp(retailTable[LNG_PAUSED], "PAUSA") == 0, "disc preference falls back to the catalog when the disc is unavailable");
	g_config.retailTextPriority = RETAIL_TEXT_PRIORITY_EXPANDED;
	s_italianRetail = NULL;
	TextTest_Reload(NATIVE_LANGUAGE_ITALIAN);
	CTR_TEST_CHECK(strcmp(retailTable[LNG_PAUSED], "PAUSED") == 0, "removing a translation restores the original retail value");
	sdata->lngStrings = NULL;
	sdata->numLngStrings = 0;
	const char *stable = NativeTextRuntime_Get(NATIVE_TEXT_SELECTED_SHIPPED, LNG_EXPANDED_MODE);
	CTR_TEST_CHECK(stable != NULL && strcmp(stable, "Modalita") == 0, "selected catalog is loaded");
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_MODE), "Modalita") == 0, "central resolver returns selected text");
	CTR_TEST_CHECK(strcmp(NativeTextRuntime_Get(NATIVE_TEXT_ENGLISH_SHIPPED, LNG_EXPANDED_MODE), "Mode") == 0, "English catalog remains available");
	s_italian = "MODE = 'Nuovo'\nCHAOS = '\xc3\xa0'\n";
	TextTest_Reload(NATIVE_LANGUAGE_ITALIAN);
	CTR_TEST_CHECK(stable == NativeTextRuntime_Get(NATIVE_TEXT_SELECTED_SHIPPED, LNG_EXPANDED_MODE) && strcmp(stable, "Nuovo") == 0,
	               "reloading preserves the published buffer address");
	CTR_TEST_CHECK(NativeTextRuntime_Get(NATIVE_TEXT_SELECTED_SHIPPED, LNG_EXPANDED_CHAOS) == NULL, "unsupported text is not published");
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_CHAOS), "Chaos") == 0, "central resolver falls back to English");
	CTR_TEST_CHECK(strcmp(NativeTextRuntime_Get(NATIVE_TEXT_ENGLISH_SHIPPED, LNG_EXPANDED_CHAOS), "Chaos") == 0, "English remains available for fallback");
	s_italian = "MODE = \"broken\n";
	TextTest_Reload(NATIVE_LANGUAGE_ITALIAN);
	CTR_TEST_CHECK(NativeTextRuntime_Get(NATIVE_TEXT_SELECTED_SHIPPED, LNG_EXPANDED_MODE) == NULL, "invalid selected catalog is not published");
	s_italian = "MODE = 'Distribuito'\n";
	s_italianOverride = "MODE = 'Personale'\n";
	s_englishOverride = "MODE = 'Custom'\nCHAOS = 'Custom chaos'\n";
	TextTest_Reload(NATIVE_LANGUAGE_ITALIAN);
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_MODE), "Personale") == 0, "selected override has first priority");
	s_italianOverride = NULL;
	TextTest_Reload(NATIVE_LANGUAGE_ITALIAN);
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_MODE), "Distribuito") == 0, "selected shipped text precedes English override");
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_CHAOS), "Custom chaos") == 0, "English override fills missing selected text");
	s_englishOverride = NULL;
	TextTest_Reload(NATIVE_LANGUAGE_ITALIAN);
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_CHAOS), "Chaos") == 0, "English shipped text is the final fallback");
	s_english = "PATCH_IMPORTED_SKIPPED = '{tracks} TRACKS {skipped_music} SKIPPED'\n";
	s_italian = "PATCH_IMPORTED_SKIPPED = '{tracks} PISTE'\n";
	TextTest_Reload(NATIVE_LANGUAGE_ITALIAN);
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_PATCH_IMPORTED_SKIPPED), "{tracks} TRACKS {skipped_music} SKIPPED") == 0,
	               "a translation missing a placeholder falls back to English");
	TextTest_Reload(NATIVE_LANGUAGE_ENGLISH);
	CTR_TEST_CHECK(NativeTextRuntime_Get(NATIVE_TEXT_SELECTED_SHIPPED, LNG_EXPANDED_MODE) == NULL, "English selection does not reuse old translations");
	s_english = NULL;
	TextTest_Reload(NATIVE_LANGUAGE_ENGLISH);
	CTR_TEST_CHECK(strcmp(LANG_GetString(LNG_EXPANDED_MODE), "MODE") == 0, "missing English uses a visible key as recovery");
	return CtrTest_Summary();
}
