#include <common.h>
#include <platform/native_spu_memory.h>

static void OtherFX_InitResolvedAttr(struct OtherFX *base, struct OtherFX *localized, u16 localizedAddr,
                                     struct ChannelAttr *attr, int volume, int lr, int distortion)
{
	struct OtherFX *effect = base;
	if (localizedAddr != 0)
	{
		// The initializer uses the base sample index before we replace its address.
		localized->spuIndex = base->spuIndex;
		effect = localized;
	}
	howl_InitChannelAttr_OtherFX(effect, attr, volume, lr, distortion);
	if (localizedAddr != 0)
	{
		attr->spuStartAddr = (void *)CTR_SPU_BYTES_AT((u32)localizedAddr);
	}
}

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
int OtherFX_Play(u32 soundID, int flags)
{
	return OtherFX_Play_LowLevel(soundID & 0xffff, flags & 0xff, HOWL_SFX_DEFAULT_FLAGS);
}

// param_3:
// 0 - normal
// 1 - echo
void OtherFX_Play_Echo(u32 soundID, int flags, int echoFlag)
{
	int otherFlags = HOWL_SFX_DEFAULT_FLAGS;

	if (echoFlag != 0)
	{
		otherFlags |= HOWL_SFX_ECHO_FLAG;
	}

	OtherFX_Play_LowLevel(soundID & 0xffff, flags & 0xff, otherFlags);
}

int OtherFX_Play_LowLevel(u32 soundID, u8 boolAntiSpam, u32 flags)
{
	struct GameTracker *gGT = sdata->gGT;
	struct ChannelStats *channel;
	int count;
	u16 id;
	struct OtherFX *ptrOtherFX;
	u32 LR = HowlSfx_LR(flags);
	u32 distortion = HowlSfx_Distortion(flags);
	u32 volume = HowlSfx_Volume(flags);
	u16 echo = (u16)HowlSfx_Echo(flags);
	struct ChannelAttr channelAttr;
	struct OtherFX localized;
	u16 localizedAddr = 0;

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

	if (!Bank_LocalizedVoice(id, &localized, &localizedAddr) && !Bank_EnsureOtherFX(id))
	{
		return 0;
	}

	const bool doubleGain = (flags & HOWL_SFX_GAIN_DOUBLE_FLAG) != 0;
	OtherFX_InitResolvedAttr(ptrOtherFX, &localized, localizedAddr, &channelAttr, HowlSfx_EffectiveVolume(volume, doubleGain), LR, distortion);
	if (localizedAddr != 0)
	{
		ptrOtherFX = &localized;
	}

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
	channel = Channel_AllocSlot_AntiSpam(id, boolAntiSpam, HOWL_CHANNEL_UPDATE_ALL_ATTRS, &channelAttr);

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

	channel->type = HOWL_CHANNEL_TYPE_OTHER_FX;
	if (doubleGain)
	{
		channel->flags |= HOWL_CHANNEL_GAIN_DOUBLE;
	}
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

u32 OtherFX_Modify(u32 soundId, u32 flags)
{
	struct ChannelStats *channel;
	struct ChannelAttr channelAttr;
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
	volume = HowlSfx_Volume(flags);
	distort = HowlSfx_Distortion(flags);
	echo = (u16)HowlSfx_Echo(flags);
	LR = (u16)HowlSfx_LR(flags);

	const bool doubleGain = (flags & HOWL_SFX_GAIN_DOUBLE_FLAG) != 0;
	howl_InitChannelAttr_OtherFX(ptrOtherFX, &channelAttr, HowlSfx_EffectiveVolume(volume, doubleGain), LR, distort);
	channelAttr.reverb = echo;

	Smart_EnterCriticalSection();

	// soundID & 0xffffffff, search for specific instance
	channel = Channel_SearchFX_EditAttr(HOWL_CHANNEL_TYPE_OTHER_FX, soundId, HOWL_CHANNEL_UPDATE_DYNAMIC_ATTRS, &channelAttr);

	if (channel != 0)
	{
		channel->flags &= ~HOWL_CHANNEL_GAIN_DOUBLE;
		if (doubleGain)
		{
			channel->flags |= HOWL_CHANNEL_GAIN_DOUBLE;
		}
		channel->echo = echo;
		channel->vol = volume;
		channel->distort = distort;
		channel->LR = LR;
	}

	Smart_ExitCriticalSection();

	return 1;
}

// specific instance of soundID
void OtherFX_Stop1(int soundID_count)
{
	Smart_EnterCriticalSection();

	// specific instance of soundID
	Channel_SearchFX_Destroy(HOWL_CHANNEL_TYPE_OTHER_FX, soundID_count, 0xffffffff);

	Smart_ExitCriticalSection();
}

// all instances of soundID
void OtherFX_Stop2(int soundID_count)
{
	Smart_EnterCriticalSection();

	// all instances of soundID
	Channel_SearchFX_Destroy(HOWL_CHANNEL_TYPE_OTHER_FX, soundID_count & 0xffff, 0xffff);

	Smart_ExitCriticalSection();
}
