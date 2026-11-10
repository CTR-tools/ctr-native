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
 static const char *s_italianRetailOverride;
static const char *s_italianName;
static const char *s_italianNameOverride;
static const char *s_frenchName;
static const char *s_frenchNameOverride;
static int s_nameOpenCount;

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
	if (strcmp(path, "fixture/resources/text/it/language.toml") == 0)
	{
		s_nameOpenCount++;
		return TextTest_File(s_italianName);
	}
	if (strcmp(path, "fixture/resources/text/fr/language.toml") == 0)
	{
		s_nameOpenCount++;
		return TextTest_File(s_frenchName);
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
	else if (strcmp(path, "text/it/retail.toml") == 0)
	{
		text = s_italianRetailOverride;
	}
	else if (strcmp(path, "text/it/language.toml") == 0)
	{
		s_nameOpenCount++;
		text = s_italianNameOverride;
	}
	else if (strcmp(path, "text/fr/language.toml") == 0)
	{
		s_nameOpenCount++;
		text = s_frenchNameOverride;
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
	CTR_TEST_SECTION("Language metadata");
	s_italianName = "self_name = 'ITALIANO'\n";
	s_italianNameOverride = "self_name = 'PERSONALE'\n";
	CTR_TEST_CHECK(strcmp(NativeTextRuntime_LanguageSelfName(NATIVE_LANGUAGE_ITALIAN), "PERSONALE") == 0,
	               "personal language metadata takes precedence over shipped metadata");
	int nameOpenCount = s_nameOpenCount;
	s_italianNameOverride = "self_name = 'CAMBIATO'\n";
	CTR_TEST_CHECK(strcmp(NativeTextRuntime_LanguageSelfName(NATIVE_LANGUAGE_ITALIAN), "PERSONALE") == 0 && s_nameOpenCount == nameOpenCount,
	               "language names are cached without opening files again");
	s_frenchName = "self_name = 'FRANCAIS'\n";
	s_frenchNameOverride = "self_name = 'INVALIDO \xc3\xa0'\n";
	CTR_TEST_CHECK(strcmp(NativeTextRuntime_LanguageSelfName(NATIVE_LANGUAGE_FRENCH), "FRANCAIS") == 0,
	               "unsupported personal names fall back to shipped metadata");
	CTR_TEST_CHECK(NativeTextRuntime_LanguageSelfName(NATIVE_LANGUAGE_GERMAN) == NULL && NativeTextRuntime_LanguageSelfName(-1) == NULL,
	               "missing metadata and unknown language slots have no name");

	CTR_TEST_SECTION("Selected and English catalogs");
	s_english = "MODE = 'Mode'\nCHAOS = 'Chaos'\n";
	s_italian = "MODE = 'Modalita'\nCHAOS = 'Caos'\n";
	s_italianRetail = "PAUSED = 'PAUSA'\nRESTART = '\xc3\xa0'\nDRIVERS_CHUMP = 'TEST %s'\n";
	CTR_TEST_CHECK(NativeTextRuntime_HasRetail(NATIVE_LANGUAGE_ITALIAN), "a usable shipped retail catalog makes Italian available without a disc");
	CTR_TEST_CHECK(!NativeTextRuntime_HasRetail(NATIVE_LANGUAGE_FRENCH), "a missing retail catalog does not make a language available");
	s_italianRetail = "";
	CTR_TEST_CHECK(NativeTextRuntime_HasRetail(NATIVE_LANGUAGE_ITALIAN), "an empty shipped catalog makes Italian available");
	s_italianRetail = "invalid TOML";
	CTR_TEST_CHECK(NativeTextRuntime_HasRetail(NATIVE_LANGUAGE_ITALIAN), "a malformed shipped catalog makes Italian available without parsing");
	s_italianRetail = "PAUSED = 'PAUSA'\nRESTART = '\xc3\xa0'\nDRIVERS_CHUMP = 'TEST %s'\n";
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
	s_italianRetailOverride = "PAUSED = 'PERSONALE'\nDRIVERS_CHUMP = 'ERRATO %d'\n";
	TextTest_Reload(NATIVE_LANGUAGE_ITALIAN);
	CTR_TEST_CHECK(strcmp(retailTable[LNG_PAUSED], "PERSONALE") == 0 && strcmp(retailTable[LNG_DRIVERS_CHUMP], "%s CHUMP") == 0,
	               "valid personal overrides precede disc text, while invalid format strings fall back");
	s_discActive = 0;
	TextTest_Reload(NATIVE_LANGUAGE_ITALIAN);
	CTR_TEST_CHECK(strcmp(retailTable[LNG_PAUSED], "PERSONALE") == 0 && strcmp(retailTable[LNG_DRIVERS_CHUMP], "TEST %s") == 0,
	               "personal overrides precede the shipped catalog and invalid overrides fall back per string");
	s_italianRetail = NULL;
	CTR_TEST_CHECK(NativeTextRuntime_HasRetail(NATIVE_LANGUAGE_ITALIAN), "a personal catalog makes text available without a disc");
	s_italianRetailOverride = "";
	CTR_TEST_CHECK(NativeTextRuntime_HasRetail(NATIVE_LANGUAGE_ITALIAN), "an empty personal catalog makes Italian available");
	s_italianRetail = "PAUSED = 'PAUSA'\n";
	s_italianRetailOverride = NULL;
	TextTest_Reload(NATIVE_LANGUAGE_ITALIAN);
	CTR_TEST_CHECK(strcmp(retailTable[LNG_PAUSED], "PAUSA") == 0, "removing personal overrides restores the shipped catalog");
	g_config.retailTextPriority = RETAIL_TEXT_PRIORITY_EXPANDED;
	s_italianRetail = NULL;
	TextTest_Reload(NATIVE_LANGUAGE_ITALIAN);
	CTR_TEST_CHECK(strcmp(retailTable[LNG_PAUSED], "PAUSED") == 0, "removing a translation restores the original retail value");
	s_italianRetailOverride = "PAUSED = \"broken\n";
	TextTest_Reload(NATIVE_LANGUAGE_ITALIAN);
	CTR_TEST_CHECK(strcmp(retailTable[LNG_PAUSED], "PAUSED") == 0, "a malformed personal catalog falls back to the original");
	s_italianRetailOverride = NULL;
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
