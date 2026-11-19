#include <shared/ctr_test.h>
#include <common.h>
#include <chaos/chaos_save.h>
#include <platform/custom-tracks/native_track_library.h>
#include <platform/custom-tracks/native_track_records.h>

struct sData sdata_static;
struct Data data;
struct rData rdata;
struct OverlayDATA_230 D230;
NativeConfig g_config;

// Match the overlay's aliases without including its renderer and asset loaders.
#define MM_MENU_MAIN (D230.menuMainMenu)
#define MM_TITLE_MENU_STATE (D230.titleMenuState)
#define MM_MENU_PLAYERS_1P2P (D230.menuPlayers1P2P)
#define MM_MENU_ADVENTURE (D230.menuAdventure)
#define MM_MENU_CHAOS_PROMPT (D230.menuChaosPrompt)
#define MM_MENU_CHAOS_CONFIG (sdata->chaosMenu.menu)
#define MM_DESIRED_MENU_INDEX (D230.desiredMenuIndex)

static int s_setupEntries;
static int s_titleReturns;
static bool s_coop;
static struct NativeTrackMetadata s_customRelicTrack;
int NativeTrackRecords_Key(const struct NativeTrackMetadata *track, struct NativeTrackRecordKey *key) { (void)track; (void)key; return 0; }
int NativeTrackRecords_LoadCurrent(enum NativeTrackRecordKind kind, const struct NativeTrackMetadata *track, const struct NativeTrackRecordKey *key, void *data, size_t size)
{ (void)kind; (void)track; (void)key; (void)data; (void)size; return 0; }
int NativeTrackRecords_SaveCurrent(enum NativeTrackRecordKind kind, const struct NativeTrackMetadata *track, const struct NativeTrackRecordKey *key, const void *data, size_t size)
{ (void)kind; (void)track; (void)key; (void)data; (void)size; return 0; }
void Platform_LogWarn(const char *fmt, ...) { (void)fmt; }

// This fixture exercises the new-file flow without a custom track library.
const struct NativeTrackRegistry *NativeTrackLibrary_Current(void) { return NULL; }
int NativeTrackLibrary_RelicTargetTime(const struct NativeTrackMetadata *track, int tier) { (void)track; (void)tier; return 115200; }
const struct NativeTrackMetadata *NativeTrackRegistry_Get(const struct NativeTrackRegistry *registry, int levelID)
{
	(void)registry;
	return levelID == NATIVE_TRACK_CUSTOM_ID_BASE ? &s_customRelicTrack : NULL;
}
size_t NativeTrackRegistry_Count(const struct NativeTrackRegistry *registry)
{
	(void)registry;
	return 0;
}

void Chaos_PublishEffects(void) {}

void MM_ChaosConfig_OnEnter(void)
{
	s_setupEntries++;
}

void MM_JumpTo_Title_Returning(void)
{
	s_titleReturns++;
}

b32 MainGameEnd_IsCoopRace(struct GameTracker *gGT)
{
	(void)gGT;
	return s_coop;
}

#include <game/GAMEPROG.c>
// Begin is file-static: include its actual module to catch a missing reset at the caller.
#include <game/230/MM_NewLoad.c>
#include <game/230/chaos/MM_ChaosSession.c>
#include <game/230/chaos/MM_Chaos.c>

void MM_ChaosConfig_OpenSession(void)
{
	s_setupEntries++;
}

void RECTMENU_ClearInput(void) {}

static void ResetFixture(void)
{
	static struct GameTracker tracker;
	memset(&sdata_static, 0, sizeof(sdata_static));
	memset(&D230, 0, sizeof(D230));
	memset(&g_config, 0, sizeof(g_config));
	memset(&tracker, 0, sizeof(tracker));
	sdata->gGT = &tracker;
	tracker.gameMode1 = ADVENTURE_MODE;
	sdata->ptrActiveMenu = &MM_MENU_MAIN;
	s_setupEntries = 0;
	s_titleReturns = 0;
	s_coop = false;
}

static void TestSessionPromptDisabled(void)
{
	CTR_TEST_SECTION("disabled session Chaos skips setup and cannot reuse previously armed rules");
	const int modes[] = {ARCADE_MODE, 0, BATTLE_MODE};
	for (int i = 0; i < (int)len(modes); i++)
	{
		ResetFixture();
		sdata->gGT->gameMode1 = modes[i];
		sdata->chaosSettings.armed = true;
		sdata->chaosSettings.durationSeconds = 15;
		sdata->chaosSessionNextMenu = &MM_MENU_CHAOS_PROMPT;
		struct RectMenu next = {0};
		MM_ChaosSession_Begin(&MM_MENU_MAIN, &next);
		CTR_TEST_CHECK(MM_MENU_MAIN.ptrNextBox_InHierarchy == &next && next.ptrPrevBox_InHierarchy == &MM_MENU_MAIN,
		               "the disabled option preserves the ordinary menu route");
		CTR_TEST_CHECK(!sdata->chaosSettings.armed && sdata->chaosSessionNextMenu == NULL,
		               "a hidden prompt cannot leave an earlier Chaos session armed");
		CTR_TEST_CHECK_EQ(sdata->chaosSettings.durationSeconds, 15, "skipping the prompt retains the chosen rule values");
		CTR_TEST_CHECK_EQ(s_setupEntries, 0, "the disabled option never opens setup");
	}
}

