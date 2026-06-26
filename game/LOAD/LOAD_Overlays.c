#include <common.h>

// param_1 = numPlyrCurrGame {1,2,3,4}
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x80033474-0x800334f4.
void LOAD_OvrLOD(u32 param_1)
{
	// change {1-4} -> {0-3}
	param_1 -= 1;

	struct GameTracker *gGT = sdata->gGT;

	// if new LOD overlay needs to load
	if ((u32)gGT->overlayIndex_LOD != param_1)
	{
#ifndef CTR_NATIVE
		// LOD overlay 226-229
		LOAD_AppendQueue(0, LT_SETADDR, BI_OVERLAYSECT2 + param_1, &OVR_Region2, LOAD_Callback_Overlay_Generic);
#endif

		// save ID, and reload next overlay (sector read invalidation)
		gGT->overlayIndex_LOD = param_1;
		gGT->overlayIndex_Threads = 0xff;
	}
	return;
}

// DLL loaded = param_1 + 221
// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800334f4-0x80033570.
void LOAD_OvrEndRace(u32 param_1)
{
	struct GameTracker *gGT = sdata->gGT;

	// if new EndOfRace overlay needs to load
	if ((u32)gGT->overlayIndex_EndOfRace != param_1)
	{
#ifndef CTR_NATIVE
		// EndOfRace overlay 221-225
		LOAD_AppendQueue(0, LT_SETADDR, BI_OVERLAYSECT1 + param_1, &OVR_Region1, LOAD_Callback_Overlay_Generic);
#endif

		gGT->overlayIndex_EndOfRace = param_1;
		gGT->overlayIndex_LOD = 0xff;
	}
	return;
}

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

// NOTE(aalhendi): ASM-verified NTSC-U 926 0x800335dc-0x80033610.
int LOAD_GetAdvPackIndex(void)
{
	int levelID = sdata->gGT->levelID;

	if ((levelID != GEM_STONE_VALLEY) && (levelID != GLACIER_PARK))
	{
		return 1;
	}

	return 2;
}
