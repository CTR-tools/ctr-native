#include <common.h>
#include "MM_FullScreenMenu.c"

_Static_assert(LNG_EXPANDED_CHAOS_EFFECT_KING_CRIMSON - LNG_EXPANDED_CHAOS_EFFECT_BASE + 1 == CHAOS_EFFECT_COUNT,
               "Chaos effect text IDs must match effect IDs");

// Shared full-screen settings: editable during creation, read-only from pause.
// Page changes draw immediately; leaving the screen happens after drawing.
static void MM_MenuProc_ChaosConfig(struct RectMenu *menu);

enum
{
	CHAOS_DURATION_MIN_SECONDS = 5,
	CHAOS_DURATION_MAX_SECONDS = 60,
	CHAOS_DURATION_STEP_SECONDS = 5,
	CHAOS_INTERVAL_MIN_SECONDS = 5,
	CHAOS_INTERVAL_MAX_SECONDS = 120,
	CHAOS_INTERVAL_STEP_SECONDS = 5,
	CHAOS_TEST_DURATION_SECONDS = 5,
	CHAOS_TEST_INTERVAL_SECONDS = 10,
};

static const int s_chaosTriggerNames[CHAOS_TRIGGER_COUNT] = {
	[CHAOS_TRIGGER_RANDOM] = LNG_EXPANDED_RANDOM,
	[CHAOS_TRIGGER_PICKUP] = LNG_EXPANDED_PICKUP,
	[CHAOS_TRIGGER_INTERVAL] = LNG_EXPANDED_INTERVAL,
	[CHAOS_TRIGGER_EXTERNAL] = LNG_EXPANDED_EXTERNAL,
};

static bool Chaos_TriggerIsTimed(int trigger)
{
	return trigger == CHAOS_TRIGGER_RANDOM || trigger == CHAOS_TRIGGER_INTERVAL || trigger == CHAOS_TRIGGER_EXTERNAL;
}

static const struct FullScreenMenu_Row s_chaosSettings[CHAOS_SETTING_COUNT] = {
	[CHAOS_SETTING_DURATION] = {
		.kind = FULLSCREEN_ROW_STEP,
		.min = CHAOS_DURATION_MIN_SECONDS, .max = CHAOS_DURATION_MAX_SECONDS,
		.step = CHAOS_DURATION_STEP_SECONDS, .suffix = "s"},
	[CHAOS_SETTING_TRIGGER] = {
		.kind = FULLSCREEN_ROW_CHOICE, .numChoices = CHAOS_TRIGGER_COUNT},
	[CHAOS_SETTING_INTERVAL] = {
		.kind = FULLSCREEN_ROW_STEP,
		.min = CHAOS_INTERVAL_MIN_SECONDS, .max = CHAOS_INTERVAL_MAX_SECONDS,
		.step = CHAOS_INTERVAL_STEP_SECONDS, .suffix = "s"},
	[CHAOS_SETTING_EFFECTS] = {.kind = FULLSCREEN_ROW_ACTION},
	[CHAOS_SETTING_CONFIRM] = {.kind = FULLSCREEN_ROW_ACTION},
};

static const int s_chaosSettingLabels[CHAOS_SETTING_COUNT] = {
	[CHAOS_SETTING_DURATION] = LNG_EXPANDED_DURATION,
	[CHAOS_SETTING_TRIGGER] = LNG_EXPANDED_TRIGGER,
	[CHAOS_SETTING_INTERVAL] = LNG_EXPANDED_INTERVAL,
	[CHAOS_SETTING_EFFECTS] = LNG_EXPANDED_EFFECTS,
	[CHAOS_SETTING_CONFIRM] = LNG_EXPANDED_CONFIRM,
};

static const int s_chaosPageTitles[CHAOS_PAGE_COUNT] = {
	[CHAOS_PAGE_SETTINGS] = LNG_EXPANDED_CHAOS_SETUP_TITLE,
	[CHAOS_PAGE_EFFECTS] = LNG_EXPANDED_CHAOS_EFFECTS_TITLE,
};

static const int s_chaosReadOnlyPageTitles[CHAOS_PAGE_COUNT] = {
	[CHAOS_PAGE_SETTINGS] = LNG_EXPANDED_CHAOS_RULES,
	[CHAOS_PAGE_EFFECTS] = LNG_EXPANDED_ACTIVE_EFFECTS,
};

