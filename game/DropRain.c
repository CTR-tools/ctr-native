#include <common.h>


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80024464-0x8002451c.
void DropRain_MakeSound(struct GameTracker *gGT)
{
	int i, lev;
	u32 rained;

	rained = 0;
	lev = gGT->levelID;

	// if you are not in
	if ((lev != TIGER_TEMPLE) && (lev != CORTEX_CASTLE))
	{
		return;
	}

	for (i = 0; i < gGT->numPlyrCurrGame; i++)
	{
		rained |= gGT->rainBuffer[i].numParticles_curr;
	}

	// if someone is rained on
	if (rained != 0)
	{
		// if there is no rain
		if (gGT->rainSoundID == 0)
		{
			gGT->rainSoundID = OtherFX_Play(0x82, 0);
		}
	}

	// if nobody is rained on
	else
	{
		if (gGT->rainSoundID != 0)
		{
			OtherFX_Stop1(gGT->rainSoundID);
			gGT->rainSoundID = 0;
		}
	}
	return;
}


// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002451c-0x80024524.
void DropRain_Reset(struct GameTracker *gGT)
{
	gGT->rainSoundID = 0;
	return;
}
