#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80030208-0x80030264.
void Garage_Init(void)
{
	struct GarageFX *garageSounds;
	char i;

	// erase backup, keep music, stop all fx
	howl_StopAudio(1, 0, 1);

	for (i = 0; i < 8; i++)
	{
		garageSounds = &sdata->garageSoundPool[i];
		garageSounds->gsp_curr = GSP_GONE;
		garageSounds->gsp_prev = GSP_GONE;
		garageSounds->volume = 0;
		garageSounds->LR = 0;
		garageSounds->audioPtr = 0;
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80030264-0x80030404.
void Garage_Enter(char charId)
{
	struct GarageFX *garageSounds;
	u8 *soundIDs;
	int i;
	int charRight;
	int charLeft;
	int LR;

	//>=8
	if (charId >= PINSTRIPE)
	{
		return;
	}

	// characterID to the left/right
	charLeft = (charId + -1) & 7;
	charRight = (charId + 1) & 7;

	soundIDs = &sdata->garageSoundIDs[0];

	// loop through all characters in garage
	for (i = 0; i < 8; i++)
	{
		garageSounds = &sdata->garageSoundPool[i];

		garageSounds->gsp_prev = GSP_GONE;
		garageSounds->volume = 0;

		// if this character is in focus
		if (i == charId)
		{
			garageSounds->gsp_curr = GSP_CENTER;

			// Balance Left/Right
			LR = 0x80;
		}

		// if this character is to the left
		else if (i == charLeft)
		{
			garageSounds->gsp_curr = GSP_LEFT;

			// 75% left, 25% right
			LR = 0x3c;
		}

		// if this character is to the right
		else if (i == charRight)
		{
			garageSounds->gsp_curr = GSP_RIGHT;

			// 25% left, 75% right
			LR = 0xc3;
		}

		// if this character is too far away
		// to make any sound at all
		else
		{
			garageSounds->gsp_curr = GSP_GONE;
			garageSounds->LR = 0x80;
			garageSounds->audioPtr = 0;
			continue;
		}

		// === only if 'i' is center/left/right ===

		garageSounds->LR = LR;

		if (soundIDs[i] == 0)
		{
			garageSounds->audioPtr = 0;
			continue;
		}

		OtherFX_RecycleNew((u32 *)&garageSounds->audioPtr, (int)soundIDs[i], 0x8000 | LR);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80030404-0x800304b8.
void Garage_PlayFX(u32 soundId, char charId)
{
	if (charId < PINSTRIPE)
	{
		// if sound == BIRD_RANDOM
		if (soundId == 0xf6)
		{
			sdata->audioRNG = ((sdata->audioRNG >> 3) + sdata->audioRNG * 0x20000000) * 5 + 1;
			soundId = (sdata->audioRNG % 3) + 0xf3;
		}

		OtherFX_Play_LowLevel(soundId & 0xffff, 1, sdata->garageSoundPool[charId].volume << 0x10 | sdata->garageSoundPool[charId].LR | 0x8000);
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800304b8-0x80030694.
void Garage_LerpFX(void)
{
	struct GarageFX *garageSounds = sdata->garageSoundPool;
	u32 *audioPtrRef;

	for (int i = 0; i < 8; ++i, ++garageSounds)
	{
		s16 targetVolume, targetLR;
		char cVar1 = garageSounds->gsp_curr;

		if (cVar1 == GSP_CENTER)
		{
			targetVolume = 0xff;
			targetLR = 0x80;
		}
		else if (cVar1 == GSP_LEFT)
		{
			targetVolume = 100;
			targetLR = 0x3c;
		}
		else if (cVar1 == GSP_RIGHT)
		{
			targetVolume = 100;
			targetLR = 0xc3;
		}
		else
		{
			targetVolume = 0;
			targetLR = garageSounds->LR;
		}

		if (targetLR == garageSounds->LR && targetVolume == garageSounds->volume)
		{
			continue;
		}

		if (targetVolume != garageSounds->volume)
		{
			s16 delta = (garageSounds->volume < targetVolume) ? 8 : -8;
			garageSounds->volume += delta;
			if ((delta > 0 && garageSounds->volume > targetVolume) || (delta < 0 && garageSounds->volume < targetVolume))
			{
				garageSounds->volume = targetVolume;
			}
		}

		if (targetLR != garageSounds->LR)
		{
			s16 delta = (garageSounds->LR < targetLR) ? 2 : -2;
			garageSounds->LR += delta;
			if ((delta > 0 && garageSounds->LR > targetLR) || (delta < 0 && garageSounds->LR < targetLR))
			{
				garageSounds->LR = targetLR;
			}
		}

		audioPtrRef = (u32 *)&garageSounds->audioPtr;
		if (sdata->garageSoundIDs[i] != 0)
		{
			OtherFX_RecycleNew(audioPtrRef, sdata->garageSoundIDs[i], ((int)garageSounds->volume << 0x10) | (int)garageSounds->LR | 0x8000U);
		}

		if (targetLR == garageSounds->LR && targetVolume == garageSounds->volume)
		{
			garageSounds->gsp_prev = garageSounds->gsp_curr;

			if (garageSounds->gsp_curr == GSP_GONE)
			{
				OtherFX_RecycleMute((int *)audioPtrRef);
			}
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80030694-0x8003074c.
void Garage_MoveLR(int desiredId)
{
	struct GarageFX *garageSounds;
	char i;
	char charRight;
	char charLeft;

	// shouldn't ever happen
	if (desiredId > 7)
	{
		return;
	}

	charLeft = (desiredId + -1) & 7;
	charRight = (desiredId + 1) & 7;

	// loop through 8 characters
	for (i = 0; i < 8; i++)
	{
		garageSounds = &sdata->garageSoundPool[i];

		// character in focus
		if (i == desiredId)
		{
			garageSounds->gsp_curr = GSP_CENTER;
		}

		else if (i == charLeft)
		{
			if (garageSounds->gsp_curr == GSP_GONE)
			{
				// 75% left, 25% right
				garageSounds->LR = 0x3c;
			}

			garageSounds->gsp_curr = GSP_LEFT;
		}

		else if (i == charRight)
		{
			if (garageSounds->gsp_curr == GSP_GONE)
			{
				// 25% left, 75% right
				garageSounds->LR = 0xc3;
			}

			garageSounds->gsp_curr = GSP_RIGHT;
		}

		else
		{
			garageSounds->gsp_curr = GSP_GONE;
		}
	}
	return;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8003074c-0x80030778.
void Garage_Leave(void)
{
	int i;
	struct GarageFX *garageSounds = sdata->garageSoundPool;

	for (i = 0; i < 8; i++)
	{
		garageSounds[i].gsp_curr = GSP_GONE;
	}

	return;
}
