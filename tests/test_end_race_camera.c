#include <shared/ctr_test.h>
#include <common.h>

struct sData sdata_static;
struct Data data;

int ratan2(int y, int x)
{
	(void)y;
	(void)x;
	return 0;
}

#include <game/CAM/CAM_EndOfRace.c>

int main(void)
{
	CTR_TEST_SECTION("End-of-race camera selection");
	struct GameTracker tracker = {0};
	struct Level level = {0};
	struct Driver driver = {0};
	struct PushBuffer pushBuffer = {0};
	struct CameraDC camera = {0};
	struct { struct SpawnType1 header; void *pointers[3]; } spawn = {0};
	s16 cameraList[] = {0};
	sdata->gGT = &tracker;
	tracker.level1 = &level;
	tracker.numPlyrCurrGame = 1;
	camera.pushBuffer = &pushBuffer;

	CAM_EndOfRace(&camera, &driver);
	CTR_TEST_CHECK((camera.flags & CAMERA_FLAG_BATTLE_END_OF_RACE) != 0, "missing spawn table uses the fallback camera");
	level.ptrSpawnType1 = &spawn.header;
	spawn.header.count = 2;
	camera.flags = 0;
	CAM_EndOfRace(&camera, &driver);
	CTR_TEST_CHECK((camera.flags & CAMERA_FLAG_BATTLE_END_OF_RACE) != 0, "short spawn table uses the fallback camera");
	spawn.header.count = 3;
	camera.flags = 0;
	CAM_EndOfRace(&camera, &driver);
	CTR_TEST_CHECK((camera.flags & CAMERA_FLAG_BATTLE_END_OF_RACE) != 0 &&
	               (camera.flags & CAMERA_FLAG_ARCADE_END_OF_RACE_REQUESTED) == 0,
	               "a null end-of-race camera uses the fallback instead of requesting a path");
	spawn.pointers[ST1_CAMERA_EOR] = cameraList;
	camera.flags = 0;
	CAM_EndOfRace(&camera, &driver);
	CTR_TEST_CHECK((camera.flags & CAMERA_FLAG_ARCADE_END_OF_RACE_REQUESTED) != 0,
	               "a race with a camera list requests the track camera");
	tracker.gameMode1 = BATTLE_MODE;
	camera.flags = 0;
	CAM_EndOfRace(&camera, &driver);
	CTR_TEST_CHECK((camera.flags & CAMERA_FLAG_BATTLE_END_OF_RACE) != 0, "battle retains its fallback camera");
	tracker.gameMode1 = 0;
	tracker.numPlyrCurrGame = 3;
	camera.flags = 0;
	CAM_EndOfRace(&camera, &driver);
	CTR_TEST_CHECK((camera.flags & CAMERA_FLAG_BATTLE_END_OF_RACE) != 0, "three-player races retain their fallback camera");
	return CtrTest_Summary();
}