static void Chaos_BuildSettingRows(void)
{
	struct ChaosMenuState *state = &sdata->chaosMenu;
	state->numSettings = 0;

	for (int setting = 0; setting < CHAOS_SETTING_COUNT; setting++)
	{
		if (setting == CHAOS_SETTING_CONFIRM && state->returnMenu != NULL)
		{
			continue;
		}
		if (setting == CHAOS_SETTING_INTERVAL && !Chaos_TriggerIsTimed(sdata->chaosSettings.trigger))
		{
			continue;
		}
		if (setting == CHAOS_SETTING_EFFECTS)
		{
			state->effectsRow = state->numSettings;
		}
		state->visibleSettings[state->numSettings++] = (enum ChaosSetting)setting;
	}
}

static int Chaos_SettingValue(enum ChaosSetting setting)
{
	const struct ChaosSettings *settings = &sdata->chaosSettings;
	switch (setting)
	{
	case CHAOS_SETTING_DURATION:
		return settings->durationSeconds;
	case CHAOS_SETTING_TRIGGER:
		return settings->trigger;
	case CHAOS_SETTING_INTERVAL:
		return settings->triggerIntervalSeconds;
	case CHAOS_SETTING_EFFECTS:
	case CHAOS_SETTING_CONFIRM:
	case CHAOS_SETTING_COUNT:
	default:
		return 0;
	}
}

// Shared row operations work on ints; commit only the changed field with its stored type.
static void Chaos_EditSetting(enum ChaosSetting setting, int direction)
{
	const struct FullScreenMenu_Row *row = &s_chaosSettings[setting];
	int value = Chaos_SettingValue(setting);
	const bool changed = direction == 0
	                         ? FullScreenMenu_RowActivate(row, &value)
	                         : FullScreenMenu_RowStep(row, &value, direction);
	if (!changed)
	{
		return;
	}

	struct ChaosSettings *settings = &sdata->chaosSettings;
	switch (setting)
	{
	case CHAOS_SETTING_DURATION:
		settings->durationSeconds = (u16)value;
		break;
	case CHAOS_SETTING_TRIGGER:
	{
		const bool wasTimed = Chaos_TriggerIsTimed(settings->trigger);
		settings->trigger = (u8)value;
		if (wasTimed != Chaos_TriggerIsTimed(settings->trigger))
		{
			Chaos_BuildSettingRows();
		}
		break;
	}
	case CHAOS_SETTING_INTERVAL:
		settings->triggerIntervalSeconds = (u16)value;
		break;
	case CHAOS_SETTING_EFFECTS:
	case CHAOS_SETTING_CONFIRM:
	case CHAOS_SETTING_COUNT:
	default:
		break;
	}
}

static void Chaos_OnEnter(struct RectMenu *returnMenu)
{
	sdata->chaosMenu = (struct ChaosMenuState){
		.menu = {
			.stringIndexTitle = RECTMENU_STRING_NONE,
			.state = EXECUTE_FUNCPTR | DISABLE_INPUT_ALLOW_FUNCPTRS,
			.funcPtr = MM_MenuProc_ChaosConfig,
		},
		.page = CHAOS_PAGE_SETTINGS,
		.returnMenu = returnMenu,
	};
	FullScreenMenu_SetShadedFrame(returnMenu == NULL);
	Chaos_BuildSettingRows();
}

static bool Chaos_CanConfirm(void)
{
	return sdata->chaosSettings.enabledCount > 0;
}

void MM_ChaosConfig_OnEnter(void)
{
	Chaos_OnEnter(NULL);
}

void MM_ChaosConfig_OpenReadOnly(struct RectMenu *returnMenu)
{
	Chaos_OnEnter(returnMenu);
	RECTMENU_ClearInput();
	sdata->ptrDesiredMenu = &sdata->chaosMenu.menu;
}

static ChaosEffectId Chaos_EffectForRow(int row)
{
	if (sdata->chaosMenu.returnMenu == NULL)
	{
		return ChaosEffect_FromIndex(row);
	}
	for (int id = 0; id < CHAOS_EFFECT_COUNT; id++)
	{
		if (!sdata->chaosSettings.effects[id])
		{
			continue;
		}
		if (row == 0)
		{
			return (ChaosEffectId)id;
		}
		row--;
	}
	return CHAOS_EFFECT_NONE;
}

