#include <common.h>
#include <platform/config/native_profile.h>
#include <platform/text/native_language_disc.h>

#include "../menu-helpers/MM_FullScreenMenu.c"

enum ProfileMenuPage { PROFILE_MENU_CLOSED, PROFILE_MENU_CHOOSE, PROFILE_MENU_ACTION, PROFILE_MENU_ERROR };
static enum ProfileMenuPage s_profilePage;
static enum ProfileMenuPage s_profileAfterError;
static bool s_profileStartup;
static bool s_profileStartupSeen;
static ConfigProfile s_profileChoice;
static int s_profileError;
static s16 s_profileRow;
static int s_profileActiveRow;
static struct FullScreenMenu_Scroll s_profileScroll;
static struct RectMenu *s_profileReturnMenu;
static s16 s_profileReturnRow;
static void (*s_profileOnApplied)(void);

static void MM_ProfileMenu_Update(struct RectMenu *menu);

static struct RectMenu s_profileMenu = {
	.stringIndexTitle = -1,
	.state = EXECUTE_FUNCPTR | DISABLE_INPUT_ALLOW_FUNCPTRS,
	.funcPtr = MM_ProfileMenu_Update,
};

static void ProfileMenu_SetPage(enum ProfileMenuPage page)
{
	s_profilePage = page;
	s_profileRow = 0;
	s_profileScroll.offset = 0;
}

static void ProfileMenu_Open(struct RectMenu *returnMenu, bool startup, enum ProfileMenuPage page, void (*onApplied)(void))
{
	s_profileReturnMenu = returnMenu;
	s_profileReturnRow = returnMenu->rowSelected;
	s_profileStartup = startup;
	s_profileOnApplied = onApplied;
	s_profileActiveRow = -1;
	ConfigProfile activeProfile;
	if (!startup && NativeConfig_LoadProfile(&activeProfile) == CONFIG_LOAD_OK)
	{
		s_profileActiveRow = activeProfile == CONFIG_PROFILE_VANILLA ? 0 : 1;
	}
	FullScreenMenu_SetShadedFrame(true);
	ProfileMenu_SetPage(page);
	if (s_profileActiveRow >= 0) { s_profileRow = (s16)s_profileActiveRow; }
	sdata->ptrDesiredMenu = &s_profileMenu;
}

static void MM_ProfileMenu_Open(struct RectMenu *returnMenu, void (*onApplied)(void))
{
	ProfileMenu_Open(returnMenu, false, PROFILE_MENU_CHOOSE, onApplied);
}

static bool MM_ProfileMenu_OpenStartup(struct RectMenu *returnMenu)
{
	if (s_profileStartupSeen) { return false; }
	s_profileStartupSeen = true;
	NativeProfileStartupStatus status = NativeProfile_GetStartupStatus();
	if (status == PROFILE_STARTUP_READY) { return false; }
	ProfileMenu_Open(returnMenu, true, status == PROFILE_STARTUP_NO_PROFILE ? PROFILE_MENU_CHOOSE : PROFILE_MENU_ERROR, NULL);
	RECTMENU_ClearInputAndTaps();
	s_profileError = status == PROFILE_STARTUP_INVALID_PROFILE ? LNG_EXPANDED_PROFILE_INVALID : LNG_EXPANDED_PROFILE_PRESET_ERROR;
	s_profileAfterError = status == PROFILE_STARTUP_INVALID_PROFILE ? PROFILE_MENU_CHOOSE : PROFILE_MENU_CLOSED;
	return true;
}

static void ProfileMenu_Close(void)
{
	s_profilePage = PROFILE_MENU_CLOSED;
	RECTMENU_ClearInputAndTaps();
	s_profileReturnMenu->rowSelected = s_profileReturnRow;
	sdata->ptrDesiredMenu = s_profileReturnMenu;
}

