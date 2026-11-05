#include "PSX_SDK_StreamState.h"

s32 CdRead2(s32 mode)
{
	register u32 statePage CTR_PSX_REGISTER("$1");
	u8 modeParam = (u8)mode;
	// NOTE(aalhendi): Setmode receives the low byte; CdlModeStream only selects the callbacks.
	CdControl(CdlSetmode, &modeParam, 0);
	if ((mode & CdlModeStream) != 0)
	{
		if ((mode & CdlModeSize1) != 0)
		{
			ST_STORE(u32, St_state8009ec90, 4976u, 0);
		}
		else
		{
			u32 gate = 1;
			ST_STORE(u32, St_state8009ec90, 4976u, gate);
		}
		CdDataCallback(data_ready_callback);
		CdReadyCallback(St_cdready_callback);
	}
	{
		u8 *readParam = 0;
		CTR_PSX_KEEP_VALUE_RELAXED(readParam);
		return CdControl(CdlReadS, readParam, 0);
	}
}
