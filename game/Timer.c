#include <common.h>

#define TIMER_RCNT RCntCNT1

enum TimerConstants
{
	TIMER_RCNT_TARGET = 0xffff,
	TIMER_RCNT_MODE = 0x2000,
	TIMER_RCNT_LOW_RECHECK_THRESHOLD = 100,
	TIMER_MILLISECONDS_PER_SECOND = 1000,
	TIMER_RCNT_UNITS_PER_SECOND = 0x147e,
	TIMER_WRAP_MILLISECONDS = 0xc7e18,
};

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004b31c-0x8004b370.
void Timer_Init()
{
	EnterCriticalSection();
	StopRCnt(TIMER_RCNT);
	SetRCnt(TIMER_RCNT, TIMER_RCNT_TARGET, TIMER_RCNT_MODE);
	StartRCnt(TIMER_RCNT);
	ExitCriticalSection();
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004b370-0x8004b3a4.
void Timer_Destroy()
{
	EnterCriticalSection();
	StopRCnt(TIMER_RCNT);
	ExitCriticalSection();
}

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004b3a4-0x8004b41c.
int Timer_GetTime_Total()
{
	s32 rcntTotal = sdata->rcntTotalUnits;
	s32 rcnt = GetRCnt(TIMER_RCNT);
	s32 sysClock = rcntTotal + rcnt;

	if (rcnt < TIMER_RCNT_LOW_RECHECK_THRESHOLD)
	{
		sysClock = sdata->rcntTotalUnits + rcnt;
	}

	return (sysClock * TIMER_MILLISECONDS_PER_SECOND) / TIMER_RCNT_UNITS_PER_SECOND;
}

// Usage: elapsed(frameStart, &frameStart)
// will overwrite new frameStart, and return
// elapsed time since previous frameStart
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8004b41c-0x8004b470.
int Timer_GetTime_Elapsed(int oldVal, int *retVal)
{
	s32 newVal = Timer_GetTime_Total();

	if (retVal != 0)
	{
		*retVal = newVal;
	}

	// impossible?
	if (newVal < oldVal)
	{
		newVal += TIMER_WRAP_MILLISECONDS;
	}

	return newVal - oldVal;
}
