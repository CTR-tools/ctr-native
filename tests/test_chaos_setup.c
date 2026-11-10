#include <shared/ctr_test.h>
#include <common.h>

struct sData sdata_static;
NativeConfig g_config;
struct Data data;
struct OverlayDATA_230 D230;
#define MM_TITLE_MENU_STATE (D230.titleMenuState)
#define MM_menuChaosConfig (sdata->chaosMenu.menu)
static struct GameTracker s_tracker;
static struct DB s_buffer;
static int s_drawnRows;
static int s_drawnFrames;
static const char *s_drawnTitle;
static bool s_drewInterval;
static bool s_drewEffects;
static bool s_drewConfirm;
static s16 s_confirmColor;
static bool s_drewOnOff;
static int s_finishes;
static int s_returns;
static bool s_drewBeforeExit;
static int s_testLevel;

const char *LANG_GetString(s32 index)
{
	static char values[LNG_EXPANDED_STRING_COUNT][24];
	if (index < LNG_EXPANDED_STRING_BASE || index >= LNG_EXPANDED_STRING_END)
	{
		return "";
	}
	char *value = values[index - LNG_EXPANDED_STRING_BASE];
	snprintf(value, sizeof(values[0]), "TEXT ID %d", (int)index);
	return value;
}

void MM_Title_CameraReset(void) {}
void MM_Title_KillThread(void) {}
void MM_QuickRace_SetupRace(void) {}
void GAMEPROG_NewProfile_InsideAdv(struct AdvProgress *progress)
{
	memset(progress, 0, sizeof(*progress));
}
void GAMEPROG_ResetAdvMultiplayer(struct AdvMultiplayerSettings *settings)
{
	settings->numPlayers = ADV_MULTIPLAYER_DEFAULT_PLAYERS;
	settings->characterID2 = ADV_MULTIPLAYER_NO_CHARACTER;
}
void MainRaceTrack_RequestLoad(s16 level)
{
	s_testLevel = level;
}
void RECTMENU_Hide(struct RectMenu *menu)
{
	menu->state |= NEEDS_TO_CLOSE;
}

void MM_NewLoad_FinishChaosSetup(void)
{
	s_finishes++;
	s_drewBeforeExit = s_drawnFrames != 0 && s_drawnRows != 0;
}

void MM_NewLoad_ReturnFromChaosSetup(void)
{
	s_returns++;
	s_drewBeforeExit = s_drawnFrames != 0 && s_drawnRows != 0;
}

// Exercise the real input callback; drawing doubles record submissions, not pixels.
#include <game/230/MM_ChaosConfig.c>
#include <game/MAIN/MainFreeze_Chaos.c>

static const char *s_drawnLabels[FULLSCREEN_MENU_MAX_VISIBLE_ROWS];

void RECTMENU_ClearInput(void)
{
	sdata->AnyPlayerTap = 0;
	sdata->AnyPlayerHold = 0;
}

int OtherFX_Play(u32 soundID, int flags)
{
	(void)soundID;
	(void)flags;
	return 0;
}

void DecalFont_DrawLineOT(char *text, s16 x, s16 y, s16 font, s16 flags, u32 *ot)
{
	(void)y;
	(void)font;
	(void)flags;
	(void)ot;
	if (x == FULLSCREEN_MENU_TITLE_X)
	{
		s_drawnTitle = text;
	}
	else if (x == FULLSCREEN_MENU_LABEL_X)
	{
		if (s_drawnRows < FULLSCREEN_MENU_MAX_VISIBLE_ROWS)
		{
			s_drawnLabels[s_drawnRows] = text;
		}
		s_drawnRows++;
		s_drewInterval |= text == LANG_GetString(LNG_EXPANDED_INTERVAL);
		s_drewEffects |= text == LANG_GetString(LNG_EXPANDED_EFFECTS) || text == LANG_GetString(LNG_EXPANDED_ACTIVE_EFFECTS);
		s_drewConfirm |= text == LANG_GetString(LNG_EXPANDED_CONFIRM);
		if (text == LANG_GetString(LNG_EXPANDED_CONFIRM))
		{
			s_confirmColor = flags;
		}
	}
	else if (x == FULLSCREEN_MENU_VALUE_X)
	{
		s_drewOnOff |= text == LANG_GetString(LNG_EXPANDED_ON) || text == LANG_GetString(LNG_EXPANDED_OFF);
	}
}

