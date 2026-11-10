#include <shared/ctr_test.h>
#include <platform/native_text_catalog.h>
#include <text_format.h>
#include <string.h>

int main(void)
{
	struct NativeTextCatalog english = {0};
	struct NativeTextCatalog selected = {0};
	int invalid = 0;
	const char base[] = "MODE = 'Mode'\nCHAOS = 'Chaos'\n";
	const char translation[] = "MODE = 'Modalit\xc3\xa0'\nUNKNOWN = 'ignored'\nCHAOS = 42\n";
	CTR_TEST_SECTION("Catalogs and individual entries");
#define EXPANDED_TEXT_KEY(name) CTR_TEST_CHECK(strcmp(NativeTextCatalog_KeyName(name), #name + sizeof("LNG_EXPANDED_") - 1) == 0, "Expanded ID maps to its shared key");
#include <expanded_text_keys.h>
#undef EXPANDED_TEXT_KEY
	CTR_TEST_CHECK_EQ(LNG_EXPANDED_MODE, LNG_RETAIL_STRING_COUNT, "the first Expanded ID follows retail");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_KeyName(LNG_EXPANDED_MODE), "MODE") == 0, "the first ID maps to its catalog key");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_KeyName(LNG_EXPANDED_CHAOS_RULES), "CHAOS_RULES") == 0, "the middle ID maps to its catalog key");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_KeyName(LNG_EXPANDED_ULTRA_HARD), "ULTRA_HARD") == 0, "difficulty maps to its catalog key");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_KeyName(LNG_EXPANDED_PRESS_START_TO_SKIP), "PRESS_START_TO_SKIP") == 0, "the skip prompt maps to its catalog key");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_KeyName(LNG_EXPANDED_NO_PREVIEW), "NO_PREVIEW") == 0, "the preview message maps to its catalog key");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_KeyName(LNG_EXPANDED_CHAOS_EFFECT_KING_CRIMSON), "CHAOS_EFFECT_KING_CRIMSON") == 0, "the last Chaos effect maps to its catalog key");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_KeyName(LNG_EXPANDED_OFF), "OFF") == 0, "the shared off ID maps to its catalog key");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_KeyName(LNG_EXPANDED_ON), "ON") == 0, "the shared on ID maps to its catalog key");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_KeyName(LNG_EXPANDED_SELECT_LANGUAGE), "SELECT_LANGUAGE") == 0, "the language picker maps to its catalog key");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_KeyName(LNG_EXPANDED_CONTROLS_PLAYER_TITLE), "CONTROLS_PLAYER_TITLE") == 0, "the last ID maps to its catalog key");
	CTR_TEST_CHECK(NativeTextCatalog_Parse(&english, NATIVE_TEXT_CATALOG_EXPANDED, base, sizeof(base) - 1, NULL), "English catalog parses");
	CTR_TEST_CHECK(NativeTextCatalog_Parse(&selected, NATIVE_TEXT_CATALOG_EXPANDED, translation, sizeof(translation) - 1, &invalid), "partial translation parses");
	CTR_TEST_CHECK_EQ(invalid, 2, "unknown keys and non-string values are reported");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_Get(&selected, LNG_EXPANDED_MODE), "Modalit\xc3\xa0") == 0,
	               "translated values remain UTF-8 until conversion");
	CTR_TEST_CHECK(NativeTextCatalog_Get(&selected, LNG_EXPANDED_CHAOS) == NULL &&
	               strcmp(NativeTextCatalog_Get(&english, LNG_EXPANDED_CHAOS), "Chaos") == 0,
	               "an invalid translation leaves the English entry available to the resolver");
	CTR_TEST_CHECK(NativeTextCatalog_Get(&selected, LNG_EXPANDED_REGULAR) == NULL, "a missing key is distinguishable");
	CTR_TEST_CHECK(NativeTextCatalog_Get(&selected, -1) == NULL, "invalid identifiers are rejected");

	CTR_TEST_SECTION("Invalid replacement preserves active catalog");
	const char malformed[] = "MODE = \"bad\n";
	CTR_TEST_CHECK(!NativeTextCatalog_Parse(&selected, NATIVE_TEXT_CATALOG_EXPANDED, malformed, sizeof(malformed) - 1, NULL), "malformed TOML is rejected");
	const char badUtf8[] = "MODE = '\xc3\x28'\n";
	CTR_TEST_CHECK(!NativeTextCatalog_Parse(&selected, NATIVE_TEXT_CATALOG_EXPANDED, badUtf8, sizeof(badUtf8) - 1, NULL), "invalid UTF-8 is rejected");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_Get(&selected, LNG_EXPANDED_MODE), "Modalit\xc3\xa0") == 0,
	               "failed replacement leaves the previous catalog intact");
	const char nul[] = "MODE = \"a\\u0000b\"\n";
	CTR_TEST_CHECK(NativeTextCatalog_Parse(&selected, NATIVE_TEXT_CATALOG_EXPANDED, nul, sizeof(nul) - 1, &invalid), "catalog with invalid entry is accepted partially");
	CTR_TEST_CHECK_EQ(invalid, 1, "embedded NUL is rejected as an individual entry");
	CTR_TEST_CHECK(NativeTextCatalog_Get(&selected, LNG_EXPANDED_MODE) == NULL &&
	               strcmp(NativeTextCatalog_Get(&english, LNG_EXPANDED_MODE), "Mode") == 0,
	               "rejected entry leaves the English entry available to the resolver");
	CTR_TEST_SECTION("Font encoding");
	char encoded[32];
	CTR_TEST_CHECK(NativeTextCatalog_Encode("ESPA\xc3\x91OL", encoded, sizeof(encoded)) && strcmp(encoded, "ESPA\003NOL") == 0,
	               "Spanish tilde uses the verified retail byte sequence");
	CTR_TEST_CHECK(NativeTextCatalog_Encode("\xc2\xa1SI!", encoded, sizeof(encoded)) && strcmp(encoded, "#SI!") == 0,
	               "inverted exclamation uses the retail font byte");
	CTR_TEST_CHECK(!NativeTextCatalog_Encode("Modalit\xc3\xa0", encoded, sizeof(encoded)), "unverified accented characters are rejected");
	CTR_TEST_CHECK(!NativeTextCatalog_Encode("AB", encoded, 2), "output requires room for the terminator");
	char decoded[32];
	CTR_TEST_CHECK(NativeTextCatalog_Decode("ESPA\003NOL\r@", decoded, sizeof(decoded)) &&
	               strcmp(decoded, "ESPA\xc3\x91OL\r@") == 0 &&
	               NativeTextCatalog_Encode(decoded, encoded, sizeof(encoded)) && strcmp(encoded, "ESPA\003NOL\r@") == 0,
	               "verified font bytes round-trip through UTF-8 without changing control glyphs");
	CTR_TEST_CHECK(NativeTextCatalog_Decode("\001A", decoded, sizeof(decoded)) &&
	               NativeTextCatalog_Encode(decoded, encoded, sizeof(encoded)) && strcmp(encoded, "\001A") == 0,
	               "unverified font bytes survive a UTF-8 round trip");
	CTR_TEST_CHECK(!NativeTextCatalog_Decode("AB", decoded, 2), "decoded output requires room for its terminator");
	FILE *templateFile = tmpfile();
	CTR_TEST_CHECK(templateFile != NULL, "a temporary stream is available for the template test");
	if (templateFile != NULL)
	{
		CTR_TEST_CHECK(NativeTextCatalog_WriteTemplateEntry(templateFile, LNG_PAUSED, "A\"B\\C\r@"),
		               "a template entry is written");
		rewind(templateFile);
		char templateLine[128];
		CTR_TEST_CHECK(fgets(templateLine, sizeof(templateLine), templateFile) != NULL &&
		               strcmp(templateLine, "# PAUSED = \"A\\\"B\\\\C\\r@\"\n") == 0,
		               "TOML template entries are commented and escaped");
		fseek(templateFile, 0, SEEK_END);
		CTR_TEST_CHECK(NativeTextCatalog_WriteTemplateEntry(templateFile, LNG_PAUSED, "\001A"),
		               "unknown font bytes are preserved in an export entry");
		rewind(templateFile);
		fgets(templateLine, sizeof(templateLine), templateFile);
		CTR_TEST_CHECK(fgets(templateLine, sizeof(templateLine), templateFile) != NULL &&
		               strcmp(templateLine, "# PAUSED = \"\\uE001A\"\n") == 0,
		               "unknown font bytes use visible TOML escapes");
		struct NativeTextCatalog roundTrip = {0};
		CTR_TEST_CHECK(NativeTextCatalog_Parse(&roundTrip, NATIVE_TEXT_CATALOG_RETAIL, templateLine + 2,
		                                      (int)strlen(templateLine + 2), NULL) &&
		               NativeTextCatalog_Encode(NativeTextCatalog_Get(&roundTrip, LNG_PAUSED), encoded, sizeof(encoded)) &&
		               strcmp(encoded, "\001A") == 0,
		               "an uncommented export restores the original font byte");
		NativeTextCatalog_Clear(&roundTrip);
		fclose(templateFile);
	}
	CTR_TEST_SECTION("Named placeholders");
	const char templates[] = "PATCH_IMPORTED_SKIPPED = '{skipped_music} / {tracks}'\n"
	                         "TRACKS_REFRESH_SUMMARY = '{tracks} {unknown}'\n"
	                         "CONTROLS_PLAYER_TITLE = 'PLAYER {player'\n";
	CTR_TEST_CHECK(NativeTextCatalog_Parse(&selected, NATIVE_TEXT_CATALOG_EXPANDED, templates, sizeof(templates) - 1, &invalid), "templates parse independently");
	CTR_TEST_CHECK_EQ(invalid, 2, "unknown and malformed placeholders are rejected");
	CTR_TEST_CHECK(NativeTextCatalog_Get(&selected, LNG_EXPANDED_TRACKS_REFRESH_SUMMARY) == NULL &&
	               NativeTextCatalog_Get(&selected, LNG_EXPANDED_CONTROLS_PLAYER_TITLE) == NULL,
	               "invalid templates remain absent for per-key fallback");
	const struct TextFormatValue values[] = {{"tracks", "3"}, {"skipped_music", "2"}};
	char formatted[32];
	CTR_TEST_CHECK(TextFormat_Apply(formatted, sizeof(formatted), NativeTextCatalog_Get(&selected, LNG_EXPANDED_PATCH_IMPORTED_SKIPPED), values, 2) &&
	               strcmp(formatted, "2 / 3") == 0, "placeholders can be reordered");
	CTR_TEST_CHECK(!TextFormat_Apply(formatted, 3, "{tracks} {skipped_music}", values, 2) && formatted[0] == '\0',
	               "insufficient output space cannot produce partial text");
	CTR_TEST_CHECK(!TextFormat_Apply(formatted, sizeof(formatted), "{unknown}", values, 2), "unknown substitutions are rejected");
	CTR_TEST_SECTION("Retail catalogs");
	struct NativeTextCatalog retail = {0};
	const char retailText[] = "PAUSED = 'PAUSA'\nTURBOS = 'TURBO'\nUNKNOWN = 'invalid'\nRESTART = 4\n";
	CTR_TEST_CHECK(NativeTextCatalog_Parse(&retail, NATIVE_TEXT_CATALOG_RETAIL, retailText, sizeof(retailText) - 1, &invalid), "retail catalog parses");
	CTR_TEST_CHECK_EQ(invalid, 2, "invalid retail entries are counted");
	for (int i = 0; i < LNG_RETAIL_STRING_COUNT; i++)
	{
		CTR_TEST_CHECK(NativeTextCatalog_KeyName(i)[0] != '\0', "each retail index has a named key");
	}
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_KeyName(LNG_PAUSED), "PAUSED") == 0 &&
	               strcmp(NativeTextCatalog_KeyName(LNG_TURBOS), "TURBOS") == 0,
	               "retail key names come from the language enum");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_Get(&retail, 0), "PAUSA") == 0 &&
	               strcmp(NativeTextCatalog_Get(&retail, LNG_RETAIL_STRING_COUNT - 1), "TURBO") == 0,
	               "first and last retail keys resolve");
	CTR_TEST_CHECK(!NativeTextCatalog_Parse(&retail, NATIVE_TEXT_CATALOG_RETAIL, malformed, sizeof(malformed) - 1, NULL) &&
	               strcmp(NativeTextCatalog_Get(&retail, 0), "PAUSA") == 0,
	               "malformed replacement preserves the catalog");
	const char overlapping[] = "MODE = 'MODALITA'\n";
	CTR_TEST_CHECK(NativeTextCatalog_Parse(&retail, NATIVE_TEXT_CATALOG_EXPANDED, overlapping, sizeof(overlapping) - 1, NULL) &&
	               NativeTextCatalog_Parse(&retail, NATIVE_TEXT_CATALOG_RETAIL, overlapping, sizeof(overlapping) - 1, NULL) &&
	               strcmp(NativeTextCatalog_Get(&retail, LNG_EXPANDED_MODE), "MODALITA") == 0 &&
	               strcmp(NativeTextCatalog_Get(&retail, LNG_MODE), "MODALITA") == 0 &&
	               NativeTextCatalog_Get(&retail, LNG_PAUSED) == NULL,
	               "the selected range resolves overlapping names and preserves the other range");
	NativeTextCatalog_Clear(&retail);
	NativeTextCatalog_Clear(&selected);
	NativeTextCatalog_Clear(&english);
	return CtrTest_Summary();
}
