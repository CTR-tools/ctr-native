#include <common.h>

// ---------------------------------------------------------------------------
// Development aid hack. Enabled by the "Quick Race" option in the
// Developer Hacks section;
//
// Reaching a race from a cold boot means the intro, then five menus: mode, race
// type, player count, difficulty, character, track. While enabled, R3 in the main
// menu skips all of it and drops straight into a 1P arcade race in Crash
// Cove as Crash, on the easiest bot difficulty and the default lap count.
// ---------------------------------------------------------------------------

// Leaves the main menu through the shared title transition, so the fade-out, camera
// reset and title thread teardown match every other route out of the menu. MM_Title
// loads the level once the transition has finished.
void MM_QuickRace_RequestStart(struct RectMenu *mainMenu)
{
	if (!g_config.quickRace)
	{
		return;
	}

	// funcPtr also runs with RECTMENU_FUNC_STATE_INPUT on the frame a row is
	// confirmed, which is the player already committed to a menu flow.
	if (mainMenu->funcState != RECTMENU_FUNC_STATE_UPDATE)
	{
		return;
	}

	// Set for as long as a submenu of the main menu is open.
	if ((mainMenu->state & DRAW_NEXT_MENU_IN_HIERARCHY) != 0)
	{
		return;
	}

	// IN_MENU is the only state where the main menu is fully faded in and idle. It
	// also excludes the intro and a race this hack already sent on its way.
	if (MM_TITLE_MENU_STATE != TITLE_MENU_STATE_IN_MENU)
	{
		return;
	}

	if ((GAMEPADS->anyoneTapped & BTN_R3) == 0)
	{
		return;
	}

	MM_TITLE_MENU_STATE = TITLE_MENU_STATE_EXITING;
	MM_DESIRED_MENU_INDEX = MM_EXIT_ROUTE_QUICK_RACE;
}

// Builds the race state the menu flow would have produced. Only driver slot 0 is
// set: a 1P race fills the rest of the grid with bots during the load.
void MM_QuickRace_SetupRace(void)
{
	struct GameTracker *gGT = GAME_TRACKER;

	// The main menu clears these before every row selection, and this route skips it.
	gGT->gameMode1 &= ~(BATTLE_MODE | ADVENTURE_MODE | TIME_TRIAL | ADVENTURE_ARENA | ARCADE_MODE | ADVENTURE_CUP);
	gGT->gameMode2 &= ~CUP_ANY_KIND;
	gGT->gameMode1 |= ARCADE_MODE;

	// Single race, three laps, easiest bots, Crash.
	gGT->numPlyrNextGame = 1;
	gGT->numLaps = MM_DEFAULT_LAP_COUNT;
	gGT->arcadeDifficulty = MM_CUP_DIFFICULTY_SPEED[0]; // o is easy
	GAME_CHARACTER_IDS[0] = CRASH_BANDICOOT;

	// A fresh event timer, as QueueLoadTrack gives every non-battle race that
	// reaches it through the menus.
	gGT->originalEventTime = TITLE_INITIAL_EVENT_TIME;

	// An idle main menu launches demo races by itself, and the flag outlives them.
	// Left set, it would take the camera and the input away in the requested race.
	gGT->boolDemoMode = 0;
}
