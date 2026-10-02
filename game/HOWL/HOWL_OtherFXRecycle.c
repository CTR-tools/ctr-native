#include <common.h>

void OtherFX_RecycleNew(u32 *soundID_Count, u32 newSoundID, u32 modifyFlags)
{
	int local = *soundID_Count;

	if (
	    // if this sound is already playing
	    (local != 0) &&

	    // if soundID doesn't match new ID
	    ((local & 0xffff) != newSoundID))
	{
		OtherFX_Stop1(local);

		*soundID_Count = 0;
	}

	if (newSoundID != (u32)-1)
	{
		// if this is a new sound
		if (*soundID_Count == 0)
		{
			*soundID_Count = OtherFX_Play_LowLevel(newSoundID & 0xffff, 0, modifyFlags);
		}
		// if not a new sound,
		// modification of old sound
		else
		{
			OtherFX_Modify(*soundID_Count, modifyFlags);
		}
	}
}

void OtherFX_RecycleMute(u32 *soundID_Count)
{
	if (*soundID_Count != 0)
	{
		OtherFX_Stop1(*soundID_Count);
		*soundID_Count = 0;
	}
}

void OtherFX_DriverCrashing(u32 boolEcho, u32 volume)
{
	u32 crashID;
	u32 packedVolume;
	// NOTE(aalhendi): Keep mode in its call argument register so volume survives its clear.
	register u32 playMode CTR_PSX_REGISTER("$5");

	if (volume >= 0xdd)
	{
		// NOTE(aalhendi): Keep the high-volume assignment on its own branch.
		CTR_PSX_OBSERVE_VALUE(boolEcho);
		crashID = 0xa;
	}
	else
	{
		crashID = 0xb;
		if (volume > 0xa0)
		{
			crashID = 0xc;
		}
	}

	playMode = 0;
	// NOTE(aalhendi): Clear mode before packing the flags.
	CTR_PSX_DEPEND_VALUE(boolEcho, playMode);
	boolEcho <<= HOWL_SFX_ECHO_SHIFT;
	packedVolume = (volume & 0xff) << HOWL_SFX_VOLUME_SHIFT;
	boolEcho |= packedVolume;
	boolEcho |= HOWL_SFX_CENTER_NO_DISTORTION;
	OtherFX_Play_LowLevel(crashID, playMode, boolEcho);
}