static void TestSessionPrompt(void)
{
	CTR_TEST_SECTION("Arcade, VS and Battle share the live Chaos rules");
	const int modes[] = {ARCADE_MODE, 0, BATTLE_MODE};
	for (int i = 0; i < (int)len(modes); i++)
	{
		ResetFixture();
		GAMEPROG_ResetChaosSettings(&sdata->chaosSettings);
		sdata->gGT->gameMode1 = modes[i];
		g_config.chaosMode = true;
		struct RectMenu next = {0};
		MM_ChaosSession_Begin(&MM_MENU_MAIN, &next);
		CTR_TEST_CHECK(MM_MENU_MAIN.ptrNextBox_InHierarchy == &MM_MENU_CHAOS_PROMPT,
		               "each supported session opens the shared mode prompt");
		CTR_TEST_CHECK_EQ(MM_MENU_CHAOS_PROMPT.rowSelected, 0, "a new session defaults to the first row, Regular");
		MM_MENU_CHAOS_PROMPT.rowSelected = 1;
		MM_MenuProc_Chaos(&MM_MENU_CHAOS_PROMPT);
		CTR_TEST_CHECK_EQ(s_setupEntries, 1, "Chaos opens the shared settings screen");
		sdata->chaosSettings.durationSeconds = 15;
		MM_ChaosSession_FinishSetup(false);
		CTR_TEST_CHECK(MM_MENU_MAIN.ptrNextBox_InHierarchy == &MM_MENU_CHAOS_PROMPT,
		               "Back from settings returns to the mode prompt without advancing");
		MM_ChaosSession_FinishSetup(true);
		CTR_TEST_CHECK(MM_MENU_MAIN.ptrNextBox_InHierarchy == &next && next.ptrPrevBox_InHierarchy == &MM_MENU_MAIN,
		               "Confirm resumes the ordinary menu chain without adding a recap box");
		sdata->gGT->gameMode1 = modes[(i + 1) % len(modes)];
		MM_ChaosSession_Begin(&MM_MENU_MAIN, &next);
		CTR_TEST_CHECK_EQ(sdata->chaosSettings.durationSeconds, 15, "rules survive another mode selection in this session");
		CTR_TEST_CHECK_EQ(MM_MENU_CHAOS_PROMPT.rowSelected, 1, "another mode reuses the armed live rules");
		MM_MENU_CHAOS_PROMPT.rowSelected = 0;
		MM_MenuProc_Chaos(&MM_MENU_CHAOS_PROMPT);
		CTR_TEST_CHECK(!sdata->chaosSettings.armed, "Regular disarms the live rules");
		CTR_TEST_CHECK(MM_MENU_MAIN.ptrNextBox_InHierarchy == &next, "Regular skips setup and continues normally");
		MM_ChaosSession_Begin(&MM_MENU_MAIN, &next);
		MM_MENU_CHAOS_PROMPT.rowSelected = -1;
		MM_MenuProc_Chaos(&MM_MENU_CHAOS_PROMPT);
		CTR_TEST_CHECK((MM_MENU_MAIN.state & DRAW_NEXT_MENU_IN_HIERARCHY) == 0 && sdata->chaosSessionNextMenu == NULL,
		               "Back from the prompt returns to the main menu and clears its pending destination");
		CTR_TEST_CHECK(MM_MENU_MAIN.ptrNextBox_InHierarchy == NULL &&
		               MM_MENU_CHAOS_PROMPT.ptrPrevBox_InHierarchy == NULL &&
		               MM_MENU_CHAOS_PROMPT.ptrNextBox_InHierarchy == NULL,
		               "Back detaches the prompt from both directions of the menu hierarchy");
	}
}

