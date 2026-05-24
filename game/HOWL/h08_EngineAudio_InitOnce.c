#include <common.h>

// Initialize car engine audio system for one driver
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80028880-0x800289b0
char DECOMP_EngineAudio_InitOnce(u32 soundID, u32 flags)

{
	struct EngineFX *ptrEngineFX;
	struct ChannelStats *channel;
	u32 distortion = (flags >> 8) & 0xff;
	u32 volume = (flags >> 0x10) & 0xff;
	u16 echo = (flags >> 0x18) & 0xff;
	u16 LR = flags & 0xff;
	struct ChannelAttr channelAttr;

	if (sdata->boolAudioEnabled == 0)
		return 0;

	// check out of bounds
	soundID = soundID & 0xffff;
	if ((int)soundID >= sdata->ptrHowlHeader->numEngineFX)
		return 0;

	// check sound is loaded
	ptrEngineFX = &sdata->howl_metaEngineFX[soundID];
	if (sdata->howl_spuAddrs[ptrEngineFX->spuIndex].spuAddr == 0)
		return 0;

	// make ChannelAttr from howl pointer somehow
	DECOMP_howl_InitChannelAttr_EngineFX(ptrEngineFX, &channelAttr, volume, LR, distortion);

	DECOMP_Smart_EnterCriticalSection();

	channel = DECOMP_Channel_AllocSlot(0x7c, &channelAttr);

	// if channel was found
	if (channel != 0)
	{
		// type engineFX
		channel->type = 0;
		channel->unk2 = 0;
		channel->echo = echo;
		channel->vol = volume;
		channel->distort = distortion;
		channel->LR = LR;
		channel->timeLeft = 0;
		channel->soundID = soundID;
		channel->flags |= 4;
	}

	DECOMP_Smart_ExitCriticalSection();

	return (channel != 0);
}

char EngineAudio_InitOnce(u32 soundID, u32 flags)
{
	return DECOMP_EngineAudio_InitOnce(soundID, flags);
}
