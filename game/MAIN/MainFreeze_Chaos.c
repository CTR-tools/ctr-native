#include <common.h>

// Build from the retail rows each time so entering a Regular file cannot retain
// the extra action. Keep the original arrays intact, including the hub hint.
static struct RectMenu *MainFreeze_ChaosPauseMenu(struct RectMenu *menu, struct MenuRow *rows, int numRows)
{
	menu->rows = rows;
	const u32 mode = sdata->gGT->gameMode1;
	if (!sdata->chaosSettings.armed || (mode & (ADVENTURE_MODE | ADVENTURE_ARENA)) == 0 ||
	    (mode & (MAIN_MENU | GAME_CUTSCENE)) != 0)
	{
		return menu;
	}

	struct MenuRow *extended = sdata->chaosPauseRows;
	memcpy(extended, rows, (size_t)numRows * sizeof(*rows));
	extended[0].rowOnPressUp = (char)numRows;
	extended[numRows - 1].rowOnPressDown = (char)numRows;
	extended[numRows] = (struct MenuRow){LNG_EXPANDED_CHAOS_RULES, (char)(numRows - 1), 0, (char)numRows, (char)numRows};
	extended[numRows + 1] = (struct MenuRow){RECTMENU_STRING_NONE, 0, 0, 0, 0};
	menu->rows = extended;
	return menu;
}

CTR_STATIC_ASSERT(len(((struct Data *)0)->rowsAdvHub) + 1 <= CHAOS_PAUSE_ROW_CAPACITY);
CTR_STATIC_ASSERT(len(((struct Data *)0)->rowsAdvRace) + 1 <= CHAOS_PAUSE_ROW_CAPACITY);
CTR_STATIC_ASSERT(len(((struct Data *)0)->rowsAdvCup) + 1 <= CHAOS_PAUSE_ROW_CAPACITY);
