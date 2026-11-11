#include "VehCommon.h"

enum { VEH_BIRTH_META_PHYS_COUNT = len(data.metaPhys) };

typedef u16 VehBirthHalfword CTR_MAY_ALIAS;
typedef u32 VehBirthWord CTR_MAY_ALIAS;

int VehBirth_EngineClassForPlayer(int player, int defaultClass)
{
	if (g_config.engineSelection && player >= 0 && player < (int)len(sdata->selectedEngineClass))
	{
		const int selected = sdata->selectedEngineClass[player];
		if (selected > 0 && selected <= NUM_CLASSES)
		{
			return selected - 1;
		}
	}
	return defaultClass;
}

void VehBirth_SetConstsForClass(struct Driver *driver, int engineID)
{
	for (int i = 0; i < VEH_BIRTH_META_PHYS_COUNT; i++)
	{
		const struct MetaPhys *metaPhys = &data.metaPhys[i];
		u32 rawValue = (u32)metaPhys->value[engineID];
		switch (metaPhys->offset)
		{
			case SPEED_CLASS_STAT_OFFSET:
				rawValue = rawValue * g_config.speedMultiplier / 100;
				break;
			case GRAVITY_OFFSET:
				rawValue = rawValue * g_config.gravityMultiplier / 100;
				break;
			case TURN_RATE_OFFSET:
				rawValue = rawValue * g_config.turnMultiplier / 100;
				break;
			case JUMP_OFFSET:
				rawValue = rawValue * g_config.jumpMultiplier / 100;
				break;
			case TURBO_FULL_BAR_RESERVE_GAIN_OFFSET:
				rawValue = rawValue * g_config.reserveMultiplier / 100;
				break;
			default:
				break;
		}

		u8 *driverField = (u8 *)driver + metaPhys->offset;
		switch (metaPhys->size)
		{
			case 1:
				driverField[0] = (u8)rawValue;
				break;
			case 2:
				*(VehBirthHalfword *)driverField = (VehBirthHalfword)rawValue;
				break;
			case 4:
				*(VehBirthWord *)driverField = rawValue;
				break;
		}
	}
}

void VehBirth_SetConsts(struct Driver *driver)
{
	const int engineID = GAME_CHARACTER_METADATA[GAME_CHARACTER_IDS[driver->driverID]].engineID;
	VehBirth_SetConstsForClass(driver, engineID);
}