static void TestSharedRulesAdventureRestore(void)
{
	CTR_TEST_SECTION("loading an Adventure replaces live rules changed in another mode");
	ResetFixture();
	GAMEPROG_ResetChaosSettings(&sdata->chaosSettings);
	CTR_TEST_CHECK(!sdata->chaosSettings.armed && sdata->chaosSettings.enabledCount > 0,
	               "shared rules initialize with usable defaults and Regular selected");
	sdata->chaosSettings.armed = true;
	sdata->chaosSettings.durationSeconds = 55;
	struct ChaosSettingsSaved saved;
	ChaosSave_Encode(&sdata->chaosSettings, &saved);
	const struct ChaosSettingsSaved original = saved;
	sdata->gGT->gameMode1 = ARCADE_MODE;
	g_config.chaosMode = true;
	struct RectMenu next = {0};
	MM_ChaosSession_Begin(&MM_MENU_MAIN, &next);
	CTR_TEST_CHECK_EQ(MM_MENU_CHAOS_PROMPT.rowSelected, 1, "Arcade reuses the previously loaded live rules");
	sdata->chaosSettings.durationSeconds = 15;
	MM_MENU_CHAOS_PROMPT.rowSelected = 0;
	MM_MenuProc_Chaos(&MM_MENU_CHAOS_PROMPT);
	CTR_TEST_CHECK(!sdata->chaosSettings.armed, "Regular in Arcade disarms the live rules");
	CTR_TEST_CHECK(memcmp(&saved, &original, sizeof(saved)) == 0, "changing live rules does not write the saved slot");
	sdata->gGT->gameMode1 = ADVENTURE_MODE;
	ChaosSave_Decode(&saved, &sdata->chaosSettings);
	CTR_TEST_CHECK(sdata->chaosSettings.armed, "loading the Adventure restores its saved Chaos activation");
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.durationSeconds, 55, "loading replaces the rules edited in Arcade");
	sdata->chaosSettings.armed = false;
	ChaosSave_Encode(&sdata->chaosSettings, &saved);
	sdata->chaosSettings.armed = true;
	ChaosSave_Decode(&saved, &sdata->chaosSettings);
	CTR_TEST_CHECK(!sdata->chaosSettings.armed, "loading a Regular Adventure disarms rules left active by Arcade");
}

static void TestAdventureChaosGate(void)
{
	CTR_TEST_SECTION("General's Chaos option gates Adventure independently of Expanded Options");
	for (int expanded = 0; expanded <= 1; expanded++)
	{
		for (int enabled = 0; enabled <= 1; enabled++)
		{
			ResetFixture();
			g_config.expandedAdventureOptions = expanded != 0;
			g_config.chaosMode = enabled != 0;
			MM_NewLoad_Begin(&MM_MENU_ADVENTURE);
			if (expanded)
			{
				CTR_TEST_CHECK_EQ(sdata->newLoad.step, NEWLOAD_STEP_DIFFICULTY, "Expanded Options still starts with difficulty");
				MM_MenuProc_AdvDifficulty(&D230.menuDifficulty);
			}
			if (enabled)
			{
				CTR_TEST_CHECK_EQ(sdata->newLoad.step, NEWLOAD_STEP_CHAOS, "the General toggle offers Chaos with or without expanded setup");
				CTR_TEST_CHECK_EQ(MM_MENU_CHAOS_PROMPT.rowSelected, 0, "Adventure starts on Regular even when Mode is the first screen");
				if (!expanded)
				{
					MM_MENU_CHAOS_PROMPT.rowSelected = -1;
					MM_MenuProc_Chaos(&MM_MENU_CHAOS_PROMPT);
					CTR_TEST_CHECK(!MM_NewLoad_InChain() && MM_MENU_ADVENTURE.ptrNextBox_InHierarchy == NULL,
					               "Back from a standalone Mode screen returns cleanly to New/Load");
					MM_NewLoad_Begin(&MM_MENU_ADVENTURE);
				}
				MM_MENU_CHAOS_PROMPT.rowSelected = 1;
				MM_MenuProc_Chaos(&MM_MENU_CHAOS_PROMPT);
			}
			if (expanded)
			{
				CTR_TEST_CHECK_EQ(sdata->newLoad.step, NEWLOAD_STEP_PLAYERS, "only Expanded Options offers player selection");
				MM_NewLoad_Advance(&MM_MENU_PLAYERS_1P2P);
			}
			CTR_TEST_CHECK_EQ(s_setupEntries, enabled, "only an enabled Chaos choice opens Adventure rules setup");
			if (enabled)
			{
				CTR_TEST_CHECK(sdata->ptrDesiredMenu == &MM_MENU_CHAOS_CONFIG, "Chaos opens the shared editor before leaving creation");
				MM_NewLoad_ReturnFromChaosSetup();
				CTR_TEST_CHECK(sdata->ptrDesiredMenu == &MM_MENU_MAIN && MM_NewLoad_InChain(),
				               "Back from rules preserves the active creation chain");
				MM_NewLoad_FinishChaosSetup();
			}
			CTR_TEST_CHECK(!MM_NewLoad_InChain() && MM_DESIRED_MENU_INDEX == MM_EXIT_ROUTE_ADV_NEW,
			               "all option combinations finish through normal Adventure creation");
			CTR_TEST_CHECK(sdata->chaosSettings.armed == (enabled != 0), "the new Adventure retains the selected Chaos state");
			CTR_TEST_CHECK((sdata->newLoad.previousMenu != NULL) == (expanded != 0),
			               "only expanded character selection needs a return destination; the garage route does not");
			if (expanded)
			{
				MM_NewLoad_OnCharacterSelectExit(false);
				CTR_TEST_CHECK(D230.menuDifficulty.ptrNextBox_InHierarchy ==
				                   (enabled ? &MM_MENU_CHAOS_PROMPT : &MM_MENU_PLAYERS_1P2P),
				               "character-select Back rebuilds only the enabled Adventure steps");
				MM_NewLoad_Retreat(&MM_MENU_PLAYERS_1P2P);
				CTR_TEST_CHECK_EQ(sdata->newLoad.step, enabled ? NEWLOAD_STEP_CHAOS : NEWLOAD_STEP_DIFFICULTY,
				                  "Back from Players skips Chaos when its General option is disabled");
			}
		}
	}
}

