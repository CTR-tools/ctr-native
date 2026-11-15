#include <shared/ctr_test.h>
#include <platform/text/native_text_catalog.h>
#include <text/text_limits.h>
#include <limits.h>
#include <string.h>

// A zero or negative override must never silently disable a check.
#define RETAIL_TEXT_KEY(name, value)
#define RETAIL_TEXT_KEY_LIMIT(name, value, limit) _Static_assert((limit) > 0, #name " requires a positive length limit");
#include <text/retail_text_keys.h>
#undef RETAIL_TEXT_KEY_LIMIT
#undef RETAIL_TEXT_KEY
#define EXPANDED_TEXT_KEY(name)
#define EXPANDED_TEXT_KEY_LIMIT(name, limit) _Static_assert((limit) > 0, #name " requires a positive length limit");
#include <text/expanded_text_keys.h>
#undef EXPANDED_TEXT_KEY_LIMIT
#undef EXPANDED_TEXT_KEY
_Static_assert(TEXT_LIMIT_DEFAULT > 0, "the default length limit must be positive");

static const int s_limits[LNG_EXPANDED_STRING_END] = {
#define RETAIL_TEXT_KEY(name, value) [name] = TEXT_LIMIT_DEFAULT,
#define RETAIL_TEXT_KEY_LIMIT(name, value, limit) [name] = limit,
#include <text/retail_text_keys.h>
#undef RETAIL_TEXT_KEY_LIMIT
#undef RETAIL_TEXT_KEY
#define EXPANDED_TEXT_KEY(name) [name] = TEXT_LIMIT_DEFAULT,
#define EXPANDED_TEXT_KEY_LIMIT(name, limit) [name] = limit,
#include <text/expanded_text_keys.h>
#undef EXPANDED_TEXT_KEY_LIMIT
#undef EXPANDED_TEXT_KEY
};

static const struct
{
	const char *path;
	enum NativeTextCatalogKind kind;
} s_catalogs[] = {
#include <text_length_catalogs.h>
};

// TOML parsing has already validated UTF-8. Continuation bytes do not start a
// new code point; escapes have already been decoded by the parser.
static size_t TextLength_Count(const char *text)
{
	size_t count = 0;
	for (const unsigned char *p = (const unsigned char *)text; *p != '\0'; p++)
	{
		if ((*p & 0xc0) != 0x80)
		{
			count++;
		}
	}
	return count;
}

static int TextLength_Fits(const char *text, int limit)
{
	return limit > 0 && TextLength_Count(text) <= (size_t)limit;
}

static void TextLength_PrintValue(const char *text)
{
	fputs("        text: \"", stdout);
	for (const unsigned char *p = (const unsigned char *)text; *p != '\0'; p++)
	{
		if (*p == '\r') { fputs("\\r", stdout); }
		else if (*p == '\n') { fputs("\\n", stdout); }
		else if (*p == '\\') { fputs("\\\\", stdout); }
		else if (*p == '"') { fputs("\\\"", stdout); }
		else { fputc(*p, stdout); }
	}
	fputs("\"\n", stdout);
}

static void TextLength_CheckCatalog(const char *path, enum NativeTextCatalogKind kind)
{
	char message[1024];
	FILE *file = fopen(path, "rb");
	snprintf(message, sizeof(message), "%s: catalog can be opened", path);
	CTR_TEST_CHECK(file != NULL, message);
	if (file == NULL) { return; }
	if (fseek(file, 0, SEEK_END) != 0) { fclose(file); CtrTest_Bail("cannot seek catalog"); }
	long size = ftell(file);
	if (size < 0 || size > INT_MAX || fseek(file, 0, SEEK_SET) != 0) { fclose(file); CtrTest_Bail("invalid catalog size"); }
	char *text = malloc((size_t)size + 1);
	if (text == NULL) { fclose(file); CtrTest_Bail("cannot allocate catalog"); }
	size_t read = fread(text, 1, (size_t)size, file);
	fclose(file);
	if (read != (size_t)size) { free(text); CtrTest_Bail("cannot read catalog"); }
	text[size] = '\0';
	struct NativeTextCatalog catalog = {0};
	int invalid = 0;
	int parsed = NativeTextCatalog_Parse(&catalog, kind, text, (int)size, &invalid);
	free(text);
	snprintf(message, sizeof(message), "%s: catalog parses without invalid entries (%d)", path, invalid);
	CTR_TEST_CHECK(parsed && invalid == 0, message);
	if (!parsed) { return; }
	int first = kind == NATIVE_TEXT_CATALOG_RETAIL ? 0 : LNG_EXPANDED_STRING_BASE;
	int end = kind == NATIVE_TEXT_CATALOG_RETAIL ? LNG_RETAIL_STRING_COUNT : LNG_EXPANDED_STRING_END;
	for (int key = first; key < end; key++)
	{
		const char *value = NativeTextCatalog_Get(&catalog, key);
		if (value == NULL) { continue; }
		size_t length = TextLength_Count(value);
		snprintf(message, sizeof(message), "%s: %s has %zu characters (maximum %d)",
		         path, NativeTextCatalog_KeyName(key), length, s_limits[key]);
		int fits = TextLength_Fits(value, s_limits[key]);
		CTR_TEST_CHECK(fits, message);
		if (!fits) { TextLength_PrintValue(value); }
	}
	NativeTextCatalog_Clear(&catalog);
}

int main(void)
{
	CTR_TEST_SECTION("Text length rules");
	CTR_TEST_CHECK_EQ(TextLength_Count("A\xc3\x91\xee\x80\x81\r"), 4, "UTF-8 code points and line breaks are counted once");
	CTR_TEST_CHECK(TextLength_Fits("ABC", 3), "a value at its limit is accepted");
	CTR_TEST_CHECK(!TextLength_Fits("ABCD", 3), "a value above its limit is rejected");
	CTR_TEST_CHECK(!TextLength_Fits("ABCD", 0), "zero is not an unlimited value");
	CTR_TEST_CHECK_EQ(s_limits[LNG_PAUSED], TEXT_LIMIT_MENU_ENTRY, "retail category supplies its limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_EXPANDED_MODE], TEXT_LIMIT_MENU_ENTRY, "Expanded category supplies its limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_RESTART], 32, "a retail entry can override its limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_EXPANDED_CHAOS], 32, "an Expanded entry can override its limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_RESUME], 30, "unannotated retail entries default to 30 characters");
	CTR_TEST_CHECK_EQ(s_limits[LNG_EXPANDED_REGULAR], 30, "unannotated Expanded entries default to 30 characters");
	CTR_TEST_CHECK(sizeof(s_catalogs) / sizeof(s_catalogs[0]) > 0, "shipped catalogs are discovered");
	for (size_t i = 0; i < sizeof(s_catalogs) / sizeof(s_catalogs[0]); i++)
	{
		TextLength_CheckCatalog(s_catalogs[i].path, s_catalogs[i].kind);
	}
	return CtrTest_Summary();
}