void RECTMENU_DrawInnerRect(RECT *rect, int inset, u32 *ot)
{
	(void)rect;
	(void)inset;
	(void)ot;
	s_drawnFrames++;
}

void RECTMENU_DrawOuterRect_Edge(RECT *rect, const Color *color, u32 flags, u32 *ot)
{
	(void)rect;
	(void)color;
	(void)flags;
	(void)ot;
}

void RECTMENU_DrawFullRect(struct RectMenu *menu, RECT *rect)
{
	(void)menu;
	RECTMENU_DrawInnerRect(rect, 4, NULL);
}

void CTR_Box_DrawClearBox(const RECT *rect, const Color *color, s32 transparency, u32 *ot, struct PrimMem *prims)
{
	(void)rect;
	(void)color;
	(void)transparency;
	(void)ot;
	(void)prims;
}

static void EnterSetup(void)
{
	memset(&sdata_static, 0, sizeof(sdata_static));
	memset(&g_config, 0, sizeof(g_config));
	s_testLevel = -1;
	memset(&s_tracker, 0, sizeof(s_tracker));
	memset(&s_buffer, 0, sizeof(s_buffer));
	sdata->gGT = &s_tracker;
	s_tracker.backBuffer = &s_buffer;
	sdata->gGamepads = &sdata->gamepadSystem;
	sdata->chaosSettings.durationSeconds = CHAOS_DURATION_DEFAULT_SECONDS;
	sdata->chaosSettings.triggerIntervalSeconds = CHAOS_TRIGGER_INTERVAL_DEFAULT_SECONDS;
	sdata->chaosSettings.armed = true;
	sdata->chaosSettings.enabledCount = CHAOS_EFFECT_COUNT;
	for (int effect = 0; effect < CHAOS_EFFECT_COUNT; effect++)
	{
		sdata->chaosSettings.effects[effect] = true;
	}
	s_finishes = 0;
	s_returns = 0;
	s_drewBeforeExit = false;
	MM_ChaosConfig_OnEnter();
}

static void Frame(int tapped, int held)
{
	s_drawnRows = 0;
	s_drawnFrames = 0;
	s_drawnTitle = NULL;
	s_drewInterval = false;
	s_drewEffects = false;
	s_drewConfirm = false;
	s_confirmColor = -1;
	s_drewOnOff = false;
	memset(s_drawnLabels, 0, sizeof(s_drawnLabels));
	sdata->gGamepads->gamepad[0].buttonsTapped = tapped;
	sdata->gGamepads->gamepad[0].buttonsHeldCurrFrame = held;
	sdata->AnyPlayerTap = tapped;
	sdata->AnyPlayerHold = held;
	MM_MenuProc_ChaosConfig(&MM_menuChaosConfig);
}

static void TestTriggerLayoutAndIntervalRetention(void)
{
	CTR_TEST_SECTION("trigger changes preserve the hidden interval and keep both action rows");
	EnterSetup();
	sdata->chaosSettings.triggerIntervalSeconds = 20;
	MM_menuChaosConfig.rowSelected = 2;
	Frame(0, BTN_RIGHT);
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.triggerIntervalSeconds, 25, "the numeric interval commits immediately");
	MM_menuChaosConfig.rowSelected = 1;
	for (int trigger = CHAOS_TRIGGER_PICKUP; trigger <= CHAOS_TRIGGER_COUNT; trigger++)
	{
		Frame(BTN_CROSS, 0);
		const int expectedTrigger = trigger % CHAOS_TRIGGER_COUNT;
		const bool timed = expectedTrigger == CHAOS_TRIGGER_RANDOM || expectedTrigger == CHAOS_TRIGGER_INTERVAL ||
		                   expectedTrigger == CHAOS_TRIGGER_EXTERNAL;
		CTR_TEST_CHECK_EQ(sdata->chaosSettings.trigger, expectedTrigger, "Cross cycles the stored trigger");
		CTR_TEST_CHECK_EQ(s_drawnRows, timed ? 5 : 4, "the destination layout is drawn in the same frame");
		CTR_TEST_CHECK(s_drewInterval == timed, "Interval is present only for timed triggers");
		CTR_TEST_CHECK(s_drewEffects && s_drewConfirm, "Effects and Confirm remain visible after every trigger change");
		CTR_TEST_CHECK_EQ(sdata->chaosSettings.triggerIntervalSeconds, 25, "hiding Interval never loses its value");
		if (expectedTrigger == CHAOS_TRIGGER_EXTERNAL)
		{
			MM_menuChaosConfig.rowSelected = 2;
			Frame(0, BTN_RIGHT);
			CTR_TEST_CHECK_EQ(sdata->chaosSettings.triggerIntervalSeconds, 30, "External exposes an editable interval");
			Frame(0, BTN_LEFT);
			MM_menuChaosConfig.rowSelected = 1;
		}
	}
}

