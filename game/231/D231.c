#include <common.h>

struct OverlayDATA_231 D231 = {.minePoolItem = {0}, .minePoolTaken = 0, .minePoolFree = 0};

#ifdef CTR_NATIVE
static struct OverlayDATA_231 s_d231InitialState;
static int s_d231InitialStateReady;

void OVR231_ResetRuntimeState(void)
{
	if (s_d231InitialStateReady == 0)
	{
		s_d231InitialState = D231;
		s_d231InitialStateReady = 1;
	}

	D231 = s_d231InitialState;
}

void OVR231_InitData(void)
{
	OVR231_ResetRuntimeState();
}
#endif
