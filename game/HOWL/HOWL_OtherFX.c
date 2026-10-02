#include <common.h>

#ifndef HOWL_OTHER_FX_METADATA
#define HOWL_OTHER_FX_METADATA     (sdata->howl_metaOtherFX)
#define HOWL_OTHER_FX_VOLUME       (sdata->vol_FX)
#define HOWL_OTHER_FX_VOICE_VOLUME (sdata->vol_Voice)
#endif

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
// 0 - allocate a new channel
// 1 - recycle matching sounds started within 10 frames
// 2 - reject a sound that is already playing
int OtherFX_Play(u32 soundID, int flags)
{
	return OtherFX_Play_LowLevel(soundID & 0xffff, flags & 0xff, HOWL_SFX_DEFAULT_FLAGS);
}

// param_3:
// 0 - normal
// 1 - echo
void OtherFX_Play_Echo(u32 soundID, int flags, int echoFlag)
{
	u32 id = soundID & 0xffff;
	OtherFX_Play_LowLevel(id, flags & 0xff, echoFlag ? HOWL_SFX_DEFAULT_FLAGS | HOWL_SFX_ECHO_FLAG : HOWL_SFX_DEFAULT_FLAGS);
}

int OtherFX_Play_LowLevel(u32 incomingSound, u8 incomingAntiSpam, u32 flags)
{
	// NOTE(aalhendi): Separate argument lifetimes preserve the retail prologue under GCC 2.8.1.
	u32 soundID = incomingSound;
	u8 boolAntiSpam = incomingAntiSpam;
	struct ChannelStats *channel;
	u32 count;
	u16 id;
	struct OtherFX *ptrOtherFX;
	u32 volume;
	u32 LR;
	u32 distortion;
	u32 echo;
	struct ChannelAttr channelAttr;

	if (GAME_AUDIO_ENABLED == 0)
		return 0;
	id = soundID & 0xffff;
	if (id >= (u32)GAME_HOWL_HEADER->numOtherFX)
		return 0;
	ptrOtherFX = &HOWL_OTHER_FX_METADATA[id];
	if (GAME_HOWL_SPU_ADDRS[ptrOtherFX->spuIndex].spuAddr == 0)
		return 0;

	volume = HowlSfx_Volume(flags);
	LR = HowlSfx_LR(flags);
	distortion = HowlSfx_Distortion(flags);
	echo = HowlSfx_Echo(flags);
	howl_InitChannelAttr_OtherFX(ptrOtherFX, &channelAttr, volume, LR, distortion);
	channelAttr.reverb = echo;
	Smart_EnterCriticalSection();

	if (boolAntiSpam == 2 && Channel_FindSound(id) != 0)
	{
		Smart_ExitCriticalSection();
		return 0;
	}

	soundID &= 0xffff;
	channel = Channel_AllocSlot_AntiSpam(soundID, boolAntiSpam, HOWL_CHANNEL_UPDATE_ALL_ATTRS, &channelAttr);
	if (channel != 0)
	{
		if ((ptrOtherFX->flags & 2) != 0)
			channel->flags |= 4;
		channel->type = HOWL_CHANNEL_TYPE_OTHER_FX;
		channel->unk2 = 0;
		channel->echo = echo;
		channel->vol = volume;
		channel->distort = distortion;
		channel->LR = LR;
		channel->timeLeft = ptrOtherFX->duration;
		count = CountSounds();
		channel->soundID = (count << 16) | soundID;
		// The tracker is read after the initializer and critical-section calls.
		channel->startFrame = GAME_TRACKER->frameTimer_MainFrame_ResetDB;
	}

	Smart_ExitCriticalSection();
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Retail reads address 0x18 after a null allocation; native
	// preserves the no-sound result without accessing unmapped null storage.
	if (channel == 0)
		return 0;
#endif
	return channel->soundID;
}

u32 OtherFX_Modify(u32 soundId, u32 flags)
{
	struct ChannelStats *channel;
	struct ChannelAttr channelAttr;
	// NOTE(aalhendi): These bindings preserve retail operand and saved-register order.
	register u32 modify CTR_PSX_REGISTER("$3");
	struct OtherFX *ptrOtherFX;
	u32 distort;
	u32 volume;
	register u16 echo CTR_PSX_REGISTER("$19");
	register u16 LR CTR_PSX_REGISTER("$20");
	u16 id;
	u16 pitch;
	u32 finalVolume;
	register u32 product CTR_PSX_REGISTER("$8");
	register u32 gain CTR_PSX_REGISTER("$2");

	if (GAME_AUDIO_ENABLED == 0)
		return 0;

	id = soundId & 0xffff;
	if (id >= (u32)GAME_HOWL_HEADER->numOtherFX)
		return 0;

	ptrOtherFX = &HOWL_OTHER_FX_METADATA[id];
	volume = HowlSfx_Volume(flags);
	distort = HowlSfx_Distortion(flags);
	echo = (u16)HowlSfx_Echo(flags);
	LR = (u16)HowlSfx_LR(flags);

	if ((ptrOtherFX->flags & 4) != 0)
		modify = HOWL_OTHER_FX_VOICE_VOLUME;
	else
		modify = HOWL_OTHER_FX_VOLUME;

	gain = ptrOtherFX->volume;
	product = modify * gain;
	product *= volume;
	// NOTE(aalhendi): Keep the multiplier result before the distortion-threshold setup.
	CTR_PSX_KEEP_VALUE(product);
	finalVolume = product >> 10;
	if (distort != HOWL_SFX_DISTORTION_NONE)
	{
		register u32 samplePitch CTR_PSX_REGISTER("$4");
		// Gain, table, and factor use $2 during separate lifetimes.
		register u32 factor CTR_PSX_REGISTER("$2");
		register s32 *table CTR_PSX_REGISTER("$2");
		table = GAME_DISTORT_CONST_OTHER_FX;
		samplePitch = ptrOtherFX->pitch;
		factor = table[distort];
		product = samplePitch * factor;
		pitch = product >> 16;
	}
	else
		pitch = ptrOtherFX->pitch;

	// Reverb is visible to SetVolume and is written again after that call.
	channelAttr.reverb = echo;
	Channel_SetVolume(&channelAttr, finalVolume, LR);
	channelAttr.pitch = pitch;
	channelAttr.reverb = echo;

	Smart_EnterCriticalSection();
	channel = Channel_SearchFX_EditAttr(HOWL_CHANNEL_TYPE_OTHER_FX, soundId, HOWL_CHANNEL_UPDATE_DYNAMIC_ATTRS, &channelAttr);
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
