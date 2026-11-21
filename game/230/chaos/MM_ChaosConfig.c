#include <common.h>
#include <chaos/chaos_event.h>
#include "MM_ChaosTestRace.h"
#include "../menu-helpers/MM_FullScreenMenu.c"

_Static_assert(LNG_EXPANDED_CHAOS_EFFECT_KING_CRIMSON - LNG_EXPANDED_CHAOS_EFFECT_BASE + 1 == CHAOS_EFFECT_COUNT,
               "Chaos effect text IDs must match effect IDs");

// Shared settings: editable during creation/session setup, read-only from pause.
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

static int Chaos_MenuPlayerCount(void)
{
	const struct GameTracker *tracker = sdata->gGT;
	const bool readOnly = sdata->chaosMenu.returnMenu != NULL;
	int players = readOnly ? tracker->numPlyrCurrGame : tracker->numPlyrNextGame;
	if (readOnly && (tracker->gameMode1 & (ADVENTURE_MODE | ADVENTURE_ARENA)) == (ADVENTURE_MODE | ADVENTURE_ARENA))
	{
		// The hub has one kart even when the saved Adventure races have two players.
		players = sdata->advMultiplayer.numPlayers;
	}
	return players < CHAOS_MAX_HUMAN_PLAYERS ? players : CHAOS_MAX_HUMAN_PLAYERS;
}

static bool Chaos_HasDistributionChoice(void)
{
	return Chaos_MenuPlayerCount() > 1;
}

static bool Chaos_SettingsArePersonal(void)
{
	return Chaos_HasDistributionChoice() &&
	       sdata->chaosSettings.distribution == CHAOS_DISTRIBUTION_PER_PLAYER;
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
	[CHAOS_SETTING_DISTRIBUTION] = {
		.kind = FULLSCREEN_ROW_CHOICE, .numChoices = CHAOS_DISTRIBUTION_COUNT},
	[CHAOS_SETTING_PLAYERS] = {.kind = FULLSCREEN_ROW_ACTION},
	[CHAOS_SETTING_EFFECTS] = {.kind = FULLSCREEN_ROW_ACTION},
	[CHAOS_SETTING_CONFIRM] = {.kind = FULLSCREEN_ROW_ACTION},
};

static const int s_chaosSettingLabels[CHAOS_SETTING_COUNT] = {
	[CHAOS_SETTING_DURATION] = LNG_EXPANDED_DURATION,
	[CHAOS_SETTING_TRIGGER] = LNG_EXPANDED_TRIGGER,
	[CHAOS_SETTING_DISTRIBUTION] = LNG_EXPANDED_CHAOS_DISTRIBUTION,
	[CHAOS_SETTING_PLAYERS] = LNG_PLAYERS,
	[CHAOS_SETTING_INTERVAL] = LNG_EXPANDED_INTERVAL,
	[CHAOS_SETTING_EFFECTS] = LNG_EXPANDED_EFFECTS,
	[CHAOS_SETTING_CONFIRM] = LNG_EXPANDED_CONFIRM,
};

static const int s_chaosPageTitles[CHAOS_PAGE_COUNT] = {
	[CHAOS_PAGE_SETTINGS] = LNG_EXPANDED_CHAOS_SETUP_TITLE,
	[CHAOS_PAGE_EFFECTS] = LNG_EXPANDED_CHAOS_EFFECTS_TITLE,
	[CHAOS_PAGE_PLAYERS] = LNG_PLAYERS,
};

static const int s_chaosReadOnlyPageTitles[CHAOS_PAGE_COUNT] = {
	[CHAOS_PAGE_SETTINGS] = LNG_EXPANDED_CHAOS_RULES,
	[CHAOS_PAGE_EFFECTS] = LNG_EXPANDED_ACTIVE_EFFECTS,
	[CHAOS_PAGE_PLAYERS] = LNG_PLAYERS,
};

static void Chaos_BuildSettingRows(void)
{
	struct ChaosMenuState *state = &sdata->chaosMenu;
	const struct ChaosSettings *settings = &sdata->chaosSettings;
	state->numSettings = 0;

	for (int setting = 0; setting < CHAOS_SETTING_COUNT; setting++)
	{
		if ((setting == CHAOS_SETTING_DISTRIBUTION || setting == CHAOS_SETTING_PLAYERS) && !Chaos_HasDistributionChoice())
		{
			continue;
		}
		if (setting == CHAOS_SETTING_CONFIRM && state->returnMenu != NULL)
		{
			continue;
		}
		if (setting == CHAOS_SETTING_INTERVAL && !Chaos_TriggerIsTimed(settings->trigger))
		{
			continue;
		}
		if (setting == CHAOS_SETTING_EFFECTS)
		{
			state->effectsRow = state->numSettings;
		}
		if (setting == CHAOS_SETTING_PLAYERS)
		{
			state->playersRow = state->numSettings;
		}
		state->visibleSettings[state->numSettings++] = (enum ChaosSetting)setting;
	}
}

