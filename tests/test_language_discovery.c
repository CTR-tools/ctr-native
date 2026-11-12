#include <SDL3/SDL.h>
#include <shared/ctr_test.h>
#include <shared/ctr_test_scratch.h>
#include <platform/native_assets.h>
#include <platform/native_config.h>
#include <platform/native_text_runtime.h>

static char s_base[512];
static char s_assets[512];

const char *NativeAssets_GetBaseDir(void) { return s_base; }
const char *NativeAssets_GetExecutableDir(void) { return s_base; }
const char *NativeAssets_GetAssetDir(void) { return s_assets; }

FILE *NativeAssets_OpenHost(const char *relativePath, const char *mode)
{
	char path[1024];
	int size = snprintf(path, sizeof(path), "%s/%s", s_assets, relativePath);
	return size < 0 || (size_t)size >= sizeof(path) ? NULL : fopen(path, mode);
}

#include <platform/native_config.c>
#include <platform/native_text_catalog.c>
#include <platform/native_text_runtime.c>

static void MakeDir(const char *relative)
{
	char path[1024];
	int size = snprintf(path, sizeof(path), "%s/%s", s_base, relative);
	if (size < 0 || (size_t)size >= sizeof(path) || !SDL_CreateDirectory(path))
	{
		CtrTest_Bail("cannot create language fixture directory");
	}
}

static void WriteFile(const char *relative, const char *text)
{
	char path[1024];
	int size = snprintf(path, sizeof(path), "%s/%s", s_base, relative);
	if (size < 0 || (size_t)size >= sizeof(path)) { CtrTest_Bail("fixture path too long"); }
	FILE *file = fopen(path, "wb");
	if (file == NULL || fwrite(text, 1, strlen(text), file) != strlen(text)) { CtrTest_Bail("cannot write language fixture"); }
	fclose(file);
}

static void RemoveFixture(const char *relative)
{
	char path[1024];
	int size = snprintf(path, sizeof(path), "%s/%s", s_base, relative);
	if (size >= 0 && (size_t)size < sizeof(path)) { SDL_RemovePath(path); }
}

int main(void)
{
	CtrTestScratch_Open(s_base, sizeof(s_base), "language-discovery", NULL);
	if (snprintf(s_assets, sizeof(s_assets), "%sassets", s_base) >= (int)sizeof(s_assets)) { CtrTest_Bail("fixture path too long"); }
	MakeDir("assets");
	MakeDir("assets/text");
	MakeDir("assets/text/pt");
	MakeDir("assets/text/aa");
	MakeDir("assets/text/bad");
	MakeDir("resources");
	MakeDir("resources/text");
	MakeDir("resources/text/pt");
	WriteFile("assets/text/pt/language.toml", "self_name = 'PORTUGUES'\n");
	WriteFile("assets/text/pt/retail.toml", "PAUSED = 'PAUSA PT'\n");
	WriteFile("assets/text/aa/language.toml", "self_name = 'AA'\n");
	WriteFile("assets/text/aa/expanded.toml", "MODE = 'AA MODE'\n");
	WriteFile("assets/text/bad/language.toml", "self_name = [broken\n");
	WriteFile("resources/text/pt/language.toml", "self_name = 'DISTRIBUIDO'\n");
	CTR_TEST_SECTION("Custom language discovery and persistence");
	NativeTextRuntime_DiscoverLanguages();
	int pt = NativeTextRuntime_LanguageSlot("pt");
	int aa = NativeTextRuntime_LanguageSlot("aa");
	CTR_TEST_CHECK(aa == NATIVE_TEXT_CUSTOM_FIRST && pt == aa + 1, "custom language IDs have deterministic order and deduplicate sources");
	CTR_TEST_CHECK(NativeTextRuntime_LanguageSlot("bad") < 0, "invalid metadata does not register a language");
	CTR_TEST_CHECK(strcmp(NativeTextRuntime_LanguageSelfName(pt), "PORTUGUES") == 0, "personal metadata overrides the shipped name");
	CTR_TEST_CHECK(NativeTextRuntime_HasText(pt) && NativeTextRuntime_HasText(aa), "retail-only and expanded-only catalogs are available");
	NativeTextRuntime_Reload(pt);
	CTR_TEST_CHECK(strcmp(NativeTextRuntime_Get(NATIVE_TEXT_SELECTED_OVERRIDE, LNG_PAUSED), "PAUSA PT") == 0,
	               "a discovered language loads its retail override");
	NativeTextRuntime_Reload(aa);
	CTR_TEST_CHECK(strcmp(NativeTextRuntime_Get(NATIVE_TEXT_SELECTED_OVERRIDE, LNG_EXPANDED_MODE), "AA MODE") == 0,
	               "a discovered language loads its Expanded override");
	g_config.textLanguage = pt;
	CTR_TEST_CHECK(!g_config.customCups, "custom cups are disabled by default");
	g_config.customCups = true;
	NativeConfig_Save();
	g_config.textLanguage = NATIVE_LANGUAGE_ENGLISH;
	g_config.customCups = false;
	NativeConfig_Load();
	CTR_TEST_CHECK(g_config.textLanguage == pt, "a custom language persists by directory ID");
	CTR_TEST_CHECK(g_config.customCups, "the custom cups option persists across reloads");
	WriteFile("config.ini", "[Language]\ntext = missing\n");
	NativeConfig_Load();
	CTR_TEST_CHECK(g_config.textLanguage == NATIVE_LANGUAGE_ENGLISH, "an unavailable custom language falls back to English");
	WriteFile("config.ini", "[Language]\ntext = 3missing\n");
	NativeConfig_Load();
	CTR_TEST_CHECK(g_config.textLanguage == NATIVE_LANGUAGE_ENGLISH, "a missing ID beginning with a digit does not select a different language");
	RemoveFixture("config.ini");
	RemoveFixture("assets/text/pt/language.toml");
	RemoveFixture("assets/text/pt/retail.toml");
	RemoveFixture("assets/text/aa/language.toml");
	RemoveFixture("assets/text/aa/expanded.toml");
	RemoveFixture("assets/text/bad/language.toml");
	RemoveFixture("resources/text/pt/language.toml");
	RemoveFixture("assets/text/pt");
	RemoveFixture("assets/text/aa");
	RemoveFixture("assets/text/bad");
	RemoveFixture("assets/text");
	RemoveFixture("assets");
	RemoveFixture("resources/text/pt");
	RemoveFixture("resources/text");
	RemoveFixture("resources");
	CtrTestScratch_Remove("language-discovery", NULL);
	return CtrTest_Summary();
}
