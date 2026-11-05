#include "PSX_SDK_StreamState.h"

void StSetStream(u32 mode, u32 start_frame, u32 end_frame, StStreamCallback func1, StStreamCallback func2)
{
	register u32 statePage CTR_PSX_REGISTER("$1");
	StSetMask(1, start_frame, end_frame);
	mode &= 1;
	ST_STORE(u32, St_state8009ebf4, 5132u, 0);
	ST_STORE(StStreamCallback, St_streamCallback1, 5140u, func1);
	ST_STORE(u32, St_streamMode, 5112u, mode);
	ST_STORE(u32, St_state8009ebfc, 5124u, 0);
	ST_STORE(u32, St_state8009ec00, 5120u, 0);
	ST_STORE(u16, St_state8009ec18, 5096u, 0);
	ST_STORE(u32, St_state8009ec04, 5116u, 0);
	ST_STORE(StStreamCallback, St_streamCallback2, 5136u, func2);
}
