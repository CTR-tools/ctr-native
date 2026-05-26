#include <common.h>

static s16 *SelectProfile_Mode(void)
{
	return (s16 *)&sdata->data10_bbb[0];
}

static s16 *SelectProfile_TimerSaveComplete(void)
{
	return (s16 *)&sdata->data10_bbb[12];
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80048e2c-0x80048edc.
void SelectProfile_ToggleMode(u32 mode)
{
	sdata->memcardAction = mode & 0xf;

	// 0x00 AdvNew, 0x10 AdvLoad, 0x20 green load/save, 0x30 ghost, 0x40 slot selected.
	*SelectProfile_Mode() = mode & 0xf0;
	*(s16 *)&sdata->data10_bbb[4] = 0;
	*(s16 *)&sdata->data10_bbb[6] = 0;
	*(s16 *)&sdata->data10_bbb[8] = 0;
	*(s16 *)&sdata->data10_bbb[10] = 0;
	*SelectProfile_TimerSaveComplete() = 0;

	SelectProfile_UnMuteCursors();

	data.menuFourAdvProfiles.drawStyle &= ~0x10;
	data.menuOverwriteAdv.drawStyle &= ~0x10;
	if (*SelectProfile_Mode() == 0x20)
	{
		data.menuFourAdvProfiles.drawStyle |= 0x10;
		data.menuOverwriteAdv.drawStyle |= 0x10;
	}

	SelectProfile_Init(data.menuFourAdvProfiles.drawStyle);

	data.menuFourAdvProfiles.rowSelected = sdata->unk_8008d73C_relatedToRowHighlighted;
	*(s16 *)&sdata->data10_bbb[2] = 0;
}