static void ProfileMenu_Select(ConfigProfile profile, bool apply, bool closeOnError)
{
	int previousTextPriority = g_config.retailTextPriority;
	ConfigLoadResult result = NativeProfile_Select(profile, apply);
	if (result != CONFIG_LOAD_OK)
	{
		s_profileError = result == CONFIG_LOAD_FILE_ERROR ? LNG_EXPANDED_PROFILE_SAVE_ERROR : LNG_EXPANDED_PROFILE_PRESET_ERROR;
		s_profileAfterError = closeOnError ? PROFILE_MENU_CLOSED : PROFILE_MENU_CHOOSE;
		s_profilePage = PROFILE_MENU_ERROR;
		RECTMENU_ClearInputAndTaps();
		return;
	}
	if (apply)
	{
		if (g_config.retailTextPriority != RETAIL_TEXT_PRIORITY_EXPANDED && !NativeLanguageDisc_HasAnyText())
		{
			g_config.retailTextPriority = RETAIL_TEXT_PRIORITY_EXPANDED;
		}
		if (g_config.retailTextPriority != previousTextPriority)
		{
			LANG_Refresh(g_config.textLanguage);
		}
		NativeConfig_Save();
		if (s_profileOnApplied != NULL) { s_profileOnApplied(); }
	}
	ProfileMenu_Close();
}

static void MM_ProfileMenu_Update(struct RectMenu *menu)
{
	(void)menu;
	struct GameTracker *tracker = sdata->gGT;
	u32 *ot = tracker->backBuffer->otMem.uiOT;
	u32 tapped = (u32)sdata->gGamepads->gamepad[0].buttonsTapped;
	const int columnX = s_profileStartup ? FULLSCREEN_MENU_TITLE_X : FULLSCREEN_MENU_MAIN_X;
	if (s_profilePage == PROFILE_MENU_ERROR)
	{
		if (FullScreenMenu_Confirm(tapped) || FullScreenMenu_Back(tapped))
		{
			if (s_profileAfterError == PROFILE_MENU_CLOSED) { ProfileMenu_Close(); }
			else
			{
				ProfileMenu_SetPage(s_profileAfterError);
				RECTMENU_ClearInputAndTaps();
			}
			return;
		}
		const char *labels[] = {LANG_GetString(s_profileError), LANG_GetString(LNG_EXPANDED_PROFILE_CONTINUE)};
		FullScreenMenu_DrawSectionScreen(LANG_GetString(LNG_EXPANDED_PROFILE_ERROR), labels, &s_profileScroll, 2, 1,
		                                 columnX, tracker, ot);
		return;
	}
	if (FullScreenMenu_Back(tapped))
	{
		if (s_profilePage == PROFILE_MENU_ACTION)
		{
			ProfileMenu_SetPage(PROFILE_MENU_CHOOSE);
			RECTMENU_ClearInputAndTaps();
		}
		else if (s_profileStartup)
		{
			ProfileMenu_Select(CONFIG_PROFILE_EXPANDED, NativeProfile_GetStartupConfigResult() == CONFIG_LOAD_NOT_FOUND, true);
		}
		else { ProfileMenu_Close(); }
		return;
	}
	const bool choosing = s_profilePage == PROFILE_MENU_CHOOSE;
	const int count = choosing ? 2 : 3;
	FullScreenMenu_NavTapped(tapped, &s_profileScroll, &s_profileRow, count);
	if (FullScreenMenu_Confirm(tapped))
	{
		if (choosing)
		{
			s_profileChoice = s_profileRow == 0 ? CONFIG_PROFILE_VANILLA : CONFIG_PROFILE_EXPANDED;
			if (s_profileStartup && NativeProfile_GetStartupConfigResult() == CONFIG_LOAD_NOT_FOUND)
			{
				ProfileMenu_Select(s_profileChoice, true, false);
			}
			else
			{
				ProfileMenu_SetPage(PROFILE_MENU_ACTION);
				RECTMENU_ClearInputAndTaps();
			}
		}
		else if (s_profileRow == 2)
		{
			if (s_profileStartup) { ProfileMenu_Select(CONFIG_PROFILE_EXPANDED, false, true); }
			else { ProfileMenu_Close(); }
		}
		else { ProfileMenu_Select(s_profileChoice, s_profileRow == 1, false); }
		return;
	}
	const char *labels[3];
	labels[0] = LANG_GetString(choosing ? LNG_EXPANDED_CONFIG_VANILLA : LNG_EXPANDED_PROFILE_FUTURE);
	labels[1] = LANG_GetString(choosing ? LNG_EXPANDED_PROFILE_EXPANDED : LNG_EXPANDED_PROFILE_APPLY);
	labels[2] = LANG_GetString(LNG_CANCEL);
	FullScreenMenu_DrawSectionScreenMarked(LANG_GetString(choosing ? LNG_EXPANDED_PROFILE_TITLE : LNG_EXPANDED_PROFILE_ACTION),
	                                       labels, &s_profileScroll, count, s_profileRow,
	                                       choosing ? s_profileActiveRow : -1, columnX, tracker, ot);
}
