#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800289b0-0x80028b54
s16 DECOMP_EngineAudio_Recalculate(u32 soundID, u32 sfx)
{
	int iVar1;
	u32 distortion = (sfx >> 8) & 0xff;
	u32 volume = (sfx >> 0x10) & 0xff;
	u16 echo = (sfx >> 0x18) & 0xff;
	u16 LR = sfx & 0xff;

	struct EngineFX *ptrEngineFX;
	struct ChannelStats *channel;
	struct ChannelAttr channelAttr;
	struct GameTracker *gGT;

	if (sdata->boolAudioEnabled == 0)
		return 0;

	soundID = soundID & 0xffff;
	if (sdata->ptrHowlHeader->numEngineFX <= (int)soundID)
		return 0;

	gGT = sdata->gGT;

	ptrEngineFX = &sdata->howl_metaEngineFX[soundID];

	if (gGT->numPlyrCurrGame > 1)
	{
		// 3P/4P game
		iVar1 = volume * 0x2d;

		// 2P game
		if (gGT->numPlyrCurrGame == 2)
		{
			iVar1 = volume * 0x37;
		}

		volume = (iVar1 << 2) >> 8;
	}

	// no distortion
	if (distortion == 0x80)
	{
		channelAttr.pitch = ptrEngineFX->pitch;
	}

	// distortion
	else
	{
		channelAttr.pitch = ptrEngineFX->pitch * data.distortConst_Engine[distortion] >> 0x10;
	}

	DECOMP_Channel_SetVolume(&channelAttr, sdata->vol_FX * ptrEngineFX->volume * volume >> 10, LR);
	channelAttr.reverb = echo;

	DECOMP_Smart_EnterCriticalSection();

	// 0 - engineFX
	// soundID & 0xffff, dont search for specific instance
	channel = DECOMP_Channel_SearchFX_EditAttr(0, soundID, 0x70, &channelAttr);

	if (channel != 0)
	{
		channel->echo = echo;
		channel->vol = volume;
		channel->distort = distortion;
		channel->LR = LR;
	}

	DECOMP_Smart_ExitCriticalSection();

	return 1;
}

s16 EngineAudio_Recalculate(u32 soundID, u32 sfx)
{
	return DECOMP_EngineAudio_Recalculate(soundID, sfx);
}
