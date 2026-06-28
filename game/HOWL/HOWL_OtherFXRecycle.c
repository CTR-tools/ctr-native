#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002e690-0x8002e724
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
		local = 0;
	}

	if (newSoundID != (u32)-1)
	{
		// if this is a new sound
		if (local == 0)
		{
			*soundID_Count = OtherFX_Play_LowLevel(newSoundID & 0xffff, 0, modifyFlags);
		}
		// if not a new sound,
		// modification of old sound
		else
		{
			OtherFX_Modify(local, modifyFlags);
		}
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002e724-0x8002e760
void OtherFX_RecycleMute(int *soundID_Count)
{
	if (*soundID_Count != 0)
	{
		OtherFX_Stop1(*soundID_Count);
		*soundID_Count = 0;
	}
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8002e760-0x8002e7bc
void OtherFX_DriverCrashing(u32 boolEcho, u32 volume)
{
	u32 crashID;

	if (volume < 0xdd)
	{
		crashID = 0xb;

		if (volume > 0xa0)
		{
			crashID = 0xc;
		}
	}
	else
	{
		crashID = 0xa;
	}

	OtherFX_Play_LowLevel(crashID, 0, HowlSfx_Pack(HOWL_SFX_LR_CENTER, HOWL_SFX_DISTORTION_NONE, volume, boolEcho));
}