static void Chaos_DrawRow(const void *context, int row, int y, bool selected, uint32_t *ot)
{
	(void)context;
	(void)selected;
	if (sdata->chaosMenu.page == CHAOS_PAGE_EFFECTS)
	{
		const ChaosEffectId effect = Chaos_EffectForRow(row);
		FullScreenMenu_DrawRowLabel(LANG_GetString(LNG_EXPANDED_CHAOS_EFFECT_BASE + effect), y, ot);
		if (sdata->chaosMenu.returnMenu == NULL)
		{
			FullScreenMenu_DrawOnOff(ChaosEffect_IsEnabled(sdata->chaosSettings.effects, effect), y, ot);
		}
	}
	else
	{
		const enum ChaosSetting setting = sdata->chaosMenu.visibleSettings[row];
		struct FullScreenMenu_Row spec = s_chaosSettings[setting];
		const char *choices[CHAOS_TRIGGER_COUNT];
		if (setting == CHAOS_SETTING_TRIGGER)
		{
			for (int i = 0; i < CHAOS_TRIGGER_COUNT; i++)
			{
				choices[i] = LANG_GetString(s_chaosTriggerNames[i]);
			}
			spec.choices = choices;
		}
		const char *label = LANG_GetString(setting == CHAOS_SETTING_EFFECTS && sdata->chaosMenu.returnMenu != NULL
		                                       ? LNG_EXPANDED_ACTIVE_EFFECTS : s_chaosSettingLabels[setting]);
		if (setting == CHAOS_SETTING_CONFIRM && !Chaos_CanConfirm())
		{
			DecalFont_DrawLineOT((char *)label, FULLSCREEN_MENU_LABEL_X, y, FONT_SMALL, GRAY, ot);
		}
		else
		{
			FullScreenMenu_DrawRowLabel(label, y, ot);
		}
		FullScreenMenu_DrawRowValue(&spec, Chaos_SettingValue(setting), y, ot);
	}
}

static int Chaos_PageRowCount(const struct ChaosMenuState *state)
{
	if (state->page == CHAOS_PAGE_SETTINGS)
	{
		return state->numSettings;
	}
	return state->returnMenu != NULL ? sdata->chaosSettings.enabledCount : CHAOS_EFFECT_COUNT;
}

// Developer shortcut: Start on an effect launches a disposable Adventure race.
static void Chaos_TestRace(ChaosEffectId effect)
{
	struct GameTracker *gGT = sdata->gGT;
	sdata->chaosSettings = (struct ChaosSettings){
		.durationSeconds = CHAOS_TEST_DURATION_SECONDS,
		.triggerIntervalSeconds = CHAOS_TEST_INTERVAL_SECONDS,
		.trigger = CHAOS_TRIGGER_INTERVAL,
		.armed = true,
		.enabledCount = 1,
	};
	sdata->chaosSettings.effects[effect] = true;
	MM_NewLoad_FinishChaosSetup();
	// Unwind creation normally, then load directly instead of visiting the garage.
	MM_TITLE_MENU_STATE = TITLE_MENU_STATE_IN_MENU;
	sdata->newLoad.previousMenu = NULL;
	MM_Title_CameraReset();
	MM_Title_KillThread();
	MM_QuickRace_SetupRace();
	gGT->gameMode1 = ADVENTURE_MODE;
	// A previous Adventure reward would select the podium overlay during loading.
	gGT->podiumRewardID = NOFUNC;
	GAMEPROG_NewProfile_InsideAdv(&GAME_ADV_PROGRESS);
	GAME_ADV_PROGRESS.characterID = CRASH_BANDICOOT;
	GAMEPROG_ResetAdvMultiplayer(&sdata->advMultiplayer);
	// This shortcut skips the warp pad that normally seeds the Adventure grid.
	for (int i = 0; i < (int)len(sdata->kartSpawnOrderArray); i++)
	{
		sdata->kartSpawnOrderArray[i] = (char)(len(sdata->kartSpawnOrderArray) - 1 - i);
	}
	sdata->advProfileIndex = 0xffff;
	gGT->currLEV = N_SANITY_BEACH;
	RECTMENU_ClearInput();
	sdata->ptrDesiredMenu = NULL;
	RECTMENU_Hide(&sdata->chaosMenu.menu);
	MainRaceTrack_RequestLoad(CRASH_COVE);
}