static void CheckFreshSettings(void)
{
	const struct ChaosSettings *settings = &sdata->chaosSettings;
	CTR_TEST_CHECK(!settings->armed, "a new file starts Regular");
	CTR_TEST_CHECK_EQ(settings->durationSeconds, 10, "duration defaults to ten seconds");
	CTR_TEST_CHECK_EQ(settings->triggerIntervalSeconds, 10, "interval defaults to ten seconds");
	CTR_TEST_CHECK_EQ(settings->trigger, CHAOS_TRIGGER_RANDOM, "trigger starts at its default");
	CTR_TEST_CHECK_EQ(settings->enabledCount, CHAOS_EFFECT_COUNT, "all default effects are counted");
	for (int i = 0; i < CHAOS_EFFECT_COUNT; i++)
	{
		CTR_TEST_CHECK(settings->effects[i], "every effect starts selected");
	}
	CTR_TEST_CHECK_EQ(settings->enabledCount, ChaosEffect_CountEnabled(settings->effects), "the cached count matches the effect array");
	CTR_TEST_CHECK_EQ(s_setupEntries, 0, "New does not inherit a previous file's Chaos setup handover");
}

// The Arcade menu builder belongs to MM_MenuFlow; this fixture exercises the
// new-file flow with its prepared row count rather than the whole title overlay.
void MM_ToggleRows_Difficulty(void)
{
	D230.menuDifficulty.rows = D230.rowsDifficulty;
	D230.rowsDifficulty[0].stringIndex = LNG_EASY;
	D230.rowsDifficulty[1].stringIndex = LNG_MEDIUM;
	D230.rowsDifficulty[2].stringIndex = LNG_HARD;
	D230.rowsDifficulty[g_config.extraDifficultyLevels ? 5 : 3].stringIndex = RECTMENU_STRING_NONE;
}

static void TestDifficultyChoices(void)
{
	CTR_TEST_SECTION("Adventure difficulty selection");
	const enum DifficultyLevel choices[] = {
	    DIFFICULTY_EASY, DIFFICULTY_NORMAL, DIFFICULTY_HARD,
	    DIFFICULTY_SUPER_HARD, DIFFICULTY_ULTRA_HARD,
	};
	for (int extra = 0; extra <= 1; extra++)
	{
		for (int row = 0; row < (extra ? 5 : 3); row++)
		{
			ResetFixture();
			g_config.expandedAdventureOptions = true;
			g_config.extraDifficultyLevels = extra != 0;
			MM_NewLoad_Begin(&MM_MENU_ADVENTURE);
			CTR_TEST_CHECK_EQ(D230.menuDifficulty.rowSelected, 1, "Medium is selected for a new file");
			CTR_TEST_CHECK_EQ(D230.rowsDifficulty[1].stringIndex, LNG_MEDIUM, "the Arcade Medium row is reused");
			CTR_TEST_CHECK((D230.menuDifficulty.state & (USE_SMALL_FONT | BIG_TEXT_IN_TITLE)) ==
			                   (USE_SMALL_FONT | BIG_TEXT_IN_TITLE),
			               "difficulty choices are compact with a large title");
			CTR_TEST_CHECK_EQ(D230.rowsDifficulty[extra ? 5 : 3].stringIndex, RECTMENU_STRING_NONE,
			                  "only the enabled difficulty rows are displayed");
			D230.menuDifficulty.rowSelected = row;
			MM_MenuProc_AdvDifficulty(&D230.menuDifficulty);
			CTR_TEST_CHECK_EQ(sdata->advDifficulty, choices[row], "the selected level is recorded");
			CTR_TEST_CHECK((D230.menuDifficulty.state & USE_SMALL_FONT) == 0, "the difficulty recap uses the large font");
			CTR_TEST_CHECK_EQ(sdata->newLoad.step, NEWLOAD_STEP_PLAYERS, "with Chaos disabled, selection skips Mode and advances to Players");
		}
	}
}

