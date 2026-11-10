#include <string.h>

#include <tomlc17.h>
#include "shared/ctr_test.h"

static toml_result_t Parse(const char *text, size_t length)
{
	return toml_parse(text, (int)length);
}

int main(void)
{
	CTR_TEST_SECTION("Catalog strings and ownership");
	const char input[] = "[expanded]\nMODE = \"Modalit\\u00e0\"\n";
	toml_result_t result = Parse(input, sizeof(input) - 1);
	CTR_TEST_CHECK(result.ok, "a catalog with a Unicode escape parses");
	toml_datum_t value = toml_seek(result.toptab, "expanded.MODE");
	CTR_TEST_CHECK(value.type == TOML_STRING, "the catalog value is a string");
	char copy[32] = {0};
	if (value.type == TOML_STRING && value.u.str.len < (int)sizeof(copy))
	{
		memcpy(copy, value.u.str.ptr, (size_t)value.u.str.len);
	}
	toml_free(result);
	CTR_TEST_CHECK(strcmp(copy, "Modalit\xc3\xa0") == 0, "a copied UTF-8 string survives release of the parse tree");

	CTR_TEST_SECTION("Invalid input");
	const char duplicate[] = "MODE = \"a\"\nMODE = \"b\"\n";
	result = Parse(duplicate, sizeof(duplicate) - 1);
	CTR_TEST_CHECK(!result.ok && result.errmsg[0] != 0, "duplicate keys produce a diagnostic");
	toml_free(result);
	const char malformed[] = "MODE = \"unfinished\n";
	result = Parse(malformed, sizeof(malformed) - 1);
	CTR_TEST_CHECK(!result.ok && result.errmsg[0] != 0, "an unfinished string produces a diagnostic");
	toml_free(result);

	CTR_TEST_SECTION("UTF-8 validation and embedded NUL");
	toml_option_t options = toml_default_option();
	options.check_utf8 = true;
	toml_set_option(options);
	const char invalidUtf8[] = "MODE = \"\xc3\x28\"\n";
	result = Parse(invalidUtf8, sizeof(invalidUtf8) - 1);
	CTR_TEST_CHECK(!result.ok, "invalid UTF-8 is rejected when validation is enabled");
	toml_free(result);
	const char nul[] = "MODE = \"a\\u0000b\"\n";
	result = Parse(nul, sizeof(nul) - 1);
	CTR_TEST_CHECK(result.ok, "an escaped NUL is valid TOML");
	value = toml_get(result.toptab, "MODE");
	CTR_TEST_CHECK(value.type == TOML_STRING && value.u.str.len == 3 && value.u.str.ptr[1] == '\0',
	               "the string length exposes an embedded NUL to callers");
	toml_free(result);
	toml_set_option(toml_default_option());
	return CtrTest_Summary();
}
