#include <common.h>

static void VehPhysCrash_Attack_SetReason(struct Driver *driver, u8 reason)
{
	*(u8 *)&driver->ChangeState_param4 = reason;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8005d218-0x8005d404.
int VehPhysCrash_Attack(struct Driver *driver1, struct Driver *driver2, int canPlayFeedback, int boolPlayBubblePop)
{
	if ((driver1->actionsFlagSet & 0x800000) == 0)
	{
		if ((driver2->actionsFlagSet & 0x800000) != 0)
		{
			driver1->ChangeState_param2 = 2;
			VehPhysCrash_Attack_SetReason(driver1, 6);
			driver1->ChangeState_param3 = (int)driver2;

			if ((canPlayFeedback != 0) && (driver1->kartState != KS_BLASTED) && (driver1->invincibleTimer == 0))
			{
				OtherFX_DriverCrashing((driver1->actionsFlagSet >> 0x10) & 1, 0xff);
				Voiceline_RequestPlay(1, data.characterIDs[driver1->driverID], 0x10);
			}
		}

		if ((driver2->instBubbleHold != NULL) && (driver1->instBubbleHold == NULL))
		{
			struct Shield *bubble = driver2->instBubbleHold->thread->object;

			bubble->flags |= 8;
			driver2->instBubbleHold = NULL;

			driver1->ChangeState_param2 = 2;
			VehPhysCrash_Attack_SetReason(driver1, 0);
			driver1->ChangeState_param3 = (int)driver2;

			if ((canPlayFeedback != 0) && (driver1->kartState != KS_BLASTED) && (driver1->invincibleTimer == 0))
			{
				OtherFX_DriverCrashing((driver1->actionsFlagSet >> 0x10) & 1, 0xff);

				if (boolPlayBubblePop != 0)
				{
					OtherFX_Play(0x4f, 1);
				}

				Voiceline_RequestPlay(1, data.characterIDs[driver1->driverID], 0x10);
			}
		}

		if ((sdata->unk_8008d9f4[0] > 0xa00) && (driver2->reserves != 0) && ((driver2->actionsFlagSet & 0x200) != 0) && (driver1->reserves == 0))
		{
			driver2->forcedJump_trampoline = 2;

			driver1->ChangeState_param2 = 3;
			VehPhysCrash_Attack_SetReason(driver1, 5);
			driver1->ChangeState_param3 = (int)driver2;
		}
	}

	return canPlayFeedback;
}