static void TestEffectsCommitAndPageTransitions(void)
{
	CTR_TEST_SECTION("effect toggles commit directly and Back draws the destination page");
	EnterSetup();
	MM_menuChaosConfig.rowSelected = 3;
	Frame(BTN_CROSS, 0);
	CTR_TEST_CHECK(s_drawnTitle == LANG_GetString(LNG_EXPANDED_CHAOS_EFFECTS_TITLE), "opening Effects draws its title immediately");
	CTR_TEST_CHECK_EQ(s_drawnRows, FULLSCREEN_MENU_MAX_VISIBLE_ROWS, "Effects uses the shared visible-row limit");
	CTR_TEST_CHECK(s_drewOnOff, "creation still shows the editable effect toggles");
	MM_menuChaosConfig.rowSelected = CHAOS_EFFECT_FLY_ME_TO_THE_MOON;
	Frame(BTN_CROSS, 0);
	CTR_TEST_CHECK(!sdata->chaosSettings.effects[CHAOS_EFFECT_FLY_ME_TO_THE_MOON], "Cross updates the stored effect without leaving its page");
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.enabledCount, CHAOS_EFFECT_COUNT - 1, "the cached count changes with the toggle");
	Frame(BTN_TRIANGLE, 0);
	CTR_TEST_CHECK(s_drawnTitle == LANG_GetString(LNG_EXPANDED_CHAOS_SETUP_TITLE), "Back draws Settings immediately");
	CTR_TEST_CHECK_EQ(MM_menuChaosConfig.rowSelected, 3, "Back selects the Effects action");
	CTR_TEST_CHECK_EQ(s_drawnRows, 5, "the return frame contains all settings rows");
	CTR_TEST_CHECK_EQ(s_returns, 0, "Effects Back stays inside the setup screen");
	MM_menuChaosConfig.rowSelected = 4;
	Frame(BTN_CROSS, 0);
	CTR_TEST_CHECK_EQ(s_finishes, 1, "Confirm proceeds once");
	CTR_TEST_CHECK(s_drewBeforeExit, "Confirm draws before handing control back to the chain");
}

static void TestConfirmRequiresAnEffect(void)
{
	CTR_TEST_SECTION("Confirm requires at least one selected effect and updates after toggling");
	EnterSetup();
	MM_menuChaosConfig.rowSelected = sdata->chaosMenu.effectsRow;
	Frame(BTN_CROSS, 0);
	for (int effect = 0; effect < CHAOS_EFFECT_COUNT; effect++)
	{
		MM_menuChaosConfig.rowSelected = effect;
		Frame(BTN_CROSS, 0);
	}
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.enabledCount, 0, "all effects can be deselected during creation");
	Frame(BTN_TRIANGLE, 0);
	CTR_TEST_CHECK(s_drewConfirm && s_confirmColor == GRAY, "Confirm remains visible but gray with no effects selected");
	MM_menuChaosConfig.rowSelected = sdata->chaosMenu.numSettings - 1;
	Frame(BTN_CROSS, 0);
	Frame(BTN_CIRCLE, 0);
	CTR_TEST_CHECK_EQ(s_finishes, 0, "neither activation button can finish an empty Chaos selection");
	CTR_TEST_CHECK_EQ(s_returns, 0, "a disabled Confirm keeps the creation settings open");
	MM_menuChaosConfig.rowSelected = sdata->chaosMenu.effectsRow;
	Frame(BTN_CROSS, 0);
	Frame(BTN_CROSS, 0);
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.enabledCount, 1, "one effect can be re-enabled without reopening setup");
	Frame(BTN_TRIANGLE, 0);
	CTR_TEST_CHECK_EQ(s_confirmColor, ORANGE, "Confirm regains its normal color as soon as an effect is selected");
	MM_menuChaosConfig.rowSelected = sdata->chaosMenu.numSettings - 1;
	Frame(BTN_CROSS, 0);
	CTR_TEST_CHECK_EQ(s_finishes, 1, "one selected effect is sufficient to confirm creation");
	CTR_TEST_CHECK(s_drewBeforeExit, "re-enabled Confirm preserves the draw-before-exit handoff");
}

