#include <common.h>

#if defined(CTR_NATIVE)
void PsyX_SPUAL_StopXA(void);
#endif

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x8001cf98-0x8001cfec.
void CDSYS_XAPauseRequest()
{
	if (sdata->boolUseDisc == 0)
	{
#if defined(CTR_NATIVE)
		PsyX_SPUAL_StopXA();
		sdata->XA_State = 0;
		sdata->XA_boolFinished = 0;
		sdata->XA_PauseFrame = sdata->gGT->frameTimer_MainFrame_ResetDB;
#endif
		return;
	}
	if (sdata->bool_XnfLoaded == 0)
		return;
	if (sdata->XA_State < 2)
		return;
	if (sdata->XA_State > 3)
		return;

	sdata->XA_State = 4;
	sdata->XA_VolumeDeduct = 0x400;
}
