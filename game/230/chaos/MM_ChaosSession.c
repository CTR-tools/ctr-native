#include <common.h>

// A NULL destination resumes the ordinary exit to character selection.
static void MM_ChaosSession_Continue(struct RectMenu *parent, struct RectMenu *next)
{
	if (next != NULL)
	{
		MM_Menu_Attach(parent, next);
		return;
	}
	parent->state |= ONLY_DRAW_TITLE;
	MM_TITLE_MENU_STATE = TITLE_MENU_STATE_EXITING;
	MM_DESIRED_MENU_INDEX = MM_EXIT_ROUTE_CHARACTER_SELECT;
}

// Event type and player count are known before offering the shared prompt.
static void MM_ChaosSession_Begin(struct RectMenu *parent, struct RectMenu *next)
{
	if (!g_config.chaosMode)
	{
		// Hiding setup must not reuse armed rules from an earlier session race.
		sdata->chaosSettings.armed = false;
		sdata->chaosSessionNextMenu = NULL;
		MM_ChaosSession_Continue(parent, next);
		return;
	}
	sdata->chaosSessionNextMenu = next;
	MM_MENU_CHAOS_PROMPT.rowSelected = sdata->chaosSettings.armed;
	MM_Menu_Attach(parent, &MM_MENU_CHAOS_PROMPT);
}

void MM_ChaosSession_FinishSetup(bool confirm)
{
	if (confirm)
	{
		// The rules are not another recap box: keep the retail chain's height.
		struct RectMenu *parent = MM_MENU_CHAOS_PROMPT.ptrPrevBox_InHierarchy;
		MM_Menu_Detach(&MM_MENU_CHAOS_PROMPT);
		MM_ChaosSession_Continue(parent, sdata->chaosSessionNextMenu);
		sdata->chaosSessionNextMenu = NULL;
	}
	RECTMENU_ClearInput();
	sdata->ptrDesiredMenu = &MM_MENU_MAIN;
}
