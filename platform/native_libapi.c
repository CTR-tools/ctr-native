/*
 * Derived from REDRIVER2/PsyCross MIT source:
 * externals/PsyCross/src/psx/LIBAPI.C
 * See THIRD_PARTY_NOTICES.md for copyright and license details.
 */

#include <macros.h>

#include <psx/libapi.h>

// CTR's retail timer reads root counter 1 once per VSync and converts units
// through divisor 0x147e before MainFrame_GameLogic scales to elapsedTimeMS.
// Native owns VBlank emission, so advance RCNT1 from emitted VBlanks instead
// of SDL wall time. Host wait jitter otherwise leaks into vehicle physics.
#define CTR_NATIVE_RCNT1_TICKS_PER_VBLANK 263u

global_variable u64 s_rootCounterValue = 0;
global_variable u64 s_rootCounterBase = 0;

void NativeRCnt_EmitVBlank(void)
{
	s_rootCounterValue += CTR_NATIVE_RCNT1_TICKS_PER_VBLANK;
}

int SetRCnt(int spec, unsigned short target, int mode)
{
	spec &= 0xffff;
	if (spec > 2)
	{
		return 0;
	}

	(void)target;
	(void)mode;
	return 1;
}

int GetRCnt(int spec)
{
	u64 counts;

	(void)spec;

	counts = s_rootCounterValue - s_rootCounterBase;
	if (counts > 0x7fffffff)
	{
		return 0x7fffffff;
	}

	return (int)counts;
}

int StartRCnt(int spec)
{
	spec &= 0xffff;
	if (spec > 2)
	{
		return 0;
	}

	return 1;
}

int StopRCnt(int spec)
{
	(void)spec;
	return 0;
}

int ResetRCnt(int spec)
{
	(void)spec;

	s_rootCounterBase = s_rootCounterValue;
	return 0;
}

int OpenEvent(unsigned int event, int spec, int mode, int32_t (*func)())
{
	(void)event;
	(void)spec;
	(void)mode;
	(void)func;
	return 0;
}

int CloseEvent(unsigned int event)
{
	(void)event;
	return 0;
}

int EnableEvent(unsigned int event)
{
	(void)event;
	return 0;
}

int TestEvent(unsigned int event)
{
	(void)event;
	return 0;
}

void InitCARD(int val)
{
	(void)val;
}

int StartCARD(void)
{
	return 0;
}

int StopCARD(void)
{
	return 0;
}

void _bu_init(void)
{
}