static void TestAdventureTimeLimits(void)
{
	CTR_TEST_SECTION("Adventure difficulty time limits");
	struct GameTracker tracker = {0};
	sdata->gGT = &tracker;
	tracker.gameMode1 = ADVENTURE_MODE;
	const int expected[] = {1000, 1200, 900, 800, 700};
	for (int i = 0; i < DIFFICULTY_COUNT; i++)
	{
		sdata->advDifficulty = (enum DifficultyLevel)i;
		CTR_TEST_CHECK_EQ(GAMEPROG_AdventureTimeLimit(1000), expected[i], "each difficulty scales the event time");
	}
	sdata->advDifficulty = DIFFICULTY_EASY;
	tracker.gameMode1 = BATTLE_MODE;
	CTR_TEST_CHECK_EQ(GAMEPROG_AdventureTimeLimit(1000), 1000, "non-Adventure event times remain unchanged");
	tracker.gameMode1 = ADVENTURE_MODE | RELIC_RACE;
	data.RelicTime[0] = 1000;
	CTR_TEST_CHECK_EQ(GAMEPROG_RelicTargetTime(0, 0), 1200, "solo relic targets use the chosen difficulty");
	s_coop = true;
	CTR_TEST_CHECK_EQ(GAMEPROG_RelicTargetTime(0, 0), 838, "co-op and difficulty multipliers compose after the existing float rounding");
	CTR_TEST_CHECK_EQ(GAMEPROG_RelicTargetTime(0, 0), 838, "reading the target repeatedly does not rescale it");
	sdata->advDifficulty = DIFFICULTY_NORMAL;
	CTR_TEST_CHECK_EQ(GAMEPROG_RelicTargetTime(0, 0), 699, "Normal retains the original co-op target");
	s_coop = false;
	CTR_TEST_SECTION("Arcade challenge difficulty time limits");
	const int arcadeValues[] = {ARCADE_DIFFICULTY_EASY, ARCADE_DIFFICULTY_NORMAL, ARCADE_DIFFICULTY_HARD,
	                            ARCADE_DIFFICULTY_SUPER_HARD, ARCADE_DIFFICULTY_ULTRA_HARD};
	const int arcadeExpected[] = {1200, 1000, 900, 800, 700};
	for (int i = 0; i < 5; i++)
	{
		tracker.arcadeDifficulty = arcadeValues[i];
		tracker.gameMode1 = ARCADE_MODE | RELIC_RACE;
		CTR_TEST_CHECK_EQ(GAMEPROG_RelicTargetTime(0, 0), arcadeExpected[i], "Arcade relic targets use the selected Arcade difficulty");
		tracker.gameMode1 = ARCADE_MODE | CRYSTAL_CHALLENGE;
		CTR_TEST_CHECK_EQ(GAMEPROG_AdventureTimeLimit(1000), arcadeExpected[i], "Arcade crystal clocks use the selected Arcade difficulty");
	}
	tracker.gameMode1 = ARCADE_MODE;
	CTR_TEST_CHECK_EQ(GAMEPROG_AdventureTimeLimit(1000), 1000, "ordinary Arcade races do not change their clocks");
	tracker.gameMode1 = ARCADE_MODE | RELIC_RACE;
	tracker.arcadeDifficulty = ARCADE_DIFFICULTY_NORMAL;
	CTR_TEST_CHECK_EQ(GAMEPROG_RelicTargetTime(NATIVE_TRACK_CUSTOM_ID_BASE, 0), 115200, "custom hundredths convert to CTR time units");
	tracker.arcadeDifficulty = ARCADE_DIFFICULTY_EASY;
	CTR_TEST_CHECK_EQ(GAMEPROG_RelicTargetTime(NATIVE_TRACK_CUSTOM_ID_BASE, 0), 138240, "custom targets use the same difficulty scaling as retail");
}

static void TestAdventureBotDifficulty(void)
{
	CTR_TEST_SECTION("Adventure bot difficulty");
	struct GameTracker tracker = {0};
	sdata->gGT = &tracker;
	tracker.gameMode1 = ADVENTURE_MODE;
	sdata->advDifficulty = DIFFICULTY_NORMAL;
	CTR_TEST_CHECK_EQ(GAMEPROG_AdventureBotDifficulty(200), 200, "Medium preserves the adaptive difficulty");
	CTR_TEST_CHECK_EQ(GAMEPROG_AdventureBossLosses(5), 5, "Medium retains boss loss assistance");
	sdata->advDifficulty = DIFFICULTY_EASY;
	CTR_TEST_CHECK_EQ(GAMEPROG_AdventureBotDifficulty(100), 75, "Easy lowers the bot coefficient");
	CTR_TEST_CHECK_EQ(GAMEPROG_AdventureBotDifficulty(300), 120, "Easy caps incremental difficulty");
	CTR_TEST_CHECK_EQ(GAMEPROG_AdventureLossIndex(0), 0, "Easy does not count a loss before the first attempt");
	CTR_TEST_CHECK_EQ(GAMEPROG_AdventureLossIndex(1), 3, "Easy gives more assistance after a loss");
	const int fixed[] = {240, 320, 640};
	for (int i = 0; i < 3; i++)
	{
		sdata->advDifficulty = (enum DifficultyLevel)(DIFFICULTY_HARD + i);
		CTR_TEST_CHECK_EQ(GAMEPROG_AdventureBotDifficulty(10), fixed[i], "fixed difficulties ignore the adaptive value");
		CTR_TEST_CHECK_EQ(GAMEPROG_AdventureBotDifficulty(200), fixed[i], "fixed difficulty remains the same after progress or losses");
		CTR_TEST_CHECK_EQ(GAMEPROG_AdventureBossLosses(5), 0, "fixed difficulty disables boss loss assistance");
		tracker.gameMode1 |= ADVENTURE_BOSS;
		CTR_TEST_CHECK_EQ(GAMEPROG_AdventureBotDifficulty(10), fixed[i] + 20, "bosses get the fixed difficulty bonus");
		tracker.gameMode1 &= ~ADVENTURE_BOSS;
	}
	tracker.gameMode1 = ARCADE_MODE;
	CTR_TEST_CHECK_EQ(GAMEPROG_AdventureBotDifficulty(100), 100, "Arcade does not use the saved Adventure setting");
	CTR_TEST_CHECK_EQ(GAMEPROG_AdventureBossLosses(5), 5, "other modes retain their loss behavior");
}

