#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80047034-0x80047198.
void RefreshCard_GhostDecodeProfile(struct GhostProfile *profile, char *fileName)
{
	int packed;

	packed = (s16)RefreshCard_GhostDecodeByte(fileName[13]);
	packed |= (s16)RefreshCard_GhostDecodeByte(fileName[14]) << 6;
	packed |= (s16)RefreshCard_GhostDecodeByte(fileName[15]) << 12;
	packed |= RefreshCard_GhostDecodeByte(fileName[16]) << 18;
	packed |= RefreshCard_GhostDecodeByte(fileName[17]) << 24;
	packed |= RefreshCard_GhostDecodeByte(fileName[18]) << 30;

	profile->characterID = packed & 0xf;
	profile->trackID = (packed >> 4) & 0x1f;
	profile->trackTime = (packed >> 9) & 0xfffff;
	profile->memcardProfileIndex = (u32)packed >> 29;

	*(u8 *)&profile->alwaysOne = 0;
	memcpy(profile->profile_name, fileName, sizeof(profile->profile_name));
	*((u8 *)&profile->trackID + 1) = 0;
}