static void TestLiveValuesAndInputKinds(void)
{
	CTR_TEST_SECTION("rendering reads live values and input changes only the intended field");
	EnterSetup();
	sdata->chaosSettings.durationSeconds = 35;
	sdata->chaosSettings.triggerIntervalSeconds = 65;
	unsigned char before[sizeof(struct ChaosSettings)];
	memcpy(before, &sdata->chaosSettings, sizeof(before));
	Frame(0, 0);
	CTR_TEST_CHECK(memcmp(before, &sdata->chaosSettings, sizeof(before)) == 0, "an idle frame cannot overwrite live settings with cached values");
	Frame(BTN_CROSS, 0);
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.durationSeconds, 35, "Cross does not step numeric values");
	Frame(0, BTN_RIGHT);
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.durationSeconds, 40, "a slider steps from the live value rather than a mirror");
	MM_menuChaosConfig.rowSelected = 1;
	Frame(0, BTN_RIGHT);
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.trigger, CHAOS_TRIGGER_RANDOM, "directions do not cycle enum choices");
	Frame(BTN_TRIANGLE, 0);
	CTR_TEST_CHECK_EQ(s_returns, 1, "Settings Back returns to the chain once");
	CTR_TEST_CHECK(s_drewBeforeExit, "Settings Back draws before handing control back to the chain");
	MM_ChaosConfig_OnEnter();
	CTR_TEST_CHECK_EQ(MM_menuChaosConfig.rowSelected, 0, "a new visit starts on Duration");
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.durationSeconds, 40, "re-entering setup preserves the current creation's values");
}

static void TestResidentRestore(void)
{
	CTR_TEST_SECTION("restoring resident state resumes Effects and the saved settings layout");
	EnterSetup();
	// Snapshot precisely the resident aggregate owned by the SDATA checkpoint
	// region. Leaving page/scroll/cache state file-static breaks this round trip.
	MM_menuChaosConfig.rowSelected = 1;
	Frame(BTN_CROSS, 0); // Pickup hides Interval.
	MM_menuChaosConfig.rowSelected = 2;
	Frame(BTN_CROSS, 0); // Effects.
	MM_menuChaosConfig.rowSelected = CHAOS_EFFECT_COUNT - 1;
	Frame(BTN_CROSS, 0);
	static struct sData snapshot;
	memcpy(&snapshot, &sdata_static, sizeof(snapshot));
	const int savedScroll = sdata->chaosMenu.scroll[CHAOS_PAGE_EFFECTS].offset;
	CTR_TEST_CHECK(savedScroll > 0, "the saved Effects page is scrolled beyond its first row");

	Frame(BTN_TRIANGLE, 0);
	MM_menuChaosConfig.rowSelected = 1;
	Frame(BTN_CROSS, 0); // Interval restores the hidden row.
	MM_ChaosConfig_OnEnter();
	memcpy(&sdata_static, &snapshot, sizeof(snapshot));
	Frame(0, 0);
	CTR_TEST_CHECK(s_drawnTitle == LANG_GetString(LNG_EXPANDED_CHAOS_EFFECTS_TITLE),
	               "an idle frame resumes the saved page rather than the later visit");
	CTR_TEST_CHECK_EQ(MM_menuChaosConfig.rowSelected, CHAOS_EFFECT_COUNT - 1, "the saved cursor resumes on the last effect");
	CTR_TEST_CHECK_EQ(sdata->chaosMenu.scroll[CHAOS_PAGE_EFFECTS].offset, savedScroll, "the saved scroll position survives the next frame");
	CTR_TEST_CHECK(!sdata->chaosSettings.effects[CHAOS_EFFECT_COUNT - 1], "the saved toggle survives without a stale value mirror");
	CTR_TEST_CHECK(MM_menuChaosConfig.funcPtr == MM_MenuProc_ChaosConfig, "the resident menu retains its input callback");
	Frame(BTN_TRIANGLE, 0);
	CTR_TEST_CHECK_EQ(MM_menuChaosConfig.rowSelected, 2, "Back uses the saved Effects action position");
	CTR_TEST_CHECK_EQ(s_drawnRows, 4, "Back resumes the saved untimed layout");
	CTR_TEST_CHECK(!s_drewInterval && s_drewEffects && s_drewConfirm, "the restored row cache agrees with the restored trigger");
	MM_menuChaosConfig.rowSelected = 3;
	Frame(BTN_CROSS, 0);
	CTR_TEST_CHECK_EQ(s_finishes, 1, "Confirm still proceeds from the restored layout");
}