static void TestNewAfterChaosCreation(void)
{
	CTR_TEST_SECTION("New resets a previous Chaos creation with expanded options enabled or disabled");
	for (int expanded = 0; expanded <= 1; expanded++)
	{
			ResetFixture();
			g_config.expandedAdventureOptions = true;
			g_config.chaosMode = true;
			MM_NewLoad_Begin(&MM_MENU_ADVENTURE);
			MM_MenuProc_AdvDifficulty(&D230.menuDifficulty);
			MM_MENU_CHAOS_PROMPT.funcState = RECTMENU_FUNC_STATE_INPUT;
			MM_MENU_CHAOS_PROMPT.rowSelected = 1;
			MM_MenuProc_Chaos(&MM_MENU_CHAOS_PROMPT);
			MM_NewLoad_Advance(&MM_MENU_PLAYERS_1P2P);
			CTR_TEST_CHECK(sdata->chaosSettings.armed, "the preceding creation really selected Chaos");
			CTR_TEST_CHECK_EQ(s_setupEntries, 1, "the preceding creation reached Chaos setup");

			sdata->chaosSettings.durationSeconds = 45;
 			sdata->advDifficulty = DIFFICULTY_HARD;
			sdata->chaosSettings.triggerIntervalSeconds = 90;
			sdata->chaosSettings.trigger = CHAOS_TRIGGER_PICKUP;
			ChaosEffect_Toggle(sdata->chaosSettings.effects, &sdata->chaosSettings.enabledCount, CHAOS_EFFECT_FLY_ME_TO_THE_MOON);

			g_config.expandedAdventureOptions = expanded != 0;
			g_config.chaosMode = expanded != 0;
			s_setupEntries = 0;
			sdata->ptrDesiredMenu = NULL;
			// Clearing only armed here would leave the previous file's custom settings.
			MM_NewLoad_Begin(&MM_MENU_ADVENTURE);
			CheckFreshSettings();
			CTR_TEST_CHECK_EQ(sdata->advDifficulty, DIFFICULTY_NORMAL, "a new file resets the previous difficulty to Normal");
			CTR_TEST_CHECK(MM_NewLoad_InChain() == (expanded != 0), "New follows the expanded creation steps when enabled");
	}
}

static void TestNewAfterLoadedChaosSettings(void)
{
	CTR_TEST_SECTION("New with no prompt discards the live settings of a loaded Chaos file");
	ResetFixture();
	// Reproduce the live state left by loading a slot; disk I/O is outside this suite.
	struct ChaosSettings saved = {
		.durationSeconds = 55,
		.triggerIntervalSeconds = 115,
		.trigger = CHAOS_TRIGGER_EXTERNAL,
		.armed = true,
	};
	saved.effects[CHAOS_EFFECT_UNBREAKABLE] = true;
	saved.enabledCount = (u8)ChaosEffect_CountEnabled(saved.effects);
	sdata->chaosSettings = saved;

	MM_NewLoad_Begin(&MM_MENU_ADVENTURE);
	CheckFreshSettings();
	CTR_TEST_CHECK_EQ(MM_DESIRED_MENU_INDEX, MM_EXIT_ROUTE_ADV_NEW, "New continues through the normal creation route");
}

static void ReachCharacterSelect(bool expanded, bool chaos, int playerCount)
{
	ResetFixture();
	g_config.expandedAdventureOptions = expanded;
	g_config.chaosMode = expanded;
	MM_NewLoad_Begin(&MM_MENU_ADVENTURE);
	if (expanded)
	{
		MM_MenuProc_AdvDifficulty(&D230.menuDifficulty);
		MM_MENU_CHAOS_PROMPT.funcState = RECTMENU_FUNC_STATE_INPUT;
		MM_MENU_CHAOS_PROMPT.rowSelected = chaos;
		MM_MenuProc_Chaos(&MM_MENU_CHAOS_PROMPT);
	}
	if (expanded)
	{
		MM_MENU_PLAYERS_1P2P.rowSelected = playerCount - 1;
		MM_NewLoad_Advance(&MM_MENU_PLAYERS_1P2P);
	}
	if (chaos)
	{
		MM_NewLoad_FinishChaosSetup();
	}
	// MM_Title captures the player count before character selection starts.
	sdata->advMultiplayer.numPlayers = playerCount;
}

