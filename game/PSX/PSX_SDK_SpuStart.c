#include "PSX_SDK_SpuInternal.h"

void SpuStart(void)
{
	u32 ev;

	if (psxSpuIsCalled == 0)
	{
		// NOTE(aalhendi): Keep the SDK's $at-based stores in the call delay slots.
		register u32 statePage CTR_PSX_REGISTER("$1");
		s32 called = 1;

		CTR_PSX_KEEP_VALUE(called);
		statePage = 0x80090000u;
		CTR_PSX_ORDER_VALUES(statePage, called);
		CTR_PSX_KEEP_VALUE(statePage);
		*(s32 *)(statePage - 15452u) = called;
		EnterCriticalSection();
		_SpuDataCallback(_spu_FiDMA);
		ev = OpenEvent(0xf0000009u, 0x20, 0x2000, NULL);
		{
			register u32 evArg CTR_PSX_REGISTER("$4") = ev;

			statePage = 0x80090000u;
			CTR_PSX_ORDER_VALUES(statePage, evArg);
			CTR_PSX_KEEP_VALUE(statePage);
			*(u32 *)(statePage - 15556u) = evArg;
			EnableEvent(evArg);
		}
		ExitCriticalSection();
	}
}