static void MM_MenuProc_ChaosConfig(struct RectMenu *menu)
{
	struct GameTracker *gGT = sdata->gGT;
	uint32_t *ot = gGT->backBuffer->otMem.uiOT;
	struct GamepadBuffer *pad = &sdata->gGamepads->gamepad[0];
	struct ChaosMenuState *state = &sdata->chaosMenu;
	const bool readOnly = state->returnMenu != NULL;
	struct GamepadBuffer pauseInput;
	if (readOnly)
	{
		pauseInput = (struct GamepadBuffer){
			.buttonsTapped = sdata->AnyPlayerTap,
			.buttonsHeldCurrFrame = sdata->AnyPlayerHold,
		};
		pad = &pauseInput;
	}
	s16 selected = menu->rowSelected;
	bool leave = false;
	bool finish = false;
	ChaosEffectId testEffect = CHAOS_EFFECT_NONE;

	FullScreenMenu_NavScroll(pad, &state->scroll[state->page], &selected, Chaos_PageRowCount(state));
	if (!readOnly && state->page == CHAOS_PAGE_EFFECTS && g_config.chaosQuickRace &&
	    (pad->buttonsTapped & BTN_START) != 0)
	{
		testEffect = Chaos_EffectForRow(selected);
	}
	if (!readOnly && state->page == CHAOS_PAGE_SETTINGS)
	{
		for (int direction = -1; direction <= 1; direction += 2)
		{
			if (FullScreenMenu_OptionStepDue(pad->buttonsHeldCurrFrame, direction))
			{
				Chaos_EditSetting(state->visibleSettings[selected], direction);
			}
		}
	}

	const bool effectsAction = state->page == CHAOS_PAGE_SETTINGS && state->visibleSettings[selected] == CHAOS_SETTING_EFFECTS;
	const bool confirmDisabled = state->page == CHAOS_PAGE_SETTINGS &&
	                             state->visibleSettings[selected] == CHAOS_SETTING_CONFIRM && !Chaos_CanConfirm();
	if ((pad->buttonsTapped & (BTN_CROSS | BTN_CIRCLE)) != 0 && confirmDisabled)
	{
		OtherFX_Play(MENU_SFX_LOCKED, 1);
	}
	if ((pad->buttonsTapped & (BTN_CROSS | BTN_CIRCLE)) != 0 && (!readOnly || effectsAction) && !confirmDisabled)
	{
		OtherFX_Play(1, 1);
		if (state->page == CHAOS_PAGE_EFFECTS)
		{
			ChaosEffect_Toggle(sdata->chaosSettings.effects, &sdata->chaosSettings.enabledCount,
			                   ChaosEffect_FromIndex(selected));
		}
		else
		{
			switch (state->visibleSettings[selected])
			{
			case CHAOS_SETTING_EFFECTS:
				state->page = CHAOS_PAGE_EFFECTS;
				selected = 0;
				break;
			case CHAOS_SETTING_CONFIRM:
				finish = true;
				break;
			case CHAOS_SETTING_DURATION:
			case CHAOS_SETTING_TRIGGER:
			case CHAOS_SETTING_INTERVAL:
				Chaos_EditSetting(state->visibleSettings[selected], 0);
				break;
			case CHAOS_SETTING_COUNT:
				break;
			}
		}
	}

	if ((pad->buttonsTapped & BTN_TRIANGLE) != 0)
	{
		OtherFX_Play(2, 1);
		if (state->page == CHAOS_PAGE_EFFECTS)
		{
			state->page = CHAOS_PAGE_SETTINGS;
			selected = state->effectsRow;
		}
		else
		{
			leave = true;
		}
	}
	if (readOnly && (pad->buttonsTapped & (BTN_START | BTN_SQUARE_one)) != 0)
	{
		OtherFX_Play(2, 1);
		leave = true;
	}

	struct FullScreenMenu_Scroll *scroll = &state->scroll[state->page];
	const int numRows = Chaos_PageRowCount(state);
	FullScreenMenu_FollowScroll(scroll, &selected, numRows);
	menu->rowSelected = selected;
	const char *title = LANG_GetString(readOnly ? s_chaosReadOnlyPageTitles[state->page] : s_chaosPageTitles[state->page]);
	FullScreenMenu_DrawScreen(title, 0, scroll, numRows, selected,
	                          Chaos_DrawRow, NULL, gGT, ot);

	if (testEffect != CHAOS_EFFECT_NONE)
	{
		Chaos_TestRace(testEffect);
	}
	else if (finish)
	{
		MM_NewLoad_FinishChaosSetup();
	}
	else if (leave)
	{
		if (readOnly)
		{
			RECTMENU_ClearInput();
			sdata->ptrDesiredMenu = state->returnMenu;
		}
		else
		{
			MM_NewLoad_ReturnFromChaosSetup();
		}
	}
}