static void TestCharacterSelectBackPreservesChoices(void)
{
	CTR_TEST_SECTION("character-select Back restores Mode and Players after an overlay reset");
	for (int chaos = 0; chaos <= 1; chaos++)
	{
		for (int players = 1; players <= 2; players++)
		{
			ReachCharacterSelect(true, chaos != 0, players);
			memset(&D230, 0, sizeof(D230));
			MM_NewLoad_OnCharacterSelectExit(false);
			CTR_TEST_CHECK_EQ(s_titleReturns, 1, "Back starts the title return transition once");
			CTR_TEST_CHECK_EQ(MM_MENU_CHAOS_PROMPT.rowSelected, chaos, "the collapsed Mode row keeps the chosen mode");
			CTR_TEST_CHECK_EQ(MM_MENU_PLAYERS_1P2P.rowSelected, players - 1, "Players keeps the chosen player count");
			CTR_TEST_CHECK(MM_MENU_ADVENTURE.ptrNextBox_InHierarchy == &D230.menuDifficulty &&
			                   D230.menuDifficulty.ptrNextBox_InHierarchy == &MM_MENU_CHAOS_PROMPT,
			               "the restored chain includes Difficulty and Mode before Players");
			CTR_TEST_CHECK(MM_MENU_CHAOS_PROMPT.ptrNextBox_InHierarchy == &MM_MENU_PLAYERS_1P2P,
			               "Players is attached below the chosen mode");
			CTR_TEST_CHECK(MM_MENU_PLAYERS_1P2P.ptrPrevBox_InHierarchy == &MM_MENU_CHAOS_PROMPT,
			               "Players can retreat without depending on a render pass");
			CTR_TEST_CHECK((MM_MENU_PLAYERS_1P2P.state & ONLY_DRAW_TITLE) == 0, "the last choice is editable");
			CTR_TEST_CHECK(sdata->chaosSettings.armed == (chaos != 0), "Back preserves the actual mode as well as its display");

			MM_NewLoad_Retreat(&MM_MENU_PLAYERS_1P2P);
			CTR_TEST_CHECK_EQ(MM_MENU_CHAOS_PROMPT.rowSelected, chaos, "retreat to Mode keeps its previous selection");
			CTR_TEST_CHECK(MM_MENU_CHAOS_PROMPT.ptrNextBox_InHierarchy == NULL, "retreat removes the Players link");
			MM_NewLoad_Retreat(&MM_MENU_CHAOS_PROMPT);
			CTR_TEST_CHECK((D230.menuDifficulty.state & USE_SMALL_FONT) != 0, "returning to Difficulty restores compact rows");
			CTR_TEST_CHECK_EQ(D230.menuDifficulty.rowSelected, 1, "retreat restores Normal difficulty");
			MM_NewLoad_Retreat(&D230.menuDifficulty);
			CTR_TEST_CHECK(!MM_NewLoad_InChain(), "retreat past Difficulty finishes the chain");
			CTR_TEST_CHECK(MM_MENU_ADVENTURE.ptrNextBox_InHierarchy == NULL, "no stale child remains on New/Load");
		}
	}
}

static void TestCharacterSelectForwardConsumesReturn(void)
{
	CTR_TEST_SECTION("finishing creation cannot redirect a later character-select Back");
	ReachCharacterSelect(true, true, 2);
	MM_NewLoad_OnCharacterSelectExit(true);
	CTR_TEST_CHECK_EQ(s_titleReturns, 0, "proceeding does not start a title return");
	MM_NewLoad_OnCharacterSelectExit(false);
	CTR_TEST_CHECK(!MM_NewLoad_InChain(), "a later Back cannot restore the completed Adventure chain");
	CTR_TEST_CHECK(MM_MENU_ADVENTURE.ptrNextBox_InHierarchy == NULL, "a later mode does not gain an Adventure submenu");
	CTR_TEST_CHECK_EQ(s_titleReturns, 1, "a later Back follows the ordinary title return");

	ReachCharacterSelect(false, false, 1);
	MM_NewLoad_OnCharacterSelectExit(false);
	CTR_TEST_CHECK(!MM_NewLoad_InChain(), "the garage route leaves no extended-character-select return context");
}

