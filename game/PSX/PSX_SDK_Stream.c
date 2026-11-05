#include "PSX_SDK_StreamState.h"

void init_ring_status(u32 first, u32 count)
{
	u32 i;
	for (i = 0; i < count;)
	{
		u32 offset = (i + first) * 8;
		u32 *base;
		i++;
		base = St_ringBase;
		base += offset;
		*base = 0;
	}
}

void StClearRing(void)
{
	register u32 statePage CTR_PSX_REGISTER("$1");
	u32 count = St_ringCount;
	CTR_PSX_OBSERVE_VALUE(count);
	ST_STORE(u32, St_state8009ec30, 5072u, 0);
	ST_STORE(u32, St_state8009ec2c, 5076u, 0);
	ST_STORE(u32, St_state8009ec28, 5080u, 0);
	ST_STORE(u32, St_state8009ec1c, 5092u, 0);
	init_ring_status(0, count);
	ST_STORE(u32, St_state8009ebf8, 5128u, 0);
	ST_STORE(u16, St_state8009ec18, 5096u, 0);
	ST_STORE(u32, St_state8009ec04, 5116u, 0);
}

void StSetMask(u32 mask, u32 start, u32 end)
{
	register u32 statePage CTR_PSX_REGISTER("$1");
	ST_STORE(u32, St_mask, 5104u, mask);
	ST_STORE(u32, St_maskStart, 5064u, start);
	ST_STORE(u32, St_maskEnd, 5108u, end);
}

void StSetRing(u32 *address, u32 count)
{
	register u32 statePage CTR_PSX_REGISTER("$1");
	ST_STORE(u32 *, St_ringBase, 5084u, address);
	ST_STORE(u32, St_ringCount, 5068u, count);
	StClearRing();
}