static int Chaos_SettingValue(enum ChaosSetting setting)
{
	const struct ChaosSettings *settings = &sdata->chaosSettings;
	switch (setting)
	{
	case CHAOS_SETTING_DISTRIBUTION:
		return settings->distribution;
	case CHAOS_SETTING_DURATION:
		return settings->durationSeconds;
	case CHAOS_SETTING_TRIGGER:
		return settings->trigger;
	case CHAOS_SETTING_INTERVAL:
		return settings->triggerIntervalSeconds;
	case CHAOS_SETTING_EFFECTS:
	case CHAOS_SETTING_PLAYERS:
	case CHAOS_SETTING_CONFIRM:
	case CHAOS_SETTING_COUNT:
	default:
		return 0;
	}
}

// Shared row operations work on ints; commit only the changed field with its stored type.
static void Chaos_EditSetting(enum ChaosSetting setting, int direction)
{
	struct FullScreenMenu_Row spec = s_chaosSettings[setting];
	// External is the final choice and has no personal provider yet.
	if (setting == CHAOS_SETTING_TRIGGER && Chaos_SettingsArePersonal())
	{
		spec.numChoices = CHAOS_TRIGGER_EXTERNAL;
	}
	int value = Chaos_SettingValue(setting);
	const bool changed = direction == 0
	                         ? FullScreenMenu_RowActivate(&spec, &value)
	                         : FullScreenMenu_RowStep(&spec, &value, direction);
	if (!changed)
	{
		return;
	}

	struct ChaosSettings *settings = &sdata->chaosSettings;
	switch (setting)
	{
	case CHAOS_SETTING_DISTRIBUTION:
		settings->distribution = (u8)value;
		if (Chaos_SettingsArePersonal() && settings->trigger == CHAOS_TRIGGER_EXTERNAL)
		{
			settings->trigger = CHAOS_TRIGGER_INTERVAL;
		}
		break;
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
	case CHAOS_SETTING_PLAYERS:
	case CHAOS_SETTING_CONFIRM:
	case CHAOS_SETTING_COUNT:
	default:
		break;
	}
}

static void Chaos_OnEnter(struct RectMenu *returnMenu, bool sessionSetup)
{
	if (returnMenu == NULL && sdata->gGT->numPlyrNextGame == 1)
	{
		sdata->chaosSettings.distribution = CHAOS_DISTRIBUTION_SHARED;
		sdata->chaosSettings.disabledPlayers[0] = false;
	}
	sdata->chaosMenu = (struct ChaosMenuState){
		.menu = {
			.stringIndexTitle = RECTMENU_STRING_NONE,
			.state = EXECUTE_FUNCPTR | DISABLE_INPUT_ALLOW_FUNCPTRS,
			.funcPtr = MM_MenuProc_ChaosConfig,
		},
		.page = CHAOS_PAGE_SETTINGS,
		.returnMenu = returnMenu,
		.sessionSetup = sessionSetup,
	};
	FullScreenMenu_SetShadedFrame(returnMenu == NULL);
	Chaos_BuildSettingRows();
}

static bool Chaos_EffectIsAvailable(ChaosEffectId effect)
{
	const struct GameTracker *tracker = sdata->gGT;
	const int players = Chaos_MenuPlayerCount();
	// An Adventure file spans every event type, not just the current hub/race.
	const int mode = (tracker->gameMode1 & ADVENTURE_MODE) != 0
	                     ? ADVENTURE_MODE : tracker->gameMode1 & ~MAIN_MENU;
	const bool personal = Chaos_SettingsArePersonal();
	return ChaosEvent_EffectFitsSelection(effect, mode, tracker->cup.cupID, players, g_config.extendedMultiplayer,
	                                     personal, sdata->chaosSettings.disabledPlayers);
}

static bool Chaos_CanConfirm(void)
{
	if (Chaos_HasDistributionChoice())
	{
		bool anyEnabled = false;
		for (int player = 0; player < Chaos_MenuPlayerCount(); player++)
		{
			anyEnabled |= !sdata->chaosSettings.disabledPlayers[player];
		}
		if (!anyEnabled)
		{
			return false;
		}
	}
	if (Chaos_SettingsArePersonal() && sdata->chaosSettings.trigger == CHAOS_TRIGGER_EXTERNAL)
	{
		return false;
	}
	for (int id = 0; id < CHAOS_EFFECT_COUNT; id++)
	{
		if (sdata->chaosSettings.effects[id] && Chaos_EffectIsAvailable((ChaosEffectId)id))
		{
			return true;
		}
	}
	return false;
}

void MM_ChaosConfig_OnEnter(void)
{
	Chaos_OnEnter(NULL, false);
}

void MM_ChaosConfig_OpenSession(void)
{
	Chaos_OnEnter(NULL, true);
	RECTMENU_ClearInput();
	sdata->ptrDesiredMenu = &sdata->chaosMenu.menu;
}

