#include <common.h>

void OVR_Region3();

#ifdef CTR_NATIVE
static void LOAD_NativeResetThreadsOverlay(u32 overlayIndex)
{
	switch (overlayIndex)
	{
	case 0:
		OVR230_InitData();
		break;
	case 1:
		OVR231_InitData();
		break;
	case 2:
		OVR232_InitData();
		break;
	case 3:
		OVR233_InitData();
		break;
	}
}
#endif

// DLL loaded = param_1 + 230
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80033570-0x800335dc.
void LOAD_OvrThreads(u32 param_1)
{
	struct GameTracker *gGT = sdata->gGT;

	// if new Threads overlay needs to load
	if ((u32)gGT->overlayIndex_Threads != param_1)
	{
#ifndef CTR_NATIVE
		gGT->overlayIndex_Threads = 0xff;
		// Threads overlay 230-233
		LOAD_AppendQueue(0, LT_SETADDR, (param_1 + 0xe6), &OVR_Region3, data.overlayCallbackFuncs[param_1]);
#else
		// NOTE(aalhendi): Native overlays are already linked, so reset the
		// overlay-owned data that retail would refresh by streaming into OVR_Region3.
		gGT->overlayIndex_Threads = 0xff;
		LOAD_NativeResetThreadsOverlay(param_1);
		((void (*)())data.overlayCallbackFuncs[param_1])();
#endif
	}
}
