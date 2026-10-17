#include <common.h>

void MainGameStart_Initialize(struct GameTracker *gGT, b32 boolStopAudio)
{
	u32 gameModeFlag;

	// DotLights_AudioAndVideo wont execute
	// if (gGT & 0x20102000 == 0), but if it did execute,
	// traffic lights would instantly hit green in cutscene
	// and main menu, while starting from top-position (0xf00)
	// when spawning in the adventure arena

	// if you're not in cutscene and not in main menu
	if ((sdata->gGT->gameMode1 & (GAME_CUTSCENE | MAIN_MENU)) == 0)
	{
		// traffic light countdown
		gGT->trafficLightsTimer = 0xf00;

		// fly-in camera animation
		gameModeFlag = gGT->gameMode1 | START_OF_RACE;
	}
	// if you are:
	else
	{
		// disable traffic lights
		gGT->trafficLightsTimer = 0;

		// no fly-in camera
		gameModeFlag = gGT->gameMode1 & ~(START_OF_RACE);
	}

	// save new game mode
	gGT->gameMode1 = gameModeFlag;
	gGT->gameMode1 &= ~(END_OF_RACE); // Remove end-of-race flag

	// this never happens in normal gameplay
	if (boolStopAudio == 0)
	{
		Music_Stop();

		// stops menu sounds
		// keep backup,
		// keep music (no music to stop),
		// stop all fx
		howl_StopAudio(0, 0, 1);
	}

	VehBirth_TeleportAll(gGT, VEH_BIRTH_SPAWN_INIT_RACE_STATE);
}

// Development aid hack, enabled by the "Skip Start Countdown" option. Rewinds the
// countdown and fly-in that MainGameStart_Initialize just armed to where they
// would be had they run to completion, so the grid goes the instant the level
// appears.
void MainGameStart_SkipCountdown(struct GameTracker *gGT)
{
	// START_OF_RACE is what MainGameStart_Initialize sets to arm the countdown and
	// the fly-in.
	if ((gGT->gameMode1 & START_OF_RACE) == 0)
	{
		return;
	}

	gGT->trafficLightsTimer = 0;

	// The running countdown is also the only thing that zeroes the race clock:
	// MainFrame clears elapsedEventTime every frame while the lights are up, and
	// nothing resets it at load. Skipping the countdown therefore has to zero it
	// here, or the next race starts on the previous race's time.
	gGT->elapsedEventTime = 0;

	// Ending transitionFrame puts the start-line fly-in on the finished path it
	// reaches naturally, which clears START_OF_RACE and turns on the race HUD on the
	// next camera frame.
	for (int i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		gGT->cameraDC[i].transitionFrame = 0;
	}
}
