#include "PSX_SDK_StreamState.h"

void data_ready_callback(void)
{
	register u32 statePage CTR_PSX_REGISTER("$1");
	// NOTE(aalhendi): Prefer the SDK's entry register while the copy scratch values are live.
	StHEADER *entry CTR_PSX_LOCAL_REGISTER_HINT("$3") = (StHEADER *)((u8 *)St_ringBase + (St_state8009ec2c << 5));
	u32 frame;
	u32 next;
	StStreamCallback callback;
	entry->id = StCOMPLETE;
	CTR_PSX_COPY_UNALIGNED_WORD(&St_backloc, &entry->loc);
	frame = entry->frameCount;
	next = St_state8009ec28;
	callback = St_streamCallback1;
	ST_STORE(u32, St_backframe, 5056u, frame);
	ST_STORE(u32, St_state8009ec2c, 5076u, next);
	if (callback != 0)
	{
		callback();
	}
	ST_STORE(u32, St_state8009ec1c, 5092u, 0);
}

// NOTE(aalhendi): CdReadyCallback's status/result are ignored by the SDK's stream thunk.
void St_cdready_callback(u8 status, u8 *result)
{
	(void)status;
	(void)result;
	StCdInterrupt();
}
