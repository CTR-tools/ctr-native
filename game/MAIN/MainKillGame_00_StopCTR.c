#include <common.h>

// NOTE(aalhendi): PSX path ASM-verified NTSC-U 926 0x8003c41c-0x8003c480.
void MainKillGame_StopCTR(void)
{
	EnterCriticalSection();
	DrawSyncCallback((void (*)(void))sdata->MainDrawCb_DrawSyncPtr);
	ExitCriticalSection();
	StopCallback();

#ifndef CTR_NATIVE
	MEMCARD_CloseCard();
#else
	// NOTE(aalhendi): Native skips PSX memcard event teardown.
#endif

	PadStopCom();
	ResetGraph(3);
	VSyncCallback(0);

	Timer_Destroy();
}
