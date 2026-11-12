#include <shared/ctr_test.h>
#include <common.h>
#include <string.h>

struct sData sdata_static;
struct rData rdata;
struct GameTracker tracker;
static int savedCalls;
static int savedLap;
static int savedDriver;

void UI_SaveLapTime(int lap, int time, s16 driverID)
{
	(void)time;
	savedCalls++;
	savedLap = lap;
	savedDriver = driverID;
}

void DecalFont_DrawLine(char *str, s16 x, s16 y, s16 font, s16 flags)
{
	(void)str; (void)x; (void)y; (void)font; (void)flags;
}

int GAMEPROG_AdventureTimeLimit(int time) { return time; }
char *RECTMENU_DrawTime(int time) { (void)time; return ""; }
void MainGameEnd_Initialize(void) {}

#include <game/UI/UI_Clock.c>

int main(void)
{
	struct Driver driver = {0};
	sdata->gGT = &tracker;
	tracker.gameMode1 = ARCADE_MODE;
	tracker.numLaps = 7;
	driver.driverID = 2;
	driver.lapIndex = 6;

	CTR_TEST_SECTION("Live and finished seven-lap race clocks");
	UI_DrawRaceClock(0, 0, UI_RACE_CLOCK_SHOW_CURRENT_TIME, &driver);
	CTR_TEST_CHECK(savedCalls > 0 && savedLap == 6 && savedDriver == 2,
	               "the running clock updates the current player's seventh lap");

	for (int player = 0; player < 4; player++)
	{
		savedCalls = 0;
		driver.driverID = player;
		driver.lapIndex = 7;
		UI_DrawRaceClock(0, 0, UI_RACE_CLOCK_SHOW_RESULTS, &driver);
		// Without the finished-lap guard, slot 7 overwrites the next player or
		// runs past the fourth player's timing storage.
		CTR_TEST_CHECK(savedCalls == 0,
		               "finished results never write an eighth lap");
	}

	tracker.numLaps = 3;
	driver.lapIndex = 3;
	UI_DrawRaceClock(0, 0, UI_RACE_CLOCK_SHOW_RESULTS, &driver);
	CTR_TEST_CHECK(savedCalls == 0, "ordinary finished races also leave recorded laps unchanged");

	return CtrTest_Summary();
}
