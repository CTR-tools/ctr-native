#include <common.h>

#if defined(CTR_NATIVE)
int PsyX_SPUAL_IsXAPlaying(void);
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001d06c-0x8001d094.
void CDSYS_XAPauseAtEnd()
{
	if (sdata->boolUseDisc == 0)
	{
#if defined(CTR_NATIVE)
		if (PsyX_SPUAL_IsXAPlaying() != 0)
			sdata->XA_CurrOffset++;
		else
			sdata->XA_State = 0;
#endif
		return;
	}

	// wait until IRQ says XA is finished
	if (sdata->XA_boolFinished == 0)
		return;

	CDSYS_XAPauseForce();
}
