#include <common.h>

// Share the prompt between Adventure creation and the session-only menu flow.

void MM_MenuProc_Chaos(struct RectMenu *menu)
{
	if (menu->funcState != RECTMENU_FUNC_STATE_INPUT)
	{
		return;
	}

	const s16 row = menu->rowSelected;

	// Triangle: back to the screen before this one.
	if (row == -1)
	{
		if (!MM_NewLoad_InChain())
		{
			MM_Menu_Detach(menu);
			sdata->chaosSessionNextMenu = NULL;
			return;
		}
		MM_NewLoad_Retreat(menu);
		return;
	}

	if ((row < 0) || (row > 1))
	{
		return;
	}

	// Row 0 is "Regular", row 1 is "Chaos".
	struct ChaosSettings *settings = &sdata->chaosSettings;
	settings->armed = (row == 1);
	if (!MM_NewLoad_InChain())
	{
		if (settings->armed)
		{
			MM_ChaosConfig_OpenSession();
		}
		else
		{
			MM_ChaosSession_FinishSetup(true);
		}
		return;
	}

	MM_NewLoad_Advance(menu);
}
