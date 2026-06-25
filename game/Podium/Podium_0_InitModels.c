#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80041c84-0x80041dc0.
void Podium_InitModels(struct GameTracker *gGT)
{
	gGT->podium_modelIndex_First = 0;
	gGT->podium_modelIndex_Second = 0;
	gGT->podium_modelIndex_Third = 0;
	gGT->podium_modelIndex_tawna = STATIC_TAWNA1;

	u8 *podiumModelIndexArr = &gGT->podium_modelIndex_First;

	for (int i = 0; i < 8; i++)
	{
		struct Driver *driver = gGT->drivers[i];

		if (driver == NULL)
		{
			continue;
		}

		s16 rank = driver->driverRank;

		if (rank < 3)
		{
			u8 characterID = data.characterIDs[driver->driverID];
			podiumModelIndexArr[rank] = characterID + STATIC_CRASHDANCE;

			if (rank != 0)
			{
				continue;
			}

			switch (characterID)
			{
			case 0:
			case 3:
				gGT->podium_modelIndex_tawna = STATIC_TAWNA2;
				break;

			case 6:
			case 7:
				gGT->podium_modelIndex_tawna = STATIC_TAWNA3;
				break;

			case 1:
			case 4:
				gGT->podium_modelIndex_tawna = STATIC_TAWNA4;
				break;

			default:
				gGT->podium_modelIndex_tawna = STATIC_TAWNA1;
				break;
			}
		}
	}
}
