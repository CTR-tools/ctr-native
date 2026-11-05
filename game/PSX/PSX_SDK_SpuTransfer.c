#include "PSX_SDK_SpuTransferState.h"

s32 SpuSetTransferMode(s32 mode)
{
	register u32 statePage CTR_PSX_REGISTER("$1");
	s32 normalized;
	switch (mode)
	{
	case 0:
		normalized = 0;
		break;
	case 1:
		normalized = 1;
		break;
	default:
		normalized = 0;
		break;
	}
	// NOTE(aalhendi): Keep the SDK's two independent signed-low page stores, as in stream state.
	statePage = 0x80090000u;
	CTR_PSX_KEEP_VALUE(statePage);
	*SPU_TRANSFER_STATE(s32, psxSpuTransferModeRaw, statePage, 15548u) = mode;
	statePage = 0x80090000u;
	CTR_PSX_KEEP_VALUE(statePage);
	*SPU_TRANSFER_STATE(s32, psxSpuTransferMode, statePage, 15408u) = normalized;
	return normalized;
}

SpuTransferCallbackProc SpuSetTransferCallback(SpuTransferCallbackProc callback)
{
	SpuTransferCallbackProc previous = psxSpuTransferCallback;
	if (callback != previous)
	{
		psxSpuTransferCallback = callback;
	}
	return previous;
}
