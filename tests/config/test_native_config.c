// Configuration and profile tests use their own scratch directory.
#include <SDL3/SDL.h>
#include <macros.h>
#include <shared/ctr_test.h>
#include <shared/ctr_test_scratch.h>
#include <platform/native_path.h>

static char s_testBaseDir[512];

const char *NativeAssets_GetBaseDir(void) { return s_testBaseDir; }
int NativeTextRuntime_LanguageSlot(const char *id) { (void)id; return -1; }
const char *NativeTextRuntime_LanguageId(int slot) { (void)slot; return NULL; }

#include <platform/config/native_config_ini.c>
#include <platform/config/native_config.c>
#include <platform/config/native_profile.c>

static void TestProfileStartup(void)
{
	CTR_TEST_SECTION("startup loads the selected profile before config overrides");
	char directory[512], profilePath[512], configPath[512], vanillaPath[512], expandedPath[512];
	NativePath_Join(directory, sizeof(directory), NativeStr8_FromCString(s_testBaseDir), NATIVE_STR8_LIT("presets"));
	NativePath_Join(profilePath, sizeof(profilePath), NativeStr8_FromCString(s_testBaseDir), NATIVE_STR8_LIT("profile.ini"));
	NativePath_Join(configPath, sizeof(configPath), NativeStr8_FromCString(s_testBaseDir), NATIVE_STR8_LIT("config.ini"));
	NativePath_Join(vanillaPath, sizeof(vanillaPath), NativeStr8_FromCString(s_testBaseDir), NATIVE_STR8_LIT("presets/vanilla.ini"));
	NativePath_Join(expandedPath, sizeof(expandedPath), NativeStr8_FromCString(s_testBaseDir), NATIVE_STR8_LIT("presets/expanded.ini"));
	const NativeConfig initial = g_config;
	remove(profilePath);
	CTR_TEST_CHECK(NativeProfile_LoadStartup() == PROFILE_STARTUP_NO_PROFILE, "missing profile requests selection");
	CTR_TEST_CHECK(SDL_CreateDirectory(directory), "scratch preset directory exists");
	FILE *file = fopen(vanillaPath, "w");
	CTR_TEST_CHECK(file != NULL, "Vanilla preset fixture can be written");
	if (file == NULL) { return; }
	fputs("[General]\nskip_intro = false\nshow_reserves_meter = true\n", file);
	fclose(file);
	file = fopen(expandedPath, "w");
	CTR_TEST_CHECK(file != NULL, "Expanded preset fixture can be written");
	if (file == NULL) { remove(vanillaPath); return; }
	fputs("[General]\nskip_intro = true\nshow_reserves_meter = true\n", file);
	fclose(file);
	remove(configPath);
	CTR_TEST_CHECK(NativeConfig_SaveProfile(CONFIG_PROFILE_EXPANDED), "fresh install profile can be saved");
	g_config.skipIntro = false;
	g_config.showReservesMeter = false;
	CTR_TEST_CHECK(NativeProfile_LoadStartup() == PROFILE_STARTUP_READY && g_config.skipIntro && g_config.showReservesMeter,
	               "fresh install loads preset values without config.ini");
	file = fopen(configPath, "w");
	CTR_TEST_CHECK(file != NULL, "config fixture can be written");
	if (file == NULL) { remove(vanillaPath); remove(expandedPath); return; }
	fputs("[General]\nshow_reserves_meter = false\n", file);
	fclose(file);
	g_config.skipIntro = false;
	g_config.showReservesMeter = false;
	g_config.engineSelection = true;
	CTR_TEST_CHECK(NativeProfile_Select(CONFIG_PROFILE_EXPANDED, false) == CONFIG_LOAD_OK,
	               "profile can be selected for future defaults only");
	CTR_TEST_CHECK(!g_config.skipIntro && !g_config.showReservesMeter && g_config.engineSelection,
	               "future-default selection preserves every current option");
	ConfigProfile selected = CONFIG_PROFILE_VANILLA;
	CTR_TEST_CHECK(NativeConfig_LoadProfile(&selected) == CONFIG_LOAD_OK && selected == CONFIG_PROFILE_EXPANDED,
	               "future-default selection persists the selected profile");
	CTR_TEST_CHECK(NativeProfile_Select(CONFIG_PROFILE_EXPANDED, true) == CONFIG_LOAD_OK,
	               "selected preset can be applied");
	CTR_TEST_CHECK(g_config.skipIntro && g_config.showReservesMeter && g_config.engineSelection,
	               "applying a preset changes only its listed options");
	CTR_TEST_CHECK(NativeProfile_Select((ConfigProfile)99, true) == CONFIG_LOAD_INVALID,
	               "invalid profile selection is rejected");
	CTR_TEST_CHECK(NativeConfig_SaveProfile(CONFIG_PROFILE_EXPANDED), "Expanded profile can be selected");
	g_config.skipIntro = false;
	g_config.showReservesMeter = true;
	CTR_TEST_CHECK(NativeProfile_LoadStartup() == PROFILE_STARTUP_READY && NativeProfile_GetStartupStatus() == PROFILE_STARTUP_READY,
	               "selected Expanded preset loads successfully");
	CTR_TEST_CHECK(g_config.skipIntro && !g_config.showReservesMeter, "config overrides only its explicit value");
	CTR_TEST_CHECK(NativeConfig_SaveProfile(CONFIG_PROFILE_VANILLA), "Vanilla profile can be selected");
	CTR_TEST_CHECK(NativeProfile_LoadStartup() == PROFILE_STARTUP_READY && !g_config.skipIntro,
	               "Vanilla selection loads the Vanilla preset");
	file = fopen(profilePath, "w");
	CTR_TEST_CHECK(file != NULL, "profile can be edited manually");
	if (file != NULL)
	{
		fputs("[Profile]\nactive = Expanded\n", file);
		fclose(file);
		g_config.skipIntro = false;
		g_config.showReservesMeter = true;
		CTR_TEST_CHECK(NativeProfile_LoadStartup() == PROFILE_STARTUP_READY && g_config.skipIntro && !g_config.showReservesMeter,
		               "manual profile change affects missing values but preserves explicit config");
		NativeConfig_Save();
		file = fopen(profilePath, "w");
		CTR_TEST_CHECK(file != NULL, "profile can be changed after saving config");
		if (file != NULL)
		{
			fputs("[Profile]\nactive = Vanilla\n", file);
			fclose(file);
			g_config.skipIntro = false;
			g_config.showReservesMeter = true;
			CTR_TEST_CHECK(NativeProfile_LoadStartup() == PROFILE_STARTUP_READY && g_config.skipIntro && !g_config.showReservesMeter,
			               "saved explicit settings survive reload and a manual profile change");
		}
	}
	remove(vanillaPath);
	CTR_TEST_CHECK(NativeProfile_Select(CONFIG_PROFILE_VANILLA, true) == CONFIG_LOAD_NOT_FOUND,
	               "missing preset cannot be selected");
	CTR_TEST_CHECK(NativeConfig_LoadProfile(&selected) == CONFIG_LOAD_OK && selected == CONFIG_PROFILE_VANILLA,
	               "failed selection preserves the previously saved profile");
	CTR_TEST_CHECK(NativeProfile_LoadStartup() == PROFILE_STARTUP_PRESET_ERROR, "missing preset is reported without stopping startup");
	file = fopen(profilePath, "w");
	CTR_TEST_CHECK(file != NULL, "invalid profile fixture can be written");
	if (file != NULL)
	{
		fputs("[Profile]\nactive = Unknown\n", file);
		fclose(file);
		CTR_TEST_CHECK(NativeProfile_LoadStartup() == PROFILE_STARTUP_INVALID_PROFILE, "invalid profile is reported");
	}
	remove(profilePath);
	remove(configPath);
	remove(expandedPath);
	SDL_RemovePath(directory);
	g_config = initial;
}

