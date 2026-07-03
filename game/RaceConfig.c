#include <common.h>

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80043c10-0x80043d24.
void RaceConfig_LoadGameOptions(void)
{
	if (sdata->boolHasLoadedOptions != 0)
	{
		return;
	}

	sdata->boolHasLoadedOptions = 1;
	s16 *volumes = &sdata->gameOptions.volFx;

	for (s32 i = 0; i < GAME_OPTIONS_VOLUME_COUNT; i++)
	{
		howl_VolumeSet(i, (u8)volumes[i]);
		memcpy(&data.rwd[0], &sdata->gameOptions.rwd[0], sizeof(data.rwd));
	}

	sdata->gGT->gameMode1 |= sdata->gameOptions.gameMode1_vibrationFlags & GAME_MODE_VIBRATION_MASK;
	howl_ModeSet((u8)sdata->gameOptions.audioMode & 1);
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80043d24-0x80043e34.
void RaceConfig_SaveGameOptions(void)
{
	s16 *volumes = &sdata->gameOptions.volFx;

	for (s32 i = 0; i < GAME_OPTIONS_VOLUME_COUNT; i++)
	{
		volumes[i] = howl_VolumeGet(i) & 0xff;
	}

	memcpy(&sdata->gameOptions.rwd[0], &data.rwd[0], sizeof(data.rwd));
	sdata->gameOptions.gameMode1_vibrationFlags = sdata->gGT->gameMode1 & GAME_MODE_VIBRATION_MASK;
	CTR_WriteU16LE(&sdata->gameOptions.audioMode, (u16)(howl_ModeGet() != 0));
}