static void TestPauseEntryVisibility(void)
{
	CTR_TEST_SECTION("only an active Chaos Adventure gains a pause action");
	EnterSetup();
	struct MenuRow base[] = {
		{LNG_RESUME, 2, 1, 0, 0},
		{LNG_EXIT_TO_MAP, 0, 2, 1, 1},
		{LNG_OPTIONS, 1, 0, 2, 2},
		{RECTMENU_STRING_NONE, 0, 0, 0, 0},
	};
	struct RectMenu pause = {.rows = base, .rowSelected = 3};
	const struct
	{
		u32 mode;
		bool adventurePause;
	} cases[] = {
		{ADVENTURE_ARENA, true},
		{ADVENTURE_MODE, true},
		{ADVENTURE_MODE | ADVENTURE_CUP, true},
		{ADVENTURE_MODE | RELIC_RACE, true},
		{ADVENTURE_MODE | CRYSTAL_CHALLENGE, true},
		{ADVENTURE_MODE | ADVENTURE_BOSS, true},
		{ARCADE_MODE, false},
		{TIME_TRIAL, false},
		{BATTLE_MODE, false},
		{MAIN_MENU | ADVENTURE_MODE, false},
		{GAME_CUTSCENE | ADVENTURE_MODE, false},
	};
	for (int i = 0; i < (int)len(cases); i++)
	{
		for (int armed = 0; armed <= 1; armed++)
		{
			for (int players = 1; players <= 2; players++)
			{
				s_tracker.gameMode1 = cases[i].mode | PAUSE_1;
				s_tracker.numPlyrCurrGame = players;
				sdata->chaosSettings.armed = armed != 0;
				const bool visible = armed && cases[i].adventurePause;
				MainFreeze_ChaosPauseMenu(&pause, base, 3);
				CTR_TEST_CHECK((pause.rows == sdata->chaosPauseRows) == visible, "the action requires a Chaos Adventure file in hub, race or cup for either player count");
				if (visible)
				{
					CTR_TEST_CHECK_EQ(pause.rows[3].stringIndex, LNG_EXPANDED_CHAOS_RULES, "Chaos Rules is appended after Options");
					CTR_TEST_CHECK_EQ(pause.rows[0].rowOnPressUp, 3, "Up from Resume wraps to Chaos Rules");
					CTR_TEST_CHECK_EQ(pause.rows[2].rowOnPressDown, 3, "Down from Options reaches Chaos Rules");
					CTR_TEST_CHECK_EQ(pause.rows[3].rowOnPressDown, 0, "Down from Chaos Rules wraps to Resume");
					CTR_TEST_CHECK_EQ(pause.rows[4].stringIndex, RECTMENU_STRING_NONE, "the extended list has a real terminator");
				}
			}
		}
	}
	s_tracker.gameMode1 = ADVENTURE_MODE | PAUSE_1;
	sdata->chaosSettings.armed = true;
	memset(sdata->chaosSettings.effects, 0, sizeof(sdata->chaosSettings.effects));
	sdata->chaosSettings.enabledCount = 0;
	MainFreeze_ChaosPauseMenu(&pause, base, 3);
	CTR_TEST_CHECK(pause.rows == sdata->chaosPauseRows, "Chaos with zero selected effects still exposes its settings");
	CTR_TEST_CHECK_EQ(base[0].rowOnPressUp, 2, "building the extra action never edits the base rows");
}

