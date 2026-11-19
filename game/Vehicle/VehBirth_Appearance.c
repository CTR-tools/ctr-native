#include "VehCommon.h"

enum
{
	VEH_BIRTH_WHEEL_SIZE = 0xccc,
	VEH_BIRTH_KART_SCALE_NORMAL = 1,
	VEH_BIRTH_RESERVED_0x412_INITIAL = 0x600,
	VEH_BIRTH_STEERING_FRAMES_RESET = 10000,
	VEH_BIRTH_QUIP_NONE = -1,
};

void VehBirth_SetWheelSizeForCharacter(struct Driver *driver, int character)
{
	driver->wheelSize = character == NITROS_OXIDE ? 0 : VEH_BIRTH_WHEEL_SIZE;
}

void VehBirth_TireSprites(struct Thread *t)
{
	struct Driver *d = t->object;
	struct IconGroup *tireAnim = GAME_TRACKER->iconGroup[0];
	struct GameTracker *battleGameTracker;
	int battleTeamID;
	int battleLifeLimit;
	int driverID = d->driverID;

	struct Icon **tire = ICONGROUP_GETICONS(tireAnim);
	d->wheelSprites = tire;
	d->kartScale = VEH_BIRTH_KART_SCALE_NORMAL;
	d->wheelSize = VEH_BIRTH_WHEEL_SIZE;

	// Menu birth keeps the default wheels until the selected character is shown.
	if (GAME_TRACKER->levelID != MAIN_MENU_LEVEL)
	{
		VehBirth_SetWheelSizeForCharacter(d, GAME_CHARACTER_IDS[driverID]);
	}
	d->tireColor = DRIVER_TIRE_COLOR_DEFAULT;
	d->tireColorCycleTimer = DRIVER_TIRE_COLOR_TIMER_INITIAL;
	d->heldItemID = HELD_ITEM_NONE;

	d->engineSoundMode = ENGINE_SOUND_DYNAMIC;

	d->AxisAngle1_normalVec.y = FP_ONE;
	d->AxisAngle2_normalVec.y = FP_ONE;
	d->reserved_0x412 = VEH_BIRTH_RESERVED_0x412_INITIAL;
	d->numFramesSpentSteering = VEH_BIRTH_STEERING_FRAMES_RESET;

	d->terrainMeta1 = VehAfterColl_GetTerrain(TERRAIN_NONE);

	battleGameTracker = GAME_TRACKER;
	battleTeamID = d->driverID;
	battleLifeLimit = battleGameTracker->battleLifeLimit;
	d->BattleHUD.teamID = battleTeamID;
	d->quip1 = VEH_BIRTH_QUIP_NONE;
	d->quip3 = VEH_BIRTH_QUIP_NONE;
	d->BattleHUD.numLives = battleLifeLimit;
}