static void TestResidentCreationRestore(void)
{
	CTR_TEST_SECTION("restoring SDATA and D230 restores the setup handover and return context");
	ResetFixture();
	CTR_TEST_CHECK(!MM_NewLoad_InChain(), "zero-initialized resident state has no active creation");
	g_config.expandedAdventureOptions = true;
	g_config.chaosMode = true;
	MM_NewLoad_Begin(&MM_MENU_ADVENTURE);
	MM_MenuProc_AdvDifficulty(&D230.menuDifficulty);
	MM_MENU_CHAOS_PROMPT.funcState = RECTMENU_FUNC_STATE_INPUT;
	MM_MENU_CHAOS_PROMPT.rowSelected = 1;
	MM_MenuProc_Chaos(&MM_MENU_CHAOS_PROMPT);
	MM_MENU_PLAYERS_1P2P.rowSelected = 1;
	MM_NewLoad_Advance(&MM_MENU_PLAYERS_1P2P);
	static struct sData resident;
	static struct OverlayDATA_230 overlay;
	memcpy(&resident, &sdata_static, sizeof(resident));
	memcpy(&overlay, &D230, sizeof(overlay));
	MM_NewLoad_FinishChaosSetup();
	MM_NewLoad_OnCharacterSelectExit(true);
	MM_NewLoad_Begin(&MM_MENU_ADVENTURE);

	// Same resident/overlay regions as a checkpoint; no menu re-entry hooks.
	memcpy(&sdata_static, &resident, sizeof(resident));
	memcpy(&D230, &overlay, sizeof(overlay));
	MM_NewLoad_Advance(&MM_MENU_PLAYERS_1P2P);
	CTR_TEST_CHECK_EQ(s_setupEntries, 1, "resuming after setup was offered cannot offer it a second time");
	CTR_TEST_CHECK(!MM_NewLoad_InChain(), "the restored chain finishes normally");
	CTR_TEST_CHECK(MM_MENU_ADVENTURE.ptrNextBox_InHierarchy == NULL, "unwind uses the restored parent links");
	sdata->advMultiplayer.numPlayers = 2;
	memcpy(&resident, &sdata_static, sizeof(resident));
	memcpy(&overlay, &D230, sizeof(overlay));
	MM_NewLoad_OnCharacterSelectExit(true);
	MM_NewLoad_Begin(&MM_MENU_ADVENTURE);
	memcpy(&sdata_static, &resident, sizeof(resident));
	memcpy(&D230, &overlay, sizeof(overlay));
	MM_NewLoad_OnCharacterSelectExit(false);
	CTR_TEST_CHECK(MM_NewLoad_InChain(), "a saved character-select return context is available again after restore");
	CTR_TEST_CHECK_EQ(MM_MENU_CHAOS_PROMPT.rowSelected, 1, "the restored return keeps Chaos selected");
	CTR_TEST_CHECK_EQ(MM_MENU_PLAYERS_1P2P.rowSelected, 1, "the restored return keeps 2P selected");
	MM_NewLoad_Retreat(&MM_MENU_PLAYERS_1P2P);
	MM_NewLoad_Retreat(&MM_MENU_CHAOS_PROMPT);
	MM_NewLoad_Retreat(&D230.menuDifficulty);
	CTR_TEST_CHECK(!MM_NewLoad_InChain(), "Back can unwind the restored chain completely");
}

static void TestCustomGhostValidation(void)
{
	union
	{
		struct GhostHeader header;
		unsigned char bytes[GHOST_RECORD_BUFFER_SIZE];
	} ghost = {0};
	struct GhostHeader *header = &ghost.header;
	unsigned char *packets = (unsigned char *)(header + 1);
	header->version = GHOST_TAPE_VERSION_RETAIL;
	header->characterID = 0;
	header->timeElapsedInRace = 1000;
	header->size = 2 * GHOST_SIZE_POSITION;
	packets[0] = GHOST_OP_POSITION;
	packets[GHOST_SIZE_POSITION] = GHOST_OP_POSITION;
	CTR_TEST_SECTION("custom ghost validation");
	CTR_TEST_CHECK(GAMEPROG_CustomGhostValid(header), "a complete recording is accepted");
	header->size--;
	CTR_TEST_CHECK(!GAMEPROG_CustomGhostValid(header), "a truncated packet is rejected");
	header->size++;
	packets[0] = GHOST_OP_IDLE;
	CTR_TEST_CHECK(!GAMEPROG_CustomGhostValid(header), "an idle packet before any position is rejected");
	packets[0] = GHOST_OP_POSITION;
	header->characterID = GAME_CHARACTER_COUNT;
	CTR_TEST_CHECK(!GAMEPROG_CustomGhostValid(header), "an unknown character is rejected");
}

int main(void)
{
	TestSharedRulesAdventureRestore();
	TestAdventureChaosGate();
	TestSessionPromptDisabled();
	TestSessionPrompt();
	TestCustomGhostValidation();
	TestAdventureBotDifficulty();
	TestAdventureTimeLimits();
	TestDifficultyChoices();
	TestNewAfterChaosCreation();
	TestNewAfterLoadedChaosSettings();
	TestCharacterSelectBackPreservesChoices();
	TestCharacterSelectForwardConsumesReturn();
	TestResidentCreationRestore();
	return CtrTest_Summary();
}