static void TestReadOnlyPauseView(void)
{
	CTR_TEST_SECTION("pause reuses Settings and Effects without changing the profile");
	EnterSetup();
	s_tracker.gameMode1 = ADVENTURE_MODE | PAUSE_1;
	struct RectMenu pause = {.rowSelected = 4};
	unsigned char before[sizeof(struct ChaosSettings)];
	memcpy(before, &sdata->chaosSettings, sizeof(before));
	MM_ChaosConfig_OpenReadOnly(&pause);
	CTR_TEST_CHECK(sdata->ptrDesiredMenu == &MM_menuChaosConfig, "the viewer becomes a standalone menu");
	sdata->ptrDesiredMenu = NULL;
	for (int row = 0; row < 3; row++)
	{
		MM_menuChaosConfig.rowSelected = row;
		Frame(BTN_CROSS | BTN_CIRCLE, BTN_LEFT);
		Frame(0, BTN_RIGHT);
	}
	CTR_TEST_CHECK(s_drawnTitle == LANG_GetString(LNG_EXPANDED_CHAOS_RULES), "pause uses the Chaos Rules title");
	CTR_TEST_CHECK_EQ(s_drawnRows, 4, "the timed read-only page contains the values and Effects");
	CTR_TEST_CHECK(!s_drewConfirm && s_drewInterval && s_drewEffects, "the pause view has no creation Confirm action");
	MM_menuChaosConfig.rowSelected = 3;
	Frame(BTN_CROSS, 0);
	CTR_TEST_CHECK(s_drawnTitle == LANG_GetString(LNG_EXPANDED_ACTIVE_EFFECTS), "Active Effects draws immediately in the read-only view");
	for (int effect = 0; effect < CHAOS_EFFECT_COUNT; effect++)
	{
		MM_menuChaosConfig.rowSelected = (s16)effect;
		Frame(BTN_CROSS | BTN_CIRCLE, BTN_RIGHT);
	}
	CTR_TEST_CHECK(sdata->chaosMenu.scroll[CHAOS_PAGE_EFFECTS].offset > 0, "the read-only effect list still scrolls to the last effect");
	CTR_TEST_CHECK(memcmp(before, &sdata->chaosSettings, sizeof(before)) == 0, "confirm buttons and directions cannot change any saved parameter or flag");
	Frame(BTN_TRIANGLE, 0);
	CTR_TEST_CHECK_EQ(MM_menuChaosConfig.rowSelected, 3, "Effects Back returns to its Settings action");
	CTR_TEST_CHECK(sdata->ptrDesiredMenu == NULL, "Effects Back stays inside the viewer");
	Frame(BTN_TRIANGLE, 0);
	CTR_TEST_CHECK(sdata->ptrDesiredMenu == &pause, "Settings Back returns to the original pause menu");
	CTR_TEST_CHECK_EQ(pause.rowSelected, 4, "the pause menu retains its selected action");
	CTR_TEST_CHECK((s_tracker.gameMode1 & PAUSE_1) != 0, "closing the viewer keeps the game paused");
	CTR_TEST_CHECK_EQ(s_finishes + s_returns, 0, "the viewer never calls the new-file chain");

	// All players may inspect pause settings, matching the parent pause menu.
	MM_ChaosConfig_OpenReadOnly(&pause);
	MM_menuChaosConfig.rowSelected = 3;
	sdata->gGamepads->gamepad[0].buttonsTapped = 0;
	sdata->AnyPlayerTap = BTN_CROSS;
	MM_MenuProc_ChaosConfig(&MM_menuChaosConfig);
	CTR_TEST_CHECK_EQ(sdata->chaosMenu.page, CHAOS_PAGE_EFFECTS, "a confirm from another player can open Effects");
	Frame(BTN_START, 0);
	CTR_TEST_CHECK(sdata->ptrDesiredMenu == &pause, "Start returns from Effects to pause");
	MM_ChaosConfig_OpenReadOnly(&pause);
	Frame(BTN_SQUARE_one, 0);
	CTR_TEST_CHECK(sdata->ptrDesiredMenu == &pause, "Square also returns to pause");

	sdata->chaosSettings.trigger = CHAOS_TRIGGER_EXTERNAL;
	MM_ChaosConfig_OpenReadOnly(&pause);
	Frame(0, 0);
	CTR_TEST_CHECK(s_drewInterval && s_drawnRows == 4, "External shows Interval in the pause rules");
	const u16 externalInterval = sdata->chaosSettings.triggerIntervalSeconds;
	MM_menuChaosConfig.rowSelected = 2;
	Frame(0, BTN_RIGHT);
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.triggerIntervalSeconds, externalInterval, "the External pause interval remains read-only");

	sdata->chaosSettings.trigger = CHAOS_TRIGGER_PICKUP;
	MM_ChaosConfig_OpenReadOnly(&pause);
	Frame(0, 0);
	CTR_TEST_CHECK_EQ(s_drawnRows, 3, "untimed triggers omit Interval in the pause view too");
	MM_menuChaosConfig.rowSelected = 2;
	Frame(BTN_CROSS, 0);
	static struct sData snapshot;
	memcpy(&snapshot, &sdata_static, sizeof(snapshot));
	MM_ChaosConfig_OnEnter();
	memcpy(&sdata_static, &snapshot, sizeof(snapshot));
	Frame(BTN_CROSS, 0);
	CTR_TEST_CHECK(sdata->chaosMenu.returnMenu == &pause, "resident restore retains the read-only mode and pause return destination");
	Frame(BTN_START, 0);
	CTR_TEST_CHECK(sdata->ptrDesiredMenu == &pause, "a restored viewer still returns to the correct pause menu");
}