static void TestDeveloperDefaultsAreImmutable(void)
{
	CTR_TEST_SECTION("developer defaults do not depend on load order");
	const NativeConfig initial = g_config;
	g_config.developerHacksEnabled = false;
	g_config.oneLapRaces = true;
	const ConfigEntry *gate = NULL;
	for (int i = 0; i < g_numConfigEntries; i++)
	{
		if (g_configEntries[i].valuePtr == &g_config.developerHacksEnabled)
		{
			gate = &g_configEntries[i];
			break;
		}
	}
	CTR_TEST_CHECK(gate != NULL, "developer gate is registered");
	NativeConfig_ApplyDependencies(gate);
	CTR_TEST_CHECK(!g_config.oneLapRaces, "disabled developer hacks reset before any load");
	g_config = initial;
}

static void TestConfigPresetPrecedence(void)
{
	CTR_TEST_SECTION("configuration overrides preset defaults");
	char presetPath[512];
	char configPath[512];
	NativePath_Join(presetPath, sizeof(presetPath), NativeStr8_FromCString(s_testBaseDir), NATIVE_STR8_LIT("preset-test.ini"));
	NativePath_Join(configPath, sizeof(configPath), NativeStr8_FromCString(s_testBaseDir), NATIVE_STR8_LIT("config.ini"));
	const NativeConfig initial = g_config;
	FILE *file = fopen(presetPath, "w");
	CTR_TEST_CHECK(file != NULL, "scratch preset can be created");
	if (file == NULL) { return; }
	fputs("[General]\nshow_reserves_meter = true\nskip_intro = true\n", file);
	fclose(file);
	file = fopen(configPath, "w");
	CTR_TEST_CHECK(file != NULL, "scratch configuration can be created");
	if (file == NULL) { remove(presetPath); return; }
	fputs("[General]\nshow_reserves_meter = false\n", file);
	fclose(file);
	g_config.showReservesMeter = false;
	g_config.skipIntro = false;
	CTR_TEST_CHECK(NativeConfig_ReadPresetDefaults(presetPath) == CONFIG_LOAD_OK, "valid preset loads");
	NativeConfig_Load();
	CTR_TEST_CHECK(!g_config.showReservesMeter && g_config.skipIntro,
	               "explicit config value wins while a missing value uses the preset");
	file = fopen(presetPath, "w");
	CTR_TEST_CHECK(file != NULL, "invalid preset can be created");
	if (file != NULL)
	{
		fputs("[General]\nskip_intro = true\nunknown = true\n", file);
		fclose(file);
		g_config.skipIntro = false;
		g_config.showReservesMeter = true;
		CTR_TEST_CHECK(NativeConfig_ReadPresetDefaults(presetPath) == CONFIG_LOAD_INVALID, "invalid preset is reported");
		NativeConfig_Load();
		CTR_TEST_CHECK(!g_config.skipIntro && !g_config.showReservesMeter,
		               "invalid preset is discarded and the user's configuration still loads");
	}
	remove(presetPath);
	remove(configPath);
	g_config = initial;
}

