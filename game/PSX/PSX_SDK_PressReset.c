#include <macros.h>
#include <psx/libetc.h>

extern void psxDctResetHw(s32 mode);

void DecDCTReset(s32 mode)
{
	if (mode == 0)
	{
		ResetCallback();
	}
	psxDctResetHw(mode);
}