static void TestFilteredActiveEffects(void)
{
	CTR_TEST_SECTION("Active Effects lists only enabled names and handles an empty selection");
	EnterSetup();
	memset(sdata->chaosSettings.effects, 0, sizeof(sdata->chaosSettings.effects));
	sdata->chaosSettings.effects[CHAOS_EFFECT_GROUNDED] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_FAIR_MODE] = true;
	sdata->chaosSettings.effects[CHAOS_EFFECT_BIG_KART_ENERGY] = true;
	sdata->chaosSettings.enabledCount = 3;
	struct RectMenu pause = {0};
	MM_ChaosConfig_OpenReadOnly(&pause);
	MM_menuChaosConfig.rowSelected = 3;
	Frame(0, 0);
	CTR_TEST_CHECK(s_drawnLabels[3] == LANG_GetString(LNG_EXPANDED_ACTIVE_EFFECTS), "the Rules action names the filtered page");
	Frame(BTN_CROSS, 0);
	CTR_TEST_CHECK_EQ(s_drawnRows, 3, "disabled effects do not leave rows or gaps");
	CTR_TEST_CHECK(s_drawnLabels[0] == LANG_GetString(LNG_EXPANDED_CHAOS_EFFECT_BASE + CHAOS_EFFECT_GROUNDED), "the first visible row maps past the disabled first effect");
	CTR_TEST_CHECK(s_drawnLabels[1] == LANG_GetString(LNG_EXPANDED_CHAOS_EFFECT_BASE + CHAOS_EFFECT_FAIR_MODE), "the next row skips disabled effects in the middle");
	CTR_TEST_CHECK(s_drawnLabels[2] == LANG_GetString(LNG_EXPANDED_CHAOS_EFFECT_BASE + CHAOS_EFFECT_BIG_KART_ENERGY), "the final enabled effect is included");
	CTR_TEST_CHECK(!s_drewOnOff, "the read-only list has no ON/OFF column");
	Frame(BTN_UP, 0);
	CTR_TEST_CHECK_EQ(MM_menuChaosConfig.rowSelected, 2, "navigation wraps within the filtered list");
	Frame(BTN_DOWN, 0);
	CTR_TEST_CHECK_EQ(MM_menuChaosConfig.rowSelected, 0, "Down wraps from the last enabled effect");

	memset(sdata->chaosSettings.effects, 0, sizeof(sdata->chaosSettings.effects));
	sdata->chaosSettings.enabledCount = 0;
	MM_ChaosConfig_OpenReadOnly(&pause);
	MM_menuChaosConfig.rowSelected = 3;
	Frame(BTN_CROSS, 0);
	CTR_TEST_CHECK(s_drawnTitle == LANG_GetString(LNG_EXPANDED_ACTIVE_EFFECTS), "an empty selection still opens Active Effects");
	CTR_TEST_CHECK_EQ(s_drawnRows, 0, "an empty selection draws no effect rows");
	CTR_TEST_CHECK(!s_drewOnOff, "the empty page has no toggle column either");
	Frame(BTN_DOWN | BTN_CROSS, 0);
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.enabledCount, 0, "navigation and confirm on an empty list cannot enable an effect");
	Frame(BTN_TRIANGLE, 0);
	CTR_TEST_CHECK_EQ(MM_menuChaosConfig.rowSelected, 3, "Back works from the empty list");
}

static void CheckTestRaceGrid(void)
{
	u32 occupied = 0;
	const int count = (int)len(sdata->kartSpawnOrderArray);
	for (int i = 0; i < count; i++)
	{
		const u8 slot = (u8)sdata->kartSpawnOrderArray[i];
		CTR_TEST_CHECK(slot < count && (occupied & (1u << slot)) == 0,
		               "each test-race kart has a distinct valid starting slot");
		if (slot < count)
		{
			occupied |= 1u << slot;
		}
	}
	CTR_TEST_CHECK_EQ(sdata->kartSpawnOrderArray[0], count - 1,
	                  "the test-race human starts at the back of the Adventure grid");
}

