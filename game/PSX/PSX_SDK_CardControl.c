#include <macros.h>

extern void EnterCriticalSection(void);
extern void ExitCriticalSection(void);
extern void ChangeClearPAD(s32 val);
extern void StartCARD2(void);
extern void StopCARD2(void);
extern void _ExitCard(void);

void StartCARD(void)
{
	EnterCriticalSection();
	StartCARD2();
	ChangeClearPAD(0);
	ExitCriticalSection();
}

void StopCARD(void)
{
	StopCARD2();
	_ExitCard();
}