static void TestProfileDeveloperDependencies(void)
{
	CTR_TEST_SECTION("developer dependencies apply after preset and config layers");
	char presetPath[512], configPath[512];
	NativePath_Join(presetPath, sizeof(presetPath), NativeStr8_FromCString(s_testBaseDir), NATIVE_STR8_LIT("preset-test.ini"));
	NativePath_Join(configPath, sizeof(configPath), NativeStr8_FromCString(s_testBaseDir), NATIVE_STR8_LIT("config.ini"));
	const NativeConfig initial = g_config;
	FILE *file = fopen(presetPath, "w");
	CTR_TEST_CHECK(file != NULL, "developer preset fixture can be written");
	if (file == NULL) { return; }
	fputs("[Developer Hacks]\ndeveloper_hacks_enabled = true\none_lap_races = true\n", file);
	fclose(file);
	file = fopen(configPath, "w");
	CTR_TEST_CHECK(file != NULL, "developer config fixture can be written");
	if (file == NULL) { remove(presetPath); return; }
	fputs("[Developer Hacks]\ndeveloper_hacks_enabled = false\n", file);
	fclose(file);
	CTR_TEST_CHECK(NativeConfig_ReadPresetDefaults(presetPath) == CONFIG_LOAD_OK, "developer preset loads");
	NativeConfig_Load();
	CTR_TEST_CHECK(!g_config.developerHacksEnabled && !g_config.oneLapRaces,
	               "config disabling the gate resets preset developer hacks");
	file = fopen(configPath, "w");
	CTR_TEST_CHECK(file != NULL, "enabled developer config fixture can be written");
	if (file != NULL)
	{
		fputs("[Developer Hacks]\none_lap_races = true\ndeveloper_hacks_enabled = true\n", file);
		fclose(file);
		g_config.developerHacksEnabled = false;
		g_config.oneLapRaces = false;
		NativeConfig_Load();
		CTR_TEST_CHECK(g_config.developerHacksEnabled && g_config.oneLapRaces,
		               "config enables hacks regardless of key order");
	}
	remove(configPath);
	file = fopen(presetPath, "w");
	CTR_TEST_CHECK(file != NULL, "disabled developer preset fixture can be written");
	if (file != NULL)
	{
		fputs("[Developer Hacks]\none_lap_races = true\ndeveloper_hacks_enabled = false\n", file);
		fclose(file);
		CTR_TEST_CHECK(NativeConfig_ReadPresetDefaults(presetPath) == CONFIG_LOAD_OK, "disabled developer preset loads");
		NativeConfig_Load();
		CTR_TEST_CHECK(!g_config.developerHacksEnabled && !g_config.oneLapRaces,
		               "missing config still resets disabled preset hacks");
		g_config.developerHacksEnabled = true;
		g_config.oneLapRaces = true;
		CTR_TEST_CHECK(NativeConfig_ApplyPresetFile(presetPath) == CONFIG_LOAD_OK, "developer preset applies directly");
		CTR_TEST_CHECK(!g_config.developerHacksEnabled && !g_config.oneLapRaces,
		               "direct preset application resets disabled hacks");
	}
	remove(presetPath);
	g_config = initial;
}

