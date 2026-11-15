#include <shared/ctr_test.h>
#include <platform/text/native_text_catalog.h>
#include <platform/config/native_config.h>
#include <text/text_limits.h>
#include <limits.h>
#include <string.h>

// The configuration table is linked without invoking its file I/O.
const char *NativeAssets_GetBaseDir(void) { return ""; }
int NativeTextRuntime_LanguageSlot(const char *id) { (void)id; return -1; }
const char *NativeTextRuntime_LanguageId(int slot) { (void)slot; return NULL; }

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

#define TEXT_TEST_DEFAULT_ENTRY(name) [name] = TEXT_LIMIT_DEFAULT,
#define TEXT_TEST_LIMIT_ENTRY(name, limit) [name] = limit,

static const int s_limits[LNG_EXPANDED_STRING_END] = {
#define RETAIL_TEXT_KEY(name, value) TEXT_TEST_DEFAULT_ENTRY(name)
#define RETAIL_TEXT_KEY_LIMIT(name, value, limit) TEXT_TEST_LIMIT_ENTRY(name, limit)
#include <text/retail_text_keys.h>
#undef RETAIL_TEXT_KEY_LIMIT
#undef RETAIL_TEXT_KEY
#define EXPANDED_TEXT_KEY(name) TEXT_TEST_DEFAULT_ENTRY(name)
#define EXPANDED_TEXT_KEY_LIMIT(name, limit) TEXT_TEST_LIMIT_ENTRY(name, limit)
#include <text/expanded_text_keys.h>
#undef EXPANDED_TEXT_KEY_LIMIT
#undef EXPANDED_TEXT_KEY
};

enum TextLengthFixtureKey
{
	TEXT_FIXTURE_DEFAULT,
	TEXT_FIXTURE_OVERRIDE,
	TEXT_FIXTURE_COUNT,
};

static const int s_fixtureLimits[TEXT_FIXTURE_COUNT] = {
	TEXT_TEST_DEFAULT_ENTRY(TEXT_FIXTURE_DEFAULT)
	TEXT_TEST_LIMIT_ENTRY(TEXT_FIXTURE_OVERRIDE, 32)
};
#undef TEXT_TEST_DEFAULT_ENTRY
#undef TEXT_TEST_LIMIT_ENTRY

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

