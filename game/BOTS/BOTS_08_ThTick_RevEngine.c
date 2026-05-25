#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001372c-0x80013838.
void BOTS_ThTick_RevEngine(struct Thread *botThread)
{
	struct Driver *botDriver = (struct Driver *)botThread->object;
	struct MaskHeadWeapon *mask = botDriver->botData.maskObj;

	if (botDriver->botData.ai_posBackup[1] < botDriver->posCurr.y)
	{ // mask grabbed
		botDriver->posCurr.y -= ((sdata->gGT->elapsedTimeMS << 9) >> 5);

		if (mask != NULL)
		{
			mask->pos[0] = (s16)(botDriver->posCurr.x >> 8);
			mask->pos[1] = (s16)(botDriver->posCurr.y >> 8);
			mask->pos[2] = (s16)(botDriver->posCurr.z >> 8);
		}

		VehPhysForce_TranslateMatrix(botThread, botDriver);
		VehFrameProc_Driving(botThread, botDriver);
		VehEmitter_DriverMain(botThread, botDriver);
	}
	else
	{ // not a mask grab
		if (mask != NULL)
		{
			mask->scale = 0x1000;
			mask->duration = 0;
			mask->rot[2] &= 0xfffe;
		}

		botDriver->botData.maskObj = NULL;
		botDriver->kartState = KS_ENGINE_REVVING;
		botDriver->clockReceive = 0;
		botDriver->squishTimer = 0;

		ThTick_SetAndExec(botThread, BOTS_ThTick_Drive);
	}
}