static void TestProfileFile(void)
{
	CTR_TEST_SECTION("profile persistence is separate from configuration");
	char path[512];
	NativePath_Join(path, sizeof(path), NativeStr8_FromCString(s_testBaseDir), NATIVE_STR8_LIT("profile.ini"));
	remove(path);
	ConfigProfile profile = CONFIG_PROFILE_EXPANDED;
	CTR_TEST_CHECK(NativeConfig_LoadProfile(&profile) == CONFIG_LOAD_NOT_FOUND, "missing profile is reported");
	CTR_TEST_CHECK(profile == CONFIG_PROFILE_EXPANDED, "missing profile does not alter the output");
	CTR_TEST_CHECK(NativeConfig_SaveProfile(CONFIG_PROFILE_VANILLA), "Vanilla profile is saved");
	CTR_TEST_CHECK(NativeConfig_LoadProfile(&profile) == CONFIG_LOAD_OK && profile == CONFIG_PROFILE_VANILLA,
	               "Vanilla profile survives a round trip");
	CTR_TEST_CHECK(NativeConfig_SaveProfile(CONFIG_PROFILE_EXPANDED), "Expanded profile is saved");
	CTR_TEST_CHECK(NativeConfig_LoadProfile(&profile) == CONFIG_LOAD_OK && profile == CONFIG_PROFILE_EXPANDED,
	               "Expanded profile survives a round trip");
	CTR_TEST_CHECK(!NativeConfig_SaveProfile((ConfigProfile)99), "invalid profile cannot be saved");
	CTR_TEST_CHECK(NativeConfig_LoadProfile(&profile) == CONFIG_LOAD_OK && profile == CONFIG_PROFILE_EXPANDED,
	               "invalid save leaves the existing profile intact");
	FILE *file = fopen(path, "w");
	CTR_TEST_CHECK(file != NULL, "invalid profile fixture can be written");
	if (file != NULL)
	{
		fputs("[Profile]\nactive = unknown\n", file);
		fclose(file);
		CTR_TEST_CHECK(NativeConfig_LoadProfile(&profile) == CONFIG_LOAD_INVALID && profile == CONFIG_PROFILE_EXPANDED,
		               "invalid profile leaves the output intact");
	}
	file = fopen(path, "w");
	CTR_TEST_CHECK(file != NULL, "empty profile fixture can be written");
	if (file != NULL)
	{
		fputs("[Profile]\n", file);
		fclose(file);
		CTR_TEST_CHECK(NativeConfig_LoadProfile(&profile) == CONFIG_LOAD_INVALID && profile == CONFIG_PROFILE_EXPANDED,
		               "profile without an active value is invalid");
	}
	remove(path);
}

