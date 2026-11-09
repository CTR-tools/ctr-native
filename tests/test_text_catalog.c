#include <shared/ctr_test.h>
#include <platform/native_text_catalog.h>
#include <string.h>

int main(void)
{
	struct NativeTextCatalog english = {0};
	struct NativeTextCatalog selected = {0};
	int invalid = 0;
	const char base[] = "MODE = 'Mode'\nCHAOS = 'Chaos'\n";
	const char translation[] = "MODE = 'Modalit\xc3\xa0'\nUNKNOWN = 'ignored'\nCHAOS = 42\n";
	CTR_TEST_SECTION("Catalogs and individual entries");
	CTR_TEST_CHECK_EQ(LNG_EXPANDED_MODE, LNG_RETAIL_STRING_COUNT, "the first Expanded ID follows retail");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_KeyName(LNG_EXPANDED_MODE), "MODE") == 0, "the first ID maps to its catalog key");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_KeyName(LNG_EXPANDED_CHAOS_RULES), "CHAOS_RULES") == 0, "the middle ID maps to its catalog key");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_KeyName(LNG_EXPANDED_ULTRA_HARD), "ULTRA_HARD") == 0, "the last ID maps to its catalog key");
	CTR_TEST_CHECK(NativeTextCatalog_Parse(&english, base, sizeof(base) - 1, NULL), "English catalog parses");
	CTR_TEST_CHECK(NativeTextCatalog_Parse(&selected, translation, sizeof(translation) - 1, &invalid), "partial translation parses");
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
	CTR_TEST_CHECK(!NativeTextCatalog_Parse(&selected, malformed, sizeof(malformed) - 1, NULL), "malformed TOML is rejected");
	const char badUtf8[] = "MODE = '\xc3\x28'\n";
	CTR_TEST_CHECK(!NativeTextCatalog_Parse(&selected, badUtf8, sizeof(badUtf8) - 1, NULL), "invalid UTF-8 is rejected");
	CTR_TEST_CHECK(strcmp(NativeTextCatalog_Get(&selected, LNG_EXPANDED_MODE), "Modalit\xc3\xa0") == 0,
	               "failed replacement leaves the previous catalog intact");
	const char nul[] = "MODE = \"a\\u0000b\"\n";
	CTR_TEST_CHECK(NativeTextCatalog_Parse(&selected, nul, sizeof(nul) - 1, &invalid), "catalog with invalid entry is accepted partially");
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
	NativeTextCatalog_Clear(&selected);
	NativeTextCatalog_Clear(&english);
	return CtrTest_Summary();
}