static int TextLength_PairFits(const char *name, const char *value)
{
	return TextLength_Count(name) + TextLength_Count(value) <= TEXT_LIMIT_FULL_SCREEN_MENU_ROW;
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

static void TextLength_CheckPair(const char *path, const char *nameKey, const char *name,
                                 const char *valueKey, const char *value)
{
	char message[1024];
	if (name == NULL || value == NULL) { return; }
	size_t total = TextLength_Count(name) + TextLength_Count(value);
	snprintf(message, sizeof(message), "%s: %s + %s has %zu characters (maximum %d)",
	         path, nameKey, valueKey, total, TEXT_LIMIT_FULL_SCREEN_MENU_ROW);
	int fits = TextLength_PairFits(name, value);
	CTR_TEST_CHECK(fits, message);
	if (!fits)
	{
		TextLength_PrintValue(name);
		TextLength_PrintValue(value);
	}
}

static void TextLength_CheckConfigPairs(const char *path, const struct NativeTextCatalog *catalog)
{
	static const int boolValues[] = {LNG_EXPANDED_OFF, LNG_EXPANDED_ON};
	static const int actionValues[] = {LNG_EXPANDED_EXPORT_DONE, LNG_EXPANDED_EXPORT_EXISTS, LNG_EXPANDED_EXPORT_FAILED};
	for (int i = 0; i < g_numConfigEntries; i++)
	{
		const ConfigEntry *entry = &g_configEntries[i];
		const char *name = NativeTextCatalog_Get(catalog, entry->lngKey);
		if (name == NULL) { continue; }
		const char *nameKey = NativeTextCatalog_KeyName(entry->lngKey);
		if (entry->type == CFG_BOOL || (entry->type == CFG_ACTION && strcmp(entry->key, "export_active_language") == 0))
		{
			const int *values = entry->type == CFG_BOOL ? boolValues : actionValues;
			int count = entry->type == CFG_BOOL ? 2 : 3;
			for (int j = 0; j < count; j++)
			{
				TextLength_CheckPair(path, nameKey, name, NativeTextCatalog_KeyName(values[j]), NativeTextCatalog_Get(catalog, values[j]));
			}
		}
		else if (entry->type == CFG_ENUM && entry->valuePtr != &g_config.textLanguage && entry->valuePtr != &g_config.voiceLanguage)
		{
			for (int j = 0; j < entry->numEnumValues; j++)
			{
				const ConfigEnumValue *choice = &entry->enumValues[j];
				const char *value = choice->lngKey >= LNG_EXPANDED_STRING_BASE ? NativeTextCatalog_Get(catalog, choice->lngKey) : choice->name;
				const char *key = choice->lngKey >= LNG_EXPANDED_STRING_BASE ? NativeTextCatalog_KeyName(choice->lngKey) : choice->name;
				TextLength_CheckPair(path, nameKey, name, key, value);
			}
		}
		else if (entry->type == CFG_INT)
		{
			char value[32];
			snprintf(value, sizeof(value), "%d%%", entry->min);
			TextLength_CheckPair(path, nameKey, name, "minimum", value);
			snprintf(value, sizeof(value), "%d%%", entry->max);
			TextLength_CheckPair(path, nameKey, name, "maximum", value);
		}
	}
}

static void TextLength_CheckMenuPairs(const char *path, const struct NativeTextCatalog *catalog)
{
	static const struct { int name, value; } pairs[] = {
		{LNG_EXPANDED_TRIGGER, LNG_EXPANDED_RANDOM}, {LNG_EXPANDED_TRIGGER, LNG_EXPANDED_PICKUP},
		{LNG_EXPANDED_TRIGGER, LNG_EXPANDED_INTERVAL}, {LNG_EXPANDED_TRIGGER, LNG_EXPANDED_EXTERNAL},
		{LNG_EXPANDED_VIBRATION, LNG_EXPANDED_ON}, {LNG_EXPANDED_VIBRATION, LNG_EXPANDED_OFF},
		{LNG_EXPANDED_DEVICE, LNG_EXPANDED_AUTOMATIC}, {LNG_EXPANDED_DEVICE, LNG_EXPANDED_DISABLED},
		{LNG_EXPANDED_DEVICE, LNG_EXPANDED_CONTROLS_KEYBOARD},
	};
	for (size_t i = 0; i < sizeof(pairs) / sizeof(pairs[0]); i++)
	{
		TextLength_CheckPair(path, NativeTextCatalog_KeyName(pairs[i].name), NativeTextCatalog_Get(catalog, pairs[i].name),
		                     NativeTextCatalog_KeyName(pairs[i].value), NativeTextCatalog_Get(catalog, pairs[i].value));
	}
	for (int key = LNG_EXPANDED_CONTROLS_L1; key <= LNG_EXPANDED_CONTROLS_RIGHT_STICK_Y; key++)
	{
		TextLength_CheckPair(path, NativeTextCatalog_KeyName(key), NativeTextCatalog_Get(catalog, key),
		                     NativeTextCatalog_KeyName(LNG_EXPANDED_CONTROLS_CONFIRM_BINDING),
		                     NativeTextCatalog_Get(catalog, LNG_EXPANDED_CONTROLS_CONFIRM_BINDING));
		TextLength_CheckPair(path, NativeTextCatalog_KeyName(key), NativeTextCatalog_Get(catalog, key),
		                     NativeTextCatalog_KeyName(LNG_EXPANDED_NONE), NativeTextCatalog_Get(catalog, LNG_EXPANDED_NONE));
	}
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
	if (kind == NATIVE_TEXT_CATALOG_EXPANDED)
	{
		TextLength_CheckConfigPairs(path, &catalog);
		TextLength_CheckMenuPairs(path, &catalog);
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
	char rowBoundary[TEXT_LIMIT_FULL_SCREEN_MENU_ROW + 2];
	memset(rowBoundary, 'A', sizeof(rowBoundary) - 1);
	rowBoundary[TEXT_LIMIT_FULL_SCREEN_MENU_ROW] = '\0';
	CTR_TEST_CHECK(TextLength_PairFits(rowBoundary, ""), "a combined row at the limit fits");
	CTR_TEST_CHECK(!TextLength_PairFits(rowBoundary, "A"), "a combined row above the limit fails");
	CTR_TEST_CHECK_EQ(s_limits[LNG_PAUSED], TEXT_LIMIT_MENU_ENTRY, "retail category supplies its limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_YOU_CRAZY_ENOUGH_TO_RACE_ME], TEXT_LIMIT_BOSS_DIALOG, "boss dialogue uses its category limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_GOOD_LUCK_YOULL_NEED_IT], TEXT_LIMIT_BOSS_DIALOG, "the last boss dialogue uses its category limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_EXPANDED_MODE], TEXT_LIMIT_MENU_ENTRY, "Expanded category supplies its limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_EXPANDED_CUPS_ADD_TRACK], TEXT_LIMIT_FULL_SCREEN_MENU_ROW, "full-screen action rows use the row limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_EXPANDED_CUPS_CREATE_NEW], TEXT_LIMIT_TRACK_NAME_LENGTH, "the create-cup entry uses the track name limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_STANDARD], TEXT_LIMIT_FULL_SCREEN_MENU_ROW, "retail full-screen rows use the row limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_EXPANDED_CONFIG_SKIP_INTROS], TEXT_LIMIT_CONFIGURATION_OPTION_NAME, "configuration option names use the name limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_MUSIC], TEXT_LIMIT_CONFIGURATION_OPTION_NAME, "retail audio option names use the name limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_EXPANDED_CONFIG_NORMAL], TEXT_LIMIT_CONFIGURATION_OPTION_VALUE, "configuration values use the value limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_STEREO], TEXT_LIMIT_CONFIGURATION_OPTION_VALUE, "retail audio values use the value limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_SETUP_BATTLE], TEXT_LIMIT_FULL_SCREEN_MENU_TITLE, "battle setup title uses the full-screen title limit");
	const int battleLabels[] = {LNG_TYPE, LNG_LENGTH, LNG_TEAMS, LNG_WEAPONS};
	for (size_t i = 0; i < sizeof(battleLabels) / sizeof(battleLabels[0]); i++)
	{
		CTR_TEST_CHECK_EQ(s_limits[battleLabels[i]], TEXT_LIMIT_BATTLE_SETUP_LABEL, "battle setup side labels use their limit");
	}
	for (int key = LNG_3_LIVES; key <= LNG_FOREVER; key++)
	{
		int limit = key >= LNG_5_POINTS && key <= LNG_15_POINTS ? TEXT_LIMIT_BATTLE_SETUP_WIDE : TEXT_LIMIT_BATTLE_SETUP_NARROW;
		CTR_TEST_CHECK_EQ(s_limits[key], limit, "battle setup lengths use the appropriate rectangle limit");
	}
	for (int key = LNG_TIME_LIMIT_3_MINUTES; key <= LNG_9_MINUTES; key++)
	{
		CTR_TEST_CHECK_EQ(s_limits[key], TEXT_LIMIT_BATTLE_SETUP_WIDE, "time-limit values use the wide rectangle limit");
	}
	for (int key = LNG_POINT_LIMIT_MODE; key <= LNG_LIFE_LIMIT_MODE; key++)
	{
		CTR_TEST_CHECK_EQ(s_limits[key], TEXT_LIMIT_BATTLE_SETUP_WIDE, "battle types use the wide rectangle limit");
	}
	CTR_TEST_CHECK_EQ(s_limits[LNG_START_BATTLE], TEXT_LIMIT_BATTLE_SETUP_WIDE, "the start action uses the wide rectangle limit");
	for (int key = LNG_CRASH_BANDICOOT; key <= LNG_FAKE_CRASH; key++)
	{
		CTR_TEST_CHECK_EQ(s_limits[key], TEXT_LIMIT_CHARACTER_NAME, "each full character name uses the character name limit");
	}
	for (int key = LNG_CRASH; key <= LNG_F_CRASH; key++)
	{
		CTR_TEST_CHECK_EQ(s_limits[key], TEXT_LIMIT_CHARACTER_NAME_SHORT, "each short character name uses the short name limit");
	}
	for (int key = LNG_GEM_STONE_VALLEY; key <= LNG_CITADEL_CITY; key++)
	{
		CTR_TEST_CHECK_EQ(s_limits[key], TEXT_LIMIT_HUB_NAME, "each hub name leaves room for navigation arrows");
	}
	for (int key = LNG_RED_GEM_CUP; key <= LNG_PURPLE_GEM_CUP; key++)
	{
		CTR_TEST_CHECK_EQ(s_limits[key], TEXT_LIMIT_FULL_SCREEN_MENU_TITLE, "each gem cup uses the full-screen title limit");
	}
	for (int key = LNG_WUMPA_CUP; key <= LNG_CRASH_CUP; key++)
	{
		CTR_TEST_CHECK_EQ(s_limits[key], TEXT_LIMIT_CUP_NAME, "each arcade cup uses the cup name limit");
	}
	const int fullScreenTitles[] = {
		LNG_EXPANDED_CHAOS_SETUP_TITLE, LNG_EXPANDED_CHAOS_EFFECTS_TITLE, LNG_EXPANDED_ACTIVE_EFFECTS,
		LNG_EXPANDED_CONFIG_AUDIO, LNG_EXPANDED_CONTROLS_PLAYER_TITLE, LNG_EXPANDED_SELECT_LANGUAGE,
		LNG_EXPANDED_TRACKS_AVAILABLE, LNG_EXPANDED_TRACKS_RELIC_TIMES, LNG_EXPANDED_TRACKS_IMPORT,
		LNG_EXPANDED_TRACKS_REFRESH, LNG_EXPANDED_TRACKS_DELETE, LNG_EXPANDED_TRACKS_DETAILS,
		LNG_EXPANDED_TRACKS_REVIEW_IMPORT, LNG_EXPANDED_TRACKS_IMPORT_TITLE,
		LNG_EXPANDED_CUPS_SELECT_TYPE, LNG_EXPANDED_CUPS_DISCARD,
	};
	for (size_t i = 0; i < sizeof(fullScreenTitles) / sizeof(fullScreenTitles[0]); i++)
	{
		CTR_TEST_CHECK_EQ(s_limits[fullScreenTitles[i]], TEXT_LIMIT_FULL_SCREEN_MENU_TITLE, "full-screen menu titles use the title limit");
	}
	const int raceMessages[] = { LNG_WRONG_WAY, LNG_FINISHED, LNG_FINAL_LAP };
	for (size_t i = 0; i < sizeof(raceMessages) / sizeof(raceMessages[0]); i++)
	{
		CTR_TEST_CHECK_EQ(s_limits[raceMessages[i]], TEXT_LIMIT_RACE_MESSAGE, "race messages use the shared race message limit");
	}
	CTR_TEST_CHECK_EQ(s_limits[LNG_BEST_LAP], TEXT_LIMIT_END_RACE_RECORD_LABEL, "the best lap label uses the end-race record limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_BEST_TIMES], TEXT_LIMIT_END_RACE_RECORD_LABEL, "the best times label uses the end-race record limit");
	CTR_TEST_CHECK_EQ(s_limits[LNG_YOUR_TIME], TEXT_LIMIT_END_RACE_RECORD_LABEL, "the your time label uses the end-race record limit");
	for (int key = LNG_EXPANDED_CHAOS_EFFECT_MIRROR_MIRROR; key <= LNG_EXPANDED_CHAOS_EFFECT_KING_CRIMSON; key++)
	{
		CTR_TEST_CHECK_EQ(s_limits[key], TEXT_LIMIT_RACE_MESSAGE, "each Chaos effect name uses the race message limit");
	}
	for (int key = LNG_EXPANDED_ENGINE_SPEED_SHORT; key <= LNG_EXPANDED_ENGINE_MAXED_OUT_SHORT; key++)
	{
		CTR_TEST_CHECK_EQ(s_limits[key], TEXT_LIMIT_THREE, "each engine class abbreviation is limited to three characters");
	}
	for (int key = LNG_EXPANDED_ENGINE_SPEED; key <= LNG_EXPANDED_ENGINE_MAXED_OUT; key++)
	{
		CTR_TEST_CHECK_EQ(s_limits[key], TEXT_LIMIT_ENGINE_CLASS, "each Expanded engine class uses the class limit");
	}
	for (int key = LNG_BEGINNER; key <= LNG_ADVANCED; key++)
	{
		CTR_TEST_CHECK_EQ(s_limits[key], TEXT_LIMIT_ENGINE_CLASS, "each retail engine class uses the class limit");
	}
	for (int key = LNG_SPEED; key <= LNG_TURN; key++)
	{
		CTR_TEST_CHECK_EQ(s_limits[key], TEXT_LIMIT_ENGINE_STAT, "each engine stat uses the stat limit");
	}
	for (int key = LNG_NITRO_COURT; key <= LNG_LAB_BASEMENT; key++)
	{
		CTR_TEST_CHECK_EQ(s_limits[key], TEXT_LIMIT_TRACK_NAME_LENGTH, "each battle arena uses the track name limit");
	}
	for (int key = LNG_DINGO_CANYON; key <= LNG_TURBO_TRACK; key++)
	{
		CTR_TEST_CHECK_EQ(s_limits[key], TEXT_LIMIT_TRACK_NAME_LENGTH, "each race track uses the track name limit");
	}
	CTR_TEST_CHECK_EQ(s_limits[LNG_TIMING_OUT_EXITING_DEMO], 40, "a retail entry can override its limit");
	CTR_TEST_CHECK_EQ(s_fixtureLimits[TEXT_FIXTURE_DEFAULT], 120, "an unannotated entry defaults to 120 characters");
	CTR_TEST_CHECK_EQ(s_fixtureLimits[TEXT_FIXTURE_OVERRIDE], 32, "an explicit limit overrides the default");
	CTR_TEST_CHECK_EQ(s_limits[LNG_EXPANDED_REGULAR], TEXT_LIMIT_MENU_ENTRY, "the regular mode uses the menu entry limit");
	CTR_TEST_CHECK(sizeof(s_catalogs) / sizeof(s_catalogs[0]) > 0, "shipped catalogs are discovered");
	for (size_t i = 0; i < sizeof(s_catalogs) / sizeof(s_catalogs[0]); i++)
	{
		TextLength_CheckCatalog(s_catalogs[i].path, s_catalogs[i].kind);
	}
	return CtrTest_Summary();
}