static void TestPresetFile(void)
{
	CTR_TEST_SECTION("preset files are validated before applying");
	char path[512];
	NativePath_Join(path, sizeof(path), NativeStr8_FromCString(s_testBaseDir), NATIVE_STR8_LIT("preset-test.ini"));
	const NativeConfig initial = g_config;
	FILE *file = fopen(path, "w");
	CTR_TEST_CHECK(file != NULL, "scratch preset can be written");
	if (file == NULL)
	{
		return;
	}
	fputs("[General]\nshow_reserves_meter = true\n[Vehicle]\nspeed_stat_multiplier = 120\n[Graphics]\nrender_scale = Original\n", file);
	fclose(file);
	g_config.showReservesMeter = false;
	g_config.skipIntro = true;
	CTR_TEST_CHECK(NativeConfig_ApplyPresetFile(path) == CONFIG_LOAD_OK, "valid preset is accepted");
	CTR_TEST_CHECK(g_config.showReservesMeter && g_config.speedMultiplier == 120 && g_config.renderScale == 1,
	               "boolean, integer and enum preset values are applied");
	CTR_TEST_CHECK(g_config.skipIntro, "options omitted from the preset are preserved");
	char configPath[512];
	NativePath_Join(configPath, sizeof(configPath), NativeStr8_FromCString(s_testBaseDir), NATIVE_STR8_LIT("config.ini"));
	file = fopen(configPath, "w");
	CTR_TEST_CHECK(file != NULL, "scratch config can be written for parser comparison");
	if (file != NULL)
	{
		fputs("[General]\nshow_reserves_meter = true\n[Vehicle]\nspeed_stat_multiplier = 120\n[Graphics]\nrender_scale = Original\n", file);
		fclose(file);
		g_config.showReservesMeter = false;
		g_config.speedMultiplier = 100;
		g_config.renderScale = 0;
		NativeConfig_Load();
		CTR_TEST_CHECK(g_config.showReservesMeter && g_config.speedMultiplier == 120 && g_config.renderScale == 1,
		               "config and preset files interpret valid values identically");
		remove(configPath);
	}

	const char *invalid[] = {
	    "[General]\nshow_reserves_meter = false\nunknown = true\n",
	    "[General]\nshow_reserves_meter = perhaps\n",
	    "[Vehicle]\nspeed_stat_multiplier = 999\n",
	    "[Graphics]\nrender_scale = 99\n",
	    "[General\nshow_reserves_meter = false\n",
	};
	for (int i = 0; i < (int)(sizeof(invalid) / sizeof(invalid[0])); i++)
	{
		file = fopen(path, "w");
		CTR_TEST_CHECK(file != NULL, "invalid scratch preset can be written");
		if (file == NULL)
		{
			break;
		}
		fputs(invalid[i], file);
		fclose(file);
		CTR_TEST_CHECK(NativeConfig_ApplyPresetFile(path) == CONFIG_LOAD_INVALID, "invalid preset is rejected");
		CTR_TEST_CHECK(g_config.showReservesMeter && g_config.speedMultiplier == 120 && g_config.renderScale == 1,
		               "invalid preset does not partially alter configuration");
	}
	remove(path);
	CTR_TEST_CHECK(NativeConfig_ApplyPresetFile(path) == CONFIG_LOAD_NOT_FOUND, "missing preset is reported");
	g_config = initial;
}

static void TestAdventureOptionsWithoutLegacyMigration(void)
{
	CTR_TEST_SECTION("Adventure options ignore obsolete keys");
	char path[512];
	NativePath_Join(path, sizeof(path), NativeStr8_FromCString(s_testBaseDir), NATIVE_STR8_LIT("config.ini"));
	const NativeConfig initial = g_config;
	const char *inputs[] = {
	    "[Adventure]\nchaos_mode = true\n",
	    "[Adventure]\nextra_options = true\nexpanded_options = false\n",
	    "[Adventure]\nexpanded_options = true\nextended_character_select = false\n",
	    "[Adventure]\nextra_options = true\n",
	};
	const bool initialValues[] = {false, true, false, true};
	const bool expected[] = {false, false, true, true};
	for (int i = 0; i < (int)(sizeof(inputs) / sizeof(inputs[0])); i++)
	{
		g_config = initial;
		g_config.expandedAdventureOptions = initialValues[i];
		FILE *file = fopen(path, "w");
		CTR_TEST_CHECK(file != NULL, "scratch config can be written");
		if (file == NULL)
		{
			break;
		}
		fputs(inputs[i], file);
		fclose(file);
		NativeConfig_Load();
		CTR_TEST_CHECK(g_config.expandedAdventureOptions == expected[i], "obsolete keys leave the initial value unchanged and explicit expanded_options is honored");
	}
	remove(path);
	g_config = initial;
}

int main(void)
{
	CtrTestScratch_Open(s_testBaseDir, sizeof(s_testBaseDir), "config-tests", NULL);
	CTR_TEST_SECTION("native_config tests");
	TestDeveloperDefaultsAreImmutable();
	TestProfileStartup();
	TestConfigPresetPrecedence();
	TestProfileDeveloperDependencies();
	TestProfileFile();
	TestPresetFile();
	TestAdventureOptionsWithoutLegacyMigration();
	CtrTestScratch_Remove("config-tests", NULL);
	return CtrTest_Summary();
}