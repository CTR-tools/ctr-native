#include <macros.h>

extern void ChangeClearPAD(s32 val);
extern void EnterCriticalSection(void);
extern void ExitCriticalSection(void);
extern s32 ReadInitPadFlag(void);
extern void InitCARD2(s32 val);
extern void _copy_memcard_patch(void);
extern void _patch_card_entry(void);
extern void _patch_card2(void);
extern void _new_card(void);
extern void _card_write(s32 chan, s32 block, void *buf);

void InitCARD(s32 val)
{
	ChangeClearPAD(0);
	EnterCriticalSection();
	if (ReadInitPadFlag() == 0)
	{
		val = 0;
	}
	InitCARD2(val);
	_copy_memcard_patch();
	_patch_card_entry();
	_patch_card2();
	ExitCriticalSection();
}

void _card_clear(s32 chan)
{
	_new_card();
	_card_write(chan, 63, NULL);
}
