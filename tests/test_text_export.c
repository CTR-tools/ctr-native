#include <shared/ctr_test.h>
#include <shared/ctr_test_scratch.h>
#include <platform/native_text_export.h>
#include <namespace_Lng.h>
#include <string.h>

static char s_exportDir[1024];
const char *NativeAssets_GetAssetDir(void) { return s_exportDir; }
const char *NativeTextRuntime_LanguageId(int slot) { return slot == 1 ? "en" : NULL; }

static const char *TestExport_String(int key)
{
	return key == LNG_PAUSED ? "A\"B\r@" : "OTHER";
}

int main(void)
{
	CTR_TEST_SECTION("Exported TOML template");
	FILE *file = tmpfile();
	CTR_TEST_CHECK(file != NULL, "a temporary export stream is available");
	if (file != NULL)
	{
		CTR_TEST_CHECK(NativeTextExport_Write(file, LNG_PAUSED, LNG_RESTART + 1, TestExport_String),
		               "active values can be written as a commented template");
		rewind(file);
		char line[256];
		CTR_TEST_CHECK(fgets(line, sizeof(line), file) != NULL && line[0] == '#', "instructions are commented");
		CTR_TEST_CHECK(fgets(line, sizeof(line), file) != NULL && line[0] == '#', "font escape instructions are commented");
		CTR_TEST_CHECK(fgets(line, sizeof(line), file) != NULL && strcmp(line, "# PAUSED = \"A\\\"B\\r@\"\n") == 0,
		               "the active value is escaped and commented");
		CTR_TEST_CHECK(fgets(line, sizeof(line), file) != NULL && strcmp(line, "# RESTART = \"OTHER\"\n") == 0,
		               "each requested key is exported");
		fclose(file);
	}
	CTR_TEST_SECTION("Export without overwriting");
	CtrTestScratch_Open(s_exportDir, sizeof(s_exportDir), "test-text-export", NULL);
	CTR_TEST_CHECK(NativeTextExport_Active(1, TestExport_String) == NATIVE_TEXT_EXPORT_OK,
	               "both files are created for the active language");
	char path[1200];
	snprintf(path, sizeof(path), "%s/text/en/retail.toml", s_exportDir);
	file = fopen(path, "rb");
	CTR_TEST_CHECK(file != NULL, "retail output exists");
	if (file != NULL) { fclose(file); }
	CTR_TEST_CHECK(NativeTextExport_Active(1, TestExport_String) == NATIVE_TEXT_EXPORT_EXISTS,
	               "existing files are reported instead of overwritten");
	file = fopen(path, "rb");
	if (file != NULL)
	{
		char line[256];
		CTR_TEST_CHECK(fgets(line, sizeof(line), file) != NULL && strncmp(line, "# Uncomment", 11) == 0,
		               "an existing retail file retains its contents");
		fclose(file);
	}
	SDL_RemovePath(path);
	snprintf(path, sizeof(path), "%s/text/en/expanded.toml", s_exportDir);
	SDL_RemovePath(path);
	snprintf(path, sizeof(path), "%s/text/en", s_exportDir);
	SDL_RemovePath(path);
	snprintf(path, sizeof(path), "%s/text", s_exportDir);
	SDL_RemovePath(path);
	CtrTestScratch_Remove("test-text-export", NULL);
	return CtrTest_Summary();
}