static void TestChaosRaceShortcut(void)
{
	CTR_TEST_SECTION("Start tests the highlighted effect only during editable creation");
	EnterSetup();
	g_config.developerHacksEnabled = true;
	Frame(BTN_START, 0);
	CTR_TEST_CHECK_EQ(s_testLevel, -1, "Start on a settings row does not launch a race");
	MM_menuChaosConfig.rowSelected = sdata->chaosMenu.effectsRow;
	Frame(BTN_CROSS, 0);
	MM_menuChaosConfig.rowSelected = CHAOS_EFFECT_FLY_ME_TO_THE_MOON;
	Frame(BTN_START, 0);
	CTR_TEST_CHECK_EQ(s_testLevel, -1, "the shortcut is disabled by default");
	g_config.chaosQuickRace = true;
	s_tracker.podiumRewardID = STATIC_TROPHY;
	sdata->chaosSettings.effects[CHAOS_EFFECT_FLY_ME_TO_THE_MOON] = false;
	Frame(BTN_START, 0);
	CTR_TEST_CHECK_EQ(s_testLevel, CRASH_COVE, "Start launches Crash Cove even when the highlighted toggle is off");
	CTR_TEST_CHECK_EQ(s_tracker.gameMode1, ADVENTURE_MODE, "the test is an Adventure race");
	CheckTestRaceGrid();
	CTR_TEST_CHECK_EQ(s_tracker.podiumRewardID, NOFUNC, "a previous Adventure reward cannot select the podium overlay for the test race");
	CTR_TEST_CHECK_EQ(sdata->advProfileIndex, 0xffff, "the test is not attached to a save slot");
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.durationSeconds, 5, "test duration is five seconds");
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.triggerIntervalSeconds, 10, "test interval is ten seconds");
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.trigger, CHAOS_TRIGGER_INTERVAL, "the test uses the interval trigger");
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.enabledCount, 1, "only one effect is counted");
	for (int id = 0; id < CHAOS_EFFECT_COUNT; id++)
	{
		CTR_TEST_CHECK(sdata->chaosSettings.effects[id] == (id == CHAOS_EFFECT_FLY_ME_TO_THE_MOON), "only the highlighted effect is enabled");
	}
	CTR_TEST_CHECK(s_drewBeforeExit, "the menu draws before requesting the transition");

	// Return to creation after another completed Adventure event in this session.
	MM_ChaosConfig_OnEnter();
	sdata->chaosMenu.page = CHAOS_PAGE_EFFECTS;
	MM_menuChaosConfig.rowSelected = CHAOS_EFFECT_BIG_KART_ENERGY;
	s_tracker.podiumRewardID = STATIC_RELIC;
	memset(sdata->kartSpawnOrderArray, 0, sizeof(sdata->kartSpawnOrderArray));
	s_testLevel = -1;
	Frame(BTN_START, 0);
	CTR_TEST_CHECK_EQ(s_testLevel, CRASH_COVE, "a subsequent shortcut launch requests the test track again");
	CheckTestRaceGrid();
	CTR_TEST_CHECK_EQ(s_tracker.podiumRewardID, NOFUNC, "every shortcut launch clears pending podium rewards");
	CTR_TEST_CHECK_EQ(sdata->chaosSettings.enabledCount, 1, "a subsequent launch still enables exactly one effect");
	CTR_TEST_CHECK(sdata->chaosSettings.effects[CHAOS_EFFECT_BIG_KART_ENERGY] &&
	               !sdata->chaosSettings.effects[CHAOS_EFFECT_FLY_ME_TO_THE_MOON],
	               "a subsequent launch replaces the previous tested effect");

	struct RectMenu pause = {0};
	s_testLevel = -1;
	MM_ChaosConfig_OpenReadOnly(&pause);
	sdata->chaosMenu.page = CHAOS_PAGE_EFFECTS;
	MM_menuChaosConfig.rowSelected = 0;
	Frame(BTN_START, 0);
	CTR_TEST_CHECK_EQ(s_testLevel, -1, "Start in pause remains a return action");
	CTR_TEST_CHECK(sdata->ptrDesiredMenu == &pause, "pause Start returns to the pause menu");
}

int main(void)
{
	TestTriggerLayoutAndIntervalRetention();
	TestEffectsCommitAndPageTransitions();
	TestConfirmRequiresAnEffect();
	TestLiveValuesAndInputKinds();
	TestResidentRestore();
	TestPauseEntryVisibility();
	TestReadOnlyPauseView();
	TestFilteredActiveEffects();
	TestChaosRaceShortcut();
	return CtrTest_Summary();
}
