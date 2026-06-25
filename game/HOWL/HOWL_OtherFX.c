#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002843c-0x80028468
int CountSounds(void)
{
	// watch it increase when scrolling in main menu
	sdata->countSounds += 1;
	if (sdata->countSounds == 0)
	{
		sdata->countSounds = 1;
	}
	return sdata->countSounds;
}

// param1 - soundID
// param2:
// 0 - play with no duplicates (dont recycle old)
// 1 - play with no duplicates (recycle old)
// 2 - play with duplicates
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80028468-0x80028494
int OtherFX_Play(u32 soundID, int flags)
{
	// ff8080:
	// 0x00 - no echo
	// 0xff - volume
	// 0x80 - distortion (none)
	// 0x80 - LR (center of left and right)
	return OtherFX_Play_LowLevel(soundID & 0xffff, flags & 0xff, 0xff8080);
}

// param_3:
// 0 - normal
// 1 - echo
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80028494-0x800284d0
void OtherFX_Play_Echo(u32 soundID, int flags, int echoFlag)
{
	// ff8080:
	// 0x00 - no echo
	// 0xff - volume
	// 0x80 - distortion (none)
	// 0x80 - LR (center of left and right)
	int otherFlags = 0xff8080;

	if (echoFlag != 0)
	{
		otherFlags |= 0x1000000;
	}

	OtherFX_Play_LowLevel(soundID & 0xffff, flags & 0xff, otherFlags);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800284d0-0x80028690
int OtherFX_Play_LowLevel(u32 soundID, u8 boolAntiSpam, u32 flags)
{
	struct GameTracker *gGT = sdata->gGT;
	struct ChannelStats *channel;
	int count;
	s16 id;
	struct OtherFX *ptrOtherFX;
	u32 LR = (flags) & 0xff;
	u32 distortion = (flags >> 8) & 0xff;
	u32 volume = (flags >> 0x10) & 0xff;
	u16 echo = (flags >> 0x18) & 0xff;
	struct ChannelAttr channelAttr;

	if (sdata->boolAudioEnabled == 0)
	{
		return 0;
	}

	id = soundID & 0xffff;

	// quit if out of bounds
	if (id >= sdata->ptrHowlHeader->numOtherFX)
	{
		return 0;
	}

	// get pointer to cseq audio, given soundID
	ptrOtherFX = &sdata->howl_metaOtherFX[id];

	// quit if effect is not loaded
	if (sdata->howl_spuAddrs[ptrOtherFX->spuIndex].spuAddr == 0)
	{
		return 0;
	}

	howl_InitChannelAttr_OtherFX(ptrOtherFX, &channelAttr, volume, LR, distortion);

	channelAttr.reverb = echo;

	Smart_EnterCriticalSection();

	// does this ever happen?
	// breakpoint 8002b5b4 in OG CTR
	if (
	    // if can not play with duplicates (at all)
	    (boolAntiSpam == 2) && (Channel_FindSound(id) != 0 // if sound is already playing
	                            ))
	{
		Smart_ExitCriticalSection();
		return 0;
	}

	// This function allows duplicates of functions,
	// but not within 10 frames of each other, depending on boolAntiSpam
	channel = Channel_AllocSlot_AntiSpam(id, boolAntiSpam, 0x7c, &channelAttr);

	if (channel == 0)
	{
		// NOTE(aalhendi): Retail falls through to a PSX null-space read here.
		// Native returns the no-sound result explicitly instead of crashing.
		Smart_ExitCriticalSection();
		return 0;
	}

	if ((ptrOtherFX->flags & 2) != 0)
	{
		channel->flags |= 4;
	}

	// type otherFX
	channel->type = 1;
	channel->unk2 = 0;
	channel->echo = echo;
	channel->vol = volume;
	channel->distort = distortion;
	channel->LR = LR;
	channel->timeLeft = ptrOtherFX->duration;

	// soundID, shift in CountSounds for
	// this specific instance of the sound
	count = CountSounds();
	channel->soundID = (count << 0x10) | id;

	// save the frame that the channel started, frameTimer_MainFrame_ResetDB
	channel->startFrame = gGT->frameTimer_MainFrame_ResetDB;

	Smart_ExitCriticalSection();
	return channel->soundID;
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80028690-0x80028808
u32 OtherFX_Modify(u32 soundId, u32 flags)
{
	struct ChannelStats *channel;
	struct ChannelAttr channelAttr;
	int modify;
	struct OtherFX *ptrOtherFX;
	u32 distort;
	u32 volume;
	u16 echo;
	u16 LR;

	if (sdata->boolAudioEnabled == 0)
	{
		return 0;
	}

	// quit if out of bounds
	if ((sdata->ptrHowlHeader->numOtherFX) <= (int)(soundId & 0xffff))
	{
		return 0;
	}

	// metaOtherFX
	ptrOtherFX = &sdata->howl_metaOtherFX[soundId & 0xffff];
	volume = flags >> 0x10 & 0xff;
	distort = flags >> 8 & 0xff;
	echo = flags >> 0x18;
	LR = flags & 0xff;

	// volume of FX
	modify = sdata->vol_FX;
	if ((ptrOtherFX->flags & 4) != 0)
	{
		// volume of Voice
		modify = sdata->vol_Voice;
	}

	// no distortion
	if (distort == 0x80)
	{
		channelAttr.pitch = ptrOtherFX->pitch;
	}

	// distortion
	else
	{
		channelAttr.pitch = ptrOtherFX->pitch * data.distortConst_OtherFX[distort] >> 0x10;
	}

	Channel_SetVolume(&channelAttr, modify * ptrOtherFX->volume * volume >> 10, LR);
	channelAttr.reverb = echo;

	Smart_EnterCriticalSection();

	// 1 - otherFX
	// soundID & 0xffffffff, search for specific instance
	channel = Channel_SearchFX_EditAttr(1, soundId, 0x70, &channelAttr);

	if (channel != 0)
	{
		channel->echo = echo;
		channel->vol = volume;
		channel->distort = distort;
		channel->LR = LR;
	}

	Smart_ExitCriticalSection();

	return 1;
}

// specific instance of soundID
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80028808-0x80028844
void OtherFX_Stop1(int soundID_count)
{
	Smart_EnterCriticalSection();

	// specific instance of soundID
	Channel_SearchFX_Destroy(1, soundID_count, 0xffffffff);

	Smart_ExitCriticalSection();
}

// all instances of soundID
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80028844-0x80028880
void OtherFX_Stop2(int soundID_count)
{
	Smart_EnterCriticalSection();

	// all instances of soundID
	Channel_SearchFX_Destroy(1, soundID_count & 0xffff, 0xffff);

	Smart_ExitCriticalSection();
}