void MM_ChaosConfig_OpenReadOnly(struct RectMenu *returnMenu)
{
	Chaos_OnEnter(returnMenu, false);
	RECTMENU_ClearInput();
	sdata->ptrDesiredMenu = &sdata->chaosMenu.menu;
}

static bool Chaos_EffectRowIsVisible(ChaosEffectId effect)
{
	return Chaos_EffectIsAvailable(effect) &&
	       (sdata->chaosMenu.returnMenu == NULL || sdata->chaosSettings.effects[effect]);
}

static ChaosEffectId Chaos_EffectForRow(int row)
{
	for (int id = 0; id < CHAOS_EFFECT_COUNT; id++)
	{
		if (!Chaos_EffectRowIsVisible((ChaosEffectId)id))
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
	if (sdata->chaosMenu.page == CHAOS_PAGE_PLAYERS)
	{
		FullScreenMenu_DrawRowLabel(LANG_GetString(LNG_PLAYER_1 + row), y, ot);
		FullScreenMenu_DrawOnOff(!sdata->chaosSettings.disabledPlayers[row], y, ot);
	}
	else if (sdata->chaosMenu.page == CHAOS_PAGE_EFFECTS)
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
		else if (setting == CHAOS_SETTING_DISTRIBUTION)
		{
			choices[CHAOS_DISTRIBUTION_SHARED] = LANG_GetString(LNG_EXPANDED_CHAOS_SHARED);
			choices[CHAOS_DISTRIBUTION_PER_PLAYER] = LANG_GetString(LNG_EXPANDED_CHAOS_PER_PLAYER);
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
	if (state->page == CHAOS_PAGE_PLAYERS)
	{
		return Chaos_MenuPlayerCount();
	}
	if (state->page == CHAOS_PAGE_SETTINGS)
	{
		return state->numSettings;
	}
	int count = 0;
	for (int id = 0; id < CHAOS_EFFECT_COUNT; id++)
	{
		if (Chaos_EffectRowIsVisible((ChaosEffectId)id))
		{
			count++;
		}
	}
	return count;
}

static void MM_MenuProc_ChaosConfig(struct RectMenu *menu)
{
	struct GameTracker *gGT = sdata->gGT;
	struct ChaosSettings *settings = &sdata->chaosSettings;
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
	if (!readOnly && !state->sessionSetup && state->page == CHAOS_PAGE_EFFECTS && g_config.chaosQuickRace &&
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
	const bool playersAction = state->page == CHAOS_PAGE_SETTINGS && state->visibleSettings[selected] == CHAOS_SETTING_PLAYERS;
	const bool confirmDisabled = state->page == CHAOS_PAGE_SETTINGS &&
	                             state->visibleSettings[selected] == CHAOS_SETTING_CONFIRM && !Chaos_CanConfirm();
	if ((pad->buttonsTapped & (BTN_CROSS | BTN_CIRCLE)) != 0 && confirmDisabled)
	{
		OtherFX_Play(MENU_SFX_LOCKED, 1);
	}
	if ((pad->buttonsTapped & (BTN_CROSS | BTN_CIRCLE)) != 0 && (!readOnly || effectsAction || playersAction) && !confirmDisabled)
	{
		OtherFX_Play(1, 1);
		if (state->page == CHAOS_PAGE_PLAYERS)
		{
			settings->disabledPlayers[selected] = !settings->disabledPlayers[selected];
		}
		else if (state->page == CHAOS_PAGE_EFFECTS)
		{
			ChaosEffect_Toggle(settings->effects, &settings->enabledCount,
			                   Chaos_EffectForRow(selected));
		}
		else
		{
			switch (state->visibleSettings[selected])
			{
			case CHAOS_SETTING_PLAYERS:
				state->page = CHAOS_PAGE_PLAYERS;
				selected = 0;
				break;
			case CHAOS_SETTING_EFFECTS:
				state->page = CHAOS_PAGE_EFFECTS;
				selected = 0;
				break;
			case CHAOS_SETTING_CONFIRM:
				finish = true;
				break;
			case CHAOS_SETTING_DURATION:
			case CHAOS_SETTING_DISTRIBUTION:
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
		if (state->page != CHAOS_PAGE_SETTINGS)
		{
			selected = state->page == CHAOS_PAGE_PLAYERS ? state->playersRow : state->effectsRow;
			state->page = CHAOS_PAGE_SETTINGS;
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
		MM_ChaosTestRace_Start(testEffect);
	}
	else if (finish)
	{
		if (state->sessionSetup)
		{
			MM_ChaosSession_FinishSetup(true);
		}
		else
		{
			MM_NewLoad_FinishChaosSetup();
		}
	}
	else if (leave)
	{
		if (readOnly)
		{
			RECTMENU_ClearInput();
			sdata->ptrDesiredMenu = state->returnMenu;
		}
		else if (state->sessionSetup)
		{
			MM_ChaosSession_FinishSetup(false);
		}
		else
		{
			MM_NewLoad_ReturnFromChaosSetup();
		}
	}
}
